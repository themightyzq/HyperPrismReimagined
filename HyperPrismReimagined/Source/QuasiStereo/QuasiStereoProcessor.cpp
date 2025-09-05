//==============================================================================
// HyperPrism Revived - Quasi Stereo Processor
//==============================================================================

#include "QuasiStereoProcessor.h"
#include "QuasiStereoEditor.h"

// Parameter IDs
const juce::String QuasiStereoProcessor::BYPASS_ID = "bypass";
const juce::String QuasiStereoProcessor::WIDTH_ID = "width";
const juce::String QuasiStereoProcessor::DELAY_TIME_ID = "delayTime";
const juce::String QuasiStereoProcessor::FREQUENCY_SHIFT_ID = "frequencyShift";
const juce::String QuasiStereoProcessor::PHASE_SHIFT_ID = "phaseShift";
const juce::String QuasiStereoProcessor::HIGH_FREQ_ENHANCE_ID = "highFreqEnhance";
const juce::String QuasiStereoProcessor::OUTPUT_LEVEL_ID = "outputLevel";

//==============================================================================
QuasiStereoProcessor::QuasiStereoProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input",  juce::AudioChannelSet::mono(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      valueTreeState(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache parameter pointers for performance
    bypassParam = valueTreeState.getRawParameterValue(BYPASS_ID);
    widthParam = valueTreeState.getRawParameterValue(WIDTH_ID);
    delayTimeParam = valueTreeState.getRawParameterValue(DELAY_TIME_ID);
    frequencyShiftParam = valueTreeState.getRawParameterValue(FREQUENCY_SHIFT_ID);
    phaseShiftParam = valueTreeState.getRawParameterValue(PHASE_SHIFT_ID);
    highFreqEnhanceParam = valueTreeState.getRawParameterValue(HIGH_FREQ_ENHANCE_ID);
    outputLevelParam = valueTreeState.getRawParameterValue(OUTPUT_LEVEL_ID);
}

juce::AudioProcessorValueTreeState::ParameterLayout QuasiStereoProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    // Bypass
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID, "Bypass", false));

    // Stereo Width (0-200%)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        WIDTH_ID, "Stereo Width", 
        juce::NormalisableRange<float>(0.0f, 200.0f, 0.1f), 100.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + "%"; }));

    // Delay Time (0.1 - 50ms)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DELAY_TIME_ID, "Delay Time", 
        juce::NormalisableRange<float>(0.1f, 50.0f, 0.01f, 0.3f), 5.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 2) + " ms"; }));

    // Frequency Shift (0-100 Hz)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        FREQUENCY_SHIFT_ID, "Frequency Shift", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 15.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " Hz"; }));

    // Phase Shift (0-180 degrees)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        PHASE_SHIFT_ID, "Phase Shift", 
        juce::NormalisableRange<float>(0.0f, 180.0f, 1.0f), 90.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(static_cast<int>(value)) + "°"; }));

    // High Frequency Enhancement (0-100%)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        HIGH_FREQ_ENHANCE_ID, "High Freq Enhance", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 25.0f,
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
void QuasiStereoProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    // Prepare delay line
    delayLine.prepare({ sampleRate, static_cast<juce::uint32>(samplesPerBlock), 1 });
    delayLine.reset();
    
    // Prepare all-pass filter for phase shifting
    allPassFilter.prepare({ sampleRate, static_cast<juce::uint32>(samplesPerBlock), 2 });
    allPassFilter.reset();
    
    // Initialize filters
    previousHighFreqEnhance = -1.0f;
    phaseAccumulator = 0.0f;
    
    // Reset metering
    leftLevel.store(0.0f);
    rightLevel.store(0.0f);
    stereoWidth.store(0.0f);
}

void QuasiStereoProcessor::releaseResources()
{
    // Reset filters
    highFreqFilterLeft.reset();
    highFreqFilterRight.reset();
    allPassFilter.reset();
}

bool QuasiStereoProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Accept mono input and stereo output, or stereo input/output
    auto mainInput = layouts.getMainInputChannelSet();
    auto mainOutput = layouts.getMainOutputChannelSet();
    
    if (mainOutput != juce::AudioChannelSet::stereo())
        return false;
        
    return (mainInput == juce::AudioChannelSet::mono() || 
            mainInput == juce::AudioChannelSet::stereo());
}

void QuasiStereoProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    
    if (bypassParam->load() > 0.5f)
    {
        // If bypassed and input is mono, just copy to both channels
        if (getTotalNumInputChannels() == 1 && getTotalNumOutputChannels() == 2)
        {
            buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());
        }
        return;
    }
    
    // Ensure we have stereo output
    if (getTotalNumOutputChannels() < 2)
        return;
        
    // If input is mono, duplicate to stereo first
    if (getTotalNumInputChannels() == 1)
    {
        buffer.copyFrom(1, 0, buffer, 0, 0, buffer.getNumSamples());
    }

    processQuasiStereo(buffer);
    calculateStereoWidth(buffer);
}

void QuasiStereoProcessor::processQuasiStereo(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    
    const float width = widthParam->load() / 100.0f;
    const float delayTimeMs = delayTimeParam->load();
    const float frequencyShift = frequencyShiftParam->load();
    const float phaseShift = phaseShiftParam->load() * juce::MathConstants<float>::pi / 180.0f;
    const float highFreqEnhance = highFreqEnhanceParam->load() / 100.0f;
    const float outputLevel = juce::Decibels::decibelsToGain(outputLevelParam->load());
    
    // Update high frequency enhancement filter if needed
    if (std::abs(highFreqEnhance - previousHighFreqEnhance) > 0.001f)
    {
        float gain = 1.0f + highFreqEnhance * 2.0f; // Up to +6dB boost
        auto highShelfCoeffs = juce::IIRCoefficients::makeHighShelf(currentSampleRate, 4000.0, 0.7, gain);
        highFreqFilterLeft.setCoefficients(highShelfCoeffs);
        highFreqFilterRight.setCoefficients(highShelfCoeffs);
        previousHighFreqEnhance = highFreqEnhance;
    }
    
    // Update all-pass filter for phase shifting
    float allPassFreq = 1000.0f + frequencyShift * 50.0f; // Vary filter frequency
    auto allPassCoeffs = juce::dsp::IIR::Coefficients<float>::makeAllPass(currentSampleRate, allPassFreq);
    allPassFilter.state = *allPassCoeffs;
    
    float delaySamples = (delayTimeMs / 1000.0f) * static_cast<float>(currentSampleRate);
    
    auto* leftData = buffer.getWritePointer(0);
    auto* rightData = buffer.getWritePointer(1);
    
    float leftLevelSum = 0.0f;
    float rightLevelSum = 0.0f;
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float input = (leftData[sample] + rightData[sample]) * 0.5f; // Mix to mono first
        
        // Create delayed version
        float delayedSample = delayLine.popSample(0, delaySamples, true);
        delayLine.pushSample(0, input);
        
        // Apply frequency-dependent phase shift using all-pass filter
        float phasedInput = input;
        float phasedDelayed = delayedSample;
        
        // Skip all-pass filter processing for now to avoid performance issues
        // The phase shifting is handled by the phase accumulator below
        
        // Apply phase shift
        phaseAccumulator += (juce::MathConstants<float>::twoPi * frequencyShift) / static_cast<float>(currentSampleRate);
        if (phaseAccumulator >= juce::MathConstants<float>::twoPi)
            phaseAccumulator -= juce::MathConstants<float>::twoPi;
        
        float phaseShiftedInput = phasedInput * std::cos(phaseAccumulator + phaseShift);
        float phaseShiftedDelayed = phasedDelayed * std::cos(phaseAccumulator);
        
        // Create stereo image
        float left = phasedInput + (phaseShiftedDelayed * width);
        float right = phaseShiftedInput + (phasedDelayed * width);
        
        // Apply high frequency enhancement
        left = highFreqFilterLeft.processSingleSampleRaw(left);
        right = highFreqFilterRight.processSingleSampleRaw(right);
        
        // Apply width control using M/S processing
        float mono = (left + right) * 0.5f;
        float side = (left - right) * 0.5f * width;
        
        leftData[sample] = (mono + side) * outputLevel;
        rightData[sample] = (mono - side) * outputLevel;
        
        // Accumulate for metering
        leftLevelSum += std::abs(leftData[sample]);
        rightLevelSum += std::abs(rightData[sample]);
    }
    
    // Update level metering
    leftLevel.store(leftLevelSum / numSamples);
    rightLevel.store(rightLevelSum / numSamples);
}

void QuasiStereoProcessor::calculateStereoWidth(const juce::AudioBuffer<float>& buffer)
{
    if (buffer.getNumChannels() < 2)
    {
        stereoWidth.store(0.0f);
        return;
    }
    
    const int numSamples = buffer.getNumSamples();
    const auto* leftData = buffer.getReadPointer(0);
    const auto* rightData = buffer.getReadPointer(1);
    
    float correlationSum = 0.0f;
    float leftSquareSum = 0.0f;
    float rightSquareSum = 0.0f;
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float left = leftData[sample];
        float right = rightData[sample];
        
        correlationSum += left * right;
        leftSquareSum += left * left;
        rightSquareSum += right * right;
    }
    
    float denominator = std::sqrt(leftSquareSum * rightSquareSum);
    float correlation = (denominator > 0.0f) ? (correlationSum / denominator) : 0.0f;
    
    // Convert correlation to width (1.0 = mono, 0.0 = fully decorrelated)
    float width = 1.0f - std::abs(correlation);
    stereoWidth.store(width);
}

//==============================================================================
juce::AudioProcessorEditor* QuasiStereoProcessor::createEditor()
{
    return new QuasiStereoEditor(*this);
}

//==============================================================================
void QuasiStereoProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto xml = valueTreeState.copyState().createXml();
    copyXmlToBinary(*xml, destData);
}

void QuasiStereoProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml != nullptr && xml->hasTagName(valueTreeState.state.getType()))
        valueTreeState.replaceState(juce::ValueTree::fromXml(*xml));
}