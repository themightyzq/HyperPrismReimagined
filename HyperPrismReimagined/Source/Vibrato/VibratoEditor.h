//==============================================================================
// HyperPrism Revived - Vibrato Editor
// Updated to match AutoPan template exactly
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "VibratoProcessor.h"
#include "../Shared/HyperPrismLookAndFeel.h"

//==============================================================================
// Clickable parameter label for X/Y assignment
//==============================================================================
class ParameterLabel : public juce::Label
{
public:
    ParameterLabel() = default;
    
    void mouseDown(const juce::MouseEvent& event) override
    {
        if (event.mods.isRightButtonDown() && onClick)
            onClick();
        else
            juce::Label::mouseDown(event);
    }
    
    std::function<void()> onClick;
};

//==============================================================================
// XY Pad component (matching AutoPan style)
//==============================================================================
class XYPad : public juce::Component
{
public:
    XYPad();
    
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    
    void setValues(float x, float y);
    void setAxisColors(const juce::Colour& xColor, const juce::Colour& yColor);
    
    std::function<void(float, float)> onValueChange;
    
private:
    void updatePosition(const juce::MouseEvent& event);
    
    float xValue = 0.5f;
    float yValue = 0.5f;
    juce::Colour xAxisColor = juce::Colour(0, 150, 255);   // Blue
    juce::Colour yAxisColor = juce::Colour(255, 220, 0);    // Yellow
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XYPad)
};

//==============================================================================
// Vibrato visualizer - shows LFO modulation and delay feedback
//==============================================================================
class VibratoMeter : public juce::Component, private juce::Timer
{
public:
    explicit VibratoMeter(VibratoProcessor& processor);
    ~VibratoMeter() override;
    
    void paint(juce::Graphics& g) override;
    
private:
    void timerCallback() override;
    
    VibratoProcessor& processor;
    float currentPhase = 0.0f;
    float rate = 5.0f;
    float depth = 0.5f;
    float delay = 10.0f;
    float feedback = 0.0f;
    std::vector<float> lfoWaveform;
    std::vector<float> delayBuffer;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VibratoMeter)
};

//==============================================================================
// Main Editor
//==============================================================================
class VibratoEditor : public juce::AudioProcessorEditor
{
public:
    VibratoEditor(VibratoProcessor&);
    ~VibratoEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void setupSlider(juce::Slider& slider, ParameterLabel& label, 
                    const juce::String& text, const juce::String& suffix);
    void updateXYPadFromParameters();
    void updateParametersFromXYPad(float x, float y);
    void showParameterMenu(juce::Label* label, const juce::String& parameterID);
    void updateParameterColors();
    void updateXYPadLabel();
    
    VibratoProcessor& audioProcessor;
    HyperPrismLookAndFeel customLookAndFeel;
    
    // Title
    juce::Label titleLabel;
    
    // Bypass
    juce::ToggleButton bypassButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    
    // Parameter controls with ParameterLabel for right-click assignment
    juce::Slider mixSlider;
    ParameterLabel mixLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    
    juce::Slider rateSlider;
    ParameterLabel rateLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rateAttachment;
    
    juce::Slider depthSlider;
    ParameterLabel depthLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> depthAttachment;
    
    juce::Slider delaySlider;
    ParameterLabel delayLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayAttachment;
    
    juce::Slider feedbackSlider;
    ParameterLabel feedbackLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> feedbackAttachment;
    
    // XY Pad
    XYPad xyPad;
    juce::Label xyPadLabel;
    
    // Vibrato meter
    VibratoMeter vibratoMeter;
    juce::Label vibratoMeterLabel;
    
    // X/Y Pad parameter assignments (support multiple parameters per axis)
    juce::StringArray xParameterIDs;
    juce::StringArray yParameterIDs;
    
    // Color coding for assignments
    const juce::Colour xAssignmentColor = juce::Colour(0, 150, 255);   // Blue
    const juce::Colour yAssignmentColor = juce::Colour(255, 220, 0);   // Yellow

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VibratoEditor)
};