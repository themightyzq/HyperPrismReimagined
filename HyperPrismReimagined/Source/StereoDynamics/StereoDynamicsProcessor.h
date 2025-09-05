//==============================================================================
// HyperPrism Revived - Stereo Dynamics Processor
//==============================================================================

#pragma once

#include <JuceHeader.h>

class StereoDynamicsProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    StereoDynamicsProcessor();
    ~StereoDynamicsProcessor() override = default;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return "HyperPrism Reimagined Stereo Dynamics"; }

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
    static const juce::String MID_THRESHOLD_ID;
    static const juce::String MID_RATIO_ID;
    static const juce::String SIDE_THRESHOLD_ID;
    static const juce::String SIDE_RATIO_ID;
    static const juce::String ATTACK_TIME_ID;
    static const juce::String RELEASE_TIME_ID;
    static const juce::String OUTPUT_LEVEL_ID;
    
    // Metering
    float getLeftLevel() const { return leftLevel.load(); }
    float getRightLevel() const { return rightLevel.load(); }
    float getMidLevel() const { return midLevel.load(); }
    float getSideLevel() const { return sideLevel.load(); }
    float getMidGainReduction() const { return midGainReduction.load(); }
    float getSideGainReduction() const { return sideGainReduction.load(); }

private:
    //==============================================================================
    class EnvelopeFollower
    {
    public:
        EnvelopeFollower() = default;
        
        void prepare(double sampleRate);
        void setAttackTime(float attackMs);
        void setReleaseTime(float releaseMs);
        void reset();
        
        float processSample(float input);
        
    private:
        double currentSampleRate = 44100.0;
        float attackCoeff = 0.0f;
        float releaseCoeff = 0.0f;
        float envelope = 0.0f;
        
        void updateCoefficients();
    };
    
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void processStereoDynamics(juce::AudioBuffer<float>& buffer);
    float calculateGainReduction(float level, float threshold, float ratio);
    void encodeLRToMS(float left, float right, float& mid, float& side);
    void decodeMSToLR(float mid, float side, float& left, float& right);
    
    juce::AudioProcessorValueTreeState valueTreeState;
    
    // Parameter pointers for performance
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* midThresholdParam = nullptr;
    std::atomic<float>* midRatioParam = nullptr;
    std::atomic<float>* sideThresholdParam = nullptr;
    std::atomic<float>* sideRatioParam = nullptr;
    std::atomic<float>* attackTimeParam = nullptr;
    std::atomic<float>* releaseTimeParam = nullptr;
    std::atomic<float>* outputLevelParam = nullptr;
    
    // DSP components
    EnvelopeFollower midEnvelopeFollower;
    EnvelopeFollower sideEnvelopeFollower;
    
    // State variables
    juce::SmoothedValue<float> smoothedMidGain;
    juce::SmoothedValue<float> smoothedSideGain;
    
    // Metering
    std::atomic<float> leftLevel { 0.0f };
    std::atomic<float> rightLevel { 0.0f };
    std::atomic<float> midLevel { 0.0f };
    std::atomic<float> sideLevel { 0.0f };
    std::atomic<float> midGainReduction { 0.0f };
    std::atomic<float> sideGainReduction { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StereoDynamicsProcessor)
};