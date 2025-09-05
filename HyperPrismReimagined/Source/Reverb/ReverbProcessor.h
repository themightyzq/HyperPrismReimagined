//==============================================================================
// HyperPrism Revived - Reverb Processor
//==============================================================================

#pragma once

#include <JuceHeader.h>

class ReverbProcessor : public juce::AudioProcessor
{
public:
    ReverbProcessor();
    ~ReverbProcessor() override = default;

    // AudioProcessor interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    
    // Editor
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    
    // Program/State
    const juce::String getName() const override { return "HyperPrism Reimagined Reverb"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }
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
    static const juce::String ROOM_SIZE_ID;
    static const juce::String DAMPING_ID;
    static const juce::String PRE_DELAY_ID;
    static const juce::String WIDTH_ID;
    static const juce::String LOW_CUT_ID;
    static const juce::String HIGH_CUT_ID;

private:
    // Parameter layout
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Audio processing
    void processReverb(juce::AudioBuffer<float>& buffer);
    void updateFilters();
    
    // State
    juce::AudioProcessorValueTreeState valueTreeState;
    
    // DSP components
    juce::Reverb reverb;
    juce::IIRFilter leftLowCut, rightLowCut;
    juce::IIRFilter leftHighCut, rightHighCut;
    
    // Pre-delay
    juce::AudioBuffer<float> preDelayBuffer;
    int preDelayWriteIndex = 0;
    int maxPreDelayInSamples = 0;
    
    // Cached parameters
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* roomSizeParam = nullptr;
    std::atomic<float>* dampingParam = nullptr;
    std::atomic<float>* preDelayParam = nullptr;
    std::atomic<float>* widthParam = nullptr;
    std::atomic<float>* lowCutParam = nullptr;
    std::atomic<float>* highCutParam = nullptr;
    
    // Processing state
    double currentSampleRate = 44100.0;
    float previousFilterFreq = -1.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ReverbProcessor)
};