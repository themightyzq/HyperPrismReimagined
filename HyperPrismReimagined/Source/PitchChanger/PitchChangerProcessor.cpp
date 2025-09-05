//==============================================================================
// HyperPrism Revived - Pitch Changer Processor
//==============================================================================

#include "PitchChangerProcessor.h"
#include "PitchChangerEditor.h"

//==============================================================================
// PitchShifter Implementation using Signalsmith Stretch
//==============================================================================
PitchChangerProcessor::PitchShifter::PitchShifter()
{
    stretcher = std::make_unique<signalsmith::stretch::SignalsmithStretch<float>>();
}

void PitchChangerProcessor::PitchShifter::prepare(double sampleRate, int blockSize)
{
    currentSampleRate = sampleRate;
    maxBlockSize = blockSize;
    
    // Use presetDefault for high-quality pitch shifting
    stretcher->presetDefault(2, sampleRate); // 2 channels for stereo
    stretcher->setTransposeFactor(1.0f); // Default to no pitch change
    
    // Prepare buffers for de-interleaved audio
    leftInputBuffer.resize(blockSize);
    rightInputBuffer.resize(blockSize);
    leftOutputBuffer.resize(blockSize);
    rightOutputBuffer.resize(blockSize);
}

void PitchChangerProcessor::PitchShifter::reset()
{
    stretcher->reset();
    std::fill(leftInputBuffer.begin(), leftInputBuffer.end(), 0.0f);
    std::fill(rightInputBuffer.begin(), rightInputBuffer.end(), 0.0f);
    std::fill(leftOutputBuffer.begin(), leftOutputBuffer.end(), 0.0f);
    std::fill(rightOutputBuffer.begin(), rightOutputBuffer.end(), 0.0f);
}

void PitchChangerProcessor::PitchShifter::setPitchShift(float pitchRatio)
{
    currentPitchRatio = pitchRatio;
    stretcher->setTransposeFactor(pitchRatio);
}

void PitchChangerProcessor::PitchShifter::setFormantShift(float formantRatio)
{
    currentFormantRatio = formantRatio;
    stretcher->setFormantFactor(formantRatio);
}

void PitchChangerProcessor::PitchShifter::processBlock(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels == 0 || numSamples == 0)
        return;
    
    // Copy to de-interleaved buffers
    if (numChannels == 1)
    {
        // Mono input - duplicate to both channels
        auto* monoData = buffer.getReadPointer(0);
        std::copy(monoData, monoData + numSamples, leftInputBuffer.data());
        std::copy(monoData, monoData + numSamples, rightInputBuffer.data());
    }
    else
    {
        // Stereo or multi-channel input
        auto* leftData = buffer.getReadPointer(0);
        std::copy(leftData, leftData + numSamples, leftInputBuffer.data());
        
        if (numChannels > 1)
        {
            auto* rightData = buffer.getReadPointer(1);
            std::copy(rightData, rightData + numSamples, rightInputBuffer.data());
        }
        else
        {
            // Only one channel, duplicate it
            std::copy(leftData, leftData + numSamples, rightInputBuffer.data());
        }
    }
    
    // Create arrays of pointers for Signalsmith Stretch
    float* inputPtrs[2] = { leftInputBuffer.data(), rightInputBuffer.data() };
    float* outputPtrs[2] = { leftOutputBuffer.data(), rightOutputBuffer.data() };
    
    // Process through Signalsmith Stretch
    stretcher->process(inputPtrs, numSamples, outputPtrs, numSamples);
    
    // Copy back to buffer
    if (numChannels == 1)
    {
        // Mix stereo output back to mono
        auto* monoData = buffer.getWritePointer(0);
        for (int i = 0; i < numSamples; ++i)
        {
            monoData[i] = (leftOutputBuffer[i] + rightOutputBuffer[i]) * 0.5f;
        }
    }
    else
    {
        // Copy stereo output
        auto* leftData = buffer.getWritePointer(0);
        std::copy(leftOutputBuffer.data(), leftOutputBuffer.data() + numSamples, leftData);
        
        if (numChannels > 1)
        {
            auto* rightData = buffer.getWritePointer(1);
            std::copy(rightOutputBuffer.data(), rightOutputBuffer.data() + numSamples, rightData);
        }
        
        // Clear any additional channels
        for (int channel = 2; channel < numChannels; ++channel)
        {
            buffer.clear(channel, 0, numSamples);
        }
    }
}

//==============================================================================
// PitchDetector Implementation
//==============================================================================
PitchChangerProcessor::PitchDetector::PitchDetector()
{
    autocorrelationBuffer.resize(analysisSize);
    analysisBuffer.resize(analysisSize);
}

void PitchChangerProcessor::PitchDetector::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    reset();
}

void PitchChangerProcessor::PitchDetector::reset()
{
    std::fill(autocorrelationBuffer.begin(), autocorrelationBuffer.end(), 0.0f);
    std::fill(analysisBuffer.begin(), analysisBuffer.end(), 0.0f);
}

float PitchChangerProcessor::PitchDetector::detectPitch(const float* buffer, int bufferSize)
{
    int analysisLength = std::min(bufferSize, analysisSize);
    
    // Copy to analysis buffer
    std::copy(buffer, buffer + analysisLength, analysisBuffer.begin());
    
    // Find pitch using autocorrelation
    float maxCorrelation = 0.0f;
    int bestDelay = 0;
    
    int minDelay = static_cast<int>(currentSampleRate / 800.0); // 800 Hz max
    int maxDelay = static_cast<int>(currentSampleRate / 50.0);  // 50 Hz min
    
    for (int delay = minDelay; delay < maxDelay && delay < analysisLength / 2; ++delay)
    {
        float correlation = autocorrelate(analysisBuffer.data(), delay, analysisLength - delay);
        
        if (correlation > maxCorrelation)
        {
            maxCorrelation = correlation;
            bestDelay = delay;
        }
    }
    
    // Convert delay to frequency
    if (bestDelay > 0)
        return static_cast<float>(currentSampleRate / bestDelay);
    else
        return 0.0f;
}

float PitchChangerProcessor::PitchDetector::autocorrelate(const float* buffer, int delay, int length)
{
    float sum = 0.0f;
    float norm1 = 0.0f;
    float norm2 = 0.0f;
    
    for (int i = 0; i < length; ++i)
    {
        float a = buffer[i];
        float b = buffer[i + delay];
        
        sum += a * b;
        norm1 += a * a;
        norm2 += b * b;
    }
    
    float normalization = std::sqrt(norm1 * norm2);
    return normalization > 0.0f ? sum / normalization : 0.0f;
}

//==============================================================================
// PitchChangerProcessor Implementation
//==============================================================================

// Parameter IDs
const juce::String PitchChangerProcessor::BYPASS_ID = "bypass";
const juce::String PitchChangerProcessor::PITCH_SHIFT_ID = "pitchShift";
const juce::String PitchChangerProcessor::FINE_TUNE_ID = "fineTune";
const juce::String PitchChangerProcessor::FORMANT_SHIFT_ID = "formantShift";
const juce::String PitchChangerProcessor::MIX_ID = "mix";
const juce::String PitchChangerProcessor::OUTPUT_LEVEL_ID = "outputLevel";

//==============================================================================
PitchChangerProcessor::PitchChangerProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      valueTreeState(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache parameter pointers for performance
    bypassParam = valueTreeState.getRawParameterValue(BYPASS_ID);
    pitchShiftParam = valueTreeState.getRawParameterValue(PITCH_SHIFT_ID);
    fineTuneParam = valueTreeState.getRawParameterValue(FINE_TUNE_ID);
    formantShiftParam = valueTreeState.getRawParameterValue(FORMANT_SHIFT_ID);
    mixParam = valueTreeState.getRawParameterValue(MIX_ID);
    outputLevelParam = valueTreeState.getRawParameterValue(OUTPUT_LEVEL_ID);
}

juce::AudioProcessorValueTreeState::ParameterLayout PitchChangerProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    // Bypass
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID, "Bypass", false));

    // Pitch Shift (-24 to +24 semitones)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        PITCH_SHIFT_ID, "Pitch Shift", 
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " st"; }));

    // Fine Tune (-100 to +100 cents)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        FINE_TUNE_ID, "Fine Tune", 
        juce::NormalisableRange<float>(-100.0f, 100.0f, 1.0f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(static_cast<int>(value)) + " cents"; }));

    // Formant Shift (-12 to +12 semitones)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        FORMANT_SHIFT_ID, "Formant Shift", 
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " st"; }));

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
void PitchChangerProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Prepare DSP components
    pitchShifter = std::make_unique<PitchShifter>();
    pitchShifter->prepare(sampleRate, samplesPerBlock);
    pitchDetector.prepare(sampleRate);
    
    // Prepare dry buffer for mixing
    dryBuffer.setSize(getTotalNumInputChannels(), samplesPerBlock);
    
    // Reset metering
    inputLevel.store(0.0f);
    outputLevel.store(0.0f);
    pitchDetection.store(0.0f);
}

void PitchChangerProcessor::releaseResources()
{
    if (pitchShifter)
        pitchShifter->reset();
    pitchDetector.reset();
}

bool PitchChangerProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void PitchChangerProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
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

    processPitchShifting(buffer);
}

void PitchChangerProcessor::processPitchShifting(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    
    const float pitchShift = pitchShiftParam->load();
    const float fineTune = fineTuneParam->load();
    const float formantShift = formantShiftParam->load();
    const float mix = mixParam->load() * 0.01f; // Convert percentage to 0-1
    const float outputGain = juce::Decibels::decibelsToGain(outputLevelParam->load());
    
    // Calculate pitch ratio from semitones
    float totalPitchShift = pitchShift + (fineTune * 0.01f); // Convert cents to semitones
    float pitchRatio = std::pow(2.0f, totalPitchShift / 12.0f);
    float formantRatio = std::pow(2.0f, formantShift / 12.0f);
    
    // Update pitch shifter parameters
    if (pitchShifter)
    {
        pitchShifter->setPitchShift(pitchRatio);
        pitchShifter->setFormantShift(formantRatio);
    }
    
    // Store dry signal for mixing
    dryBuffer.makeCopyOf(buffer);
    
    float inputLevelSum = 0.0f;
    float outputLevelSum = 0.0f;
    
    // Detect pitch on first channel
    if (numChannels > 0)
    {
        auto* firstChannelData = buffer.getReadPointer(0);
        float detectedPitch = pitchDetector.detectPitch(firstChannelData, numSamples);
        pitchDetection.store(detectedPitch);
    }
    
    // Process pitch shifting
    if (pitchShifter)
        pitchShifter->processBlock(buffer);
    
    // Mix dry and wet signals
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        auto* dryData = dryBuffer.getReadPointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float dry = dryData[sample];
            float wet = channelData[sample];
            
            inputLevelSum += std::abs(dry);
            
            // Mix and apply output level
            float output = (dry * (1.0f - mix) + wet * mix) * outputGain;
            channelData[sample] = output;
            
            outputLevelSum += std::abs(output);
        }
    }
    
    // Update metering
    inputLevel.store(inputLevelSum / (numSamples * numChannels));
    outputLevel.store(outputLevelSum / (numSamples * numChannels));
}

//==============================================================================
juce::AudioProcessorEditor* PitchChangerProcessor::createEditor()
{
    return new PitchChangerEditor(*this);
}

//==============================================================================
void PitchChangerProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto xml = valueTreeState.copyState().createXml();
    copyXmlToBinary(*xml, destData);
}

void PitchChangerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml != nullptr && xml->hasTagName(valueTreeState.state.getType()))
        valueTreeState.replaceState(juce::ValueTree::fromXml(*xml));
}