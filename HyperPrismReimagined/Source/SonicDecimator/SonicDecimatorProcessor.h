//==============================================================================
// HyperPrism Reimagined - Sonic Decimator Processor
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <array>

// 4x oversampling wraps the bit/rate quantiser only (SampleRateReducer::processSample +
// BitCrusher::processSample, called from processDecimation()) to push the quantisation
// harmonics above the original Nyquist before they alias back down. The
// SampleRateReducer's hold counter is scaled by the same factor (its "original sample
// rate" is prepared as hostRate * kOversamplingFactor) so a given Rate setting sounds
// the same as before at 1x. Flip to 1 for an un-oversampled A/B comparison; never ship
// it that way.
#define HP_SONICDECIMATOR_FORCE_1X 0

class SonicDecimatorProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    SonicDecimatorProcessor();
    ~SonicDecimatorProcessor() override = default;

    static constexpr int kOversamplingFactor = 4;

    //==============================================================================
    void prepareToPlay(double sampleRate, int) override;
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

    // Editor size persistence (see getStateInformation/setStateInformation)
    int getEditorWidth() const noexcept { return editorWidth.load(); }
    int getEditorHeight() const noexcept { return editorHeight.load(); }
    void setEditorSize(int w, int h) noexcept { editorWidth.store(w); editorHeight.store(h); }

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
        
        void prepare(double sampleRate, int samplesPerBlock);
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
    
    // DSP components, one set per channel. A single shared set processed channel by
    // channel let channel 0's hold counter, last held sample and dither RNG carry into
    // channel 1 every block (stereo crosstalk; fixed 2026-09-23). Sized for the widest
    // bus layout isBusesLayoutSupported can accept; indexed by channel, clamped.
    static constexpr int kMaxChannels = 8;
    std::array<BitCrusher, kMaxChannels> bitCrushers;
    std::array<SampleRateReducer, kMaxChannels> sampleRateReducers;
    std::array<NoiseShaper, kMaxChannels> noiseShapers;
    
    // State variables
    juce::AudioBuffer<float> dryBuffer;
    
    // Metering
    std::atomic<float> inputLevel { 0.0f };
    std::atomic<float> outputLevel { 0.0f };
    std::atomic<float> bitReduction { 0.0f };
    std::atomic<float> sampleReduction { 0.0f };

    // 4x oversampling around the bit/rate quantiser only (see HP_SONICDECIMATOR_FORCE_1X
    // above). Rebuilt in prepareToPlay; never touched from processBlock.
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;

    // Editor size persistence, read/written from the message thread only.
    std::atomic<int> editorWidth { 0 };
    std::atomic<int> editorHeight { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SonicDecimatorProcessor)
};