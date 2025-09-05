//==============================================================================
// HyperPrism Revived - Stereo Dynamics Processor
//==============================================================================

#include "StereoDynamicsProcessor.h"
#include "StereoDynamicsEditor.h"

//==============================================================================
// EnvelopeFollower Implementation
//==============================================================================
void StereoDynamicsProcessor::EnvelopeFollower::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    updateCoefficients();
    reset();
}

void StereoDynamicsProcessor::EnvelopeFollower::setAttackTime(float attackMs)
{
    attackCoeff = std::exp(-1.0f / (attackMs * 0.001f * static_cast<float>(currentSampleRate)));
}

void StereoDynamicsProcessor::EnvelopeFollower::setReleaseTime(float releaseMs)
{
    releaseCoeff = std::exp(-1.0f / (releaseMs * 0.001f * static_cast<float>(currentSampleRate)));
}

void StereoDynamicsProcessor::EnvelopeFollower::reset()
{
    envelope = 0.0f;
}

float StereoDynamicsProcessor::EnvelopeFollower::processSample(float input)
{
    float inputLevel = std::abs(input);
    
    if (inputLevel > envelope)
    {
        // Attack
        envelope = inputLevel + (envelope - inputLevel) * attackCoeff;
    }
    else
    {
        // Release
        envelope = inputLevel + (envelope - inputLevel) * releaseCoeff;
    }
    
    return envelope;
}

void StereoDynamicsProcessor::EnvelopeFollower::updateCoefficients()
{
    // Default values will be overridden by setAttackTime/setReleaseTime
    setAttackTime(10.0f);  // 10ms
    setReleaseTime(100.0f); // 100ms
}

//==============================================================================
// StereoDynamicsProcessor Implementation
//==============================================================================

// Parameter IDs
const juce::String StereoDynamicsProcessor::BYPASS_ID = "bypass";
const juce::String StereoDynamicsProcessor::MID_THRESHOLD_ID = "midThreshold";
const juce::String StereoDynamicsProcessor::MID_RATIO_ID = "midRatio";
const juce::String StereoDynamicsProcessor::SIDE_THRESHOLD_ID = "sideThreshold";
const juce::String StereoDynamicsProcessor::SIDE_RATIO_ID = "sideRatio";
const juce::String StereoDynamicsProcessor::ATTACK_TIME_ID = "attackTime";
const juce::String StereoDynamicsProcessor::RELEASE_TIME_ID = "releaseTime";
const juce::String StereoDynamicsProcessor::OUTPUT_LEVEL_ID = "outputLevel";

//==============================================================================
StereoDynamicsProcessor::StereoDynamicsProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      valueTreeState(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache parameter pointers for performance
    bypassParam = valueTreeState.getRawParameterValue(BYPASS_ID);
    midThresholdParam = valueTreeState.getRawParameterValue(MID_THRESHOLD_ID);
    midRatioParam = valueTreeState.getRawParameterValue(MID_RATIO_ID);
    sideThresholdParam = valueTreeState.getRawParameterValue(SIDE_THRESHOLD_ID);
    sideRatioParam = valueTreeState.getRawParameterValue(SIDE_RATIO_ID);
    attackTimeParam = valueTreeState.getRawParameterValue(ATTACK_TIME_ID);
    releaseTimeParam = valueTreeState.getRawParameterValue(RELEASE_TIME_ID);
    outputLevelParam = valueTreeState.getRawParameterValue(OUTPUT_LEVEL_ID);
}

juce::AudioProcessorValueTreeState::ParameterLayout StereoDynamicsProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    // Bypass
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID, "Bypass", false));

    // Mid Threshold (-60 to 0 dB)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        MID_THRESHOLD_ID, "Mid Threshold", 
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -20.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    // Mid Ratio (1:1 to 20:1)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        MID_RATIO_ID, "Mid Ratio", 
        juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f, 0.3f), 4.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + ":1"; }));

    // Side Threshold (-60 to 0 dB)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        SIDE_THRESHOLD_ID, "Side Threshold", 
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -30.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    // Side Ratio (1:1 to 20:1)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        SIDE_RATIO_ID, "Side Ratio", 
        juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f, 0.3f), 6.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + ":1"; }));

    // Attack Time (0.1 to 100 ms)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        ATTACK_TIME_ID, "Attack Time", 
        juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f, 0.3f), 10.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " ms"; }));

    // Release Time (10 to 5000 ms)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        RELEASE_TIME_ID, "Release Time", 
        juce::NormalisableRange<float>(10.0f, 5000.0f, 1.0f, 0.3f), 100.0f,
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
void StereoDynamicsProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    
    // Prepare envelope followers
    midEnvelopeFollower.prepare(sampleRate);
    sideEnvelopeFollower.prepare(sampleRate);
    
    // Initialize smoothed values
    smoothedMidGain.reset(sampleRate, 0.01); // 10ms smoothing
    smoothedSideGain.reset(sampleRate, 0.01);
    smoothedMidGain.setCurrentAndTargetValue(1.0f);
    smoothedSideGain.setCurrentAndTargetValue(1.0f);
    
    // Reset metering
    leftLevel.store(0.0f);
    rightLevel.store(0.0f);
    midLevel.store(0.0f);
    sideLevel.store(0.0f);
    midGainReduction.store(0.0f);
    sideGainReduction.store(0.0f);
}

void StereoDynamicsProcessor::releaseResources()
{
    // Reset envelope followers
    midEnvelopeFollower.reset();
    sideEnvelopeFollower.reset();
}

bool StereoDynamicsProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void StereoDynamicsProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
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

    processStereoDynamics(buffer);
}

void StereoDynamicsProcessor::processStereoDynamics(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    
    const float midThreshold = juce::Decibels::decibelsToGain(midThresholdParam->load());
    const float midRatio = midRatioParam->load();
    const float sideThreshold = juce::Decibels::decibelsToGain(sideThresholdParam->load());
    const float sideRatio = sideRatioParam->load();
    const float attackTime = attackTimeParam->load();
    const float releaseTime = releaseTimeParam->load();
    const float outputLevel = juce::Decibels::decibelsToGain(outputLevelParam->load());
    
    // Update envelope follower parameters
    midEnvelopeFollower.setAttackTime(attackTime);
    midEnvelopeFollower.setReleaseTime(releaseTime);
    sideEnvelopeFollower.setAttackTime(attackTime);
    sideEnvelopeFollower.setReleaseTime(releaseTime);
    
    auto* leftData = buffer.getWritePointer(0);
    auto* rightData = buffer.getWritePointer(1);
    
    float leftLevelSum = 0.0f;
    float rightLevelSum = 0.0f;
    float midLevelSum = 0.0f;
    float sideLevelSum = 0.0f;
    float midGainReductionSum = 0.0f;
    float sideGainReductionSum = 0.0f;
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float left = leftData[sample];
        float right = rightData[sample];
        
        // Encode L/R to M/S
        float mid, side;
        encodeLRToMS(left, right, mid, side);
        
        // Store original M/S levels for metering
        midLevelSum += std::abs(mid);
        sideLevelSum += std::abs(side);
        
        // Process dynamics on M/S channels separately
        
        // Mid channel dynamics
        float midEnvelope = midEnvelopeFollower.processSample(mid);
        float midGain = 1.0f;
        if (midEnvelope > midThreshold)
        {
            midGain = calculateGainReduction(midEnvelope, midThreshold, midRatio);
        }
        smoothedMidGain.setTargetValue(midGain);
        float currentMidGain = smoothedMidGain.getNextValue();
        mid *= currentMidGain;
        
        // Side channel dynamics
        float sideEnvelope = sideEnvelopeFollower.processSample(side);
        float sideGain = 1.0f;
        if (sideEnvelope > sideThreshold)
        {
            sideGain = calculateGainReduction(sideEnvelope, sideThreshold, sideRatio);
        }
        smoothedSideGain.setTargetValue(sideGain);
        float currentSideGain = smoothedSideGain.getNextValue();
        side *= currentSideGain;
        
        // Store gain reduction for metering (in dB)
        midGainReductionSum += juce::Decibels::gainToDecibels(currentMidGain);
        sideGainReductionSum += juce::Decibels::gainToDecibels(currentSideGain);
        
        // Decode M/S back to L/R
        float processedLeft, processedRight;
        decodeMSToLR(mid, side, processedLeft, processedRight);
        
        // Apply output level
        processedLeft *= outputLevel;
        processedRight *= outputLevel;
        
        // Store processed audio
        leftData[sample] = processedLeft;
        rightData[sample] = processedRight;
        
        // Accumulate for output level metering
        leftLevelSum += std::abs(processedLeft);
        rightLevelSum += std::abs(processedRight);
    }
    
    // Update metering
    leftLevel.store(leftLevelSum / numSamples);
    rightLevel.store(rightLevelSum / numSamples);
    midLevel.store(midLevelSum / numSamples);
    sideLevel.store(sideLevelSum / numSamples);
    
    // Gain reduction metering (convert to positive values for display)
    float avgMidGR = std::abs(midGainReductionSum / numSamples);
    float avgSideGR = std::abs(sideGainReductionSum / numSamples);
    midGainReduction.store(avgMidGR);
    sideGainReduction.store(avgSideGR);
}

float StereoDynamicsProcessor::calculateGainReduction(float level, float threshold, float ratio)
{
    if (level <= threshold)
        return 1.0f;
    
    // Calculate how much above threshold
    float overThreshold = level - threshold;
    
    // Apply compression ratio
    float compressedOverThreshold = overThreshold / ratio;
    
    // Calculate target level
    float targetLevel = threshold + compressedOverThreshold;
    
    // Return gain reduction factor
    return targetLevel / level;
}

void StereoDynamicsProcessor::encodeLRToMS(float left, float right, float& mid, float& side)
{
    // Standard M/S encoding
    mid = (left + right) * 0.5f;      // Mid = (L + R) / 2
    side = (left - right) * 0.5f;     // Side = (L - R) / 2
}

void StereoDynamicsProcessor::decodeMSToLR(float mid, float side, float& left, float& right)
{
    // Standard M/S decoding
    left = mid + side;                // L = M + S
    right = mid - side;               // R = M - S
}

//==============================================================================
juce::AudioProcessorEditor* StereoDynamicsProcessor::createEditor()
{
    return new StereoDynamicsEditor(*this);
}

//==============================================================================
void StereoDynamicsProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto xml = valueTreeState.copyState().createXml();
    copyXmlToBinary(*xml, destData);
}

void StereoDynamicsProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml != nullptr && xml->hasTagName(valueTreeState.state.getType()))
        valueTreeState.replaceState(juce::ValueTree::fromXml(*xml));
}