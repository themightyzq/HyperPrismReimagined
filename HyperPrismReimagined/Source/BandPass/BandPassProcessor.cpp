//==============================================================================
// HyperPrism Revived - Band-Pass Filter Processor Implementation
//==============================================================================

#include "BandPassProcessor.h"
#include "BandPassEditor.h"

// Parameter IDs
const juce::String BandPassProcessor::BYPASS_ID = "bypass";
const juce::String BandPassProcessor::CENTER_FREQ_ID = "centerFreq";
const juce::String BandPassProcessor::BANDWIDTH_ID = "bandwidth";
const juce::String BandPassProcessor::GAIN_ID = "gain";
const juce::String BandPassProcessor::MIX_ID = "mix";

//==============================================================================
BandPassProcessor::BandPassProcessor()
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

BandPassProcessor::~BandPassProcessor()
{
}

//==============================================================================
const juce::String BandPassProcessor::getName() const
{
    return JucePlugin_Name;
}

bool BandPassProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool BandPassProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool BandPassProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double BandPassProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int BandPassProcessor::getNumPrograms()
{
    return 1;
}

int BandPassProcessor::getCurrentProgram()
{
    return 0;
}

void BandPassProcessor::setCurrentProgram(int)
{
}

const juce::String BandPassProcessor::getProgramName(int)
{
    return "Default";
}

void BandPassProcessor::changeProgramName(int, const juce::String&)
{
}

//==============================================================================
void BandPassProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());
    
    highPassFilter.prepare(spec);
    lowPassFilter.prepare(spec);
    
    // Initialize smoothed values
    const float smoothTime = 0.005f; // 5ms smoothing for real-time response
    centerFreqSmoothed.reset(sampleRate, smoothTime);
    bandwidthSmoothed.reset(sampleRate, smoothTime);
    gainSmoothed.reset(sampleRate, smoothTime);
    mixSmoothed.reset(sampleRate, smoothTime);
    
    // Set initial values
    centerFreqSmoothed.setCurrentAndTargetValue(*valueTreeState.getRawParameterValue(CENTER_FREQ_ID));
    bandwidthSmoothed.setCurrentAndTargetValue(*valueTreeState.getRawParameterValue(BANDWIDTH_ID));
    gainSmoothed.setCurrentAndTargetValue(*valueTreeState.getRawParameterValue(GAIN_ID));
    mixSmoothed.setCurrentAndTargetValue(*valueTreeState.getRawParameterValue(MIX_ID));
    
    updateFilters();
}

void BandPassProcessor::releaseResources()
{
    highPassFilter.reset();
    lowPassFilter.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool BandPassProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
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

void BandPassProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
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
    centerFreqSmoothed.setTargetValue(*valueTreeState.getRawParameterValue(CENTER_FREQ_ID));
    bandwidthSmoothed.setTargetValue(*valueTreeState.getRawParameterValue(BANDWIDTH_ID));
    gainSmoothed.setTargetValue(*valueTreeState.getRawParameterValue(GAIN_ID));
    mixSmoothed.setTargetValue(*valueTreeState.getRawParameterValue(MIX_ID));

    // Store dry signal for mixing
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    // Always update filters to ensure real-time parameter changes
    updateFilters();

    // Apply filters in series (high-pass then low-pass = band-pass)
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    
    highPassFilter.process(context);
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
bool BandPassProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* BandPassProcessor::createEditor()
{
    return new BandPassEditor(*this);
}

//==============================================================================
void BandPassProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = valueTreeState.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void BandPassProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(valueTreeState.state.getType()))
            valueTreeState.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
void BandPassProcessor::updateFilters()
{
    float centerFreq = valueTreeState.getRawParameterValue(CENTER_FREQ_ID)->load();
    float bandwidth = valueTreeState.getRawParameterValue(BANDWIDTH_ID)->load();
    
    // Calculate cutoff frequencies from center frequency and bandwidth
    // Bandwidth in octaves: bandwidth = log2(f2/f1)
    float octaves = bandwidth * 0.01f * 4.0f; // 0-100% maps to 0-4 octaves
    float factor = std::pow(2.0f, octaves * 0.5f);
    
    float lowFreq = centerFreq / factor;
    float highFreq = centerFreq * factor;
    
    // Clamp frequencies to valid range
    lowFreq = juce::jlimit(20.0f, static_cast<float>(currentSampleRate * 0.45), lowFreq);
    highFreq = juce::jlimit(20.0f, static_cast<float>(currentSampleRate * 0.45), highFreq);
    
    // Create filter coefficients (Q = 0.707 for Butterworth response)
    auto hpCoeffs = CoefficientsType::makeHighPass(currentSampleRate, lowFreq, 0.707f);
    auto lpCoeffs = CoefficientsType::makeLowPass(currentSampleRate, highFreq, 0.707f);
    
    // Properly assign coefficients to ProcessorDuplicator
    *highPassFilter.state = *hpCoeffs;
    *lowPassFilter.state = *lpCoeffs;
}

juce::AudioProcessorValueTreeState::ParameterLayout BandPassProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID, "Bypass", false));

    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        CENTER_FREQ_ID, "Center Frequency", 
        juce::NormalisableRange<float>(100.0f, 10000.0f, 1.0f, 0.3f), 1000.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 0) + " Hz"; }));

    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        BANDWIDTH_ID, "Bandwidth", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f,
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