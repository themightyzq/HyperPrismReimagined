//==============================================================================
// HyperPrism Revived - Delay Processor Implementation
//==============================================================================

#include "DelayProcessor.h"
#include "DelayEditor.h"

// Parameter IDs
const juce::String DelayProcessor::BYPASS_ID = "bypass";
const juce::String DelayProcessor::MIX_ID = "mix";
const juce::String DelayProcessor::DELAY_TIME_ID = "delayTime";
const juce::String DelayProcessor::FEEDBACK_ID = "feedback";
const juce::String DelayProcessor::LOW_CUT_ID = "lowCut";
const juce::String DelayProcessor::HIGH_CUT_ID = "highCut";
const juce::String DelayProcessor::TEMPO_SYNC_ID = "tempoSync";
const juce::String DelayProcessor::STEREO_OFFSET_ID = "stereoOffset";

DelayProcessor::DelayProcessor()
     : AudioProcessor(BusesProperties()
                      .withInput("Input", juce::AudioChannelSet::stereo(), true)
                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
       valueTreeState(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache parameter pointers for efficient access
    bypassParam = valueTreeState.getRawParameterValue(BYPASS_ID);
    mixParam = valueTreeState.getRawParameterValue(MIX_ID);
    delayTimeParam = valueTreeState.getRawParameterValue(DELAY_TIME_ID);
    feedbackParam = valueTreeState.getRawParameterValue(FEEDBACK_ID);
    lowCutParam = valueTreeState.getRawParameterValue(LOW_CUT_ID);
    highCutParam = valueTreeState.getRawParameterValue(HIGH_CUT_ID);
    tempoSyncParam = valueTreeState.getRawParameterValue(TEMPO_SYNC_ID);
    stereoOffsetParam = valueTreeState.getRawParameterValue(STEREO_OFFSET_ID);
}

juce::AudioProcessorValueTreeState::ParameterLayout DelayProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;
    
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID, "Bypass", false));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        MIX_ID, "Mix", 0.0f, 1.0f, 0.5f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DELAY_TIME_ID, "Delay Time", 
        juce::NormalisableRange<float>(1.0f, 2000.0f, 0.1f, 0.3f), 125.0f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        FEEDBACK_ID, "Feedback", 0.0f, 0.95f, 0.3f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        LOW_CUT_ID, "Low Cut", 
        juce::NormalisableRange<float>(20.0f, 2000.0f, 1.0f, 0.3f), 20.0f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        HIGH_CUT_ID, "High Cut", 
        juce::NormalisableRange<float>(200.0f, 20000.0f, 1.0f, 0.3f), 20000.0f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        TEMPO_SYNC_ID, "Tempo Sync", false));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        STEREO_OFFSET_ID, "Stereo Offset", 
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), 0.0f));
    
    return { parameters.begin(), parameters.end() };
}

void DelayProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    // Prepare delay lines (max 4 seconds)
    int maxDelayInSamples = static_cast<int>(sampleRate * 4.0);
    leftDelay.prepare(sampleRate, maxDelayInSamples);
    rightDelay.prepare(sampleRate, maxDelayInSamples);
    
    // Prepare filters
    leftLowCut.reset();
    rightLowCut.reset();
    leftHighCut.reset();
    rightHighCut.reset();
    
    // Reset filter state
    previousFilterFreq = -1.0f;
}

void DelayProcessor::releaseResources()
{
    leftDelay.reset();
    rightDelay.reset();
}

bool DelayProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
        
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void DelayProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
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
    
    // Process delay effect
    processDelay(buffer);
}

void DelayProcessor::processDelay(juce::AudioBuffer<float>& buffer)
{
    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();
    
    if (numChannels < 2)
        return;
    
    // Get parameter values
    float mix = mixParam->load();
    float delayTimeMs = delayTimeParam->load();
    float feedback = feedbackParam->load();
    float stereoOffsetMs = stereoOffsetParam->load();
    
    // Update filters if needed
    updateFilters();
    
    // Convert delay times to samples
    float leftDelayInSamples = (delayTimeMs / 1000.0f) * static_cast<float>(currentSampleRate);
    float rightDelayInSamples = leftDelayInSamples + ((stereoOffsetMs / 1000.0f) * static_cast<float>(currentSampleRate));
    
    leftDelay.setDelay(leftDelayInSamples);
    rightDelay.setDelay(rightDelayInSamples);
    
    // Get audio data
    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getWritePointer(1);
    
    // Process each sample
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Process left channel
        float leftInput = leftChannel[sample];
        float leftDelayed = leftDelay.processSample(leftInput, feedback);
        
        // Apply filtering
        leftDelayed = leftLowCut.processSingleSampleRaw(leftDelayed);
        leftDelayed = leftHighCut.processSingleSampleRaw(leftDelayed);
        
        leftChannel[sample] = leftInput + (mix * (leftDelayed - leftInput));
        
        // Process right channel
        float rightInput = rightChannel[sample];
        float rightDelayed = rightDelay.processSample(rightInput, feedback);
        
        // Apply filtering
        rightDelayed = rightLowCut.processSingleSampleRaw(rightDelayed);
        rightDelayed = rightHighCut.processSingleSampleRaw(rightDelayed);
        
        rightChannel[sample] = rightInput + (mix * (rightDelayed - rightInput));
    }
}

void DelayProcessor::updateFilters()
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

juce::AudioProcessorEditor* DelayProcessor::createEditor()
{
    return new DelayEditor(*this);
}

void DelayProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = valueTreeState.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void DelayProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(valueTreeState.state.getType()))
            valueTreeState.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// DelayLine implementation
void DelayProcessor::DelayLine::prepare(double, int maxDelayInSamples)
{
    maxDelay = maxDelayInSamples;
    buffer.setSize(1, maxDelayInSamples + 1);
    reset();
}

void DelayProcessor::DelayLine::reset()
{
    buffer.clear();
    writeIndex = 0;
}

void DelayProcessor::DelayLine::setDelay(float delaySamples)
{
    this->delayInSamples = juce::jlimit(0.0f, static_cast<float>(maxDelay), delaySamples);
}

float DelayProcessor::DelayLine::processSample(float input, float feedback)
{
    if (maxDelay == 0)
        return input;
    
    // Calculate read position with fractional delay
    float readPosition = writeIndex - delayInSamples;
    if (readPosition < 0.0f)
        readPosition += maxDelay;
    
    // Linear interpolation for fractional delay
    int readIndex1 = static_cast<int>(readPosition);
    int readIndex2 = (readIndex1 + 1) % maxDelay;
    float fraction = readPosition - readIndex1;
    
    auto* bufferData = buffer.getReadPointer(0);
    float delayedSample = bufferData[readIndex1] * (1.0f - fraction) + 
                         bufferData[readIndex2] * fraction;
    
    // Write input plus feedback to delay line
    auto* writeData = buffer.getWritePointer(0);
    writeData[writeIndex] = input + (delayedSample * feedback);
    
    // Advance write index
    writeIndex = (writeIndex + 1) % maxDelay;
    
    return delayedSample;
}