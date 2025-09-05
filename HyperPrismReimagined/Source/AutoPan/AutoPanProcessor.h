//==============================================================================
// HyperPrism Revived - Auto Pan Processor
//==============================================================================

#pragma once

#include <JuceHeader.h>

class LFO
{
public:
    LFO() = default;
    
    void prepare(double sampleRate);
    void setFrequency(float frequencyHz);
    void setWaveform(int waveformType);
    void reset();
    
    float getNextSample();
    float getPhase() const { return phase; }
    
    enum WaveformType
    {
        Sine = 0,
        Triangle,
        Square,
        Sawtooth,
        Random
    };
    
private:
    double currentSampleRate = 44100.0;
    float frequency = 1.0f;
    int waveform = Sine;
    float phase = 0.0f;
    float phaseIncrement = 0.0f;
    float randomValue = 0.0f;
    float targetRandomValue = 0.0f;
    int randomCounter = 0;
    
    void updatePhaseIncrement();
};

class AutoPanProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    AutoPanProcessor();
    ~AutoPanProcessor() override = default;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return "HyperPrism Reimagined Auto Pan"; }

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
    static const juce::String RATE_ID;
    static const juce::String DEPTH_ID;
    static const juce::String WAVEFORM_ID;
    static const juce::String PHASE_ID;
    static const juce::String SYNC_ID;
    static const juce::String OUTPUT_LEVEL_ID;
    
    // Metering
    float getLeftLevel() const { return leftLevel.load(); }
    float getRightLevel() const { return rightLevel.load(); }
    float getLFOValue() const { return lfoValue.load(); }
    float getPanPosition() const { return panPosition.load(); }
    float getLFOPhase() const { return lfoPhase.load(); }

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void processAutoPan(juce::AudioBuffer<float>& buffer);
    void calculatePanGains(float panValue, float& leftGain, float& rightGain);
    
    juce::AudioProcessorValueTreeState valueTreeState;
    
    // Parameter pointers for performance
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* rateParam = nullptr;
    std::atomic<float>* depthParam = nullptr;
    std::atomic<float>* waveformParam = nullptr;
    std::atomic<float>* phaseParam = nullptr;
    std::atomic<float>* syncParam = nullptr;
    std::atomic<float>* outputLevelParam = nullptr;
    
    // DSP components
    LFO lfo;
    
    // State variables
    juce::SmoothedValue<float> smoothedLeftGain;
    juce::SmoothedValue<float> smoothedRightGain;
    
    // Metering
    std::atomic<float> leftLevel { 0.0f };
    std::atomic<float> rightLevel { 0.0f };
    std::atomic<float> lfoValue { 0.0f };
    std::atomic<float> panPosition { 0.0f };
    std::atomic<float> lfoPhase { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoPanProcessor)
};