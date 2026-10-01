//==============================================================================
// HyperPrism Reimagined - Band-Pass Filter Processor Implementation
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
    preparedBlockSize = juce::jmax(1, samplesPerBlock);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(preparedBlockSize);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    // Give both filters real order-2 coefficients BEFORE prepare(), so the filter order never
    // changes (which would reallocate filter state) on the audio thread.
    updateFilters(*valueTreeState.getRawParameterValue(CENTER_FREQ_ID),
                  *valueTreeState.getRawParameterValue(BANDWIDTH_ID));

    highPassFilter.prepare(spec);
    lowPassFilter.prepare(spec);

    // Initialize smoothed values (30 ms ramps)
    const double smoothTime = 0.03; // 30 ms (the old 5 ms smoothers were never read)
    centerFreqSmoothed.reset(sampleRate, smoothTime);
    bandwidthSmoothed.reset(sampleRate, smoothTime);
    gainSmoothed.reset(sampleRate, smoothTime);
    mixSmoothed.reset(sampleRate, smoothTime);

    // Set initial values
    centerFreqSmoothed.setCurrentAndTargetValue(*valueTreeState.getRawParameterValue(CENTER_FREQ_ID));
    bandwidthSmoothed.setCurrentAndTargetValue(*valueTreeState.getRawParameterValue(BANDWIDTH_ID));
    gainSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(valueTreeState.getRawParameterValue(GAIN_ID)->load()));
    mixSmoothed.setCurrentAndTargetValue(valueTreeState.getRawParameterValue(MIX_ID)->load() * 0.01f);

    dryBuffer.setSize(juce::jmax(1, juce::jmax(getTotalNumInputChannels(), getTotalNumOutputChannels())),
                      preparedBlockSize);
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

void BandPassProcessor::processChunk(juce::AudioBuffer<float>& chunk)
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
        if (centerFreqSmoothed.isSmoothing() || bandwidthSmoothed.isSmoothing())
            updateFilters(centerFreqSmoothed.skip(len), bandwidthSmoothed.skip(len));

        // Apply filters in series (high-pass then low-pass = band-pass)
        auto subBlock = fullBlock.getSubBlock(static_cast<size_t>(pos), static_cast<size_t>(len));
        juce::dsp::ProcessContextReplacing<float> context(subBlock);
        highPassFilter.process(context);
        lowPassFilter.process(context);

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
    state.setProperty("editor_width",  getEditorWidth(),  nullptr);
    state.setProperty("editor_height", getEditorHeight(), nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void BandPassProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(valueTreeState.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml(*xmlState);
            setEditorSize(static_cast<int>(tree.getProperty("editor_width", 0)),
                          static_cast<int>(tree.getProperty("editor_height", 0)));
            tree.removeProperty("editor_width", nullptr);
            tree.removeProperty("editor_height", nullptr);
            valueTreeState.replaceState(tree);
        }
}

//==============================================================================
void BandPassProcessor::updateFilters(float centerFreq, float bandwidth)
{
    // Calculate cutoff frequencies from center frequency and bandwidth
    // Bandwidth in octaves: bandwidth = log2(f2/f1)
    float octaves = bandwidth * 0.01f * 4.0f; // 0-100% maps to 0-4 octaves
    float factor = std::pow(2.0f, octaves * 0.5f);

    float lowFreq = centerFreq / factor;
    float highFreq = centerFreq * factor;

    // Clamp frequencies to valid range
    lowFreq = juce::jlimit(20.0f, static_cast<float>(currentSampleRate * 0.45), lowFreq);
    highFreq = juce::jlimit(20.0f, static_cast<float>(currentSampleRate * 0.45), highFreq);

    // Write coefficients in place (Q = 0.707 for Butterworth response); no allocation
    *highPassFilter.state = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass(currentSampleRate, lowFreq, 0.707f);
    *lowPassFilter.state = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(currentSampleRate, highFreq, 0.707f);
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