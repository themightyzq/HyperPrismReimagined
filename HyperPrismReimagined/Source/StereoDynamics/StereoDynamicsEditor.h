//==============================================================================
// HyperPrism Revived - Stereo Dynamics Editor
// Updated to match AutoPan template exactly
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "StereoDynamicsProcessor.h"
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
// Stereo dynamics visualizer - shows M/S levels and gain reduction
//==============================================================================
class StereoDynamicsMeter : public juce::Component, private juce::Timer
{
public:
    explicit StereoDynamicsMeter(StereoDynamicsProcessor& processor);
    ~StereoDynamicsMeter() override;
    
    void paint(juce::Graphics& g) override;
    
private:
    void timerCallback() override;
    
    StereoDynamicsProcessor& processor;
    float leftLevel = 0.0f;
    float rightLevel = 0.0f;
    float midLevel = 0.0f;
    float sideLevel = 0.0f;
    float midGainReduction = 0.0f;
    float sideGainReduction = 0.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StereoDynamicsMeter)
};

//==============================================================================
// Main Editor
//==============================================================================
class StereoDynamicsEditor : public juce::AudioProcessorEditor
{
public:
    StereoDynamicsEditor(StereoDynamicsProcessor&);
    ~StereoDynamicsEditor() override;

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
    
    StereoDynamicsProcessor& audioProcessor;
    HyperPrismLookAndFeel customLookAndFeel;
    
    // Title
    juce::Label titleLabel;
    
    // Bypass
    juce::ToggleButton bypassButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    
    // Parameter controls with ParameterLabel for right-click assignment
    juce::Slider midThresholdSlider;
    ParameterLabel midThresholdLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> midThresholdAttachment;
    
    juce::Slider midRatioSlider;
    ParameterLabel midRatioLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> midRatioAttachment;
    
    juce::Slider sideThresholdSlider;
    ParameterLabel sideThresholdLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sideThresholdAttachment;
    
    juce::Slider sideRatioSlider;
    ParameterLabel sideRatioLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sideRatioAttachment;
    
    juce::Slider attackTimeSlider;
    ParameterLabel attackTimeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackTimeAttachment;
    
    juce::Slider releaseTimeSlider;
    ParameterLabel releaseTimeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseTimeAttachment;
    
    juce::Slider outputLevelSlider;
    ParameterLabel outputLevelLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputLevelAttachment;
    
    // XY Pad
    XYPad xyPad;
    juce::Label xyPadLabel;
    
    // Stereo dynamics meter
    StereoDynamicsMeter stereoDynamicsMeter;
    juce::Label stereoDynamicsMeterLabel;
    
    // X/Y Pad parameter assignments (support multiple parameters per axis)
    juce::StringArray xParameterIDs;
    juce::StringArray yParameterIDs;
    
    // Color coding for assignments
    const juce::Colour xAssignmentColor = juce::Colour(0, 150, 255);   // Blue
    const juce::Colour yAssignmentColor = juce::Colour(255, 220, 0);   // Yellow

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StereoDynamicsEditor)
};