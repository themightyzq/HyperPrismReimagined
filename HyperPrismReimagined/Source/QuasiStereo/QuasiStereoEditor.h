//==============================================================================
// HyperPrism Revived - Quasi Stereo Editor
// Updated to match AutoPan template exactly
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "QuasiStereoProcessor.h"
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
// Stereo width meter - shows L/R levels and stereo width
//==============================================================================
class StereoWidthMeter : public juce::Component, private juce::Timer
{
public:
    explicit StereoWidthMeter(QuasiStereoProcessor& processor);
    ~StereoWidthMeter() override;
    
    void paint(juce::Graphics& g) override;
    
private:
    void timerCallback() override;
    
    QuasiStereoProcessor& processor;
    float leftLevel = 0.0f;
    float rightLevel = 0.0f;
    float stereoWidth = 0.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StereoWidthMeter)
};

//==============================================================================
// Main Editor
//==============================================================================
class QuasiStereoEditor : public juce::AudioProcessorEditor
{
public:
    QuasiStereoEditor(QuasiStereoProcessor&);
    ~QuasiStereoEditor() override;

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
    
    QuasiStereoProcessor& audioProcessor;
    HyperPrismLookAndFeel customLookAndFeel;
    
    // Title
    juce::Label titleLabel;
    
    // Bypass
    juce::ToggleButton bypassButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    
    // Parameter controls with ParameterLabel for right-click assignment
    juce::Slider widthSlider;
    ParameterLabel widthLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> widthAttachment;
    
    juce::Slider delayTimeSlider;
    ParameterLabel delayTimeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> delayTimeAttachment;
    
    juce::Slider frequencyShiftSlider;
    ParameterLabel frequencyShiftLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> frequencyShiftAttachment;
    
    juce::Slider phaseShiftSlider;
    ParameterLabel phaseShiftLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> phaseShiftAttachment;
    
    juce::Slider highFreqEnhanceSlider;
    ParameterLabel highFreqEnhanceLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> highFreqEnhanceAttachment;
    
    juce::Slider outputLevelSlider;
    ParameterLabel outputLevelLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputLevelAttachment;
    
    // XY Pad
    XYPad xyPad;
    juce::Label xyPadLabel;
    
    // Stereo width meter
    StereoWidthMeter stereoWidthMeter;
    juce::Label stereoWidthMeterLabel;
    
    // X/Y Pad parameter assignments (support multiple parameters per axis)
    juce::StringArray xParameterIDs;
    juce::StringArray yParameterIDs;
    
    // Color coding for assignments
    const juce::Colour xAssignmentColor = juce::Colour(0, 150, 255);   // Blue
    const juce::Colour yAssignmentColor = juce::Colour(255, 220, 0);   // Yellow

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(QuasiStereoEditor)
};