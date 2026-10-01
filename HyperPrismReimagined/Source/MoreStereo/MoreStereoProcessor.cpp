//==============================================================================
// HyperPrism Reimagined - More Stereo Processor
//==============================================================================

#include "MoreStereoProcessor.h"
#include "MoreStereoEditor.h"

// Parameter IDs
const juce::String MoreStereoProcessor::BYPASS_ID = "bypass";
const juce::String MoreStereoProcessor::WIDTH_ID = "width";
const juce::String MoreStereoProcessor::BASS_MONO_ID = "bassMono";
const juce::String MoreStereoProcessor::CROSSOVER_FREQ_ID = "crossoverFreq";
const juce::String MoreStereoProcessor::STEREO_ENHANCE_ID = "stereoEnhance";
const juce::String MoreStereoProcessor::AMBIENCE_ID = "ambience";
const juce::String MoreStereoProcessor::OUTPUT_LEVEL_ID = "outputLevel";

//==============================================================================
MoreStereoProcessor::MoreStereoProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      valueTreeState(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache parameter pointers for performance
    bypassParam = valueTreeState.getRawParameterValue(BYPASS_ID);
    widthParam = valueTreeState.getRawParameterValue(WIDTH_ID);
    bassMonoParam = valueTreeState.getRawParameterValue(BASS_MONO_ID);
    crossoverFreqParam = valueTreeState.getRawParameterValue(CROSSOVER_FREQ_ID);
    stereoEnhanceParam = valueTreeState.getRawParameterValue(STEREO_ENHANCE_ID);
    ambienceParam = valueTreeState.getRawParameterValue(AMBIENCE_ID);
    outputLevelParam = valueTreeState.getRawParameterValue(OUTPUT_LEVEL_ID);
}

juce::AudioProcessorValueTreeState::ParameterLayout MoreStereoProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    // Bypass
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID, "Bypass", false));

    // Stereo Width (0-300%)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        WIDTH_ID, "Stereo Width", 
        juce::NormalisableRange<float>(0.0f, 300.0f, 0.1f), 150.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + "%"; }));

    // Bass Mono (0-100%)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        BASS_MONO_ID, "Bass Mono", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 70.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + "%"; }));

    // Crossover Frequency (50Hz - 500Hz)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        CROSSOVER_FREQ_ID, "Crossover Freq", 
        juce::NormalisableRange<float>(50.0f, 500.0f, 1.0f, 0.3f), 120.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(static_cast<int>(value)) + " Hz"; }));

    // Stereo Enhancement (0-100%)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        STEREO_ENHANCE_ID, "Stereo Enhance", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 40.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + "%"; }));

    // Ambience (0-100%)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        AMBIENCE_ID, "Ambience", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 20.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + "%"; }));

    // Output Level (-20 to +20 dB)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        OUTPUT_LEVEL_ID, "Output Level", 
        juce::NormalisableRange<float>(-20.0f, 20.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    return { parameters.begin(), parameters.end() };
}

//==============================================================================
void MoreStereoProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    preparedBlockSize = juce::jmax(1, samplesPerBlock);

    // Prepare reverb for ambience
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(preparedBlockSize);
    spec.numChannels = 2;

    reverb.prepare(spec);
    reverb.reset();

    // Configure reverb parameters for subtle ambience
    juce::Reverb::Parameters reverbParams;
    reverbParams.roomSize = 0.3f;
    reverbParams.damping = 0.7f;
    reverbParams.wetLevel = 0.2f;
    reverbParams.dryLevel = 0.8f;
    reverbParams.width = 1.0f;
    reverbParams.freezeMode = 0.0f;
    reverb.setParameters(reverbParams);

    // Prepare ambience delay lines
    ambienceDelayLeft.prepare({ sampleRate, static_cast<juce::uint32>(preparedBlockSize), 1 });
    ambienceDelayRight.prepare({ sampleRate, static_cast<juce::uint32>(preparedBlockSize), 1 });
    ambienceDelayLeft.reset();
    ambienceDelayRight.reset();

    // Linkwitz-Riley crossover (low + high sums flat)
    crossover.prepare(spec);
    crossover.setCutoffFrequency(crossoverFreqParam->load());
    crossover.reset();

    // Smoothing (30 ms), starting at the current values.
    constexpr double smoothingSeconds = 0.03;
    for (auto* s : { &widthSmoothed, &bassMonoSmoothed, &stereoEnhanceSmoothed, &ambienceSmoothed })
        s->reset(sampleRate, smoothingSeconds);
    crossoverSmoothed.reset(sampleRate, smoothingSeconds);
    outputGainSmoothed.reset(sampleRate, smoothingSeconds);
    widthSmoothed.setCurrentAndTargetValue(widthParam->load() / 100.0f);
    bassMonoSmoothed.setCurrentAndTargetValue(bassMonoParam->load() / 100.0f);
    stereoEnhanceSmoothed.setCurrentAndTargetValue(stereoEnhanceParam->load() / 100.0f);
    ambienceSmoothed.setCurrentAndTargetValue(ambienceParam->load() / 100.0f);
    crossoverSmoothed.setCurrentAndTargetValue(crossoverFreqParam->load());
    outputGainSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(outputLevelParam->load()));

    // Pre-allocate the ambience (reverb input) buffer
    ambienceBuffer.setSize(2, preparedBlockSize);

    // Reset metering
    leftLevel.store(0.0f);
    rightLevel.store(0.0f);
    stereoWidth.store(0.0f);
    ambienceLevel.store(0.0f);
}

void MoreStereoProcessor::releaseResources()
{
    crossover.reset();
    reverb.reset();
}

bool MoreStereoProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void MoreStereoProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    if (bypassParam->load() > 0.5f)
        return;

    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    if (buffer.getNumChannels() < 2 || preparedBlockSize <= 0)
        return;

    widthSmoothed.setTargetValue(widthParam->load() / 100.0f);
    bassMonoSmoothed.setTargetValue(bassMonoParam->load() / 100.0f);
    stereoEnhanceSmoothed.setTargetValue(stereoEnhanceParam->load() / 100.0f);
    ambienceSmoothed.setTargetValue(ambienceParam->load() / 100.0f);
    crossoverSmoothed.setTargetValue(crossoverFreqParam->load());
    outputGainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(outputLevelParam->load()));

    // ambienceBuffer is sized for preparedBlockSize; a host that sends a bigger block gets it
    // processed in chunks of at most that size.
    float leftSum = 0.0f, rightSum = 0.0f, ambienceSum = 0.0f;
    const int numSamples = buffer.getNumSamples();
    for (int start = 0; start < numSamples; start += preparedBlockSize)
        processMoreStereo(buffer, start, juce::jmin(preparedBlockSize, numSamples - start),
                          leftSum, rightSum, ambienceSum);

    // Update level metering
    leftLevel.store(leftSum / numSamples);
    rightLevel.store(rightSum / numSamples);
    ambienceLevel.store(ambienceSum / numSamples);

    calculateStereoWidth(buffer);
}

void MoreStereoProcessor::processMoreStereo(juce::AudioBuffer<float>& buffer, int start, int numSamples,
                                            float& leftLevelSum, float& rightLevelSum, float& ambienceLevelSum)
{
    auto* leftData = buffer.getWritePointer(0, start);
    auto* rightData = buffer.getWritePointer(1, start);

    // Ambience (reverb + short delays) runs while Ambience is above zero or ramping to it.
    const bool ambienceActive = ambienceSmoothed.isSmoothing() || ambienceSmoothed.getTargetValue() > 0.001f;
    if (ambienceActive)
    {
        ambienceBuffer.copyFrom(0, 0, leftData, numSamples);
        ambienceBuffer.copyFrom(1, 0, rightData, numSamples);
        auto ambienceBlock = juce::dsp::AudioBlock<float>(ambienceBuffer).getSubBlock(0, (size_t) numSamples);
        reverb.process(juce::dsp::ProcessContextReplacing<float>(ambienceBlock));
    }
    const auto* ambienceLeft = ambienceBuffer.getReadPointer(0);
    const auto* ambienceRight = ambienceBuffer.getReadPointer(1);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        if (crossoverSmoothed.isSmoothing())
            crossover.setCutoffFrequency(crossoverSmoothed.getNextValue());

        const float width = widthSmoothed.getNextValue();
        const float bassMonoAmount = bassMonoSmoothed.getNextValue();
        const float stereoEnhance = stereoEnhanceSmoothed.getNextValue();
        const float ambienceAmount = ambienceSmoothed.getNextValue();
        const float outputGain = outputGainSmoothed.getNextValue();

        // Crossover: bass + treble == input (all-passed), so neutral settings sum flat.
        float bassL = 0.0f, trebleL = 0.0f, bassR = 0.0f, trebleR = 0.0f;
        crossover.processSample(0, leftData[sample], bassL, trebleL);
        crossover.processSample(1, rightData[sample], bassR, trebleR);

        // Bass towards mono
        const float bassMono = (bassL + bassR) * 0.5f;
        bassL = bassL * (1.0f - bassMonoAmount) + bassMono * bassMonoAmount;
        bassR = bassR * (1.0f - bassMonoAmount) + bassMono * bassMonoAmount;

        // Treble: stereo enhancement, then overall width
        float mid = (trebleL + trebleR) * 0.5f;
        float side = (trebleL - trebleR) * 0.5f * (1.0f + stereoEnhance * 2.0f);
        trebleL = mid + side;
        trebleR = mid - side;
        mid = (trebleL + trebleR) * 0.5f;
        side = (trebleL - trebleR) * 0.5f * width;
        trebleL = mid + side;
        trebleR = mid - side;

        float outL = (bassL + trebleL) * outputGain;
        float outR = (bassR + trebleR) * outputGain;

        if (ambienceActive)
        {
            // Add small delays (3-7 ms) for width
            const float leftDelayed = ambienceDelayLeft.popSample(0, 3.0f * 48.0f, true);
            const float rightDelayed = ambienceDelayRight.popSample(0, 7.0f * 48.0f, true);
            ambienceDelayLeft.pushSample(0, ambienceLeft[sample]);
            ambienceDelayRight.pushSample(0, ambienceRight[sample]);

            const float ambL = leftDelayed * ambienceAmount * 0.3f;
            const float ambR = rightDelayed * ambienceAmount * 0.3f;
            outL += ambL;
            outR += ambR;
            ambienceLevelSum += (std::abs(ambL) + std::abs(ambR)) * 0.5f;
        }

        leftData[sample] = outL;
        rightData[sample] = outR;
        leftLevelSum += std::abs(outL);
        rightLevelSum += std::abs(outR);
    }
}

void MoreStereoProcessor::calculateStereoWidth(const juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const auto* leftData = buffer.getReadPointer(0);
    const auto* rightData = buffer.getReadPointer(1);
    
    float correlationSum = 0.0f;
    float leftSquareSum = 0.0f;
    float rightSquareSum = 0.0f;
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float left = leftData[static_cast<size_t>(sample)];
        float right = rightData[static_cast<size_t>(sample)];
        
        correlationSum += left * right;
        leftSquareSum += left * left;
        rightSquareSum += right * right;
    }
    
    float denominator = std::sqrt(leftSquareSum * rightSquareSum);
    float correlation = (denominator > 0.0f) ? (correlationSum / denominator) : 0.0f;
    
    // Convert correlation to width (1.0 = mono, 0.0 = fully decorrelated)
    float width = 1.0f - std::abs(correlation);
    stereoWidth.store(width);
}

//==============================================================================
juce::AudioProcessorEditor* MoreStereoProcessor::createEditor()
{
    return new MoreStereoEditor(*this);
}

//==============================================================================
void MoreStereoProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = valueTreeState.copyState();
    state.setProperty("editor_width", getEditorWidth(), nullptr);
    state.setProperty("editor_height", getEditorHeight(), nullptr);
    auto xml = state.createXml();
    copyXmlToBinary(*xml, destData);
}

void MoreStereoProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml != nullptr && xml->hasTagName(valueTreeState.state.getType()))
    {
        auto newState = juce::ValueTree::fromXml(*xml);
        setEditorSize(static_cast<int>(newState.getProperty("editor_width", 0)),
                      static_cast<int>(newState.getProperty("editor_height", 0)));
        newState.removeProperty("editor_width", nullptr);
        newState.removeProperty("editor_height", nullptr);
        valueTreeState.replaceState(newState);
    }
}