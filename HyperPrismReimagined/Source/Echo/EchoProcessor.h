//==============================================================================
// HyperPrism Revived - Echo Processor
// Simple digital delay with feedback control
//==============================================================================

#pragma once

#include <JuceHeader.h>

class EchoProcessor : public juce::AudioProcessor
{
public:
    static constexpr auto DELAY_ID = "delay";
    static constexpr auto FEEDBACK_ID = "feedback";
    static constexpr auto MIX_ID = "mix";
    static constexpr auto BYPASS_ID = "bypass";

    EchoProcessor();
    ~EchoProcessor() override;

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
    double getTailLengthSeconds() const override { return 2.0; }

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
    
    // Delay line
    juce::dsp::DelayLine<float> delayLineLeft { 192000 };  // 2 seconds at 96kHz
    juce::dsp::DelayLine<float> delayLineRight { 192000 };
    
    // Current parameter values
    float currentSampleRate = 44100.0f;
    
    // Parameter smoothing
    juce::SmoothedValue<float> delaySmoothed;
    juce::SmoothedValue<float> feedbackSmoothed;
    juce::SmoothedValue<float> mixSmoothed;
    
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EchoProcessor)
};