//==============================================================================
// HyperPrism Revived - M+S Matrix Processor
//==============================================================================

#include "MSMatrixProcessor.h"
#include "MSMatrixEditor.h"

// Parameter IDs
const juce::String MSMatrixProcessor::BYPASS_ID = "bypass";
const juce::String MSMatrixProcessor::MATRIX_MODE_ID = "matrixMode";
const juce::String MSMatrixProcessor::MID_LEVEL_ID = "midLevel";
const juce::String MSMatrixProcessor::SIDE_LEVEL_ID = "sideLevel";
const juce::String MSMatrixProcessor::MID_SOLO_ID = "midSolo";
const juce::String MSMatrixProcessor::SIDE_SOLO_ID = "sideSolo";
const juce::String MSMatrixProcessor::STEREO_BALANCE_ID = "stereoBalance";
const juce::String MSMatrixProcessor::OUTPUT_LEVEL_ID = "outputLevel";

//==============================================================================
MSMatrixProcessor::MSMatrixProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      valueTreeState(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache parameter pointers for performance
    bypassParam = valueTreeState.getRawParameterValue(BYPASS_ID);
    matrixModeParam = valueTreeState.getRawParameterValue(MATRIX_MODE_ID);
    midLevelParam = valueTreeState.getRawParameterValue(MID_LEVEL_ID);
    sideLevelParam = valueTreeState.getRawParameterValue(SIDE_LEVEL_ID);
    midSoloParam = valueTreeState.getRawParameterValue(MID_SOLO_ID);
    sideSoloParam = valueTreeState.getRawParameterValue(SIDE_SOLO_ID);
    stereoBalanceParam = valueTreeState.getRawParameterValue(STEREO_BALANCE_ID);
    outputLevelParam = valueTreeState.getRawParameterValue(OUTPUT_LEVEL_ID);
}

juce::AudioProcessorValueTreeState::ParameterLayout MSMatrixProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    // Bypass
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID, "Bypass", false));

    // Matrix Mode
    parameters.push_back(std::make_unique<juce::AudioParameterChoice>(
        MATRIX_MODE_ID, "Matrix Mode", 
        juce::StringArray{"L/R → M/S", "M/S → L/R", "M/S Through"}, 0));

    // Mid Level (-inf to +20 dB)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        MID_LEVEL_ID, "Mid Level", 
        juce::NormalisableRange<float>(-60.0f, 20.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { 
            return value <= -59.9f ? "-∞ dB" : juce::String(value, 1) + " dB"; 
        }));

    // Side Level (-inf to +20 dB)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        SIDE_LEVEL_ID, "Side Level", 
        juce::NormalisableRange<float>(-60.0f, 20.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { 
            return value <= -59.9f ? "-∞ dB" : juce::String(value, 1) + " dB"; 
        }));

    // Mid Solo
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        MID_SOLO_ID, "Mid Solo", false));

    // Side Solo
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        SIDE_SOLO_ID, "Side Solo", false));

    // Stereo Balance (-100% to +100%)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        STEREO_BALANCE_ID, "Stereo Balance", 
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
void MSMatrixProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    
    // Initialize smoothed values
    smoothedMidLevel.reset(sampleRate, 0.05); // 50ms smoothing
    smoothedSideLevel.reset(sampleRate, 0.05);
    smoothedStereoBalance.reset(sampleRate, 0.05);
    
    smoothedMidLevel.setCurrentAndTargetValue(1.0f);
    smoothedSideLevel.setCurrentAndTargetValue(1.0f);
    smoothedStereoBalance.setCurrentAndTargetValue(0.0f);
    
    // Reset metering
    leftLevel.store(0.0f);
    rightLevel.store(0.0f);
    midLevel.store(0.0f);
    sideLevel.store(0.0f);
}

void MSMatrixProcessor::releaseResources()
{
    // Nothing specific to release
}

bool MSMatrixProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void MSMatrixProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    
    if (bypassParam->load() > 0.5f)
        return;
        
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    if (buffer.getNumChannels() < 2)
        return;

    // Route to appropriate processing based on matrix mode
    int matrixMode = static_cast<int>(matrixModeParam->load());
    
    switch (matrixMode)
    {
        case LRToMS:
            processLRToMS(buffer);
            break;
        case MSToLR:
            processMSToLR(buffer);
            break;
        case MSThrough:
            processMSThrough(buffer);
            break;
    }
}

void MSMatrixProcessor::processLRToMS(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    
    const float midLevelDB = midLevelParam->load();
    const float sideLevelDB = sideLevelParam->load();
    const bool midSolo = midSoloParam->load() > 0.5f;
    const bool sideSolo = sideSoloParam->load() > 0.5f;
    const float stereoBalance = stereoBalanceParam->load() / 100.0f; // -1 to +1
    const float outputLevel = juce::Decibels::decibelsToGain(outputLevelParam->load());
    
    // Convert to linear gain
    float midGain = (midLevelDB <= -59.9f) ? 0.0f : juce::Decibels::decibelsToGain(midLevelDB);
    float sideGain = (sideLevelDB <= -59.9f) ? 0.0f : juce::Decibels::decibelsToGain(sideLevelDB);
    
    // Apply solo logic
    if (midSolo && !sideSolo)
        sideGain = 0.0f;
    else if (sideSolo && !midSolo)
        midGain = 0.0f;
    
    // Set smoothed target values
    smoothedMidLevel.setTargetValue(midGain);
    smoothedSideLevel.setTargetValue(sideGain);
    smoothedStereoBalance.setTargetValue(stereoBalance);
    
    auto* leftData = buffer.getWritePointer(0);
    auto* rightData = buffer.getWritePointer(1);
    
    float leftLevelSum = 0.0f;
    float rightLevelSum = 0.0f;
    float midLevelSum = 0.0f;
    float sideLevelSum = 0.0f;
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float left = leftData[sample];
        float right = rightData[sample];
        
        // Encode L/R to M/S
        float mid, side;
        encodeLRToMS(left, right, mid, side);
        
        // Store original M/S for metering
        midLevelSum += std::abs(mid);
        sideLevelSum += std::abs(side);
        
        // Apply smoothed gains
        float currentMidGain = smoothedMidLevel.getNextValue();
        float currentSideGain = smoothedSideLevel.getNextValue();
        float currentBalance = smoothedStereoBalance.getNextValue();
        
        mid *= currentMidGain;
        side *= currentSideGain;
        
        // Decode back to L/R
        float processedLeft, processedRight;
        decodeMSToLR(mid, side, processedLeft, processedRight);
        
        // Apply stereo balance
        float balanceLeftGain = 1.0f;
        float balanceRightGain = 1.0f;
        
        if (currentBalance < 0.0f) // Balance left
        {
            balanceRightGain = 1.0f + currentBalance;
        }
        else if (currentBalance > 0.0f) // Balance right
        {
            balanceLeftGain = 1.0f - currentBalance;
        }
        
        processedLeft *= balanceLeftGain;
        processedRight *= balanceRightGain;
        
        // Apply output level and store
        leftData[sample] = processedLeft * outputLevel;
        rightData[sample] = processedRight * outputLevel;
        
        // Accumulate for output metering
        leftLevelSum += std::abs(leftData[sample]);
        rightLevelSum += std::abs(rightData[sample]);
    }
    
    // Update metering
    leftLevel.store(leftLevelSum / numSamples);
    rightLevel.store(rightLevelSum / numSamples);
    midLevel.store(midLevelSum / numSamples);
    sideLevel.store(sideLevelSum / numSamples);
}

void MSMatrixProcessor::processMSToLR(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    
    const float midLevelDB = midLevelParam->load();
    const float sideLevelDB = sideLevelParam->load();
    const bool midSolo = midSoloParam->load() > 0.5f;
    const bool sideSolo = sideSoloParam->load() > 0.5f;
    const float stereoBalance = stereoBalanceParam->load() / 100.0f;
    const float outputLevel = juce::Decibels::decibelsToGain(outputLevelParam->load());
    
    float midGain = (midLevelDB <= -59.9f) ? 0.0f : juce::Decibels::decibelsToGain(midLevelDB);
    float sideGain = (sideLevelDB <= -59.9f) ? 0.0f : juce::Decibels::decibelsToGain(sideLevelDB);
    
    // Apply solo logic
    if (midSolo && !sideSolo)
        sideGain = 0.0f;
    else if (sideSolo && !midSolo)
        midGain = 0.0f;
    
    smoothedMidLevel.setTargetValue(midGain);
    smoothedSideLevel.setTargetValue(sideGain);
    smoothedStereoBalance.setTargetValue(stereoBalance);
    
    auto* leftData = buffer.getWritePointer(0);   // Mid input
    auto* rightData = buffer.getWritePointer(1);  // Side input
    
    float leftLevelSum = 0.0f;
    float rightLevelSum = 0.0f;
    float midLevelSum = 0.0f;
    float sideLevelSum = 0.0f;
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float mid = leftData[sample];   // Input is M/S format
        float side = rightData[sample];
        
        // Store original M/S for metering
        midLevelSum += std::abs(mid);
        sideLevelSum += std::abs(side);
        
        // Apply smoothed gains
        float currentMidGain = smoothedMidLevel.getNextValue();
        float currentSideGain = smoothedSideLevel.getNextValue();
        float currentBalance = smoothedStereoBalance.getNextValue();
        
        mid *= currentMidGain;
        side *= currentSideGain;
        
        // Decode M/S to L/R
        float left, right;
        decodeMSToLR(mid, side, left, right);
        
        // Apply stereo balance
        float balanceLeftGain = 1.0f;
        float balanceRightGain = 1.0f;
        
        if (currentBalance < 0.0f)
        {
            balanceRightGain = 1.0f + currentBalance;
        }
        else if (currentBalance > 0.0f)
        {
            balanceLeftGain = 1.0f - currentBalance;
        }
        
        left *= balanceLeftGain;
        right *= balanceRightGain;
        
        // Apply output level and store
        leftData[sample] = left * outputLevel;
        rightData[sample] = right * outputLevel;
        
        // Accumulate for output metering
        leftLevelSum += std::abs(leftData[sample]);
        rightLevelSum += std::abs(rightData[sample]);
    }
    
    // Update metering
    leftLevel.store(leftLevelSum / numSamples);
    rightLevel.store(rightLevelSum / numSamples);
    midLevel.store(midLevelSum / numSamples);
    sideLevel.store(sideLevelSum / numSamples);
}

void MSMatrixProcessor::processMSThrough(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    
    const float midLevelDB = midLevelParam->load();
    const float sideLevelDB = sideLevelParam->load();
    const bool midSolo = midSoloParam->load() > 0.5f;
    const bool sideSolo = sideSoloParam->load() > 0.5f;
    const float outputLevel = juce::Decibels::decibelsToGain(outputLevelParam->load());
    
    float midGain = (midLevelDB <= -59.9f) ? 0.0f : juce::Decibels::decibelsToGain(midLevelDB);
    float sideGain = (sideLevelDB <= -59.9f) ? 0.0f : juce::Decibels::decibelsToGain(sideLevelDB);
    
    // Apply solo logic
    if (midSolo && !sideSolo)
        sideGain = 0.0f;
    else if (sideSolo && !midSolo)
        midGain = 0.0f;
    
    smoothedMidLevel.setTargetValue(midGain);
    smoothedSideLevel.setTargetValue(sideGain);
    
    auto* midData = buffer.getWritePointer(0);   // Mid channel
    auto* sideData = buffer.getWritePointer(1);  // Side channel
    
    float leftLevelSum = 0.0f;
    float rightLevelSum = 0.0f;
    float midLevelSum = 0.0f;
    float sideLevelSum = 0.0f;
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float mid = midData[sample];
        float side = sideData[sample];
        
        // Store original for metering
        midLevelSum += std::abs(mid);
        sideLevelSum += std::abs(side);
        
        // Apply smoothed gains
        float currentMidGain = smoothedMidLevel.getNextValue();
        float currentSideGain = smoothedSideLevel.getNextValue();
        
        mid *= currentMidGain * outputLevel;
        side *= currentSideGain * outputLevel;
        
        // Store processed M/S
        midData[sample] = mid;
        sideData[sample] = side;
        
        // For output metering in M/S mode, use M/S values
        leftLevelSum += std::abs(mid);
        rightLevelSum += std::abs(side);
    }
    
    // Update metering
    leftLevel.store(leftLevelSum / numSamples);   // Shows Mid level in L meter
    rightLevel.store(rightLevelSum / numSamples); // Shows Side level in R meter
    midLevel.store(midLevelSum / numSamples);
    sideLevel.store(sideLevelSum / numSamples);
}

void MSMatrixProcessor::encodeLRToMS(float left, float right, float& mid, float& side)
{
    // Standard M/S encoding
    mid = (left + right) * 0.5f;      // Mid = (L + R) / 2
    side = (left - right) * 0.5f;     // Side = (L - R) / 2
}

void MSMatrixProcessor::decodeMSToLR(float mid, float side, float& left, float& right)
{
    // Standard M/S decoding
    left = mid + side;                // L = M + S
    right = mid - side;               // R = M - S
}

//==============================================================================
juce::AudioProcessorEditor* MSMatrixProcessor::createEditor()
{
    return new MSMatrixEditor(*this);
}

//==============================================================================
void MSMatrixProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto xml = valueTreeState.copyState().createXml();
    copyXmlToBinary(*xml, destData);
}

void MSMatrixProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml != nullptr && xml->hasTagName(valueTreeState.state.getType()))
        valueTreeState.replaceState(juce::ValueTree::fromXml(*xml));
}