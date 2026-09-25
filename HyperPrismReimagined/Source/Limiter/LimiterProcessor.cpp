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
    currentSampleRate = sampleRate;
    
    // Calculate maximum lookahead samples needed
    int maxLookaheadSamples = static_cast<int>(std::ceil(20.0 * sampleRate / 1000.0));
    
    // Prepare lookahead buffer
    lookaheadBuffer.setSize(2, maxLookaheadSamples + samplesPerBlock);
    lookaheadBuffer.clear();
    lookaheadWritePos = 0;
    
    // Initialize envelope followers and smoothed gains
    envelopeFollowers.resize(2, 0.0f);
    smoothedGains.resize(2, 1.0f);

    // 50ms ramp so turning the Release knob doesn't step the release-time coefficient
    // abruptly; advanced per block in processBlock() via skip(), not per sample.
    releaseMsSmoothed.reset(sampleRate, 0.05);
    releaseMsSmoothed.setCurrentAndTargetValue(releaseParam->get());
}

void LimiterProcessor::releaseResources()
{
    lookaheadBuffer.clear();
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

    if (bypassParam->load() > 0.5f)
        return;

    // Get parameter values
    float ceilingDB = ceilingParam->get();
    float ceilingLinear = juce::Decibels::decibelsToGain(ceilingDB);
    float releaseTime = releaseParam->get();
    float lookaheadMs = lookaheadParam->get();
    bool useSoftClip = softClipParam->get();
    float inputGainDB = inputGainParam->get();
    float inputGainLinear = juce::Decibels::decibelsToGain(inputGainDB);

    // Calculate lookahead samples
    lookaheadSamples = static_cast<int>(lookaheadMs * currentSampleRate / 1000.0);

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();

    // FIX (was: hard-coded 0.999f release / 0.01f attack coefficients below, so the Release
    // parameter was read above but never used -- see CHANGELOG). Release now drives the same
    // coefficient formula the previously-dead processLimiting() used
    // (coeff = exp(-1000 / (release_ms * sampleRate))), smoothed at block rate (skip()) so the
    // knob can't step it abruptly, computed once per block (no allocation, sample-rate
    // correct). The 0.1ms fixed attack is unchanged -- there is no Attack parameter, and an
    // effectively-instant attack is the intended behaviour for a limiter.
    releaseMsSmoothed.setTargetValue(releaseTime);
    const float smoothedReleaseMs = releaseMsSmoothed.skip(numSamples);
    const float releaseCoeff = static_cast<float>(
        std::exp(-1000.0 / (static_cast<double>(smoothedReleaseMs) * currentSampleRate)));
    constexpr float attackCoeff = 0.01f; // fixed ~0.1ms-equivalent fast attack, unchanged

    float maxGainReduction = 1.0f;
    bool hitCeiling = false;
    
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Apply input gain
            float input = channelData[static_cast<size_t>(sample)] * inputGainLinear;
            
            // Simplified limiting without expensive lookahead loop
            // Use immediate input instead of complex lookahead processing
            float inputAbs = std::abs(input);
            
            // Fast envelope follower
            float& envelope = envelopeFollowers[static_cast<size_t>(channel)];
            if (inputAbs > envelope)
                envelope = inputAbs; // Instant attack
            else
                envelope = inputAbs + releaseCoeff * (envelope - inputAbs); // Release, now Release-controlled

            // Calculate gain reduction
            float& smoothedGain = smoothedGains[static_cast<size_t>(channel)];
            float targetGain = (envelope > ceilingLinear) ? ceilingLinear / envelope : 1.0f;

            // Simple gain smoothing
            if (targetGain < smoothedGain)
                smoothedGain = targetGain + attackCoeff * (smoothedGain - targetGain); // Fast attack, fixed
            else
                smoothedGain = targetGain + releaseCoeff * (smoothedGain - targetGain); // Release, now Release-controlled
            
            // Apply limiting to original input (no delay for performance)
            float output = input * smoothedGain;
            
            // Apply soft clipping if enabled
            if (useSoftClip && std::abs(output) > ceilingLinear)
            {
                output = softClip(output / ceilingLinear) * ceilingLinear;
            }
            
            // Hard clip as final safety
            output = std::max(-ceilingLinear, std::min(ceilingLinear, output));
            
            channelData[static_cast<size_t>(sample)] = output;
            
            // Update metering
            maxGainReduction = std::min(maxGainReduction, smoothedGain);
            if (std::abs(output) >= ceilingLinear * 0.99f)
                hitCeiling = true;
        }
    }
    
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