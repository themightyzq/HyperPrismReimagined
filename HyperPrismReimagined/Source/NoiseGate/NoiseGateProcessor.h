#pragma once

#include <JuceHeader.h>

class NoiseGateProcessor : public juce::AudioProcessor
{
public:
    NoiseGateProcessor();
    ~NoiseGateProcessor() override;

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

    // Parameters
    juce::AudioParameterFloat* threshold;
    juce::AudioParameterFloat* attack;
    juce::AudioParameterFloat* hold;
    juce::AudioParameterFloat* release;
    juce::AudioParameterFloat* range;
    juce::AudioParameterFloat* lookahead;
    
    // Value Tree State (for proper parameter management)
    juce::AudioProcessorValueTreeState* getValueTreeState() { return nullptr; }
    
    // Get gate status for LED
    bool isGateOpen() const { return gateOpen; }

private:
    // DSP members
    double currentSampleRate;
    
    // Envelope follower state per channel
    std::vector<float> envelopeState;
    
    // Gate state per channel
    std::vector<float> gateState;
    std::vector<int> holdCounter;
    
    // Lookahead buffer
    juce::dsp::DelayLine<float> lookaheadBuffer;
    
    // Gate status
    std::atomic<bool> gateOpen;
    
    // Helper functions
    float dbToLinear(float db) const;
    float linearToDb(float linear) const;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoiseGateProcessor)
};