//==============================================================================
// HyperPrism Revived - Quasi Stereo Processor
//==============================================================================

#pragma once

#include <JuceHeader.h>

class QuasiStereoProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    QuasiStereoProcessor();
    ~QuasiStereoProcessor() override = default;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return "HyperPrism Reimagined Quasi Stereo"; }

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
    static const juce::String WIDTH_ID;
    static const juce::String DELAY_TIME_ID;
    static const juce::String FREQUENCY_SHIFT_ID;
    static const juce::String PHASE_SHIFT_ID;
    static const juce::String HIGH_FREQ_ENHANCE_ID;
    static const juce::String OUTPUT_LEVEL_ID;
    
    // Metering
    float getLeftLevel() const { return leftLevel.load(); }
    float getRightLevel() const { return rightLevel.load(); }
    float getStereoWidth() const { return stereoWidth.load(); }

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void processQuasiStereo(juce::AudioBuffer<float>& buffer);
    void calculateStereoWidth(const juce::AudioBuffer<float>& buffer);
    
    juce::AudioProcessorValueTreeState valueTreeState;
    
    // Parameter pointers for performance
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* widthParam = nullptr;
    std::atomic<float>* delayTimeParam = nullptr;
    std::atomic<float>* frequencyShiftParam = nullptr;
    std::atomic<float>* phaseShiftParam = nullptr;
    std::atomic<float>* highFreqEnhanceParam = nullptr;
    std::atomic<float>* outputLevelParam = nullptr;
    
    // DSP components
    juce::dsp::DelayLine<float> delayLine { 4800 }; // Max 100ms at 48kHz
    juce::IIRFilter highFreqFilterLeft, highFreqFilterRight;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> allPassFilter;
    
    // State variables
    double currentSampleRate = 44100.0;
    float previousHighFreqEnhance = -1.0f;
    float phaseAccumulator = 0.0f;
    
    // Metering
    std::atomic<float> leftLevel { 0.0f };
    std::atomic<float> rightLevel { 0.0f };
    std::atomic<float> stereoWidth { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuasiStereoProcessor)
};