#pragma once

#include <JuceHeader.h>

class BassMaximiserProcessor : public juce::AudioProcessor
{
public:
    BassMaximiserProcessor();
    ~BassMaximiserProcessor() override;

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
    juce::AudioParameterFloat* frequencyParam;      // 20-500 Hz cutoff frequency
    juce::AudioParameterFloat* boostParam;          // 0-20 dB bass boost
    juce::AudioParameterFloat* harmonicsParam;      // 0-100% sub-harmonic generation
    juce::AudioParameterFloat* tightnessParam;      // 0-100% compression/limiting on bass
    juce::AudioParameterFloat* outputGainParam;     // -20 to 20 dB output gain
    juce::AudioParameterBool* phaseInvertParam;     // Phase invert for bass frequencies

    // Get current bass level for metering (RMS)
    float getCurrentBassLevel() const { return currentBassLevel.load(); }
    
    // Get the AudioProcessorValueTreeState
    juce::AudioProcessorValueTreeState& getValueTreeState() { return apvts; }

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
    int currentBlockSize = 512;
    
    // Band split at the Frequency parameter: a 4th-order Linkwitz-Riley crossover, whose low
    // and high outputs sum to a flat (all-pass) response. The old split summed a Butterworth
    // low-pass and high-pass at the same frequency, which cancels exactly at the crossover
    // (a notch: 80 Hz with Boost at 0 dB, about 113 Hz at the default +6 dB).
    juce::dsp::LinkwitzRileyFilter<float> crossover;

    // Sub-harmonic (one octave down) generation: an octave divider per channel (a flip-flop
    // toggled on each upward zero crossing of the bass band, with hysteresis), then a
    // low-pass at the crossover frequency to keep only the sub-octave.
    juce::dsp::LinkwitzRileyFilter<float> subFilter;
    float subFlipFlop[2] = { 1.0f, 1.0f };
    bool subArmed[2] = { false, false };
    
    // Bass compression/limiting (tightness control)
    std::vector<float> bassEnvelopes;  // One per channel
    std::vector<float> bassGainReduction;  // One per channel
    
    // Bass level metering
    std::atomic<float> currentBassLevel { 0.0f };
    juce::LinearSmoothedValue<float> bassLevelSmoother;
    
    // Output gain smoothing
    juce::LinearSmoothedValue<float> outputGainSmoother;

    // Frequency, Boost and Harmonics smoothing (30 ms). Frequency moves the crossover
    // per sample while it ramps.
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> frequencySmoother;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> boostGainSmoother;
    juce::LinearSmoothedValue<float> harmonicsSmoother;
    
    // Helper functions
    float generateSubHarmonic(float input, int channel);
    float processBassCompression(float input, float& envelope, float& gainReduction,
                               float tightness);
    float calculateRMS(const float* buffer, int numSamples);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassMaximiserProcessor)
};