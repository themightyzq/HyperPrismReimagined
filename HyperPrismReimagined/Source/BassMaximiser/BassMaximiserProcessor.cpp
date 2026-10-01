#include "BassMaximiserProcessor.h"
#include "BassMaximiserEditor.h"

BassMaximiserProcessor::BassMaximiserProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters",
      {
          std::make_unique<juce::AudioParameterFloat>("frequency", "Frequency", 
              juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f, 0.3f), 80.0f, "Hz"),
          std::make_unique<juce::AudioParameterFloat>("boost", "Boost", 
              juce::NormalisableRange<float>(0.0f, 20.0f, 0.1f), 6.0f, "dB"),
          std::make_unique<juce::AudioParameterFloat>("harmonics", "Harmonics", 
              juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 25.0f, "%"),
          std::make_unique<juce::AudioParameterFloat>("tightness", "Tightness", 
              juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 50.0f, "%"),
          std::make_unique<juce::AudioParameterFloat>("outputGain", "Output Gain", 
              juce::NormalisableRange<float>(-20.0f, 20.0f, 0.1f), 0.0f, "dB"),
          std::make_unique<juce::AudioParameterBool>("phaseInvert", "Phase Invert", false),
          std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false)
      })
{
    // Get parameter references
    frequencyParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("frequency"));
    boostParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("boost"));
    harmonicsParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("harmonics"));
    tightnessParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("tightness"));
    outputGainParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("outputGain"));
    phaseInvertParam = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("phaseInvert"));
    bypassParam = apvts.getRawParameterValue("bypass");
}

BassMaximiserProcessor::~BassMaximiserProcessor()
{
}

const juce::String BassMaximiserProcessor::getName() const
{
    return JucePlugin_Name;
}

bool BassMaximiserProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool BassMaximiserProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool BassMaximiserProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double BassMaximiserProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int BassMaximiserProcessor::getNumPrograms()
{
    return 1;
}

int BassMaximiserProcessor::getCurrentProgram()
{
    return 0;
}

void BassMaximiserProcessor::setCurrentProgram(int)
{
}

const juce::String BassMaximiserProcessor::getProgramName(int)
{
    return {};
}

void BassMaximiserProcessor::changeProgramName(int, const juce::String&)
{
}

void BassMaximiserProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;
    
    // Initialize filters
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 2;

    const float crossoverHz = juce::jmin(frequencyParam->get(), static_cast<float>(sampleRate * 0.45));
    crossover.prepare(spec);
    crossover.setCutoffFrequency(crossoverHz);
    subFilter.setType(juce::dsp::LinkwitzRileyFilterType::lowpass);
    subFilter.prepare(spec);
    subFilter.setCutoffFrequency(crossoverHz);
    for (int ch = 0; ch < 2; ++ch)
    {
        subFlipFlop[ch] = 1.0f;
        subArmed[ch] = false;
    }

    // Initialize bass processing arrays
    bassEnvelopes.assign(2, 0.0f);
    bassGainReduction.assign(2, 1.0f);

    // Initialize smoothers
    bassLevelSmoother.reset(sampleRate, 0.1);
    bassLevelSmoother.setCurrentAndTargetValue(0.0f);
    outputGainSmoother.reset(sampleRate, 0.05);
    outputGainSmoother.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(outputGainParam->get()));
    frequencySmoother.reset(sampleRate, 0.03);
    frequencySmoother.setCurrentAndTargetValue(crossoverHz);
    boostGainSmoother.reset(sampleRate, 0.03);
    boostGainSmoother.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(boostParam->get()));
    harmonicsSmoother.reset(sampleRate, 0.03);
    harmonicsSmoother.setCurrentAndTargetValue(harmonicsParam->get() / 100.0f);
}

void BassMaximiserProcessor::releaseResources()
{
    crossover.reset();
    subFilter.reset();
}

bool BassMaximiserProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void BassMaximiserProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    if (bypassParam->load() > 0.5f)
        return;

    // Frequency (the bass/high crossover point), Boost and Harmonics ramp to their targets;
    // tightness and phase invert are read once per block.
    frequencySmoother.setTargetValue(juce::jmin(frequencyParam->get(), static_cast<float>(currentSampleRate * 0.45)));
    boostGainSmoother.setTargetValue(juce::Decibels::decibelsToGain(boostParam->get()));
    harmonicsSmoother.setTargetValue(harmonicsParam->get() / 100.0f);
    outputGainSmoother.setTargetValue(juce::Decibels::decibelsToGain(outputGainParam->get()));
    const float tightness = tightnessParam->get() / 100.0f;
    const bool phaseInvert = phaseInvertParam->get();

    float totalBassLevel = 0.0f;
    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin(totalNumInputChannels, buffer.getNumChannels(), 2);

    // Samples outer, channels inner, so every smoother advances once per sample.
    for (int sample = 0; sample < numSamples; ++sample)
    {
        if (frequencySmoother.isSmoothing())
        {
            const float hz = frequencySmoother.getNextValue();
            crossover.setCutoffFrequency(hz);
            subFilter.setCutoffFrequency(hz);
        }
        const float boostGain = boostGainSmoother.getNextValue();
        const float harmonics = harmonicsSmoother.getNextValue();
        const float outputGain = outputGainSmoother.getNextValue();

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            const float input = channelData[sample];

            // Split signal into bass and high frequencies (Linkwitz-Riley: bass + high = input,
            // all-passed, so the bands sum flat when nothing below changes them).
            float bassSignal = 0.0f, highSignal = 0.0f;
            crossover.processSample(channel, input, bassSignal, highSignal);

            // Apply boost to bass signal
            const float boostedBass = bassSignal * boostGain;

            // Generate the sub-octave and keep only the band below the crossover
            const float subHarmonic = subFilter.processSample(channel, generateSubHarmonic(boostedBass, channel));

            // Apply bass compression/limiting (tightness)
            float processedBass = processBassCompression(boostedBass, bassEnvelopes[static_cast<size_t>(channel)],
                                                         bassGainReduction[static_cast<size_t>(channel)], tightness);

            // Apply phase invert if enabled
            if (phaseInvert)
                processedBass = -processedBass;

            // Combine bass, sub-harmonics, and high frequencies, then output gain
            channelData[sample] = (processedBass + subHarmonic * harmonics + highSignal) * outputGain;

            // Accumulate bass level for metering (only channel 0 for stereo linking)
            if (channel == 0)
                totalBassLevel += processedBass * processedBass;
        }
    }

    // Update bass level meter (RMS)
    float rmsLevel = numSamples > 0 ? std::sqrt(totalBassLevel / numSamples) : 0.0f;
    bassLevelSmoother.setTargetValue(rmsLevel);
    currentBassLevel.store(bassLevelSmoother.getNextValue());
}

bool BassMaximiserProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* BassMaximiserProcessor::createEditor()
{
    return new BassMaximiserEditor(*this);
}

void BassMaximiserProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("editor_width", getEditorWidth(), nullptr);
    state.setProperty("editor_height", getEditorHeight(), nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void BassMaximiserProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
        {
            auto newState = juce::ValueTree::fromXml(*xmlState);
            setEditorSize((int) newState.getProperty("editor_width", 0), (int) newState.getProperty("editor_height", 0));
            newState.removeProperty("editor_width", nullptr);
            newState.removeProperty("editor_height", nullptr);
            apvts.replaceState(newState);
        }
}

float BassMaximiserProcessor::generateSubHarmonic(float input, int channel)
{
    // Octave divider. The flip-flop toggles on each upward zero crossing of the bass band, so
    // it is a square wave at half the bass frequency; multiplying the band by it puts a
    // component one octave down (and one at 1.5x, which the low-pass after this removes).
    // The crossing needs the signal to have gone below -hysteresis first, so low-level noise
    // around zero cannot chatter it. (The previous generator started its phase at 0 and only
    // advanced it while the phase was already non-zero, so it never produced anything.)
    constexpr float hysteresis = 1.0e-3f;
    const auto ch = static_cast<size_t>(juce::jlimit(0, 1, channel));

    if (input < -hysteresis)
    {
        subArmed[ch] = true;
    }
    else if (input > hysteresis && subArmed[ch])
    {
        subArmed[ch] = false;
        subFlipFlop[ch] = -subFlipFlop[ch];
    }

    return input * subFlipFlop[ch];
}

float BassMaximiserProcessor::processBassCompression(float input, float& envelope, float& gainReduction,
                                                   float tightness)
{
    // INVESTIGATED (was flagged as a possible bug: this function used to take an unused
    // `frequency` argument). No audible defect: the crossover frequency IS applied to the
    // sound -- processBlock() sets the Linkwitz-Riley crossover from the Frequency parameter,
    // and the `input` this function receives (boostedBass) is already that crossover's
    // low band. The
    // Frequency control's tooltip ("Crossover frequency -- bass below this point is boosted",
    // see BassMaximiserEditor.cpp) is satisfied by that band split; this function's own
    // attack/release/threshold constants govern the tightness (compression) shaping applied
    // after the crossover, and were never meant to vary with it. The redundant argument is
    // removed; see CHANGELOG.

    if (tightness <= 0.0f)
        return input;

    // Simple envelope follower
    float absInput = std::abs(input);
    float attack = 0.01f;  // Fast attack
    float release = 0.1f;  // Slower release
    
    if (absInput > envelope)
    {
        envelope += (absInput - envelope) * attack;
    }
    else
    {
        envelope += (absInput - envelope) * release;
    }
    
    // Calculate gain reduction based on envelope and tightness
    float threshold = 0.5f;  // Fixed threshold for simplicity
    float ratio = 1.0f + (tightness * 9.0f);  // 1:1 to 10:1 ratio
    
    if (envelope > threshold)
    {
        float excess = envelope - threshold;
        float compressedExcess = excess / ratio;
        float targetGain = (threshold + compressedExcess) / envelope;
        gainReduction = targetGain * tightness + (1.0f - tightness);
    }
    else
    {
        gainReduction = 1.0f;
    }
    
    return input * gainReduction;
}

float BassMaximiserProcessor::calculateRMS(const float* buffer, int numSamples)
{
    float sum = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        sum += buffer[static_cast<size_t>(i)] * buffer[static_cast<size_t>(i)];
    }
    return std::sqrt(sum / numSamples);
}