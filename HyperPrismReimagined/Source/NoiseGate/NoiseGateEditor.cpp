//==============================================================================
// HyperPrism Reimagined - Noise Gate Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "NoiseGateEditor.h"

//==============================================================================
// XYPad Implementation (matching AutoPan style)
//==============================================================================
XYPad::XYPad()
{
    setRepaintsOnMouseActivity(true);
}

void XYPad::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(HyperPrismLookAndFeel::Colors::surfaceVariant);
    g.fillRoundedRectangle(bounds, 5.0f);
    
    // Grid lines
    g.setColour(HyperPrismLookAndFeel::Colors::outlineVariant.withAlpha(0.3f));
    for (int i = 1; i < 4; ++i)
    {
        float x = bounds.getWidth() * i / 4.0f;
        float y = bounds.getHeight() * i / 4.0f;
        g.drawLine(x, 0, x, bounds.getHeight(), 0.5f);
        g.drawLine(0, y, bounds.getWidth(), y, 0.5f);
    }
    
    // Border
    g.setColour(HyperPrismLookAndFeel::Colors::outline);
    g.drawRoundedRectangle(bounds, 5.0f, 2.0f);
    
    // Crosshair position
    float xPos = xValue * bounds.getWidth();
    float yPos = (1.0f - yValue) * bounds.getHeight();
    
    // Draw crosshair with colored lines
    g.setColour(xAxisColor.withAlpha(0.8f));
    g.drawLine(xPos, 0, xPos, bounds.getHeight(), 2.0f);
    
    g.setColour(yAxisColor.withAlpha(0.8f));
    g.drawLine(0, yPos, bounds.getWidth(), yPos, 2.0f);
    
    // Draw circle at intersection (blend of both colors)
    auto intersectionColor = xAxisColor.interpolatedWith(yAxisColor, 0.5f);
    g.setColour(intersectionColor);
    g.fillEllipse(xPos - 6, yPos - 6, 12, 12);
    g.setColour(HyperPrismLookAndFeel::Colors::onSurface);
    g.fillEllipse(xPos - 3, yPos - 3, 6, 6);
}

void XYPad::mouseDown(const juce::MouseEvent& event)
{
    updatePosition(event);
}

void XYPad::mouseDrag(const juce::MouseEvent& event)
{
    updatePosition(event);
}

void XYPad::updatePosition(const juce::MouseEvent& event)
{
    auto bounds = getLocalBounds().toFloat();
    xValue = juce::jlimit(0.0f, 1.0f, event.x / bounds.getWidth());
    yValue = juce::jlimit(0.0f, 1.0f, 1.0f - (event.y / bounds.getHeight()));
    
    if (onValueChange)
        onValueChange(xValue, yValue);
    
    repaint();
}

void XYPad::setValues(float x, float y)
{
    xValue = juce::jlimit(0.0f, 1.0f, x);
    yValue = juce::jlimit(0.0f, 1.0f, y);
    repaint();
}

void XYPad::setAxisColors(const juce::Colour& xColor, const juce::Colour& yColor)
{
    xAxisColor = xColor;
    yAxisColor = yColor;
    repaint();
}

//==============================================================================
// GateLED Implementation
//==============================================================================
GateLED::GateLED(NoiseGateProcessor& p) 
    : processor(p)
{
    startTimerHz(30); // 30 FPS update rate
}

GateLED::~GateLED()
{
    stopTimer();
}

void GateLED::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(HyperPrismLookAndFeel::Colors::background);
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Border
    g.setColour(HyperPrismLookAndFeel::Colors::outline);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    
    // LED
    auto ledBounds = bounds.reduced(15.0f);
    
    if (isGateOpen)
    {
        // Green glow when open
        g.setColour(HyperPrismLookAndFeel::Colors::success.withAlpha(0.3f));
        g.fillEllipse(ledBounds.expanded(5.0f));
        
        g.setColour(HyperPrismLookAndFeel::Colors::success);
        g.fillEllipse(ledBounds);
        
        g.setColour(HyperPrismLookAndFeel::Colors::success.brighter(0.5f));
        g.fillEllipse(ledBounds.reduced(ledBounds.getWidth() * 0.3f));
    }
    else
    {
        // Dark when closed
        g.setColour(HyperPrismLookAndFeel::Colors::surfaceVariant);
        g.fillEllipse(ledBounds);
        
        g.setColour(HyperPrismLookAndFeel::Colors::outline);
        g.drawEllipse(ledBounds, 1.0f);
    }
    
    // Label
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(12.0f);
    g.drawText(isGateOpen ? "OPEN" : "CLOSED", bounds, juce::Justification::centred);
}

void GateLED::timerCallback()
{
    bool newState = processor.isGateOpen();
    if (newState != isGateOpen)
    {
        isGateOpen = newState;
        repaint();
    }
}

//==============================================================================
// NoiseGateEditor Implementation
//==============================================================================
NoiseGateEditor::NoiseGateEditor(NoiseGateProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), gateLED(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterNames.add("threshold");
    yParameterNames.add("release");
    
    // Title (matching AutoPan style)
    titleLabel.setText("HyperPrism Reimagined Noise Gate", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions("Arial", "Bold", 24.0f)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Setup sliders with consistent style (6 parameters)
    setupSlider(thresholdSlider, thresholdLabel, "Threshold", " dB");
    setupSlider(attackSlider, attackLabel, "Attack", " ms");
    setupSlider(holdSlider, holdLabel, "Hold", " ms");
    setupSlider(releaseSlider, releaseLabel, "Release", " ms");
    setupSlider(rangeSlider, rangeLabel, "Range", " dB");
    setupSlider(lookaheadSlider, lookaheadLabel, "Lookahead", " ms");
    
    // Set parameter ranges (no AudioProcessorValueTreeState, so manual setup)
    thresholdSlider.setRange(-60.0, 0.0, 0.1);
    attackSlider.setRange(0.1, 100.0, 0.1);
    holdSlider.setRange(0.0, 1000.0, 1.0);
    releaseSlider.setRange(1.0, 5000.0, 1.0);
    rangeSlider.setRange(0.0, 60.0, 0.1);
    lookaheadSlider.setRange(0.0, 10.0, 0.1);
    
    // Connect sliders to parameters
    thresholdSlider.onValueChange = [this] { 
        audioProcessor.threshold->setValueNotifyingHost(
            audioProcessor.threshold->convertTo0to1(static_cast<float>(thresholdSlider.getValue())));
        updateXYPadFromParameters(); 
    };
    attackSlider.onValueChange = [this] { 
        audioProcessor.attack->setValueNotifyingHost(
            audioProcessor.attack->convertTo0to1(static_cast<float>(attackSlider.getValue())));
        updateXYPadFromParameters(); 
    };
    holdSlider.onValueChange = [this] { 
        audioProcessor.hold->setValueNotifyingHost(
            audioProcessor.hold->convertTo0to1(static_cast<float>(holdSlider.getValue())));
        updateXYPadFromParameters(); 
    };
    releaseSlider.onValueChange = [this] { 
        audioProcessor.release->setValueNotifyingHost(
            audioProcessor.release->convertTo0to1(static_cast<float>(releaseSlider.getValue())));
        updateXYPadFromParameters(); 
    };
    rangeSlider.onValueChange = [this] { 
        audioProcessor.range->setValueNotifyingHost(
            audioProcessor.range->convertTo0to1(static_cast<float>(rangeSlider.getValue())));
        updateXYPadFromParameters(); 
    };
    lookaheadSlider.onValueChange = [this] { 
        audioProcessor.lookahead->setValueNotifyingHost(
            audioProcessor.lookahead->convertTo0to1(static_cast<float>(lookaheadSlider.getValue())));
        updateXYPadFromParameters(); 
    };
    
    // Set up right-click handlers for parameter assignment
    thresholdLabel.onClick = [this]() { showParameterMenu(&thresholdLabel, "threshold"); };
    attackLabel.onClick = [this]() { showParameterMenu(&attackLabel, "attack"); };
    holdLabel.onClick = [this]() { showParameterMenu(&holdLabel, "hold"); };
    releaseLabel.onClick = [this]() { showParameterMenu(&releaseLabel, "release"); };
    rangeLabel.onClick = [this]() { showParameterMenu(&rangeLabel, "range"); };
    lookaheadLabel.onClick = [this]() { showParameterMenu(&lookaheadLabel, "lookahead"); };
    
    // Bypass button (top right like AutoPan)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::red);
    bypassButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
    addAndMakeVisible(bypassButton);
    
    // No bypass parameter in this processor, so we'll leave it unconnected
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);
    xyPadLabel.setText("Threshold / Release", juce::dontSendNotification);
    xyPadLabel.setJustificationType(juce::Justification::centred);
    xyPadLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    
    // Add gate LED
    addAndMakeVisible(gateLED);
    gateLEDLabel.setText("Gate Status", juce::dontSendNotification);
    gateLEDLabel.setJustificationType(juce::Justification::centred);
    gateLEDLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(gateLEDLabel);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Initialize slider values from parameters
    thresholdSlider.setValue(audioProcessor.threshold->get(), juce::dontSendNotification);
    attackSlider.setValue(audioProcessor.attack->get(), juce::dontSendNotification);
    holdSlider.setValue(audioProcessor.hold->get(), juce::dontSendNotification);
    releaseSlider.setValue(audioProcessor.release->get(), juce::dontSendNotification);
    rangeSlider.setValue(audioProcessor.range->get(), juce::dontSendNotification);
    lookaheadSlider.setValue(audioProcessor.lookahead->get(), juce::dontSendNotification);
    
    setSize(650, 600);
}

NoiseGateEditor::~NoiseGateEditor()
{
    setLookAndFeel(nullptr);
}

void NoiseGateEditor::paint(juce::Graphics& g)
{
    // Dark background matching AutoPan
    g.fillAll(HyperPrismLookAndFeel::Colors::background);
}

void NoiseGateEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(40));
    
    // Bypass button (top right)
    bypassButton.setBounds(bounds.getWidth() - 100, 10, 80, 30);
    
    bounds.reduce(20, 10);
    
    // 6 parameter layout: 3+3 sliders in two rows (similar to MoreStereo/MSMatrix)
    // Available height after title and margins: ~500px
    
    auto topRow = bounds.removeFromTop(140);
    auto sliderWidth = 80;
    auto spacing = 20;
    
    // Calculate total width needed for 3 sliders
    auto totalSliderWidth = sliderWidth * 3 + spacing * 2;
    auto startX = (bounds.getWidth() - totalSliderWidth) / 2;
    topRow.removeFromLeft(startX);
    
    // Top row: Threshold, Attack, Hold
    thresholdSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    thresholdLabel.setBounds(thresholdSlider.getX(), thresholdSlider.getBottom(), sliderWidth, 20);
    topRow.removeFromLeft(spacing);
    
    attackSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    attackLabel.setBounds(attackSlider.getX(), attackSlider.getBottom(), sliderWidth, 20);
    topRow.removeFromLeft(spacing);
    
    holdSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    holdLabel.setBounds(holdSlider.getX(), holdSlider.getBottom(), sliderWidth, 20);
    
    bounds.removeFromTop(10);
    
    // Second row: Release, Range, Lookahead
    auto secondRow = bounds.removeFromTop(140);
    secondRow.removeFromLeft(startX);
    
    releaseSlider.setBounds(secondRow.removeFromLeft(sliderWidth).reduced(0, 20));
    releaseLabel.setBounds(releaseSlider.getX(), releaseSlider.getBottom(), sliderWidth, 20);
    secondRow.removeFromLeft(spacing);
    
    rangeSlider.setBounds(secondRow.removeFromLeft(sliderWidth).reduced(0, 20));
    rangeLabel.setBounds(rangeSlider.getX(), rangeSlider.getBottom(), sliderWidth, 20);
    secondRow.removeFromLeft(spacing);
    
    lookaheadSlider.setBounds(secondRow.removeFromLeft(sliderWidth).reduced(0, 20));
    lookaheadLabel.setBounds(lookaheadSlider.getX(), lookaheadSlider.getBottom(), sliderWidth, 20);
    
    // Bottom section - XY Pad and LED side by side
    bounds.removeFromTop(10);
    
    // Split remaining space horizontally for XY pad and LED
    auto bottomArea = bounds;
    auto componentHeight = 180;
    
    // XY Pad on left (200x180 standard)
    auto xyPadWidth = 200;
    auto xyPadHeight = 180;
    auto xyPadBounds = bottomArea.withWidth(xyPadWidth).withHeight(xyPadHeight);
    auto ledSize = 100;
    auto totalBottomWidth = xyPadWidth + 20 + ledSize; // XY pad + spacing + LED
    auto bottomStartX = (bottomArea.getWidth() - totalBottomWidth) / 2;
    xyPadBounds.translate(bottomStartX, 0);
    
    xyPad.setBounds(xyPadBounds);
    xyPadLabel.setBounds(xyPadBounds.getX(), xyPadBounds.getBottom() + 5, xyPadWidth, 20);
    
    // Gate LED on right
    auto ledBounds = bottomArea.withX(xyPadBounds.getRight() + 20).withWidth(ledSize).withHeight(ledSize);
    ledBounds.translate(0, (componentHeight - ledSize) / 2);
    
    gateLED.setBounds(ledBounds);
    gateLEDLabel.setBounds(ledBounds.getX(), xyPadBounds.getBottom() + 5, ledSize, 20);
}

void NoiseGateEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
                               const juce::String& text, const juce::String& suffix)
{
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::darkgrey);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::grey);
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::cyan);
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::lightgrey);
    slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    
    if (!suffix.isEmpty())
        slider.setTextValueSuffix(suffix);
    
    addAndMakeVisible(slider);
    
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(label);
}

void NoiseGateEditor::updateParameterColors()
{
    // Update label colors based on X/Y assignments
    auto updateLabelColor = [this](ParameterLabel& label, const juce::String& paramName) {
        bool isAssignedToX = xParameterNames.contains(paramName);
        bool isAssignedToY = yParameterNames.contains(paramName);
        
        if (isAssignedToX && isAssignedToY)
            label.setColour(juce::Label::textColourId, xAssignmentColor.interpolatedWith(yAssignmentColor, 0.5f));
        else if (isAssignedToX)
            label.setColour(juce::Label::textColourId, xAssignmentColor);
        else if (isAssignedToY)
            label.setColour(juce::Label::textColourId, yAssignmentColor);
        else
            label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    };
    
    updateLabelColor(thresholdLabel, "threshold");
    updateLabelColor(attackLabel, "attack");
    updateLabelColor(holdLabel, "hold");
    updateLabelColor(releaseLabel, "release");
    updateLabelColor(rangeLabel, "range");
    updateLabelColor(lookaheadLabel, "lookahead");
}

void NoiseGateEditor::updateXYPadFromParameters()
{
    // For multiple parameters, use the average of their normalized values
    float xValue = 0.0f;
    float yValue = 0.0f;
    
    // Helper to get normalized value for a parameter
    auto getNormalizedValue = [this](const juce::String& paramName) -> float {
        if (paramName == "threshold")
            return audioProcessor.threshold->convertTo0to1(audioProcessor.threshold->get());
        else if (paramName == "attack")
            return audioProcessor.attack->convertTo0to1(audioProcessor.attack->get());
        else if (paramName == "hold")
            return audioProcessor.hold->convertTo0to1(audioProcessor.hold->get());
        else if (paramName == "release")
            return audioProcessor.release->convertTo0to1(audioProcessor.release->get());
        else if (paramName == "range")
            return audioProcessor.range->convertTo0to1(audioProcessor.range->get());
        else if (paramName == "lookahead")
            return audioProcessor.lookahead->convertTo0to1(audioProcessor.lookahead->get());
        return 0.5f;
    };
    
    // Calculate average X value
    if (!xParameterNames.isEmpty())
    {
        for (const auto& paramName : xParameterNames)
        {
            xValue += getNormalizedValue(paramName);
        }
        xValue /= xParameterNames.size();
    }
    
    // Calculate average Y value
    if (!yParameterNames.isEmpty())
    {
        for (const auto& paramName : yParameterNames)
        {
            yValue += getNormalizedValue(paramName);
        }
        yValue /= yParameterNames.size();
    }
    
    xyPad.setValues(xValue, yValue);
}

void NoiseGateEditor::updateParametersFromXYPad(float x, float y)
{
    // Helper to set parameter from normalized value
    auto setParameterValue = [this](const juce::String& paramName, float normalizedValue) {
        if (paramName == "threshold") {
            audioProcessor.threshold->setValueNotifyingHost(normalizedValue);
            thresholdSlider.setValue(audioProcessor.threshold->get(), juce::dontSendNotification);
        }
        else if (paramName == "attack") {
            audioProcessor.attack->setValueNotifyingHost(normalizedValue);
            attackSlider.setValue(audioProcessor.attack->get(), juce::dontSendNotification);
        }
        else if (paramName == "hold") {
            audioProcessor.hold->setValueNotifyingHost(normalizedValue);
            holdSlider.setValue(audioProcessor.hold->get(), juce::dontSendNotification);
        }
        else if (paramName == "release") {
            audioProcessor.release->setValueNotifyingHost(normalizedValue);
            releaseSlider.setValue(audioProcessor.release->get(), juce::dontSendNotification);
        }
        else if (paramName == "range") {
            audioProcessor.range->setValueNotifyingHost(normalizedValue);
            rangeSlider.setValue(audioProcessor.range->get(), juce::dontSendNotification);
        }
        else if (paramName == "lookahead") {
            audioProcessor.lookahead->setValueNotifyingHost(normalizedValue);
            lookaheadSlider.setValue(audioProcessor.lookahead->get(), juce::dontSendNotification);
        }
    };
    
    // Update all assigned X parameters
    for (const auto& paramName : xParameterNames)
    {
        setParameterValue(paramName, x);
    }
    
    // Update all assigned Y parameters
    for (const auto& paramName : yParameterNames)
    {
        setParameterValue(paramName, y);
    }
}

void NoiseGateEditor::showParameterMenu(juce::Label* label, const juce::String& parameterName)
{
    juce::PopupMenu menu;
    
    // Add header
    menu.addSectionHeader("Assign to X/Y Pad");
    menu.addSeparator();
    
    // Check if this parameter is currently assigned
    bool isAssignedToX = xParameterNames.contains(parameterName);
    bool isAssignedToY = yParameterNames.contains(parameterName);
    
    menu.addItem(1, "Toggle X-axis", true, isAssignedToX);
    menu.addItem(2, "Toggle Y-axis", true, isAssignedToY);
    
    menu.addSeparator();
    menu.addItem(3, "Clear all assignments");
    
    // Show the menu
    menu.showMenuAsync(juce::PopupMenu::Options()
        .withTargetComponent(label)
        .withMinimumWidth(150),
        [this, parameterName](int result)
        {
            if (result == 1)
            {
                // Toggle X assignment
                if (xParameterNames.contains(parameterName))
                    xParameterNames.removeString(parameterName);
                else
                    xParameterNames.add(parameterName);
                    
                // Ensure at least one parameter is assigned
                if (xParameterNames.isEmpty())
                    xParameterNames.add("threshold");
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 2)
            {
                // Toggle Y assignment
                if (yParameterNames.contains(parameterName))
                    yParameterNames.removeString(parameterName);
                else
                    yParameterNames.add(parameterName);
                    
                // Ensure at least one parameter is assigned
                if (yParameterNames.isEmpty())
                    yParameterNames.add("release");
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterNames.clear();
                yParameterNames.clear();
                xParameterNames.add("threshold");
                yParameterNames.add("release");
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void NoiseGateEditor::updateXYPadLabel()
{
    auto getParameterDisplayName = [](const juce::String& paramName) -> juce::String {
        if (paramName == "threshold") return "Threshold";
        if (paramName == "attack") return "Attack";
        if (paramName == "hold") return "Hold";
        if (paramName == "release") return "Release";
        if (paramName == "range") return "Range";
        if (paramName == "lookahead") return "Lookahead";
        return paramName;
    };
    
    juce::String xLabel;
    juce::String yLabel;
    
    // Build X label
    if (xParameterNames.size() == 0)
        xLabel = "None";
    else if (xParameterNames.size() == 1)
        xLabel = getParameterDisplayName(xParameterNames[0]);
    else
        xLabel = "Multiple";
        
    // Build Y label
    if (yParameterNames.size() == 0)
        yLabel = "None";
    else if (yParameterNames.size() == 1)
        yLabel = getParameterDisplayName(yParameterNames[0]);
    else
        yLabel = "Multiple";
        
    xyPadLabel.setText(xLabel + " / " + yLabel, juce::dontSendNotification);
}