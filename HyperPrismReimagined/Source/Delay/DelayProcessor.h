//==============================================================================
// HyperPrism Revived - Delay Processor
//==============================================================================

#pragma once

#include <JuceHeader.h>

class DelayProcessor : public juce::AudioProcessor
{
public:
    DelayProcessor();
    ~DelayProcessor() override = default;

    // AudioProcessor interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    
    // Editor
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    
    // Program/State
    const juce::String getName() const override { return "HyperPrism Reimagined Delay"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const juce::String getProgramName(int index) override { return "Default"; }
    void changeProgramName(int index, const juce::String& newName) override {}
    
    // State save/restore
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    
    // Parameter access
    juce::AudioProcessorValueTreeState& getValueTreeState() { return valueTreeState; }

    // Parameter IDs
    static const juce::String BYPASS_ID;
    static const juce::String MIX_ID;
    static const juce::String DELAY_TIME_ID;
    static const juce::String FEEDBACK_ID;
    static const juce::String LOW_CUT_ID;
    static const juce::String HIGH_CUT_ID;
    static const juce::String TEMPO_SYNC_ID;
    static const juce::String STEREO_OFFSET_ID;

private:
    // Parameter layout
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Audio processing
    void processDelay(juce::AudioBuffer<float>& buffer);
    void updateFilters();
    
    // Delay line class
    class DelayLine
    {
    public:
        void prepare(double sampleRate, int maxDelayInSamples);
        void reset();
        void setDelay(float delayInSamples);
        float processSample(float input, float feedback);
        
    private:
        juce::AudioBuffer<float> buffer;
        int writeIndex = 0;
        float delayInSamples = 0.0f;
        int maxDelay = 0;
    };
    
    // State
    juce::AudioProcessorValueTreeState valueTreeState;
    
    // DSP components
    DelayLine leftDelay, rightDelay;
    juce::IIRFilter leftLowCut, rightLowCut;
    juce::IIRFilter leftHighCut, rightHighCut;
    
    // Cached parameters
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* delayTimeParam = nullptr;
    std::atomic<float>* feedbackParam = nullptr;
    std::atomic<float>* lowCutParam = nullptr;
    std::atomic<float>* highCutParam = nullptr;
    std::atomic<float>* tempoSyncParam = nullptr;
    std::atomic<float>* stereoOffsetParam = nullptr;
    
    // Processing state
    double currentSampleRate = 44100.0;
    float previousFilterFreq = -1.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DelayProcessor)
};