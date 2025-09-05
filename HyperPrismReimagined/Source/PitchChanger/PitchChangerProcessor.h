//==============================================================================
// HyperPrism Revived - Pitch Changer Processor
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "../../../ThirdParty/signalsmith-stretch/signalsmith-stretch.h"

class PitchChangerProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    PitchChangerProcessor();
    ~PitchChangerProcessor() override = default;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return "HyperPrism Reimagined Pitch Changer"; }

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
    static const juce::String PITCH_SHIFT_ID;
    static const juce::String FINE_TUNE_ID;
    static const juce::String FORMANT_SHIFT_ID;
    static const juce::String MIX_ID;
    static const juce::String OUTPUT_LEVEL_ID;
    
    // Metering
    float getInputLevel() const { return inputLevel.load(); }
    float getOutputLevel() const { return outputLevel.load(); }
    float getPitchDetection() const { return pitchDetection.load(); }

private:
    //==============================================================================
    // Pitch shifter using Signalsmith Stretch
    class PitchShifter
    {
    public:
        PitchShifter();
        ~PitchShifter() = default;
        
        void prepare(double sampleRate, int blockSize);
        void reset();
        
        void setPitchShift(float pitchRatio);
        void setFormantShift(float formantRatio);
        
        void processBlock(juce::AudioBuffer<float>& buffer);
        
    private:
        std::unique_ptr<signalsmith::stretch::SignalsmithStretch<float>> stretcher;
        
        double currentSampleRate = 44100.0;
        int maxBlockSize = 512;
        float currentPitchRatio = 1.0f;
        float currentFormantRatio = 1.0f;
        
        std::vector<float> leftInputBuffer;
        std::vector<float> rightInputBuffer;
        std::vector<float> leftOutputBuffer;
        std::vector<float> rightOutputBuffer;
    };
    
    class PitchDetector
    {
    public:
        PitchDetector();
        
        void prepare(double sampleRate);
        void reset();
        
        float detectPitch(const float* buffer, int bufferSize);
        
    private:
        static constexpr int analysisSize = 1024;
        
        double currentSampleRate = 44100.0;
        std::vector<float> autocorrelationBuffer;
        std::vector<float> analysisBuffer;
        
        float autocorrelate(const float* buffer, int delay, int bufferSize);
    };
    
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void processPitchShifting(juce::AudioBuffer<float>& buffer);
    
    juce::AudioProcessorValueTreeState valueTreeState;
    
    // Parameter pointers for performance
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* pitchShiftParam = nullptr;
    std::atomic<float>* fineTuneParam = nullptr;
    std::atomic<float>* formantShiftParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* outputLevelParam = nullptr;
    
    // DSP components
    std::unique_ptr<PitchShifter> pitchShifter;
    PitchDetector pitchDetector;
    
    // State variables
    juce::AudioBuffer<float> dryBuffer;
    
    // Metering
    std::atomic<float> inputLevel { 0.0f };
    std::atomic<float> outputLevel { 0.0f };
    std::atomic<float> pitchDetection { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchChangerProcessor)
};