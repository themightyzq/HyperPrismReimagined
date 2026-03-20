//==============================================================================
// HyperPrism Reimagined - Compressor Editor Header
// Updated to match modern template
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "CompressorProcessor.h"
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

class XYPad : public juce::Component, public juce::SettableTooltipClient
{
public:
    XYPad();
    
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    
    std::function<void(float, float)> onValueChange;
    
    void setValues(float x, float y);
    void setAxisColors(const juce::Colour& xColor, const juce::Colour& yColor);
    
private:
    float xValue = 0.5f;
    float yValue = 0.5f;
    juce::Colour xAxisColor = juce::Colours::cyan;
    juce::Colour yAxisColor = juce::Colours::yellow;
    
    void updatePosition(const juce::MouseEvent& event);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XYPad)
};

class GainReductionMeter : public juce::Component, private juce::Timer
{
public:
    GainReductionMeter(CompressorProcessor& p);
    ~GainReductionMeter() override;
    
    void paint(juce::Graphics& g) override;
    void timerCallback() override;
    
private:
    CompressorProcessor& processor;
    float currentReduction = 0.0f;
    float targetReduction = 0.0f;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GainReductionMeter)
};

class CompressorEditor : public juce::AudioProcessorEditor
{
public:
    CompressorEditor(CompressorProcessor&);
    ~CompressorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

private:
    CompressorProcessor& audioProcessor;
    
    // Look and Feel
    HyperPrismLookAndFeel customLookAndFeel;
    
    // Sliders
    juce::Slider thresholdSlider;
    juce::Slider ratioSlider;
    juce::Slider attackSlider;
    juce::Slider releaseSlider;
    juce::Slider kneeSlider;
    juce::Slider makeupGainSlider;
    juce::Slider mixSlider;
    
    // Labels (using ParameterLabel for right-click functionality)
    ParameterLabel thresholdLabel;
    ParameterLabel ratioLabel;
    ParameterLabel attackLabel;
    ParameterLabel releaseLabel;
    ParameterLabel kneeLabel;
    ParameterLabel makeupGainLabel;
    ParameterLabel mixLabel;
    juce::Label titleLabel;
    juce::Label brandLabel;

    // Bypass
    juce::TextButton bypassButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;

    // XY Pad
    XYPad xyPad;
    juce::Label xyPadLabel;
    
    // Gain Reduction Meter
    GainReductionMeter gainReductionMeter;
    
    // Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> thresholdAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> ratioAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> releaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> kneeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> makeupGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    
    // XY Pad parameter assignment
    juce::StringArray xParameterIDs;
    juce::StringArray yParameterIDs;
    
    // Assignment colors
    const juce::Colour xAssignmentColor = juce::Colour(0, 150, 255);
    const juce::Colour yAssignmentColor = juce::Colour(255, 220, 0);
    
    void setupSlider(juce::Slider& slider, juce::Label& label, const juce::String& text);
    void updateParameterColors();
    void updateXYPadFromParameters();
    void updateParametersFromXYPad(float x, float y);
    void showParameterMenu(juce::Component* target, const juce::String& parameterID);
    void updateXYPadLabel();
    
    int outputSectionX = 0;
    int outputSectionY = 0;

    juce::TooltipWindow tooltipWindow { this, 500 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CompressorEditor)
};

//==============================================================================