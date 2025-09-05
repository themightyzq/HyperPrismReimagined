#pragma once

#include <JuceHeader.h>

class HarmonicExciterProcessor : public juce::AudioProcessor
{
public:
    HarmonicExciterProcessor();
    ~HarmonicExciterProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Parameters
    juce::AudioParameterFloat* driveParam;
    juce::AudioParameterFloat* frequencyParam;
    juce::AudioParameterFloat* harmonicsParam;
    juce::AudioParameterFloat* mixParam;
    juce::AudioParameterChoice* typeParam;

    // Get current output level for metering
    float getCurrentOutputLevel() const { return outputLevel.load(); }

private:
    // Processing components
    juce::dsp::LinkwitzRileyFilter<float> highPassFilter;
    juce::dsp::LinkwitzRileyFilter<float> lowPassFilter;
    
    // Output level for metering
    std::atomic<float> outputLevel { 0.0f };
    
    // Sample rate storage
    double currentSampleRate = 44100.0;
    
    // Harmonic generation functions
    float generateWarmHarmonics(float input, float drive, float harmonics);
    float generateBrightHarmonics(float input, float drive, float harmonics);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HarmonicExciterProcessor)
};