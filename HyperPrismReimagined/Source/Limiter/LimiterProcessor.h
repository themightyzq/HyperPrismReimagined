#pragma once

#include <JuceHeader.h>

class LimiterProcessor : public juce::AudioProcessor
{
public:
    LimiterProcessor();
    ~LimiterProcessor() override;

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
    juce::AudioParameterFloat* ceilingParam;
    juce::AudioParameterFloat* releaseParam;
    juce::AudioParameterFloat* lookaheadParam;
    juce::AudioParameterBool* softClipParam;
    juce::AudioParameterFloat* inputGainParam;

    // Get current gain reduction for metering
    float getCurrentGainReduction() const { return currentGainReduction.load(); }
    bool getPeakIndicator() const { return peakIndicator.load(); }
    void resetPeakIndicator() { peakIndicator.store(false); }
    
    // Get the AudioProcessorValueTreeState
    juce::AudioProcessorValueTreeState& getStateInformation() { return apvts; }

private:
    // Parameter state
    juce::AudioProcessorValueTreeState apvts;
    // DSP members
    double currentSampleRate = 44100.0;
    
    // Circular buffer for lookahead
    juce::AudioBuffer<float> lookaheadBuffer;
    int lookaheadWritePos = 0;
    int lookaheadSamples = 0;
    
    // Envelope followers for each channel
    std::vector<float> envelopeFollowers;
    
    // Smoothing for gain changes
    std::vector<float> smoothedGains;
    
    // Metering
    std::atomic<float> currentGainReduction { 0.0f };
    std::atomic<bool> peakIndicator { false };
    
    // Helper functions
    float processLimiting(float input, float ceiling, float& envelope, float& smoothedGain, float release);
    float softClip(float input);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LimiterProcessor)
};