#pragma once

#include <JuceHeader.h>

class CompressorProcessor : public juce::AudioProcessor
{
public:
    CompressorProcessor();
    ~CompressorProcessor() override;

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

    juce::AudioProcessorValueTreeState apvts;
    
    float getGainReduction() const { return currentGainReduction.load(); }

private:
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Compression parameters
    std::atomic<float>* thresholdParam = nullptr;
    std::atomic<float>* ratioParam = nullptr;
    std::atomic<float>* attackParam = nullptr;
    std::atomic<float>* releaseParam = nullptr;
    std::atomic<float>* kneeParam = nullptr;
    std::atomic<float>* makeupGainParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    
    // Compression state
    float envelope = 0.0f;
    std::atomic<float> currentGainReduction { 0.0f };
    
    // Sample rate
    double currentSampleRate = 44100.0;
    
    // Helper functions
    float calculateAttackCoeff(float attackTimeMs);
    float calculateReleaseCoeff(float releaseTimeMs);
    float applyCompression(float inputSample, float& envelopeValue);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorProcessor)
};