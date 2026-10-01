#pragma once

#include <JuceHeader.h>

class NoiseGateProcessor : public juce::AudioProcessor
{
public:
    NoiseGateProcessor();
    ~NoiseGateProcessor() override;

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
    // reads and writes them directly. IDs, ranges, defaults and order are unchanged:
    // threshold, attack, hold, release, range, lookahead, bypass.
    juce::AudioParameterFloat* threshold = nullptr;
    juce::AudioParameterFloat* attack = nullptr;
    juce::AudioParameterFloat* hold = nullptr;
    juce::AudioParameterFloat* release = nullptr;
    juce::AudioParameterFloat* range = nullptr;
    juce::AudioParameterFloat* lookahead = nullptr;

    juce::AudioProcessorValueTreeState& getValueTreeState() { return valueTreeState; }
    int getEditorWidth() const { return editorWidth.load(); }
    int getEditorHeight() const { return editorHeight.load(); }
    void setEditorSize(int w, int h) { editorWidth = w; editorHeight = h; }
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Old sessions carry an XmlElement tagged "NoiseGateState" with one attribute per
    // float parameter (bypass was never saved); the APVTS tree is typed "NoiseGate" so
    // setStateInformation can tell the two apart.
    static constexpr const char* legacyStateTag = "NoiseGateState";
    static constexpr const char* stateType = "NoiseGate";
    
    // Get gate status for LED
    bool isGateOpen() const { return gateOpen; }

private:
    // DSP members
    double currentSampleRate;
    
    // Envelope follower state per channel
    std::vector<float> envelopeState;
    
    // Gate state per channel
    std::vector<float> gateState;
    std::vector<int> holdCounter;
    
    // Lookahead. The audio is always delayed by the maximum lookahead (kMaxLookaheadMs),
    // which is the latency reported to the host, so it never changes with the Lookahead
    // setting or the block size. The detector reads the same per-channel ring buffer
    // `lookahead` ms nearer the input, so the gate reacts that much before the audio arrives.
    static constexpr double kMaxLookaheadMs = 10.0;
    int maxDelaySamples = 0;
    int ringSize = 1;
    int writePosition = 0;
    std::vector<float> delayRing; // ringSize samples per channel, pre-allocated

    // Bypass
    juce::AudioParameterBool* bypassParamBool = nullptr;

    // Gate status
    std::atomic<bool> gateOpen;

    // Declared after everything it does not depend on, and last among the members the
    // constructor initialises, so the init list order matches declaration order.
    juce::AudioProcessorValueTreeState valueTreeState;
    
    // Helper functions
    float dbToLinear(float db) const;
    float linearToDb(float linear) const;

    // Editor size persistence
    std::atomic<int> editorWidth { 0 }, editorHeight { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoiseGateProcessor)
};