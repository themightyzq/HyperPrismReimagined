//==============================================================================
// HyperPrism Revived - Phaser Processor Implementation
//==============================================================================

#include "PhaserProcessor.h"
#include "PhaserEditor.h"

// Parameter IDs
const juce::String PhaserProcessor::BYPASS_ID = "bypass";
const juce::String PhaserProcessor::RATE_ID = "rate";
const juce::String PhaserProcessor::DEPTH_ID = "depth";
const juce::String PhaserProcessor::FEEDBACK_ID = "feedback";
const juce::String PhaserProcessor::STAGES_ID = "stages";
const juce::String PhaserProcessor::MIX_ID = "mix";

//==============================================================================
PhaserProcessor::PhaserProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
    valueTreeState(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

PhaserProcessor::~PhaserProcessor()
{
}

//==============================================================================
const juce::String PhaserProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PhaserProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PhaserProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PhaserProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PhaserProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PhaserProcessor::getNumPrograms()
{
    return 1;
}

int PhaserProcessor::getCurrentProgram()
{
    return 0;
}

void PhaserProcessor::setCurrentProgram(int)
{
}

const juce::String PhaserProcessor::getProgramName(int)
{
    return "Default";
}

void PhaserProcessor::changeProgramName(int, const juce::String&)
{
}

//==============================================================================
void PhaserProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    // Prepare all-pass filters
    for (auto& filter : allPassFiltersL)
        filter.prepare(sampleRate);
    for (auto& filter : allPassFiltersR)
        filter.prepare(sampleRate);
    
    // Initialize smoothed values
    const float smoothTime = 0.005f; // 5ms smoothing for real-time response
    rateSmoothed.reset(sampleRate, smoothTime);
    depthSmoothed.reset(sampleRate, smoothTime);
    feedbackSmoothed.reset(sampleRate, smoothTime);
    mixSmoothed.reset(sampleRate, smoothTime);
    
    // Set initial values
    rateSmoothed.setCurrentAndTargetValue(*valueTreeState.getRawParameterValue(RATE_ID));
    depthSmoothed.setCurrentAndTargetValue(*valueTreeState.getRawParameterValue(DEPTH_ID));
    feedbackSmoothed.setCurrentAndTargetValue(*valueTreeState.getRawParameterValue(FEEDBACK_ID));
    mixSmoothed.setCurrentAndTargetValue(*valueTreeState.getRawParameterValue(MIX_ID));
    
    lfoPhase = 0.0f;
}

void PhaserProcessor::releaseResources()
{
    for (auto& filter : allPassFiltersL)
        filter.reset();
    for (auto& filter : allPassFiltersR)
        filter.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PhaserProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    #if ! JucePlugin_IsSynth
     if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
         return false;
    #endif

    return true;
  #endif
}
#endif

void PhaserProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Check bypass
    if (*valueTreeState.getRawParameterValue(BYPASS_ID) > 0.5f)
        return;

    // Update smoothed parameters
    rateSmoothed.setTargetValue(*valueTreeState.getRawParameterValue(RATE_ID));
    depthSmoothed.setTargetValue(*valueTreeState.getRawParameterValue(DEPTH_ID));
    feedbackSmoothed.setTargetValue(*valueTreeState.getRawParameterValue(FEEDBACK_ID));
    mixSmoothed.setTargetValue(*valueTreeState.getRawParameterValue(MIX_ID));
    
    const int stages = static_cast<int>(*valueTreeState.getRawParameterValue(STAGES_ID));
    
    // Store dry signal for mixing
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);
    
    // Process each channel
    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        auto& filters = (channel == 0) ? allPassFiltersL : allPassFiltersR;
        
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            // Update LFO
            const float rate = rateSmoothed.getNextValue();
            const float depth = depthSmoothed.getNextValue() * 0.01f; // Convert to 0-1
            const float feedback = feedbackSmoothed.getNextValue() * 0.01f * 0.95f; // Convert to 0-0.95
            
            lfoPhase += rate / static_cast<float>(currentSampleRate);
            if (lfoPhase >= 1.0f)
                lfoPhase -= 1.0f;
            
            // Calculate LFO value (sine wave)
            float lfoValue = std::sin(2.0f * juce::MathConstants<float>::pi * lfoPhase);
            
            // Map LFO to frequency range (200Hz - 2000Hz)
            float centerFreq = 1100.0f;
            float freqRange = 900.0f;
            float modulatedFreq = centerFreq + (lfoValue * freqRange * depth);
            
            // Process through all-pass filters
            float input = channelData[sample];
            float output = input;
            
            // Apply feedback
            static float feedbackMemoryL = 0.0f;
            static float feedbackMemoryR = 0.0f;
            float& feedbackMemory = (channel == 0) ? feedbackMemoryL : feedbackMemoryR;
            
            output += feedbackMemory * feedback;
            
            // Process through stages
            for (int stage = 0; stage < stages; ++stage)
            {
                output = filters[stage].process(output, modulatedFreq);
            }
            
            feedbackMemory = output;
            
            // Mix dry and wet signals
            const float mix = mixSmoothed.getNextValue() * 0.01f;
            channelData[sample] = input * (1.0f - mix) + output * mix;
        }
    }
}

//==============================================================================
bool PhaserProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* PhaserProcessor::createEditor()
{
    return new PhaserEditor(*this);
}

//==============================================================================
void PhaserProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = valueTreeState.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PhaserProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(valueTreeState.state.getType()))
            valueTreeState.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout PhaserProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID, "Bypass", false));

    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        RATE_ID, "Rate", 
        juce::NormalisableRange<float>(0.01f, 10.0f, 0.01f, 0.5f), 0.5f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 2) + " Hz"; }));

    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DEPTH_ID, "Depth", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " %"; }));

    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        FEEDBACK_ID, "Feedback", 
        juce::NormalisableRange<float>(-95.0f, 95.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " %"; }));

    parameters.push_back(std::make_unique<juce::AudioParameterInt>(
        STAGES_ID, "Stages", 2, 12, 4));

    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        MIX_ID, "Mix", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " %"; }));

    return { parameters.begin(), parameters.end() };
}