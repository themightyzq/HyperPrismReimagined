#include "LimiterProcessor.h"
#include "LimiterEditor.h"

static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "ceiling", "Ceiling", 
        juce::NormalisableRange<float>(-30.0f, 0.0f, 0.1f), 
        -0.3f));
    
    // Default is 20.8 ms, not a round number: it is the release time that the old
    // hardcoded-0.999f-per-sample release coefficient (see the processBlock() fix note)
    // implied at 48 kHz, computed from the same coeff = exp(-1000 / (release_ms * fs))
    // formula the Release parameter now actually drives. Kept close to that value so a
    // freshly-created instance (no saved session) sounds the same as it did before the fix.
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "release", "Release",
        juce::NormalisableRange<float>(1.0f, 1000.0f, 0.1f, 0.5f),
        20.8f));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "lookahead", "Lookahead", 
        juce::NormalisableRange<float>(0.0f, 20.0f, 0.1f), 
        5.0f));
    
    layout.add(std::make_unique<juce::AudioParameterBool>(
        "softclip", "Soft Clip", false));
    
    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "inputgain", "Input Gain",
        juce::NormalisableRange<float>(-20.0f, 20.0f, 0.1f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        "bypass", "Bypass", false));

    return layout;
}

LimiterProcessor::LimiterProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "LimiterState", createParameterLayout())
{
    // Get parameter pointers
    ceilingParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("ceiling"));
    releaseParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("release"));
    lookaheadParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("lookahead"));
    softClipParam = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("softclip"));
    inputGainParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("inputgain"));
    bypassParam = apvts.getRawParameterValue("bypass");
}

LimiterProcessor::~LimiterProcessor()
{
}

const juce::String LimiterProcessor::getName() const
{
    return "HyperPrism Reimagined Limiter";
}

bool LimiterProcessor::acceptsMidi() const
{
    return false;
}

bool LimiterProcessor::producesMidi() const
{
    return false;
}

bool LimiterProcessor::isMidiEffect() const
{
    return false;
}

double LimiterProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int LimiterProcessor::getNumPrograms()
{
    return 1;
}

int LimiterProcessor::getCurrentProgram()
{
    return 0;
}

void LimiterProcessor::setCurrentProgram(int)
{
}

const juce::String LimiterProcessor::getProgramName(int)
{
    return {};
}

void LimiterProcessor::changeProgramName(int, const juce::String&)
{
}

void LimiterProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(samplesPerBlock);
    currentSampleRate = sampleRate;

    // Fixed lookahead delay = the maximum Lookahead, reported as latency.
    maxDelaySamples = static_cast<int>(std::ceil(kMaxLookaheadMs * sampleRate / 1000.0));
    ringSize = maxDelaySamples + 1;
    delayRing.assign(static_cast<size_t>(2 * ringSize), 0.0f);
    writePosition = 0;
    setLatencySamples(maxDelaySamples);

    // Initialize envelope followers and smoothed gains
    envelopeFollowers.assign(2, 0.0f);
    smoothedGains.assign(2, 1.0f);

    // 50ms ramp so turning the Release knob doesn't step the release-time coefficient
    // abruptly; advanced per block in processBlock() via skip(), not per sample.
    releaseMsSmoothed.reset(sampleRate, 0.05);
    releaseMsSmoothed.setCurrentAndTargetValue(releaseParam->get());

    inputGainSmoothed.reset(sampleRate, 0.03);
    ceilingSmoothed.reset(sampleRate, 0.03);
    inputGainSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(inputGainParam->get()));
    ceilingSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(ceilingParam->get()));
}

void LimiterProcessor::releaseResources()
{
    std::fill(delayRing.begin(), delayRing.end(), 0.0f);
    writePosition = 0;
}

bool LimiterProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

float LimiterProcessor::softClip(float input)
{
    // Soft clipping using tanh
    return std::tanh(input * 0.7f) / 0.7f;
}

void LimiterProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    const int numChannels = juce::jmin(totalNumInputChannels, buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    if (numChannels == 0 || numSamples == 0 || delayRing.empty())
        return;

    // Bypassed audio still goes through the lookahead delay, so the timing the host
    // compensates for does not jump when Bypass is toggled.
    if (bypassParam->load() > 0.5f)
    {
        int position = writePosition;
        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            float* ring = delayRing.data() + static_cast<size_t>(channel * ringSize);
            position = writePosition;
            for (int sample = 0; sample < numSamples; ++sample)
            {
                ring[position] = channelData[sample];
                channelData[sample] = ring[(position + ringSize - maxDelaySamples) % ringSize];
                if (++position == ringSize)
                    position = 0;
            }
        }
        writePosition = position;
        return;
    }

    // Get parameter values
    const bool useSoftClip = softClipParam->get();
    ceilingSmoothed.setTargetValue(juce::Decibels::decibelsToGain(ceilingParam->get()));
    inputGainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(inputGainParam->get()));

    // Lookahead: the gain computer leads the delayed audio by lookaheadSamples.
    const int lookaheadSamples = juce::jlimit(0, maxDelaySamples,
        static_cast<int>(lookaheadParam->get() * currentSampleRate / 1000.0));
    const int detectorDelay = maxDelaySamples - lookaheadSamples;

    // FIX (was: hard-coded 0.999f release / 0.01f attack coefficients below, so the Release
    // parameter was read above but never used -- see CHANGELOG). Release now drives the same
    // coefficient formula the previously-dead processLimiting() used
    // (coeff = exp(-1000 / (release_ms * sampleRate))), smoothed at block rate (skip()) so the
    // knob can't step it abruptly, computed once per block (no allocation, sample-rate
    // correct).
    releaseMsSmoothed.setTargetValue(releaseParam->get());
    const float smoothedReleaseMs = releaseMsSmoothed.skip(numSamples);
    const float releaseCoeff = static_cast<float>(
        std::exp(-1000.0 / (static_cast<double>(smoothedReleaseMs) * currentSampleRate)));

    // Attack: with no lookahead, the fixed near-instant 0.01 coefficient as before. With
    // lookahead, the gain falls over the lookahead window instead (to within 1 % of its target
    // by the time the peak reaches the output), so the reduction is in place without a step.
    const float attackCoeff = lookaheadSamples > 0
        ? static_cast<float>(std::exp(-4.6 / static_cast<double>(lookaheadSamples)))
        : 0.01f;

    float maxGainReduction = 1.0f;
    bool hitCeiling = false;
    int position = writePosition;

    // Per-sample ceiling and input gain (smoothed, shared by both channels).
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float ceilingLinear = ceilingSmoothed.getNextValue();
        const float inputGainLinear = inputGainSmoothed.getNextValue();

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            float* ring = delayRing.data() + static_cast<size_t>(channel * ringSize);

            // Apply input gain, then store in the lookahead ring
            ring[position] = channelData[sample] * inputGainLinear;
            const float input = ring[(position + ringSize - maxDelaySamples) % ringSize];
            const float detectorAbs = std::abs(ring[(position + ringSize - detectorDelay) % ringSize]);

            // Fast envelope follower on the look-ahead signal
            float& envelope = envelopeFollowers[static_cast<size_t>(channel)];
            if (detectorAbs > envelope)
                envelope = detectorAbs; // Instant attack
            else
                envelope = detectorAbs + releaseCoeff * (envelope - detectorAbs); // Release, Release-controlled

            // Calculate gain reduction
            float& smoothedGain = smoothedGains[static_cast<size_t>(channel)];
            const float targetGain = (envelope > ceilingLinear) ? ceilingLinear / envelope : 1.0f;

            if (targetGain < smoothedGain)
                smoothedGain = targetGain + attackCoeff * (smoothedGain - targetGain); // Attack
            else
                smoothedGain = targetGain + releaseCoeff * (smoothedGain - targetGain); // Release

            // Apply limiting to the delayed audio
            float output = input * smoothedGain;

            // Apply soft clipping if enabled
            if (useSoftClip && std::abs(output) > ceilingLinear)
                output = softClip(output / ceilingLinear) * ceilingLinear;

            // Hard clip as final safety
            output = std::max(-ceilingLinear, std::min(ceilingLinear, output));

            channelData[sample] = output;

            // Update metering
            maxGainReduction = std::min(maxGainReduction, smoothedGain);
            if (std::abs(output) >= ceilingLinear * 0.99f)
                hitCeiling = true;
        }

        if (++position == ringSize)
            position = 0;
    }

    writePosition = position;

    // Update metering values
    currentGainReduction.store(1.0f - maxGainReduction);
    if (hitCeiling)
        peakIndicator.store(true);
}

bool LimiterProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* LimiterProcessor::createEditor()
{
    return new LimiterEditor(*this);
}

void LimiterProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty("editor_width", getEditorWidth(), nullptr);
    state.setProperty("editor_height", getEditorHeight(), nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void LimiterProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
        {
            auto newState = juce::ValueTree::fromXml(*xmlState);
            setEditorSize(static_cast<int>(newState.getProperty("editor_width", 0)),
                          static_cast<int>(newState.getProperty("editor_height", 0)));
            newState.removeProperty("editor_width", nullptr);
            newState.removeProperty("editor_height", nullptr);
            apvts.replaceState(newState);
        }
}