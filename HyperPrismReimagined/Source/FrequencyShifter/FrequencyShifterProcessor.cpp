//==============================================================================
// HyperPrism Reimagined - Frequency Shifter Processor
//==============================================================================

#include "FrequencyShifterProcessor.h"
#include "FrequencyShifterEditor.h"

//==============================================================================
// HilbertTransform Implementation
//==============================================================================
FrequencyShifterProcessor::HilbertTransform::HilbertTransform()
{
    createHilbertCoefficients();
}

void FrequencyShifterProcessor::HilbertTransform::prepare(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 1;
    
    // Create and assign Hilbert transform coefficients
    createHilbertCoefficients();
    hilbertFilter.prepare(spec);
    *hilbertFilter.coefficients = juce::dsp::FIR::Coefficients<float>(
        hilbertCoefficients.data(), static_cast<size_t>(filterOrder));
    
    delayLine.prepare(spec);
    delayLine.setMaximumDelayInSamples(filterOrder);
    delayLine.setDelay(static_cast<float>(filterOrder / 2)); // Compensate for filter delay
    
    reset();
}

void FrequencyShifterProcessor::HilbertTransform::reset()
{
    hilbertFilter.reset();
    delayLine.reset();
}

void FrequencyShifterProcessor::HilbertTransform::processBlock(juce::AudioBuffer<float>& buffer)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            auto result = processSample(channelData[sample]);
            channelData[sample] = result.first; // Return real part
        }
    }
}

std::pair<float, float> FrequencyShifterProcessor::HilbertTransform::processSample(float input)
{
    // Process through Hilbert transform filter for imaginary component
    float imaginary = hilbertFilter.processSample(input);
    
    // Delay original signal to align with filtered signal
    delayLine.pushSample(0, input);
    float real = delayLine.popSample(0);
    
    return {real, imaginary};
}

void FrequencyShifterProcessor::HilbertTransform::createHilbertCoefficients()
{
    hilbertCoefficients.resize(filterOrder);
    
    // Create Hilbert transform coefficients
    for (int i = 0; i < filterOrder; ++i)
    {
        int n = i - filterOrder / 2;
        if (n == 0)
        {
            hilbertCoefficients[i] = 0.0f;
        }
        else if (n % 2 == 0)
        {
            hilbertCoefficients[i] = 0.0f;
        }
        else
        {
            hilbertCoefficients[i] = 2.0f / (juce::MathConstants<float>::pi * n);
        }
        
        // Apply windowing
        float window = 0.5f * (1.0f - std::cos(2.0f * juce::MathConstants<float>::pi * i / (filterOrder - 1)));
        hilbertCoefficients[i] *= window;
    }
}

//==============================================================================
// Oscillator Implementation
//==============================================================================
void FrequencyShifterProcessor::Oscillator::prepare(double sampleRate)
{
    currentSampleRate = sampleRate;
    updatePhaseIncrement();
    reset();
}

void FrequencyShifterProcessor::Oscillator::setFrequency(float newFrequency)
{
    frequency = newFrequency;
    updatePhaseIncrement();
}

void FrequencyShifterProcessor::Oscillator::reset()
{
    phase = 0.0;
}

std::pair<float, float> FrequencyShifterProcessor::Oscillator::getNextSample()
{
    float cosValue = static_cast<float>(std::cos(phase));
    float sinValue = static_cast<float>(std::sin(phase));
    
    phase += phaseIncrement;
    // Wrap both directions: phaseIncrement is negative for downward shifts, so
    // the accumulator must wrap up as well as down or it runs unbounded.
    while (phase >= juce::MathConstants<double>::twoPi)
        phase -= juce::MathConstants<double>::twoPi;
    while (phase < 0.0)
        phase += juce::MathConstants<double>::twoPi;

    return {cosValue, sinValue};
}

void FrequencyShifterProcessor::Oscillator::updatePhaseIncrement()
{
    phaseIncrement = juce::MathConstants<double>::twoPi * frequency / currentSampleRate;
}

//==============================================================================
// FrequencyShifterProcessor Implementation
//==============================================================================

// Parameter IDs
const juce::String FrequencyShifterProcessor::BYPASS_ID = "bypass";
const juce::String FrequencyShifterProcessor::FREQUENCY_SHIFT_ID = "frequencyShift";
const juce::String FrequencyShifterProcessor::FINE_SHIFT_ID = "fineShift";
const juce::String FrequencyShifterProcessor::MIX_ID = "mix";
const juce::String FrequencyShifterProcessor::OUTPUT_LEVEL_ID = "outputLevel";

//==============================================================================
FrequencyShifterProcessor::FrequencyShifterProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input",  juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      valueTreeState(*this, nullptr, "Parameters", createParameterLayout())
{
    // Cache parameter pointers for performance
    bypassParam = valueTreeState.getRawParameterValue(BYPASS_ID);
    frequencyShiftParam = valueTreeState.getRawParameterValue(FREQUENCY_SHIFT_ID);
    fineShiftParam = valueTreeState.getRawParameterValue(FINE_SHIFT_ID);
    mixParam = valueTreeState.getRawParameterValue(MIX_ID);
    outputLevelParam = valueTreeState.getRawParameterValue(OUTPUT_LEVEL_ID);
}

juce::AudioProcessorValueTreeState::ParameterLayout FrequencyShifterProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    // Bypass
    parameters.push_back(std::make_unique<juce::AudioParameterBool>(
        BYPASS_ID, "Bypass", false));

    // Frequency Shift (-5000 to +5000 Hz). Symmetric skew (0.5) concentrates
    // resolution near 0 Hz -- the musical small-shift zone (slow drift/detune/
    // phasing) gets most of the knob travel; the kHz extremes sit at the ends.
    // Note: negative shifts move DOWN (additive), not a mirror of positive; the
    // spectral fold-around-DC only appears at large negative shifts.
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        FREQUENCY_SHIFT_ID, "Frequency Shift",
        juce::NormalisableRange<float>(-5000.0f, 5000.0f, 1.0f, 0.5f, true), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " Hz"; }));

    // Fine Shift (-100 to +100 cents)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        FINE_SHIFT_ID, "Fine Shift", 
        juce::NormalisableRange<float>(-100.0f, 100.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " cents"; }));

    // Mix (0% to 100%)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        MIX_ID, "Mix", 
        juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 50.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(static_cast<int>(value)) + "%"; }));

    // Output Level (-20 to +20 dB)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        OUTPUT_LEVEL_ID, "Output Level", 
        juce::NormalisableRange<float>(-20.0f, 20.0f, 0.1f), 0.0f,
        juce::String(), juce::AudioProcessorParameter::genericParameter,
        [](float value, int) { return juce::String(value, 1) + " dB"; }));

    return { parameters.begin(), parameters.end() };
}

//==============================================================================
void FrequencyShifterProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    const int latency = HilbertTransform::getLatencySamples();

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 1;

    // Per-channel analytic-signal state (no cross-channel contamination) and a
    // matching dry-path compensation delay so dry/wet stay time-aligned.
    for (auto& h : hilbertTransforms)
        h.prepare(sampleRate, samplesPerBlock);

    for (auto& d : dryDelay)
    {
        d.prepare(spec);
        d.setMaximumDelayInSamples(latency);
        d.setDelay(static_cast<float>(latency));
        d.reset();
    }

    oscillator.prepare(sampleRate);

    // Report the analytic-path latency so the host can compensate.
    setLatencySamples(latency);

    // Reset metering
    inputLevel.store(0.0f);
    outputLevel.store(0.0f);
}

void FrequencyShifterProcessor::releaseResources()
{
    for (auto& h : hilbertTransforms)
        h.reset();
    for (auto& d : dryDelay)
        d.reset();
    oscillator.reset();
}

bool FrequencyShifterProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void FrequencyShifterProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
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

    processFrequencyShifting(buffer);
}

void FrequencyShifterProcessor::processFrequencyShifting(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    
    const float frequencyShift = frequencyShiftParam->load();
    const float fineShift = fineShiftParam->load();
    const float mix = mixParam->load() * 0.01f; // Convert percentage to 0-1
    const float outputLevelGain = juce::Decibels::decibelsToGain(outputLevelParam->load());
    
    // Calculate total frequency shift (coarse + fine)
    float totalShift = frequencyShift + (fineShift * 0.01f * frequencyShift); // Fine as percentage of coarse
    oscillator.setFrequency(totalShift);
    
    float inputLevelSum = 0.0f;
    float outputLevelSum = 0.0f;

    const int activeChannels = juce::jmin(numChannels, static_cast<int>(hilbertTransforms.size()));

    // Sample-outer / channel-inner: the oscillator is advanced once per sample
    // and the same cos/sin is applied to every channel, so all channels are
    // shifted in lock-step (no per-channel phase drift). Each channel keeps its
    // own Hilbert + delay state.
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Get oscillator values once for this time step
        auto oscValues = oscillator.getNextSample();
        const float cosShift = oscValues.first;
        const float sinShift = oscValues.second;

        for (int channel = 0; channel < activeChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);

            const float input = channelData[sample];
            inputLevelSum += std::abs(input);

            // Get analytic signal (complex representation) from this channel's
            // own Hilbert state.
            auto analyticSignal = hilbertTransforms[static_cast<size_t>(channel)].processSample(input);
            const float real = analyticSignal.first;
            const float imaginary = analyticSignal.second;

            // Delay-compensate the dry path so it aligns with the wet path
            // (both delayed by the analytic-path group delay) -> no comb filtering.
            dryDelay[static_cast<size_t>(channel)].pushSample(0, input);
            const float dry = dryDelay[static_cast<size_t>(channel)].popSample(0);

            // Frequency shift using complex multiplication
            // (real + j*imag) * (cos + j*sin) = (real*cos - imag*sin) + j*(real*sin + imag*cos)
            const float shiftedReal = real * cosShift - imaginary * sinShift;

            // Mix time-aligned dry and wet signals, then apply output level
            float output = dry * (1.0f - mix) + shiftedReal * mix;
            output *= outputLevelGain;

            channelData[sample] = output;
            outputLevelSum += std::abs(output);
        }
    }

    // Update metering
    if (activeChannels > 0)
    {
        inputLevel.store(inputLevelSum / (numSamples * activeChannels));
        outputLevel.store(outputLevelSum / (numSamples * activeChannels));
    }
}

//==============================================================================
juce::AudioProcessorEditor* FrequencyShifterProcessor::createEditor()
{
    return new FrequencyShifterEditor(*this);
}

//==============================================================================
void FrequencyShifterProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto xml = valueTreeState.copyState().createXml();
    copyXmlToBinary(*xml, destData);
}

void FrequencyShifterProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml != nullptr && xml->hasTagName(valueTreeState.state.getType()))
        valueTreeState.replaceState(juce::ValueTree::fromXml(*xml));
}