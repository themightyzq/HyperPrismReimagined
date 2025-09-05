#pragma once

#include <JuceHeader.h>

class BassMaximiserProcessor : public juce::AudioProcessor
{
public:
    BassMaximiserProcessor();
    ~BassMaximiserProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Public parameters
    juce::AudioParameterFloat* frequencyParam;      // 20-500 Hz cutoff frequency
    juce::AudioParameterFloat* boostParam;          // 0-20 dB bass boost
    juce::AudioParameterFloat* harmonicsParam;      // 0-100% sub-harmonic generation
    juce::AudioParameterFloat* tightnessParam;      // 0-100% compression/limiting on bass
    juce::AudioParameterFloat* outputGainParam;     // -20 to 20 dB output gain
    juce::AudioParameterBool* phaseInvertParam;     // Phase invert for bass frequencies

    // Get current bass level for metering (RMS)
    float getCurrentBassLevel() const { return currentBassLevel.load(); }
    
    // Get the AudioProcessorValueTreeState
    juce::AudioProcessorValueTreeState& getValueTreeState() { return apvts; }

private:
    // Parameter state
    juce::AudioProcessorValueTreeState apvts;
    
    // DSP members
    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;
    
    // Bass enhancement filters
    juce::dsp::IIR::Filter<float> bassFilter[2];  // Low-pass filter for bass isolation
    juce::dsp::IIR::Filter<float> highPassFilter[2];  // High-pass for everything else
    
    // Sub-harmonic generation
    juce::AudioBuffer<float> subHarmonicBuffer;
    float subHarmonicPhase[2] = {0.0f, 0.0f};
    
    // Bass compression/limiting (tightness control)
    std::vector<float> bassEnvelopes;  // One per channel
    std::vector<float> bassGainReduction;  // One per channel
    
    // Bass level metering
    std::atomic<float> currentBassLevel { 0.0f };
    juce::LinearSmoothedValue<float> bassLevelSmoother;
    
    // Output gain smoothing
    juce::LinearSmoothedValue<float> outputGainSmoother;
    
    // Helper functions
    void updateFilters();
    float generateSubHarmonic(float input, float& phase, float harmonicsAmount);
    float processBassCompression(float input, float& envelope, float& gainReduction, 
                               float tightness, float frequency);
    float calculateRMS(const float* buffer, int numSamples);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassMaximiserProcessor)
};