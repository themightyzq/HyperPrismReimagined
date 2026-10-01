//==============================================================================
// HyperPrism Reimagined - Tremolo Processor Implementation
//==============================================================================

#include "TremoloProcessor.h"
#include "TremoloEditor.h"

// Parameter IDs
const juce::String TremoloProcessor::BYPASS_ID = "bypass";
const juce::String TremoloProcessor::RATE_ID = "rate";
const juce::String TremoloProcessor::DEPTH_ID = "depth";
const juce::String TremoloProcessor::WAVEFORM_ID = "waveform";
const juce::String TremoloProcessor::STEREO_PHASE_ID = "stereoPhase";
const juce::String TremoloProcessor::MIX_ID = "mix";

//==============================================================================
TremoloProcessor::TremoloProcessor()
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

TremoloProcessor::~TremoloProcessor()
{
}

//==============================================================================
const juce::String TremoloProcessor::getName() const
{
    return JucePlugin_Name;
}

bool TremoloProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool TremoloProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool TremoloProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double TremoloProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int TremoloProcessor::getNumPrograms()
{
    return 1;
}

int TremoloProcessor::getCurrentProgram()
{
    return 0;
}

void TremoloProcessor::setCurrentProgram(int)
{
}

const juce::String TremoloProcessor::getProgramName(int)
{
    return "Default";
}

void TremoloProcessor::changeProgramName(int, const juce::String&)
{
}

//==============================================================================
void TremoloProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    // Prepare the LFO
    lfoLeft.prepare(sampleRate);
    
    // Initialize smoothed values
    const float smoothTime = 0.02f; // 20ms smoothing
    rateSmoothed.reset(sampleRate, smoothTime);
    depthSmoothed.reset(sampleRate, smoothTime);
    mixSmoothed.reset(sampleRate, smoothTime);
    
    // Set initial values
    rateSmoothed.setCurrentAndTargetValue(*valueTreeState.getRawParameterValue(RATE_ID));
    depthSmoothed.setCurrentAndTargetValue(*valueTreeState.getRawParameterValue(DEPTH_ID));
    mixSmoothed.setCurrentAndTargetValue(*valueTreeState.getRawParameterValue(MIX_ID));
    
    // Reset the LFO phase
    lfoLeft.reset();

    preparedBlockSize = juce::jmax(1, samplesPerBlock);
    dryBuffer.setSize(juce::jmax(1, juce::jmax(getTotalNumInputChannels(), getTotalNumOutputChannels())),
                      preparedBlockSize);
    rateValues.assign(static_cast<size_t>(preparedBlockSize), 0.0f);
    depthValues.assign(static_cast<size_t>(preparedBlockSize), 0.0f);
    mixValues.assign(static_cast<size_t>(preparedBlockSize), 0.0f);
}

void TremoloProcessor::releaseResources()
{
    lfoLeft.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool TremoloProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
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

void TremoloProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
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
    mixSmoothed.setTargetValue(*valueTreeState.getRawParameterValue(MIX_ID));
    
    // Get waveform and stereo phase
    const auto waveform = static_cast<Waveform>(static_cast<int>(*valueTreeState.getRawParameterValue(WAVEFORM_ID)));
    const float stereoPhase = *valueTreeState.getRawParameterValue(STEREO_PHASE_ID) / 360.0f;
    
    // Split host blocks larger than the prepared size so dryBuffer is never outgrown
    const int totalSamples = buffer.getNumSamples();
    for (int start = 0; start < totalSamples; start += preparedBlockSize)
    {
        const int len = juce::jmin(preparedBlockSize, totalSamples - start);
        juce::AudioBuffer<float> chunk(buffer.getArrayOfWritePointers(), buffer.getNumChannels(), start, len);
        processChunk(chunk, waveform, stereoPhase);
    }
}

void TremoloProcessor::processChunk(juce::AudioBuffer<float>& buffer, Waveform waveform, float stereoPhase)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin(buffer.getNumChannels(), dryBuffer.getNumChannels());

    // Store dry signal for mixing
    for (int channel = 0; channel < numChannels; ++channel)
        dryBuffer.copyFrom(channel, 0, buffer, channel, 0, numSamples);

    // Advance each smoother exactly once per sample; every channel then reads the same values
    for (int sample = 0; sample < numSamples; ++sample)
    {
        rateValues[static_cast<size_t>(sample)] = rateSmoothed.getNextValue();
        depthValues[static_cast<size_t>(sample)] = depthSmoothed.getNextValue() * 0.01f; // Convert to 0-1
        mixValues[static_cast<size_t>(sample)] = mixSmoothed.getNextValue() * 0.01f;     // Convert to 0-1
    }

    // One LFO step per sample; the right channel reads the same LFO Stereo Phase later.
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float rate = rateValues[static_cast<size_t>(sample)];
        const float depth = depthValues[static_cast<size_t>(sample)];
        const float mix = mixValues[static_cast<size_t>(sample)];

        const float leftValue = lfoLeft.process(rate, waveform);
        float rightPhase = lfoLeft.getPhase() + stereoPhase;
        if (rightPhase >= 1.0f)
            rightPhase -= 1.0f;
        const float rightValue = LFO::shape(rightPhase, waveform);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            const auto* dryData = dryBuffer.getReadPointer(channel);
            const float lfoValue = (channel == 0) ? leftValue : rightValue;

            // Convert bipolar LFO (-1 to 1) to unipolar amplitude modulation (0 to 1)
            // At depth = 0%, amplitude stays at 1.0
            // At depth = 100%, amplitude varies from 0.0 to 1.0
            const float amplitude = 1.0f - (depth * 0.5f * (1.0f - lfoValue));

            // Apply tremolo effect and mix dry and wet signals
            const float wetSignal = channelData[static_cast<size_t>(sample)] * amplitude;
            channelData[static_cast<size_t>(sample)] = dryData[static_cast<size_t>(sample)] * (1.0f - mix) + wetSignal * mix;
        }
    }
}

//==============================================================================
bool TremoloProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* TremoloProcessor::createEditor()
{
    return new TremoloEditor(*this);
}

//==============================================================================
void TremoloProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = valueTreeState.copyState();
    state.setProperty("editor_width", getEditorWidth(), nullptr);
    state.setProperty("editor_height", getEditorHeight(), nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void TremoloProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName(valueTreeState.state.getType()))
        {
            auto newTree = juce::ValueTree::fromXml(*xmlState);
            setEditorSize((int) newTree.getProperty("editor_width", 0),
                          (int) newTree.getProperty("editor_height", 0));
            newTree.removeProperty("editor_width", nullptr);
            newTree.removeProperty("editor_height", nullptr);
            valueTreeState.replaceState(newTree);
        }
    }
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout TremoloProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID, "Bypass", false));

    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        RATE_ID, "Rate", 
        juce::NormalisableRange<float>(0.1f, 20.0f, 0.01f, 0.5f), 2.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 2) + " Hz"; }));

    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DEPTH_ID, "Depth", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " %"; }));

    parameters.push_back(std::make_unique<juce::AudioParameterChoice>(
        WAVEFORM_ID, "Waveform", 
        juce::StringArray{ "Sine", "Triangle", "Square" }, 0));

    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        STEREO_PHASE_ID, "Stereo Phase", 
        juce::NormalisableRange<float>(0.0f, 180.0f, 1.0f), 90.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(static_cast<int>(value)) + " deg"; }));

    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        MIX_ID, "Mix", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " %"; }));

    return { parameters.begin(), parameters.end() };
}