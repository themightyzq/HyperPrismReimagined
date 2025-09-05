//==============================================================================
// HyperPrism Revived - High-Pass Filter Processor
//==============================================================================

#pragma once

#include <JuceHeader.h>

class HighPassProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    HighPassProcessor();
    ~HighPassProcessor() override;

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
    static const juce::String FREQUENCY_ID;
    static const juce::String RESONANCE_ID;
    static const juce::String GAIN_ID;
    static const juce::String MIX_ID;

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState valueTreeState;
    
    // Filter processing
    using FilterType = juce::dsp::IIR::Filter<float>;
    using CoefficientsType = juce::dsp::IIR::Coefficients<float>;
    
    juce::dsp::ProcessorDuplicator<FilterType, CoefficientsType> highPassFilter;
    
    // Parameter smoothing
    juce::SmoothedValue<float> frequencySmoothed;
    juce::SmoothedValue<float> resonanceSmoothed;
    juce::SmoothedValue<float> gainSmoothed;
    juce::SmoothedValue<float> mixSmoothed;
    
    double currentSampleRate = 44100.0;
    
    void updateFilter();
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HighPassProcessor)
};