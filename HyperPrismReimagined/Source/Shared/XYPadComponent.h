//==============================================================================
// HyperPrism Revived - Modern X/Y Pad Component
// Replaces the original Blue Window X/Y control
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "HyperPrismLookAndFeel.h"

class XYPadComponent : public juce::Component, 
                       public juce::AudioProcessorValueTreeState::Listener
{
public:
    XYPadComponent();
    ~XYPadComponent() override;

    // Parameter attachment
    void attachToParameters(juce::AudioProcessorValueTreeState& apvts, 
                           const juce::String& xParamId, 
                           const juce::String& yParamId);

    // Visual customization
    void setXLabel(const juce::String& label) { xLabel = label; repaint(); }
    void setYLabel(const juce::String& label) { yLabel = label; repaint(); }
    void setXUnit(const juce::String& unit) { xUnit = unit; repaint(); }
    void setYUnit(const juce::String& unit) { yUnit = unit; repaint(); }
    void setPadSize(int size) { padSize = size; resized(); }
    
    // Get current normalized values
    float getXValue() const { return xValue; }
    float getYValue() const { return yValue; }
    
    // Set values programmatically
    void setXValue(float newValue, bool sendNotification = true);
    void setYValue(float newValue, bool sendNotification = true);
    void setXYValue(float newX, float newY, bool sendNotification = true);

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;

    // Callback for value changes
    std::function<void(float x, float y)> onValueChanged;
    
    // Prevent recursion during parameter updates (public for callback access)
    bool updatingFromHost = false;
    
    // AudioProcessorValueTreeState::Listener override
    void parameterChanged(const juce::String& parameterID, float newValue) override;

private:
    void updateFromMouse(const juce::MouseEvent& event);
    void updateParameterValues();
    
    juce::Rectangle<int> getPadBounds() const;
    juce::Point<float> getThumbPosition() const;
    
    // Parameter attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> xAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> yAttachment;
    juce::Slider xSlider, ySlider; // Hidden sliders for parameter handling
    
    // Parameter IDs for listener
    juce::String xParamID, yParamID;
    juce::AudioProcessorValueTreeState* valueTreeState = nullptr;
    
    // Values (0.0 to 1.0)
    float xValue = 0.5f;
    float yValue = 0.5f;
    
    // Visual properties
    juce::String xLabel = "X";
    juce::String yLabel = "Y";
    juce::String xUnit = "";
    juce::String yUnit = "";
    int padSize = 200;
    int thumbRadius = 8;
    
    // Interaction state
    bool isDragging = false;
    juce::Point<int> lastMousePos;
    
    // Colors from look and feel
    juce::Colour backgroundColour;
    juce::Colour gridColour;
    juce::Colour thumbColour;
    juce::Colour thumbHoverColour;
    juce::Colour labelColour;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XYPadComponent)
};