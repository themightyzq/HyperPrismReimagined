//==============================================================================
// HyperPrism Reimagined - Pan Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "PanEditor.h"

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
// PanMeter Implementation
//==============================================================================
PanMeter::PanMeter(PanProcessor& p) 
    : processor(p)
{
    startTimerHz(30); // 30 FPS update rate
}

PanMeter::~PanMeter()
{
    stopTimer();
}

void PanMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(HyperPrismLookAndFeel::Colors::background);
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Border
    g.setColour(HyperPrismLookAndFeel::Colors::outline);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    
    // Create meter area
    auto meterArea = bounds.reduced(10.0f);
    
    // Draw pan position arc visualization
    float centerX = meterArea.getCentreX();
    float centerY = meterArea.getCentreY();
    float radius = juce::jmin(meterArea.getWidth(), meterArea.getHeight()) * 0.4f;
    
    // Background arc
    g.setColour(HyperPrismLookAndFeel::Colors::surfaceVariant);
    juce::Path arcPath;
    arcPath.addCentredArc(centerX, centerY, radius, radius, 0.0f, -juce::MathConstants<float>::halfPi, juce::MathConstants<float>::halfPi, true);
    g.strokePath(arcPath, juce::PathStrokeType(4.0f));
    
    // L/R indicators
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(12.0f);
    g.drawText("L", centerX - radius - 20, centerY - 10, 20, 20, juce::Justification::centred);
    g.drawText("R", centerX + radius, centerY - 10, 20, 20, juce::Justification::centred);
    g.drawText("C", centerX - 10, centerY - radius - 20, 20, 20, juce::Justification::centred);
    
    // Pan position indicator
    float panAngle = -juce::MathConstants<float>::halfPi + (panPosition * juce::MathConstants<float>::halfPi);
    float indicatorX = centerX + radius * std::cos(panAngle);
    float indicatorY = centerY + radius * std::sin(panAngle);
    
    // Draw pan position line
    g.setColour(HyperPrismLookAndFeel::Colors::primary);
    g.drawLine(centerX, centerY, indicatorX, indicatorY, 3.0f);
    
    // Draw pan position dot
    g.setColour(HyperPrismLookAndFeel::Colors::primary);
    g.fillEllipse(indicatorX - 6, indicatorY - 6, 12, 12);
    g.setColour(HyperPrismLookAndFeel::Colors::onSurface);
    g.fillEllipse(indicatorX - 3, indicatorY - 3, 6, 6);
    
    // Draw level bars at bottom
    auto levelArea = meterArea.removeFromBottom(40);
    levelArea.removeFromTop(10);
    
    float barWidth = levelArea.getWidth() * 0.35f;
    float spacing = levelArea.getWidth() * 0.1f;
    
    // Left level bar
    auto leftBar = levelArea.removeFromLeft(barWidth);
    g.setColour(HyperPrismLookAndFeel::Colors::surfaceVariant);
    g.fillRoundedRectangle(leftBar.toFloat(), 2.0f);
    
    if (leftLevel > 0.001f)
    {
        auto leftLevelBar = leftBar.toFloat();
        leftLevelBar.removeFromLeft(leftLevelBar.getWidth() * (1.0f - leftLevel));
        g.setColour(HyperPrismLookAndFeel::Colors::success);
        g.fillRoundedRectangle(leftLevelBar, 2.0f);
    }
    
    levelArea.removeFromLeft(spacing);
    
    // Right level bar
    auto rightBar = levelArea.removeFromLeft(barWidth);
    g.setColour(HyperPrismLookAndFeel::Colors::surfaceVariant);
    g.fillRoundedRectangle(rightBar.toFloat(), 2.0f);
    
    if (rightLevel > 0.001f)
    {
        auto rightLevelBar = rightBar.toFloat();
        rightLevelBar.removeFromLeft(rightLevelBar.getWidth() * (1.0f - rightLevel));
        g.setColour(HyperPrismLookAndFeel::Colors::primary);
        g.fillRoundedRectangle(rightLevelBar, 2.0f);
    }
    
}

void PanMeter::timerCallback()
{
    float newLeftLevel = processor.getLeftLevel();
    float newRightLevel = processor.getRightLevel();
    float newPanPosition = processor.getPanPosition();
    
    // Smooth the changes
    const float smoothing = 0.7f;
    leftLevel = leftLevel * smoothing + newLeftLevel * (1.0f - smoothing);
    rightLevel = rightLevel * smoothing + newRightLevel * (1.0f - smoothing);
    panPosition = panPosition * smoothing + newPanPosition * (1.0f - smoothing);
    
    repaint();
}

//==============================================================================
// PanEditor Implementation
//==============================================================================
PanEditor::PanEditor(PanProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), panMeter(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add(PanProcessor::PAN_POSITION_ID);
    yParameterIDs.add(PanProcessor::WIDTH_ID);
    
    // Title
    titleLabel.setText("PAN", juce::dontSendNotification);
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
    
    // Setup sliders with consistent style (4 sliders + 1 dropdown = 5 parameters)
    setupSlider(panPositionSlider, panPositionLabel, "Pan Position");
    panPositionSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::modulation);
    setupSlider(widthSlider, widthLabel, "Width");
    widthSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::modulation);
    setupSlider(balanceSlider, balanceLabel, "Balance");
    balanceSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::modulation);
    setupSlider(outputLevelSlider, outputLevelLabel, "Output");
    outputLevelSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::output);
    
    // Set parameter ranges
    panPositionSlider.setRange(-1.0, 1.0, 0.01);
    widthSlider.setRange(0.0, 100.0, 0.1);
    balanceSlider.setRange(-1.0, 1.0, 0.01);
    outputLevelSlider.setRange(-20.0, 20.0, 0.1);
    
    // Pan law dropdown
    panLawComboBox.addItem("Linear", 1);
    panLawComboBox.addItem("-3dB", 2);
    panLawComboBox.addItem("-4.5dB", 3);
    panLawComboBox.addItem("-6dB", 4);
    panLawComboBox.setColour(juce::ComboBox::backgroundColourId, HyperPrismLookAndFeel::Colors::surfaceVariant);
    panLawComboBox.setColour(juce::ComboBox::textColourId, HyperPrismLookAndFeel::Colors::onSurface);
    panLawComboBox.setColour(juce::ComboBox::arrowColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    panLawComboBox.setColour(juce::ComboBox::outlineColourId, HyperPrismLookAndFeel::Colors::outline);
    addAndMakeVisible(panLawComboBox);
    
    panLawLabel.setText("Pan Law", juce::dontSendNotification);
    panLawLabel.setJustificationType(juce::Justification::centred);
    panLawLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    addAndMakeVisible(panLawLabel);
    
    // Set up right-click handlers for parameter assignment
    panPositionLabel.onClick = [this]() { showParameterMenu(&panPositionLabel, PanProcessor::PAN_POSITION_ID); };
    widthLabel.onClick = [this]() { showParameterMenu(&widthLabel, PanProcessor::WIDTH_ID); };
    balanceLabel.onClick = [this]() { showParameterMenu(&balanceLabel, PanProcessor::BALANCE_ID); };
    outputLevelLabel.onClick = [this]() { showParameterMenu(&outputLevelLabel, PanProcessor::OUTPUT_LEVEL_ID); };

    // Register right-click on sliders for XY pad assignment
    panPositionSlider.addMouseListener(this, true);
    panPositionSlider.getProperties().set("xyParamID", PanProcessor::PAN_POSITION_ID);
    widthSlider.addMouseListener(this, true);
    widthSlider.getProperties().set("xyParamID", PanProcessor::WIDTH_ID);
    balanceSlider.addMouseListener(this, true);
    balanceSlider.getProperties().set("xyParamID", PanProcessor::BALANCE_ID);
    outputLevelSlider.addMouseListener(this, true);
    outputLevelSlider.getProperties().set("xyParamID", PanProcessor::OUTPUT_LEVEL_ID);

    
    // Bypass button (top right)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setClickingTogglesState(true);
    addAndMakeVisible(bypassButton);
    
    // Create attachments
    auto& vts = audioProcessor.getValueTreeState();
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        vts, PanProcessor::BYPASS_ID, bypassButton);
    panPositionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, PanProcessor::PAN_POSITION_ID, panPositionSlider);
    panLawAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        vts, PanProcessor::PAN_LAW_ID, panLawComboBox);
    widthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, PanProcessor::WIDTH_ID, widthSlider);
    balanceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, PanProcessor::BALANCE_ID, balanceSlider);
    outputLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, PanProcessor::OUTPUT_LEVEL_ID, outputLevelSlider);
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);
    xyPadLabel.setText("Pan Position / Width", juce::dontSendNotification);
    xyPadLabel.setJustificationType(juce::Justification::centred);
    xyPadLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    xyPad.setTooltip("Click and drag to control assigned parameters. Right-click parameter labels to assign X/Y axes.");

    // Add pan meter
    addAndMakeVisible(panMeter);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    panPositionSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    widthSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    balanceSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    outputLevelSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    // Tooltips
    panPositionSlider.setTooltip("Stereo position — left to right");
    widthSlider.setTooltip("Stereo width of the signal");
    balanceSlider.setTooltip("Panning law curve — affects how volume changes");
    outputLevelSlider.setTooltip("Overall output volume");
    bypassButton.setTooltip("Bypass the effect");

    setSize(700, 550);
    setResizable(true, true);
    setResizeLimits(600, 520, 900, 750);
}

PanEditor::~PanEditor()
{
    setLookAndFeel(nullptr);
}

void PanEditor::paint(juce::Graphics& g)
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

    paintColumnHeader(panPositionSlider.getX() - 2, panPositionSlider.getY() - 20, 120,
                      "POSITION", HyperPrismLookAndFeel::Colors::modulation);
    paintColumnHeader(panLawComboBox.getX() - 2, panLawComboBox.getY() - 20, 120,
                      "PAN LAW", HyperPrismLookAndFeel::Colors::modulation);

    // Output section header
    paintColumnHeader(outputSectionX, outputSectionY,
                      getWidth() - outputSectionX - 12,
                      "OUTPUT", HyperPrismLookAndFeel::Colors::output);
}

void PanEditor::resized()
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

    // Column 1: POSITION -- Pan Position, Width, Balance
    int y1 = colTop + knobDiam / 2;
    centerKnob(panPositionSlider, panPositionLabel, col1.getX(), colWidth, y1, knobDiam);
    centerKnob(widthSlider, widthLabel, col1.getX(), colWidth, y1 + vSpace, knobDiam);
    centerKnob(balanceSlider, balanceLabel, col1.getX(), colWidth, y1 + vSpace * 2, knobDiam);

    // Column 2: PAN LAW -- combo box
    int col2Top = col2.getY() + 20;
    int comboY = col2Top + 20;
    panLawComboBox.setBounds(col2.getX() + 5, comboY, colWidth - 10, 24);
    panLawLabel.setBounds(col2.getX(), comboY + 26, colWidth, 14);

    // --- Right side: XY pad + output + meter ---
    auto rightSide = bounds;
    rightSide.removeFromLeft(12);

    int outputHeight = 130;
    int xyHeight = juce::jmax(200, rightSide.getHeight() - outputHeight - 22);
    auto xyArea = rightSide.removeFromTop(xyHeight);
    xyPad.setBounds(xyArea);
    xyPadLabel.setBounds(xyArea.getX(), xyArea.getBottom() + 2, xyArea.getWidth(), 16);
    rightSide.removeFromTop(20);

    // Bottom right: Output knob + Meter
    auto bottomRight = rightSide;
    auto outputArea = bottomRight.removeFromLeft(140);
    auto meterArea = bottomRight.reduced(4);

    int outKnob = 58;
    int outY = outputArea.getY() + 24;

    centerKnob(outputLevelSlider, outputLevelLabel, outputArea.getX() + 20, 100, outY + outKnob / 2, outKnob);

    outputSectionX = outputArea.getX();
    outputSectionY = outputArea.getY();

    // Meter
    panMeter.setBounds(meterArea.reduced(4));
}

void PanEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void PanEditor::updateParameterColors()
{
    auto neutralColor = HyperPrismLookAndFeel::Colors::onSurfaceVariant;
    panPositionLabel.setColour(juce::Label::textColourId, neutralColor);
    widthLabel.setColour(juce::Label::textColourId, neutralColor);
    balanceLabel.setColour(juce::Label::textColourId, neutralColor);
    outputLevelLabel.setColour(juce::Label::textColourId, neutralColor);

    // Set XY assignment properties on sliders for LookAndFeel badge drawing
    auto updateSliderXY = [this](juce::Slider& slider, const juce::String& paramID)
    {
        if (xParameterIDs.contains(paramID))
            slider.getProperties().set("xyAxisX", true);
        else
            slider.getProperties().remove("xyAxisX");

        if (yParameterIDs.contains(paramID))
            slider.getProperties().set("xyAxisY", true);
        else
            slider.getProperties().remove("xyAxisY");

        slider.repaint();
    };

    updateSliderXY(panPositionSlider, PanProcessor::PAN_POSITION_ID);
    updateSliderXY(widthSlider, PanProcessor::WIDTH_ID);
    updateSliderXY(balanceSlider, PanProcessor::BALANCE_ID);
    updateSliderXY(outputLevelSlider, PanProcessor::OUTPUT_LEVEL_ID);
    repaint();
}

void PanEditor::updateXYPadFromParameters()
{
    // For multiple parameters, use the average of their normalized values
    float xValue = 0.0f;
    float yValue = 0.0f;
    
    auto& vts = audioProcessor.getValueTreeState();
    
    // Calculate average X value
    if (!xParameterIDs.isEmpty())
    {
        for (const auto& paramID : xParameterIDs)
        {
            if (auto* param = vts.getRawParameterValue(paramID))
            {
                if (auto* paramObj = vts.getParameter(paramID))
                {
                    xValue += paramObj->convertTo0to1(*param);
                }
            }
        }
        xValue /= xParameterIDs.size();
    }
    
    // Calculate average Y value
    if (!yParameterIDs.isEmpty())
    {
        for (const auto& paramID : yParameterIDs)
        {
            if (auto* param = vts.getRawParameterValue(paramID))
            {
                if (auto* paramObj = vts.getParameter(paramID))
                {
                    yValue += paramObj->convertTo0to1(*param);
                }
            }
        }
        yValue /= yParameterIDs.size();
    }
    
    xyPad.setValues(xValue, yValue);
}

void PanEditor::updateParametersFromXYPad(float x, float y)
{
    auto& vts = audioProcessor.getValueTreeState();
    
    // Update all assigned X parameters
    for (const auto& paramID : xParameterIDs)
    {
        if (auto* param = vts.getParameter(paramID))
            param->setValueNotifyingHost(x);
    }
    
    // Update all assigned Y parameters
    for (const auto& paramID : yParameterIDs)
    {
        if (auto* param = vts.getParameter(paramID))
            param->setValueNotifyingHost(y);
    }
}


void PanEditor::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        auto* source = event.eventComponent;
        auto paramID = source->getProperties()["xyParamID"].toString();
        if (paramID.isNotEmpty())
            showParameterMenu(source, paramID);
    }
}
void PanEditor::showParameterMenu(juce::Component* target, const juce::String& parameterID)
{
    juce::PopupMenu menu;
    
    // Add header
    menu.addSectionHeader("Assign to X/Y Pad");
    menu.addSeparator();
    
    // Check if this parameter is currently assigned
    bool isAssignedToX = xParameterIDs.contains(parameterID);
    bool isAssignedToY = yParameterIDs.contains(parameterID);
    
    menu.addItem(1, "Toggle X-axis", true, isAssignedToX);
    menu.addItem(2, "Toggle Y-axis", true, isAssignedToY);
    
    menu.addSeparator();
    menu.addItem(3, "Clear all assignments");
    
    // Show the menu
    menu.showMenuAsync(juce::PopupMenu::Options()
        .withTargetComponent(target)
        .withMinimumWidth(150),
        [this, parameterID](int result)
        {
            if (result == 1)
            {
                // Toggle X assignment
                if (xParameterIDs.contains(parameterID))
                    xParameterIDs.removeString(parameterID);
                else
                    xParameterIDs.add(parameterID);
                    
                // Ensure at least one parameter is assigned
                if (xParameterIDs.isEmpty())
                    xParameterIDs.add(PanProcessor::PAN_POSITION_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 2)
            {
                // Toggle Y assignment
                if (yParameterIDs.contains(parameterID))
                    yParameterIDs.removeString(parameterID);
                else
                    yParameterIDs.add(parameterID);
                    
                // Ensure at least one parameter is assigned
                if (yParameterIDs.isEmpty())
                    yParameterIDs.add(PanProcessor::WIDTH_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add(PanProcessor::PAN_POSITION_ID);
                yParameterIDs.add(PanProcessor::WIDTH_ID);
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void PanEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == PanProcessor::PAN_POSITION_ID) return "Pan Position";
        if (paramID == PanProcessor::WIDTH_ID) return "Width";
        if (paramID == PanProcessor::BALANCE_ID) return "Balance";
        if (paramID == PanProcessor::OUTPUT_LEVEL_ID) return "Output";
        return paramID;
    };
    
    juce::String xLabel;
    juce::String yLabel;
    
    // Build X label
    if (xParameterIDs.size() == 0)
        xLabel = "None";
    else if (xParameterIDs.size() == 1)
        xLabel = getParameterName(xParameterIDs[0]);
    else
        xLabel = "Multiple";
        
    // Build Y label
    if (yParameterIDs.size() == 0)
        yLabel = "None";
    else if (yParameterIDs.size() == 1)
        yLabel = getParameterName(yParameterIDs[0]);
    else
        yLabel = "Multiple";
        
    xyPadLabel.setText(xLabel + " / " + yLabel, juce::dontSendNotification);
}