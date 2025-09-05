#include "CompressorProcessor.h"
#include "CompressorEditor.h"

CompressorProcessor::CompressorProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    // Get parameter pointers
    thresholdParam = apvts.getRawParameterValue("threshold");
    ratioParam = apvts.getRawParameterValue("ratio");
    attackParam = apvts.getRawParameterValue("attack");
    releaseParam = apvts.getRawParameterValue("release");
    kneeParam = apvts.getRawParameterValue("knee");
    makeupGainParam = apvts.getRawParameterValue("makeupGain");
    mixParam = apvts.getRawParameterValue("mix");
}

CompressorProcessor::~CompressorProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout CompressorProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "threshold",
        "Threshold",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f),
        -20.0f,
        "dB"));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "ratio",
        "Ratio",
        juce::NormalisableRange<float>(1.0f, 20.0f, 0.1f),
        4.0f,
        ":1"));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "attack",
        "Attack",
        juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f, 0.5f),
        10.0f,
        "ms"));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "release",
        "Release",
        juce::NormalisableRange<float>(10.0f, 2000.0f, 1.0f, 0.5f),
        100.0f,
        "ms"));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "knee",
        "Knee",
        juce::NormalisableRange<float>(0.0f, 10.0f, 0.1f),
        2.0f,
        "dB"));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "makeupGain",
        "Makeup Gain",
        juce::NormalisableRange<float>(-20.0f, 20.0f, 0.1f),
        0.0f,
        "dB"));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "mix",
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f,
        "%"));

    return layout;
}

void CompressorProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    envelope = 0.0f;
}

void CompressorProcessor::releaseResources()
{
}

bool CompressorProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

float CompressorProcessor::calculateAttackCoeff(float attackTimeMs)
{
    return std::exp(-1.0f / (attackTimeMs * 0.001f * currentSampleRate));
}

float CompressorProcessor::calculateReleaseCoeff(float releaseTimeMs)
{
    return std::exp(-1.0f / (releaseTimeMs * 0.001f * currentSampleRate));
}

float CompressorProcessor::applyCompression(float inputSample, float& envelopeValue)
{
    const float threshold = thresholdParam->load();
    const float ratio = ratioParam->load();
    const float knee = kneeParam->load();
    const float makeupGain = juce::Decibels::decibelsToGain(makeupGainParam->load());
    
    // Get input level in dB
    float inputDb = juce::Decibels::gainToDecibels(std::abs(inputSample));
    
    // Calculate gain reduction
    float gainReductionDb = 0.0f;
    
    if (inputDb > threshold)
    {
        // Hard knee compression
        if (knee < 0.1f)
        {
            gainReductionDb = (inputDb - threshold) * (1.0f - 1.0f / ratio);
        }
        // Soft knee compression
        else
        {
            float kneeStart = threshold - knee;
            float kneeEnd = threshold + knee;
            
            if (inputDb > kneeEnd)
            {
                gainReductionDb = (inputDb - threshold) * (1.0f - 1.0f / ratio);
            }
            else if (inputDb > kneeStart)
            {
                float kneeProgress = (inputDb - kneeStart) / (2.0f * knee);
                float kneeRatio = 1.0f + (ratio - 1.0f) * kneeProgress * kneeProgress;
                gainReductionDb = (inputDb - kneeStart) * (1.0f - 1.0f / kneeRatio);
            }
        }
    }
    
    float targetGainReduction = juce::Decibels::decibelsToGain(-gainReductionDb);
    
    // Apply attack/release envelope
    float attackCoeff = calculateAttackCoeff(attackParam->load());
    float releaseCoeff = calculateReleaseCoeff(releaseParam->load());
    
    if (targetGainReduction < envelopeValue)
        envelopeValue = targetGainReduction + (envelopeValue - targetGainReduction) * attackCoeff;
    else
        envelopeValue = targetGainReduction + (envelopeValue - targetGainReduction) * releaseCoeff;
    
    // Store current gain reduction for metering
    currentGainReduction.store(1.0f - envelopeValue);
    
    // Apply compression and makeup gain
    return inputSample * envelopeValue * makeupGain;
}

void CompressorProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    const float mixAmount = mixParam->load() * 0.01f;
    
    // Create a copy for dry signal (parallel compression)
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);
    
    // Process each channel
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        auto* dryData = dryBuffer.getReadPointer(channel);
        
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float inputSample = channelData[sample];
            float compressedSample = applyCompression(inputSample, envelope);
            
            // Mix dry and wet signals for parallel compression
            channelData[sample] = dryData[sample] * (1.0f - mixAmount) + compressedSample * mixAmount;
        }
    }
}

bool CompressorProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* CompressorProcessor::createEditor()
{
    return new CompressorEditor(*this);
}

const juce::String CompressorProcessor::getName() const
{
    return JucePlugin_Name;
}

bool CompressorProcessor::acceptsMidi() const
{
    return false;
}

bool CompressorProcessor::producesMidi() const
{
    return false;
}

bool CompressorProcessor::isMidiEffect() const
{
    return false;
}

double CompressorProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int CompressorProcessor::getNumPrograms()
{
    return 1;
}

int CompressorProcessor::getCurrentProgram()
{
    return 0;
}

void CompressorProcessor::setCurrentProgram(int index)
{
}

const juce::String CompressorProcessor::getProgramName(int index)
{
    return {};
}

void CompressorProcessor::changeProgramName(int index, const juce::String& newName)
{
}

void CompressorProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void CompressorProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}