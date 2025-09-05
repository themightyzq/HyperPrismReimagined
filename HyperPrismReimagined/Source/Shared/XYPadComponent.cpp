//==============================================================================
// HyperPrism Revived - Modern X/Y Pad Component Implementation
//==============================================================================

#include "XYPadComponent.h"

XYPadComponent::XYPadComponent()
{
    // Set up hidden sliders for parameter handling
    xSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    ySlider.setSliderStyle(juce::Slider::LinearVertical);
    xSlider.setRange(0.0, 1.0);
    ySlider.setRange(0.0, 1.0);
    xSlider.setValue(xValue, juce::dontSendNotification);
    ySlider.setValue(yValue, juce::dontSendNotification);
    
    // Add invisible sliders
    addChildComponent(xSlider);
    addChildComponent(ySlider);
    
    // Let parameter attachments handle slider changes
    // We'll sync the display values through parameter listener callbacks
    
    // Initialize colors
    backgroundColour = HyperPrismLookAndFeel::Colors::surfaceVariant;
    gridColour = HyperPrismLookAndFeel::Colors::outline;
    thumbColour = HyperPrismLookAndFeel::Colors::primary;
    thumbHoverColour = HyperPrismLookAndFeel::Colors::primary.brighter(0.3f);
    labelColour = HyperPrismLookAndFeel::Colors::onSurface;
    
    setSize(240, 240);
}

XYPadComponent::~XYPadComponent()
{
    // Remove parameter listeners
    if (valueTreeState) {
        valueTreeState->removeParameterListener(xParamID, this);
        valueTreeState->removeParameterListener(yParamID, this);
    }
    
    // Clear attachments before destroying sliders to prevent crash
    xAttachment.reset();
    yAttachment.reset();
}

void XYPadComponent::attachToParameters(juce::AudioProcessorValueTreeState& apvts, 
                                      const juce::String& xParamId, 
                                      const juce::String& yParamId)
{
    // Store references for parameter listening
    valueTreeState = &apvts;
    xParamID = xParamId;
    yParamID = yParamId;
    
    // Set up parameter attachments
    xAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, xParamId, xSlider);
    yAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, yParamId, ySlider);
    
    // Add parameter listeners
    apvts.addParameterListener(xParamId, this);
    apvts.addParameterListener(yParamId, this);
    
    // Initialize display values
    auto* xParam = apvts.getParameter(xParamId);
    auto* yParam = apvts.getParameter(yParamId);
    if (xParam && yParam) {
        xValue = xParam->getValue();
        yValue = yParam->getValue();
        repaint();
    }
}

void XYPadComponent::setXValue(float newValue, bool sendNotification)
{
    xValue = juce::jlimit(0.0f, 1.0f, newValue);
    
    // Convert normalized value to actual parameter range
    if (valueTreeState) {
        auto* xParam = valueTreeState->getParameter(xParamID);
        if (xParam) {
            float xActualValue = xParam->convertFrom0to1(xValue);
            updatingFromHost = true;
            xSlider.setValue(xActualValue, sendNotification ? juce::sendNotification : juce::dontSendNotification);
            updatingFromHost = false;
        }
    }
    
    repaint();
}

void XYPadComponent::setYValue(float newValue, bool sendNotification)
{
    yValue = juce::jlimit(0.0f, 1.0f, newValue);
    
    // Convert normalized value to actual parameter range
    if (valueTreeState) {
        auto* yParam = valueTreeState->getParameter(yParamID);
        if (yParam) {
            float yActualValue = yParam->convertFrom0to1(yValue);
            updatingFromHost = true;
            ySlider.setValue(yActualValue, sendNotification ? juce::sendNotification : juce::dontSendNotification);
            updatingFromHost = false;
        }
    }
    
    repaint();
}

void XYPadComponent::setXYValue(float newX, float newY, bool sendNotification)
{
    xValue = juce::jlimit(0.0f, 1.0f, newX);
    yValue = juce::jlimit(0.0f, 1.0f, newY);
    
    // Convert normalized values to actual parameter ranges for sliders
    if (valueTreeState && !updatingFromHost) {
        auto* xParam = valueTreeState->getParameter(xParamID);
        auto* yParam = valueTreeState->getParameter(yParamID);
        
        if (xParam && yParam) {
            float xActualValue = xParam->convertFrom0to1(xValue);
            float yActualValue = yParam->convertFrom0to1(yValue);
            
            updatingFromHost = true;
            xSlider.setValue(xActualValue, sendNotification ? juce::sendNotification : juce::dontSendNotification);
            ySlider.setValue(yActualValue, sendNotification ? juce::sendNotification : juce::dontSendNotification);
            updatingFromHost = false;
        }
    }
    
    repaint();
}

void XYPadComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    auto padBounds = getPadBounds().toFloat();
    
    // Draw background
    g.setColour(backgroundColour);
    g.fillRoundedRectangle(bounds, 8.0f);
    
    // Draw border
    g.setColour(gridColour);
    g.drawRoundedRectangle(bounds, 8.0f, 2.0f);
    
    // Draw pad area background
    g.setColour(HyperPrismLookAndFeel::Colors::surface);
    g.fillRoundedRectangle(padBounds, 4.0f);
    
    // Draw grid lines
    g.setColour(gridColour.withAlpha(0.3f));
    
    // Vertical grid lines
    for (int i = 1; i < 4; ++i)
    {
        float x = padBounds.getX() + (padBounds.getWidth() * i / 4.0f);
        g.drawLine(x, padBounds.getY(), x, padBounds.getBottom(), 1.0f);
    }
    
    // Horizontal grid lines
    for (int i = 1; i < 4; ++i)
    {
        float y = padBounds.getY() + (padBounds.getHeight() * i / 4.0f);
        g.drawLine(padBounds.getX(), y, padBounds.getRight(), y, 1.0f);
    }
    
    // Draw center crosshairs
    g.setColour(gridColour.withAlpha(0.5f));
    float centerX = padBounds.getCentreX();
    float centerY = padBounds.getCentreY();
    g.drawLine(centerX, padBounds.getY(), centerX, padBounds.getBottom(), 1.5f);
    g.drawLine(padBounds.getX(), centerY, padBounds.getRight(), centerY, 1.5f);
    
    // Draw value trails (subtle path showing recent movement)
    g.setColour(thumbColour.withAlpha(0.2f));
    auto thumbPos = getThumbPosition();
    
    // Draw crosshairs at thumb position
    g.setColour(thumbColour.withAlpha(0.4f));
    g.drawLine(thumbPos.x, padBounds.getY(), thumbPos.x, padBounds.getBottom(), 1.0f);
    g.drawLine(padBounds.getX(), thumbPos.y, padBounds.getRight(), thumbPos.y, 1.0f);
    
    // Draw thumb
    auto thumbColor = isDragging ? thumbHoverColour : thumbColour;
    g.setColour(thumbColor);
    g.fillEllipse(thumbPos.x - thumbRadius, thumbPos.y - thumbRadius, 
                  thumbRadius * 2, thumbRadius * 2);
    
    // Draw thumb border
    g.setColour(HyperPrismLookAndFeel::Colors::background);
    g.drawEllipse(thumbPos.x - thumbRadius, thumbPos.y - thumbRadius, 
                  thumbRadius * 2, thumbRadius * 2, 2.0f);
    
    // Draw labels
    g.setColour(labelColour);
    g.setFont(HyperPrismLookAndFeel::Colors::onSurface.toString().contains("f0f6fc") ? 
              juce::Font(juce::FontOptions(12.0f).withStyle("Bold")) : juce::Font(juce::FontOptions(12.0f)));
    
    // X label (bottom)
    auto xLabelBounds = juce::Rectangle<float>(bounds.getX(), padBounds.getBottom() + 8, 
                                              bounds.getWidth(), 20);
    g.drawText(xLabel, xLabelBounds, juce::Justification::centred);
    
    // Y label (left, rotated)
    g.saveState();
    g.addTransform(juce::AffineTransform::rotation(-juce::MathConstants<float>::halfPi, 
                                                  bounds.getX() + 15, bounds.getCentreY()));
    auto yLabelBounds = juce::Rectangle<float>(-40, -10, 80, 20);
    g.drawText(yLabel, yLabelBounds, juce::Justification::centred);
    g.restoreState();
    
    // Draw dynamic parameter value display with units
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.setColour(labelColour.withAlpha(0.8f));
    
    // Get actual parameter values with units if available
    juce::String xDisplayText, yDisplayText;
    
    if (valueTreeState) {
        auto* xParam = valueTreeState->getParameter(xParamID);
        auto* yParam = valueTreeState->getParameter(yParamID);
        
        if (xParam && yParam) {
            float xActualValue = xParam->convertFrom0to1(xValue);
            float yActualValue = yParam->convertFrom0to1(yValue);
            
            xDisplayText = juce::String(xActualValue, 2) + xUnit;
            yDisplayText = juce::String(yActualValue, 2) + yUnit;
        } else {
            xDisplayText = juce::String(xValue, 2);
            yDisplayText = juce::String(yValue, 2);
        }
    } else {
        xDisplayText = juce::String(xValue, 2);
        yDisplayText = juce::String(yValue, 2);
    }
    
    // X value (bottom right)
    g.drawText(xDisplayText, bounds.getRight() - 60, bounds.getBottom() - 25, 55, 15, 
               juce::Justification::centredRight);
    
    // Y value (top right)  
    g.drawText(yDisplayText, bounds.getRight() - 60, bounds.getY() + 5, 55, 15, 
               juce::Justification::centredRight);
}

void XYPadComponent::resized()
{
    // Position hidden sliders (they don't need to be visible)
    xSlider.setBounds(0, 0, 1, 1);
    ySlider.setBounds(0, 0, 1, 1);
}

void XYPadComponent::mouseDown(const juce::MouseEvent& event)
{
    if (getPadBounds().contains(event.getPosition()))
    {
        isDragging = true;
        updateFromMouse(event);
        repaint();
    }
}

void XYPadComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (isDragging)
    {
        updateFromMouse(event);
        repaint();
    }
}

void XYPadComponent::mouseUp(const juce::MouseEvent& event)
{
    isDragging = false;
    repaint();
}

void XYPadComponent::mouseDoubleClick(const juce::MouseEvent& event)
{
    if (getPadBounds().contains(event.getPosition()))
    {
        // Reset to center
        setXYValue(0.5f, 0.5f, true);
    }
}

void XYPadComponent::updateFromMouse(const juce::MouseEvent& event)
{
    auto padBounds = getPadBounds();
    auto pos = event.getPosition();
    
    // Constrain to pad bounds
    pos.x = juce::jlimit(padBounds.getX(), padBounds.getRight(), pos.x);
    pos.y = juce::jlimit(padBounds.getY(), padBounds.getBottom(), pos.y);
    
    // Convert to normalized values with more precision
    float newX = juce::jlimit(0.0f, 1.0f, (float)(pos.x - padBounds.getX()) / (float)padBounds.getWidth());
    float newY = juce::jlimit(0.0f, 1.0f, 1.0f - (float)(pos.y - padBounds.getY()) / (float)padBounds.getHeight()); // Invert Y: top = 1.0, bottom = 0.0
    
    // Update values directly without intermediate conversions to prevent glitching
    xValue = newX;
    yValue = newY;
    
    // Update sliders with proper bounds checking and avoid recursion
    if (valueTreeState && !updatingFromHost) {
        auto* xParam = valueTreeState->getParameter(xParamID);
        auto* yParam = valueTreeState->getParameter(yParamID);
        
        if (xParam && yParam) {
            updatingFromHost = true;
            xSlider.setValue(xParam->convertFrom0to1(xValue), juce::sendNotification);
            ySlider.setValue(yParam->convertFrom0to1(yValue), juce::sendNotification);
            updatingFromHost = false;
        }
    }
    
    repaint();
}

void XYPadComponent::updateParameterValues()
{
    if (onValueChanged)
        onValueChanged(xValue, yValue);
}

void XYPadComponent::parameterChanged(const juce::String& parameterID, float newValue)
{
    // Only update display values when parameters change from external sources
    // newValue is already normalized (0-1) from the parameter system
    if (!updatingFromHost) {
        if (parameterID == xParamID) {
            xValue = juce::jlimit(0.0f, 1.0f, newValue);
            repaint();
        } else if (parameterID == yParamID) {
            yValue = juce::jlimit(0.0f, 1.0f, newValue);
            repaint();
        }
    }
}

juce::Rectangle<int> XYPadComponent::getPadBounds() const
{
    auto bounds = getLocalBounds();
    int margin = 30;
    int size = juce::jmin(bounds.getWidth() - margin * 2, bounds.getHeight() - margin * 2);
    
    return juce::Rectangle<int>((bounds.getWidth() - size) / 2, 
                               (bounds.getHeight() - size) / 2 + 5, // Slight offset for labels
                               size, size);
}

juce::Point<float> XYPadComponent::getThumbPosition() const
{
    auto padBounds = getPadBounds().toFloat();
    
    float x = padBounds.getX() + (xValue * padBounds.getWidth());
    float y = padBounds.getY() + ((1.0f - yValue) * padBounds.getHeight()); // Invert Y for display
    
    return juce::Point<float>(x, y);
}