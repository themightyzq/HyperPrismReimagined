//==============================================================================
// HyperPrism Revived - Single Delay Processor
//==============================================================================

#pragma once

#include <JuceHeader.h>

class SingleDelayProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    SingleDelayProcessor();
    ~SingleDelayProcessor() override = default;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return "HyperPrism Reimagined Single Delay"; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    
    //==============================================================================
    juce::AudioProcessorValueTreeState& getValueTreeState() { return valueTreeState; }
    
    // Parameter IDs
    static const juce::String BYPASS_ID;
    static const juce::String DELAY_TIME_ID;
    static const juce::String FEEDBACK_ID;
    static const juce::String WETDRY_MIX_ID;
    static const juce::String HIGH_CUT_ID;
    static const juce::String LOW_CUT_ID;
    static const juce::String STEREO_SPREAD_ID;
    
    // Metering
    float getInputLevel() const { return inputLevel.load(); }
    float getOutputLevel() const { return outputLevel.load(); }

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void processDelay(juce::AudioBuffer<float>& buffer);
    void updateFilters();
    
    juce::AudioProcessorValueTreeState valueTreeState;
    
    // Parameter pointers for performance
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* delayTimeParam = nullptr;
    std::atomic<float>* feedbackParam = nullptr;
    std::atomic<float>* wetDryMixParam = nullptr;
    std::atomic<float>* highCutParam = nullptr;
    std::atomic<float>* lowCutParam = nullptr;
    std::atomic<float>* stereoSpreadParam = nullptr;
    
    // DSP components
    juce::dsp::DelayLine<float> delayLineLeft { 192000 }; // Max 4 seconds at 48kHz
    juce::dsp::DelayLine<float> delayLineRight { 192000 };
    juce::IIRFilter highCutFilterLeft, highCutFilterRight;
    juce::IIRFilter lowCutFilterLeft, lowCutFilterRight;
    
    // State variables
    double currentSampleRate = 44100.0;
    float previousHighCut = -1.0f;
    float previousLowCut = -1.0f;
    
    // Metering
    std::atomic<float> inputLevel { 0.0f };
    std::atomic<float> outputLevel { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SingleDelayProcessor)
};