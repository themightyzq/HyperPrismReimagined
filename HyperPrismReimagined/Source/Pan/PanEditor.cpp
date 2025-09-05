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
    
    // Pan value text
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(10.0f);
    juce::String panText = (std::abs(panPosition) < 0.01f) ? "CENTER" : 
                          (panPosition > 0.0f ? "R " + juce::String(static_cast<int>(panPosition * 100)) + "%" : 
                           "L " + juce::String(static_cast<int>(-panPosition * 100)) + "%");
    g.drawText(panText, bounds.removeFromBottom(15).reduced(5, 0), juce::Justification::centred);
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
    
    // Title (matching AutoPan style)
    titleLabel.setText("HyperPrism Reimagined Pan", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions("Arial", "Bold", 24.0f)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Setup sliders with consistent style (4 sliders + 1 dropdown = 5 parameters)
    setupSlider(panPositionSlider, panPositionLabel, "Pan Position", "");
    setupSlider(widthSlider, widthLabel, "Width", "");
    setupSlider(balanceSlider, balanceLabel, "Balance", "");
    setupSlider(outputLevelSlider, outputLevelLabel, "Output Level", " dB");
    
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
    panLawComboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colours::darkgrey);
    panLawComboBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    panLawComboBox.setColour(juce::ComboBox::arrowColourId, juce::Colours::lightgrey);
    panLawComboBox.setColour(juce::ComboBox::outlineColourId, juce::Colours::grey);
    addAndMakeVisible(panLawComboBox);
    
    panLawLabel.setText("Pan Law", juce::dontSendNotification);
    panLawLabel.setJustificationType(juce::Justification::centred);
    panLawLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(panLawLabel);
    
    // Set up right-click handlers for parameter assignment
    panPositionLabel.onClick = [this]() { showParameterMenu(&panPositionLabel, PanProcessor::PAN_POSITION_ID); };
    widthLabel.onClick = [this]() { showParameterMenu(&widthLabel, PanProcessor::WIDTH_ID); };
    balanceLabel.onClick = [this]() { showParameterMenu(&balanceLabel, PanProcessor::BALANCE_ID); };
    outputLevelLabel.onClick = [this]() { showParameterMenu(&outputLevelLabel, PanProcessor::OUTPUT_LEVEL_ID); };
    
    // Bypass button (top right like AutoPan)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::red);
    bypassButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
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
    xyPadLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    
    // Add pan meter
    addAndMakeVisible(panMeter);
    panMeterLabel.setText("Pan Visualization", juce::dontSendNotification);
    panMeterLabel.setJustificationType(juce::Justification::centred);
    panMeterLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(panMeterLabel);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    panPositionSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    widthSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    balanceSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    outputLevelSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    setSize(650, 600);
}

PanEditor::~PanEditor()
{
    setLookAndFeel(nullptr);
}

void PanEditor::paint(juce::Graphics& g)
{
    // Dark background matching AutoPan
    g.fillAll(HyperPrismLookAndFeel::Colors::background);
}

void PanEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(40));
    
    // Bypass button (top right)
    bypassButton.setBounds(bounds.getWidth() - 100, 10, 80, 30);
    
    bounds.reduce(20, 10);
    
    // Optimized layout for 650x600 - single row with all 4 controls
    auto sliderWidth = 80;
    auto spacing = 15;
    
    // Single row with all 4 controls
    auto controlsRow = bounds.removeFromTop(130);
    auto totalControlsWidth = sliderWidth * 4 + spacing * 3;
    auto controlsStartX = (bounds.getWidth() - totalControlsWidth) / 2;
    controlsRow.removeFromLeft(controlsStartX);
    
    // All controls in one row: Pan Position, Width, Balance, Output Level
    panPositionSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    panPositionLabel.setBounds(panPositionSlider.getX(), panPositionSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    widthSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    widthLabel.setBounds(widthSlider.getX(), widthSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    balanceSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    balanceLabel.setBounds(balanceSlider.getX(), balanceSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    outputLevelSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    outputLevelLabel.setBounds(outputLevelSlider.getX(), outputLevelSlider.getBottom(), sliderWidth, 20);
    
    bounds.removeFromTop(15);
    
    // Pan Law dropdown (centered, tighter spacing)
    auto dropdownRow = bounds.removeFromTop(60);
    auto dropdownWidth = 120;
    auto dropdownX = bounds.getX() + (bounds.getWidth() - dropdownWidth) / 2;
    
    panLawLabel.setBounds(dropdownX, dropdownRow.getY() + 5, dropdownWidth, 20);
    panLawComboBox.setBounds(dropdownX, dropdownRow.getY() + 25, dropdownWidth, 25);
    
    // Bottom section - XY Pad and Meter side by side (brought up)
    bounds.removeFromTop(15);
    
    // Split remaining space horizontally for XY pad and meter
    auto bottomArea = bounds;
    auto panelHeight = 180; // Standard XY pad height
    
    // Calculate positioning to center both panels
    auto xyPadWidth = 200;
    auto meterSize = 180;
    auto totalBottomWidth = xyPadWidth + 20 + meterSize; // XY pad + spacing + meter
    auto bottomStartX = (bottomArea.getWidth() - totalBottomWidth) / 2;
    
    // XY Pad on left
    auto xyPadBounds = bottomArea.withX(bottomArea.getX() + bottomStartX).withWidth(xyPadWidth).withHeight(panelHeight);
    xyPad.setBounds(xyPadBounds);
    
    // Pan meter on right (matching height)
    auto meterBounds = bottomArea.withX(xyPadBounds.getRight() + 20).withWidth(meterSize).withHeight(panelHeight);
    panMeter.setBounds(meterBounds);
    
    // Align labels at the same Y position
    auto labelY = xyPadBounds.getBottom() + 5;
    xyPadLabel.setBounds(xyPad.getX(), labelY, xyPadWidth, 20);
    panMeterLabel.setBounds(panMeter.getX(), labelY, meterSize, 20);
}

void PanEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void PanEditor::updateParameterColors()
{
    // Update label colors based on X/Y assignments
    auto updateLabelColor = [this](ParameterLabel& label, const juce::String& paramID) {
        bool isAssignedToX = xParameterIDs.contains(paramID);
        bool isAssignedToY = yParameterIDs.contains(paramID);
        
        if (isAssignedToX && isAssignedToY)
            label.setColour(juce::Label::textColourId, xAssignmentColor.interpolatedWith(yAssignmentColor, 0.5f));
        else if (isAssignedToX)
            label.setColour(juce::Label::textColourId, xAssignmentColor);
        else if (isAssignedToY)
            label.setColour(juce::Label::textColourId, yAssignmentColor);
        else
            label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    };
    
    updateLabelColor(panPositionLabel, PanProcessor::PAN_POSITION_ID);
    updateLabelColor(widthLabel, PanProcessor::WIDTH_ID);
    updateLabelColor(balanceLabel, PanProcessor::BALANCE_ID);
    updateLabelColor(outputLevelLabel, PanProcessor::OUTPUT_LEVEL_ID);
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

void PanEditor::showParameterMenu(juce::Label* label, const juce::String& parameterID)
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
        .withTargetComponent(label)
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
        if (paramID == PanProcessor::OUTPUT_LEVEL_ID) return "Output Level";
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