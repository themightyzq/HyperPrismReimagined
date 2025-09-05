//==============================================================================
// HyperPrism Revived - Frequency Shifter Processor
//==============================================================================

#pragma once

#include <JuceHeader.h>

class FrequencyShifterProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    FrequencyShifterProcessor();
    ~FrequencyShifterProcessor() override = default;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return "HyperPrism Reimagined Frequency Shifter"; }

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
    static const juce::String FREQUENCY_SHIFT_ID;
    static const juce::String FINE_SHIFT_ID;
    static const juce::String MIX_ID;
    static const juce::String OUTPUT_LEVEL_ID;
    
    // Metering
    float getInputLevel() const { return inputLevel.load(); }
    float getOutputLevel() const { return outputLevel.load(); }

private:
    //==============================================================================
    class HilbertTransform
    {
    public:
        HilbertTransform();
        
        void prepare(double sampleRate);
        void reset();
        
        void processBlock(juce::AudioBuffer<float>& buffer);
        std::pair<float, float> processSample(float input); // Returns {real, imaginary}
        
    private:
        static constexpr int filterOrder = 256;
        
        juce::dsp::FIR::Filter<float> hilbertFilter;
        juce::dsp::DelayLine<float> delayLine;
        
        void createHilbertCoefficients();
        std::vector<float> hilbertCoefficients;
    };
    
    class Oscillator
    {
    public:
        Oscillator() = default;
        
        void prepare(double sampleRate);
        void setFrequency(float frequency);
        void reset();
        
        std::pair<float, float> getNextSample(); // Returns {cos, sin}
        
    private:
        double currentSampleRate = 44100.0;
        float frequency = 0.0f;
        double phase = 0.0;
        double phaseIncrement = 0.0;
        
        void updatePhaseIncrement();
    };
    
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void processFrequencyShifting(juce::AudioBuffer<float>& buffer);
    
    juce::AudioProcessorValueTreeState valueTreeState;
    
    // Parameter pointers for performance
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* frequencyShiftParam = nullptr;
    std::atomic<float>* fineShiftParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* outputLevelParam = nullptr;
    
    // DSP components
    HilbertTransform hilbertTransform;
    Oscillator oscillator;
    
    // Metering
    std::atomic<float> inputLevel { 0.0f };
    std::atomic<float> outputLevel { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FrequencyShifterProcessor)
};