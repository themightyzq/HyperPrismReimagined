//==============================================================================
// HyperPrism Reimagined - Low-Pass Filter Editor
// Updated to match AutoPan template exactly
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <zqsfx_ui/zqsfx_ui.h>
#include "LowPassProcessor.h"
#include "../Shared/HyperPrismLookAndFeel.h"
#include "../Shared/HyperPrismAbout.h"

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
class XYPad : public juce::Component, public juce::SettableTooltipClient
{
public:
    XYPad();
    
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key) override;
    
    void setValues(float x, float y);
    void setAxisColors(const juce::Colour& xColor, const juce::Colour& yColor);
    
    std::function<void(float, float)> onValueChange;
    
private:
    void updatePosition(const juce::MouseEvent& event);
    
    float xValue = 0.5f;
    float yValue = 0.5f;
    juce::Colour xAxisColor = HyperPrismLookAndFeel::Colors::dynamics;   // was blue literal
    juce::Colour yAxisColor = HyperPrismLookAndFeel::Colors::frequency;    // was yellow literal
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XYPad)
};

//==============================================================================
// Main Editor
//==============================================================================
class LowPassEditor : public juce::AudioProcessorEditor
{
public:
    LowPassEditor(LowPassProcessor&);
    ~LowPassEditor() override;

    void paint(juce::Graphics&) override;

    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

private:
    void setupControls();
    void setupXYPad();
    void setupSlider(juce::Slider& slider, ParameterLabel& label, 
                    const juce::String& text);
    void updateXYPadFromParameters();
    void updateParametersFromXYPad(float x, float y);
    void showParameterMenu(juce::Component* target, const juce::String& parameterID);
    void updateParameterColors();
    void updateXYPadLabel();
    
    LowPassProcessor& audioProcessor;
    HyperPrismLookAndFeel customLookAndFeel;

    // ZQ SFX company mark + About box trigger (style guide section 5)
    zqsfx::ui::LogoMark logo { JucePlugin_Name };
    
    // Title
    juce::Label titleLabel;
    juce::Label brandLabel;

    // Bypass
    juce::TextButton bypassButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    
    // Parameter controls with ParameterLabel for right-click assignment
    juce::Slider frequencySlider;
    ParameterLabel frequencyLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> frequencyAttachment;
    
    juce::Slider resonanceSlider;
    ParameterLabel resonanceLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> resonanceAttachment;
    
    juce::Slider gainSlider;
    ParameterLabel gainLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gainAttachment;
    
    juce::Slider mixSlider;
    ParameterLabel mixLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    
    // XY Pad
    XYPad xyPad;
    juce::Label xyPadLabel;
    
    // X/Y Pad parameter assignments (support multiple parameters per axis)
    juce::StringArray xParameterIDs;
    juce::StringArray yParameterIDs;
    
    // Color coding for assignments
    const juce::Colour xAssignmentColor = HyperPrismLookAndFeel::Colors::dynamics;   // was blue literal
    const juce::Colour yAssignmentColor = HyperPrismLookAndFeel::Colors::frequency;   // was yellow literal

    int outputSectionX = 0;
    int outputSectionY = 0;

    juce::TooltipWindow tooltipWindow { this, 500 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LowPassEditor)
};