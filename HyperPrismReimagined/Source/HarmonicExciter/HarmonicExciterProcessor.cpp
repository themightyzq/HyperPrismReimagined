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
    preparedBlockSize = juce::jmax(1, samplesPerBlock);
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(preparedBlockSize);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());
    
    // Initialize filter. The exciter works on the band ABOVE Frequency; JUCE's
    // LinkwitzRileyFilter would otherwise stay at its default lowpass type.
    highPassFilter.setType(juce::dsp::LinkwitzRileyFilterType::highpass);
    highPassFilter.prepare(spec);
    highPassFilter.setCutoffFrequency(frequencyParam->get());

    highFreqBuffer.setSize(getTotalNumInputChannels(), preparedBlockSize);

    constexpr double smoothingSeconds = 0.03;
    driveSmoothed.reset(sampleRate, smoothingSeconds);
    harmonicsSmoothed.reset(sampleRate, smoothingSeconds);
    mixSmoothed.reset(sampleRate, smoothingSeconds);
    frequencySmoothed.reset(sampleRate, smoothingSeconds);
    driveSmoothed.setCurrentAndTargetValue(driveParam->get() / 100.0f);
    harmonicsSmoothed.setCurrentAndTargetValue(harmonicsParam->get());
    mixSmoothed.setCurrentAndTargetValue(mixParam->get() / 100.0f);
    frequencySmoothed.setCurrentAndTargetValue(frequencyParam->get());
    driveValues.assign((size_t) preparedBlockSize, 0.0f);
    harmonicsValues.assign((size_t) preparedBlockSize, 0.0f);

    int latency = 0;
#if HP_HARMONICEXCITER_FORCE_1X
    oversampling.reset();
#else
    static_assert(kOversamplingFactor == 4, "oversampling stage count below assumes 4x (2 half-band stages)");
    oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        (size_t) juce::jmax(1, getTotalNumInputChannels()),
        (size_t) 2, // log2(kOversamplingFactor)
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,
        true); // integer latency, so the dry delay below can match it exactly
    oversampling->initProcessing((size_t) preparedBlockSize);
    oversampling->reset();
    latency = juce::roundToInt(oversampling->getLatencyInSamples());
#endif

    // setDelay asserts against the maximum, so the maximum is set first.
    dryDelay.setMaximumDelayInSamples(juce::jmax(1, latency));
    dryDelay.prepare(spec);
    dryDelay.setDelay(static_cast<float>(latency));
    dryDelay.reset();

    setLatencySamples(latency);
}

void HarmonicExciterProcessor::releaseResources()
{
    if (oversampling != nullptr)
        oversampling->reset();
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
    const int numSamples = buffer.getNumSamples();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    if (numSamples == 0 || preparedBlockSize <= 0)
        return;

    // Only the input channels carry audio (and only they have filter/oversampler state).
    juce::dsp::AudioBlock<float> fullBlock(buffer.getArrayOfWritePointers(),
                                           static_cast<size_t>(juce::jmin(totalNumInputChannels, buffer.getNumChannels())),
                                           static_cast<size_t>(numSamples));

    // Get parameter values
    const bool bypassed = bypassParamBool->get();
    const int type = typeParam->getIndex();
    driveSmoothed.setTargetValue(driveParam->get() / 100.0f);
    harmonicsSmoothed.setTargetValue(harmonicsParam->get());
    mixSmoothed.setTargetValue(mixParam->get() / 100.0f);
    frequencySmoothed.setTargetValue(juce::jmin(frequencyParam->get(), static_cast<float>(currentSampleRate * 0.45)));

    // The filters, buffer and oversampler are sized for preparedBlockSize; a host that
    // sends a bigger block gets it processed in chunks of at most that size.
    for (int start = 0; start < numSamples; start += preparedBlockSize)
    {
        const int chunk = juce::jmin(preparedBlockSize, numSamples - start);
        auto block = fullBlock.getSubBlock(static_cast<size_t>(start), static_cast<size_t>(chunk));

        // Bypass still runs the dry delay so timing does not jump while latency stays
        // reported.
        if (bypassed)
            delayDryOnly(block);
        else
            processChunk(block, type);
    }

    if (bypassed)
        return;

    // Calculate output level for metering
    float maxLevel = 0.0f;
    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
    {
        auto channelLevel = buffer.getMagnitude(channel, 0, numSamples);
        maxLevel = std::max(maxLevel, channelLevel);
    }
    outputLevel.store(maxLevel);
}

void HarmonicExciterProcessor::delayDryOnly(juce::dsp::AudioBlock<float> block)
{
    const auto n = (int) block.getNumSamples();
    driveSmoothed.skip(n);
    harmonicsSmoothed.skip(n);
    mixSmoothed.skip(n);
    if (frequencySmoothed.isSmoothing())
    {
        frequencySmoothed.skip(n);
        highPassFilter.setCutoffFrequency(frequencySmoothed.getCurrentValue());
    }

    for (size_t channel = 0; channel < block.getNumChannels(); ++channel)
    {
        auto* data = block.getChannelPointer(channel);
        const int ch = static_cast<int>(channel);

        for (size_t sample = 0; sample < block.getNumSamples(); ++sample)
        {
            dryDelay.pushSample(ch, data[sample]);
            data[sample] = dryDelay.popSample(ch);
        }
    }
}

void HarmonicExciterProcessor::processChunk(juce::dsp::AudioBlock<float> block, int type)
{
    const size_t numChannels = block.getNumChannels();
    const size_t numSamples = block.getNumSamples();

    // Copy original to high frequency buffer for filtering
    auto highFreqBlock = juce::dsp::AudioBlock<float>(highFreqBuffer)
                             .getSubsetChannelBlock(0, numChannels)
                             .getSubBlock(0, numSamples);
    highFreqBlock.copyFrom(block);

    // Apply high-pass filter to extract high frequencies (cutoff moves per sample while the
    // Frequency parameter ramps), and collect this chunk's per-sample Drive and Harmonics.
    for (size_t sample = 0; sample < numSamples; ++sample)
    {
        if (frequencySmoothed.isSmoothing())
            highPassFilter.setCutoffFrequency(frequencySmoothed.getNextValue());
        driveValues[sample] = driveSmoothed.getNextValue();
        harmonicsValues[sample] = harmonicsSmoothed.getNextValue();

        for (size_t channel = 0; channel < numChannels; ++channel)
        {
            auto* data = highFreqBlock.getChannelPointer(channel);
            data[sample] = highPassFilter.processSample(static_cast<int>(channel), data[sample]);
        }
    }

    // The harmonic generator is the nonlinear stage: run it 4x oversampled so the
    // harmonics it manufactures are pushed above the base Nyquist before the
    // downsampling half-band filter removes them (HP_HARMONICEXCITER_FORCE_1X=1
    // disables this for an A/B comparison).
#if HP_HARMONICEXCITER_FORCE_1X
    for (size_t channel = 0; channel < numChannels; ++channel)
    {
        auto* highFreqData = highFreqBlock.getChannelPointer(channel);

        for (size_t sample = 0; sample < numSamples; ++sample)
        {
            highFreqData[sample] = (type == 0)
                ? generateWarmHarmonics(highFreqData[sample], driveValues[sample], harmonicsValues[sample])
                : generateBrightHarmonics(highFreqData[sample], driveValues[sample], harmonicsValues[sample]);
        }
    }
#else
    jassert(oversampling != nullptr);
    auto oversampledBlock = oversampling->processSamplesUp(highFreqBlock);

    for (size_t channel = 0; channel < oversampledBlock.getNumChannels(); ++channel)
    {
        auto* data = oversampledBlock.getChannelPointer(channel);

        for (size_t sample = 0; sample < oversampledBlock.getNumSamples(); ++sample)
        {
            const auto base = sample / (size_t) kOversamplingFactor;
            data[sample] = (type == 0)
                ? generateWarmHarmonics(data[sample], driveValues[base], harmonicsValues[base])
                : generateBrightHarmonics(data[sample], driveValues[base], harmonicsValues[base]);
        }
    }

    oversampling->processSamplesDown(highFreqBlock);
#endif

    // Mix the dry signal, delayed by the oversampler latency, with the harmonic signal
    for (size_t sample = 0; sample < numSamples; ++sample)
    {
        const float mix = mixSmoothed.getNextValue();

        for (size_t channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = block.getChannelPointer(channel);
            const auto* highFreqData = highFreqBlock.getChannelPointer(channel);
            const int ch = static_cast<int>(channel);

            dryDelay.pushSample(ch, channelData[sample]);
            channelData[sample] = dryDelay.popSample(ch) + (highFreqData[sample] * mix);
        }
    }
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
    state.setProperty("editor_width", editorWidth.load(), nullptr);
    state.setProperty("editor_height", editorHeight.load(), nullptr);
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
        auto state = juce::ValueTree::fromXml(*xmlState);
        setEditorSize(static_cast<int>(state.getProperty("editor_width", 0)),
                      static_cast<int>(state.getProperty("editor_height", 0)));
        state.removeProperty("editor_width", nullptr);
        state.removeProperty("editor_height", nullptr);
        valueTreeState.replaceState(state);
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