//==============================================================================
// HyperPrism Revived - Echo Processor Implementation
//==============================================================================

#include "EchoProcessor.h"
#include "EchoEditor.h"

EchoProcessor::EchoProcessor()
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
    parameters(*this, nullptr, juce::Identifier("Echo"), createParameterLayout())
{
    // Initialize smoothed values
    delaySmoothed.reset(50);
    feedbackSmoothed.reset(50);
    mixSmoothed.reset(50);
}

EchoProcessor::~EchoProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout EchoProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        DELAY_ID,
        "Delay",
        juce::NormalisableRange<float>(0.0f, 2000.0f, 1.0f),
        250.0f,
        "ms"));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        FEEDBACK_ID,
        "Feedback",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
        30.0f,
        "%"));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        MIX_ID,
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f),
        50.0f,
        "%"));

    layout.add(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID,
        "Bypass",
        false));

    return layout;
}

void EchoProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = static_cast<float>(sampleRate);
    
    // Prepare delay lines
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 1;
    
    delayLineLeft.prepare(spec);
    delayLineRight.prepare(spec);
    
    delayLineLeft.reset();
    delayLineRight.reset();
    
    // Set smoothing rates
    const double smoothingTime = 0.05; // 50ms
    delaySmoothed.reset(sampleRate, smoothingTime);
    feedbackSmoothed.reset(sampleRate, smoothingTime);
    mixSmoothed.reset(sampleRate, smoothingTime);
}

void EchoProcessor::releaseResources()
{
    delayLineLeft.reset();
    delayLineRight.reset();
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool EchoProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}
#endif

void EchoProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
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

    // Update smoothed parameters
    delaySmoothed.setTargetValue(parameters.getRawParameterValue(DELAY_ID)->load());
    feedbackSmoothed.setTargetValue(parameters.getRawParameterValue(FEEDBACK_ID)->load() * 0.01f);
    mixSmoothed.setTargetValue(parameters.getRawParameterValue(MIX_ID)->load() * 0.01f);

    // Process each channel
    const int numChannels = juce::jmin(totalNumInputChannels, totalNumOutputChannels);
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float currentDelay = delaySmoothed.getNextValue();
        const float currentFeedback = feedbackSmoothed.getNextValue();
        const float currentMix = mixSmoothed.getNextValue();
        
        // Convert delay time to samples
        const float delaySamples = (currentDelay / 1000.0f) * currentSampleRate;
        
        // Process left channel
        if (numChannels > 0)
        {
            float* channelData = buffer.getWritePointer(0);
            
            // Read from delay line
            float delayedSample = delayLineLeft.popSample(0, delaySamples, true);
            
            // Apply feedback
            float inputWithFeedback = channelData[sample] + (delayedSample * currentFeedback);
            
            // Push to delay line
            delayLineLeft.pushSample(0, inputWithFeedback);
            
            // Mix dry and wet signals
            channelData[sample] = channelData[sample] * (1.0f - currentMix) + delayedSample * currentMix;
        }
        
        // Process right channel
        if (numChannels > 1)
        {
            float* channelData = buffer.getWritePointer(1);
            
            // Read from delay line
            float delayedSample = delayLineRight.popSample(0, delaySamples, true);
            
            // Apply feedback
            float inputWithFeedback = channelData[sample] + (delayedSample * currentFeedback);
            
            // Push to delay line
            delayLineRight.pushSample(0, inputWithFeedback);
            
            // Mix dry and wet signals
            channelData[sample] = channelData[sample] * (1.0f - currentMix) + delayedSample * currentMix;
        }
    }
}

juce::AudioProcessorEditor* EchoProcessor::createEditor()
{
    return new EchoEditor(*this);
}

void EchoProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void EchoProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}