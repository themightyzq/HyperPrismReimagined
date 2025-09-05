//==============================================================================
// HyperPrism Revived - HyperPhaser Processor Implementation
//==============================================================================

#include "HyperPhaserProcessor.h"
#include "HyperPhaserEditor.h"

HyperPhaserProcessor::HyperPhaserProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput("Input", juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
    parameters(*this, nullptr, juce::Identifier("HyperPhaser"), createParameterLayout())
{
    // Initialize smoothed values
    baseFreqSmoothed.reset(50);
    sweepRateSmoothed.reset(50);
    depthSmoothed.reset(50);
    bandwidthSmoothed.reset(50);
    feedbackSmoothed.reset(50);
    mixSmoothed.reset(50);
}

HyperPhaserProcessor::~HyperPhaserProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout HyperPhaserProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        BASE_FREQ_ID,
        "Base Frequency",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.5f),
        1000.0f,
        "Hz"));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        SWEEP_RATE_ID,
        "Sweep Rate",
        juce::NormalisableRange<float>(0.01f, 20.0f, 0.01f, 0.5f),
        0.5f,
        "Hz"));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        PEAK_NOTCH_DEPTH_ID,
        "Peak/Notch Depth",
        juce::NormalisableRange<float>(-200.0f, 200.0f, 1.0f),
        150.0f,
        "%"));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        BANDWIDTH_ID,
        "Bandwidth",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
        50.0f,
        "%"));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        FEEDBACK_ID,
        "Feedback",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
        50.0f,
        "%"));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        MIX_ID,
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
        100.0f,
        "%"));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID,
        "Bypass",
        false));

    return layout;
}

void HyperPhaserProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = static_cast<float>(sampleRate);
    
    // Reset channel states
    for (auto& channel : channelStates)
        channel.reset();
    
    // Set smoothing rates
    const double smoothingTime = 0.05; // 50ms
    baseFreqSmoothed.reset(sampleRate, smoothingTime);
    sweepRateSmoothed.reset(sampleRate, smoothingTime);
    depthSmoothed.reset(sampleRate, smoothingTime);
    bandwidthSmoothed.reset(sampleRate, smoothingTime);
    feedbackSmoothed.reset(sampleRate, smoothingTime);
    mixSmoothed.reset(sampleRate, smoothingTime);
}

void HyperPhaserProcessor::releaseResources()
{
    for (auto& channel : channelStates)
        channel.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool HyperPhaserProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}
#endif

float HyperPhaserProcessor::calculateAllpassCoefficient(float frequency)
{
    // Calculate allpass coefficient for given frequency
    // Using bilinear transform approximation
    const float tanArg = juce::MathConstants<float>::pi * frequency / currentSampleRate;
    return (1.0f - tanArg) / (1.0f + tanArg);
}

float HyperPhaserProcessor::processPeakNotchDepth(float input, float depth)
{
    // Process the peak/notch depth parameter
    // Positive values create notches, negative create peaks
    const float normalizedDepth = depth / 100.0f;
    
    if (normalizedDepth >= 0.0f)
    {
        // Notch mode: subtract phase-shifted signal
        return 1.0f - normalizedDepth;
    }
    else
    {
        // Peak mode: add phase-shifted signal with resonance
        return 1.0f + std::abs(normalizedDepth) * 2.0f;
    }
}

void HyperPhaserProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    // Clear unused output channels
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, numSamples);

    // Get parameter values
    const bool bypassed = parameters.getRawParameterValue(BYPASS_ID)->load() > 0.5f;
    if (bypassed)
        return;

    // Get current parameter values directly for real-time response
    const float baseFreq = parameters.getRawParameterValue(BASE_FREQ_ID)->load();
    const float sweepRate = parameters.getRawParameterValue(SWEEP_RATE_ID)->load();
    const float depth = parameters.getRawParameterValue(PEAK_NOTCH_DEPTH_ID)->load();
    const float bandwidth = parameters.getRawParameterValue(BANDWIDTH_ID)->load();
    const float feedback = parameters.getRawParameterValue(FEEDBACK_ID)->load() * 0.01f;
    const float mix = parameters.getRawParameterValue(MIX_ID)->load() * 0.01f;

    // Process each channel
    const int numChannels = juce::jmin(totalNumInputChannels, 2);
    
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        auto& state = channelStates[channel];
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            
            // Update LFO
            const float lfoValue = std::sin(state.lfoPhase);
            state.lfoPhase += 2.0f * juce::MathConstants<float>::pi * sweepRate / currentSampleRate;
            if (state.lfoPhase >= 2.0f * juce::MathConstants<float>::pi)
                state.lfoPhase -= 2.0f * juce::MathConstants<float>::pi;
            
            // Calculate modulated frequency
            const float modulatedFreq = baseFreq * std::pow(2.0f, lfoValue);
            
            // Get input sample
            float inputSample = channelData[sample];
            float processedSample = inputSample;
            
            // Apply allpass stages
            const float coefficient = calculateAllpassCoefficient(modulatedFreq);
            const float bandwidthFactor = 1.0f + (bandwidth / 100.0f) * 3.0f; // 1 to 4 stages based on bandwidth
            const int activeStages = static_cast<int>(bandwidthFactor * 2.0f); // 2 to 8 stages
            
            for (int stage = 0; stage < activeStages && stage < ChannelState::NUM_STAGES; ++stage)
            {
                processedSample = state.stages[stage].process(processedSample, coefficient);
            }
            
            // Apply peak/notch depth processing
            const float depthGain = processPeakNotchDepth(processedSample, depth);
            processedSample *= depthGain;
            
            // Apply feedback (with limiting for stability)
            if (feedback > 0.0f)
            {
                processedSample += processedSample * feedback;
                processedSample = juce::jlimit(-1.0f, 1.0f, processedSample);
            }
            
            // Mix dry and wet signals
            channelData[sample] = inputSample * (1.0f - mix) + processedSample * mix;
        }
    }
}

juce::AudioProcessorEditor* HyperPhaserProcessor::createEditor()
{
    return new HyperPhaserEditor(*this);
}

void HyperPhaserProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void HyperPhaserProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}