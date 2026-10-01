#pragma once

#include <JuceHeader.h>

class LimiterProcessor : public juce::AudioProcessor
{
public:
    LimiterProcessor();
    ~LimiterProcessor() override;

    void prepareToPlay(double sampleRate, int) override;
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

    // Public parameters
    juce::AudioParameterFloat* ceilingParam;
    juce::AudioParameterFloat* releaseParam;
    juce::AudioParameterFloat* lookaheadParam;
    juce::AudioParameterBool* softClipParam;
    juce::AudioParameterFloat* inputGainParam;

    // Get current gain reduction for metering
    float getCurrentGainReduction() const { return currentGainReduction.load(); }
    bool getPeakIndicator() const { return peakIndicator.load(); }
    void resetPeakIndicator() { peakIndicator.store(false); }
    
    // Get the AudioProcessorValueTreeState
    juce::AudioProcessorValueTreeState& getStateInformation() { return apvts; }
    juce::AudioProcessorValueTreeState& getValueTreeState() { return apvts; }

    // Editor size persistence
    int getEditorWidth() const { return editorWidth.load(); }
    int getEditorHeight() const { return editorHeight.load(); }
    void setEditorSize(int w, int h) { editorWidth.store(w); editorHeight.store(h); }

private:
    // Parameter state
    juce::AudioProcessorValueTreeState apvts;
    std::atomic<int> editorWidth { 0 }, editorHeight { 0 };
    std::atomic<float>* bypassParam = nullptr;
    // DSP members
    double currentSampleRate = 44100.0;
    
    // Lookahead. The audio is always delayed by the maximum Lookahead (kMaxLookaheadMs),
    // which is the latency reported to the host, so it never changes with the Lookahead
    // setting or the block size. The gain computer reads the same per-channel ring buffer
    // `lookahead` ms nearer the input, and ramps the gain down over that window, so gain
    // reduction is in place when a peak reaches the output.
    static constexpr double kMaxLookaheadMs = 20.0;
    int maxDelaySamples = 0;
    int ringSize = 1;
    int writePosition = 0;
    std::vector<float> delayRing; // ringSize samples per channel, pre-allocated
    
    // Envelope followers for each channel
    std::vector<float> envelopeFollowers;
    
    // Smoothing for gain changes
    std::vector<float> smoothedGains;

    // Smooths the Release parameter itself (ms) so changing the knob doesn't step the
    // release-time coefficient abruptly; advanced once per block via skip(), see
    // processBlock(). Fix for the "Release parameter has no audible effect" defect.
    juce::SmoothedValue<float> releaseMsSmoothed;

    // Input gain and ceiling, smoothed over 30 ms (per-sample gains).
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> inputGainSmoothed;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> ceilingSmoothed;

    // Metering
    std::atomic<float> currentGainReduction { 0.0f };
    std::atomic<bool> peakIndicator { false };

    // Helper functions
    float softClip(float input);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LimiterProcessor)
};