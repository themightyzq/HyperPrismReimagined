//==============================================================================
// HyperPrism Revived - Bass Maximiser Editor
// Updated to match AutoPan template exactly
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "BassMaximiserProcessor.h"
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
class BassMaximiserEditor : public juce::AudioProcessorEditor
{
public:
    BassMaximiserEditor(BassMaximiserProcessor&);
    ~BassMaximiserEditor() override;

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
    
    BassMaximiserProcessor& audioProcessor;
    HyperPrismLookAndFeel customLookAndFeel;
    
    // Title
    juce::Label titleLabel;
    
    // Bypass
    juce::ToggleButton bypassButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    
    // Parameter controls with ParameterLabel for right-click assignment
    juce::Slider frequencySlider;
    ParameterLabel frequencyLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> frequencyAttachment;
    
    juce::Slider boostSlider;
    ParameterLabel boostLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> boostAttachment;
    
    juce::Slider harmonicsSlider;
    ParameterLabel harmonicsLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> harmonicsAttachment;
    
    juce::Slider tightnessSlider;
    ParameterLabel tightnessLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> tightnessAttachment;
    
    juce::Slider outputGainSlider;
    ParameterLabel outputGainLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;
    
    // Phase Invert toggle
    juce::ToggleButton phaseInvertButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> phaseInvertAttachment;
    
    // XY Pad
    XYPad xyPad;
    juce::Label xyPadLabel;
    
    // X/Y Pad parameter assignments (support multiple parameters per axis)
    juce::StringArray xParameterIDs;
    juce::StringArray yParameterIDs;
    
    // Color coding for assignments
    const juce::Colour xAssignmentColor = juce::Colour(0, 150, 255);   // Blue
    const juce::Colour yAssignmentColor = juce::Colour(255, 220, 0);   // Yellow
    
    // Parameter ID constants
    static constexpr const char* FREQUENCY_ID = "frequency";
    static constexpr const char* BOOST_ID = "boost";
    static constexpr const char* HARMONICS_ID = "harmonics";
    static constexpr const char* TIGHTNESS_ID = "tightness";
    static constexpr const char* OUTPUT_GAIN_ID = "outputGain";
    static constexpr const char* PHASE_INVERT_ID = "phaseInvert";
    static constexpr const char* BYPASS_ID = "bypass";

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BassMaximiserEditor)
};