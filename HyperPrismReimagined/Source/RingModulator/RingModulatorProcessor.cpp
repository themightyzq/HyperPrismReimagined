#include "RingModulatorProcessor.h"
#include "RingModulatorEditor.h"

RingModulatorProcessor::RingModulatorProcessor()
    : AudioProcessor(BusesProperties()
                    .withInput("Input", juce::AudioChannelSet::stereo(), true)
                    .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

RingModulatorProcessor::~RingModulatorProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout RingModulatorProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Carrier Frequency (1 Hz to 8000 Hz)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "carrier_freq",
        "Carrier Frequency",
        juce::NormalisableRange<float>(1.0f, 8000.0f, 0.1f, 0.5f),
        440.0f));

    // Modulator Frequency (0.1 Hz to 1000 Hz)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "modulator_freq",
        "Modulator Frequency",
        juce::NormalisableRange<float>(0.1f, 1000.0f, 0.01f, 0.5f),
        5.0f));

    // Carrier Waveform (0: Sine, 1: Triangle, 2: Square, 3: Saw)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "carrier_waveform",
        "Carrier Waveform",
        juce::StringArray{"Sine", "Triangle", "Square", "Saw"},
        0));

    // Modulator Waveform (0: Sine, 1: Triangle, 2: Square, 3: Saw)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "modulator_waveform",
        "Modulator Waveform",
        juce::StringArray{"Sine", "Triangle", "Square", "Saw"},
        0));

    // Mix (0-100%)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "mix",
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        50.0f));

    return { params.begin(), params.end() };
}

void RingModulatorProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Reset phases
    carrierPhase = 0.0f;
    modulatorPhase = 0.0f;
}

void RingModulatorProcessor::releaseResources()
{
}

bool RingModulatorProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

float RingModulatorProcessor::generateWaveform(float phase, int waveformType)
{
    switch (waveformType)
    {
        case 0: // Sine
            return std::sin(phase);
            
        case 1: // Triangle
        {
            float normalizedPhase = phase / juce::MathConstants<float>::twoPi;
            normalizedPhase = std::fmod(normalizedPhase, 1.0f);
            if (normalizedPhase < 0.25f)
                return 4.0f * normalizedPhase;
            else if (normalizedPhase < 0.75f)
                return 2.0f - 4.0f * normalizedPhase;
            else
                return 4.0f * normalizedPhase - 4.0f;
        }
            
        case 2: // Square
            return (std::sin(phase) > 0.0f) ? 1.0f : -1.0f;
            
        case 3: // Saw
        {
            float normalizedPhase = phase / juce::MathConstants<float>::twoPi;
            normalizedPhase = std::fmod(normalizedPhase, 1.0f);
            return 2.0f * normalizedPhase - 1.0f;
        }
            
        default:
            return 0.0f;
    }
}

void RingModulatorProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    const float sampleRate = static_cast<float>(getSampleRate());
    
    // Get parameter values
    const float carrierFreq = apvts.getRawParameterValue("carrier_freq")->load();
    const float modulatorFreq = apvts.getRawParameterValue("modulator_freq")->load();
    const int carrierWaveform = static_cast<int>(apvts.getRawParameterValue("carrier_waveform")->load());
    const int modulatorWaveform = static_cast<int>(apvts.getRawParameterValue("modulator_waveform")->load());
    const float mixPercent = apvts.getRawParameterValue("mix")->load();
    const float mix = mixPercent * 0.01f;
    
    // Calculate phase increments
    const float carrierPhaseInc = (carrierFreq * juce::MathConstants<float>::twoPi) / sampleRate;
    const float modulatorPhaseInc = (modulatorFreq * juce::MathConstants<float>::twoPi) / sampleRate;
    
    // Process each channel
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        
        // Reset phases for each channel to maintain stereo consistency
        float localCarrierPhase = carrierPhase;
        float localModulatorPhase = modulatorPhase;
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Generate carrier and modulator signals
            const float carrier = generateWaveform(localCarrierPhase, carrierWaveform);
            const float modulator = generateWaveform(localModulatorPhase, modulatorWaveform);
            
            // Ring modulation: multiply input signal with carrier modulated by modulator
            const float ringModSignal = channelData[sample] * carrier * (1.0f + modulator) * 0.5f;
            
            // Mix dry and wet signals
            channelData[sample] = (1.0f - mix) * channelData[sample] + mix * ringModSignal;
            
            // Update phases
            localCarrierPhase += carrierPhaseInc;
            localModulatorPhase += modulatorPhaseInc;
            
            // Wrap phases
            if (localCarrierPhase >= juce::MathConstants<float>::twoPi)
                localCarrierPhase -= juce::MathConstants<float>::twoPi;
            if (localModulatorPhase >= juce::MathConstants<float>::twoPi)
                localModulatorPhase -= juce::MathConstants<float>::twoPi;
        }
    }
    
    // Update global phases for next block
    carrierPhase += carrierPhaseInc * numSamples;
    modulatorPhase += modulatorPhaseInc * numSamples;
    
    // Wrap global phases
    while (carrierPhase >= juce::MathConstants<float>::twoPi)
        carrierPhase -= juce::MathConstants<float>::twoPi;
    while (modulatorPhase >= juce::MathConstants<float>::twoPi)
        modulatorPhase -= juce::MathConstants<float>::twoPi;
}

bool RingModulatorProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* RingModulatorProcessor::createEditor()
{
    return new RingModulatorEditor(*this);
}

void RingModulatorProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void RingModulatorProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

const juce::String RingModulatorProcessor::getName() const
{
    return "HyperPrism Reimagined Ring Modulator";
}

bool RingModulatorProcessor::acceptsMidi() const
{
    return false;
}

bool RingModulatorProcessor::producesMidi() const
{
    return false;
}

bool RingModulatorProcessor::isMidiEffect() const
{
    return false;
}

double RingModulatorProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int RingModulatorProcessor::getNumPrograms()
{
    return 1;
}

int RingModulatorProcessor::getCurrentProgram()
{
    return 0;
}

void RingModulatorProcessor::setCurrentProgram(int index)
{
}

const juce::String RingModulatorProcessor::getProgramName(int index)
{
    return {};
}

void RingModulatorProcessor::changeProgramName(int index, const juce::String& newName)
{
}