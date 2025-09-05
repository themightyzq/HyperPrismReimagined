//==============================================================================
// HyperPrism Revived - Sonic Decimator Processor
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
void SonicDecimatorProcessor::SampleRateReducer::prepare(double sampleRate)
{
    originalSampleRate = sampleRate;
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = 512;
    spec.numChannels = 1;
    
    antiAliasFilter.prepare(spec);
    
    reset();
}

void SonicDecimatorProcessor::SampleRateReducer::setSampleRate(float newTargetSampleRate)
{
    targetSampleRate = newTargetSampleRate;
    
    // Update anti-aliasing filter cutoff
    if (antiAliasingEnabled && targetSampleRate < originalSampleRate)
    {
        float cutoffFreq = targetSampleRate * 0.45f; // Slightly below Nyquist
        auto coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(
            originalSampleRate, cutoffFreq);
        antiAliasFilter.coefficients = coefficients;
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
    if (targetSampleRate >= originalSampleRate)
        return input; // No reduction needed
    
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
    // Prepare DSP components
    sampleRateReducer.prepare(sampleRate);
    bitCrusher.reset();
    noiseShaper.reset();
    
    // Prepare dry buffer for mixing
    dryBuffer.setSize(getTotalNumInputChannels(), samplesPerBlock);
    
    // Reset metering
    inputLevel.store(0.0f);
    outputLevel.store(0.0f);
    bitReduction.store(0.0f);
    sampleReduction.store(0.0f);
}

void SonicDecimatorProcessor::releaseResources()
{
    sampleRateReducer.reset();
    bitCrusher.reset();
    noiseShaper.reset();
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
    
    if (bypassParam->load() > 0.5f)
        return;
        
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    if (buffer.getNumChannels() < 1)
        return;

    processDecimation(buffer);
}

void SonicDecimatorProcessor::processDecimation(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    
    const float bitDepth = bitDepthParam->load();
    const float sampleRate = sampleRateParam->load();
    const bool antiAlias = antiAliasParam->load() > 0.5f;
    const bool dither = ditherParam->load() > 0.5f;
    const float mix = mixParam->load() * 0.01f; // Convert percentage to 0-1
    const float outputGain = juce::Decibels::decibelsToGain(outputLevelParam->load());
    
    // Update DSP parameters
    bitCrusher.setBitDepth(bitDepth);
    bitCrusher.setDithering(dither);
    sampleRateReducer.setSampleRate(sampleRate);
    sampleRateReducer.setAntiAliasing(antiAlias);
    
    // Store dry signal for mixing
    dryBuffer.makeCopyOf(buffer);
    
    float inputLevelSum = 0.0f;
    float outputLevelSum = 0.0f;
    
    // Calculate reduction amounts for metering
    float originalSampleRate = static_cast<float>(getSampleRate());
    float sampleReductionAmount = 1.0f - (sampleRate / originalSampleRate);
    float bitReductionAmount = 1.0f - (bitDepth / 24.0f);
    
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        auto* dryData = dryBuffer.getReadPointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float input = channelData[sample];
            inputLevelSum += std::abs(input);
            
            // Apply sample rate reduction first
            float sampleReduced = sampleRateReducer.processSample(input);
            
            // Then apply bit crushing
            float bitCrushed = bitCrusher.processSample(sampleReduced);
            
            // Mix dry and wet signals
            float output = (dryData[sample] * (1.0f - mix) + bitCrushed * mix) * outputGain;
            channelData[sample] = output;
            
            outputLevelSum += std::abs(output);
        }
    }
    
    // Update metering
    inputLevel.store(inputLevelSum / (numSamples * numChannels));
    outputLevel.store(outputLevelSum / (numSamples * numChannels));
    bitReduction.store(bitReductionAmount);
    sampleReduction.store(sampleReductionAmount);
}

//==============================================================================
juce::AudioProcessorEditor* SonicDecimatorProcessor::createEditor()
{
    return new SonicDecimatorEditor(*this);
}

//==============================================================================
void SonicDecimatorProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto xml = valueTreeState.copyState().createXml();
    copyXmlToBinary(*xml, destData);
}

void SonicDecimatorProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml != nullptr && xml->hasTagName(valueTreeState.state.getType()))
        valueTreeState.replaceState(juce::ValueTree::fromXml(*xml));
}