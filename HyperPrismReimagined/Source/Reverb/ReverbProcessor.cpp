//==============================================================================
// HyperPrism Revived - Reverb Processor Implementation
//==============================================================================

#include "ReverbProcessor.h"
#include "ReverbEditor.h"

// Parameter IDs
const juce::String ReverbProcessor::BYPASS_ID = "bypass";
const juce::String ReverbProcessor::MIX_ID = "mix";
const juce::String ReverbProcessor::ROOM_SIZE_ID = "roomSize";
const juce::String ReverbProcessor::DAMPING_ID = "damping";
const juce::String ReverbProcessor::PRE_DELAY_ID = "preDelay";
const juce::String ReverbProcessor::WIDTH_ID = "width";
const juce::String ReverbProcessor::LOW_CUT_ID = "lowCut";
const juce::String ReverbProcessor::HIGH_CUT_ID = "highCut";

ReverbProcessor::ReverbProcessor()
     : AudioProcessor(BusesProperties()
                      .withInput("Input", juce::AudioChannelSet::stereo(), true)
                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
       valueTreeState(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache parameter pointers for efficient access
    bypassParam = valueTreeState.getRawParameterValue(BYPASS_ID);
    mixParam = valueTreeState.getRawParameterValue(MIX_ID);
    roomSizeParam = valueTreeState.getRawParameterValue(ROOM_SIZE_ID);
    dampingParam = valueTreeState.getRawParameterValue(DAMPING_ID);
    preDelayParam = valueTreeState.getRawParameterValue(PRE_DELAY_ID);
    widthParam = valueTreeState.getRawParameterValue(WIDTH_ID);
    lowCutParam = valueTreeState.getRawParameterValue(LOW_CUT_ID);
    highCutParam = valueTreeState.getRawParameterValue(HIGH_CUT_ID);
}

juce::AudioProcessorValueTreeState::ParameterLayout ReverbProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;
    
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID, "Bypass", false));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        MIX_ID, "Mix", 0.0f, 1.0f, 0.3f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        ROOM_SIZE_ID, "Room Size", 0.1f, 1.0f, 0.5f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DAMPING_ID, "Damping", 0.0f, 1.0f, 0.5f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        PRE_DELAY_ID, "Pre Delay", 
        juce::NormalisableRange<float>(0.0f, 200.0f, 1.0f), 20.0f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        WIDTH_ID, "Width", 0.0f, 1.0f, 1.0f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        LOW_CUT_ID, "Low Cut", 
        juce::NormalisableRange<float>(20.0f, 2000.0f, 1.0f, 0.3f), 20.0f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        HIGH_CUT_ID, "High Cut", 
        juce::NormalisableRange<float>(200.0f, 20000.0f, 1.0f, 0.3f), 20000.0f));
    
    return { parameters.begin(), parameters.end() };
}

void ReverbProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    
    // Prepare reverb
    juce::Reverb::Parameters reverbParams;
    reverbParams.roomSize = 0.5f;
    reverbParams.damping = 0.5f;
    reverbParams.wetLevel = 1.0f;
    reverbParams.dryLevel = 0.0f;
    reverbParams.width = 1.0f;
    reverb.setParameters(reverbParams);
    reverb.setSampleRate(sampleRate);
    
    // Prepare pre-delay (max 500ms)
    maxPreDelayInSamples = static_cast<int>(sampleRate * 0.5);
    preDelayBuffer.setSize(2, maxPreDelayInSamples);
    preDelayBuffer.clear();
    preDelayWriteIndex = 0;
    
    // Prepare filters
    leftLowCut.reset();
    rightLowCut.reset();
    leftHighCut.reset();
    rightHighCut.reset();
    
    // Reset filter state
    previousFilterFreq = -1.0f;
}

void ReverbProcessor::releaseResources()
{
    reverb.reset();
    preDelayBuffer.clear();
}

bool ReverbProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
        
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void ReverbProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    // Clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
    
    // Bypass check
    if (bypassParam->load() > 0.5f)
        return;
    
    // Process reverb effect
    processReverb(buffer);
}

void ReverbProcessor::processReverb(juce::AudioBuffer<float>& buffer)
{
    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();
    
    if (numChannels < 2)
        return;
    
    // Get parameter values
    float mix = mixParam->load();
    float roomSize = roomSizeParam->load();
    float damping = dampingParam->load();
    float preDelayMs = preDelayParam->load();
    float width = widthParam->load();
    
    // Update reverb parameters
    juce::Reverb::Parameters reverbParams;
    reverbParams.roomSize = roomSize;
    reverbParams.damping = damping;
    reverbParams.wetLevel = 1.0f;
    reverbParams.dryLevel = 0.0f;
    reverbParams.width = width;
    reverb.setParameters(reverbParams);
    
    // Update filters if needed
    updateFilters();
    
    // Calculate pre-delay in samples
    int preDelayInSamples = static_cast<int>((preDelayMs / 1000.0f) * currentSampleRate);
    preDelayInSamples = juce::jlimit(0, maxPreDelayInSamples - 1, preDelayInSamples);
    
    // Create a copy for dry signal
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);
    
    // Apply pre-delay
    if (preDelayInSamples > 0)
    {
        auto* leftChannel = buffer.getWritePointer(0);
        auto* rightChannel = buffer.getWritePointer(1);
        auto* preDelayLeft = preDelayBuffer.getWritePointer(0);
        auto* preDelayRight = preDelayBuffer.getWritePointer(1);
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Read delayed samples
            int readIndex = (preDelayWriteIndex - preDelayInSamples + maxPreDelayInSamples) % maxPreDelayInSamples;
            float delayedLeft = preDelayLeft[readIndex];
            float delayedRight = preDelayRight[readIndex];
            
            // Write current samples to delay buffer
            preDelayLeft[preDelayWriteIndex] = leftChannel[sample];
            preDelayRight[preDelayWriteIndex] = rightChannel[sample];
            
            // Replace current samples with delayed ones
            leftChannel[sample] = delayedLeft;
            rightChannel[sample] = delayedRight;
            
            // Advance write index
            preDelayWriteIndex = (preDelayWriteIndex + 1) % maxPreDelayInSamples;
        }
    }
    
    // Process reverb
    if (numChannels == 2)
    {
        reverb.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), numSamples);
    }
    else
    {
        reverb.processMono(buffer.getWritePointer(0), numSamples);
    }
    
    // Apply filtering to wet signal
    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getWritePointer(1);
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        leftChannel[sample] = leftLowCut.processSingleSampleRaw(leftChannel[sample]);
        leftChannel[sample] = leftHighCut.processSingleSampleRaw(leftChannel[sample]);
        
        rightChannel[sample] = rightLowCut.processSingleSampleRaw(rightChannel[sample]);
        rightChannel[sample] = rightHighCut.processSingleSampleRaw(rightChannel[sample]);
    }
    
    // Mix wet and dry signals
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        auto* dryData = dryBuffer.getReadPointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            channelData[sample] = dryData[sample] + (mix * (channelData[sample] - dryData[sample]));
        }
    }
}

void ReverbProcessor::updateFilters()
{
    float lowCutFreq = lowCutParam->load();
    float highCutFreq = highCutParam->load();
    
    // Only update if frequencies changed
    if (std::abs(lowCutFreq - previousFilterFreq) > 0.1f || 
        std::abs(highCutFreq - previousFilterFreq) > 0.1f)
    {
        // High-pass filter (low cut)
        leftLowCut.setCoefficients(juce::IIRCoefficients::makeHighPass(currentSampleRate, lowCutFreq, 0.707f));
        rightLowCut.setCoefficients(juce::IIRCoefficients::makeHighPass(currentSampleRate, lowCutFreq, 0.707f));
        
        // Low-pass filter (high cut)
        leftHighCut.setCoefficients(juce::IIRCoefficients::makeLowPass(currentSampleRate, highCutFreq, 0.707f));
        rightHighCut.setCoefficients(juce::IIRCoefficients::makeLowPass(currentSampleRate, highCutFreq, 0.707f));
        
        previousFilterFreq = lowCutFreq; // Track one of them
    }
}

juce::AudioProcessorEditor* ReverbProcessor::createEditor()
{
    return new ReverbEditor(*this);
}

void ReverbProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = valueTreeState.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ReverbProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(valueTreeState.state.getType()))
            valueTreeState.replaceState(juce::ValueTree::fromXml(*xmlState));
}