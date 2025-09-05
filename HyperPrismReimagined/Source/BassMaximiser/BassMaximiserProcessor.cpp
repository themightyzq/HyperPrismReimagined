#include "BassMaximiserProcessor.h"
#include "BassMaximiserEditor.h"

BassMaximiserProcessor::BassMaximiserProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor(BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
      apvts(*this, nullptr, "Parameters", 
      {
          std::make_unique<juce::AudioParameterFloat>("frequency", "Frequency", 
              juce::NormalisableRange<float>(20.0f, 500.0f, 1.0f, 0.3f), 80.0f, "Hz"),
          std::make_unique<juce::AudioParameterFloat>("boost", "Boost", 
              juce::NormalisableRange<float>(0.0f, 20.0f, 0.1f), 6.0f, "dB"),
          std::make_unique<juce::AudioParameterFloat>("harmonics", "Harmonics", 
              juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 25.0f, "%"),
          std::make_unique<juce::AudioParameterFloat>("tightness", "Tightness", 
              juce::NormalisableRange<float>(0.0f, 100.0f, 1.0f), 50.0f, "%"),
          std::make_unique<juce::AudioParameterFloat>("outputGain", "Output Gain", 
              juce::NormalisableRange<float>(-20.0f, 20.0f, 0.1f), 0.0f, "dB"),
          std::make_unique<juce::AudioParameterBool>("phaseInvert", "Phase Invert", false)
      })
{
    // Get parameter references
    frequencyParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("frequency"));
    boostParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("boost"));
    harmonicsParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("harmonics"));
    tightnessParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("tightness"));
    outputGainParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("outputGain"));
    phaseInvertParam = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("phaseInvert"));
}

BassMaximiserProcessor::~BassMaximiserProcessor()
{
}

const juce::String BassMaximiserProcessor::getName() const
{
    return JucePlugin_Name;
}

bool BassMaximiserProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool BassMaximiserProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool BassMaximiserProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double BassMaximiserProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int BassMaximiserProcessor::getNumPrograms()
{
    return 1;
}

int BassMaximiserProcessor::getCurrentProgram()
{
    return 0;
}

void BassMaximiserProcessor::setCurrentProgram(int index)
{
}

const juce::String BassMaximiserProcessor::getProgramName(int index)
{
    return {};
}

void BassMaximiserProcessor::changeProgramName(int index, const juce::String& newName)
{
}

void BassMaximiserProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;
    
    // Initialize filters
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 2;
    
    for (int ch = 0; ch < 2; ++ch)
    {
        bassFilter[ch].prepare(spec);
        highPassFilter[ch].prepare(spec);
    }
    
    updateFilters();
    
    // Initialize sub-harmonic buffer
    subHarmonicBuffer.setSize(2, samplesPerBlock);
    subHarmonicBuffer.clear();
    
    // Initialize bass processing arrays
    bassEnvelopes.resize(2, 0.0f);
    bassGainReduction.resize(2, 1.0f);
    
    // Initialize smoothers
    bassLevelSmoother.reset(sampleRate, 0.1);
    bassLevelSmoother.setCurrentAndTargetValue(0.0f);
    
    outputGainSmoother.reset(sampleRate, 0.05);
    outputGainSmoother.setCurrentAndTargetValue(juce::Decibels::decibelsToGain(outputGainParam->get()));
}

void BassMaximiserProcessor::releaseResources()
{
    subHarmonicBuffer.setSize(0, 0);
}

bool BassMaximiserProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    #if ! JucePlugin_IsSynth
     if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
         return false;
    #endif

    return true;
  #endif
}

void BassMaximiserProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Update filters if frequency changed
    updateFilters();
    
    // Get current parameter values
    float frequency = frequencyParam->get();
    float boost = boostParam->get();
    float harmonics = harmonicsParam->get() / 100.0f;
    float tightness = tightnessParam->get() / 100.0f;
    bool phaseInvert = phaseInvertParam->get();
    
    // Update output gain smoother
    outputGainSmoother.setTargetValue(juce::Decibels::decibelsToGain(outputGainParam->get()));
    
    // Clear sub-harmonic buffer
    subHarmonicBuffer.clear();
    
    float totalBassLevel = 0.0f;
    int numSamples = buffer.getNumSamples();
    
    // Process each channel
    for (int channel = 0; channel < juce::jmin(totalNumInputChannels, 2); ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        auto* subHarmonicData = subHarmonicBuffer.getWritePointer(channel);
        
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float input = channelData[sample];
            
            // Split signal into bass and high frequencies
            float bassSignal = bassFilter[channel].processSample(input);
            float highSignal = highPassFilter[channel].processSample(input);
            
            // Apply boost to bass signal
            float boostedBass = bassSignal * juce::Decibels::decibelsToGain(boost);
            
            // Generate sub-harmonics
            float subHarmonic = generateSubHarmonic(boostedBass, subHarmonicPhase[channel], harmonics);
            subHarmonicData[sample] = subHarmonic;
            
            // Apply bass compression/limiting (tightness)
            float processedBass = processBassCompression(boostedBass, bassEnvelopes[channel], 
                                                       bassGainReduction[channel], tightness, frequency);
            
            // Apply phase invert if enabled
            if (phaseInvert)
                processedBass = -processedBass;
            
            // Combine bass, sub-harmonics, and high frequencies
            float output = processedBass + (subHarmonic * harmonics) + highSignal;
            
            // Apply output gain
            output *= outputGainSmoother.getNextValue();
            
            channelData[sample] = output;
            
            // Accumulate bass level for metering (only channel 0 for stereo linking)
            if (channel == 0)
                totalBassLevel += processedBass * processedBass;
        }
    }
    
    // Update bass level meter (RMS)
    float rmsLevel = std::sqrt(totalBassLevel / numSamples);
    bassLevelSmoother.setTargetValue(rmsLevel);
    currentBassLevel.store(bassLevelSmoother.getNextValue());
}

bool BassMaximiserProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* BassMaximiserProcessor::createEditor()
{
    return new BassMaximiserEditor(*this);
}

void BassMaximiserProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void BassMaximiserProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

void BassMaximiserProcessor::updateFilters()
{
    float frequency = frequencyParam->get();
    
    // Create low-pass filter coefficients for bass isolation
    auto bassCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, frequency, 0.707f);
    
    // Create high-pass filter coefficients for everything else  
    auto highCoeffs = juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, frequency, 0.707f);
    
    for (int ch = 0; ch < 2; ++ch)
    {
        bassFilter[ch].coefficients = bassCoeffs;
        highPassFilter[ch].coefficients = highCoeffs;
    }
}

float BassMaximiserProcessor::generateSubHarmonic(float input, float& phase, float harmonicsAmount)
{
    if (harmonicsAmount <= 0.0f)
        return 0.0f;
        
    // Generate sub-harmonic at half frequency (one octave down)
    float subHarmonic = std::sin(phase) * input * 0.5f;
    
    // Update phase based on input signal's zero crossings
    // This creates a more musical sub-harmonic effect
    if ((input > 0.0f && phase < 0.0f) || (input < 0.0f && phase > 0.0f))
    {
        phase += juce::MathConstants<float>::pi / currentSampleRate * 2.0f;
    }
    
    // Keep phase in range
    if (phase > juce::MathConstants<float>::twoPi)
        phase -= juce::MathConstants<float>::twoPi;
    
    return subHarmonic;
}

float BassMaximiserProcessor::processBassCompression(float input, float& envelope, float& gainReduction, 
                                                   float tightness, float frequency)
{
    if (tightness <= 0.0f)
        return input;
    
    // Simple envelope follower
    float absInput = std::abs(input);
    float attack = 0.01f;  // Fast attack
    float release = 0.1f;  // Slower release
    
    if (absInput > envelope)
    {
        envelope += (absInput - envelope) * attack;
    }
    else
    {
        envelope += (absInput - envelope) * release;
    }
    
    // Calculate gain reduction based on envelope and tightness
    float threshold = 0.5f;  // Fixed threshold for simplicity
    float ratio = 1.0f + (tightness * 9.0f);  // 1:1 to 10:1 ratio
    
    if (envelope > threshold)
    {
        float excess = envelope - threshold;
        float compressedExcess = excess / ratio;
        float targetGain = (threshold + compressedExcess) / envelope;
        gainReduction = targetGain * tightness + (1.0f - tightness);
    }
    else
    {
        gainReduction = 1.0f;
    }
    
    return input * gainReduction;
}

float BassMaximiserProcessor::calculateRMS(const float* buffer, int numSamples)
{
    float sum = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        sum += buffer[i] * buffer[i];
    }
    return std::sqrt(sum / numSamples);
}