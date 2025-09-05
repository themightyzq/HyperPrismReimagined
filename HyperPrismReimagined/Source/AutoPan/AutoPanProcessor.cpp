//==============================================================================
// HyperPrism Revived - Auto Pan Processor
//==============================================================================

#include "AutoPanProcessor.h"
#include "AutoPanEditor.h"

//==============================================================================
// LFO Implementation
//==============================================================================
void LFO::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    updatePhaseIncrement();
    reset();
}

void LFO::setFrequency(float frequencyHz)
{
    frequency = frequencyHz;
    updatePhaseIncrement();
}

void LFO::setWaveform(int waveformType)
{
    waveform = waveformType;
}

void LFO::reset()
{
    phase = 0.0f;
    randomValue = 0.0f;
    targetRandomValue = 0.0f;
    randomCounter = 0;
}

float LFO::getNextSample()
{
    float output = 0.0f;
    
    switch (waveform)
    {
        case Sine:
            output = std::sin(phase * juce::MathConstants<float>::twoPi);
            break;
            
        case Triangle:
            if (phase < 0.5f)
                output = 4.0f * phase - 1.0f;
            else
                output = 3.0f - 4.0f * phase;
            break;
            
        case Square:
            output = (phase < 0.5f) ? 1.0f : -1.0f;
            break;
            
        case Sawtooth:
            output = 2.0f * phase - 1.0f;
            break;
            
        case Random:
            // Sample and hold random values
            if (randomCounter <= 0)
            {
                targetRandomValue = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
                randomCounter = static_cast<int>(currentSampleRate / (frequency * 8.0f)); // 8 steps per cycle
            }
            
            // Smooth transition to new random value
            const float smoothing = 0.99f;
            randomValue = randomValue * smoothing + targetRandomValue * (1.0f - smoothing);
            output = randomValue;
            randomCounter--;
            break;
    }
    
    // Advance phase
    phase += phaseIncrement;
    if (phase >= 1.0f)
        phase -= 1.0f;
    
    return output;
}

void LFO::updatePhaseIncrement()
{
    phaseIncrement = static_cast<float>(frequency / currentSampleRate);
}

//==============================================================================
// AutoPanProcessor Implementation
//==============================================================================

// Parameter IDs
const juce::String AutoPanProcessor::BYPASS_ID = "bypass";
const juce::String AutoPanProcessor::RATE_ID = "rate";
const juce::String AutoPanProcessor::DEPTH_ID = "depth";
const juce::String AutoPanProcessor::WAVEFORM_ID = "waveform";
const juce::String AutoPanProcessor::PHASE_ID = "phase";
const juce::String AutoPanProcessor::SYNC_ID = "sync";
const juce::String AutoPanProcessor::OUTPUT_LEVEL_ID = "outputLevel";

//==============================================================================
AutoPanProcessor::AutoPanProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      valueTreeState(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache parameter pointers for performance
    bypassParam = valueTreeState.getRawParameterValue(BYPASS_ID);
    rateParam = valueTreeState.getRawParameterValue(RATE_ID);
    depthParam = valueTreeState.getRawParameterValue(DEPTH_ID);
    waveformParam = valueTreeState.getRawParameterValue(WAVEFORM_ID);
    phaseParam = valueTreeState.getRawParameterValue(PHASE_ID);
    syncParam = valueTreeState.getRawParameterValue(SYNC_ID);
    outputLevelParam = valueTreeState.getRawParameterValue(OUTPUT_LEVEL_ID);
}

juce::AudioProcessorValueTreeState::ParameterLayout AutoPanProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    // Bypass
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID, "Bypass", false));

    // Rate (0.1 - 20 Hz)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        RATE_ID, "Rate", 
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.01f, 0.3f), 1.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 2) + " Hz"; }));

    // Depth (0-100%)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DEPTH_ID, "Depth", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 75.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + "%"; }));

    // Waveform
    parameters.push_back(std::make_unique<juce::AudioParameterChoice>(
        WAVEFORM_ID, "Waveform", 
        juce::StringArray{"Sine", "Triangle", "Square", "Sawtooth", "Random"}, 0));

    // Phase (0-360 degrees)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        PHASE_ID, "Phase", 
        juce::NormalisableRange<float>(0.0f, 360.0f, 1.0f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(static_cast<int>(value)) + "°"; }));

    // Sync (for future tempo sync implementation)
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        SYNC_ID, "Tempo Sync", false));

    // Output Level (-20 to +20 dB)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        OUTPUT_LEVEL_ID, "Output Level", 
        juce::NormalisableRange<float>(-20.0f, 20.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    return { parameters.begin(), parameters.end() };
}

//==============================================================================
void AutoPanProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    
    // Prepare LFO
    lfo.prepare(sampleRate);
    
    // Initialize smoothed values
    smoothedLeftGain.reset(sampleRate, 0.02); // 20ms smoothing
    smoothedRightGain.reset(sampleRate, 0.02);
    smoothedLeftGain.setCurrentAndTargetValue(1.0f);
    smoothedRightGain.setCurrentAndTargetValue(1.0f);
    
    // Reset metering
    leftLevel.store(0.0f);
    rightLevel.store(0.0f);
    lfoValue.store(0.0f);
}

void AutoPanProcessor::releaseResources()
{
    // Nothing specific to release
}

bool AutoPanProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void AutoPanProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    
    if (bypassParam->load() > 0.5f)
        return;
        
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    processAutoPan(buffer);
}

void AutoPanProcessor::processAutoPan(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    if (numChannels < 2)
        return; // Need stereo input for auto panning
    
    const float rate = rateParam->load();
    const float depth = depthParam->load() / 100.0f;
    const int waveform = static_cast<int>(waveformParam->load());
    const float phase = phaseParam->load();
    const float outputLevel = juce::Decibels::decibelsToGain(outputLevelParam->load());
    
    // Update LFO parameters
    lfo.setFrequency(rate);
    lfo.setWaveform(waveform);
    
    auto* leftData = buffer.getWritePointer(0);
    auto* rightData = buffer.getWritePointer(1);
    
    float leftLevelSum = 0.0f;
    float rightLevelSum = 0.0f;
    float lfoSum = 0.0f;
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Get LFO value
        float lfoSample = lfo.getNextSample();
        
        // Apply phase offset
        float phaseRadians = phase * juce::MathConstants<float>::pi / 180.0f;
        float phasedLFO = std::sin(std::asin(lfoSample) + phaseRadians);
        
        // Apply depth
        float panValue = phasedLFO * depth;
        
        // Calculate pan gains
        float leftGain, rightGain;
        calculatePanGains(panValue, leftGain, rightGain);
        
        // Apply output level
        leftGain *= outputLevel;
        rightGain *= outputLevel;
        
        // Set target gains for smoothing
        smoothedLeftGain.setTargetValue(leftGain);
        smoothedRightGain.setTargetValue(rightGain);
        
        // Get smoothed gains
        float currentLeftGain = smoothedLeftGain.getNextValue();
        float currentRightGain = smoothedRightGain.getNextValue();
        
        // Store original signals
        float leftInput = leftData[sample];
        float rightInput = rightData[sample];
        
        // Apply panning to combined mono signal
        float monoSignal = (leftInput + rightInput) * 0.5f;
        
        leftData[sample] = monoSignal * currentLeftGain;
        rightData[sample] = monoSignal * currentRightGain;
        
        // Accumulate for metering
        leftLevelSum += std::abs(leftData[sample]);
        rightLevelSum += std::abs(rightData[sample]);
        lfoSum += std::abs(phasedLFO);
    }
    
    // Update metering
    leftLevel.store(leftLevelSum / numSamples);
    rightLevel.store(rightLevelSum / numSamples);
    lfoValue.store(lfoSum / numSamples);
    
    // Update pan position (average of last pan value)
    panPosition.store(lfo.getNextSample() * depth);
    
    // Update LFO phase
    lfoPhase.store(lfo.getPhase());
}

void AutoPanProcessor::calculatePanGains(float panValue, float& leftGain, float& rightGain)
{
    // panValue ranges from -1 (full left) to +1 (full right)
    // Use equal power panning law
    
    float angle = (panValue + 1.0f) * juce::MathConstants<float>::pi * 0.25f; // 0 to π/2
    leftGain = std::cos(angle);
    rightGain = std::sin(angle);
}

//==============================================================================
juce::AudioProcessorEditor* AutoPanProcessor::createEditor()
{
    return new AutoPanEditor(*this);
}

//==============================================================================
void AutoPanProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto xml = valueTreeState.copyState().createXml();
    copyXmlToBinary(*xml, destData);
}

void AutoPanProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml != nullptr && xml->hasTagName(valueTreeState.state.getType()))
        valueTreeState.replaceState(juce::ValueTree::fromXml(*xml));
}