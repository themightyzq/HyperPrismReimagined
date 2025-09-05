//==============================================================================
// HyperPrism Revived - More Stereo Processor
//==============================================================================

#pragma once

#include <JuceHeader.h>

class MoreStereoProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    MoreStereoProcessor();
    ~MoreStereoProcessor() override = default;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return "HyperPrism Reimagined More Stereo"; }

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
    static const juce::String BASS_MONO_ID;
    static const juce::String CROSSOVER_FREQ_ID;
    static const juce::String STEREO_ENHANCE_ID;
    static const juce::String AMBIENCE_ID;
    static const juce::String OUTPUT_LEVEL_ID;
    
    // Metering
    float getLeftLevel() const { return leftLevel.load(); }
    float getRightLevel() const { return rightLevel.load(); }
    float getStereoWidth() const { return stereoWidth.load(); }
    float getAmbienceLevel() const { return ambienceLevel.load(); }

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void processMoreStereo(juce::AudioBuffer<float>& buffer);
    void calculateStereoWidth(const juce::AudioBuffer<float>& buffer);
    
    juce::AudioProcessorValueTreeState valueTreeState;
    
    // Parameter pointers for performance
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* widthParam = nullptr;
    std::atomic<float>* bassMonoParam = nullptr;
    std::atomic<float>* crossoverFreqParam = nullptr;
    std::atomic<float>* stereoEnhanceParam = nullptr;
    std::atomic<float>* ambienceParam = nullptr;
    std::atomic<float>* outputLevelParam = nullptr;
    
    // DSP components for crossover
    juce::IIRFilter lowPassLeft, lowPassRight;
    juce::IIRFilter highPassLeft, highPassRight;
    
    // Ambience processing
    juce::dsp::Reverb reverb;
    juce::dsp::DelayLine<float> ambienceDelayLeft { 4800 };
    juce::dsp::DelayLine<float> ambienceDelayRight { 4800 };
    
    // State variables
    double currentSampleRate = 44100.0;
    float previousCrossoverFreq = -1.0f;
    
    // Metering
    std::atomic<float> leftLevel { 0.0f };
    std::atomic<float> rightLevel { 0.0f };
    std::atomic<float> stereoWidth { 0.0f };
    std::atomic<float> ambienceLevel { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MoreStereoProcessor)
};