//==============================================================================
// HyperPrism Reimagined - Chorus Processor
//==============================================================================

#pragma once

#include <JuceHeader.h>

class ChorusProcessor : public juce::AudioProcessor
{
public:
    ChorusProcessor();
    ~ChorusProcessor() override = default;

    // AudioProcessor interface
    void prepareToPlay(double sampleRate, int) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages) override;
    
    // Editor
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    
    // Program/State
    const juce::String getName() const override { return "HyperPrism Reimagined Chorus"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }
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

    // Parameter IDs
    static const juce::String BYPASS_ID;
    static const juce::String MIX_ID;
    static const juce::String RATE_ID;
    static const juce::String DEPTH_ID;
    static const juce::String FEEDBACK_ID;
    static const juce::String DELAY_ID;
    static const juce::String LOW_CUT_ID;
    static const juce::String HIGH_CUT_ID;

    // Editor size, persisted with the session as editor_width/editor_height properties
    // (0 = never set, use the editor default). Atomics: a host may call getStateInformation
    // off the message thread while the editor's resized() writes here.
    int  getEditorWidth()  const { return editorWidth.load(); }
    int  getEditorHeight() const { return editorHeight.load(); }
    void setEditorSize(int w, int h) { editorWidth.store(w); editorHeight.store(h); }

private:
    std::atomic<int> editorWidth { 0 };
    std::atomic<int> editorHeight { 0 };
    // Parameter layout
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    
    // Audio processing
    void processChorus(juce::AudioBuffer<float>& buffer);
    void updateFilters();
    
    // Chorus delay line class
    class ChorusDelayLine
    {
    public:
        void prepare(double newSampleRate, float maxDelayMs);
        void reset();
        float processSample(float input, float delayMs, float feedback);
        
    private:
        juce::AudioBuffer<float> buffer;
        int writeIndex = 0;
        int maxDelaySamples = 0;
        double sampleRate = 44100.0;
    };
    
    // State
    juce::AudioProcessorValueTreeState valueTreeState;
    
    // DSP components
    ChorusDelayLine leftDelayLine, rightDelayLine;
    juce::IIRFilter leftLowCut, rightLowCut;
    juce::IIRFilter leftHighCut, rightHighCut;
    
    // LFO for modulation
    float lfoPhase = 0.0f;
    float lfoPhaseRight = 0.0f;
    
    // Cached parameters
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* rateParam = nullptr;
    std::atomic<float>* depthParam = nullptr;
    std::atomic<float>* feedbackParam = nullptr;
    std::atomic<float>* delayParam = nullptr;
    std::atomic<float>* lowCutParam = nullptr;
    std::atomic<float>* highCutParam = nullptr;
    
    // Pre-allocated dry buffer (real-time safe)
    juce::AudioBuffer<float> dryBuffer;

    // Processing state
    double currentSampleRate = 44100.0;
    float previousLowCutFreq = -1.0f;
    float previousHighCutFreq = -1.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChorusProcessor)
};