//==============================================================================
// HyperPrism Revived - Pitch Changer Editor
// Updated to match AutoPan template exactly
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "PitchChangerProcessor.h"
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
// Pitch meter - shows I/O levels and pitch detection
//==============================================================================
class PitchMeter : public juce::Component, private juce::Timer
{
public:
    explicit PitchMeter(PitchChangerProcessor& processor);
    ~PitchMeter() override;
    
    void paint(juce::Graphics& g) override;
    
private:
    void timerCallback() override;
    
    PitchChangerProcessor& processor;
    float inputLevel = 0.0f;
    float outputLevel = 0.0f;
    float detectedPitch = 0.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchMeter)
};

//==============================================================================
// Main Editor
//==============================================================================
class PitchChangerEditor : public juce::AudioProcessorEditor
{
public:
    PitchChangerEditor(PitchChangerProcessor&);
    ~PitchChangerEditor() override;

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
    
    PitchChangerProcessor& audioProcessor;
    HyperPrismLookAndFeel customLookAndFeel;
    
    // Title
    juce::Label titleLabel;
    
    // Bypass
    juce::ToggleButton bypassButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    
    // Parameter controls with ParameterLabel for right-click assignment
    juce::Slider pitchShiftSlider;
    ParameterLabel pitchShiftLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchShiftAttachment;
    
    juce::Slider fineTuneSlider;
    ParameterLabel fineTuneLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> fineTuneAttachment;
    
    juce::Slider formantShiftSlider;
    ParameterLabel formantShiftLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> formantShiftAttachment;
    
    juce::Slider mixSlider;
    ParameterLabel mixLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    
    juce::Slider outputLevelSlider;
    ParameterLabel outputLevelLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputLevelAttachment;
    
    // XY Pad
    XYPad xyPad;
    juce::Label xyPadLabel;
    
    // Pitch meter
    PitchMeter pitchMeter;
    juce::Label pitchMeterLabel;
    
    // X/Y Pad parameter assignments (support multiple parameters per axis)
    juce::StringArray xParameterIDs;
    juce::StringArray yParameterIDs;
    
    // Color coding for assignments
    const juce::Colour xAssignmentColor = juce::Colour(0, 150, 255);   // Blue
    const juce::Colour yAssignmentColor = juce::Colour(255, 220, 0);   // Yellow

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PitchChangerEditor)
};