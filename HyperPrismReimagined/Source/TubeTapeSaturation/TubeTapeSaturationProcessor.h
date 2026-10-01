//==============================================================================
// HyperPrism Reimagined - Tube/Tape Saturation Processor
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>

// 4x oversampling wraps the saturation waveshaper only (processTubeSaturation /
// processTapeSaturation / processTransformerSaturation) to push the harmonics that
// distortion generates above the original Nyquist before they alias back down.
// Flip to 1 for an un-oversampled A/B comparison; never ship it that way.
#define HP_TUBETAPE_FORCE_1X 0

class TubeTapeSaturationProcessor : public juce::AudioProcessor
{
public:
    TubeTapeSaturationProcessor();
    ~TubeTapeSaturationProcessor() override = default;

    static constexpr int kOversamplingFactor = 4;

    // AudioProcessor interface
    void prepareToPlay(double sampleRate, int) override;
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

    // Editor size persistence (see getStateInformation/setStateInformation)
    int getEditorWidth() const noexcept { return editorWidth.load(); }
    int getEditorHeight() const noexcept { return editorHeight.load(); }
    void setEditorSize(int w, int h) noexcept { editorWidth.store(w); editorHeight.store(h); }

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

    // Audio processing. processBlock splits host blocks larger than the prepared size into
    // chunks of at most preparedBlockSize samples (the oversampler and the per-sample
    // parameter buffers are sized for that) and hands each to processChunk or delayDryOnly.
    void processChunk(juce::dsp::AudioBlock<float> block);
    void delayDryOnly(juce::dsp::AudioBlock<float> block);
    void updateShelfFilters(float warmth, float brightness);
    void calculateHarmonicContent(const juce::AudioBuffer<float>& buffer);

    // Saturation algorithms. `channel` selects that channel's hysteresis state.
    float processTubeSaturation(float input, float drive, float warmth, float brightness);
    float processTapeSaturation(float input, float drive, float warmth, float brightness);
    float processTransformerSaturation(float input, float drive, float warmth, float brightness, int channel);

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
    int preparedBlockSize = 0;
    float previousWarmth = -1.0f;
    float previousBrightness = -1.0f;

    // Parameter smoothing (30 ms). Drive, warmth and brightness feed the waveshaper sample by
    // sample (and the shelf filters every kShelfUpdateInterval samples while they move); output
    // level is a per-sample gain.
    static constexpr int kShelfUpdateInterval = 32;
    juce::SmoothedValue<float> driveSmoothed, warmthSmoothed, brightnessSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outputGainSmoothed;

    // Per-base-rate-sample parameter values for the current chunk (sized preparedBlockSize),
    // read by the oversampled waveshaper loop at index sample / kOversamplingFactor.
    std::vector<float> driveValues, warmthValues, brightnessValues;

    // Transformer hysteresis memory, one per channel (was a function-level static shared by
    // every channel and every instance in the session).
    static constexpr int kMaxChannels = 2;
    std::array<float, kMaxChannels> hysteresisMemory {};

    // Harmonic content analysis
    std::atomic<float> harmonicContent { 0.0f };
    std::atomic<float> inputLevel { 0.0f };
    std::atomic<float> outputLevel { 0.0f };
    float previousInputRMS = 0.0f;
    float previousOutputRMS = 0.0f;

    // DC blocking filters
    juce::IIRFilter dcBlockLeft, dcBlockRight;

    // 4x oversampling around the waveshaper only (see HP_TUBETAPE_FORCE_1X above).
    // Rebuilt in prepareToPlay (channel count / block size are only known there) with integer
    // latency; never touched from processBlock, so no audio-thread allocation.
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;

    // Delays bypassed audio by the oversampler's integer latency, so the timing the host
    // compensates for (setLatencySamples) does not jump when Bypass is toggled.
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay;

    // Editor size persistence, read/written from the message thread only.
    std::atomic<int> editorWidth { 0 };
    std::atomic<int> editorHeight { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TubeTapeSaturationProcessor)
};
