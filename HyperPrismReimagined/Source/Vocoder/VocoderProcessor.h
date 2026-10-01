//==============================================================================
// HyperPrism Reimagined - Vocoder Processor
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <array>

class VocoderProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    VocoderProcessor();
    ~VocoderProcessor() override = default;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return "HyperPrism Reimagined Vocoder"; }

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
    int getEditorWidth() const { return editorWidth.load(); }
    int getEditorHeight() const { return editorHeight.load(); }
    void setEditorSize(int w, int h) { editorWidth = w; editorHeight = h; }

    // Parameter IDs
    static const juce::String BYPASS_ID;
    static const juce::String CARRIER_FREQ_ID;
    static const juce::String MODULATOR_GAIN_ID;
    static const juce::String BAND_COUNT_ID;
    static const juce::String RELEASE_TIME_ID;
    static const juce::String OUTPUT_LEVEL_ID;
    
    // Metering
    float getCarrierLevel() const { return carrierLevel.load(); }
    float getModulatorLevel() const { return modulatorLevel.load(); }
    float getOutputLevel() const { return outputLevel.load(); }
    const std::vector<float>& getBandLevels() const { return bandLevels; }

private:
    //==============================================================================
    static constexpr int maxBands = 16;
    static constexpr int defaultBands = 8;
    
    class VocoderBand
    {
    public:
        VocoderBand() = default;

        void prepare(double sampleRate, int samplesPerBlock);
        // bandwidthHz is converted to the filter's Q (centre / bandwidth). The coefficients
        // are written into the filters' existing storage, so this does not allocate.
        void setFrequency(float frequency, float bandwidthHz);
        void setReleaseCoefficient(float coefficient) { releaseCoeff = coefficient; }
        void reset();
        
        float processCarrier(float carrierSample);
        float processModulator(float modulatorSample);
        float getOutput();
        float getEnvelopeLevel() const { return envelopeLevel; }
        
    private:
        double currentSampleRate = 44100.0;
        
        // Bandpass filters for carrier and modulator
        juce::dsp::IIR::Filter<float> carrierFilter;
        juce::dsp::IIR::Filter<float> modulatorFilter;
        
        // Envelope follower for modulator
        float envelopeLevel = 0.0f;
        float releaseCoeff = 0.95f;
        
        // Processed carrier signal
        float processedCarrier = 0.0f;
        
    };
    
    class CarrierOscillator
    {
    public:
        CarrierOscillator() = default;
        
        void prepare(double sampleRate);
        void setFrequency(float frequency);
        void reset();
        
        float getNextSample();
        
    private:
        double currentSampleRate = 44100.0;
        float frequency = 440.0f;
        double phase = 0.0;
        double phaseIncrement = 0.0;
        
        void updatePhaseIncrement();
    };
    
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    void processVocoding(juce::AudioBuffer<float>& buffer);
    // Recomputes the band centres for currentBandCount and rebuilds every channel's band
    // filters at currentSampleRate. Allocation-free (called from prepareToPlay and, when
    // Band Count changes, from processBlock).
    void setupVocoderBands();
    
    juce::AudioProcessorValueTreeState valueTreeState;
    
    // Parameter pointers for performance
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* carrierFreqParam = nullptr;
    std::atomic<float>* modulatorGainParam = nullptr;
    std::atomic<float>* bandCountParam = nullptr;
    std::atomic<float>* releaseTimeParam = nullptr;
    std::atomic<float>* outputLevelParam = nullptr;
    
    // DSP components: one filter bank and one carrier oscillator per channel (they used to be
    // shared, so the second channel ran the oscillator and filters on from where the first
    // left off). Sized for the maximum band count up front.
    static constexpr int kMaxChannels = 2;
    std::array<std::array<VocoderBand, maxBands>, kMaxChannels> vocoderBands;
    std::array<CarrierOscillator, kMaxChannels> carrierOscillators;

    // State variables
    double currentSampleRate = 44100.0;
    int currentBandCount = defaultBands;
    std::array<float, maxBands> bandFrequencies {};

    // Modulator gain and output level, smoothed over 30 ms.
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> modulatorGainSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outputGainSmoothed;
    
    // Pre-allocated buffer for band level accumulation (real-time safe)
    std::vector<float> bandLevelSums;

    // Metering
    std::atomic<float> carrierLevel { 0.0f };
    std::atomic<float> modulatorLevel { 0.0f };
    std::atomic<float> outputLevel { 0.0f };
    mutable std::vector<float> bandLevels;

    // Editor size persistence
    std::atomic<int> editorWidth { 0 }, editorHeight { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocoderProcessor)
};