//==============================================================================
// HyperPrism Reimagined - Vocoder Editor
// Updated to match AutoPan template exactly
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <zqsfx_ui/zqsfx_ui.h>
#include "VocoderProcessor.h"
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
// Vocoder visualizer - shows band levels and carrier/modulator signals
//==============================================================================
class VocoderMeter : public juce::Component, private juce::Timer
{
public:
    explicit VocoderMeter(VocoderProcessor& processor);
    ~VocoderMeter() override;
    
    void paint(juce::Graphics& g) override;
    
private:
    void timerCallback() override;
    
    VocoderProcessor& processor;
    std::vector<float> smoothedBandLevels;
    float carrierLevel = 0.0f;
    float modulatorLevel = 0.0f;
    float outputLevel = 0.0f;
    int bandCount = 8;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocoderMeter)
};

//==============================================================================
// Main Editor
//==============================================================================
class VocoderEditor : public juce::AudioProcessorEditor
{
public:
    VocoderEditor(VocoderProcessor&);
    ~VocoderEditor() override;

    void paint(juce::Graphics&) override;

    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

private:
    void setupSlider(juce::Slider& slider, ParameterLabel& label, 
                    const juce::String& text);
    void updateXYPadFromParameters();
    void updateParametersFromXYPad(float x, float y);
    void showParameterMenu(juce::Component* target, const juce::String& parameterID);
    void updateParameterColors();
    void updateXYPadLabel();
    
    VocoderProcessor& audioProcessor;
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
    juce::Slider carrierFreqSlider;
    ParameterLabel carrierFreqLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> carrierFreqAttachment;
    
    juce::Slider modulatorGainSlider;
    ParameterLabel modulatorGainLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modulatorGainAttachment;
    
    juce::Slider bandCountSlider;
    ParameterLabel bandCountLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> bandCountAttachment;
    
    juce::Slider releaseTimeSlider;
    ParameterLabel releaseTimeLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseTimeAttachment;
    
    juce::Slider outputLevelSlider;
    ParameterLabel outputLevelLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputLevelAttachment;
    
    // XY Pad
    XYPad xyPad;
    juce::Label xyPadLabel;
    
    // Vocoder meter
    VocoderMeter vocoderMeter;
    
    // X/Y Pad parameter assignments (support multiple parameters per axis)
    juce::StringArray xParameterIDs;
    juce::StringArray yParameterIDs;
    
    // Color coding for assignments
    const juce::Colour xAssignmentColor = HyperPrismLookAndFeel::Colors::dynamics;   // was blue literal
    const juce::Colour yAssignmentColor = HyperPrismLookAndFeel::Colors::frequency;   // was yellow literal

    // Layout positions for paint()
    int outputSectionX = 0;
    int outputSectionY = 0;

    juce::TooltipWindow tooltipWindow { this, 500 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VocoderEditor)
};