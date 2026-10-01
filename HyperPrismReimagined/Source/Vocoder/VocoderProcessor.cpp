//==============================================================================
// HyperPrism Reimagined - Vocoder Processor
//==============================================================================

#include "VocoderProcessor.h"
#include "VocoderEditor.h"

//==============================================================================
// VocoderBand Implementation
//==============================================================================
void VocoderProcessor::VocoderBand::prepare(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 1;

    // Real second-order coefficients before reset(), so the filters' order never changes on
    // the audio thread (IIR::Filter reallocates its state when it does).
    setFrequency(1000.0f, 500.0f);
    carrierFilter.prepare(spec);
    modulatorFilter.prepare(spec);

    releaseCoeff = std::exp(-1.0f / (50.0f * 0.001f * static_cast<float>(currentSampleRate))); // 50 ms
    reset();
}

void VocoderProcessor::VocoderBand::setFrequency(float frequency, float bandwidthHz)
{
    // Band-pass at `frequency` whose -3 dB bandwidth is bandwidthHz: Q = centre / bandwidth.
    // (This used to pass the bandwidth in Hz straight in as Q, which made every band a few Hz
    // wide.) Written into each filter's existing coefficient storage: no allocation.
    const float nyquistLimit = static_cast<float>(currentSampleRate * 0.49);
    const float centre = juce::jlimit(10.0f, nyquistLimit, frequency);
    const float q = centre / juce::jmax(1.0f, bandwidthHz);
    const auto coefficients = juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass(currentSampleRate, centre, q);

    *carrierFilter.coefficients = coefficients;
    *modulatorFilter.coefficients = coefficients;
}

void VocoderProcessor::VocoderBand::reset()
{
    carrierFilter.reset();
    modulatorFilter.reset();
    envelopeLevel = 0.0f;
    processedCarrier = 0.0f;
}

float VocoderProcessor::VocoderBand::processCarrier(float carrierSample)
{
    processedCarrier = carrierFilter.processSample(carrierSample);
    return processedCarrier;
}

float VocoderProcessor::VocoderBand::processModulator(float modulatorSample)
{
    // Filter the modulator signal
    float filteredModulator = modulatorFilter.processSample(modulatorSample);
    
    // Extract envelope (rectify and smooth)
    float rectified = std::abs(filteredModulator);
    
    // Envelope follower with attack/release
    if (rectified > envelopeLevel)
    {
        // Fast attack
        envelopeLevel = rectified + (envelopeLevel - rectified) * 0.1f;
    }
    else
    {
        // Slower release
        envelopeLevel = rectified + (envelopeLevel - rectified) * releaseCoeff;
    }
    
    return envelopeLevel;
}

float VocoderProcessor::VocoderBand::getOutput()
{
    // Apply modulator envelope to carrier
    return processedCarrier * envelopeLevel;
}


//==============================================================================
// CarrierOscillator Implementation
//==============================================================================
void VocoderProcessor::CarrierOscillator::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    updatePhaseIncrement();
    reset();
}

void VocoderProcessor::CarrierOscillator::setFrequency(float newFrequency)
{
    frequency = newFrequency;
    updatePhaseIncrement();
}

void VocoderProcessor::CarrierOscillator::reset()
{
    phase = 0.0;
}

float VocoderProcessor::CarrierOscillator::getNextSample()
{
    // Generate sawtooth wave for rich harmonic content
    float output = static_cast<float>(2.0 * (phase / juce::MathConstants<double>::twoPi) - 1.0);
    
    phase += phaseIncrement;
    if (phase >= juce::MathConstants<double>::twoPi)
        phase -= juce::MathConstants<double>::twoPi;
    
    return output;
}

void VocoderProcessor::CarrierOscillator::updatePhaseIncrement()
{
    phaseIncrement = juce::MathConstants<double>::twoPi * frequency / currentSampleRate;
}

//==============================================================================
// VocoderProcessor Implementation
//==============================================================================

// Parameter IDs
const juce::String VocoderProcessor::BYPASS_ID = "bypass";
const juce::String VocoderProcessor::CARRIER_FREQ_ID = "carrierFreq";
const juce::String VocoderProcessor::MODULATOR_GAIN_ID = "modulatorGain";
const juce::String VocoderProcessor::BAND_COUNT_ID = "bandCount";
const juce::String VocoderProcessor::RELEASE_TIME_ID = "releaseTime";
const juce::String VocoderProcessor::OUTPUT_LEVEL_ID = "outputLevel";

//==============================================================================
VocoderProcessor::VocoderProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      valueTreeState(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache parameter pointers for performance
    bypassParam = valueTreeState.getRawParameterValue(BYPASS_ID);
    carrierFreqParam = valueTreeState.getRawParameterValue(CARRIER_FREQ_ID);
    modulatorGainParam = valueTreeState.getRawParameterValue(MODULATOR_GAIN_ID);
    bandCountParam = valueTreeState.getRawParameterValue(BAND_COUNT_ID);
    releaseTimeParam = valueTreeState.getRawParameterValue(RELEASE_TIME_ID);
    outputLevelParam = valueTreeState.getRawParameterValue(OUTPUT_LEVEL_ID);
    
    // Initialize band levels for metering
    bandLevels.resize(maxBands, 0.0f);
    bandLevelSums.resize(maxBands, 0.0f);
}

juce::AudioProcessorValueTreeState::ParameterLayout VocoderProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    // Bypass
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID, "Bypass", false));

    // Carrier Frequency (50 to 2000 Hz)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        CARRIER_FREQ_ID, "Carrier Frequency", 
        juce::NormalisableRange<float>(50.0f, 2000.0f, 1.0f, 0.3f), 220.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " Hz"; }));

    // Modulator Gain (-20 to +20 dB)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        MODULATOR_GAIN_ID, "Modulator Gain", 
        juce::NormalisableRange<float>(-20.0f, 20.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    // Band Count (4 to 16)
    parameters.push_back(std::make_unique<juce::AudioParameterInt>(
        BAND_COUNT_ID, "Band Count", 4, 16, 8));

    // Release Time (10 to 500 ms)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        RELEASE_TIME_ID, "Release Time", 
        juce::NormalisableRange<float>(10.0f, 500.0f, 1.0f, 0.3f), 50.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 0) + " ms"; }));

    // Output Level (-20 to +20 dB)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        OUTPUT_LEVEL_ID, "Output Level", 
        juce::NormalisableRange<float>(-20.0f, 20.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    return { parameters.begin(), parameters.end() };
}

//==============================================================================
void VocoderProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // Prepare DSP components with actual buffer size (fixes 512-sample artifact bug), then
    // build the band filters at this sample rate (they used to be built once, at 44.1 kHz,
    // in the constructor, and never rebuilt).
    for (auto& channelBands : vocoderBands)
        for (auto& band : channelBands)
            band.prepare(sampleRate, samplesPerBlock);

    currentBandCount = juce::jlimit(4, maxBands, static_cast<int>(bandCountParam->load()));
    setupVocoderBands();

    for (auto& channelBands : vocoderBands)
        for (auto& band : channelBands)
            band.reset();

    for (auto& oscillator : carrierOscillators)
    {
        oscillator.prepare(sampleRate);
        oscillator.setFrequency(carrierFreqParam->load());
    }

    modulatorGainSmoothed.reset(sampleRate, 0.03);
    outputGainSmoothed.reset(sampleRate, 0.03);
    modulatorGainSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(modulatorGainParam->load()));
    outputGainSmoothed.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(outputLevelParam->load()));

    // Reset metering
    carrierLevel.store(0.0f);
    modulatorLevel.store(0.0f);
    outputLevel.store(0.0f);
    std::fill(bandLevels.begin(), bandLevels.end(), 0.0f);
}

void VocoderProcessor::releaseResources()
{
    for (auto& channelBands : vocoderBands)
        for (auto& band : channelBands)
            band.reset();

    for (auto& oscillator : carrierOscillators)
        oscillator.reset();
}

bool VocoderProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void VocoderProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;
    
    if (bypassParam->load() > 0.5f)
        return;
        
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    if (buffer.getNumChannels() < 1)
        return;

    processVocoding(buffer);
}

void VocoderProcessor::processVocoding(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin(buffer.getNumChannels(), kMaxChannels);
    if (numSamples == 0 || numChannels == 0)
        return;

    const float carrierFreq = carrierFreqParam->load();
    const int bandCount = juce::jlimit(4, maxBands, static_cast<int>(bandCountParam->load()));
    const float releaseTime = releaseTimeParam->load();
    modulatorGainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(modulatorGainParam->load()));
    outputGainSmoothed.setTargetValue(juce::Decibels::decibelsToGain(outputLevelParam->load()));

    // Update band count if changed (allocation-free: the banks are sized for maxBands)
    if (bandCount != currentBandCount)
    {
        currentBandCount = bandCount;
        setupVocoderBands();
    }

    // Update carrier frequency (phase stays continuous)
    for (auto& oscillator : carrierOscillators)
        oscillator.setFrequency(carrierFreq);

    // Update release time for all bands
    const float releaseCoeff = std::exp(-1.0f / (releaseTime * 0.001f * static_cast<float>(currentSampleRate)));
    for (auto& channelBands : vocoderBands)
        for (int i = 0; i < currentBandCount; ++i)
            channelBands[static_cast<size_t>(i)].setReleaseCoefficient(releaseCoeff);

    float carrierLevelSum = 0.0f;
    float modulatorLevelSum = 0.0f;
    float outputLevelSum = 0.0f;

    // Reset band level accumulation (pre-allocated buffer)
    std::fill(bandLevelSums.begin(), bandLevelSums.end(), 0.0f);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float modulatorGain = modulatorGainSmoothed.getNextValue();
        const float outputGain = outputGainSmoothed.getNextValue();

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            auto& bands = vocoderBands[static_cast<size_t>(channel)];

            // Use input as modulator
            const float modulator = channelData[sample] * modulatorGain;
            modulatorLevelSum += std::abs(modulator);

            // Generate carrier signal (this channel's own oscillator)
            const float carrier = carrierOscillators[static_cast<size_t>(channel)].getNextSample();
            carrierLevelSum += std::abs(carrier);

            // Process through vocoder bands
            float output = 0.0f;
            for (int i = 0; i < currentBandCount; ++i)
            {
                auto& band = bands[static_cast<size_t>(i)];
                band.processCarrier(carrier);
                band.processModulator(modulator);
                output += band.getOutput();

                // Accumulate band levels for metering
                bandLevelSums[static_cast<size_t>(i)] += band.getEnvelopeLevel();
            }

            output *= outputGain;
            channelData[sample] = output;
            outputLevelSum += std::abs(output);
        }
    }

    // Update metering
    carrierLevel.store(carrierLevelSum / static_cast<float>(numSamples * numChannels));
    modulatorLevel.store(modulatorLevelSum / static_cast<float>(numSamples * numChannels));
    outputLevel.store(outputLevelSum / static_cast<float>(numSamples * numChannels));

    // Update band levels
    for (int i = 0; i < maxBands; ++i)
    {
        if (i < currentBandCount)
            bandLevels[static_cast<size_t>(i)] = bandLevelSums[static_cast<size_t>(i)] / static_cast<float>(numSamples);
        else
            bandLevels[static_cast<size_t>(i)] = 0.0f;
    }
}

void VocoderProcessor::setupVocoderBands()
{
    const int count = juce::jlimit(4, maxBands, currentBandCount);

    // Logarithmically spaced band frequencies
    const float minFreq = 80.0f;   // Lowest band frequency
    const float maxFreq = 8000.0f; // Highest band frequency

    for (int i = 0; i < count; ++i)
    {
        float ratio = static_cast<float>(i) / static_cast<float>(count - 1);
        bandFrequencies[static_cast<size_t>(i)] = minFreq * std::pow(maxFreq / minFreq, ratio);
    }

    // Setup each band with appropriate frequency and bandwidth (in Hz)
    for (int i = 0; i < count; ++i)
    {
        const float centerFreq = bandFrequencies[static_cast<size_t>(i)];
        float bandwidth;

        if (i == 0)
            bandwidth = (bandFrequencies[1] - centerFreq) * 0.8f;                       // First band
        else if (i == count - 1)
            bandwidth = (centerFreq - bandFrequencies[static_cast<size_t>(i - 1)]) * 0.8f; // Last band
        else
            bandwidth = (bandFrequencies[static_cast<size_t>(i + 1)] - bandFrequencies[static_cast<size_t>(i - 1)]) * 0.4f;

        for (auto& channelBands : vocoderBands)
            channelBands[static_cast<size_t>(i)].setFrequency(centerFreq, bandwidth);
    }
}

//==============================================================================
juce::AudioProcessorEditor* VocoderProcessor::createEditor()
{
    return new VocoderEditor(*this);
}

//==============================================================================
void VocoderProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = valueTreeState.copyState();
    state.setProperty("editor_width", getEditorWidth(), nullptr);
    state.setProperty("editor_height", getEditorHeight(), nullptr);
    auto xml = state.createXml();
    copyXmlToBinary(*xml, destData);
}

void VocoderProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml != nullptr && xml->hasTagName(valueTreeState.state.getType()))
    {
        auto newTree = juce::ValueTree::fromXml(*xml);
        setEditorSize((int) newTree.getProperty("editor_width", 0),
                      (int) newTree.getProperty("editor_height", 0));
        newTree.removeProperty("editor_width", nullptr);
        newTree.removeProperty("editor_height", nullptr);
        valueTreeState.replaceState(newTree);
    }
}