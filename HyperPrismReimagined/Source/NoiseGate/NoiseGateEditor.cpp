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
    
    // Title
    titleLabel.setText("NOISE GATE", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(16.0f).withStyle("Bold")));
    titleLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurface);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    // Brand label
    brandLabel.setText("HyperPrism Reimagined", juce::dontSendNotification);
    brandLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    brandLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    brandLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(brandLabel);
    
    // Setup sliders with consistent style (6 parameters)
    setupSlider(thresholdSlider, thresholdLabel, "Threshold");
    thresholdSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::dynamics);
    setupSlider(attackSlider, attackLabel, "Attack");
    attackSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::timing);
    setupSlider(holdSlider, holdLabel, "Hold");
    holdSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::timing);
    setupSlider(releaseSlider, releaseLabel, "Release");
    releaseSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::timing);
    setupSlider(rangeSlider, rangeLabel, "Range");
    rangeSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::dynamics);
    setupSlider(lookaheadSlider, lookaheadLabel, "Lookahead");
    lookaheadSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::dynamics);
    
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

    // Register right-click on sliders for XY pad assignment
    thresholdSlider.addMouseListener(this, true);
    thresholdSlider.getProperties().set("xyParamID", "threshold");
    attackSlider.addMouseListener(this, true);
    attackSlider.getProperties().set("xyParamID", "attack");
    holdSlider.addMouseListener(this, true);
    holdSlider.getProperties().set("xyParamID", "hold");
    releaseSlider.addMouseListener(this, true);
    releaseSlider.getProperties().set("xyParamID", "release");
    rangeSlider.addMouseListener(this, true);
    rangeSlider.getProperties().set("xyParamID", "range");
    lookaheadSlider.addMouseListener(this, true);
    lookaheadSlider.getProperties().set("xyParamID", "lookahead");

    
    // Bypass button (top right)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setClickingTogglesState(true);
    addAndMakeVisible(bypassButton);
    
    // No bypass parameter in this processor, so we'll leave it unconnected
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);
    xyPadLabel.setText("Threshold / Release", juce::dontSendNotification);
    xyPadLabel.setJustificationType(juce::Justification::centred);
    xyPadLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    xyPad.setTooltip("Click and drag to control assigned parameters. Right-click parameter labels to assign X/Y axes.");

    // Add gate LED
    addAndMakeVisible(gateLED);
    gateLEDLabel.setText("Gate Status", juce::dontSendNotification);
    gateLEDLabel.setJustificationType(juce::Justification::centred);
    gateLEDLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
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
    
    // Tooltips
    thresholdSlider.setTooltip("Signal level below which the gate closes and mutes audio");
    attackSlider.setTooltip("How quickly the gate opens when signal exceeds threshold");
    holdSlider.setTooltip("Minimum time the gate stays open after signal drops");
    releaseSlider.setTooltip("How quickly the gate closes after hold time expires");
    rangeSlider.setTooltip("How much the signal is attenuated when the gate is closed");
    lookaheadSlider.setTooltip("Look ahead time for smoother gate operation");
    bypassButton.setTooltip("Bypass the effect");

    setSize(700, 550);
    setResizable(true, true);
    setResizeLimits(600, 520, 900, 750);
}

NoiseGateEditor::~NoiseGateEditor()
{
    setLookAndFeel(nullptr);
}

void NoiseGateEditor::paint(juce::Graphics& g)
{
    g.fillAll(HyperPrismLookAndFeel::Colors::background);

    // Accent line
    g.setColour(HyperPrismLookAndFeel::Colors::primary.withAlpha(0.4f));
    g.fillRect(12, 4, getWidth() - 24, 2);

    // Version
    g.setColour(HyperPrismLookAndFeel::Colors::outline);
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.drawText("v1.0.0", getLocalBounds().removeFromBottom(20).removeFromRight(70),
               juce::Justification::centredRight);

    // Column section headers
    auto paintColumnHeader = [&](int x, int y, int width,
                                  const juce::String& title, juce::Colour color)
    {
        g.setColour(color.withAlpha(0.7f));
        g.setFont(juce::Font(juce::FontOptions(9.0f).withStyle("Bold")));
        g.drawText(title, x, y, width, 14, juce::Justification::centredLeft);
        g.setColour(HyperPrismLookAndFeel::Colors::outline.withAlpha(0.3f));
        g.drawLine(static_cast<float>(x), static_cast<float>(y + 14),
                   static_cast<float>(x + width), static_cast<float>(y + 14), 0.5f);
    };

    paintColumnHeader(thresholdSlider.getX() - 2, thresholdSlider.getY() - 20, 120,
                      "DETECTION", HyperPrismLookAndFeel::Colors::dynamics);
    paintColumnHeader(attackSlider.getX() - 2, attackSlider.getY() - 20, 120,
                      "TIMING", HyperPrismLookAndFeel::Colors::timing);
}

void NoiseGateEditor::resized()
{
    auto bounds = getLocalBounds();

    // === HEADER (72px) ===
    auto header = bounds.removeFromTop(72);
    titleLabel.setBounds(header.getX() + 12, 30, header.getWidth() - 112, 20);
    brandLabel.setBounds(header.getX() + 12, 50, header.getWidth() - 112, 16);
    bypassButton.setBounds(header.getRight() - 90, 36, 80, 26);

    // === FOOTER ===
    bounds.removeFromBottom(20);

    // === CONTENT ===
    bounds.reduce(12, 4);

    // --- Left: Two parameter columns (dynamic width) ---
    int rightSideWidth = 312;
    int columnsTotalWidth = bounds.getWidth() - rightSideWidth;
    auto columnsArea = bounds.removeFromLeft(columnsTotalWidth);
    int colWidth = (columnsArea.getWidth() - 10) / 2;
    auto col1 = columnsArea.removeFromLeft(colWidth);
    columnsArea.removeFromLeft(10);
    auto col2 = columnsArea;

    int knobDiam = 80;
    int vSpace = 107;
    int colTop = col1.getY() + 20;

    auto centerKnob = [&](juce::Slider& slider, juce::Label& label,
                           int colX, int colW, int cy, int kd)
    {
        int kx = colX + (colW - kd) / 2;
        int ky = cy - kd / 2;
        slider.setBounds(kx, ky, kd, kd);
        label.setBounds(colX, ky + kd + 1, colW, 16);
    };

    // Column 1: DETECTION -- Threshold, Range, Lookahead
    int y1 = colTop + knobDiam / 2;
    centerKnob(thresholdSlider, thresholdLabel, col1.getX(), colWidth, y1, knobDiam);
    centerKnob(rangeSlider, rangeLabel, col1.getX(), colWidth, y1 + vSpace, knobDiam);
    centerKnob(lookaheadSlider, lookaheadLabel, col1.getX(), colWidth, y1 + vSpace * 2, knobDiam);

    // Column 2: TIMING -- Attack, Hold, Release
    centerKnob(attackSlider, attackLabel, col2.getX(), colWidth, y1, knobDiam);
    centerKnob(holdSlider, holdLabel, col2.getX(), colWidth, y1 + vSpace, knobDiam);
    centerKnob(releaseSlider, releaseLabel, col2.getX(), colWidth, y1 + vSpace * 2, knobDiam);

    // --- Right side: XY pad + gate LED ---
    auto rightSide = bounds;
    rightSide.removeFromLeft(12);

    int outputHeight = 130;
    int xyHeight = juce::jmax(200, rightSide.getHeight() - outputHeight - 22);
    auto xyArea = rightSide.removeFromTop(xyHeight);
    xyPad.setBounds(xyArea);
    xyPadLabel.setBounds(xyArea.getX(), xyArea.getBottom() + 2, xyArea.getWidth(), 16);
    rightSide.removeFromTop(20);

    // Bottom right: Gate LED centered (no output knobs)
    auto bottomRight = rightSide;

    outputSectionX = bottomRight.getX();
    outputSectionY = bottomRight.getY();

    int ledSize = juce::jmin(80, bottomRight.getHeight());
    int ledX = bottomRight.getX() + (bottomRight.getWidth() - ledSize) / 2;
    gateLED.setBounds(ledX, bottomRight.getY(), ledSize, ledSize);
    gateLEDLabel.setBounds(ledX, bottomRight.getY() + ledSize + 2, ledSize, 16);
}

void NoiseGateEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
                               const juce::String& text)
{
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    slider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::primary);

    addAndMakeVisible(slider);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    addAndMakeVisible(label);
}

void NoiseGateEditor::updateParameterColors()
{
    auto neutralColor = HyperPrismLookAndFeel::Colors::onSurfaceVariant;
    thresholdLabel.setColour(juce::Label::textColourId, neutralColor);
    attackLabel.setColour(juce::Label::textColourId, neutralColor);
    holdLabel.setColour(juce::Label::textColourId, neutralColor);
    releaseLabel.setColour(juce::Label::textColourId, neutralColor);
    rangeLabel.setColour(juce::Label::textColourId, neutralColor);
    lookaheadLabel.setColour(juce::Label::textColourId, neutralColor);

    // Set XY assignment properties on sliders for LookAndFeel badge drawing
    auto updateSliderXY = [this](juce::Slider& slider, const juce::String& paramID)
    {
        if (xParameterNames.contains(paramID))
            slider.getProperties().set("xyAxisX", true);
        else
            slider.getProperties().remove("xyAxisX");

        if (yParameterNames.contains(paramID))
            slider.getProperties().set("xyAxisY", true);
        else
            slider.getProperties().remove("xyAxisY");

        slider.repaint();
    };

    updateSliderXY(thresholdSlider, "threshold");
    updateSliderXY(attackSlider, "attack");
    updateSliderXY(holdSlider, "hold");
    updateSliderXY(releaseSlider, "release");
    updateSliderXY(rangeSlider, "range");
    updateSliderXY(lookaheadSlider, "lookahead");
    repaint();
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


void NoiseGateEditor::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        auto* source = event.eventComponent;
        auto paramID = source->getProperties()["xyParamID"].toString();
        if (paramID.isNotEmpty())
            showParameterMenu(source, paramID);
    }
}
void NoiseGateEditor::showParameterMenu(juce::Component* target, const juce::String& parameterName)
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
        .withTargetComponent(target)
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