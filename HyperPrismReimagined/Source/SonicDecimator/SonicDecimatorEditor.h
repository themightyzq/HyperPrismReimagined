//==============================================================================
// HyperPrism Revived - Sonic Decimator Editor
// Updated to match AutoPan template exactly
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "SonicDecimatorProcessor.h"
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
// Decimation visualizer - shows bit reduction and sample rate reduction
//==============================================================================
class DecimationMeter : public juce::Component, private juce::Timer
{
public:
    explicit DecimationMeter(SonicDecimatorProcessor& processor);
    ~DecimationMeter() override;
    
    void paint(juce::Graphics& g) override;
    
private:
    void timerCallback() override;
    
    SonicDecimatorProcessor& processor;
    float inputLevel = 0.0f;
    float outputLevel = 0.0f;
    float bitReduction = 0.0f;
    float sampleReduction = 0.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DecimationMeter)
};

//==============================================================================
// Main Editor
//==============================================================================
class SonicDecimatorEditor : public juce::AudioProcessorEditor
{
public:
    SonicDecimatorEditor(SonicDecimatorProcessor&);
    ~SonicDecimatorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void setupSlider(juce::Slider& slider, ParameterLabel& label, 
                    const juce::String& text, const juce::String& suffix);
    void setupToggleButton(juce::ToggleButton& button, ParameterLabel& label, 
                          const juce::String& text);
    void updateXYPadFromParameters();
    void updateParametersFromXYPad(float x, float y);
    void showParameterMenu(juce::Label* label, const juce::String& parameterID);
    void updateParameterColors();
    void updateXYPadLabel();
    
    SonicDecimatorProcessor& audioProcessor;
    HyperPrismLookAndFeel customLookAndFeel;
    
    // Title
    juce::Label titleLabel;
    
    // Bypass
    juce::ToggleButton bypassButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    
    // Parameter controls with ParameterLabel for right-click assignment
    juce::Slider bitDepthSlider;
    ParameterLabel bitDepthLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bitDepthAttachment;
    
    juce::Slider sampleRateSlider;
    ParameterLabel sampleRateLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sampleRateAttachment;
    
    juce::Slider mixSlider;
    ParameterLabel mixLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    
    juce::Slider outputLevelSlider;
    ParameterLabel outputLevelLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputLevelAttachment;
    
    juce::ToggleButton antiAliasButton;
    ParameterLabel antiAliasLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> antiAliasAttachment;
    
    juce::ToggleButton ditherButton;
    ParameterLabel ditherLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> ditherAttachment;
    
    // XY Pad
    XYPad xyPad;
    juce::Label xyPadLabel;
    
    // Decimation meter
    DecimationMeter decimationMeter;
    juce::Label decimationMeterLabel;
    
    // X/Y Pad parameter assignments (support multiple parameters per axis)
    juce::StringArray xParameterIDs;
    juce::StringArray yParameterIDs;
    
    // Color coding for assignments
    const juce::Colour xAssignmentColor = juce::Colour(0, 150, 255);   // Blue
    const juce::Colour yAssignmentColor = juce::Colour(255, 220, 0);   // Yellow

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SonicDecimatorEditor)
};