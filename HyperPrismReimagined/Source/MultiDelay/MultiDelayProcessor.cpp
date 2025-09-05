//==============================================================================
// HyperPrism Revived - Multi Delay Processor
//==============================================================================

#include "MultiDelayProcessor.h"
#include "MultiDelayEditor.h"

// Parameter IDs
const juce::String MultiDelayProcessor::BYPASS_ID = "bypass";
const juce::String MultiDelayProcessor::MASTER_MIX_ID = "masterMix";
const juce::String MultiDelayProcessor::GLOBAL_FEEDBACK_ID = "globalFeedback";

const juce::String MultiDelayProcessor::DELAY1_TIME_ID = "delay1Time";
const juce::String MultiDelayProcessor::DELAY1_LEVEL_ID = "delay1Level";
const juce::String MultiDelayProcessor::DELAY1_PAN_ID = "delay1Pan";
const juce::String MultiDelayProcessor::DELAY1_FEEDBACK_ID = "delay1Feedback";

const juce::String MultiDelayProcessor::DELAY2_TIME_ID = "delay2Time";
const juce::String MultiDelayProcessor::DELAY2_LEVEL_ID = "delay2Level";
const juce::String MultiDelayProcessor::DELAY2_PAN_ID = "delay2Pan";
const juce::String MultiDelayProcessor::DELAY2_FEEDBACK_ID = "delay2Feedback";

const juce::String MultiDelayProcessor::DELAY3_TIME_ID = "delay3Time";
const juce::String MultiDelayProcessor::DELAY3_LEVEL_ID = "delay3Level";
const juce::String MultiDelayProcessor::DELAY3_PAN_ID = "delay3Pan";
const juce::String MultiDelayProcessor::DELAY3_FEEDBACK_ID = "delay3Feedback";

const juce::String MultiDelayProcessor::DELAY4_TIME_ID = "delay4Time";
const juce::String MultiDelayProcessor::DELAY4_LEVEL_ID = "delay4Level";
const juce::String MultiDelayProcessor::DELAY4_PAN_ID = "delay4Pan";
const juce::String MultiDelayProcessor::DELAY4_FEEDBACK_ID = "delay4Feedback";

//==============================================================================
MultiDelayProcessor::MultiDelayProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      valueTreeState(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache parameter pointers for performance
    bypassParam = valueTreeState.getRawParameterValue(BYPASS_ID);
    masterMixParam = valueTreeState.getRawParameterValue(MASTER_MIX_ID);
    globalFeedbackParam = valueTreeState.getRawParameterValue(GLOBAL_FEEDBACK_ID);
    
    // Cache delay parameter pointers
    delayTimeParams[0] = valueTreeState.getRawParameterValue(DELAY1_TIME_ID);
    delayLevelParams[0] = valueTreeState.getRawParameterValue(DELAY1_LEVEL_ID);
    delayPanParams[0] = valueTreeState.getRawParameterValue(DELAY1_PAN_ID);
    delayFeedbackParams[0] = valueTreeState.getRawParameterValue(DELAY1_FEEDBACK_ID);
    
    delayTimeParams[1] = valueTreeState.getRawParameterValue(DELAY2_TIME_ID);
    delayLevelParams[1] = valueTreeState.getRawParameterValue(DELAY2_LEVEL_ID);
    delayPanParams[1] = valueTreeState.getRawParameterValue(DELAY2_PAN_ID);
    delayFeedbackParams[1] = valueTreeState.getRawParameterValue(DELAY2_FEEDBACK_ID);
    
    delayTimeParams[2] = valueTreeState.getRawParameterValue(DELAY3_TIME_ID);
    delayLevelParams[2] = valueTreeState.getRawParameterValue(DELAY3_LEVEL_ID);
    delayPanParams[2] = valueTreeState.getRawParameterValue(DELAY3_PAN_ID);
    delayFeedbackParams[2] = valueTreeState.getRawParameterValue(DELAY3_FEEDBACK_ID);
    
    delayTimeParams[3] = valueTreeState.getRawParameterValue(DELAY4_TIME_ID);
    delayLevelParams[3] = valueTreeState.getRawParameterValue(DELAY4_LEVEL_ID);
    delayPanParams[3] = valueTreeState.getRawParameterValue(DELAY4_PAN_ID);
    delayFeedbackParams[3] = valueTreeState.getRawParameterValue(DELAY4_FEEDBACK_ID);
}

juce::AudioProcessorValueTreeState::ParameterLayout MultiDelayProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    // Bypass
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID, "Bypass", false));

    // Master Mix (0-100%)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        MASTER_MIX_ID, "Master Mix", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + "%"; }));

    // Global Feedback (0-90%)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        GLOBAL_FEEDBACK_ID, "Global Feedback", 
        juce::NormalisableRange<float>(0.0f, 90.0f, 0.1f), 15.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + "%"; }));

    // Delay 1 parameters
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DELAY1_TIME_ID, "Delay 1 Time", 
        juce::NormalisableRange<float>(1.0f, 2000.0f, 0.1f, 0.3f), 125.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " ms"; }));
    
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DELAY1_LEVEL_ID, "Delay 1 Level", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 75.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + "%"; }));
    
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DELAY1_PAN_ID, "Delay 1 Pan", 
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), -50.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return value == 0.0f ? "Center" : 
            (value > 0.0f ? "R" + juce::String(value, 0) : "L" + juce::String(-value, 0)); }));
    
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DELAY1_FEEDBACK_ID, "Delay 1 Feedback", 
        juce::NormalisableRange<float>(0.0f, 90.0f, 0.1f), 25.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + "%"; }));

    // Delay 2 parameters
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DELAY2_TIME_ID, "Delay 2 Time", 
        juce::NormalisableRange<float>(1.0f, 2000.0f, 0.1f, 0.3f), 250.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " ms"; }));
    
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DELAY2_LEVEL_ID, "Delay 2 Level", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 60.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + "%"; }));
    
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DELAY2_PAN_ID, "Delay 2 Pan", 
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), 50.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return value == 0.0f ? "Center" : 
            (value > 0.0f ? "R" + juce::String(value, 0) : "L" + juce::String(-value, 0)); }));
    
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DELAY2_FEEDBACK_ID, "Delay 2 Feedback", 
        juce::NormalisableRange<float>(0.0f, 90.0f, 0.1f), 35.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + "%"; }));

    // Delay 3 parameters
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DELAY3_TIME_ID, "Delay 3 Time", 
        juce::NormalisableRange<float>(1.0f, 2000.0f, 0.1f, 0.3f), 500.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " ms"; }));
    
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DELAY3_LEVEL_ID, "Delay 3 Level", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 45.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + "%"; }));
    
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DELAY3_PAN_ID, "Delay 3 Pan", 
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), -25.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return value == 0.0f ? "Center" : 
            (value > 0.0f ? "R" + juce::String(value, 0) : "L" + juce::String(-value, 0)); }));
    
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DELAY3_FEEDBACK_ID, "Delay 3 Feedback", 
        juce::NormalisableRange<float>(0.0f, 90.0f, 0.1f), 20.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + "%"; }));

    // Delay 4 parameters
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DELAY4_TIME_ID, "Delay 4 Time", 
        juce::NormalisableRange<float>(1.0f, 2000.0f, 0.1f, 0.3f), 750.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " ms"; }));
    
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DELAY4_LEVEL_ID, "Delay 4 Level", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 30.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + "%"; }));
    
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DELAY4_PAN_ID, "Delay 4 Pan", 
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), 25.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return value == 0.0f ? "Center" : 
            (value > 0.0f ? "R" + juce::String(value, 0) : "L" + juce::String(-value, 0)); }));
    
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DELAY4_FEEDBACK_ID, "Delay 4 Feedback", 
        juce::NormalisableRange<float>(0.0f, 90.0f, 0.1f), 15.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + "%"; }));

    return { parameters.begin(), parameters.end() };
}

//==============================================================================
void MultiDelayProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    // Prepare all delay lines
    for (auto& delayLine : delayLines)
    {
        delayLine.leftDelay.prepare({ sampleRate, static_cast<juce::uint32>(samplesPerBlock), 1 });
        delayLine.rightDelay.prepare({ sampleRate, static_cast<juce::uint32>(samplesPerBlock), 1 });
        delayLine.leftDelay.reset();
        delayLine.rightDelay.reset();
        delayLine.levelMeter.store(0.0f);
    }
    
    // Reset metering
    inputLevel.store(0.0f);
    outputLevel.store(0.0f);
}

void MultiDelayProcessor::releaseResources()
{
    // Nothing specific to release
}

bool MultiDelayProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void MultiDelayProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    
    if (bypassParam->load() > 0.5f)
        return;
        
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    processMultiDelay(buffer);
}

void MultiDelayProcessor::processMultiDelay(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    const float masterMix = masterMixParam->load() / 100.0f;
    const float globalFeedback = globalFeedbackParam->load() / 100.0f;
    
    // Input level metering
    float inputRMS = buffer.getRMSLevel(0, 0, numSamples);
    if (numChannels > 1)
        inputRMS = std::max(inputRMS, buffer.getRMSLevel(1, 0, numSamples));
    inputLevel.store(inputRMS);
    
    // Create copies for dry signal
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);
    
    // Clear output buffer for wet signal accumulation
    buffer.clear();
    
    // Process each delay line
    for (int delayIndex = 0; delayIndex < NUM_DELAYS; ++delayIndex)
    {
        const float delayTimeMs = delayTimeParams[delayIndex]->load();
        const float delayLevel = delayLevelParams[delayIndex]->load() / 100.0f;
        const float delayPan = delayPanParams[delayIndex]->load() / 100.0f; // -1 to +1
        const float delayFeedback = delayFeedbackParams[delayIndex]->load() / 100.0f;
        
        if (delayLevel < 0.001f) // Skip if level is essentially zero
            continue;
            
        float delaySamples = (delayTimeMs / 1000.0f) * static_cast<float>(currentSampleRate);
        
        // Calculate pan coefficients
        float leftPanGain = 1.0f;
        float rightPanGain = 1.0f;
        
        if (delayPan < 0.0f) // Pan left
        {
            rightPanGain = 1.0f + delayPan; // Reduce right channel
        }
        else if (delayPan > 0.0f) // Pan right
        {
            leftPanGain = 1.0f - delayPan; // Reduce left channel
        }
        
        auto& delayLine = delayLines[delayIndex];
        float delayLevelSum = 0.0f;
        
        for (int channel = 0; channel < numChannels; ++channel)
        {
            const auto* dryData = dryBuffer.getReadPointer(channel);
            auto* wetData = buffer.getWritePointer(channel);
            auto& currentDelayLine = (channel == 0) ? delayLine.leftDelay : delayLine.rightDelay;
            
            float panGain = (channel == 0) ? leftPanGain : rightPanGain;
            
            for (int sample = 0; sample < numSamples; ++sample)
            {
                float input = dryData[sample];
                
                // Get delayed sample
                float delayedSample = currentDelayLine.popSample(0, delaySamples, true);
                
                // Apply local feedback + global feedback from all delays
                float feedbackSum = delayedSample * delayFeedback;
                
                // Add global feedback from all other delay lines
                for (int otherDelay = 0; otherDelay < NUM_DELAYS; ++otherDelay)
                {
                    if (otherDelay != delayIndex)
                    {
                        auto& otherDelayLine = (channel == 0) ? delayLines[otherDelay].leftDelay : delayLines[otherDelay].rightDelay;
                        float otherDelayTime = (delayTimeParams[otherDelay]->load() / 1000.0f) * static_cast<float>(currentSampleRate);
                        float otherDelayedSample = otherDelayLine.popSample(0, otherDelayTime, true);
                        feedbackSum += otherDelayedSample * globalFeedback * 0.25f; // Attenuated global feedback
                    }
                }
                
                float feedbackInput = input + feedbackSum;
                
                // Push to delay line
                currentDelayLine.pushSample(0, feedbackInput);
                
                // Add to output with level, pan, and master mix
                wetData[sample] += delayedSample * delayLevel * panGain;
                
                // Accumulate for level metering
                delayLevelSum += std::abs(delayedSample) * delayLevel;
            }
        }
        
        // Update delay line meter
        delayLine.levelMeter.store(delayLevelSum / (numSamples * numChannels));
    }
    
    // Mix dry and wet signals
    for (int channel = 0; channel < numChannels; ++channel)
    {
        const auto* dryData = dryBuffer.getReadPointer(channel);
        auto* outputData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            outputData[sample] = (dryData[sample] * (1.0f - masterMix)) + (outputData[sample] * masterMix);
        }
    }
    
    // Output level metering
    float outputRMS = buffer.getRMSLevel(0, 0, numSamples);
    if (numChannels > 1)
        outputRMS = std::max(outputRMS, buffer.getRMSLevel(1, 0, numSamples));
    outputLevel.store(outputRMS);
}

std::array<float, 4> MultiDelayProcessor::getDelayLevels() const
{
    std::array<float, 4> levels;
    for (int i = 0; i < NUM_DELAYS; ++i)
    {
        levels[i] = delayLines[i].levelMeter.load();
    }
    return levels;
}

//==============================================================================
juce::AudioProcessorEditor* MultiDelayProcessor::createEditor()
{
    return new MultiDelayEditor(*this);
}

//==============================================================================
void MultiDelayProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto xml = valueTreeState.copyState().createXml();
    copyXmlToBinary(*xml, destData);
}

void MultiDelayProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml != nullptr && xml->hasTagName(valueTreeState.state.getType()))
        valueTreeState.replaceState(juce::ValueTree::fromXml(*xml));
}