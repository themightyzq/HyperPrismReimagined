//==============================================================================
// HyperPrism Revived - Tube/Tape Saturation Processor
//==============================================================================

#pragma once

#include <JuceHeader.h>

class TubeTapeSaturationProcessor : public juce::AudioProcessor
{
public:
    TubeTapeSaturationProcessor();
    ~TubeTapeSaturationProcessor() override = default;

    // AudioProcessor interface
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    
    // Editor
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    
    // Program/State
    const juce::String getName() const override { return "HyperPrism Reimagined Tube/Tape Saturation"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int /*index*/) override {}
    const juce::String getProgramName(int /*index*/) override { return "Default"; }
    void changeProgramName(int /*index*/, const juce::String& /*newName*/) override {}
    
    // State save/restore
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    
    // Parameter access
    juce::AudioProcessorValueTreeState& getValueTreeState() { return valueTreeState; }
    
    // Get harmonic content for meter display
    float getHarmonicContent() const { return harmonicContent.load(); }
    
    // Get metering levels
    float getInputLevel() const { return inputLevel.load(); }
    float getOutputLevel() const { return outputLevel.load(); }

    // Parameter IDs
    static const juce::String BYPASS_ID;
    static const juce::String DRIVE_ID;
    static const juce::String TYPE_ID;
    static const juce::String WARMTH_ID;
    static const juce::String BRIGHTNESS_ID;
    static const juce::String OUTPUT_LEVEL_ID;

    // Saturation types
    enum SaturationType
    {
        Tube = 0,
        Tape,
        Transformer
    };

private:
    // Parameter layout
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Audio processing
    void processSaturation(juce::AudioBuffer<float>& buffer);
    void updateFilters();
    void calculateHarmonicContent(const juce::AudioBuffer<float>& buffer);
    
    // Saturation algorithms
    float processTubeSaturation(float input, float drive, float warmth, float brightness);
    float processTapeSaturation(float input, float drive, float warmth, float brightness);
    float processTransformerSaturation(float input, float drive, float warmth, float brightness);
    
    // Helper functions
    float softClip(float input, float amount);
    float asymmetricClip(float input, float amount);
    float tanhSaturation(float input, float amount);
    
    // State
    juce::AudioProcessorValueTreeState valueTreeState;
    
    // DSP components for warmth and brightness shaping
    juce::IIRFilter lowShelfLeft, lowShelfRight;   // For warmth control
    juce::IIRFilter highShelfLeft, highShelfRight; // For brightness control
    
    // Cached parameters
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* typeParam = nullptr;
    std::atomic<float>* warmthParam = nullptr;
    std::atomic<float>* brightnessParam = nullptr;
    std::atomic<float>* outputLevelParam = nullptr;
    
    // Processing state
    double currentSampleRate = 44100.0;
    float previousWarmth = -1.0f;
    float previousBrightness = -1.0f;
    
    // Harmonic content analysis
    std::atomic<float> harmonicContent { 0.0f };
    std::atomic<float> inputLevel { 0.0f };
    std::atomic<float> outputLevel { 0.0f };
    float previousInputRMS = 0.0f;
    float previousOutputRMS = 0.0f;
    
    // DC blocking filters
    juce::IIRFilter dcBlockLeft, dcBlockRight;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TubeTapeSaturationProcessor)
};