#include "NoiseGateProcessor.h"
#include "NoiseGateEditor.h"
#include <array>

NoiseGateProcessor::NoiseGateProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      currentSampleRate(44100.0),
      gateOpen(false),
      valueTreeState(*this, nullptr, stateType, createParameterLayout())
{
    threshold       = dynamic_cast<juce::AudioParameterFloat*>(valueTreeState.getParameter("threshold"));
    attack          = dynamic_cast<juce::AudioParameterFloat*>(valueTreeState.getParameter("attack"));
    hold            = dynamic_cast<juce::AudioParameterFloat*>(valueTreeState.getParameter("hold"));
    release         = dynamic_cast<juce::AudioParameterFloat*>(valueTreeState.getParameter("release"));
    range           = dynamic_cast<juce::AudioParameterFloat*>(valueTreeState.getParameter("range"));
    lookahead       = dynamic_cast<juce::AudioParameterFloat*>(valueTreeState.getParameter("lookahead"));
    bypassParamBool = dynamic_cast<juce::AudioParameterBool*>(valueTreeState.getParameter("bypass"));
    jassert(threshold && attack && hold && release && range && lookahead && bypassParamBool);
}

juce::AudioProcessorValueTreeState::ParameterLayout NoiseGateProcessor::createParameterLayout()
{
    // Same IDs, names, ranges, defaults, units and order as the pre-migration addParameter calls.
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        "threshold", "Threshold", juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -20.0f, "dB"));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        "attack", "Attack", juce::NormalisableRange<float>(0.1f, 100.0f, 0.1f, 0.5f), 1.0f, "ms"));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        "hold", "Hold", juce::NormalisableRange<float>(0.0f, 500.0f, 0.1f), 10.0f, "ms"));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        "release", "Release", juce::NormalisableRange<float>(1.0f, 5000.0f, 1.0f, 0.5f), 100.0f, "ms"));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        "range", "Range", juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -40.0f, "dB"));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        "lookahead", "Lookahead", juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f), 2.0f, "ms"));
    parameters.push_back(std::make_unique<juce::AudioParameterBool>("bypass", "Bypass", false));

    return { parameters.begin(), parameters.end() };
}

NoiseGateProcessor::~NoiseGateProcessor()
{
}

const juce::String NoiseGateProcessor::getName() const
{
    return "HyperPrism Reimagined Noise Gate";
}

bool NoiseGateProcessor::acceptsMidi() const
{
    return false;
}

bool NoiseGateProcessor::producesMidi() const
{
    return false;
}

bool NoiseGateProcessor::isMidiEffect() const
{
    return false;
}

double NoiseGateProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int NoiseGateProcessor::getNumPrograms()
{
    return 1;
}

int NoiseGateProcessor::getCurrentProgram()
{
    return 0;
}

void NoiseGateProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String NoiseGateProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void NoiseGateProcessor::changeProgramName(int index, const juce::String& newName)
{
    juce::ignoreUnused(index, newName);
}

void NoiseGateProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    // Initialize per-channel states
    const int numChannels = getTotalNumInputChannels();
    envelopeState.resize(numChannels, 0.0f);
    gateState.resize(numChannels, 0.0f);
    holdCounter.resize(numChannels, 0);
    
    // Prepare lookahead buffer
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = numChannels;
    
    lookaheadBuffer.prepare(spec);
    lookaheadBuffer.setMaximumDelayInSamples(static_cast<int>(sampleRate * 0.01)); // 10ms max

    // Pre-allocate lookahead data buffer
    lookaheadData.resize(static_cast<size_t>(samplesPerBlock) * 2);
}

void NoiseGateProcessor::releaseResources()
{
    lookaheadBuffer.reset();
}

bool NoiseGateProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void NoiseGateProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;
    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    // Clear any output channels that don't have corresponding input
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    if (numSamples == 0)
        return;

    if (bypassParamBool->get())
        return;

    // Get parameter values
    const float thresholdDb = threshold->get();
    const float thresholdLinear = dbToLinear(thresholdDb);
    const float attackMs = attack->get();
    const float holdMs = hold->get();
    const float releaseMs = release->get();
    const float rangeDb = range->get();
    const float rangeLinear = dbToLinear(rangeDb);
    const float lookaheadMs = lookahead->get();
    
    // Calculate time constants
    const float attackCoeff = 1.0f - std::exp(-1.0f / (attackMs * 0.001f * currentSampleRate));
    const float releaseCoeff = 1.0f - std::exp(-1.0f / (releaseMs * 0.001f * currentSampleRate));
    const int holdSamples = static_cast<int>(holdMs * 0.001f * currentSampleRate);
    const int lookaheadSamples = static_cast<int>(lookaheadMs * 0.001f * currentSampleRate);
    
    // Set lookahead delay
    lookaheadBuffer.setDelay(static_cast<float>(lookaheadSamples));
    
    // Process each channel
    bool anyGateOpen = false;
    
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        float* channelData = buffer.getWritePointer(channel);
        
        // Copy data for lookahead processing (pre-allocated buffer)
        jassert(lookaheadData.size() >= static_cast<size_t>(numSamples));
        std::copy(channelData, channelData + numSamples, lookaheadData.data());
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Get input level (use lookahead for detection)
            int lookaheadIndex = sample + lookaheadSamples;
            float inputLevel = 0.0f;
            
            if (lookaheadIndex < numSamples)
            {
                inputLevel = std::abs(lookaheadData[lookaheadIndex]);
            }
            else
            {
                inputLevel = std::abs(channelData[sample]);
            }
            
            // Envelope follower
            if (inputLevel > envelopeState[channel])
            {
                // Attack
                envelopeState[channel] += attackCoeff * (inputLevel - envelopeState[channel]);
            }
            else
            {
                // Release
                envelopeState[channel] += releaseCoeff * (inputLevel - envelopeState[channel]);
            }
            
            // Gate logic
            float targetGate = 0.0f;
            
            if (envelopeState[channel] > thresholdLinear)
            {
                targetGate = 1.0f;
                holdCounter[channel] = holdSamples;
            }
            else if (holdCounter[channel] > 0)
            {
                targetGate = 1.0f;
                holdCounter[channel]--;
            }
            else
            {
                targetGate = 0.0f;
            }
            
            // Smooth gate transitions
            if (targetGate > gateState[channel])
            {
                // Opening
                gateState[channel] += attackCoeff * (targetGate - gateState[channel]);
            }
            else
            {
                // Closing
                gateState[channel] += releaseCoeff * (targetGate - gateState[channel]);
            }
            
            // Apply gate
            float gateGain = rangeLinear + (1.0f - rangeLinear) * gateState[channel];
            
            // Process through lookahead buffer
            lookaheadBuffer.pushSample(channel, channelData[sample]);
            channelData[sample] = lookaheadBuffer.popSample(channel) * gateGain;
            
            // Update gate status
            if (gateState[channel] > 0.5f)
                anyGateOpen = true;
        }
    }
    
    // Update gate status for LED
    gateOpen = anyGateOpen;
}

bool NoiseGateProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* NoiseGateProcessor::createEditor()
{
    return new NoiseGateEditor(*this);
}

void NoiseGateProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = valueTreeState.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void NoiseGateProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState == nullptr)
        return;

    if (xmlState->hasTagName(valueTreeState.state.getType()))
    {
        valueTreeState.replaceState(juce::ValueTree::fromXml(*xmlState));
        return;
    }

    if (xmlState->hasTagName(legacyStateTag))
    {
        // Pre-migration session: one attribute per float parameter in real units, keyed by
        // paramID; bypass was never saved. Restore through the parameters so the APVTS
        // tree, the host and the editor all see the same values.
        const std::array<juce::RangedAudioParameter*, 6> legacyParams {
            threshold, attack, hold, release, range, lookahead };
        for (juce::RangedAudioParameter* p : legacyParams)
        {
            if (p != nullptr && xmlState->hasAttribute(p->paramID))
                p->setValueNotifyingHost(p->convertTo0to1(
                    static_cast<float>(xmlState->getDoubleAttribute(p->paramID))));
        }
    }
}

float NoiseGateProcessor::dbToLinear(float db) const
{
    return std::pow(10.0f, db / 20.0f);
}

float NoiseGateProcessor::linearToDb(float linear) const
{
    return 20.0f * std::log10(juce::jmax(0.00001f, linear));
}