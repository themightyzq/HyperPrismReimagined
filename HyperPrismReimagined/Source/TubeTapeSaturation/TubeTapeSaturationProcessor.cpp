//==============================================================================
// HyperPrism Revived - Tube/Tape Saturation Processor
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
    juce::ignoreUnused(samplesPerBlock);
    currentSampleRate = sampleRate;
    
    // Initialize filters
    juce::IIRCoefficients dcBlockCoeffs = juce::IIRCoefficients::makeHighPass(sampleRate, 20.0);
    dcBlockLeft.setCoefficients(dcBlockCoeffs);
    dcBlockRight.setCoefficients(dcBlockCoeffs);
    
    // Initialize shelf filters for warmth and brightness
    updateFilters();
    
    // Reset processing state
    previousInputRMS = 0.0f;
    previousOutputRMS = 0.0f;
    harmonicContent.store(0.0f);
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
    
    if (bypassParam->load() > 0.5f)
        return;
        
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    updateFilters();
    processSaturation(buffer);
    calculateHarmonicContent(buffer);
}

void TubeTapeSaturationProcessor::processSaturation(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    // Calculate input level
    float inputSum = 0.0f;
    for (int channel = 0; channel < numChannels; ++channel)
    {
        const auto* channelData = buffer.getReadPointer(channel);
        for (int sample = 0; sample < numSamples; ++sample)
        {
            inputSum += std::abs(channelData[sample]);
        }
    }
    inputLevel.store(inputSum / (numChannels * numSamples));
    
    const float drive = driveParam->load() / 100.0f;
    const int type = static_cast<int>(typeParam->load());
    const float warmth = warmthParam->load() / 100.0f;
    const float brightness = brightnessParam->load() / 100.0f;
    const float outputGain = juce::Decibels::decibelsToGain(outputLevelParam->load());
    
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        auto& lowShelf = (channel == 0) ? lowShelfLeft : lowShelfRight;
        auto& highShelf = (channel == 0) ? highShelfLeft : highShelfRight;
        auto& dcBlock = (channel == 0) ? dcBlockLeft : dcBlockRight;
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float input = channelData[sample];
            
            // Pre-filtering for warmth and brightness shaping
            float processed = lowShelf.processSingleSampleRaw(input);
            processed = highShelf.processSingleSampleRaw(processed);
            
            // Apply saturation based on type
            switch (type)
            {
                case Tube:
                    processed = processTubeSaturation(processed, drive, warmth, brightness);
                    break;
                case Tape:
                    processed = processTapeSaturation(processed, drive, warmth, brightness);
                    break;
                case Transformer:
                    processed = processTransformerSaturation(processed, drive, warmth, brightness);
                    break;
            }
            
            // DC blocking
            processed = dcBlock.processSingleSampleRaw(processed);
            
            // Output level adjustment
            channelData[sample] = processed * outputGain;
        }
    }
}

void TubeTapeSaturationProcessor::updateFilters()
{
    const float warmth = warmthParam->load() / 100.0f;
    const float brightness = brightnessParam->load() / 100.0f;
    
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
            sumSquares += channelData[sample] * channelData[sample];
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
float TubeTapeSaturationProcessor::processTransformerSaturation(float input, float drive, float warmth, float brightness)
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
    
    // Add magnetic hysteresis simulation
    static float previousOutput = 0.0f;
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
    auto xml = valueTreeState.copyState().createXml();
    copyXmlToBinary(*xml, destData);
}

void TubeTapeSaturationProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml != nullptr && xml->hasTagName(valueTreeState.state.getType()))
        valueTreeState.replaceState(juce::ValueTree::fromXml(*xml));
}