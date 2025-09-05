//==============================================================================
// HyperPrism Revived - Band-Reject (Notch) Filter Processor
//==============================================================================

#pragma once

#include <JuceHeader.h>

class BandRejectProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    BandRejectProcessor();
    ~BandRejectProcessor() override;

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
    static const juce::String CENTER_FREQ_ID;
    static const juce::String Q_ID;
    static const juce::String GAIN_ID;
    static const juce::String MIX_ID;

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState valueTreeState;
    
    // Filter processing - using band-stop filter
    using FilterType = juce::dsp::IIR::Filter<float>;
    using CoefficientsType = juce::dsp::IIR::Coefficients<float>;
    
    juce::dsp::ProcessorDuplicator<FilterType, CoefficientsType> notchFilter;
    
    // Parameter smoothing
    juce::SmoothedValue<float> centerFreqSmoothed;
    juce::SmoothedValue<float> qSmoothed;
    juce::SmoothedValue<float> gainSmoothed;
    juce::SmoothedValue<float> mixSmoothed;
    
    double currentSampleRate = 44100.0;
    
    void updateFilter();
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BandRejectProcessor)
};