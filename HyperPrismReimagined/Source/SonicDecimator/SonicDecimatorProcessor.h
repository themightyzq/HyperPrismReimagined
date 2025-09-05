//==============================================================================
// HyperPrism Revived - Sonic Decimator Processor
//==============================================================================

#pragma once

#include <JuceHeader.h>

class SonicDecimatorProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    SonicDecimatorProcessor();
    ~SonicDecimatorProcessor() override = default;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return "HyperPrism Reimagined Sonic Decimator"; }

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
    static const juce::String BIT_DEPTH_ID;
    static const juce::String SAMPLE_RATE_ID;
    static const juce::String ANTI_ALIAS_ID;
    static const juce::String DITHER_ID;
    static const juce::String MIX_ID;
    static const juce::String OUTPUT_LEVEL_ID;
    
    // Metering
    float getInputLevel() const { return inputLevel.load(); }
    float getOutputLevel() const { return outputLevel.load(); }
    float getBitReduction() const { return bitReduction.load(); }
    float getSampleReduction() const { return sampleReduction.load(); }

private:
    //==============================================================================
    class BitCrusher
    {
    public:
        BitCrusher() = default;
        
        void setBitDepth(float bitDepth);
        void setDithering(bool enableDither);
        void reset();
        
        float processSample(float input);
        
    private:
        float bitDepth = 16.0f;
        bool ditherEnabled = false;
        float quantizationStep = 1.0f / 32768.0f; // 16-bit
        
        juce::Random random;
        
        void updateQuantizationStep();
    };
    
    class SampleRateReducer
    {
    public:
        SampleRateReducer() = default;
        
        void prepare(double sampleRate);
        void setSampleRate(float targetSampleRate);
        void setAntiAliasing(bool enableAntiAlias);
        void reset();
        
        float processSample(float input);
        
    private:
        double originalSampleRate = 44100.0;
        float targetSampleRate = 44100.0f;
        bool antiAliasingEnabled = true;
        
        float sampleCounter = 0.0f;
        float lastOutputSample = 0.0f;
        
        // Anti-aliasing filter
        juce::dsp::IIR::Filter<float> antiAliasFilter;
    };
    
    class NoiseShaper
    {
    public:
        NoiseShaper() = default;
        
        void reset();
        float processSample(float input, float quantizationNoise);
        
    private:
        float delayedError = 0.0f;
    };
    
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void processDecimation(juce::AudioBuffer<float>& buffer);
    
    juce::AudioProcessorValueTreeState valueTreeState;
    
    // Parameter pointers for performance
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* bitDepthParam = nullptr;
    std::atomic<float>* sampleRateParam = nullptr;
    std::atomic<float>* antiAliasParam = nullptr;
    std::atomic<float>* ditherParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* outputLevelParam = nullptr;
    
    // DSP components
    BitCrusher bitCrusher;
    SampleRateReducer sampleRateReducer;
    NoiseShaper noiseShaper;
    
    // State variables
    juce::AudioBuffer<float> dryBuffer;
    
    // Metering
    std::atomic<float> inputLevel { 0.0f };
    std::atomic<float> outputLevel { 0.0f };
    std::atomic<float> bitReduction { 0.0f };
    std::atomic<float> sampleReduction { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SonicDecimatorProcessor)
};