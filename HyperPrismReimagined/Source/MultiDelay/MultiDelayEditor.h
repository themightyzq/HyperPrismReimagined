//==============================================================================
// HyperPrism Revived - Multi Delay Editor
// Updated to match AutoPan template with complex layout
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "MultiDelayProcessor.h"
#include "../Shared/HyperPrismLookAndFeel.h"
#include <array>

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
// Multi-delay meter - shows input, 4 delays, and output levels
//==============================================================================
class MultiDelayMeter : public juce::Component, private juce::Timer
{
public:
    explicit MultiDelayMeter(MultiDelayProcessor& processor);
    ~MultiDelayMeter() override;
    
    void paint(juce::Graphics& g) override;
    
private:
    void timerCallback() override;
    
    MultiDelayProcessor& processor;
    float inputLevel = 0.0f;
    float outputLevel = 0.0f;
    std::array<float, 4> delayLevels { 0.0f, 0.0f, 0.0f, 0.0f };
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiDelayMeter)
};

//==============================================================================
// Main Editor
//==============================================================================
class MultiDelayEditor : public juce::AudioProcessorEditor
{
public:
    MultiDelayEditor(MultiDelayProcessor&);
    ~MultiDelayEditor() override;

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
    
    MultiDelayProcessor& audioProcessor;
    HyperPrismLookAndFeel customLookAndFeel;
    
    // Title
    juce::Label titleLabel;
    
    // Bypass
    juce::ToggleButton bypassButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    
    // Global controls with ParameterLabel for right-click assignment
    juce::Slider masterMixSlider;
    ParameterLabel masterMixLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> masterMixAttachment;
    
    juce::Slider globalFeedbackSlider;
    ParameterLabel globalFeedbackLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> globalFeedbackAttachment;
    
    // Delay controls (4 sets) - compact layout
    std::array<juce::Label, 4> delayGroupLabels;
    
    std::array<juce::Slider, 4> delayTimeSliders;
    std::array<ParameterLabel, 4> delayTimeLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> delayTimeAttachments;
    
    std::array<juce::Slider, 4> delayLevelSliders;
    std::array<ParameterLabel, 4> delayLevelLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> delayLevelAttachments;
    
    std::array<juce::Slider, 4> delayPanSliders;
    std::array<ParameterLabel, 4> delayPanLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> delayPanAttachments;
    
    std::array<juce::Slider, 4> delayFeedbackSliders;
    std::array<ParameterLabel, 4> delayFeedbackLabels;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 4> delayFeedbackAttachments;
    
    // XY Pad
    XYPad xyPad;
    juce::Label xyPadLabel;
    
    // Multi-delay meter
    MultiDelayMeter multiDelayMeter;
    juce::Label multiDelayMeterLabel;
    
    // X/Y Pad parameter assignments (support multiple parameters per axis)
    juce::StringArray xParameterIDs;
    juce::StringArray yParameterIDs;
    
    // Color coding for assignments
    const juce::Colour xAssignmentColor = juce::Colour(0, 150, 255);   // Blue
    const juce::Colour yAssignmentColor = juce::Colour(255, 220, 0);   // Yellow

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MultiDelayEditor)
};