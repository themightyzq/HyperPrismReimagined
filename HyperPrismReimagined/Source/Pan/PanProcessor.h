//==============================================================================
// HyperPrism Revived - Pan Processor
//==============================================================================

#pragma once

#include <JuceHeader.h>

class PanProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    PanProcessor();
    ~PanProcessor() override = default;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return "HyperPrism Reimagined Pan"; }

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
    static const juce::String PAN_POSITION_ID;
    static const juce::String PAN_LAW_ID;
    static const juce::String WIDTH_ID;
    static const juce::String BALANCE_ID;
    static const juce::String OUTPUT_LEVEL_ID;
    
    // Metering
    float getLeftLevel() const { return leftLevel.load(); }
    float getRightLevel() const { return rightLevel.load(); }
    float getPanPosition() const { return panPositionParam ? panPositionParam->load() / 100.0f : 0.0f; }

private:
    //==============================================================================
    enum PanLawType
    {
        Linear = 0,
        EqualPower,
        NegThreeDB,
        NegSixDB
    };
    
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void processPanning(juce::AudioBuffer<float>& buffer);
    void calculatePanGains(float panValue, int panLawType, float& leftGain, float& rightGain);
    
    juce::AudioProcessorValueTreeState valueTreeState;
    
    // Parameter pointers for performance
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* panPositionParam = nullptr;
    std::atomic<float>* panLawParam = nullptr;
    std::atomic<float>* widthParam = nullptr;
    std::atomic<float>* balanceParam = nullptr;
    std::atomic<float>* outputLevelParam = nullptr;
    
    // State variables
    juce::SmoothedValue<float> smoothedLeftGain;
    juce::SmoothedValue<float> smoothedRightGain;
    
    // Metering
    std::atomic<float> leftLevel { 0.0f };
    std::atomic<float> rightLevel { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanProcessor)
};