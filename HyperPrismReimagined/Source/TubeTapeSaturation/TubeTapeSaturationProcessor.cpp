//==============================================================================
// HyperPrism Reimagined - Tube/Tape Saturation Processor
//==============================================================================

#include "TubeTapeSaturationProcessor.h"
#include "TubeTapeSaturationEditor.h"

// Parameter IDs
const juce::String TubeTapeSaturationProcessor::BYPASS_ID = "bypass";
const juce::String TubeTapeSaturationProcessor::DRIVE_ID = "drive";
const juce::String TubeTapeSaturationProcessor::TYPE_ID = "type";
const juce::String TubeTapeSaturationProcessor::WARMTH_ID = "warmth";
const juce::String TubeTapeSaturationProcessor::BRIGHTNESS_ID = "brightness";
const juce::String TubeTapeSaturationProcessor::OUTPUT_LEVEL_ID = "outputLevel";

//==============================================================================
TubeTapeSaturationProcessor::TubeTapeSaturationProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      valueTreeState(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache parameter pointers for performance
    bypassParam = valueTreeState.getRawParameterValue(BYPASS_ID);
    driveParam = valueTreeState.getRawParameterValue(DRIVE_ID);
    typeParam = valueTreeState.getRawParameterValue(TYPE_ID);
    warmthParam = valueTreeState.getRawParameterValue(WARMTH_ID);
    brightnessParam = valueTreeState.getRawParameterValue(BRIGHTNESS_ID);
    outputLevelParam = valueTreeState.getRawParameterValue(OUTPUT_LEVEL_ID);
}

juce::AudioProcessorValueTreeState::ParameterLayout TubeTapeSaturationProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    // Bypass
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID, "Bypass", false));

    // Drive (0-100%)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DRIVE_ID, "Drive", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 25.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + "%"; }));

    // Type (Tube/Tape/Transformer)
    parameters.push_back(std::make_unique<juce::AudioParameterChoice>(
        TYPE_ID, "Type", 
        juce::StringArray{"Tube", "Tape", "Transformer"}, 0));

    // Warmth (0-100%) - low-frequency saturation character
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        WARMTH_ID, "Warmth", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + "%"; }));

    // Brightness (0-100%) - high-frequency saturation character
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        BRIGHTNESS_ID, "Brightness", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + "%"; }));

    // Output Level (-20 to +20 dB)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        OUTPUT_LEVEL_ID, "Output Level", 
        juce::NormalisableRange<float>(-20.0f, 20.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    return { parameters.begin(), parameters.end() };
}

//==============================================================================
void TubeTapeSaturationProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    preparedBlockSize = juce::jmax(1, samplesPerBlock);
    const int numChannels = juce::jmax(1, juce::jmin(getTotalNumInputChannels(), kMaxChannels));

    // Initialize filters
    juce::IIRCoefficients dcBlockCoeffs = juce::IIRCoefficients::makeHighPass(sampleRate, 20.0);
    dcBlockLeft.setCoefficients(dcBlockCoeffs);
    dcBlockRight.setCoefficients(dcBlockCoeffs);
    dcBlockLeft.reset();
    dcBlockRight.reset();

    // Parameter smoothing starts at the current values (no ramp on the first block).
    constexpr double smoothingSeconds = 0.03;
    driveSmoothed.reset(sampleRate, smoothingSeconds);
    warmthSmoothed.reset(sampleRate, smoothingSeconds);
    brightnessSmoothed.reset(sampleRate, smoothingSeconds);
    outputGainSmoothed.reset(sampleRate, smoothingSeconds);
    driveSmoothed.setCurrentAndTargetValue(driveParam->load() / 100.0f);
    warmthSmoothed.setCurrentAndTargetValue(warmthParam->load() / 100.0f);
    brightnessSmoothed.setCurrentAndTargetValue(brightnessParam->load() / 100.0f);
    outputGainSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(outputLevelParam->load()));

    driveValues.assign((size_t) preparedBlockSize, 0.0f);
    warmthValues.assign((size_t) preparedBlockSize, 0.0f);
    brightnessValues.assign((size_t) preparedBlockSize, 0.0f);

    // Initialize shelf filters for warmth and brightness
    previousWarmth = -1.0f;
    previousBrightness = -1.0f;
    updateShelfFilters(warmthSmoothed.getCurrentValue(), brightnessSmoothed.getCurrentValue());
    lowShelfLeft.reset();
    lowShelfRight.reset();
    highShelfLeft.reset();
    highShelfRight.reset();

    hysteresisMemory.fill(0.0f);

    // Reset processing state
    previousInputRMS = 0.0f;
    previousOutputRMS = 0.0f;
    harmonicContent.store(0.0f);

    int latency = 0;
#if HP_TUBETAPE_FORCE_1X
    oversampling.reset();
#else
    static_assert(kOversamplingFactor == 4, "oversampling stage count below assumes 4x (2 half-band stages)");
    oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        (size_t) numChannels,
        (size_t) 2, // log2(kOversamplingFactor)
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,
        true); // integer latency, so bypass can be delayed by exactly the same whole number of samples
    oversampling->initProcessing((size_t) preparedBlockSize);
    oversampling->reset();
    latency = juce::roundToInt(oversampling->getLatencyInSamples());
#endif

    // setDelay asserts against the maximum, so the maximum is set first.
    juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32) preparedBlockSize, (juce::uint32) numChannels };
    dryDelay.setMaximumDelayInSamples(juce::jmax(1, latency));
    dryDelay.prepare(spec);
    dryDelay.setDelay(static_cast<float>(latency));
    dryDelay.reset();

    setLatencySamples(latency);
}

void TubeTapeSaturationProcessor::releaseResources()
{
    // Reset filters
    dcBlockLeft.reset();
    dcBlockRight.reset();
    lowShelfLeft.reset();
    lowShelfRight.reset();
    highShelfLeft.reset();
    highShelfRight.reset();

    if (oversampling != nullptr)
        oversampling->reset();
}

bool TubeTapeSaturationProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void TubeTapeSaturationProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    if (numSamples == 0 || preparedBlockSize <= 0)
        return;

    const bool bypassed = bypassParam->load() > 0.5f;

    driveSmoothed.setTargetValue(driveParam->load() / 100.0f);
    warmthSmoothed.setTargetValue(warmthParam->load() / 100.0f);
    brightnessSmoothed.setTargetValue(brightnessParam->load() / 100.0f);
    outputGainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(outputLevelParam->load()));

    // Only the input channels carry audio (and only they have filter/oversampler state).
    const int numChannels = juce::jmin(totalNumInputChannels, buffer.getNumChannels(), kMaxChannels);
    juce::dsp::AudioBlock<float> fullBlock(buffer.getArrayOfWritePointers(),
                                           static_cast<size_t>(numChannels),
                                           static_cast<size_t>(numSamples));

    // The oversampler and the per-sample parameter buffers are sized for preparedBlockSize; a
    // host that sends a bigger block gets it processed in chunks of at most that size.
    for (int start = 0; start < numSamples; start += preparedBlockSize)
    {
        const int chunk = juce::jmin(preparedBlockSize, numSamples - start);
        auto block = fullBlock.getSubBlock(static_cast<size_t>(start), static_cast<size_t>(chunk));

        // Bypass still runs the latency delay so timing does not jump while latency stays
        // reported.
        if (bypassed)
            delayDryOnly(block);
        else
            processChunk(block);
    }

    if (! bypassed)
        calculateHarmonicContent(buffer);
}

void TubeTapeSaturationProcessor::delayDryOnly(juce::dsp::AudioBlock<float> block)
{
    const auto n = (int) block.getNumSamples();
    driveSmoothed.skip(n);
    warmthSmoothed.skip(n);
    brightnessSmoothed.skip(n);
    outputGainSmoothed.skip(n);

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

void TubeTapeSaturationProcessor::processChunk(juce::dsp::AudioBlock<float> block)
{
    const int numChannels = static_cast<int>(block.getNumChannels());
    const int numSamples = static_cast<int>(block.getNumSamples());

    // Calculate input level, and keep the bypass delay line fed with the input so switching
    // to bypass continues from the right audio.
    float inputSum = 0.0f;
    for (int channel = 0; channel < numChannels; ++channel)
    {
        const auto* channelData = block.getChannelPointer(static_cast<size_t>(channel));
        for (int sample = 0; sample < numSamples; ++sample)
        {
            inputSum += std::abs(channelData[sample]);
            dryDelay.pushSample(channel, channelData[sample]);
            dryDelay.popSample(channel);
        }
    }
    inputLevel.store(inputSum / static_cast<float>(numChannels * numSamples));

    const int type = static_cast<int>(typeParam->load());

    // Per-sample parameter values for this chunk.
    for (int i = 0; i < numSamples; ++i)
    {
        driveValues[(size_t) i] = driveSmoothed.getNextValue();
        warmthValues[(size_t) i] = warmthSmoothed.getNextValue();
        brightnessValues[(size_t) i] = brightnessSmoothed.getNextValue();
    }

    // Pre-filtering for warmth/brightness shaping, at the base sample rate (linear, does not
    // need oversampling). The shelves are rebuilt every kShelfUpdateInterval samples while those
    // parameters move; juce::IIRCoefficients is a plain value type, so this does not allocate.
    for (int start = 0; start < numSamples; start += kShelfUpdateInterval)
    {
        const int end = juce::jmin(numSamples, start + kShelfUpdateInterval);
        updateShelfFilters(warmthValues[(size_t) start], brightnessValues[(size_t) start]);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = block.getChannelPointer(static_cast<size_t>(channel));
            auto& lowShelf = (channel == 0) ? lowShelfLeft : lowShelfRight;
            auto& highShelf = (channel == 0) ? highShelfLeft : highShelfRight;

            for (int sample = start; sample < end; ++sample)
            {
                float processed = lowShelf.processSingleSampleRaw(channelData[sample]);
                channelData[sample] = highShelf.processSingleSampleRaw(processed);
            }
        }
    }

    auto shape = [this, type] (float x, float drive, float warmth, float brightness, int channel)
    {
        switch (type)
        {
            case Tube:        return processTubeSaturation(x, drive, warmth, brightness);
            case Tape:        return processTapeSaturation(x, drive, warmth, brightness);
            case Transformer: return processTransformerSaturation(x, drive, warmth, brightness, channel);
            default:          return x;
        }
    };

    // The waveshaper is the nonlinear stage: run it 4x oversampled so the harmonics it
    // generates are pushed above the base Nyquist before the downsampling half-band
    // filter removes them (HP_TUBETAPE_FORCE_1X=1 disables this for an A/B comparison).
#if HP_TUBETAPE_FORCE_1X
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = block.getChannelPointer(static_cast<size_t>(channel));
        for (int sample = 0; sample < numSamples; ++sample)
            channelData[sample] = shape(channelData[sample], driveValues[(size_t) sample],
                                        warmthValues[(size_t) sample], brightnessValues[(size_t) sample], channel);
    }
#else
    jassert(oversampling != nullptr);
    auto oversampledBlock = oversampling->processSamplesUp(block);

    for (size_t channel = 0; channel < oversampledBlock.getNumChannels(); ++channel)
    {
        auto* data = oversampledBlock.getChannelPointer(channel);

        for (size_t sample = 0; sample < oversampledBlock.getNumSamples(); ++sample)
        {
            const auto base = sample / (size_t) kOversamplingFactor;
            data[sample] = shape(data[sample], driveValues[base], warmthValues[base], brightnessValues[base],
                                 static_cast<int>(channel));
        }
    }

    oversampling->processSamplesDown(block);
#endif

    // DC blocking + output level, at the base sample rate.
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float gain = outputGainSmoothed.getNextValue();
        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = block.getChannelPointer(static_cast<size_t>(channel));
            auto& dcBlock = (channel == 0) ? dcBlockLeft : dcBlockRight;
            channelData[sample] = dcBlock.processSingleSampleRaw(channelData[sample]) * gain;
        }
    }
}

void TubeTapeSaturationProcessor::updateShelfFilters(float warmth, float brightness)
{
    if (std::abs(warmth - previousWarmth) > 0.001f ||
        std::abs(brightness - previousBrightness) > 0.001f)
    {
        // Warmth control - low shelf filter (80Hz)
        float warmthGain = juce::jmap(warmth, 0.0f, 1.0f, -6.0f, 6.0f);
        auto lowShelfCoeffs = juce::IIRCoefficients::makeLowShelf(currentSampleRate, 80.0, 0.7, juce::Decibels::decibelsToGain(warmthGain));
        lowShelfLeft.setCoefficients(lowShelfCoeffs);
        lowShelfRight.setCoefficients(lowShelfCoeffs);

        // Brightness control - high shelf filter (8kHz)
        float brightnessGain = juce::jmap(brightness, 0.0f, 1.0f, -6.0f, 6.0f);
        auto highShelfCoeffs = juce::IIRCoefficients::makeHighShelf(currentSampleRate, 8000.0, 0.7, juce::Decibels::decibelsToGain(brightnessGain));
        highShelfLeft.setCoefficients(highShelfCoeffs);
        highShelfRight.setCoefficients(highShelfCoeffs);

        previousWarmth = warmth;
        previousBrightness = brightness;
    }
}

void TubeTapeSaturationProcessor::calculateHarmonicContent(const juce::AudioBuffer<float>& buffer)
{
    // Simple harmonic content estimation based on RMS difference
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numSamples == 0) return;
    
    float sumSquares = 0.0f;
    for (int channel = 0; channel < numChannels; ++channel)
    {
        const auto* channelData = buffer.getReadPointer(channel);
        for (int sample = 0; sample < numSamples; ++sample)
        {
            sumSquares += channelData[static_cast<size_t>(sample)] * channelData[static_cast<size_t>(sample)];
        }
    }
    
    float currentRMS = std::sqrt(sumSquares / (numChannels * numSamples));
    
    // Update output level
    outputLevel.store(currentRMS);
    
    // Estimate harmonic content based on RMS change and drive amount
    float drive = driveParam->load() / 100.0f;
    float harmonicEstimate = std::min(1.0f, drive * currentRMS * 2.0f);
    
    // Smooth the harmonic content for display
    const float smoothingFactor = 0.95f;
    harmonicContent.store(harmonicContent.load() * smoothingFactor + harmonicEstimate * (1.0f - smoothingFactor));
}

// Tube saturation - warm, musical distortion with even harmonics
float TubeTapeSaturationProcessor::processTubeSaturation(float input, float drive, float warmth, float brightness)
{
    // Scale input based on drive
    float scaledInput = input * (1.0f + drive * 4.0f);
    
    // Asymmetric tube-style saturation
    float output;
    if (scaledInput > 0.0f)
    {
        // Stronger positive saturation for tube-like asymmetry
        output = tanhSaturation(scaledInput, 1.0f + drive * 1.5f);
        // Add second harmonic emphasis
        output += std::sin(2.0f * scaledInput) * drive * 0.1f;
    }
    else
    {
        // Less saturation on negative half for tube-like asymmetry
        output = tanhSaturation(scaledInput, 0.5f + drive * 0.7f);
    }
    
    // Add warmth-dependent compression and harmonic emphasis
    output = softClip(output, warmth * 0.4f);
    
    // Apply brightness - tube saturation reduces high frequencies
    float brightnessFactor = 1.0f - (1.0f - brightness) * 0.3f;
    output *= brightnessFactor;
    
    // Compensate for level increase
    return output * (0.8f / (1.0f + drive * 0.3f));
}

// Tape saturation - smooth compression with high-frequency rolloff
float TubeTapeSaturationProcessor::processTapeSaturation(float input, float drive, float warmth, float brightness)
{
    // Tape-style smooth saturation with compression
    float scaledInput = input * (1.0f + drive * 2.5f);
    
    // Symmetric tape-style compression with subtle third harmonic
    float output = tanhSaturation(scaledInput, 1.2f + drive * 0.5f);
    
    // Add tape-style soft knee compression
    float compressionThreshold = 0.6f - warmth * 0.2f;
    if (std::abs(output) > compressionThreshold)
    {
        float excess = std::abs(output) - compressionThreshold;
        float compressionRatio = 3.0f + warmth * 2.0f;
        float compressedExcess = excess / compressionRatio;
        output = (output > 0) ? (compressionThreshold + compressedExcess) : -(compressionThreshold + compressedExcess);
    }
    
    // Tape-style high frequency loss (more pronounced than tube)
    float brightnessFactor = 0.7f + brightness * 0.3f;
    output *= brightnessFactor;
    
    // Add subtle tape wobble/modulation
    output *= (1.0f + std::sin(input * 50.0f) * drive * 0.02f);
    
    // Tape-style level compensation
    return output * (0.75f / (1.0f + drive * 0.2f));
}

// Transformer saturation - iron core saturation with magnetic hysteresis simulation
float TubeTapeSaturationProcessor::processTransformerSaturation(float input, float drive, float warmth, float brightness, int channel)
{
    // Transformer-style saturation with hysteresis-like behavior
    float scaledInput = input * (1.0f + drive * 5.0f);
    
    // Hard limiting at lower threshold to simulate iron core saturation
    float saturationThreshold = 0.5f - drive * 0.2f;
    float output;
    
    if (std::abs(scaledInput) > saturationThreshold)
    {
        // Hard clipping with odd harmonic emphasis
        output = asymmetricClip(scaledInput, saturationThreshold + 0.2f);
        
        // Add strong third and fifth harmonics for transformer character
        output += std::sin(3.0f * scaledInput) * drive * 0.15f;
        output += std::sin(5.0f * scaledInput) * drive * 0.08f;
    }
    else
    {
        output = tanhSaturation(scaledInput, 0.8f + drive * 0.5f);
    }
    
    // Add magnetic hysteresis simulation. Per-channel memory (hysteresisMemory); this was a
    // function-level static shared by both channels and by every instance in the session.
    float& previousOutput = hysteresisMemory[(size_t) juce::jlimit(0, kMaxChannels - 1, channel)];
    float hysteresisFactor = warmth * 0.1f;
    output = output * (1.0f - hysteresisFactor) + previousOutput * hysteresisFactor;
    previousOutput = output;
    
    // Transformer has less high-frequency loss than tape but more than tube
    float brightnessFactor = 0.85f + brightness * 0.15f;
    output *= brightnessFactor;
    
    // Add subtle low-frequency emphasis (transformer coupling)
    output *= (1.0f + warmth * 0.2f);
    
    // Transformer-style level compensation
    return output * (0.7f / (1.0f + drive * 0.4f));
}

// Soft clipping function
float TubeTapeSaturationProcessor::softClip(float input, float amount)
{
    if (amount < 0.001f) return input;
    
    float threshold = 1.0f - amount;
    if (std::abs(input) < threshold)
        return input;
    
    float sign = (input > 0.0f) ? 1.0f : -1.0f;
    float excess = std::abs(input) - threshold;
    float softClipped = threshold + excess / (1.0f + excess / amount);
    
    return sign * softClipped;
}

// Asymmetric clipping for tube-like distortion
float TubeTapeSaturationProcessor::asymmetricClip(float input, float threshold)
{
    if (input > threshold)
        return threshold + (input - threshold) * 0.3f;
    else if (input < -threshold)
        return -threshold + (input + threshold) * 0.7f;
    else
        return input;
}

// Hyperbolic tangent saturation
float TubeTapeSaturationProcessor::tanhSaturation(float input, float amount)
{
    return std::tanh(input * amount) / amount;
}

//==============================================================================
juce::AudioProcessorEditor* TubeTapeSaturationProcessor::createEditor()
{
    return new TubeTapeSaturationEditor(*this);
}

//==============================================================================
void TubeTapeSaturationProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = valueTreeState.copyState();
    state.setProperty("editor_width", editorWidth.load(), nullptr);
    state.setProperty("editor_height", editorHeight.load(), nullptr);
    auto xml = state.createXml();
    copyXmlToBinary(*xml, destData);
}

void TubeTapeSaturationProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml != nullptr && xml->hasTagName(valueTreeState.state.getType()))
    {
        auto state = juce::ValueTree::fromXml(*xml);
        setEditorSize(static_cast<int>(state.getProperty("editor_width", 0)),
                      static_cast<int>(state.getProperty("editor_height", 0)));
        state.removeProperty("editor_width", nullptr);
        state.removeProperty("editor_height", nullptr);
        valueTreeState.replaceState(state);
    }
}