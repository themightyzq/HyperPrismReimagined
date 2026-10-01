//==============================================================================
// HyperPrism Reimagined - Multi Delay Processor
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
    
    // Smoothers start at the current parameter values
    masterMixSmoothed.reset (sampleRate, 0.03);
    masterMixSmoothed.setCurrentAndTargetValue (masterMixParam->load());
    globalFeedbackSmoothed.reset (sampleRate, 0.03);
    globalFeedbackSmoothed.setCurrentAndTargetValue (globalFeedbackParam->load());
    
    for (size_t d = 0; d < static_cast<size_t>(NUM_DELAYS); ++d)
    {
        delayTimeSmoothed[d].reset (sampleRate, 0.03);
        delayTimeSmoothed[d].setCurrentAndTargetValue (delayTimeParams[d]->load());
        delayLevelSmoothed[d].reset (sampleRate, 0.03);
        delayLevelSmoothed[d].setCurrentAndTargetValue (delayLevelParams[d]->load());
        delayPanSmoothed[d].reset (sampleRate, 0.03);
        delayPanSmoothed[d].setCurrentAndTargetValue (delayPanParams[d]->load());
        delayFeedbackSmoothed[d].reset (sampleRate, 0.03);
        delayFeedbackSmoothed[d].setCurrentAndTargetValue (delayFeedbackParams[d]->load());
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
    const int numChannels = juce::jmin(buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    
    masterMixSmoothed.setTargetValue(masterMixParam->load());
    globalFeedbackSmoothed.setTargetValue(globalFeedbackParam->load());
    
    for (size_t d = 0; d < static_cast<size_t>(NUM_DELAYS); ++d)
    {
        delayTimeSmoothed[d].setTargetValue(delayTimeParams[d]->load());
        delayLevelSmoothed[d].setTargetValue(delayLevelParams[d]->load());
        delayPanSmoothed[d].setTargetValue(delayPanParams[d]->load());
        delayFeedbackSmoothed[d].setTargetValue(delayFeedbackParams[d]->load());
    }
    
    // Input level metering
    float inputRMS = buffer.getRMSLevel(0, 0, numSamples);
    if (numChannels > 1)
        inputRMS = std::max(inputRMS, buffer.getRMSLevel(1, 0, numSamples));
    inputLevel.store(inputRMS);
    
    // Per-tap settings, recomputed from the smoothed parameters once per sample
    std::array<bool, NUM_DELAYS> tapActive {};
    std::array<float, NUM_DELAYS> tapDelaySamples {};
    std::array<float, NUM_DELAYS> tapLevel {};
    std::array<float, NUM_DELAYS> tapFeedback {};
    std::array<float, NUM_DELAYS> tapLeftGain {};
    std::array<float, NUM_DELAYS> tapRightGain {};
    std::array<float, NUM_DELAYS> tapLevelSum {};
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float masterMix = masterMixSmoothed.getNextValue() / 100.0f;
        const float globalFeedback = globalFeedbackSmoothed.getNextValue() / 100.0f;
        
        for (size_t d = 0; d < static_cast<size_t>(NUM_DELAYS); ++d)
        {
            const float delayTimeMs = delayTimeSmoothed[d].getNextValue();
            const float delayPan = delayPanSmoothed[d].getNextValue() / 100.0f; // -1 to +1
            
            tapLevel[d] = delayLevelSmoothed[d].getNextValue() / 100.0f;
            tapFeedback[d] = delayFeedbackSmoothed[d].getNextValue() / 100.0f;
            // A tap whose level is essentially zero is silent: it adds nothing to the output
            // or the global feedback, but its line keeps running (below) so that raising the
            // level later echoes recent input, not audio left over from when it was last on.
            tapActive[d] = tapLevel[d] >= 0.001f;
            tapDelaySamples[d] = (delayTimeMs / 1000.0f) * static_cast<float>(currentSampleRate);
            
            // Pan coefficients: panning left reduces the right channel and vice versa
            tapLeftGain[d] = delayPan > 0.0f ? 1.0f - delayPan : 1.0f;
            tapRightGain[d] = delayPan < 0.0f ? 1.0f + delayPan : 1.0f;
        }
        
        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* data = buffer.getWritePointer(channel);
            const float input = data[sample];
            
            // Read every tap exactly once per sample. popSample(..., true) advances that
            // line's read pointer, so a second read of the same tap (as the old global
            // feedback sum did) would shift its echo time.
            std::array<float, NUM_DELAYS> delayed {};
            float delayedSum = 0.0f;
            
            for (size_t d = 0; d < static_cast<size_t>(NUM_DELAYS); ++d)
            {
                auto& line = (channel == 0) ? delayLines[d].leftDelay : delayLines[d].rightDelay;
                delayed[d] = line.popSample(0, tapDelaySamples[d], true);
                
                if (tapActive[d])
                    delayedSum += delayed[d];
            }
            
            float wet = 0.0f;
            
            for (size_t d = 0; d < static_cast<size_t>(NUM_DELAYS); ++d)
            {
                auto& line = (channel == 0) ? delayLines[d].leftDelay : delayLines[d].rightDelay;
                
                if (! tapActive[d])
                {
                    // Keep recording the dry input only, so no feedback builds up unheard.
                    line.pushSample(0, input);
                    continue;
                }
                
                // Local feedback + attenuated global feedback from the other taps
                const float feedbackSum = delayed[d] * tapFeedback[d]
                                        + (delayedSum - delayed[d]) * globalFeedback * 0.25f;
                
                line.pushSample(0, input + feedbackSum);
                
                const float panGain = (channel == 0) ? tapLeftGain[d] : tapRightGain[d];
                wet += delayed[d] * tapLevel[d] * panGain;
                
                // Accumulate for level metering
                tapLevelSum[d] += std::abs(delayed[d]) * tapLevel[d];
            }
            
            // Mix dry and wet signals
            data[sample] = (input * (1.0f - masterMix)) + (wet * masterMix);
        }
    }
    
    // Update delay line meters (inactive taps read 0)
    const float meterScale = 1.0f / static_cast<float>(juce::jmax(1, numSamples * numChannels));
    for (size_t d = 0; d < static_cast<size_t>(NUM_DELAYS); ++d)
        delayLines[d].levelMeter.store(tapLevelSum[d] * meterScale);
    
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
        levels[static_cast<size_t>(i)] = delayLines[static_cast<size_t>(i)].levelMeter.load();
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
    auto state = valueTreeState.copyState();
    state.setProperty("editor_width", getEditorWidth(), nullptr);
    state.setProperty("editor_height", getEditorHeight(), nullptr);
    auto xml = state.createXml();
    copyXmlToBinary(*xml, destData);
}

void MultiDelayProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml != nullptr && xml->hasTagName(valueTreeState.state.getType()))
    {
        auto newState = juce::ValueTree::fromXml(*xml);
        setEditorSize(static_cast<int>(newState.getProperty("editor_width", 0)),
                      static_cast<int>(newState.getProperty("editor_height", 0)));
        newState.removeProperty("editor_width", nullptr);
        newState.removeProperty("editor_height", nullptr);
        valueTreeState.replaceState(newState);
    }
}