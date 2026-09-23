#include "HarmonicExciterProcessor.h"
#include "HarmonicExciterEditor.h"

HarmonicExciterProcessor::HarmonicExciterProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      valueTreeState(*this, nullptr, stateType, createParameterLayout())
{
    driveParam      = dynamic_cast<juce::AudioParameterFloat*>(valueTreeState.getParameter("drive"));
    frequencyParam  = dynamic_cast<juce::AudioParameterFloat*>(valueTreeState.getParameter("frequency"));
    harmonicsParam  = dynamic_cast<juce::AudioParameterFloat*>(valueTreeState.getParameter("harmonics"));
    mixParam        = dynamic_cast<juce::AudioParameterFloat*>(valueTreeState.getParameter("mix"));
    typeParam       = dynamic_cast<juce::AudioParameterChoice*>(valueTreeState.getParameter("type"));
    bypassParamBool = dynamic_cast<juce::AudioParameterBool*>(valueTreeState.getParameter("bypass"));
    jassert(driveParam && frequencyParam && harmonicsParam && mixParam && typeParam && bypassParamBool);
}

juce::AudioProcessorValueTreeState::ParameterLayout HarmonicExciterProcessor::createParameterLayout()
{
    // Same IDs, names, ranges, defaults and order as the pre-migration addParameter calls.
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        "drive", "Drive",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 30.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " %"; }));

    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        "frequency", "Frequency",
        juce::NormalisableRange<float>(1000.0f, 20000.0f, 1.0f, 0.3f), 5000.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(static_cast<int>(value)) + " Hz"; }));

    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        "harmonics", "Harmonics",
        juce::NormalisableRange<float>(1.0f, 5.0f, 0.1f), 2.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1); }));

    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        "mix", "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " %"; }));

    parameters.push_back(std::make_unique<juce::AudioParameterChoice>(
        "type", "Type", juce::StringArray("Warm", "Bright"), 0));

    parameters.push_back(std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false));

    return { parameters.begin(), parameters.end() };
}

HarmonicExciterProcessor::~HarmonicExciterProcessor()
{
}

const juce::String HarmonicExciterProcessor::getName() const
{
    return "HyperPrism Reimagined Harmonic Exciter";
}

bool HarmonicExciterProcessor::acceptsMidi() const
{
    return false;
}

bool HarmonicExciterProcessor::producesMidi() const
{
    return false;
}

bool HarmonicExciterProcessor::isMidiEffect() const
{
    return false;
}

double HarmonicExciterProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int HarmonicExciterProcessor::getNumPrograms()
{
    return 1;
}

int HarmonicExciterProcessor::getCurrentProgram()
{
    return 0;
}

void HarmonicExciterProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String HarmonicExciterProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void HarmonicExciterProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void HarmonicExciterProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    
    // Initialize filters
    highPassFilter.prepare(spec);
    lowPassFilter.prepare(spec);
    
    // Set initial filter frequencies
    highPassFilter.setCutoffFrequency(frequencyParam->get());
    lowPassFilter.setCutoffFrequency(frequencyParam->get());

    dryBuffer.setSize(getTotalNumInputChannels(), samplesPerBlock);
    highFreqBuffer.setSize(getTotalNumInputChannels(), samplesPerBlock);
}

void HarmonicExciterProcessor::releaseResources()
{
}

bool HarmonicExciterProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void HarmonicExciterProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    if (bypassParamBool->get())
        return;

    // Get parameter values
    const float drive = driveParam->get() / 100.0f;
    const float frequency = frequencyParam->get();
    const float harmonics = harmonicsParam->get();
    const float mix = mixParam->get() / 100.0f;
    const int type = typeParam->getIndex();

    // Update filter frequencies
    highPassFilter.setCutoffFrequency(frequency);
    lowPassFilter.setCutoffFrequency(frequency);

    // Use pre-allocated buffers for processing
    dryBuffer.setSize(totalNumInputChannels, buffer.getNumSamples(), false, false, true);
    highFreqBuffer.setSize(totalNumInputChannels, buffer.getNumSamples(), false, false, true);
    
    // Copy original signal to dry buffer
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
        dryBuffer.copyFrom(channel, 0, buffer, channel, 0, buffer.getNumSamples());

    // Copy original to high frequency buffer for filtering
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
        highFreqBuffer.copyFrom(channel, 0, buffer, channel, 0, buffer.getNumSamples());

    // Apply high-pass filter to extract high frequencies
    juce::dsp::AudioBlock<float> highFreqBlock(highFreqBuffer);
    juce::dsp::ProcessContextReplacing<float> highFreqContext(highFreqBlock);
    highPassFilter.process(highFreqContext);

    // Process each channel
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        auto* highFreqData = highFreqBuffer.getReadPointer(channel);
        auto* dryData = dryBuffer.getReadPointer(channel);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float input = highFreqData[sample];
            float processedSample;

            // Apply harmonic generation based on type
            if (type == 0) // Warm
            {
                processedSample = generateWarmHarmonics(input, drive, harmonics);
            }
            else // Bright
            {
                processedSample = generateBrightHarmonics(input, drive, harmonics);
            }

            // Mix dry and processed signals
            channelData[sample] = dryData[sample] + (processedSample * mix);
        }
    }

    // Calculate output level for metering
    float maxLevel = 0.0f;
    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
    {
        auto channelLevel = buffer.getMagnitude(channel, 0, buffer.getNumSamples());
        maxLevel = std::max(maxLevel, channelLevel);
    }
    outputLevel.store(maxLevel);
}

float HarmonicExciterProcessor::generateWarmHarmonics(float input, float drive, float harmonics)
{
    // Warm algorithm - emphasizes even harmonics with soft saturation
    float driven = input * (1.0f + drive * 9.0f); // Scale drive 0-10x
    
    // Soft saturation using hyperbolic tangent
    float saturated = std::tanh(driven * harmonics);
    
    // Add subtle even harmonic content
    float evenHarmonic = std::sin(saturated * juce::MathConstants<float>::pi * 0.5f) * 0.3f;
    
    return saturated + evenHarmonic * drive;
}

float HarmonicExciterProcessor::generateBrightHarmonics(float input, float drive, float harmonics)
{
    // Bright algorithm - emphasizes odd harmonics with hard clipping
    float driven = input * (1.0f + drive * 9.0f);
    
    // Hard clipping with cubic shaping
    float clipped = juce::jlimit(-1.0f, 1.0f, driven * harmonics);
    float cubic = clipped - (clipped * clipped * clipped) / 3.0f;
    
    // Add odd harmonic content
    float oddHarmonic = std::sin(cubic * juce::MathConstants<float>::pi) * 0.4f;
    
    return cubic + oddHarmonic * drive;
}

bool HarmonicExciterProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* HarmonicExciterProcessor::createEditor()
{
    return new HarmonicExciterEditor(*this);
}

void HarmonicExciterProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = valueTreeState.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void HarmonicExciterProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState == nullptr)
        return;

    if (xmlState->hasTagName(valueTreeState.state.getType()))
    {
        valueTreeState.replaceState(juce::ValueTree::fromXml(*xmlState));
        return;
    }

    if (xmlState->hasTagName(legacyStateTag))
    {
        // Pre-migration session: one attribute per parameter in real units, type as an
        // index, no bypass (it was never saved). Restore through the parameters so the
        // APVTS tree, the host and the editor all see the same values.
        auto setReal = [](juce::RangedAudioParameter* p, double value)
        {
            if (p != nullptr)
                p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(value)));
        };
        setReal(driveParam,     xmlState->getDoubleAttribute("drive",     driveParam->get()));
        setReal(frequencyParam, xmlState->getDoubleAttribute("frequency", frequencyParam->get()));
        setReal(harmonicsParam, xmlState->getDoubleAttribute("harmonics", harmonicsParam->get()));
        setReal(mixParam,       xmlState->getDoubleAttribute("mix",       mixParam->get()));
        setReal(typeParam,      xmlState->getIntAttribute("type",         typeParam->getIndex()));
    }
}