//==============================================================================
// HyperPrism Revived - Auto Pan Editor
// Updated to match Compressor UI template
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "AutoPanProcessor.h"
#include "../Shared/HyperPrismLookAndFeel.h"
#include "../Shared/StandardLayout.h"

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
// XY Pad component (matching Compressor style)
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
// Pan Position Meter - horizontal meter showing current pan position
//==============================================================================
class PanPositionMeter : public juce::Component, private juce::Timer
{
public:
    explicit PanPositionMeter(AutoPanProcessor& processor);
    ~PanPositionMeter() override;
    
    void paint(juce::Graphics& g) override;
    
private:
    void timerCallback() override;
    float getWaveformValue(float phase, int waveformType);
    
    AutoPanProcessor& processor;
    float currentPanPosition = 0.0f;
    float lfoPhase = 0.0f;
    int currentWaveform = 0;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PanPositionMeter)
};

//==============================================================================
// Main Editor
//==============================================================================
class AutoPanEditor : public juce::AudioProcessorEditor
{
public:
    AutoPanEditor(AutoPanProcessor&);
    ~AutoPanEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void setupSlider(juce::Slider& slider, juce::Label& label, 
                    const juce::String& text, const juce::String& suffix);
    void updateXYPadFromParameters();
    void updateParametersFromXYPad(float x, float y);
    void showParameterMenu(juce::Label* label, const juce::String& parameterID);
    void assignParameterToXYPad(const juce::String& parameterID, bool assignToX);
    void updateParameterColors();
    void updateXYPadLabel();
    
    AutoPanProcessor& audioProcessor;
    HyperPrismLookAndFeel customLookAndFeel;
    
    // Title
    juce::Label titleLabel;
    
    // Bypass
    juce::ToggleButton bypassButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    
    // Main controls
    juce::Slider rateSlider;
    ParameterLabel rateLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> rateAttachment;
    
    juce::Slider depthSlider;
    ParameterLabel depthLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> depthAttachment;
    
    juce::Slider phaseSlider;
    ParameterLabel phaseLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> phaseAttachment;
    
    juce::Slider outputLevelSlider;
    ParameterLabel outputLevelLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputLevelAttachment;
    
    // Waveform selector
    juce::ComboBox waveformComboBox;
    juce::Label waveformLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> waveformAttachment;
    
    // Sync button
    juce::ToggleButton syncButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> syncAttachment;
    
    // XY Pad
    XYPad xyPad;
    juce::Label xyPadLabel;
    
    // Pan position meter
    PanPositionMeter panPositionMeter;
    juce::Label meterLabel;
    
    // X/Y Pad parameter assignments (support multiple parameters per axis)
    juce::StringArray xParameterIDs;
    juce::StringArray yParameterIDs;
    
    // Color coding for assignments
    const juce::Colour xAssignmentColor = juce::Colour(0, 150, 255);   // Blue
    const juce::Colour yAssignmentColor = juce::Colour(255, 220, 0);   // Yellow

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutoPanEditor)
};