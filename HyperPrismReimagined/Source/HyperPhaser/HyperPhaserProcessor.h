//==============================================================================
// HyperPrism Revived - HyperPhaser Processor
// Deep analog-sounding phaser/flanger with unique peak/notch control
//==============================================================================

#pragma once

#include <JuceHeader.h>

class HyperPhaserProcessor : public juce::AudioProcessor
{
public:
    static constexpr auto BASE_FREQ_ID = "baseFreq";
    static constexpr auto SWEEP_RATE_ID = "sweepRate";
    static constexpr auto PEAK_NOTCH_DEPTH_ID = "peakNotchDepth";
    static constexpr auto BANDWIDTH_ID = "bandwidth";
    static constexpr auto FEEDBACK_ID = "feedback";
    static constexpr auto MIX_ID = "mix";
    static constexpr auto BYPASS_ID = "bypass";

    HyperPhaserProcessor();
    ~HyperPhaserProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const juce::String getProgramName(int index) override { return {}; }
    void changeProgramName(int index, const juce::String& newName) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() { return parameters; }

private:
    juce::AudioProcessorValueTreeState parameters;
    
    // Allpass filter stage
    struct AllpassStage
    {
        float z1 = 0.0f;
        float process(float input, float coefficient)
        {
            float output = -input + z1;
            z1 = input + coefficient * output;
            return output;
        }
        void reset() { z1 = 0.0f; }
    };
    
    // Per-channel processing state
    struct ChannelState
    {
        static constexpr int NUM_STAGES = 8;
        std::array<AllpassStage, NUM_STAGES> stages;
        float lfoPhase = 0.0f;
        
        void reset()
        {
            for (auto& stage : stages)
                stage.reset();
            lfoPhase = 0.0f;
        }
    };
    
    std::array<ChannelState, 2> channelStates;
    
    // Current parameter values
    float currentSampleRate = 44100.0f;
    
    // Parameter smoothing
    juce::SmoothedValue<float> baseFreqSmoothed;
    juce::SmoothedValue<float> sweepRateSmoothed;
    juce::SmoothedValue<float> depthSmoothed;
    juce::SmoothedValue<float> bandwidthSmoothed;
    juce::SmoothedValue<float> feedbackSmoothed;
    juce::SmoothedValue<float> mixSmoothed;
    
    // Helper functions
    float calculateAllpassCoefficient(float frequency);
    float processPeakNotchDepth(float input, float depth);
    
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HyperPhaserProcessor)
};