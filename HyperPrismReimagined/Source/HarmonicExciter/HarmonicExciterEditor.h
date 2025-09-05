//==============================================================================
// HyperPrism Revived - Harmonic Exciter Editor
// Updated to match AutoPan template exactly
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "HarmonicExciterProcessor.h"
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
// Main Editor
//==============================================================================
class HarmonicExciterEditor : public juce::AudioProcessorEditor
{
public:
    HarmonicExciterEditor(HarmonicExciterProcessor&);
    ~HarmonicExciterEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    // Parameter IDs
    static constexpr auto DRIVE_ID = "drive";
    static constexpr auto FREQUENCY_ID = "frequency";
    static constexpr auto HARMONICS_ID = "harmonics";
    static constexpr auto MIX_ID = "mix";
    static constexpr auto TYPE_ID = "type";
    static constexpr auto BYPASS_ID = "bypass";

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
    
    HarmonicExciterProcessor& audioProcessor;
    HyperPrismLookAndFeel customLookAndFeel;
    
    // Title
    juce::Label titleLabel;
    
    // Bypass
    juce::ToggleButton bypassButton;
    
    // Parameter controls with ParameterLabel for right-click assignment
    juce::Slider driveSlider;
    ParameterLabel driveLabel;
    
    juce::Slider frequencySlider;
    ParameterLabel frequencyLabel;
    
    juce::Slider harmonicsSlider;
    ParameterLabel harmonicsLabel;
    
    juce::Slider mixSlider;
    ParameterLabel mixLabel;
    
    // Type selector
    juce::ComboBox typeComboBox;
    juce::Label typeLabel;
    
    // XY Pad
    XYPad xyPad;
    juce::Label xyPadLabel;
    
    // X/Y Pad parameter assignments (support multiple parameters per axis)
    juce::StringArray xParameterIDs;
    juce::StringArray yParameterIDs;
    
    // Color coding for assignments
    const juce::Colour xAssignmentColor = juce::Colour(0, 150, 255);   // Blue
    const juce::Colour yAssignmentColor = juce::Colour(255, 220, 0);   // Yellow

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HarmonicExciterEditor)
};