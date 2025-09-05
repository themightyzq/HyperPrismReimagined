//==============================================================================
// HyperPrism Revived - Phaser Processor
//==============================================================================

#pragma once

#include <JuceHeader.h>

class PhaserProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    PhaserProcessor();
    ~PhaserProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState& getValueTreeState() { return valueTreeState; }

    // Parameter IDs
    static const juce::String BYPASS_ID;
    static const juce::String RATE_ID;
    static const juce::String DEPTH_ID;
    static const juce::String FEEDBACK_ID;
    static const juce::String STAGES_ID;
    static const juce::String MIX_ID;

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState valueTreeState;
    
    // Phaser implementation using all-pass filters
    class AllPassFilter
    {
    public:
        void prepare(double sampleRate)
        {
            delay = 0.0f;
            this->sampleRate = static_cast<float>(sampleRate);
        }
        
        float process(float input, float frequency)
        {
            // Calculate all-pass coefficient
            float tanValue = std::tan(juce::MathConstants<float>::pi * frequency / sampleRate);
            float coefficient = (tanValue - 1.0f) / (tanValue + 1.0f);
            
            float output = coefficient * input + delay;
            delay = input - coefficient * output;
            
            return output;
        }
        
        void reset() { delay = 0.0f; }
        
    private:
        float delay = 0.0f;
        float sampleRate = 44100.0f;
    };
    
    static constexpr int maxStages = 12;
    std::array<AllPassFilter, maxStages> allPassFiltersL;
    std::array<AllPassFilter, maxStages> allPassFiltersR;
    
    // LFO for modulation
    float lfoPhase = 0.0f;
    
    // Parameter smoothing
    juce::SmoothedValue<float> rateSmoothed;
    juce::SmoothedValue<float> depthSmoothed;
    juce::SmoothedValue<float> feedbackSmoothed;
    juce::SmoothedValue<float> mixSmoothed;
    
    double currentSampleRate = 44100.0;
    
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhaserProcessor)
};