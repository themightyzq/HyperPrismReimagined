//==============================================================================
// HyperPrism Revived - Vocoder Processor
//==============================================================================

#include "VocoderProcessor.h"
#include "VocoderEditor.h"

//==============================================================================
// VocoderBand Implementation
//==============================================================================
void VocoderProcessor::VocoderBand::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = 512;
    spec.numChannels = 1;
    
    carrierFilter.prepare(spec);
    modulatorFilter.prepare(spec);
    
    updateEnvelopeCoeff();
    reset();
}

void VocoderProcessor::VocoderBand::setFrequency(float frequency, float bandwidth)
{
    // Create bandpass filters using second-order sections
    auto coefficients = juce::dsp::IIR::Coefficients<float>::makeBandPass(
        currentSampleRate, frequency, bandwidth);
    
    carrierFilter.coefficients = coefficients;
    modulatorFilter.coefficients = coefficients;
}

void VocoderProcessor::VocoderBand::setReleaseTime(float releaseMs)
{
    releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * static_cast<float>(currentSampleRate)));
}

void VocoderProcessor::VocoderBand::reset()
{
    carrierFilter.reset();
    modulatorFilter.reset();
    envelopeLevel = 0.0f;
    processedCarrier = 0.0f;
}

float VocoderProcessor::VocoderBand::processCarrier(float carrierSample)
{
    processedCarrier = carrierFilter.processSample(carrierSample);
    return processedCarrier;
}

float VocoderProcessor::VocoderBand::processModulator(float modulatorSample)
{
    // Filter the modulator signal
    float filteredModulator = modulatorFilter.processSample(modulatorSample);
    
    // Extract envelope (rectify and smooth)
    float rectified = std::abs(filteredModulator);
    
    // Envelope follower with attack/release
    if (rectified > envelopeLevel)
    {
        // Fast attack
        envelopeLevel = rectified + (envelopeLevel - rectified) * 0.1f;
    }
    else
    {
        // Slower release
        envelopeLevel = rectified + (envelopeLevel - rectified) * releaseCoeff;
    }
    
    return envelopeLevel;
}

float VocoderProcessor::VocoderBand::getOutput()
{
    // Apply modulator envelope to carrier
    return processedCarrier * envelopeLevel;
}

void VocoderProcessor::VocoderBand::updateEnvelopeCoeff()
{
    setReleaseTime(50.0f); // Default 50ms release
}

//==============================================================================
// CarrierOscillator Implementation
//==============================================================================
void VocoderProcessor::CarrierOscillator::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    updatePhaseIncrement();
    reset();
}

void VocoderProcessor::CarrierOscillator::setFrequency(float newFrequency)
{
    frequency = newFrequency;
    updatePhaseIncrement();
}

void VocoderProcessor::CarrierOscillator::reset()
{
    phase = 0.0;
}

float VocoderProcessor::CarrierOscillator::getNextSample()
{
    // Generate sawtooth wave for rich harmonic content
    float output = static_cast<float>(2.0 * (phase / juce::MathConstants<double>::twoPi) - 1.0);
    
    phase += phaseIncrement;
    if (phase >= juce::MathConstants<double>::twoPi)
        phase -= juce::MathConstants<double>::twoPi;
    
    return output;
}

void VocoderProcessor::CarrierOscillator::updatePhaseIncrement()
{
    phaseIncrement = juce::MathConstants<double>::twoPi * frequency / currentSampleRate;
}

//==============================================================================
// VocoderProcessor Implementation
//==============================================================================

// Parameter IDs
const juce::String VocoderProcessor::BYPASS_ID = "bypass";
const juce::String VocoderProcessor::CARRIER_FREQ_ID = "carrierFreq";
const juce::String VocoderProcessor::MODULATOR_GAIN_ID = "modulatorGain";
const juce::String VocoderProcessor::BAND_COUNT_ID = "bandCount";
const juce::String VocoderProcessor::RELEASE_TIME_ID = "releaseTime";
const juce::String VocoderProcessor::OUTPUT_LEVEL_ID = "outputLevel";

//==============================================================================
VocoderProcessor::VocoderProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      valueTreeState(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache parameter pointers for performance
    bypassParam = valueTreeState.getRawParameterValue(BYPASS_ID);
    carrierFreqParam = valueTreeState.getRawParameterValue(CARRIER_FREQ_ID);
    modulatorGainParam = valueTreeState.getRawParameterValue(MODULATOR_GAIN_ID);
    bandCountParam = valueTreeState.getRawParameterValue(BAND_COUNT_ID);
    releaseTimeParam = valueTreeState.getRawParameterValue(RELEASE_TIME_ID);
    outputLevelParam = valueTreeState.getRawParameterValue(OUTPUT_LEVEL_ID);
    
    // Initialize band levels for metering
    bandLevels.resize(maxBands, 0.0f);
    
    setupVocoderBands();
}

juce::AudioProcessorValueTreeState::ParameterLayout VocoderProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    // Bypass
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID, "Bypass", false));

    // Carrier Frequency (50 to 2000 Hz)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        CARRIER_FREQ_ID, "Carrier Frequency", 
        juce::NormalisableRange<float>(50.0f, 2000.0f, 1.0f, 0.3f), 220.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " Hz"; }));

    // Modulator Gain (-20 to +20 dB)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        MODULATOR_GAIN_ID, "Modulator Gain", 
        juce::NormalisableRange<float>(-20.0f, 20.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    // Band Count (4 to 16)
    parameters.push_back(std::make_unique<juce::AudioParameterInt>(
        BAND_COUNT_ID, "Band Count", 4, 16, 8));

    // Release Time (10 to 500 ms)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        RELEASE_TIME_ID, "Release Time", 
        juce::NormalisableRange<float>(10.0f, 500.0f, 1.0f, 0.3f), 50.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 0) + " ms"; }));

    // Output Level (-20 to +20 dB)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        OUTPUT_LEVEL_ID, "Output Level", 
        juce::NormalisableRange<float>(-20.0f, 20.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    return { parameters.begin(), parameters.end() };
}

//==============================================================================
void VocoderProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    
    // Prepare DSP components
    for (auto& band : vocoderBands)
        band.prepare(sampleRate);
    
    carrierOscillator.prepare(sampleRate);
    
    // Reset metering
    carrierLevel.store(0.0f);
    modulatorLevel.store(0.0f);
    outputLevel.store(0.0f);
    std::fill(bandLevels.begin(), bandLevels.end(), 0.0f);
}

void VocoderProcessor::releaseResources()
{
    for (auto& band : vocoderBands)
        band.reset();
    
    carrierOscillator.reset();
}

bool VocoderProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void VocoderProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
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

    processVocoding(buffer);
}

void VocoderProcessor::processVocoding(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    
    const float carrierFreq = carrierFreqParam->load();
    const float modulatorGain = juce::Decibels::decibelsToGain(modulatorGainParam->load());
    const int bandCount = static_cast<int>(bandCountParam->load());
    const float releaseTime = releaseTimeParam->load();
    const float outputGain = juce::Decibels::decibelsToGain(outputLevelParam->load());
    
    // Update band count if changed
    if (bandCount != currentBandCount)
    {
        currentBandCount = bandCount;
        setupVocoderBands();
    }
    
    // Update carrier frequency
    carrierOscillator.setFrequency(carrierFreq);
    
    // Update release time for all bands
    for (int i = 0; i < currentBandCount; ++i)
        vocoderBands[i].setReleaseTime(releaseTime);
    
    float carrierLevelSum = 0.0f;
    float modulatorLevelSum = 0.0f;
    float outputLevelSum = 0.0f;
    
    // Reset band level accumulation
    std::vector<float> bandLevelSums(maxBands, 0.0f);
    
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Use input as modulator
            float modulator = channelData[sample] * modulatorGain;
            modulatorLevelSum += std::abs(modulator);
            
            // Generate carrier signal
            float carrier = carrierOscillator.getNextSample();
            carrierLevelSum += std::abs(carrier);
            
            // Process through vocoder bands
            float output = 0.0f;
            
            for (int i = 0; i < currentBandCount; ++i)
            {
                // Process carrier and modulator through band filters
                vocoderBands[i].processCarrier(carrier);
                vocoderBands[i].processModulator(modulator);
                
                // Get band output and accumulate
                float bandOutput = vocoderBands[i].getOutput();
                output += bandOutput;
                
                // Accumulate band levels for metering
                bandLevelSums[i] += vocoderBands[i].getEnvelopeLevel();
            }
            
            // Apply output level
            output *= outputGain;
            
            channelData[sample] = output;
            outputLevelSum += std::abs(output);
        }
    }
    
    // Update metering
    carrierLevel.store(carrierLevelSum / (numSamples * numChannels));
    modulatorLevel.store(modulatorLevelSum / (numSamples * numChannels));
    outputLevel.store(outputLevelSum / (numSamples * numChannels));
    
    // Update band levels
    for (int i = 0; i < maxBands; ++i)
    {
        if (i < currentBandCount)
            bandLevels[i] = bandLevelSums[i] / numSamples;
        else
            bandLevels[i] = 0.0f;
    }
}

void VocoderProcessor::setupVocoderBands()
{
    // Ensure we have enough bands
    if (vocoderBands.size() < static_cast<size_t>(maxBands))
        vocoderBands.resize(maxBands);
    
    // Calculate logarithmically spaced band frequencies
    bandFrequencies.clear();
    bandFrequencies.resize(currentBandCount);
    
    const float minFreq = 80.0f;   // Lowest band frequency
    const float maxFreq = 8000.0f; // Highest band frequency
    
    for (int i = 0; i < currentBandCount; ++i)
    {
        float ratio = static_cast<float>(i) / (currentBandCount - 1);
        bandFrequencies[i] = minFreq * std::pow(maxFreq / minFreq, ratio);
    }
    
    // Setup each band with appropriate frequency and bandwidth
    for (int i = 0; i < currentBandCount; ++i)
    {
        float centerFreq = bandFrequencies[i];
        float bandwidth;
        
        if (i == 0)
        {
            // First band
            bandwidth = (bandFrequencies[1] - centerFreq) * 0.8f;
        }
        else if (i == currentBandCount - 1)
        {
            // Last band
            bandwidth = (centerFreq - bandFrequencies[i - 1]) * 0.8f;
        }
        else
        {
            // Middle bands
            bandwidth = (bandFrequencies[i + 1] - bandFrequencies[i - 1]) * 0.4f;
        }
        
        vocoderBands[i].setFrequency(centerFreq, bandwidth);
    }
}

//==============================================================================
juce::AudioProcessorEditor* VocoderProcessor::createEditor()
{
    return new VocoderEditor(*this);
}

//==============================================================================
void VocoderProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto xml = valueTreeState.copyState().createXml();
    copyXmlToBinary(*xml, destData);
}

void VocoderProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml != nullptr && xml->hasTagName(valueTreeState.state.getType()))
        valueTreeState.replaceState(juce::ValueTree::fromXml(*xml));
}