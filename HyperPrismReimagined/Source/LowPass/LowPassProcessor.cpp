//==============================================================================
// HyperPrism Revived - Low-Pass Filter Processor Implementation
//==============================================================================

#include "LowPassProcessor.h"
#include "LowPassEditor.h"

// Parameter IDs
const juce::String LowPassProcessor::BYPASS_ID = "bypass";
const juce::String LowPassProcessor::FREQUENCY_ID = "frequency";
const juce::String LowPassProcessor::RESONANCE_ID = "resonance";
const juce::String LowPassProcessor::GAIN_ID = "gain";
const juce::String LowPassProcessor::MIX_ID = "mix";

//==============================================================================
LowPassProcessor::LowPassProcessor()
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

LowPassProcessor::~LowPassProcessor()
{
}

//==============================================================================
const juce::String LowPassProcessor::getName() const
{
    return JucePlugin_Name;
}

bool LowPassProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool LowPassProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool LowPassProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double LowPassProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int LowPassProcessor::getNumPrograms()
{
    return 1;
}

int LowPassProcessor::getCurrentProgram()
{
    return 0;
}

void LowPassProcessor::setCurrentProgram(int index)
{
}

const juce::String LowPassProcessor::getProgramName(int index)
{
    return "Default";
}

void LowPassProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void LowPassProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());
    
    lowPassFilter.prepare(spec);
    
    // Initialize smoothed values
    const float smoothTime = 0.005f; // 5ms smoothing for real-time response
    frequencySmoothed.reset(sampleRate, smoothTime);
    resonanceSmoothed.reset(sampleRate, smoothTime);
    gainSmoothed.reset(sampleRate, smoothTime);
    mixSmoothed.reset(sampleRate, smoothTime);
    
    // Set initial values
    frequencySmoothed.setCurrentAndTargetValue(valueTreeState.getRawParameterValue(FREQUENCY_ID)->load());
    resonanceSmoothed.setCurrentAndTargetValue(valueTreeState.getRawParameterValue(RESONANCE_ID)->load());
    gainSmoothed.setCurrentAndTargetValue(valueTreeState.getRawParameterValue(GAIN_ID)->load());
    mixSmoothed.setCurrentAndTargetValue(valueTreeState.getRawParameterValue(MIX_ID)->load());
    
    updateFilter();
}

void LowPassProcessor::releaseResources()
{
    lowPassFilter.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool LowPassProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
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

void LowPassProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Check bypass
    if (valueTreeState.getRawParameterValue(BYPASS_ID)->load() > 0.5f)
        return;

    // Update smoothed parameters
    frequencySmoothed.setTargetValue(valueTreeState.getRawParameterValue(FREQUENCY_ID)->load());
    resonanceSmoothed.setTargetValue(valueTreeState.getRawParameterValue(RESONANCE_ID)->load());
    gainSmoothed.setTargetValue(valueTreeState.getRawParameterValue(GAIN_ID)->load());
    mixSmoothed.setTargetValue(valueTreeState.getRawParameterValue(MIX_ID)->load());

    // Store dry signal for mixing
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    // Always update filter to ensure real-time parameter changes
    updateFilter();

    // Apply filter
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    lowPassFilter.process(context);

    // Apply gain
    float currentGain = juce::Decibels::decibelsToGain(valueTreeState.getRawParameterValue(GAIN_ID)->load());
    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            channelData[sample] *= currentGain;
        }
    }

    // Mix dry and wet signals
    float mixValue = valueTreeState.getRawParameterValue(MIX_ID)->load() * 0.01f; // Convert percentage to ratio
    for (int channel = 0; channel < totalNumOutputChannels; ++channel)
    {
        auto* wetData = buffer.getWritePointer(channel);
        auto* dryData = dryBuffer.getReadPointer(channel);
        
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            wetData[sample] = dryData[sample] * (1.0f - mixValue) + wetData[sample] * mixValue;
        }
    }
}

//==============================================================================
bool LowPassProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* LowPassProcessor::createEditor()
{
    return new LowPassEditor(*this);
}

//==============================================================================
void LowPassProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = valueTreeState.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void LowPassProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(valueTreeState.state.getType()))
            valueTreeState.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
void LowPassProcessor::updateFilter()
{
    float frequency = valueTreeState.getRawParameterValue(FREQUENCY_ID)->load();
    float resonance = valueTreeState.getRawParameterValue(RESONANCE_ID)->load();
    
    // Clamp frequency to valid range
    frequency = juce::jlimit(20.0f, static_cast<float>(currentSampleRate * 0.45), frequency);
    
    // Calculate Q from resonance (0-100% -> 0.1-20)
    float q = juce::jmap(resonance, 0.0f, 100.0f, 0.1f, 20.0f);
    
    // Create low-pass filter coefficients
    auto coefficients = CoefficientsType::makeLowPass(currentSampleRate, frequency, q);
    *lowPassFilter.state = *coefficients;
}

juce::AudioProcessorValueTreeState::ParameterLayout LowPassProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID, "Bypass", false));

    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        FREQUENCY_ID, "Frequency", 
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 10000.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 0) + " Hz"; }));

    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        RESONANCE_ID, "Resonance", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 10.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " %"; }));

    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        GAIN_ID, "Gain", 
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        MIX_ID, "Mix", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " %"; }));

    return { parameters.begin(), parameters.end() };
}