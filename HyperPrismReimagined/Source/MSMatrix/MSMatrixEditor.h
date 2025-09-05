//==============================================================================
// HyperPrism Revived - M+S Matrix Editor
// Updated to match AutoPan template exactly
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "MSMatrixProcessor.h"
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
// M/S Meter - shows Mid/Side levels in stereo field visualization
//==============================================================================
class MSMeter : public juce::Component, private juce::Timer
{
public:
    explicit MSMeter(MSMatrixProcessor& processor);
    ~MSMeter() override;
    
    void paint(juce::Graphics& g) override;
    
private:
    void timerCallback() override;
    
    MSMatrixProcessor& processor;
    float leftLevel = 0.0f;
    float rightLevel = 0.0f;
    float midLevel = 0.0f;
    float sideLevel = 0.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MSMeter)
};

//==============================================================================
// Main Editor
//==============================================================================
class MSMatrixEditor : public juce::AudioProcessorEditor
{
public:
    MSMatrixEditor(MSMatrixProcessor&);
    ~MSMatrixEditor() override;

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
    
    MSMatrixProcessor& audioProcessor;
    HyperPrismLookAndFeel customLookAndFeel;
    
    // Title
    juce::Label titleLabel;
    
    // Bypass
    juce::ToggleButton bypassButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    
    // Matrix mode selector
    juce::ComboBox matrixModeComboBox;
    juce::Label matrixModeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> matrixModeAttachment;
    
    // Parameter controls with ParameterLabel for right-click assignment
    juce::Slider midLevelSlider;
    ParameterLabel midLevelLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> midLevelAttachment;
    
    juce::Slider sideLevelSlider;
    ParameterLabel sideLevelLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sideLevelAttachment;
    
    juce::Slider stereoBalanceSlider;
    ParameterLabel stereoBalanceLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> stereoBalanceAttachment;
    
    juce::Slider outputLevelSlider;
    ParameterLabel outputLevelLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputLevelAttachment;
    
    // Solo buttons
    juce::ToggleButton midSoloButton;
    juce::Label midSoloLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> midSoloAttachment;
    
    juce::ToggleButton sideSoloButton;
    juce::Label sideSoloLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> sideSoloAttachment;
    
    // XY Pad
    XYPad xyPad;
    juce::Label xyPadLabel;
    
    // M/S Meter
    MSMeter msMeter;
    juce::Label msMeterLabel;
    
    // X/Y Pad parameter assignments (support multiple parameters per axis)
    juce::StringArray xParameterIDs;
    juce::StringArray yParameterIDs;
    
    // Color coding for assignments
    const juce::Colour xAssignmentColor = juce::Colour(0, 150, 255);   // Blue
    const juce::Colour yAssignmentColor = juce::Colour(255, 220, 0);   // Yellow

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MSMatrixEditor)
};