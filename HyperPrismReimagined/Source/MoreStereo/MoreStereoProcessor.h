//==============================================================================
// HyperPrism Reimagined - More Stereo Processor
//==============================================================================

#pragma once

#include <JuceHeader.h>

class MoreStereoProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    MoreStereoProcessor();
    ~MoreStereoProcessor() override = default;

    //==============================================================================
    void prepareToPlay(double sampleRate, int) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================================
    const juce::String getName() const override { return "HyperPrism Reimagined More Stereo"; }

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

    // Editor size persistence
    int getEditorWidth() const { return editorWidth.load(); }
    int getEditorHeight() const { return editorHeight.load(); }
    void setEditorSize(int w, int h) { editorWidth.store(w); editorHeight.store(h); }

    // Parameter IDs
    static const juce::String BYPASS_ID;
    static const juce::String WIDTH_ID;
    static const juce::String BASS_MONO_ID;
    static const juce::String CROSSOVER_FREQ_ID;
    static const juce::String STEREO_ENHANCE_ID;
    static const juce::String AMBIENCE_ID;
    static const juce::String OUTPUT_LEVEL_ID;
    
    // Metering
    float getLeftLevel() const { return leftLevel.load(); }
    float getRightLevel() const { return rightLevel.load(); }
    float getStereoWidth() const { return stereoWidth.load(); }
    float getAmbienceLevel() const { return ambienceLevel.load(); }

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    // Processes samples [start, start + numSamples) of buffer; numSamples <= preparedBlockSize.
    void processMoreStereo(juce::AudioBuffer<float>& buffer, int start, int numSamples,
                           float& leftLevelSum, float& rightLevelSum, float& ambienceLevelSum);
    void calculateStereoWidth(const juce::AudioBuffer<float>& buffer);
    
    juce::AudioProcessorValueTreeState valueTreeState;
    std::atomic<int> editorWidth { 0 }, editorHeight { 0 };

    // Parameter pointers for performance
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* widthParam = nullptr;
    std::atomic<float>* bassMonoParam = nullptr;
    std::atomic<float>* crossoverFreqParam = nullptr;
    std::atomic<float>* stereoEnhanceParam = nullptr;
    std::atomic<float>* ambienceParam = nullptr;
    std::atomic<float>* outputLevelParam = nullptr;
    
    // Bass/treble split: a 4th-order Linkwitz-Riley crossover, whose two bands sum flat. The
    // old Butterworth low-pass + high-pass pair at the same frequency notched the crossover.
    juce::dsp::LinkwitzRileyFilter<float> crossover;

    // Parameter smoothing (30 ms); the crossover moves per sample while it ramps.
    juce::LinearSmoothedValue<float> widthSmoothed, bassMonoSmoothed, stereoEnhanceSmoothed, ambienceSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> crossoverSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> outputGainSmoothed;
    
    // Ambience processing
    juce::dsp::Reverb reverb;
    juce::dsp::DelayLine<float> ambienceDelayLeft { 4800 };
    juce::dsp::DelayLine<float> ambienceDelayRight { 4800 };
    
    // Pre-allocated reverb input buffer (real-time safe), sized preparedBlockSize.
    juce::AudioBuffer<float> ambienceBuffer;
    int preparedBlockSize = 0;

    // State variables
    double currentSampleRate = 44100.0;
    
    // Metering
    std::atomic<float> leftLevel { 0.0f };
    std::atomic<float> rightLevel { 0.0f };
    std::atomic<float> stereoWidth { 0.0f };
    std::atomic<float> ambienceLevel { 0.0f };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MoreStereoProcessor)
};