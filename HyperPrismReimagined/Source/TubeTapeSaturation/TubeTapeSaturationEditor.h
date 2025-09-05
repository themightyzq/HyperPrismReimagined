//==============================================================================
// HyperPrism Revived - Tube/Tape Saturation Editor
// Updated to match AutoPan template exactly
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "TubeTapeSaturationProcessor.h"
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
// Saturation visualizer - shows harmonic content and saturation characteristics
//==============================================================================
class SaturationMeter : public juce::Component, private juce::Timer
{
public:
    explicit SaturationMeter(TubeTapeSaturationProcessor& processor);
    ~SaturationMeter() override;
    
    void paint(juce::Graphics& g) override;
    
private:
    void timerCallback() override;
    
    TubeTapeSaturationProcessor& processor;
    float harmonicContent = 0.0f;
    float inputLevel = 0.0f;
    float outputLevel = 0.0f;
    int saturationType = 0;
    float drive = 0.0f;
    std::vector<float> harmonicSpectrum;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SaturationMeter)
};

//==============================================================================
// Main Editor
//==============================================================================
class TubeTapeSaturationEditor : public juce::AudioProcessorEditor
{
public:
    TubeTapeSaturationEditor(TubeTapeSaturationProcessor&);
    ~TubeTapeSaturationEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void setupSlider(juce::Slider& slider, ParameterLabel& label, 
                    const juce::String& text, const juce::String& suffix);
    void setupComboBox(juce::ComboBox& comboBox, ParameterLabel& label, 
                      const juce::String& text);
    void updateXYPadFromParameters();
    void updateParametersFromXYPad(float x, float y);
    void showParameterMenu(juce::Label* label, const juce::String& parameterID);
    void updateParameterColors();
    void updateXYPadLabel();
    
    TubeTapeSaturationProcessor& audioProcessor;
    HyperPrismLookAndFeel customLookAndFeel;
    
    // Title
    juce::Label titleLabel;
    
    // Bypass
    juce::ToggleButton bypassButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    
    // Parameter controls with ParameterLabel for right-click assignment
    juce::Slider driveSlider;
    ParameterLabel driveLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> driveAttachment;
    
    juce::Slider warmthSlider;
    ParameterLabel warmthLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> warmthAttachment;
    
    juce::Slider brightnessSlider;
    ParameterLabel brightnessLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> brightnessAttachment;
    
    juce::Slider outputLevelSlider;
    ParameterLabel outputLevelLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputLevelAttachment;
    
    juce::ComboBox typeComboBox;
    ParameterLabel typeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> typeAttachment;
    
    // XY Pad
    XYPad xyPad;
    juce::Label xyPadLabel;
    
    // Saturation meter
    SaturationMeter saturationMeter;
    juce::Label saturationMeterLabel;
    
    // X/Y Pad parameter assignments (support multiple parameters per axis)
    juce::StringArray xParameterIDs;
    juce::StringArray yParameterIDs;
    
    // Color coding for assignments
    const juce::Colour xAssignmentColor = juce::Colour(0, 150, 255);   // Blue
    const juce::Colour yAssignmentColor = juce::Colour(255, 220, 0);   // Yellow

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TubeTapeSaturationEditor)
};