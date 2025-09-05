//==============================================================================
// HyperPrism Revived - Noise Gate Editor
// Updated to match AutoPan template exactly
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "NoiseGateProcessor.h"
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
// Gate LED - shows gate open/closed status
//==============================================================================
class GateLED : public juce::Component, private juce::Timer
{
public:
    explicit GateLED(NoiseGateProcessor& processor);
    ~GateLED() override;
    
    void paint(juce::Graphics& g) override;
    
private:
    void timerCallback() override;
    
    NoiseGateProcessor& processor;
    bool isGateOpen = false;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GateLED)
};

//==============================================================================
// Main Editor
//==============================================================================
class NoiseGateEditor : public juce::AudioProcessorEditor
{
public:
    NoiseGateEditor(NoiseGateProcessor&);
    ~NoiseGateEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void setupSlider(juce::Slider& slider, ParameterLabel& label, 
                    const juce::String& text, const juce::String& suffix);
    void updateXYPadFromParameters();
    void updateParametersFromXYPad(float x, float y);
    void showParameterMenu(juce::Label* label, const juce::String& parameterName);
    void updateParameterColors();
    void updateXYPadLabel();
    
    NoiseGateProcessor& audioProcessor;
    HyperPrismLookAndFeel customLookAndFeel;
    
    // Title
    juce::Label titleLabel;
    
    // Bypass
    juce::ToggleButton bypassButton;
    
    // Parameter controls with ParameterLabel for right-click assignment
    juce::Slider thresholdSlider;
    ParameterLabel thresholdLabel;
    
    juce::Slider attackSlider;
    ParameterLabel attackLabel;
    
    juce::Slider holdSlider;
    ParameterLabel holdLabel;
    
    juce::Slider releaseSlider;
    ParameterLabel releaseLabel;
    
    juce::Slider rangeSlider;
    ParameterLabel rangeLabel;
    
    juce::Slider lookaheadSlider;
    ParameterLabel lookaheadLabel;
    
    // XY Pad
    XYPad xyPad;
    juce::Label xyPadLabel;
    
    // Gate LED
    GateLED gateLED;
    juce::Label gateLEDLabel;
    
    // X/Y Pad parameter assignments (support multiple parameters per axis)
    juce::StringArray xParameterNames;
    juce::StringArray yParameterNames;
    
    // Color coding for assignments
    const juce::Colour xAssignmentColor = juce::Colour(0, 150, 255);   // Blue
    const juce::Colour yAssignmentColor = juce::Colour(255, 220, 0);   // Yellow

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NoiseGateEditor)
};