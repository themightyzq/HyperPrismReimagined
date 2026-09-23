#pragma once

#include <JuceHeader.h>

// 4x oversampling wraps the harmonic generator only (generateWarmHarmonics /
// generateBrightHarmonics) to push the harmonics it manufactures above the original
// Nyquist before they alias back down. Flip to 1 for an un-oversampled A/B comparison;
// never ship it that way.
#define HP_HARMONICEXCITER_FORCE_1X 0

class HarmonicExciterProcessor : public juce::AudioProcessor
{
public:
    HarmonicExciterProcessor();
    ~HarmonicExciterProcessor() override;

    static constexpr int kOversamplingFactor = 4;

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

    // Parameters live in the APVTS (migrated 2026-09-23 from addParameter). The raw
    // pointers are kept, now pointing at the APVTS-owned parameters, because the editor
    // reads and writes them directly at many call sites. IDs, ranges, defaults and order
    // are unchanged: drive, frequency, harmonics, mix, type, bypass.
    juce::AudioParameterFloat* driveParam = nullptr;
    juce::AudioParameterFloat* frequencyParam = nullptr;
    juce::AudioParameterFloat* harmonicsParam = nullptr;
    juce::AudioParameterFloat* mixParam = nullptr;
    juce::AudioParameterChoice* typeParam = nullptr;
    juce::AudioParameterBool* bypassParamBool = nullptr;

    juce::AudioProcessorValueTreeState& getValueTreeState() { return valueTreeState; }
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Old sessions (saved before the APVTS migration) carry an XmlElement tagged
    // "HarmonicExciter" with one attribute per parameter; the APVTS tree is typed
    // "HarmonicExciterState" so setStateInformation can tell the two apart.
    static constexpr const char* legacyStateTag = "HarmonicExciter";
    static constexpr const char* stateType = "HarmonicExciterState";

    // Get current output level for metering
    float getCurrentOutputLevel() const { return outputLevel.load(); }

    // Editor size persistence (see getStateInformation/setStateInformation)
    int getEditorWidth() const noexcept { return editorWidth.load(); }
    int getEditorHeight() const noexcept { return editorHeight.load(); }
    void setEditorSize(int w, int h) noexcept { editorWidth.store(w); editorHeight.store(h); }

private:
    juce::AudioProcessorValueTreeState valueTreeState;

    // Processing components
    juce::dsp::LinkwitzRileyFilter<float> highPassFilter;
    juce::dsp::LinkwitzRileyFilter<float> lowPassFilter;
    
    // Output level for metering
    std::atomic<float> outputLevel { 0.0f };
    
    // Sample rate storage
    double currentSampleRate = 44100.0;

    // Pre-allocated buffers
    juce::AudioBuffer<float> dryBuffer;
    juce::AudioBuffer<float> highFreqBuffer;

    // Harmonic generation functions
    float generateWarmHarmonics(float input, float drive, float harmonics);
    float generateBrightHarmonics(float input, float drive, float harmonics);

    // 4x oversampling around the harmonic generator only (see HP_HARMONICEXCITER_FORCE_1X
    // above). Rebuilt in prepareToPlay; never touched from processBlock.
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;

    // Editor size persistence, read/written from the message thread only.
    std::atomic<int> editorWidth { 0 };
    std::atomic<int> editorHeight { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HarmonicExciterProcessor)
};