//==============================================================================
// HyperPrism Revived - Multi Delay Processor
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <array>

class MultiDelayProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    MultiDelayProcessor();
    ~MultiDelayProcessor() override = default;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return "HyperPrism Reimagined Multi Delay"; }

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
    static const juce::String MASTER_MIX_ID;
    static const juce::String GLOBAL_FEEDBACK_ID;
    
    // Delay line parameter IDs (4 delays)
    static const juce::String DELAY1_TIME_ID;
    static const juce::String DELAY1_LEVEL_ID;
    static const juce::String DELAY1_PAN_ID;
    static const juce::String DELAY1_FEEDBACK_ID;
    
    static const juce::String DELAY2_TIME_ID;
    static const juce::String DELAY2_LEVEL_ID;
    static const juce::String DELAY2_PAN_ID;
    static const juce::String DELAY2_FEEDBACK_ID;
    
    static const juce::String DELAY3_TIME_ID;
    static const juce::String DELAY3_LEVEL_ID;
    static const juce::String DELAY3_PAN_ID;
    static const juce::String DELAY3_FEEDBACK_ID;
    
    static const juce::String DELAY4_TIME_ID;
    static const juce::String DELAY4_LEVEL_ID;
    static const juce::String DELAY4_PAN_ID;
    static const juce::String DELAY4_FEEDBACK_ID;
    
    // Metering
    float getInputLevel() const { return inputLevel.load(); }
    float getOutputLevel() const { return outputLevel.load(); }
    std::array<float, 4> getDelayLevels() const;

private:
    //==============================================================================
    static constexpr int NUM_DELAYS = 4;
    
    struct DelayLine
    {
        juce::dsp::DelayLine<float> leftDelay { 192000 };
        juce::dsp::DelayLine<float> rightDelay { 192000 };
        std::atomic<float> levelMeter { 0.0f };
    };
    
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void processMultiDelay(juce::AudioBuffer<float>& buffer);
    
    juce::AudioProcessorValueTreeState valueTreeState;
    
    // Parameter pointers for performance
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* masterMixParam = nullptr;
    std::atomic<float>* globalFeedbackParam = nullptr;
    
    // Delay parameters
    std::array<std::atomic<float>*, NUM_DELAYS> delayTimeParams;
    std::array<std::atomic<float>*, NUM_DELAYS> delayLevelParams;
    std::array<std::atomic<float>*, NUM_DELAYS> delayPanParams;
    std::array<std::atomic<float>*, NUM_DELAYS> delayFeedbackParams;
    
    // DSP components
    std::array<DelayLine, NUM_DELAYS> delayLines;
    
    // State variables
    double currentSampleRate = 44100.0;
    
    // Metering
    std::atomic<float> inputLevel { 0.0f };
    std::atomic<float> outputLevel { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiDelayProcessor)
};