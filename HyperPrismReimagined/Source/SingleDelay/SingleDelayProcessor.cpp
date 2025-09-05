//==============================================================================
// HyperPrism Revived - Single Delay Processor
//==============================================================================

#include "SingleDelayProcessor.h"
#include "SingleDelayEditor.h"

// Parameter IDs
const juce::String SingleDelayProcessor::BYPASS_ID = "bypass";
const juce::String SingleDelayProcessor::DELAY_TIME_ID = "delayTime";
const juce::String SingleDelayProcessor::FEEDBACK_ID = "feedback";
const juce::String SingleDelayProcessor::WETDRY_MIX_ID = "wetDryMix";
const juce::String SingleDelayProcessor::HIGH_CUT_ID = "highCut";
const juce::String SingleDelayProcessor::LOW_CUT_ID = "lowCut";
const juce::String SingleDelayProcessor::STEREO_SPREAD_ID = "stereoSpread";

//==============================================================================
SingleDelayProcessor::SingleDelayProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      valueTreeState(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache parameter pointers for performance
    bypassParam = valueTreeState.getRawParameterValue(BYPASS_ID);
    delayTimeParam = valueTreeState.getRawParameterValue(DELAY_TIME_ID);
    feedbackParam = valueTreeState.getRawParameterValue(FEEDBACK_ID);
    wetDryMixParam = valueTreeState.getRawParameterValue(WETDRY_MIX_ID);
    highCutParam = valueTreeState.getRawParameterValue(HIGH_CUT_ID);
    lowCutParam = valueTreeState.getRawParameterValue(LOW_CUT_ID);
    stereoSpreadParam = valueTreeState.getRawParameterValue(STEREO_SPREAD_ID);
}

juce::AudioProcessorValueTreeState::ParameterLayout SingleDelayProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    // Bypass
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID, "Bypass", false));

    // Delay Time (1ms to 2000ms)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DELAY_TIME_ID, "Delay Time", 
        juce::NormalisableRange<float>(1.0f, 2000.0f, 0.1f, 0.3f), 250.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " ms"; }));

    // Feedback (0-95%)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        FEEDBACK_ID, "Feedback", 
        juce::NormalisableRange<float>(0.0f, 95.0f, 0.1f), 25.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + "%"; }));

    // Wet/Dry Mix (0-100%)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        WETDRY_MIX_ID, "Wet/Dry Mix", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 30.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + "%"; }));

    // High Cut (500Hz to 20kHz)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        HIGH_CUT_ID, "High Cut", 
        juce::NormalisableRange<float>(500.0f, 20000.0f, 1.0f, 0.3f), 8000.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return value < 1000.0f ? 
            juce::String(static_cast<int>(value)) + " Hz" : 
            juce::String(value / 1000.0f, 1) + " kHz"; }));

    // Low Cut (20Hz to 1kHz)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        LOW_CUT_ID, "Low Cut", 
        juce::NormalisableRange<float>(20.0f, 1000.0f, 1.0f, 0.3f), 80.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(static_cast<int>(value)) + " Hz"; }));

    // Stereo Spread (0-100%)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        STEREO_SPREAD_ID, "Stereo Spread", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + "%"; }));

    return { parameters.begin(), parameters.end() };
}

//==============================================================================
void SingleDelayProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    currentSampleRate = sampleRate;
    
    // Prepare delay lines
    delayLineLeft.prepare({ sampleRate, static_cast<juce::uint32>(samplesPerBlock), 1 });
    delayLineRight.prepare({ sampleRate, static_cast<juce::uint32>(samplesPerBlock), 1 });
    
    delayLineLeft.reset();
    delayLineRight.reset();
    
    // Initialize filters
    updateFilters();
    
    // Reset metering
    inputLevel.store(0.0f);
    outputLevel.store(0.0f);
}

void SingleDelayProcessor::releaseResources()
{
    // Reset filters
    highCutFilterLeft.reset();
    highCutFilterRight.reset();
    lowCutFilterLeft.reset();
    lowCutFilterRight.reset();
}

bool SingleDelayProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void SingleDelayProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    
    if (bypassParam->load() > 0.5f)
        return;
        
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    updateFilters();
    processDelay(buffer);
}

void SingleDelayProcessor::processDelay(juce::AudioBuffer<float>& buffer)
{
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    const float delayTimeMs = delayTimeParam->load();
    const float feedback = feedbackParam->load() / 100.0f;
    const float wetDryMix = wetDryMixParam->load() / 100.0f;
    const float stereoSpread = stereoSpreadParam->load() / 100.0f;
    
    // Calculate delay time in samples
    float delaySamples = (delayTimeMs / 1000.0f) * static_cast<float>(currentSampleRate);
    
    // Apply stereo spread (different delay times for L/R)
    float delayLeft = delaySamples;
    float delayRight = delaySamples * (1.0f + stereoSpread * 0.1f); // Up to 10% difference
    
    // Input level metering
    float inputRMS = buffer.getRMSLevel(0, 0, numSamples);
    if (numChannels > 1)
        inputRMS = std::max(inputRMS, buffer.getRMSLevel(1, 0, numSamples));
    inputLevel.store(inputRMS);
    
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        auto& delayLine = (channel == 0) ? delayLineLeft : delayLineRight;
        auto& highCutFilter = (channel == 0) ? highCutFilterLeft : highCutFilterRight;
        auto& lowCutFilter = (channel == 0) ? lowCutFilterLeft : lowCutFilterRight;
        
        float currentDelay = (channel == 0) ? delayLeft : delayRight;
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float input = channelData[sample];
            
            // Get delayed sample
            float delayedSample = delayLine.popSample(0, currentDelay, true);
            
            // Apply feedback filtering
            delayedSample = highCutFilter.processSingleSampleRaw(delayedSample);
            delayedSample = lowCutFilter.processSingleSampleRaw(delayedSample);
            
            // Calculate feedback input
            float feedbackInput = input + (delayedSample * feedback);
            
            // Push to delay line
            delayLine.pushSample(0, feedbackInput);
            
            // Mix wet and dry signals
            float dryLevel = 1.0f - wetDryMix;
            float wetLevel = wetDryMix;
            
            channelData[sample] = (input * dryLevel) + (delayedSample * wetLevel);
        }
    }
    
    // Output level metering
    float outputRMS = buffer.getRMSLevel(0, 0, numSamples);
    if (numChannels > 1)
        outputRMS = std::max(outputRMS, buffer.getRMSLevel(1, 0, numSamples));
    outputLevel.store(outputRMS);
}

void SingleDelayProcessor::updateFilters()
{
    const float highCut = highCutParam->load();
    const float lowCut = lowCutParam->load();
    
    if (std::abs(highCut - previousHighCut) > 1.0f ||
        std::abs(lowCut - previousLowCut) > 1.0f)
    {
        // High cut filter (low-pass)
        auto highCutCoeffs = juce::IIRCoefficients::makeLowPass(currentSampleRate, highCut);
        highCutFilterLeft.setCoefficients(highCutCoeffs);
        highCutFilterRight.setCoefficients(highCutCoeffs);
        
        // Low cut filter (high-pass)
        auto lowCutCoeffs = juce::IIRCoefficients::makeHighPass(currentSampleRate, lowCut);
        lowCutFilterLeft.setCoefficients(lowCutCoeffs);
        lowCutFilterRight.setCoefficients(lowCutCoeffs);
        
        previousHighCut = highCut;
        previousLowCut = lowCut;
    }
}

//==============================================================================
juce::AudioProcessorEditor* SingleDelayProcessor::createEditor()
{
    return new SingleDelayEditor(*this);
}

//==============================================================================
void SingleDelayProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto xml = valueTreeState.copyState().createXml();
    copyXmlToBinary(*xml, destData);
}

void SingleDelayProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml != nullptr && xml->hasTagName(valueTreeState.state.getType()))
        valueTreeState.replaceState(juce::ValueTree::fromXml(*xml));
}

