//==============================================================================
// HyperPrism Revived - Band-Reject (Notch) Filter Processor Implementation
//==============================================================================

#include "BandRejectProcessor.h"
#include "BandRejectEditor.h"

// Parameter IDs
const juce::String BandRejectProcessor::BYPASS_ID = "bypass";
const juce::String BandRejectProcessor::CENTER_FREQ_ID = "centerFreq";
const juce::String BandRejectProcessor::Q_ID = "q";
const juce::String BandRejectProcessor::GAIN_ID = "gain";
const juce::String BandRejectProcessor::MIX_ID = "mix";

//==============================================================================
BandRejectProcessor::BandRejectProcessor()
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

BandRejectProcessor::~BandRejectProcessor()
{
}

//==============================================================================
const juce::String BandRejectProcessor::getName() const
{
    return JucePlugin_Name;
}

bool BandRejectProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool BandRejectProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool BandRejectProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double BandRejectProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int BandRejectProcessor::getNumPrograms()
{
    return 1;
}

int BandRejectProcessor::getCurrentProgram()
{
    return 0;
}

void BandRejectProcessor::setCurrentProgram(int)
{
}

const juce::String BandRejectProcessor::getProgramName(int)
{
    return "Default";
}

void BandRejectProcessor::changeProgramName(int, const juce::String&)
{
}

//==============================================================================
void BandRejectProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());
    
    notchFilter.prepare(spec);
    
    // Initialize smoothed values
    const float smoothTime = 0.005f; // 5ms smoothing for real-time response
    centerFreqSmoothed.reset(sampleRate, smoothTime);
    qSmoothed.reset(sampleRate, smoothTime);
    gainSmoothed.reset(sampleRate, smoothTime);
    mixSmoothed.reset(sampleRate, smoothTime);
    
    // Set initial values
    centerFreqSmoothed.setCurrentAndTargetValue(valueTreeState.getRawParameterValue(CENTER_FREQ_ID)->load());
    qSmoothed.setCurrentAndTargetValue(valueTreeState.getRawParameterValue(Q_ID)->load());
    gainSmoothed.setCurrentAndTargetValue(valueTreeState.getRawParameterValue(GAIN_ID)->load());
    mixSmoothed.setCurrentAndTargetValue(valueTreeState.getRawParameterValue(MIX_ID)->load());
    
    updateFilter();
}

void BandRejectProcessor::releaseResources()
{
    notchFilter.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool BandRejectProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
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

void BandRejectProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
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
    centerFreqSmoothed.setTargetValue(valueTreeState.getRawParameterValue(CENTER_FREQ_ID)->load());
    qSmoothed.setTargetValue(valueTreeState.getRawParameterValue(Q_ID)->load());
    gainSmoothed.setTargetValue(valueTreeState.getRawParameterValue(GAIN_ID)->load());
    mixSmoothed.setTargetValue(valueTreeState.getRawParameterValue(MIX_ID)->load());

    // Store dry signal for mixing
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    // Always update filter to ensure real-time parameter changes
    updateFilter();

    // Apply notch filter
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    notchFilter.process(context);

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
bool BandRejectProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* BandRejectProcessor::createEditor()
{
    return new BandRejectEditor(*this);
}

//==============================================================================
void BandRejectProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = valueTreeState.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void BandRejectProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(valueTreeState.state.getType()))
            valueTreeState.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
void BandRejectProcessor::updateFilter()
{
    float centerFreq = valueTreeState.getRawParameterValue(CENTER_FREQ_ID)->load();
    float q = valueTreeState.getRawParameterValue(Q_ID)->load();
    
    // Clamp frequency to valid range
    centerFreq = juce::jlimit(20.0f, static_cast<float>(currentSampleRate * 0.45), centerFreq);
    
    // Create notch filter coefficients
    auto coefficients = CoefficientsType::makeNotch(currentSampleRate, centerFreq, q);
    *notchFilter.state = *coefficients;
}

juce::AudioProcessorValueTreeState::ParameterLayout BandRejectProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID, "Bypass", false));

    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        CENTER_FREQ_ID, "Center Frequency", 
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), 1000.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 0) + " Hz"; }));

    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        Q_ID, "Q", 
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.01f, 0.5f), 1.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 2); }));

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