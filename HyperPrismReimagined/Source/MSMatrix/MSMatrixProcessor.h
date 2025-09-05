//==============================================================================
// HyperPrism Revived - M+S Matrix Processor
//==============================================================================

#pragma once

#include <JuceHeader.h>

class MSMatrixProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    MSMatrixProcessor();
    ~MSMatrixProcessor() override = default;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return "HyperPrism Reimagined M+S Matrix"; }

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
    static const juce::String MATRIX_MODE_ID;
    static const juce::String MID_LEVEL_ID;
    static const juce::String SIDE_LEVEL_ID;
    static const juce::String MID_SOLO_ID;
    static const juce::String SIDE_SOLO_ID;
    static const juce::String STEREO_BALANCE_ID;
    static const juce::String OUTPUT_LEVEL_ID;
    
    // Metering
    float getLeftLevel() const { return leftLevel.load(); }
    float getRightLevel() const { return rightLevel.load(); }
    float getMidLevel() const { return midLevel.load(); }
    float getSideLevel() const { return sideLevel.load(); }

private:
    //==============================================================================
    enum MatrixMode
    {
        LRToMS = 0,  // L/R input -> M/S processing -> L/R output
        MSToLR,      // M/S input -> L/R processing -> L/R output
        MSThrough    // M/S input -> M/S processing -> M/S output
    };
    
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void processLRToMS(juce::AudioBuffer<float>& buffer);
    void processMSToLR(juce::AudioBuffer<float>& buffer);
    void processMSThrough(juce::AudioBuffer<float>& buffer);
    void encodeLRToMS(float left, float right, float& mid, float& side);
    void decodeMSToLR(float mid, float side, float& left, float& right);
    
    juce::AudioProcessorValueTreeState valueTreeState;
    
    // Parameter pointers for performance
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* matrixModeParam = nullptr;
    std::atomic<float>* midLevelParam = nullptr;
    std::atomic<float>* sideLevelParam = nullptr;
    std::atomic<float>* midSoloParam = nullptr;
    std::atomic<float>* sideSoloParam = nullptr;
    std::atomic<float>* stereoBalanceParam = nullptr;
    std::atomic<float>* outputLevelParam = nullptr;
    
    // State variables
    juce::SmoothedValue<float> smoothedMidLevel;
    juce::SmoothedValue<float> smoothedSideLevel;
    juce::SmoothedValue<float> smoothedStereoBalance;
    
    // Metering
    std::atomic<float> leftLevel { 0.0f };
    std::atomic<float> rightLevel { 0.0f };
    std::atomic<float> midLevel { 0.0f };
    std::atomic<float> sideLevel { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MSMatrixProcessor)
};