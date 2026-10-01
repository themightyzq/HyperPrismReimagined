//==============================================================================
// HyperPrism Reimagined - Sonic Decimator Processor
//==============================================================================

#include "SonicDecimatorProcessor.h"
#include "SonicDecimatorEditor.h"

//==============================================================================
// BitCrusher Implementation
//==============================================================================
void SonicDecimatorProcessor::BitCrusher::setBitDepth(float newBitDepth)
{
    bitDepth = newBitDepth;
    updateQuantizationStep();
}

void SonicDecimatorProcessor::BitCrusher::setDithering(bool enableDither)
{
    ditherEnabled = enableDither;
}

void SonicDecimatorProcessor::BitCrusher::reset()
{
    random.setSeed(juce::Time::currentTimeMillis());
}

float SonicDecimatorProcessor::BitCrusher::processSample(float input)
{
    if (bitDepth >= 24.0f)
        return input; // No processing needed for high bit depths
    
    // Add dither if enabled
    float ditheredInput = input;
    if (ditherEnabled)
    {
        float ditherAmount = quantizationStep * 0.5f;
        float ditherNoise = (random.nextFloat() - 0.5f) * ditherAmount;
        ditheredInput += ditherNoise;
    }
    
    // Quantize to target bit depth
    float scaledInput = ditheredInput / quantizationStep;
    float quantized = std::round(scaledInput) * quantizationStep;
    
    // Clamp to valid range
    return juce::jlimit(-1.0f, 1.0f, quantized);
}

void SonicDecimatorProcessor::BitCrusher::updateQuantizationStep()
{
    if (bitDepth <= 1.0f)
    {
        quantizationStep = 1.0f; // Extreme bit crushing
    }
    else
    {
        float maxValue = std::pow(2.0f, bitDepth - 1.0f);
        quantizationStep = 1.0f / maxValue;
    }
}

//==============================================================================
// SampleRateReducer Implementation
//==============================================================================
void SonicDecimatorProcessor::SampleRateReducer::prepare(double processingRate, double hostRate, int samplesPerBlock)
{
    originalSampleRate = processingRate;
    hostSampleRate = hostRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = processingRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 1;

    // Give the filter real second-order coefficients before reset(), so their order never
    // changes on the audio thread (IIR::Filter reallocates its state when it does).
    *antiAliasFilter.coefficients = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(
        processingRate, static_cast<float>(juce::jmin(hostRate, processingRate) * 0.45));
    coefficientsTarget = -1.0f;
    antiAliasFilter.prepare(spec);

    reset();
}

void SonicDecimatorProcessor::SampleRateReducer::setSampleRate(float newTargetSampleRate)
{
    targetSampleRate = newTargetSampleRate;

    // Update anti-aliasing filter cutoff, only when it changes, into the filter's existing
    // coefficient storage (no allocation; this runs on the audio thread).
    if (antiAliasingEnabled && targetSampleRate < hostSampleRate
        && targetSampleRate != coefficientsTarget)
    {
        float cutoffFreq = targetSampleRate * 0.45f; // Slightly below Nyquist
        *antiAliasFilter.coefficients = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(
            originalSampleRate, cutoffFreq);
        coefficientsTarget = targetSampleRate;
    }
}

void SonicDecimatorProcessor::SampleRateReducer::setAntiAliasing(bool enableAntiAlias)
{
    antiAliasingEnabled = enableAntiAlias;
}

void SonicDecimatorProcessor::SampleRateReducer::reset()
{
    sampleCounter = 0.0f;
    lastOutputSample = 0.0f;
    antiAliasFilter.reset();
}

float SonicDecimatorProcessor::SampleRateReducer::processSample(float input)
{
    // Rate at or above the host rate: no reduction (compared with the host rate, not the
    // oversampled processing rate, as before oversampling was added).
    if (targetSampleRate >= hostSampleRate)
        return input;

    // Apply anti-aliasing filter before downsampling
    float filteredInput = antiAliasingEnabled ? antiAliasFilter.processSample(input) : input;

    // Calculate decimation ratio
    float decimationRatio = static_cast<float>(originalSampleRate) / targetSampleRate;

    // Sample and hold decimation
    sampleCounter += 1.0f;

    if (sampleCounter >= decimationRatio)
    {
        lastOutputSample = filteredInput;
        sampleCounter -= decimationRatio;
    }

    return lastOutputSample;
}

//==============================================================================
// NoiseShaper Implementation
//==============================================================================
void SonicDecimatorProcessor::NoiseShaper::reset()
{
    delayedError = 0.0f;
}

float SonicDecimatorProcessor::NoiseShaper::processSample(float input, float quantizationNoise)
{
    // Simple first-order noise shaping
    float shapedInput = input + delayedError;
    delayedError = quantizationNoise;
    
    return shapedInput;
}

//==============================================================================
// SonicDecimatorProcessor Implementation
//==============================================================================

// Parameter IDs
const juce::String SonicDecimatorProcessor::BYPASS_ID = "bypass";
const juce::String SonicDecimatorProcessor::BIT_DEPTH_ID = "bitDepth";
const juce::String SonicDecimatorProcessor::SAMPLE_RATE_ID = "sampleRate";
const juce::String SonicDecimatorProcessor::ANTI_ALIAS_ID = "antiAlias";
const juce::String SonicDecimatorProcessor::DITHER_ID = "dither";
const juce::String SonicDecimatorProcessor::MIX_ID = "mix";
const juce::String SonicDecimatorProcessor::OUTPUT_LEVEL_ID = "outputLevel";

//==============================================================================
SonicDecimatorProcessor::SonicDecimatorProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      valueTreeState(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache parameter pointers for performance
    bypassParam = valueTreeState.getRawParameterValue(BYPASS_ID);
    bitDepthParam = valueTreeState.getRawParameterValue(BIT_DEPTH_ID);
    sampleRateParam = valueTreeState.getRawParameterValue(SAMPLE_RATE_ID);
    antiAliasParam = valueTreeState.getRawParameterValue(ANTI_ALIAS_ID);
    ditherParam = valueTreeState.getRawParameterValue(DITHER_ID);
    mixParam = valueTreeState.getRawParameterValue(MIX_ID);
    outputLevelParam = valueTreeState.getRawParameterValue(OUTPUT_LEVEL_ID);
}

juce::AudioProcessorValueTreeState::ParameterLayout SonicDecimatorProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    // Bypass
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID, "Bypass", false));

    // Bit Depth (1 to 24 bits)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        BIT_DEPTH_ID, "Bit Depth", 
        juce::NormalisableRange<float>(1.0f, 24.0f, 0.1f, 0.3f), 16.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " bits"; }));

    // Sample Rate (1000 to 48000 Hz)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        SAMPLE_RATE_ID, "Sample Rate", 
        juce::NormalisableRange<float>(1000.0f, 48000.0f, 100.0f, 0.3f), 44100.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(static_cast<int>(value)) + " Hz"; }));

    // Anti-Aliasing
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        ANTI_ALIAS_ID, "Anti-Aliasing", true));

    // Dither
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        DITHER_ID, "Dither", false));

    // Mix (0% to 100%)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        MIX_ID, "Mix", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 100.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(static_cast<int>(value)) + "%"; }));

    // Output Level (-20 to +20 dB)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        OUTPUT_LEVEL_ID, "Output Level", 
        juce::NormalisableRange<float>(-20.0f, 20.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    return { parameters.begin(), parameters.end() };
}

//==============================================================================
void SonicDecimatorProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    preparedBlockSize = juce::jmax(1, samplesPerBlock);
    const int numChannels = juce::jmax(1, getTotalNumInputChannels());

    // Prepare DSP components with actual buffer size (fixes 512-sample artifact bug)
    for (auto& r : sampleRateReducers)
    {
#if HP_SONICDECIMATOR_FORCE_1X
        r.prepare(sampleRate, sampleRate, preparedBlockSize);
#else
        // The quantiser runs on 4x-oversampled audio, so as far as the sample-and-hold
        // decimator and its anti-alias filter are concerned, the processing rate is
        // hostRate * kOversamplingFactor. targetSampleRate (set per-block from the Rate
        // parameter) stays in real Hz, so a given Rate setting still decimates to the same
        // audible rate as it did at 1x -- this is the required hold-counter scaling.
        r.prepare(sampleRate * kOversamplingFactor, sampleRate, preparedBlockSize * kOversamplingFactor);
#endif
    }
    for (auto& b : bitCrushers) b.reset();
    for (auto& n : noiseShapers) n.reset();

    // Prepare dry buffer for mixing
    dryBuffer.setSize(numChannels, preparedBlockSize);

    constexpr double smoothingSeconds = 0.03;
    mixSmoothed.reset(sampleRate, smoothingSeconds);
    outputGainSmoothed.reset(sampleRate, smoothingSeconds);
    mixSmoothed.setCurrentAndTargetValue(mixParam->load() * 0.01f);
    outputGainSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(outputLevelParam->load()));

    // Reset metering
    inputLevel.store(0.0f);
    outputLevel.store(0.0f);
    bitReduction.store(0.0f);
    sampleReduction.store(0.0f);

    int latency = 0;
#if HP_SONICDECIMATOR_FORCE_1X
    oversampling.reset();
#else
    static_assert(kOversamplingFactor == 4, "oversampling stage count below assumes 4x (2 half-band stages)");
    oversampling = std::make_unique<juce::dsp::Oversampling<float>>(
        (size_t) numChannels,
        (size_t) 2, // log2(kOversamplingFactor)
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
        true,
        true); // integer latency, so the dry delay below can match it exactly
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

void SonicDecimatorProcessor::releaseResources()
{
    for (auto& r : sampleRateReducers) r.reset();
    for (auto& b : bitCrushers) b.reset();
    for (auto& n : noiseShapers) n.reset();

    if (oversampling != nullptr)
        oversampling->reset();
}

bool SonicDecimatorProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void SonicDecimatorProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    const int numChannels = juce::jmin(totalNumInputChannels, buffer.getNumChannels(), dryBuffer.getNumChannels());
    if (numChannels < 1 || numSamples == 0 || preparedBlockSize <= 0)
        return;

    const bool bypassed = bypassParam->load() > 0.5f;

    const float bitDepth = bitDepthParam->load();
    const float sampleRate = sampleRateParam->load();
    const bool antiAlias = antiAliasParam->load() > 0.5f;
    const bool dither = ditherParam->load() > 0.5f;
    mixSmoothed.setTargetValue(mixParam->load() * 0.01f); // Convert percentage to 0-1
    outputGainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(outputLevelParam->load()));

    // Update DSP parameters (the reducer only rebuilds its filter when Rate changes).
    for (auto& b : bitCrushers)
    {
        b.setBitDepth(bitDepth);
        b.setDithering(dither);
    }
    for (auto& r : sampleRateReducers)
    {
        r.setAntiAliasing(antiAlias);
        r.setSampleRate(sampleRate);
    }

    juce::dsp::AudioBlock<float> fullBlock(buffer.getArrayOfWritePointers(),
                                           static_cast<size_t>(numChannels),
                                           static_cast<size_t>(numSamples));

    // The oversampler and dryBuffer are sized for preparedBlockSize; a host that sends a bigger
    // block gets it processed in chunks of at most that size.
    for (int start = 0; start < numSamples; start += preparedBlockSize)
    {
        const int chunk = juce::jmin(preparedBlockSize, numSamples - start);
        auto block = fullBlock.getSubBlock(static_cast<size_t>(start), static_cast<size_t>(chunk));

        // Bypass still runs the dry delay so timing does not jump while latency stays reported.
        if (bypassed)
            delayDryOnly(block);
        else
            processChunk(block);
    }

    if (! bypassed)
    {
        // Calculate reduction amounts for metering
        float originalSampleRate = static_cast<float>(getSampleRate());
        sampleReduction.store(1.0f - (sampleRate / originalSampleRate));
        bitReduction.store(1.0f - (bitDepth / 24.0f));
    }
}

void SonicDecimatorProcessor::delayDryOnly(juce::dsp::AudioBlock<float> block)
{
    mixSmoothed.skip((int) block.getNumSamples());
    outputGainSmoothed.skip((int) block.getNumSamples());

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

void SonicDecimatorProcessor::processChunk(juce::dsp::AudioBlock<float> block)
{
    const int numSamples = static_cast<int>(block.getNumSamples());
    const int numChannels = static_cast<int>(block.getNumChannels());

    // Store the dry signal for mixing, delayed by the oversampler latency so it lines up with
    // the wet signal.
    float inputLevelSum = 0.0f;
    for (int channel = 0; channel < numChannels; ++channel)
    {
        const auto* in = block.getChannelPointer(static_cast<size_t>(channel));
        auto* dry = dryBuffer.getWritePointer(channel);
        for (int sample = 0; sample < numSamples; ++sample)
        {
            inputLevelSum += std::abs(in[sample]);
            dryDelay.pushSample(channel, in[sample]);
            dry[sample] = dryDelay.popSample(channel);
        }
    }

    // The bit/rate quantiser is the nonlinear stage: run it 4x oversampled so the
    // quantisation harmonics it introduces are pushed above the base Nyquist before the
    // downsampling half-band filter removes them (HP_SONICDECIMATOR_FORCE_1X=1 disables
    // this for an A/B comparison). The SampleRateReducer's hold counter was already
    // scaled for the oversampled rate in prepareToPlay.
#if HP_SONICDECIMATOR_FORCE_1X
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = block.getChannelPointer(static_cast<size_t>(channel));
        auto& reducer = sampleRateReducers[static_cast<size_t>(juce::jmin(channel, kMaxChannels - 1))];
        auto& crusher = bitCrushers[static_cast<size_t>(juce::jmin(channel, kMaxChannels - 1))];

        for (int sample = 0; sample < numSamples; ++sample)
            channelData[sample] = crusher.processSample(reducer.processSample(channelData[sample]));
    }
#else
    jassert(oversampling != nullptr);
    auto oversampledBlock = oversampling->processSamplesUp(block);

    for (size_t channel = 0; channel < oversampledBlock.getNumChannels(); ++channel)
    {
        auto* data = oversampledBlock.getChannelPointer(channel);
        auto& reducer = sampleRateReducers[juce::jmin(channel, (size_t) kMaxChannels - 1)];
        auto& crusher = bitCrushers[juce::jmin(channel, (size_t) kMaxChannels - 1)];

        for (size_t sample = 0; sample < oversampledBlock.getNumSamples(); ++sample)
        {
            float sampleReduced = reducer.processSample(data[sample]);
            data[sample] = crusher.processSample(sampleReduced);
        }
    }

    oversampling->processSamplesDown(block);
#endif

    // Mix dry/wet and apply output gain, at the base sample rate.
    float outputLevelSum = 0.0f;
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float mix = mixSmoothed.getNextValue();
        const float outputGain = outputGainSmoothed.getNextValue();

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = block.getChannelPointer(static_cast<size_t>(channel));
            const auto* dryData = dryBuffer.getReadPointer(channel);
            const float output = (dryData[sample] * (1.0f - mix) + channelData[sample] * mix) * outputGain;
            channelData[sample] = output;
            outputLevelSum += std::abs(output);
        }
    }

    // Update metering
    inputLevel.store(inputLevelSum / static_cast<float>(numSamples * numChannels));
    outputLevel.store(outputLevelSum / static_cast<float>(numSamples * numChannels));
}

//==============================================================================
juce::AudioProcessorEditor* SonicDecimatorProcessor::createEditor()
{
    return new SonicDecimatorEditor(*this);
}

//==============================================================================
void SonicDecimatorProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = valueTreeState.copyState();
    state.setProperty("editor_width", editorWidth.load(), nullptr);
    state.setProperty("editor_height", editorHeight.load(), nullptr);
    auto xml = state.createXml();
    copyXmlToBinary(*xml, destData);
}

void SonicDecimatorProcessor::setStateInformation(const void* data, int sizeInBytes)
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