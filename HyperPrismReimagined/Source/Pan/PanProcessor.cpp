//==============================================================================
// HyperPrism Revived - Pan Processor
//==============================================================================

#include "PanProcessor.h"
#include "PanEditor.h"

// Parameter IDs
const juce::String PanProcessor::BYPASS_ID = "bypass";
const juce::String PanProcessor::PAN_POSITION_ID = "panPosition";
const juce::String PanProcessor::PAN_LAW_ID = "panLaw";
const juce::String PanProcessor::WIDTH_ID = "width";
const juce::String PanProcessor::BALANCE_ID = "balance";
const juce::String PanProcessor::OUTPUT_LEVEL_ID = "outputLevel";

//==============================================================================
PanProcessor::PanProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      valueTreeState(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache parameter pointers for performance
    bypassParam = valueTreeState.getRawParameterValue(BYPASS_ID);
    panPositionParam = valueTreeState.getRawParameterValue(PAN_POSITION_ID);
    panLawParam = valueTreeState.getRawParameterValue(PAN_LAW_ID);
    widthParam = valueTreeState.getRawParameterValue(WIDTH_ID);
    balanceParam = valueTreeState.getRawParameterValue(BALANCE_ID);
    outputLevelParam = valueTreeState.getRawParameterValue(OUTPUT_LEVEL_ID);
}

juce::AudioProcessorValueTreeState::ParameterLayout PanProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    // Bypass
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID, "Bypass", false));

    // Pan Position (-100% to +100%)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        PAN_POSITION_ID, "Pan Position", 
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { 
            if (std::abs(value) < 0.1f) return juce::String("Center");
            return value > 0.0f ? "R" + juce::String(value, 1) : "L" + juce::String(-value, 1); 
        }));

    // Pan Law
    parameters.push_back(std::make_unique<juce::AudioParameterChoice>(
        PAN_LAW_ID, "Pan Law", 
        juce::StringArray{"Linear", "Equal Power", "-3dB", "-6dB"}, 1));

    // Stereo Width (0-200%)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        WIDTH_ID, "Stereo Width", 
        juce::NormalisableRange<float>(0.0f, 200.0f, 0.1f), 100.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + "%"; }));

    // Balance (-100% to +100%)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        BALANCE_ID, "Balance", 
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { 
            if (std::abs(value) < 0.1f) return juce::String("Center");
            return value > 0.0f ? "R" + juce::String(value, 1) : "L" + juce::String(-value, 1); 
        }));

    // Output Level (-20 to +20 dB)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        OUTPUT_LEVEL_ID, "Output Level", 
        juce::NormalisableRange<float>(-20.0f, 20.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    return { parameters.begin(), parameters.end() };
}

//==============================================================================
void PanProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    
    // Initialize smoothed values
    smoothedLeftGain.reset(sampleRate, 0.05); // 50ms smoothing
    smoothedRightGain.reset(sampleRate, 0.05);
    smoothedLeftGain.setCurrentAndTargetValue(1.0f);
    smoothedRightGain.setCurrentAndTargetValue(1.0f);
    
    // Reset metering
    leftLevel.store(0.0f);
    rightLevel.store(0.0f);
}

void PanProcessor::releaseResources()
{
    // Nothing specific to release
}

bool PanProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void PanProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    
    if (bypassParam->load() > 0.5f)
        return;
        
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    processPanning(buffer);
}

void PanProcessor::processPanning(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels < 2)
        return; // Need stereo input for panning
    
    const float panPosition = panPositionParam->load() / 100.0f; // -1 to +1
    const int panLawType = static_cast<int>(panLawParam->load());
    const float width = widthParam->load() / 100.0f; // 0 to 2
    const float balance = balanceParam->load() / 100.0f; // -1 to +1
    const float outputLevel = juce::Decibels::decibelsToGain(outputLevelParam->load());
    
    // Calculate pan gains
    float panLeftGain, panRightGain;
    calculatePanGains(panPosition, panLawType, panLeftGain, panRightGain);
    
    // Apply balance
    float balanceLeftGain = 1.0f;
    float balanceRightGain = 1.0f;
    if (balance < 0.0f) // Balance left
    {
        balanceRightGain = 1.0f + balance;
    }
    else if (balance > 0.0f) // Balance right
    {
        balanceLeftGain = 1.0f - balance;
    }
    
    // Set target gains for smoothing
    smoothedLeftGain.setTargetValue(panLeftGain * balanceLeftGain * outputLevel);
    smoothedRightGain.setTargetValue(panRightGain * balanceRightGain * outputLevel);
    
    auto* leftData = buffer.getWritePointer(0);
    auto* rightData = buffer.getWritePointer(1);
    
    // Store original signals for stereo width processing
    juce::AudioBuffer<float> originalBuffer;
    originalBuffer.makeCopyOf(buffer);
    const auto* originalLeft = originalBuffer.getReadPointer(0);
    const auto* originalRight = originalBuffer.getReadPointer(1);
    
    float leftLevelSum = 0.0f;
    float rightLevelSum = 0.0f;
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float leftInput = originalLeft[sample];
        float rightInput = originalRight[sample];
        
        // Apply stereo width
        float mono = (leftInput + rightInput) * 0.5f;
        float side = (leftInput - rightInput) * 0.5f * width;
        
        float widthLeft = mono + side;
        float widthRight = mono - side;
        
        // Apply smoothed gains
        float currentLeftGain = smoothedLeftGain.getNextValue();
        float currentRightGain = smoothedRightGain.getNextValue();
        
        leftData[sample] = widthLeft * currentLeftGain;
        rightData[sample] = widthRight * currentRightGain;
        
        // Accumulate for metering
        leftLevelSum += std::abs(leftData[sample]);
        rightLevelSum += std::abs(rightData[sample]);
    }
    
    // Update metering
    leftLevel.store(leftLevelSum / numSamples);
    rightLevel.store(rightLevelSum / numSamples);
}

void PanProcessor::calculatePanGains(float panValue, int panLawType, float& leftGain, float& rightGain)
{
    // panValue ranges from -1 (full left) to +1 (full right)
    // 0 = center
    
    switch (panLawType)
    {
        case Linear:
        {
            leftGain = (panValue <= 0.0f) ? 1.0f : (1.0f - panValue);
            rightGain = (panValue >= 0.0f) ? 1.0f : (1.0f + panValue);
            break;
        }
        
        case EqualPower:
        {
            float angle = (panValue + 1.0f) * juce::MathConstants<float>::pi * 0.25f; // 0 to π/2
            leftGain = std::cos(angle);
            rightGain = std::sin(angle);
            break;
        }
        
        case NegThreeDB:
        {
            float absPan = std::abs(panValue);
            float attenuation = absPan * 0.5f; // -3dB at full pan
            
            if (panValue <= 0.0f) // Panning left
            {
                leftGain = 1.0f;
                rightGain = 1.0f - attenuation;
            }
            else // Panning right
            {
                leftGain = 1.0f - attenuation;
                rightGain = 1.0f;
            }
            break;
        }
        
        case NegSixDB:
        {
            float absPanel = std::abs(panValue);
            float attenuation = absPanel; // -6dB at full pan
            
            if (panValue <= 0.0f) // Panning left
            {
                leftGain = 1.0f;
                rightGain = 1.0f - attenuation;
            }
            else // Panning right
            {
                leftGain = 1.0f - attenuation;
                rightGain = 1.0f;
            }
            break;
        }
        
        default:
            leftGain = 1.0f;
            rightGain = 1.0f;
            break;
    }
}

//==============================================================================
juce::AudioProcessorEditor* PanProcessor::createEditor()
{
    return new PanEditor(*this);
}

//==============================================================================
void PanProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto xml = valueTreeState.copyState().createXml();
    copyXmlToBinary(*xml, destData);
}

void PanProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml != nullptr && xml->hasTagName(valueTreeState.state.getType()))
        valueTreeState.replaceState(juce::ValueTree::fromXml(*xml));
}