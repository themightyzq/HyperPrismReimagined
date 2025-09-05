//==============================================================================
// HyperPrism Revived - Vibrato Processor
//==============================================================================

#pragma once

#include <JuceHeader.h>

class VibratoProcessor : public juce::AudioProcessor
{
public:
    VibratoProcessor();
    ~VibratoProcessor() override = default;

    // AudioProcessor interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    
    // Editor
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    
    // Program/State
    const juce::String getName() const override { return "HyperPrism Reimagined Vibrato"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 1.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int /*index*/) override {}
    const juce::String getProgramName(int /*index*/) override { return "Default"; }
    void changeProgramName(int /*index*/, const juce::String& /*newName*/) override {}
    
    // State save/restore
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    
    // Parameter access
    juce::AudioProcessorValueTreeState& getValueTreeState() { return valueTreeState; }

    // Parameter IDs
    static const juce::String BYPASS_ID;
    static const juce::String MIX_ID;
    static const juce::String RATE_ID;
    static const juce::String DEPTH_ID;
    static const juce::String DELAY_ID;
    static const juce::String FEEDBACK_ID;

private:
    // Parameter layout
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Audio processing
    void processVibrato(juce::AudioBuffer<float>& buffer);
    
    // Vibrato delay line class
    class VibratoDelayLine
    {
    public:
        void prepare(double sampleRate, float maxDelayMs);
        void reset();
        float processSample(float input, float delayMs, float feedback);
        
    private:
        juce::AudioBuffer<float> buffer;
        int writeIndex = 0;
        int maxDelaySamples = 0;
        double sampleRate = 44100.0;
    };
    
    // State
    juce::AudioProcessorValueTreeState valueTreeState;
    
    // DSP components
    VibratoDelayLine leftDelayLine, rightDelayLine;
    
    // LFO for modulation
    float lfoPhase = 0.0f;
    
    // Cached parameters
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* rateParam = nullptr;
    std::atomic<float>* depthParam = nullptr;
    std::atomic<float>* delayParam = nullptr;
    std::atomic<float>* feedbackParam = nullptr;
    
    // Processing state
    double currentSampleRate = 44100.0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VibratoProcessor)
};