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
    juce::ignoreUnused(samplesPerBlock);
    currentSampleRate = sampleRate;
    
    // Initialize per-channel states
    const int numChannels = juce::jmax(1, getTotalNumInputChannels());
    envelopeState.assign(static_cast<size_t>(numChannels), 0.0f);
    gateState.assign(static_cast<size_t>(numChannels), 0.0f);
    holdCounter.assign(static_cast<size_t>(numChannels), 0);

    // Fixed lookahead delay = the maximum Lookahead, reported as latency.
    maxDelaySamples = static_cast<int>(std::ceil(kMaxLookaheadMs * 0.001 * sampleRate));
    ringSize = maxDelaySamples + 1;
    delayRing.assign(static_cast<size_t>(ringSize * numChannels), 0.0f);
    writePosition = 0;

    setLatencySamples(maxDelaySamples);
}

void NoiseGateProcessor::releaseResources()
{
    std::fill(delayRing.begin(), delayRing.end(), 0.0f);
    writePosition = 0;
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

    const int numChannels = juce::jmin(totalNumInputChannels, buffer.getNumChannels(),
                                       static_cast<int>(envelopeState.size()));
    if (numSamples == 0 || numChannels == 0 || delayRing.empty())
        return;

    // Bypassed audio still goes through the lookahead delay, so the timing the host
    // compensates for does not jump when Bypass is toggled.
    const bool bypassed = bypassParamBool->get();

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
    const float attackCoeff = static_cast<float>(1.0 - std::exp(-1.0 / (attackMs * 0.001 * currentSampleRate)));
    const float releaseCoeff = static_cast<float>(1.0 - std::exp(-1.0 / (releaseMs * 0.001 * currentSampleRate)));
    const int holdSamples = static_cast<int>(holdMs * 0.001f * currentSampleRate);
    const int lookaheadSamples = juce::jlimit(0, maxDelaySamples, static_cast<int>(lookaheadMs * 0.001f * currentSampleRate));
    const int detectorDelay = maxDelaySamples - lookaheadSamples; // detector leads the audio by lookaheadSamples
    
    bool anyGateOpen = false;
    int position = writePosition;

    for (int channel = 0; channel < numChannels; ++channel)
    {
        float* channelData = buffer.getWritePointer(channel);
        float* ring = delayRing.data() + static_cast<size_t>(channel * ringSize);
        auto& envelope = envelopeState[static_cast<size_t>(channel)];
        auto& gate = gateState[static_cast<size_t>(channel)];
        auto& holdLeft = holdCounter[static_cast<size_t>(channel)];
        position = writePosition;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            ring[position] = channelData[sample];
            const int audioIndex = (position + ringSize - maxDelaySamples) % ringSize;
            const float delayedAudio = ring[audioIndex];

            if (bypassed)
            {
                channelData[sample] = delayedAudio;
            }
            else
            {
                // Detector input: the signal lookaheadSamples ahead of the delayed audio
                const int detectorIndex = (position + ringSize - detectorDelay) % ringSize;
                const float inputLevel = std::abs(ring[detectorIndex]);

                // Envelope follower
                if (inputLevel > envelope)
                    envelope += attackCoeff * (inputLevel - envelope);   // Attack
                else
                    envelope += releaseCoeff * (inputLevel - envelope);  // Release

                // Gate logic
                float targetGate = 0.0f;
                if (envelope > thresholdLinear)
                {
                    targetGate = 1.0f;
                    holdLeft = holdSamples;
                }
                else if (holdLeft > 0)
                {
                    targetGate = 1.0f;
                    holdLeft--;
                }

                // Smooth gate transitions
                if (targetGate > gate)
                    gate += attackCoeff * (targetGate - gate);   // Opening
                else
                    gate += releaseCoeff * (targetGate - gate);  // Closing

                // Apply gate to the delayed audio
                const float gateGain = rangeLinear + (1.0f - rangeLinear) * gate;
                channelData[sample] = delayedAudio * gateGain;

                if (gate > 0.5f)
                    anyGateOpen = true;
            }

            if (++position == ringSize)
                position = 0;
        }
    }

    writePosition = position;

    // Update gate status for LED
    gateOpen = bypassed ? false : anyGateOpen;
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
    state.setProperty("editor_width", getEditorWidth(), nullptr);
    state.setProperty("editor_height", getEditorHeight(), nullptr);
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
        auto newTree = juce::ValueTree::fromXml(*xmlState);
        setEditorSize((int) newTree.getProperty("editor_width", 0),
                      (int) newTree.getProperty("editor_height", 0));
        newTree.removeProperty("editor_width", nullptr);
        newTree.removeProperty("editor_height", nullptr);
        valueTreeState.replaceState(newTree);
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