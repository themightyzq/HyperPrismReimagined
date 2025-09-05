//==============================================================================
// HyperPrism Revived - Chorus Processor Implementation
//==============================================================================

#include "ChorusProcessor.h"
#include "ChorusEditor.h"

// Parameter IDs
const juce::String ChorusProcessor::BYPASS_ID = "bypass";
const juce::String ChorusProcessor::MIX_ID = "mix";
const juce::String ChorusProcessor::RATE_ID = "rate";
const juce::String ChorusProcessor::DEPTH_ID = "depth";
const juce::String ChorusProcessor::FEEDBACK_ID = "feedback";
const juce::String ChorusProcessor::DELAY_ID = "delay";
const juce::String ChorusProcessor::LOW_CUT_ID = "lowCut";
const juce::String ChorusProcessor::HIGH_CUT_ID = "highCut";

ChorusProcessor::ChorusProcessor()
     : AudioProcessor(BusesProperties()
                      .withInput("Input", juce::AudioChannelSet::stereo(), true)
                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
       valueTreeState(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache parameter pointers for efficient access
    bypassParam = valueTreeState.getRawParameterValue(BYPASS_ID);
    mixParam = valueTreeState.getRawParameterValue(MIX_ID);
    rateParam = valueTreeState.getRawParameterValue(RATE_ID);
    depthParam = valueTreeState.getRawParameterValue(DEPTH_ID);
    feedbackParam = valueTreeState.getRawParameterValue(FEEDBACK_ID);
    delayParam = valueTreeState.getRawParameterValue(DELAY_ID);
    lowCutParam = valueTreeState.getRawParameterValue(LOW_CUT_ID);
    highCutParam = valueTreeState.getRawParameterValue(HIGH_CUT_ID);
}

juce::AudioProcessorValueTreeState::ParameterLayout ChorusProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;
    
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID, "Bypass", false));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        MIX_ID, "Mix", 0.0f, 1.0f, 0.5f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        RATE_ID, "Rate", 
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.1f, 0.5f), 1.0f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DEPTH_ID, "Depth", 0.0f, 1.0f, 0.5f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        FEEDBACK_ID, "Feedback", 0.0f, 0.95f, 0.2f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DELAY_ID, "Delay", 
        juce::NormalisableRange<float>(1.0f, 50.0f, 0.1f), 15.0f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        LOW_CUT_ID, "Low Cut", 
        juce::NormalisableRange<float>(20.0f, 2000.0f, 1.0f, 0.3f), 20.0f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        HIGH_CUT_ID, "High Cut", 
        juce::NormalisableRange<float>(200.0f, 20000.0f, 1.0f, 0.3f), 20000.0f));
    
    return { parameters.begin(), parameters.end() };
}

void ChorusProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    
    // Prepare delay lines (max 100ms for chorus)
    leftDelayLine.prepare(sampleRate, 100.0f);
    rightDelayLine.prepare(sampleRate, 100.0f);
    
    // Initialize LFO phases with slight offset for stereo width
    lfoPhase = 0.0f;
    lfoPhaseRight = juce::MathConstants<float>::pi * 0.25f; // 45-degree offset
    
    // Prepare filters
    leftLowCut.reset();
    rightLowCut.reset();
    leftHighCut.reset();
    rightHighCut.reset();
    
    // Reset filter state
    previousFilterFreq = -1.0f;
}

void ChorusProcessor::releaseResources()
{
    leftDelayLine.reset();
    rightDelayLine.reset();
}

bool ChorusProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
        
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void ChorusProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
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
    
    // Process chorus effect
    processChorus(buffer);
}

void ChorusProcessor::processChorus(juce::AudioBuffer<float>& buffer)
{
    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();
    
    if (numChannels < 2)
        return;
    
    // Get parameter values
    float mix = mixParam->load();
    float rate = rateParam->load();
    float depth = depthParam->load();
    float feedback = feedbackParam->load();
    float delayMs = delayParam->load();
    
    // Update filters if needed
    updateFilters();
    
    // Calculate LFO increment
    float lfoIncrement = (rate * juce::MathConstants<float>::twoPi) / static_cast<float>(currentSampleRate);
    
    // Create a copy for dry signal
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);
    
    // Get audio data
    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getWritePointer(1);
    
    // Process each sample
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Calculate LFO values
        float lfoLeft = std::sin(lfoPhase);
        float lfoRight = std::sin(lfoPhaseRight);
        
        // Apply depth and calculate modulated delay times
        float leftDelayTime = delayMs + (lfoLeft * depth * delayMs * 0.5f);
        float rightDelayTime = delayMs + (lfoRight * depth * delayMs * 0.5f);
        
        // Process left channel
        float leftInput = leftChannel[sample];
        float leftChorus = leftDelayLine.processSample(leftInput, leftDelayTime, feedback);
        
        // Apply filtering
        leftChorus = leftLowCut.processSingleSampleRaw(leftChorus);
        leftChorus = leftHighCut.processSingleSampleRaw(leftChorus);
        
        // Process right channel
        float rightInput = rightChannel[sample];
        float rightChorus = rightDelayLine.processSample(rightInput, rightDelayTime, feedback);
        
        // Apply filtering
        rightChorus = rightLowCut.processSingleSampleRaw(rightChorus);
        rightChorus = rightHighCut.processSingleSampleRaw(rightChorus);
        
        // Mix wet and dry signals
        leftChannel[sample] = leftInput + (mix * (leftChorus - leftInput));
        rightChannel[sample] = rightInput + (mix * (rightChorus - rightInput));
        
        // Advance LFO phases
        lfoPhase += lfoIncrement;
        lfoPhaseRight += lfoIncrement;
        
        // Wrap phases
        if (lfoPhase >= juce::MathConstants<float>::twoPi)
            lfoPhase -= juce::MathConstants<float>::twoPi;
        if (lfoPhaseRight >= juce::MathConstants<float>::twoPi)
            lfoPhaseRight -= juce::MathConstants<float>::twoPi;
    }
}

void ChorusProcessor::updateFilters()
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

juce::AudioProcessorEditor* ChorusProcessor::createEditor()
{
    return new ChorusEditor(*this);
}

void ChorusProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = valueTreeState.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ChorusProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(valueTreeState.state.getType()))
            valueTreeState.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// ChorusDelayLine implementation
void ChorusProcessor::ChorusDelayLine::prepare(double sampleRate, float maxDelayMs)
{
    this->sampleRate = sampleRate;
    maxDelaySamples = static_cast<int>((maxDelayMs / 1000.0f) * sampleRate) + 1;
    buffer.setSize(1, maxDelaySamples);
    reset();
}

void ChorusProcessor::ChorusDelayLine::reset()
{
    buffer.clear();
    writeIndex = 0;
}

float ChorusProcessor::ChorusDelayLine::processSample(float input, float delayMs, float feedback)
{
    if (maxDelaySamples == 0)
        return input;
    
    // Convert delay time to samples
    float delaySamples = (delayMs / 1000.0f) * static_cast<float>(sampleRate);
    delaySamples = juce::jlimit(0.0f, static_cast<float>(maxDelaySamples - 1), delaySamples);
    
    // Calculate read position with fractional delay
    float readPosition = writeIndex - delaySamples;
    if (readPosition < 0.0f)
        readPosition += maxDelaySamples;
    
    // Linear interpolation for fractional delay
    int readIndex1 = static_cast<int>(readPosition);
    int readIndex2 = (readIndex1 + 1) % maxDelaySamples;
    float fraction = readPosition - readIndex1;
    
    auto* bufferData = buffer.getReadPointer(0);
    float delayedSample = bufferData[readIndex1] * (1.0f - fraction) + 
                         bufferData[readIndex2] * fraction;
    
    // Write input plus feedback to delay line
    auto* writeData = buffer.getWritePointer(0);
    writeData[writeIndex] = input + (delayedSample * feedback);
    
    // Advance write index
    writeIndex = (writeIndex + 1) % maxDelaySamples;
    
    return delayedSample;
}