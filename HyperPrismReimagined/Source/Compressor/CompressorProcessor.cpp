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
    bypassParam = apvts.getRawParameterValue("bypass");
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

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "bypass", "Bypass", false));

    return layout;
}

void CompressorProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    envelope = 0.0f;
    dryBuffer.setSize(getTotalNumInputChannels(), samplesPerBlock);
}

void CompressorProcessor::releaseResources()
{
}

bool CompressorProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

float CompressorProcessor::calculateAttackCoeff(float attackTimeMs)
{
    return std::exp(-1.0f / (attackTimeMs * 0.001f * currentSampleRate));
}

float CompressorProcessor::calculateReleaseCoeff(float releaseTimeMs)
{
    return std::exp(-1.0f / (releaseTimeMs * 0.001f * currentSampleRate));
}

void CompressorProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    if (bypassParam->load() > 0.5f)
        return;

    const float mixAmount = mixParam->load() * 0.01f;
    const float threshold = thresholdParam->load();
    const float ratio = ratioParam->load();
    const float knee = kneeParam->load();
    const float makeupGain = juce::Decibels::decibelsToGain(makeupGainParam->load());
    const float attackCoeff = calculateAttackCoeff(attackParam->load());
    const float releaseCoeff = calculateReleaseCoeff(releaseParam->load());

    dryBuffer.makeCopyOf(buffer);

    const int numSamples = buffer.getNumSamples();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Linked stereo: detect from max level across all channels
        float maxInputLevel = 0.0f;
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
            maxInputLevel = std::max(maxInputLevel, std::abs(buffer.getSample(channel, sample)));

        float inputDb = juce::Decibels::gainToDecibels(maxInputLevel);
        float gainReductionDb = 0.0f;

        if (inputDb > threshold)
        {
            if (knee < 0.1f)
            {
                gainReductionDb = (inputDb - threshold) * (1.0f - 1.0f / ratio);
            }
            else
            {
                float kneeStart = threshold - knee;
                float kneeEnd = threshold + knee;
                if (inputDb > kneeEnd)
                    gainReductionDb = (inputDb - threshold) * (1.0f - 1.0f / ratio);
                else if (inputDb > kneeStart)
                {
                    float kneeProgress = (inputDb - kneeStart) / (2.0f * knee);
                    float kneeRatio = 1.0f + (ratio - 1.0f) * kneeProgress * kneeProgress;
                    gainReductionDb = (inputDb - kneeStart) * (1.0f - 1.0f / kneeRatio);
                }
            }
        }

        float targetGainReduction = juce::Decibels::decibelsToGain(-gainReductionDb);

        if (targetGainReduction < envelope)
            envelope = targetGainReduction + (envelope - targetGainReduction) * attackCoeff;
        else
            envelope = targetGainReduction + (envelope - targetGainReduction) * releaseCoeff;

        currentGainReduction.store(1.0f - envelope);

        // Apply same gain to all channels
        for (int channel = 0; channel < totalNumInputChannels; ++channel)
        {
            float dry = dryBuffer.getSample(channel, sample);
            float compressed = buffer.getSample(channel, sample) * envelope * makeupGain;
            buffer.setSample(channel, sample, dry * (1.0f - mixAmount) + compressed * mixAmount);
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