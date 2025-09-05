//==============================================================================
// HyperPrism Revived - More Stereo Editor
// Updated to match AutoPan template exactly
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "MoreStereoProcessor.h"
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
// Enhanced stereo meter - shows L/R levels, stereo width, and ambience
//==============================================================================
class EnhancedStereoMeter : public juce::Component, private juce::Timer
{
public:
    explicit EnhancedStereoMeter(MoreStereoProcessor& processor);
    ~EnhancedStereoMeter() override;
    
    void paint(juce::Graphics& g) override;
    
private:
    void timerCallback() override;
    
    MoreStereoProcessor& processor;
    float leftLevel = 0.0f;
    float rightLevel = 0.0f;
    float stereoWidth = 0.0f;
    float ambienceLevel = 0.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnhancedStereoMeter)
};

//==============================================================================
// Main Editor
//==============================================================================
class MoreStereoEditor : public juce::AudioProcessorEditor
{
public:
    MoreStereoEditor(MoreStereoProcessor&);
    ~MoreStereoEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void setupControls();
    void setupXYPad();
    void setupSlider(juce::Slider& slider, ParameterLabel& label, 
                    const juce::String& text, const juce::String& suffix);
    void updateXYPadFromParameters();
    void updateParametersFromXYPad(float x, float y);
    void showParameterMenu(juce::Label* label, const juce::String& parameterID);
    void updateParameterColors();
    void updateXYPadLabel();
    
    MoreStereoProcessor& audioProcessor;
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
    
    juce::Slider bassMonoSlider;
    ParameterLabel bassMonoLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bassMonoAttachment;
    
    juce::Slider crossoverFreqSlider;
    ParameterLabel crossoverFreqLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> crossoverFreqAttachment;
    
    juce::Slider stereoEnhanceSlider;
    ParameterLabel stereoEnhanceLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stereoEnhanceAttachment;
    
    juce::Slider ambienceSlider;
    ParameterLabel ambienceLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ambienceAttachment;
    
    juce::Slider outputLevelSlider;
    ParameterLabel outputLevelLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputLevelAttachment;
    
    // XY Pad
    XYPad xyPad;
    juce::Label xyPadLabel;
    
    // Enhanced stereo meter
    EnhancedStereoMeter enhancedStereoMeter;
    juce::Label enhancedStereoMeterLabel;
    
    // X/Y Pad parameter assignments (support multiple parameters per axis)
    juce::StringArray xParameterIDs;
    juce::StringArray yParameterIDs;
    
    // Color coding for assignments
    const juce::Colour xAssignmentColor = juce::Colour(0, 150, 255);   // Blue
    const juce::Colour yAssignmentColor = juce::Colour(255, 220, 0);   // Yellow

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MoreStereoEditor)
};