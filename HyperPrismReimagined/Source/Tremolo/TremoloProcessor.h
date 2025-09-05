//==============================================================================
// HyperPrism Revived - Tremolo Processor
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
        void prepare(double sampleRate)
        {
            this->sampleRate = static_cast<float>(sampleRate);
            phase = 0.0f;
        }
        
        float process(float rate, Waveform waveform)
        {
            // Update phase
            phase += rate / sampleRate;
            if (phase >= 1.0f)
                phase -= 1.0f;
            
            // Generate waveform
            switch (waveform)
            {
                case Waveform::Sine:
                    return std::sin(2.0f * juce::MathConstants<float>::pi * phase);
                    
                case Waveform::Triangle:
                {
                    // Triangle wave: rises from -1 to 1 in first half, falls from 1 to -1 in second half
                    if (phase < 0.5f)
                        return 4.0f * phase - 1.0f;
                    else
                        return 3.0f - 4.0f * phase;
                }
                    
                case Waveform::Square:
                    return phase < 0.5f ? 1.0f : -1.0f;
                    
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
    
    // LFOs for each channel
    LFO lfoLeft;
    LFO lfoRight;
    
    // Parameter smoothing
    juce::SmoothedValue<float> rateSmoothed;
    juce::SmoothedValue<float> depthSmoothed;
    juce::SmoothedValue<float> mixSmoothed;
    
    double currentSampleRate = 44100.0;
    
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TremoloProcessor)
};