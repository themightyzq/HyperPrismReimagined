//==============================================================================
// HyperPrism Reimagined - Band-Reject (Notch) Filter Processor Implementation
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
    preparedBlockSize = juce::jmax(1, samplesPerBlock);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(preparedBlockSize);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Give the filter real order-2 coefficients BEFORE prepare(), so the filter order never
    // changes (which would reallocate filter state) on the audio thread.
    updateFilter(valueTreeState.getRawParameterValue(CENTER_FREQ_ID)->load(), valueTreeState.getRawParameterValue(Q_ID)->load());

    notchFilter.prepare(spec);

    // Initialize smoothed values (30 ms ramps)
    const double smoothTime = 0.03; // 30 ms (the old 5 ms smoothers were never read)
    centerFreqSmoothed.reset(sampleRate, smoothTime);
    qSmoothed.reset(sampleRate, smoothTime);
    gainSmoothed.reset(sampleRate, smoothTime);
    mixSmoothed.reset(sampleRate, smoothTime);

    // Set initial values
    centerFreqSmoothed.setCurrentAndTargetValue(valueTreeState.getRawParameterValue(CENTER_FREQ_ID)->load());
    qSmoothed.setCurrentAndTargetValue(valueTreeState.getRawParameterValue(Q_ID)->load());
    gainSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(valueTreeState.getRawParameterValue(GAIN_ID)->load()));
    mixSmoothed.setCurrentAndTargetValue(valueTreeState.getRawParameterValue(MIX_ID)->load() * 0.01f);

    dryBuffer.setSize(juce::jmax(1, juce::jmax(getTotalNumInputChannels(), getTotalNumOutputChannels())),
                      preparedBlockSize);
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
    gainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(valueTreeState.getRawParameterValue(GAIN_ID)->load()));
    mixSmoothed.setTargetValue(valueTreeState.getRawParameterValue(MIX_ID)->load() * 0.01f); // percentage to ratio

    // Split host blocks larger than the prepared size so dryBuffer is never outgrown
    const int numSamples = buffer.getNumSamples();
    for (int start = 0; start < numSamples; start += preparedBlockSize)
    {
        const int len = juce::jmin(preparedBlockSize, numSamples - start);
        juce::AudioBuffer<float> chunk(buffer.getArrayOfWritePointers(), buffer.getNumChannels(), start, len);
        processChunk(chunk);
    }
}

void BandRejectProcessor::processChunk(juce::AudioBuffer<float>& chunk)
{
    const int numSamples = chunk.getNumSamples();
    const int numChannels = juce::jmin(chunk.getNumChannels(), dryBuffer.getNumChannels());

    // Store dry signal for mixing
    for (int ch = 0; ch < numChannels; ++ch)
        dryBuffer.copyFrom(ch, 0, chunk, ch, 0, numSamples);

    juce::dsp::AudioBlock<float> chunkBlock(chunk);
    auto fullBlock = chunkBlock.getSubsetChannelBlock(0, static_cast<size_t>(numChannels));

    constexpr int subBlockSize = 16;
    for (int pos = 0; pos < numSamples; pos += subBlockSize)
    {
        const int len = juce::jmin(subBlockSize, numSamples - pos);

        // Recompute coefficients from the smoothed values while they are moving
        if (centerFreqSmoothed.isSmoothing() || qSmoothed.isSmoothing())
            updateFilter(centerFreqSmoothed.skip(len), qSmoothed.skip(len));

        auto subBlock = fullBlock.getSubBlock(static_cast<size_t>(pos), static_cast<size_t>(len));
        juce::dsp::ProcessContextReplacing<float> context(subBlock);
        notchFilter.process(context);

        // Apply gain and dry/wet mix, one smoothed value per sample
        for (int i = 0; i < len; ++i)
        {
            const float gain = gainSmoothed.getNextValue();
            const float mixValue = mixSmoothed.getNextValue();

            for (int ch = 0; ch < numChannels; ++ch)
            {
                auto* wet = chunk.getWritePointer(ch);
                const auto* dry = dryBuffer.getReadPointer(ch);
                wet[pos + i] = dry[pos + i] * (1.0f - mixValue) + wet[pos + i] * gain * mixValue;
            }
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
    state.setProperty("editor_width", getEditorWidth(), nullptr);
    state.setProperty("editor_height", getEditorHeight(), nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void BandRejectProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(valueTreeState.state.getType()))
        {
            auto newState = juce::ValueTree::fromXml(*xmlState);
            setEditorSize((int) newState.getProperty("editor_width", 0), (int) newState.getProperty("editor_height", 0));
            newState.removeProperty("editor_width", nullptr);
            newState.removeProperty("editor_height", nullptr);
            valueTreeState.replaceState(newState);
        }
}

//==============================================================================
void BandRejectProcessor::updateFilter(float centerFreq, float q)
{
    // Clamp frequency to valid range
    centerFreq = juce::jlimit(20.0f, static_cast<float>(currentSampleRate * 0.45), centerFreq);

    // Write notch coefficients in place; no allocation
    *notchFilter.state = juce::dsp::IIR::ArrayCoefficients<float>::makeNotch(currentSampleRate, centerFreq, q);
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