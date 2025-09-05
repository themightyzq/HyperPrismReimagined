//==============================================================================
// HyperPrism Revived - High-Pass Filter Processor Implementation
//==============================================================================

#include "HighPassProcessor.h"
#include "HighPassEditor.h"

// Parameter IDs
const juce::String HighPassProcessor::BYPASS_ID = "bypass";
const juce::String HighPassProcessor::FREQUENCY_ID = "frequency";
const juce::String HighPassProcessor::RESONANCE_ID = "resonance";
const juce::String HighPassProcessor::GAIN_ID = "gain";
const juce::String HighPassProcessor::MIX_ID = "mix";

//==============================================================================
HighPassProcessor::HighPassProcessor()
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

HighPassProcessor::~HighPassProcessor()
{
}

//==============================================================================
const juce::String HighPassProcessor::getName() const
{
    return JucePlugin_Name;
}

bool HighPassProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool HighPassProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool HighPassProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double HighPassProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int HighPassProcessor::getNumPrograms()
{
    return 1;
}

int HighPassProcessor::getCurrentProgram()
{
    return 0;
}

void HighPassProcessor::setCurrentProgram(int index)
{
}

const juce::String HighPassProcessor::getProgramName(int index)
{
    return "Default";
}

void HighPassProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void HighPassProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());
    
    highPassFilter.prepare(spec);
    
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

void HighPassProcessor::releaseResources()
{
    highPassFilter.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool HighPassProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
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

void HighPassProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
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
    highPassFilter.process(context);

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
bool HighPassProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* HighPassProcessor::createEditor()
{
    return new HighPassEditor(*this);
}

//==============================================================================
void HighPassProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = valueTreeState.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void HighPassProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(valueTreeState.state.getType()))
            valueTreeState.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
void HighPassProcessor::updateFilter()
{
    float frequency = valueTreeState.getRawParameterValue(FREQUENCY_ID)->load();
    float resonance = valueTreeState.getRawParameterValue(RESONANCE_ID)->load();
    
    // Clamp frequency to valid range
    frequency = juce::jlimit(20.0f, static_cast<float>(currentSampleRate * 0.45), frequency);
    
    // Calculate Q from resonance (0-100% -> 0.1-20)
    float q = juce::jmap(resonance, 0.0f, 100.0f, 0.1f, 20.0f);
    
    // Create high-pass filter coefficients
    auto coefficients = CoefficientsType::makeHighPass(currentSampleRate, frequency, q);
    *highPassFilter.state = *coefficients;
}

juce::AudioProcessorValueTreeState::ParameterLayout HighPassProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID, "Bypass", false));

    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        FREQUENCY_ID, "Frequency", 
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 1000.0f,
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