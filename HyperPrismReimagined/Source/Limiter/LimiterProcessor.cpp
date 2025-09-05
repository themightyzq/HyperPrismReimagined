#include "LimiterProcessor.h"
#include "LimiterEditor.h"

static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "ceiling", "Ceiling", 
        juce::NormalisableRange<float>(-30.0f, 0.0f, 0.1f), 
        -0.3f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "release", "Release", 
        juce::NormalisableRange<float>(1.0f, 1000.0f, 0.1f, 0.5f), 
        50.0f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "lookahead", "Lookahead", 
        juce::NormalisableRange<float>(0.0f, 20.0f, 0.1f), 
        5.0f));
    
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "softclip", "Soft Clip", false));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "inputgain", "Input Gain", 
        juce::NormalisableRange<float>(-20.0f, 20.0f, 0.1f), 
        0.0f));
    
    return layout;
}

LimiterProcessor::LimiterProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "LimiterState", createParameterLayout())
{
    // Get parameter pointers
    ceilingParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("ceiling"));
    releaseParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("release"));
    lookaheadParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("lookahead"));
    softClipParam = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("softclip"));
    inputGainParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("inputgain"));
}

LimiterProcessor::~LimiterProcessor()
{
}

const juce::String LimiterProcessor::getName() const
{
    return "HyperPrism Reimagined Limiter";
}

bool LimiterProcessor::acceptsMidi() const
{
    return false;
}

bool LimiterProcessor::producesMidi() const
{
    return false;
}

bool LimiterProcessor::isMidiEffect() const
{
    return false;
}

double LimiterProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int LimiterProcessor::getNumPrograms()
{
    return 1;
}

int LimiterProcessor::getCurrentProgram()
{
    return 0;
}

void LimiterProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String LimiterProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void LimiterProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void LimiterProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    // Calculate maximum lookahead samples needed
    int maxLookaheadSamples = static_cast<int>(std::ceil(20.0 * sampleRate / 1000.0));
    
    // Prepare lookahead buffer
    lookaheadBuffer.setSize(2, maxLookaheadSamples + samplesPerBlock);
    lookaheadBuffer.clear();
    lookaheadWritePos = 0;
    
    // Initialize envelope followers and smoothed gains
    envelopeFollowers.resize(2, 0.0f);
    smoothedGains.resize(2, 1.0f);
}

void LimiterProcessor::releaseResources()
{
    lookaheadBuffer.clear();
}

bool LimiterProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

float LimiterProcessor::softClip(float input)
{
    // Soft clipping using tanh
    return std::tanh(input * 0.7f) / 0.7f;
}

float LimiterProcessor::processLimiting(float input, float ceiling, float& envelope, 
                                       float& smoothedGain, float release)
{
    // Peak detection
    float inputAbs = std::abs(input);
    
    // Attack is instant for limiting
    if (inputAbs > envelope)
    {
        envelope = inputAbs;
    }
    else
    {
        // Release
        float releaseCoeff = std::exp(-1000.0f / (release * currentSampleRate));
        envelope = inputAbs + releaseCoeff * (envelope - inputAbs);
    }
    
    // Calculate gain reduction
    float targetGain = 1.0f;
    if (envelope > ceiling)
    {
        targetGain = ceiling / envelope;
    }
    
    // Smooth gain changes to prevent clicks
    float attackTime = 0.1f; // 0.1ms attack for limiting
    float attackCoeff = std::exp(-1000.0f / (attackTime * currentSampleRate));
    float releaseCoeff = std::exp(-1000.0f / (release * currentSampleRate));
    
    if (targetGain < smoothedGain)
        smoothedGain = targetGain + attackCoeff * (smoothedGain - targetGain);
    else
        smoothedGain = targetGain + releaseCoeff * (smoothedGain - targetGain);
    
    return smoothedGain;
}

void LimiterProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Get parameter values
    float ceilingDB = ceilingParam->get();
    float ceilingLinear = juce::Decibels::decibelsToGain(ceilingDB);
    float releaseTime = releaseParam->get();
    float lookaheadMs = lookaheadParam->get();
    bool useSoftClip = softClipParam->get();
    float inputGainDB = inputGainParam->get();
    float inputGainLinear = juce::Decibels::decibelsToGain(inputGainDB);
    
    // Calculate lookahead samples
    lookaheadSamples = static_cast<int>(lookaheadMs * currentSampleRate / 1000.0);
    
    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    
    float maxGainReduction = 1.0f;
    bool hitCeiling = false;
    
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Apply input gain
            float input = channelData[sample] * inputGainLinear;
            
            // Simplified limiting without expensive lookahead loop
            // Use immediate input instead of complex lookahead processing
            float inputAbs = std::abs(input);
            
            // Fast envelope follower
            float& envelope = envelopeFollowers[channel];
            if (inputAbs > envelope)
                envelope = inputAbs; // Instant attack
            else
                envelope = inputAbs + 0.999f * (envelope - inputAbs); // Fast release
            
            // Calculate gain reduction
            float& smoothedGain = smoothedGains[channel];
            float targetGain = (envelope > ceilingLinear) ? ceilingLinear / envelope : 1.0f;
            
            // Simple gain smoothing
            if (targetGain < smoothedGain)
                smoothedGain = targetGain + 0.01f * (smoothedGain - targetGain); // Fast attack
            else
                smoothedGain = targetGain + 0.999f * (smoothedGain - targetGain); // Slow release
            
            // Apply limiting to original input (no delay for performance)
            float output = input * smoothedGain;
            
            // Apply soft clipping if enabled
            if (useSoftClip && std::abs(output) > ceilingLinear)
            {
                output = softClip(output / ceilingLinear) * ceilingLinear;
            }
            
            // Hard clip as final safety
            output = std::max(-ceilingLinear, std::min(ceilingLinear, output));
            
            channelData[sample] = output;
            
            // Update metering
            maxGainReduction = std::min(maxGainReduction, smoothedGain);
            if (std::abs(output) >= ceilingLinear * 0.99f)
                hitCeiling = true;
        }
    }
    
    // Update metering values
    currentGainReduction.store(1.0f - maxGainReduction);
    if (hitCeiling)
        peakIndicator.store(true);
}

bool LimiterProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* LimiterProcessor::createEditor()
{
    return new LimiterEditor(*this);
}

void LimiterProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void LimiterProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}