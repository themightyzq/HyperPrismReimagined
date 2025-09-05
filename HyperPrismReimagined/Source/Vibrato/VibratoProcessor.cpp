//==============================================================================
// HyperPrism Revived - Vibrato Processor Implementation
//==============================================================================

#include "VibratoProcessor.h"
#include "VibratoEditor.h"

// Parameter IDs
const juce::String VibratoProcessor::BYPASS_ID = "bypass";
const juce::String VibratoProcessor::MIX_ID = "mix";
const juce::String VibratoProcessor::RATE_ID = "rate";
const juce::String VibratoProcessor::DEPTH_ID = "depth";
const juce::String VibratoProcessor::DELAY_ID = "delay";
const juce::String VibratoProcessor::FEEDBACK_ID = "feedback";

VibratoProcessor::VibratoProcessor()
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
    delayParam = valueTreeState.getRawParameterValue(DELAY_ID);
    feedbackParam = valueTreeState.getRawParameterValue(FEEDBACK_ID);
}

juce::AudioProcessorValueTreeState::ParameterLayout VibratoProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;
    
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID, "Bypass", false));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        MIX_ID, "Mix", 
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        RATE_ID, "Rate", 
        juce::NormalisableRange<float>(0.1f, 10.0f, 0.1f, 0.5f), 5.0f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DEPTH_ID, "Depth", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 30.0f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DELAY_ID, "Delay", 
        juce::NormalisableRange<float>(0.0f, 50.0f, 0.1f), 5.0f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        FEEDBACK_ID, "Feedback", 
        juce::NormalisableRange<float>(-95.0f, 95.0f, 0.1f), 0.0f));
    
    return { parameters.begin(), parameters.end() };
}

void VibratoProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    
    // Prepare delay lines (max 100ms for vibrato)
    leftDelayLine.prepare(sampleRate, 100.0f);
    rightDelayLine.prepare(sampleRate, 100.0f);
    
    // Initialize LFO phase
    lfoPhase = 0.0f;
}

void VibratoProcessor::releaseResources()
{
    leftDelayLine.reset();
    rightDelayLine.reset();
}

bool VibratoProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
        
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() ||
           layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void VibratoProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
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
    
    // Process vibrato effect
    processVibrato(buffer);
}

void VibratoProcessor::processVibrato(juce::AudioBuffer<float>& buffer)
{
    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();
    
    if (numChannels == 0)
        return;
    
    // Get parameter values
    float mix = mixParam->load();
    float rate = rateParam->load();
    float depth = depthParam->load() / 100.0f;  // Convert percentage to 0-1
    float baseDelayMs = delayParam->load();
    float feedback = feedbackParam->load() / 100.0f;  // Convert percentage to -0.95 to 0.95
    
    // Calculate LFO increment
    float lfoIncrement = (rate * juce::MathConstants<float>::twoPi) / static_cast<float>(currentSampleRate);
    
    // Calculate depth in milliseconds (50 cents = ~3% pitch change = ~30ms at 1kHz)
    float depthMs = depth * 3.0f;  // Scale depth to reasonable delay modulation range
    
    // Process each channel
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        
        // Get the appropriate delay line
        VibratoDelayLine* delayLine = (channel == 0) ? &leftDelayLine : &rightDelayLine;
        
        // Reset LFO phase for each channel to maintain sync
        float channelLfoPhase = lfoPhase;
        
        // Process each sample
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Calculate LFO value
            float lfoValue = std::sin(channelLfoPhase);
            
            // Calculate modulated delay time
            float modulatedDelay = baseDelayMs + (lfoValue * depthMs);
            
            // Ensure delay is positive
            modulatedDelay = juce::jmax(0.1f, modulatedDelay);
            
            // Process through delay line
            float input = channelData[sample];
            float vibratoOutput = delayLine->processSample(input, modulatedDelay, feedback);
            
            // Mix wet and dry signals
            channelData[sample] = input * (1.0f - mix) + vibratoOutput * mix;
            
            // Advance LFO phase
            channelLfoPhase += lfoIncrement;
            if (channelLfoPhase >= juce::MathConstants<float>::twoPi)
                channelLfoPhase -= juce::MathConstants<float>::twoPi;
        }
    }
    
    // Update the main LFO phase
    lfoPhase += lfoIncrement * numSamples;
    while (lfoPhase >= juce::MathConstants<float>::twoPi)
        lfoPhase -= juce::MathConstants<float>::twoPi;
}

juce::AudioProcessorEditor* VibratoProcessor::createEditor()
{
    return new VibratoEditor(*this);
}

void VibratoProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = valueTreeState.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void VibratoProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(valueTreeState.state.getType()))
            valueTreeState.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// VibratoDelayLine implementation
void VibratoProcessor::VibratoDelayLine::prepare(double sampleRate, float maxDelayMs)
{
    this->sampleRate = sampleRate;
    maxDelaySamples = static_cast<int>((maxDelayMs / 1000.0f) * sampleRate) + 1;
    buffer.setSize(1, maxDelaySamples);
    reset();
}

void VibratoProcessor::VibratoDelayLine::reset()
{
    buffer.clear();
    writeIndex = 0;
}

float VibratoProcessor::VibratoDelayLine::processSample(float input, float delayMs, float feedback)
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
    
    // Cubic interpolation for better quality pitch shifting
    int readIndex0 = static_cast<int>(readPosition - 1.0f);
    int readIndex1 = static_cast<int>(readPosition);
    int readIndex2 = static_cast<int>(readPosition + 1.0f);
    int readIndex3 = static_cast<int>(readPosition + 2.0f);
    
    // Wrap indices
    if (readIndex0 < 0) readIndex0 += maxDelaySamples;
    if (readIndex2 >= maxDelaySamples) readIndex2 -= maxDelaySamples;
    if (readIndex3 >= maxDelaySamples) readIndex3 -= maxDelaySamples;
    
    float fraction = readPosition - readIndex1;
    
    auto* bufferData = buffer.getReadPointer(0);
    
    // Cubic interpolation
    float y0 = bufferData[readIndex0];
    float y1 = bufferData[readIndex1];
    float y2 = bufferData[readIndex2];
    float y3 = bufferData[readIndex3];
    
    float a0 = -0.5f * y0 + 1.5f * y1 - 1.5f * y2 + 0.5f * y3;
    float a1 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
    float a2 = -0.5f * y0 + 0.5f * y2;
    float a3 = y1;
    
    float delayedSample = ((a0 * fraction + a1) * fraction + a2) * fraction + a3;
    
    // Write input plus feedback to delay line
    auto* writeData = buffer.getWritePointer(0);
    writeData[writeIndex] = input + (delayedSample * feedback);
    
    // Advance write index
    writeIndex = (writeIndex + 1) % maxDelaySamples;
    
    return delayedSample;
}