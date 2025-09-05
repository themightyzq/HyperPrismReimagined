//==============================================================================
// HyperPrism Revived - Flanger Processor Implementation
//==============================================================================

#include "FlangerProcessor.h"
#include "FlangerEditor.h"

// Parameter IDs
const juce::String FlangerProcessor::BYPASS_ID = "bypass";
const juce::String FlangerProcessor::MIX_ID = "mix";
const juce::String FlangerProcessor::RATE_ID = "rate";
const juce::String FlangerProcessor::DEPTH_ID = "depth";
const juce::String FlangerProcessor::FEEDBACK_ID = "feedback";
const juce::String FlangerProcessor::DELAY_ID = "delay";
const juce::String FlangerProcessor::PHASE_ID = "phase";
const juce::String FlangerProcessor::LOW_CUT_ID = "lowCut";
const juce::String FlangerProcessor::HIGH_CUT_ID = "highCut";

FlangerProcessor::FlangerProcessor()
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
    phaseParam = valueTreeState.getRawParameterValue(PHASE_ID);
    lowCutParam = valueTreeState.getRawParameterValue(LOW_CUT_ID);
    highCutParam = valueTreeState.getRawParameterValue(HIGH_CUT_ID);
}

juce::AudioProcessorValueTreeState::ParameterLayout FlangerProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;
    
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID, "Bypass", false));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        MIX_ID, "Mix", 0.0f, 1.0f, 0.5f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        RATE_ID, "Rate", 
        juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.5f), 0.5f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DEPTH_ID, "Depth", 0.0f, 1.0f, 0.7f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        FEEDBACK_ID, "Feedback", -0.99f, 0.99f, 0.5f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DELAY_ID, "Delay", 
        juce::NormalisableRange<float>(0.5f, 20.0f, 0.1f), 5.0f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        PHASE_ID, "Phase", 0.0f, 180.0f, 90.0f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        LOW_CUT_ID, "Low Cut", 
        juce::NormalisableRange<float>(20.0f, 2000.0f, 1.0f, 0.3f), 20.0f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        HIGH_CUT_ID, "High Cut", 
        juce::NormalisableRange<float>(200.0f, 20000.0f, 1.0f, 0.3f), 20000.0f));
    
    return { parameters.begin(), parameters.end() };
}

void FlangerProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    
    // Prepare delay lines (max 50ms for flanger - shorter than chorus)
    leftDelayLine.prepare(sampleRate, 50.0f);
    rightDelayLine.prepare(sampleRate, 50.0f);
    
    // Initialize LFO phases
    lfoPhase = 0.0f;
    lfoPhaseRight = 0.0f;
    
    // Prepare filters
    leftLowCut.reset();
    rightLowCut.reset();
    leftHighCut.reset();
    rightHighCut.reset();
    
    // Reset filter state
    previousFilterFreq = -1.0f;
}

void FlangerProcessor::releaseResources()
{
    leftDelayLine.reset();
    rightDelayLine.reset();
}

bool FlangerProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
        
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void FlangerProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
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
    
    // Process flanger effect
    processFlanger(buffer);
}

void FlangerProcessor::processFlanger(juce::AudioBuffer<float>& buffer)
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
    float phaseOffset = phaseParam->load();
    
    // Update filters if needed
    updateFilters();
    
    // Calculate LFO increment
    float lfoIncrement = (rate * juce::MathConstants<float>::twoPi) / static_cast<float>(currentSampleRate);
    
    // Convert phase offset to radians
    float phaseOffsetRad = (phaseOffset / 180.0f) * juce::MathConstants<float>::pi;
    
    // Create a copy for dry signal
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);
    
    // Get audio data
    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getWritePointer(1);
    
    // Process each sample
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Calculate LFO values with phase offset
        float lfoLeft = std::sin(lfoPhase);
        float lfoRight = std::sin(lfoPhaseRight);
        
        // Apply depth and calculate modulated delay times
        float leftDelayTime = delayMs + (lfoLeft * depth * delayMs);
        float rightDelayTime = delayMs + (lfoRight * depth * delayMs);
        
        // Ensure positive delay times
        leftDelayTime = juce::jmax(0.1f, leftDelayTime);
        rightDelayTime = juce::jmax(0.1f, rightDelayTime);
        
        // Process left channel
        float leftInput = leftChannel[sample];
        float leftFlanger = leftDelayLine.processSample(leftInput, leftDelayTime, feedback);
        
        // Apply filtering
        leftFlanger = leftLowCut.processSingleSampleRaw(leftFlanger);
        leftFlanger = leftHighCut.processSingleSampleRaw(leftFlanger);
        
        // Process right channel
        float rightInput = rightChannel[sample];
        float rightFlanger = rightDelayLine.processSample(rightInput, rightDelayTime, feedback);
        
        // Apply filtering
        rightFlanger = rightLowCut.processSingleSampleRaw(rightFlanger);
        rightFlanger = rightHighCut.processSingleSampleRaw(rightFlanger);
        
        // Mix wet and dry signals
        leftChannel[sample] = leftInput + (mix * (leftFlanger - leftInput));
        rightChannel[sample] = rightInput + (mix * (rightFlanger - rightInput));
        
        // Advance LFO phases
        lfoPhase += lfoIncrement;
        lfoPhaseRight = lfoPhase + phaseOffsetRad;
        
        // Wrap phases
        if (lfoPhase >= juce::MathConstants<float>::twoPi)
            lfoPhase -= juce::MathConstants<float>::twoPi;
        if (lfoPhaseRight >= juce::MathConstants<float>::twoPi)
            lfoPhaseRight -= juce::MathConstants<float>::twoPi;
        if (lfoPhaseRight < 0.0f)
            lfoPhaseRight += juce::MathConstants<float>::twoPi;
    }
}

void FlangerProcessor::updateFilters()
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

juce::AudioProcessorEditor* FlangerProcessor::createEditor()
{
    return new FlangerEditor(*this);
}

void FlangerProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = valueTreeState.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void FlangerProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(valueTreeState.state.getType()))
            valueTreeState.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// FlangerDelayLine implementation
void FlangerProcessor::FlangerDelayLine::prepare(double rate, float maxDelayMs)
{
    sampleRate = rate;
    maxDelaySamples = static_cast<int>((maxDelayMs / 1000.0f) * sampleRate) + 1;
    buffer.setSize(1, maxDelaySamples);
    reset();
}

void FlangerProcessor::FlangerDelayLine::reset()
{
    buffer.clear();
    writeIndex = 0;
}

float FlangerProcessor::FlangerDelayLine::processSample(float input, float delayMs, float feedback)
{
    if (maxDelaySamples == 0)
        return input;
    
    // Convert delay time to samples
    float delaySamples = (delayMs / 1000.0f) * static_cast<float>(sampleRate);
    delaySamples = juce::jlimit(0.1f, static_cast<float>(maxDelaySamples - 1), delaySamples);
    
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