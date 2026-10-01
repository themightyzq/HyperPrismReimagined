//==============================================================================
// HyperPrism Reimagined - Tremolo Processor
//==============================================================================

#pragma once

#include <JuceHeader.h>

class TremoloProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    TremoloProcessor();
    ~TremoloProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

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
    static const juce::String RATE_ID;
    static const juce::String DEPTH_ID;
    static const juce::String WAVEFORM_ID;
    static const juce::String STEREO_PHASE_ID;
    static const juce::String MIX_ID;

    // Waveform types
    enum class Waveform
    {
        Sine = 0,
        Triangle,
        Square
    };

private:
    //==============================================================================
    juce::AudioProcessorValueTreeState valueTreeState;
    
    // LFO implementation
    class LFO
    {
    public:
        void prepare(double newSampleRate)
        {
            sampleRate = static_cast<float>(newSampleRate);
            phase = 0.0f;
        }
        
        // Advances the phase by one sample and returns the waveform at the new phase.
        float process(float rate, Waveform waveform)
        {
            // Update phase
            phase += rate / sampleRate;
            if (phase >= 1.0f)
                phase -= 1.0f;

            return shape(phase, waveform);
        }

        // The waveform at `atPhase` (0..1), without touching the LFO's own phase.
        static float shape(float atPhase, Waveform waveform)
        {
            // Generate waveform
            switch (waveform)
            {
                case Waveform::Sine:
                    return std::sin(2.0f * juce::MathConstants<float>::pi * atPhase);
                    
                case Waveform::Triangle:
                {
                    // Triangle wave: rises from -1 to 1 in first half, falls from 1 to -1 in second half
                    if (atPhase < 0.5f)
                        return 4.0f * atPhase - 1.0f;
                    else
                        return 3.0f - 4.0f * atPhase;
                }
                    
                case Waveform::Square:
                    return atPhase < 0.5f ? 1.0f : -1.0f;
                    
                default:
                    return 0.0f;
            }
        }
        
        void setPhase(float newPhase) { phase = newPhase; }
        float getPhase() const { return phase; }
        
        void reset() { phase = 0.0f; }
        
    private:
        float phase = 0.0f;
        float sampleRate = 44100.0f;
    };
    
    // One LFO; the right channel reads it at (phase + Stereo Phase), sample by sample, so the
    // left/right relationship is exactly the Stereo Phase setting at any block size. (A second
    // LFO used to be re-synced from this one's end-of-block phase, which made it depend on
    // the block size.)
    LFO lfoLeft;
    
    // Parameter smoothing
    juce::SmoothedValue<float> rateSmoothed;
    juce::SmoothedValue<float> depthSmoothed;
    juce::SmoothedValue<float> mixSmoothed;
    
    double currentSampleRate = 44100.0;

    juce::AudioBuffer<float> dryBuffer;

    // Largest block handled in one piece (host blocks above this are chunked) and the
    // per-sample smoothed parameter values for one chunk, allocated in prepareToPlay
    int preparedBlockSize = 512;
    std::vector<float> rateValues, depthValues, mixValues;

    void processChunk(juce::AudioBuffer<float>& chunk, Waveform waveform, float stereoPhase);

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Editor size persistence
    std::atomic<int> editorWidth { 0 }, editorHeight { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TremoloProcessor)
};