//==============================================================================
// HyperPrism Revived - More Stereo Processor
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
    
    // Prepare reverb for ambience
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
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
    ambienceDelayLeft.prepare({ sampleRate, static_cast<juce::uint32>(samplesPerBlock), 1 });
    ambienceDelayRight.prepare({ sampleRate, static_cast<juce::uint32>(samplesPerBlock), 1 });
    ambienceDelayLeft.reset();
    ambienceDelayRight.reset();
    
    // Initialize crossover filters
    auto defaultLowPassCoeffs = juce::IIRCoefficients::makeLowPass(sampleRate, 120.0);
    auto defaultHighPassCoeffs = juce::IIRCoefficients::makeHighPass(sampleRate, 120.0);
    
    lowPassLeft.setCoefficients(defaultLowPassCoeffs);
    lowPassRight.setCoefficients(defaultLowPassCoeffs);
    highPassLeft.setCoefficients(defaultHighPassCoeffs);
    highPassRight.setCoefficients(defaultHighPassCoeffs);
    
    lowPassLeft.reset();
    lowPassRight.reset();
    highPassLeft.reset();
    highPassRight.reset();
    
    previousCrossoverFreq = -1.0f;
    
    // Reset metering
    leftLevel.store(0.0f);
    rightLevel.store(0.0f);
    stereoWidth.store(0.0f);
    ambienceLevel.store(0.0f);
}

void MoreStereoProcessor::releaseResources()
{
    // Reset filters
    lowPassLeft.reset();
    lowPassRight.reset();
    highPassLeft.reset();
    highPassRight.reset();
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

    if (buffer.getNumChannels() < 2)
        return;

    processMoreStereo(buffer);
    calculateStereoWidth(buffer);
}

void MoreStereoProcessor::processMoreStereo(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    
    const float width = widthParam->load() / 100.0f;
    const float bassMonoAmount = bassMonoParam->load() / 100.0f;
    const float crossoverFreq = crossoverFreqParam->load();
    const float stereoEnhance = stereoEnhanceParam->load() / 100.0f;
    const float ambienceAmount = ambienceParam->load() / 100.0f;
    const float outputLevel = juce::Decibels::decibelsToGain(outputLevelParam->load());
    
    // Update crossover filters if frequency changed
    if (std::abs(crossoverFreq - previousCrossoverFreq) > 1.0f)
    {
        auto lowPassCoeffs = juce::IIRCoefficients::makeLowPass(currentSampleRate, crossoverFreq);
        auto highPassCoeffs = juce::IIRCoefficients::makeHighPass(currentSampleRate, crossoverFreq);
        
        lowPassLeft.setCoefficients(lowPassCoeffs);
        lowPassRight.setCoefficients(lowPassCoeffs);
        highPassLeft.setCoefficients(highPassCoeffs);
        highPassRight.setCoefficients(highPassCoeffs);
        
        previousCrossoverFreq = crossoverFreq;
    }
    
    auto* leftData = buffer.getWritePointer(0);
    auto* rightData = buffer.getWritePointer(1);
    
    // Create separate buffers for processing
    juce::AudioBuffer<float> bassBuffer;
    juce::AudioBuffer<float> trebleBuffer;
    juce::AudioBuffer<float> ambienceBuffer;
    
    bassBuffer.makeCopyOf(buffer);
    trebleBuffer.makeCopyOf(buffer);
    ambienceBuffer.makeCopyOf(buffer);
    
    // Apply crossover filtering
    auto* bassLeft = bassBuffer.getWritePointer(0);
    auto* bassRight = bassBuffer.getWritePointer(1);
    auto* trebleLeft = trebleBuffer.getWritePointer(0);
    auto* trebleRight = trebleBuffer.getWritePointer(1);
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Filter bass frequencies
        bassLeft[sample] = lowPassLeft.processSingleSampleRaw(bassLeft[sample]);
        bassRight[sample] = lowPassRight.processSingleSampleRaw(bassRight[sample]);
        
        // Filter treble frequencies
        trebleLeft[sample] = highPassLeft.processSingleSampleRaw(trebleLeft[sample]);
        trebleRight[sample] = highPassRight.processSingleSampleRaw(trebleRight[sample]);
    }
    
    // Process bass frequencies (make mono if required)
    if (bassMonoAmount > 0.001f)
    {
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float mono = (bassLeft[sample] + bassRight[sample]) * 0.5f;
            float dryLeft = bassLeft[sample] * (1.0f - bassMonoAmount);
            float dryRight = bassRight[sample] * (1.0f - bassMonoAmount);
            float wetMono = mono * bassMonoAmount;
            
            bassLeft[sample] = dryLeft + wetMono;
            bassRight[sample] = dryRight + wetMono;
        }
    }
    
    // Process treble frequencies (apply stereo enhancement)
    if (stereoEnhance > 0.001f)
    {
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float mono = (trebleLeft[sample] + trebleRight[sample]) * 0.5f;
            float side = (trebleLeft[sample] - trebleRight[sample]) * 0.5f;
            
            // Enhance stereo image
            side *= (1.0f + stereoEnhance * 2.0f);
            
            trebleLeft[sample] = mono + side;
            trebleRight[sample] = mono - side;
        }
    }
    
    // Apply overall stereo width to treble
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float mono = (trebleLeft[sample] + trebleRight[sample]) * 0.5f;
        float side = (trebleLeft[sample] - trebleRight[sample]) * 0.5f * width;
        
        trebleLeft[sample] = mono + side;
        trebleRight[sample] = mono - side;
    }
    
    // Process ambience if enabled
    float ambienceLevelSum = 0.0f;
    if (ambienceAmount > 0.001f)
    {
        // Apply reverb to create ambience
        auto ambienceBlock = juce::dsp::AudioBlock<float>(ambienceBuffer);
        auto ambienceContext = juce::dsp::ProcessContextReplacing<float>(ambienceBlock);
        reverb.process(ambienceContext);
        
        // Add slight delays for width
        auto* ambienceLeft = ambienceBuffer.getWritePointer(0);
        auto* ambienceRight = ambienceBuffer.getWritePointer(1);
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float leftInput = ambienceLeft[sample];
            float rightInput = ambienceRight[sample];
            
            // Add small delays (3-7 ms)
            float leftDelayed = ambienceDelayLeft.popSample(0, 3.0f * 48.0f, true);
            float rightDelayed = ambienceDelayRight.popSample(0, 7.0f * 48.0f, true);
            
            ambienceDelayLeft.pushSample(0, leftInput);
            ambienceDelayRight.pushSample(0, rightInput);
            
            ambienceLeft[sample] = leftDelayed * ambienceAmount * 0.3f;
            ambienceRight[sample] = rightDelayed * ambienceAmount * 0.3f;
            
            ambienceLevelSum += (std::abs(ambienceLeft[sample]) + std::abs(ambienceRight[sample])) * 0.5f;
        }
    }
    
    // Combine all components
    float leftLevelSum = 0.0f;
    float rightLevelSum = 0.0f;
    
    for (int sample = 0; sample < numSamples; ++sample)
    {
        leftData[sample] = (bassLeft[sample] + trebleLeft[sample]) * outputLevel;
        rightData[sample] = (bassRight[sample] + trebleRight[sample]) * outputLevel;
        
        // Add ambience
        if (ambienceAmount > 0.001f)
        {
            leftData[sample] += ambienceBuffer.getSample(0, sample);
            rightData[sample] += ambienceBuffer.getSample(1, sample);
        }
        
        // Accumulate for metering
        leftLevelSum += std::abs(leftData[sample]);
        rightLevelSum += std::abs(rightData[sample]);
    }
    
    // Update level metering
    leftLevel.store(leftLevelSum / numSamples);
    rightLevel.store(rightLevelSum / numSamples);
    ambienceLevel.store(ambienceLevelSum / numSamples);
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
        float left = leftData[sample];
        float right = rightData[sample];
        
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
    auto xml = valueTreeState.copyState().createXml();
    copyXmlToBinary(*xml, destData);
}

void MoreStereoProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml != nullptr && xml->hasTagName(valueTreeState.state.getType()))
        valueTreeState.replaceState(juce::ValueTree::fromXml(*xml));
}