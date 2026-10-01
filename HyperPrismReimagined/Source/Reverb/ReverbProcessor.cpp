//==============================================================================
// HyperPrism Reimagined - Reverb Processor Implementation
//==============================================================================

#include "ReverbProcessor.h"
#include "ReverbEditor.h"

// Parameter IDs
const juce::String ReverbProcessor::BYPASS_ID = "bypass";
const juce::String ReverbProcessor::MIX_ID = "mix";
const juce::String ReverbProcessor::ROOM_SIZE_ID = "roomSize";
const juce::String ReverbProcessor::DAMPING_ID = "damping";
const juce::String ReverbProcessor::PRE_DELAY_ID = "preDelay";
const juce::String ReverbProcessor::WIDTH_ID = "width";
const juce::String ReverbProcessor::LOW_CUT_ID = "lowCut";
const juce::String ReverbProcessor::HIGH_CUT_ID = "highCut";

ReverbProcessor::ReverbProcessor()
     : AudioProcessor(BusesProperties()
                      .withInput("Input", juce::AudioChannelSet::stereo(), true)
                      .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
       valueTreeState(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache parameter pointers for efficient access
    bypassParam = valueTreeState.getRawParameterValue(BYPASS_ID);
    mixParam = valueTreeState.getRawParameterValue(MIX_ID);
    roomSizeParam = valueTreeState.getRawParameterValue(ROOM_SIZE_ID);
    dampingParam = valueTreeState.getRawParameterValue(DAMPING_ID);
    preDelayParam = valueTreeState.getRawParameterValue(PRE_DELAY_ID);
    widthParam = valueTreeState.getRawParameterValue(WIDTH_ID);
    lowCutParam = valueTreeState.getRawParameterValue(LOW_CUT_ID);
    highCutParam = valueTreeState.getRawParameterValue(HIGH_CUT_ID);
}

juce::AudioProcessorValueTreeState::ParameterLayout ReverbProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;
    
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID, "Bypass", false));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        MIX_ID, "Mix", 0.0f, 1.0f, 0.3f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        ROOM_SIZE_ID, "Room Size", 0.1f, 1.0f, 0.5f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        DAMPING_ID, "Damping", 0.0f, 1.0f, 0.5f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        PRE_DELAY_ID, "Pre Delay", 
        juce::NormalisableRange<float>(0.0f, 200.0f, 1.0f), 20.0f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        WIDTH_ID, "Width", 0.0f, 1.0f, 1.0f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        LOW_CUT_ID, "Low Cut", 
        juce::NormalisableRange<float>(20.0f, 2000.0f, 1.0f, 0.3f), 20.0f));
        
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        HIGH_CUT_ID, "High Cut", 
        juce::NormalisableRange<float>(200.0f, 20000.0f, 1.0f, 0.3f), 20000.0f));
    
    return { parameters.begin(), parameters.end() };
}

void ReverbProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    
    // Prepare reverb
    juce::Reverb::Parameters reverbParams;
    reverbParams.roomSize = 0.5f;
    reverbParams.damping = 0.5f;
    reverbParams.wetLevel = 1.0f;
    reverbParams.dryLevel = 0.0f;
    reverbParams.width = 1.0f;
    reverb.setParameters(reverbParams);
    reverb.setSampleRate(sampleRate);
    
    // Prepare pre-delay (max 500ms)
    maxPreDelayInSamples = static_cast<int>(sampleRate * 0.5);
    preDelayBuffer.setSize(2, maxPreDelayInSamples);
    preDelayBuffer.clear();
    preDelayWriteIndex = 0;
    preDelayTap = juce::jlimit(0, maxPreDelayInSamples - 1,
                               static_cast<int>((preDelayParam->load() / 1000.0f) * sampleRate));
    preDelayNextTap = preDelayTap;
    preDelayFade = 1.0f;
    preDelayFadeStep = static_cast<float>(1.0 / (0.03 * sampleRate));
    
    // Prepare filters
    leftLowCut.reset();
    rightLowCut.reset();
    leftHighCut.reset();
    rightHighCut.reset();
    
    // Smoothers start at the current parameter values
    mixSmoothed.reset (sampleRate, 0.03);     mixSmoothed.setCurrentAndTargetValue (mixParam->load());
    lowCutSmoothed.reset (sampleRate, 0.03);  lowCutSmoothed.setCurrentAndTargetValue (lowCutParam->load());
    highCutSmoothed.reset (sampleRate, 0.03); highCutSmoothed.setCurrentAndTargetValue (highCutParam->load());

    // Force the filter coefficients to be computed on the first block
    previousLowCutFreq = -1.0f;
    previousHighCutFreq = -1.0f;
    filterUpdateCounter = 0;

    // Host blocks larger than this are processed in chunks of this size
    preparedBlockSize = juce::jmax (1, samplesPerBlock);
    dryBuffer.setSize(juce::jmax (2, getTotalNumInputChannels()), preparedBlockSize);
    dryBuffer.clear();
}

void ReverbProcessor::releaseResources()
{
    reverb.reset();
    preDelayBuffer.clear();
}

bool ReverbProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
        
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void ReverbProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    
    // Clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
    
    // Bypass check
    if (bypassParam->load() > 0.5f)
        return;
    
    // Process reverb effect, in chunks no larger than the prepared block size
    const int numSamples = buffer.getNumSamples();
    
    for (int start = 0; start < numSamples; start += preparedBlockSize)
    {
        const int len = juce::jmin(preparedBlockSize, numSamples - start);
        juce::AudioBuffer<float> chunk(buffer.getArrayOfWritePointers(), buffer.getNumChannels(), start, len);
        processReverb(chunk);
    }
}

void ReverbProcessor::processReverb(juce::AudioBuffer<float>& buffer)
{
    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();
    
    if (numChannels < 2)
        return;
    
    // Get parameter values
    float roomSize = roomSizeParam->load();
    float damping = dampingParam->load();
    float preDelayMs = preDelayParam->load();
    
    mixSmoothed.setTargetValue(mixParam->load());
    lowCutSmoothed.setTargetValue(lowCutParam->load());
    highCutSmoothed.setTargetValue(highCutParam->load());
    
    // Width is not smoothed here: juce::Reverb already ramps its wet gains when it changes
    float width = widthParam->load();
    
    // Update reverb parameters
    juce::Reverb::Parameters reverbParams;
    reverbParams.roomSize = roomSize;
    reverbParams.damping = damping;
    reverbParams.wetLevel = 1.0f;
    reverbParams.dryLevel = 0.0f;
    reverbParams.width = width;
    reverb.setParameters(reverbParams);
    
    // Update filters if needed
    updateFilters(lowCutSmoothed.getCurrentValue(), highCutSmoothed.getCurrentValue());
    
    // Calculate pre-delay in samples
    int preDelayInSamples = static_cast<int>((preDelayMs / 1000.0f) * currentSampleRate);
    preDelayInSamples = juce::jlimit(0, maxPreDelayInSamples - 1, preDelayInSamples);
    
    // Copy the dry signal into the pre-allocated buffer (chunks never exceed its size)
    const int dryChannels = juce::jmin(numChannels, dryBuffer.getNumChannels());
    for (int channel = 0; channel < dryChannels; ++channel)
        dryBuffer.copyFrom(channel, 0, buffer, channel, 0, numSamples);
    
    // Apply pre-delay. The line is always written, so a later Pre-Delay change reads real
    // audio, and a change crossfades between the old and the new read tap (see the header).
    if (preDelayFade >= 1.0f && preDelayInSamples != preDelayTap)
    {
        preDelayNextTap = preDelayInSamples;
        preDelayFade = 0.0f;
    }

    {
        auto* leftChannel = buffer.getWritePointer(0);
        auto* rightChannel = buffer.getWritePointer(1);
        auto* preDelayLeft = preDelayBuffer.getWritePointer(0);
        auto* preDelayRight = preDelayBuffer.getWritePointer(1);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Write the current samples first, so a tap of 0 reads the input itself
            preDelayLeft[preDelayWriteIndex] = leftChannel[static_cast<size_t>(sample)];
            preDelayRight[preDelayWriteIndex] = rightChannel[static_cast<size_t>(sample)];

            const int readIndex = (preDelayWriteIndex - preDelayTap + maxPreDelayInSamples) % maxPreDelayInSamples;
            float delayedLeft = preDelayLeft[readIndex];
            float delayedRight = preDelayRight[readIndex];

            if (preDelayFade < 1.0f)
            {
                const int nextIndex = (preDelayWriteIndex - preDelayNextTap + maxPreDelayInSamples) % maxPreDelayInSamples;
                delayedLeft += preDelayFade * (preDelayLeft[nextIndex] - delayedLeft);
                delayedRight += preDelayFade * (preDelayRight[nextIndex] - delayedRight);

                preDelayFade += preDelayFadeStep;
                if (preDelayFade >= 1.0f)
                {
                    preDelayFade = 1.0f;
                    preDelayTap = preDelayNextTap;
                }
            }

            leftChannel[static_cast<size_t>(sample)] = delayedLeft;
            rightChannel[static_cast<size_t>(sample)] = delayedRight;

            // Advance write index
            preDelayWriteIndex = (preDelayWriteIndex + 1) % maxPreDelayInSamples;
        }
    }
    
    // Process reverb
    if (numChannels == 2)
    {
        reverb.processStereo(buffer.getWritePointer(0), buffer.getWritePointer(1), numSamples);
    }
    else
    {
        reverb.processMono(buffer.getWritePointer(0), numSamples);
    }
    
    // Apply filtering to wet signal
    auto* leftChannel = buffer.getWritePointer(0);
    auto* rightChannel = buffer.getWritePointer(1);
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float mix = mixSmoothed.getNextValue();
        const float lowCutFreq = lowCutSmoothed.getNextValue();
        const float highCutFreq = highCutSmoothed.getNextValue();
        
        // Re-derive the filter coefficients every 16 samples while a cutoff is moving
        if (++filterUpdateCounter >= 16)
        {
            filterUpdateCounter = 0;
            updateFilters(lowCutFreq, highCutFreq);
        }
        
        leftChannel[static_cast<size_t>(sample)] = leftLowCut.processSingleSampleRaw(leftChannel[static_cast<size_t>(sample)]);
        leftChannel[static_cast<size_t>(sample)] = leftHighCut.processSingleSampleRaw(leftChannel[static_cast<size_t>(sample)]);
        
        rightChannel[static_cast<size_t>(sample)] = rightLowCut.processSingleSampleRaw(rightChannel[static_cast<size_t>(sample)]);
        rightChannel[static_cast<size_t>(sample)] = rightHighCut.processSingleSampleRaw(rightChannel[static_cast<size_t>(sample)]);
        
        // Mix wet and dry signals
        for (int channel = 0; channel < dryChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            const float dry = dryBuffer.getSample(channel, sample);
            channelData[static_cast<size_t>(sample)] = dry + (mix * (channelData[static_cast<size_t>(sample)] - dry));
        }
    }
    
    // Make sure the filters end the block exactly on the current cutoffs
    updateFilters(lowCutSmoothed.getCurrentValue(), highCutSmoothed.getCurrentValue());
}

void ReverbProcessor::updateFilters(float lowCutFreq, float highCutFreq)
{
    // Only update if frequencies changed
    if (lowCutFreq != previousLowCutFreq || highCutFreq != previousHighCutFreq)
    {
        // High-pass filter (low cut)
        leftLowCut.setCoefficients(juce::IIRCoefficients::makeHighPass(currentSampleRate, lowCutFreq, 0.707f));
        rightLowCut.setCoefficients(juce::IIRCoefficients::makeHighPass(currentSampleRate, lowCutFreq, 0.707f));
        
        // Low-pass filter (high cut)
        leftHighCut.setCoefficients(juce::IIRCoefficients::makeLowPass(currentSampleRate, highCutFreq, 0.707f));
        rightHighCut.setCoefficients(juce::IIRCoefficients::makeLowPass(currentSampleRate, highCutFreq, 0.707f));
        
        previousLowCutFreq = lowCutFreq;
        previousHighCutFreq = highCutFreq;
    }
}

juce::AudioProcessorEditor* ReverbProcessor::createEditor()
{
    return new ReverbEditor(*this);
}

void ReverbProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = valueTreeState.copyState();
    state.setProperty("editor_width", getEditorWidth(), nullptr);
    state.setProperty("editor_height", getEditorHeight(), nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ReverbProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName(valueTreeState.state.getType()))
        {
            auto newTree = juce::ValueTree::fromXml(*xmlState);
            setEditorSize((int) newTree.getProperty("editor_width", 0),
                          (int) newTree.getProperty("editor_height", 0));
            newTree.removeProperty("editor_width", nullptr);
            newTree.removeProperty("editor_height", nullptr);
            valueTreeState.replaceState(newTree);
        }
    }
}