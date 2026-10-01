//==============================================================================
// HyperPrism Reimagined - Delay Processor Implementation
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

void DelayProcessor::prepareToPlay(double sampleRate, int)
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
    
    // Smoothers start at the current parameter values
    mixSmoothed.reset (sampleRate, 0.03);          mixSmoothed.setCurrentAndTargetValue (mixParam->load());
    delayTimeSmoothed.reset (sampleRate, 0.03);    delayTimeSmoothed.setCurrentAndTargetValue (delayTimeParam->load());
    feedbackSmoothed.reset (sampleRate, 0.03);     feedbackSmoothed.setCurrentAndTargetValue (feedbackParam->load());
    stereoOffsetSmoothed.reset (sampleRate, 0.03); stereoOffsetSmoothed.setCurrentAndTargetValue (stereoOffsetParam->load());
    lowCutSmoothed.reset (sampleRate, 0.03);       lowCutSmoothed.setCurrentAndTargetValue (lowCutParam->load());
    highCutSmoothed.reset (sampleRate, 0.03);      highCutSmoothed.setCurrentAndTargetValue (highCutParam->load());

    // Force the filter coefficients to be computed on the first block
    previousLowCutFreq = -1.0f;
    previousHighCutFreq = -1.0f;
    filterUpdateCounter = 0;
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
    
    mixSmoothed.setTargetValue(mixParam->load());
    delayTimeSmoothed.setTargetValue(delayTimeParam->load());
    feedbackSmoothed.setTargetValue(feedbackParam->load());
    stereoOffsetSmoothed.setTargetValue(stereoOffsetParam->load());
    lowCutSmoothed.setTargetValue(lowCutParam->load());
    highCutSmoothed.setTargetValue(highCutParam->load());
    
    // Update filters if needed
    updateFilters(lowCutSmoothed.getCurrentValue(), highCutSmoothed.getCurrentValue());
    
    const float samplesPerMs = static_cast<float>(currentSampleRate) / 1000.0f;
    
    // Get audio data
    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getWritePointer(1);
    
    // Process each sample
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float mix = mixSmoothed.getNextValue();
        const float delayTimeMs = delayTimeSmoothed.getNextValue();
        const float feedback = feedbackSmoothed.getNextValue();
        const float stereoOffsetMs = stereoOffsetSmoothed.getNextValue();
        const float lowCutFreq = lowCutSmoothed.getNextValue();
        const float highCutFreq = highCutSmoothed.getNextValue();
        
        // Re-derive the filter coefficients every 16 samples while a cutoff is moving
        if (++filterUpdateCounter >= 16)
        {
            filterUpdateCounter = 0;
            updateFilters(lowCutFreq, highCutFreq);
        }
        
        // Smoothed (fractional) delay times in samples
        const float leftDelayInSamples = delayTimeMs * samplesPerMs;
        const float rightDelayInSamples = leftDelayInSamples + stereoOffsetMs * samplesPerMs;
        leftDelay.setDelay(leftDelayInSamples);
        rightDelay.setDelay(rightDelayInSamples);
        
        // Process left channel
        float leftInput = leftChannel[static_cast<size_t>(sample)];
        float leftDelayed = leftDelay.processSample(leftInput, feedback);
        
        // Apply filtering
        leftDelayed = leftLowCut.processSingleSampleRaw(leftDelayed);
        leftDelayed = leftHighCut.processSingleSampleRaw(leftDelayed);
        
        leftChannel[static_cast<size_t>(sample)] = leftInput + (mix * (leftDelayed - leftInput));
        
        // Process right channel
        float rightInput = rightChannel[static_cast<size_t>(sample)];
        float rightDelayed = rightDelay.processSample(rightInput, feedback);
        
        // Apply filtering
        rightDelayed = rightLowCut.processSingleSampleRaw(rightDelayed);
        rightDelayed = rightHighCut.processSingleSampleRaw(rightDelayed);
        
        rightChannel[static_cast<size_t>(sample)] = rightInput + (mix * (rightDelayed - rightInput));
    }
    
    // Make sure the filters end the block exactly on the current cutoffs
    updateFilters(lowCutSmoothed.getCurrentValue(), highCutSmoothed.getCurrentValue());
}

void DelayProcessor::updateFilters(float lowCutFreq, float highCutFreq)
{
    // Only update if frequencies changed
    if (lowCutFreq != previousLowCutFreq || highCutFreq != previousHighCutFreq)
    {
        // High-pass filter (low cut)
        leftLowCut.setCoefficients(juce::IIRCoefficients::makeHighPass(currentSampleRate, lowCutFreq, 0.707f));
        rightLowCut.setCoefficients(juce::IIRCoefficients::makeHighPass(currentSampleRate, lowCutFreq, 0.707f));
        
        // Low-pass filter (high cut)
        leftHighCut.setCoefficients(juce::IIRCoefficients::makeLowPass(currentSampleRate, highCutFreq, 0.707f));
        rightHighCut.setCoefficients(juce::IIRCoefficients::makeLowPass(currentSampleRate, highCutFreq, 0.707f));
        
        previousLowCutFreq = lowCutFreq;
        previousHighCutFreq = highCutFreq;
    }
}

juce::AudioProcessorEditor* DelayProcessor::createEditor()
{
    return new DelayEditor(*this);
}

void DelayProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = valueTreeState.copyState();
    state.setProperty("editor_width", getEditorWidth(), nullptr);
    state.setProperty("editor_height", getEditorHeight(), nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void DelayProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(valueTreeState.state.getType()))
        {
            auto newState = juce::ValueTree::fromXml(*xmlState);
            setEditorSize((int) newState.getProperty("editor_width", 0), (int) newState.getProperty("editor_height", 0));
            newState.removeProperty("editor_width", nullptr);
            newState.removeProperty("editor_height", nullptr);
            valueTreeState.replaceState(newState);
        }
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