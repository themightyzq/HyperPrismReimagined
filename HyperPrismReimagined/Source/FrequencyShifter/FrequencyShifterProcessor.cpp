//==============================================================================
// HyperPrism Revived - Frequency Shifter Processor
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

void FrequencyShifterProcessor::HilbertTransform::prepare(double sampleRate)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = 512;
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
    if (phase >= juce::MathConstants<double>::twoPi)
        phase -= juce::MathConstants<double>::twoPi;
    
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

    // Frequency Shift (-2000 to +2000 Hz)
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        FREQUENCY_SHIFT_ID, "Frequency Shift", 
        juce::NormalisableRange<float>(-2000.0f, 2000.0f, 1.0f), 0.0f,
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
    juce::ignoreUnused(samplesPerBlock);
    
    // Prepare DSP components
    hilbertTransform.prepare(sampleRate);
    oscillator.prepare(sampleRate);
    
    // Reset metering
    inputLevel.store(0.0f);
    outputLevel.store(0.0f);
}

void FrequencyShifterProcessor::releaseResources()
{
    hilbertTransform.reset();
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
    
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float input = channelData[sample];
            inputLevelSum += std::abs(input);
            
            // Get analytic signal (complex representation)
            auto analyticSignal = hilbertTransform.processSample(input);
            float real = analyticSignal.first;
            float imaginary = analyticSignal.second;
            
            // Get oscillator values
            auto oscValues = oscillator.getNextSample();
            float cosShift = oscValues.first;
            float sinShift = oscValues.second;
            
            // Frequency shift using complex multiplication
            // (real + j*imag) * (cos + j*sin) = (real*cos - imag*sin) + j*(real*sin + imag*cos)
            float shiftedReal = real * cosShift - imaginary * sinShift;
            
            // Mix dry and wet signals
            float output = input * (1.0f - mix) + shiftedReal * mix;
            
            // Apply output level
            output *= outputLevelGain;
            
            channelData[sample] = output;
            outputLevelSum += std::abs(output);
        }
    }
    
    // Update metering
    inputLevel.store(inputLevelSum / (numSamples * numChannels));
    outputLevel.store(outputLevelSum / (numSamples * numChannels));
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