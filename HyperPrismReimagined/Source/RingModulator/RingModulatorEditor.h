//==============================================================================
// HyperPrism Revived - Ring Modulator Editor
// Updated to match AutoPan template exactly
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "RingModulatorProcessor.h"
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
// Ring modulation visualizer - shows carrier/modulator waveforms
//==============================================================================
class RingModulatorMeter : public juce::Component, private juce::Timer
{
public:
    explicit RingModulatorMeter(RingModulatorProcessor& processor);
    ~RingModulatorMeter() override;
    
    void paint(juce::Graphics& g) override;
    
private:
    void timerCallback() override;
    
    RingModulatorProcessor& processor;
    std::vector<float> carrierWaveform;
    std::vector<float> modulatorWaveform;
    std::vector<float> outputWaveform;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RingModulatorMeter)
};

//==============================================================================
// Main Editor
//==============================================================================
class RingModulatorEditor : public juce::AudioProcessorEditor
{
public:
    RingModulatorEditor(RingModulatorProcessor&);
    ~RingModulatorEditor() override;

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
    
    RingModulatorProcessor& audioProcessor;
    HyperPrismLookAndFeel customLookAndFeel;
    
    // Title
    juce::Label titleLabel;
    
    // Bypass
    juce::ToggleButton bypassButton;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    
    // Parameter controls with ParameterLabel for right-click assignment
    juce::Slider carrierFreqSlider;
    ParameterLabel carrierFreqLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> carrierFreqAttachment;
    
    juce::Slider modulatorFreqSlider;
    ParameterLabel modulatorFreqLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> modulatorFreqAttachment;
    
    juce::ComboBox carrierWaveformBox;
    ParameterLabel carrierWaveformLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> carrierWaveformAttachment;
    
    juce::ComboBox modulatorWaveformBox;
    ParameterLabel modulatorWaveformLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modulatorWaveformAttachment;
    
    juce::Slider mixSlider;
    ParameterLabel mixLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment;
    
    // XY Pad
    XYPad xyPad;
    juce::Label xyPadLabel;
    
    // Ring modulator meter
    RingModulatorMeter ringModulatorMeter;
    juce::Label ringModulatorMeterLabel;
    
    // X/Y Pad parameter assignments (support multiple parameters per axis)
    juce::StringArray xParameterIDs;
    juce::StringArray yParameterIDs;
    
    // Color coding for assignments
    const juce::Colour xAssignmentColor = juce::Colour(0, 150, 255);   // Blue
    const juce::Colour yAssignmentColor = juce::Colour(255, 220, 0);   // Yellow

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RingModulatorEditor)
};