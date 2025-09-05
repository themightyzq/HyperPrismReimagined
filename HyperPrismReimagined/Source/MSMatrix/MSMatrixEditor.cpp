//==============================================================================
// HyperPrism Reimagined - M+S Matrix Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "MSMatrixEditor.h"

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
// MSMeter Implementation
//==============================================================================
MSMeter::MSMeter(MSMatrixProcessor& p) 
    : processor(p)
{
    startTimerHz(30); // 30 FPS update rate
}

MSMeter::~MSMeter()
{
    stopTimer();
}

void MSMeter::paint(juce::Graphics& g)
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
    
    // Draw stereo field visualization
    float centerX = meterArea.getCentreX();
    float centerY = meterArea.getCentreY();
    float radius = juce::jmin(meterArea.getWidth(), meterArea.getHeight()) * 0.4f;
    
    // Draw background circle
    g.setColour(HyperPrismLookAndFeel::Colors::surfaceVariant);
    g.fillEllipse(centerX - radius, centerY - radius, radius * 2, radius * 2);
    
    // Draw axes
    g.setColour(HyperPrismLookAndFeel::Colors::outlineVariant);
    g.drawLine(centerX - radius - 10, centerY, centerX + radius + 10, centerY, 1.0f);
    g.drawLine(centerX, centerY - radius - 10, centerX, centerY + radius + 10, 1.0f);
    
    // Draw M/S levels as a dot in the stereo field
    float xPos = centerX + (sideLevel - 0.5f) * 2.0f * radius * 0.8f;
    float yPos = centerY - (midLevel - 0.5f) * 2.0f * radius * 0.8f;
    
    // Draw level dot with glow effect
    g.setColour(HyperPrismLookAndFeel::Colors::primary.withAlpha(0.3f));
    g.fillEllipse(xPos - 12, yPos - 12, 24, 24);
    
    g.setColour(HyperPrismLookAndFeel::Colors::primary);
    g.fillEllipse(xPos - 8, yPos - 8, 16, 16);
    
    g.setColour(juce::Colours::white);
    g.fillEllipse(xPos - 4, yPos - 4, 8, 8);
    
    // Labels
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(10.0f);
    g.drawText("M", centerX - 5, centerY - radius - 25, 10, 15, juce::Justification::centred);
    g.drawText("S", centerX + radius + 5, centerY - 7, 15, 15, juce::Justification::left);
    
    // Level values
    g.setFont(9.0f);
    g.setColour(HyperPrismLookAndFeel::Colors::primary);
    auto valueArea = bounds.removeFromBottom(20).reduced(5, 0);
    g.drawText("M: " + juce::String(midLevel, 2) + " S: " + juce::String(sideLevel, 2), 
               valueArea, juce::Justification::centred);
}

void MSMeter::timerCallback()
{
    float newLeftLevel = processor.getLeftLevel();
    float newRightLevel = processor.getRightLevel();
    float newMidLevel = processor.getMidLevel();
    float newSideLevel = processor.getSideLevel();
    
    // Smooth the level changes
    const float smoothing = 0.7f;
    leftLevel = leftLevel * smoothing + newLeftLevel * (1.0f - smoothing);
    rightLevel = rightLevel * smoothing + newRightLevel * (1.0f - smoothing);
    midLevel = midLevel * smoothing + newMidLevel * (1.0f - smoothing);
    sideLevel = sideLevel * smoothing + newSideLevel * (1.0f - smoothing);
    
    repaint();
}

//==============================================================================
// MSMatrixEditor Implementation
//==============================================================================
MSMatrixEditor::MSMatrixEditor(MSMatrixProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), msMeter(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add(MSMatrixProcessor::MID_LEVEL_ID);
    yParameterIDs.add(MSMatrixProcessor::SIDE_LEVEL_ID);
    
    // Title (matching AutoPan style)
    titleLabel.setText("HyperPrism Reimagined M+S Matrix", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions("Arial", "Bold", 24.0f)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Setup sliders with consistent style (4 sliders)
    setupSlider(midLevelSlider, midLevelLabel, "Mid Level", " dB");
    setupSlider(sideLevelSlider, sideLevelLabel, "Side Level", " dB");
    setupSlider(stereoBalanceSlider, stereoBalanceLabel, "Stereo Balance", "");
    setupSlider(outputLevelSlider, outputLevelLabel, "Output Level", " dB");
    
    // Set up right-click handlers for parameter assignment
    midLevelLabel.onClick = [this]() { showParameterMenu(&midLevelLabel, MSMatrixProcessor::MID_LEVEL_ID); };
    sideLevelLabel.onClick = [this]() { showParameterMenu(&sideLevelLabel, MSMatrixProcessor::SIDE_LEVEL_ID); };
    stereoBalanceLabel.onClick = [this]() { showParameterMenu(&stereoBalanceLabel, MSMatrixProcessor::STEREO_BALANCE_ID); };
    outputLevelLabel.onClick = [this]() { showParameterMenu(&outputLevelLabel, MSMatrixProcessor::OUTPUT_LEVEL_ID); };
    
    // Matrix mode selector
    matrixModeComboBox.addItem("L/R → M/S", 1);
    matrixModeComboBox.addItem("M/S → L/R", 2);
    matrixModeComboBox.addItem("M/S Through", 3);
    matrixModeComboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colours::darkgrey);
    matrixModeComboBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    matrixModeComboBox.setColour(juce::ComboBox::arrowColourId, juce::Colours::lightgrey);
    matrixModeComboBox.setColour(juce::ComboBox::outlineColourId, juce::Colours::grey);
    addAndMakeVisible(matrixModeComboBox);
    
    matrixModeLabel.setText("Matrix Mode", juce::dontSendNotification);
    matrixModeLabel.setJustificationType(juce::Justification::centred);
    matrixModeLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(matrixModeLabel);
    
    // Solo buttons
    midSoloButton.setButtonText("Mid Solo");
    midSoloButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    midSoloButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::cyan);
    addAndMakeVisible(midSoloButton);
    
    midSoloLabel.setText("Mid Solo", juce::dontSendNotification);
    midSoloLabel.setJustificationType(juce::Justification::centred);
    midSoloLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(midSoloLabel);
    
    sideSoloButton.setButtonText("Side Solo");
    sideSoloButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    sideSoloButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::cyan);
    addAndMakeVisible(sideSoloButton);
    
    sideSoloLabel.setText("Side Solo", juce::dontSendNotification);
    sideSoloLabel.setJustificationType(juce::Justification::centred);
    sideSoloLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(sideSoloLabel);
    
    // Bypass button (top right like AutoPan)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::red);
    bypassButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
    addAndMakeVisible(bypassButton);
    
    // Create attachments
    auto& vts = audioProcessor.getValueTreeState();
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        vts, MSMatrixProcessor::BYPASS_ID, bypassButton);
    matrixModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        vts, MSMatrixProcessor::MATRIX_MODE_ID, matrixModeComboBox);
    midLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, MSMatrixProcessor::MID_LEVEL_ID, midLevelSlider);
    sideLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, MSMatrixProcessor::SIDE_LEVEL_ID, sideLevelSlider);
    midSoloAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        vts, MSMatrixProcessor::MID_SOLO_ID, midSoloButton);
    sideSoloAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        vts, MSMatrixProcessor::SIDE_SOLO_ID, sideSoloButton);
    stereoBalanceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, MSMatrixProcessor::STEREO_BALANCE_ID, stereoBalanceSlider);
    outputLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, MSMatrixProcessor::OUTPUT_LEVEL_ID, outputLevelSlider);
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);
    xyPadLabel.setText("Mid Level / Side Level", juce::dontSendNotification);
    xyPadLabel.setJustificationType(juce::Justification::centred);
    xyPadLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    
    // Add M/S meter
    addAndMakeVisible(msMeter);
    msMeterLabel.setText("M/S Field", juce::dontSendNotification);
    msMeterLabel.setJustificationType(juce::Justification::centred);
    msMeterLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(msMeterLabel);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    midLevelSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    sideLevelSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    stereoBalanceSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    outputLevelSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    setSize(650, 600);
}

MSMatrixEditor::~MSMatrixEditor()
{
    setLookAndFeel(nullptr);
}

void MSMatrixEditor::paint(juce::Graphics& g)
{
    // Dark background matching AutoPan
    g.fillAll(HyperPrismLookAndFeel::Colors::background);
}

void MSMatrixEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(40));
    
    // Bypass button (top right)
    bypassButton.setBounds(bounds.getWidth() - 100, 10, 80, 30);
    
    bounds.reduce(20, 10);
    
    // Complex layout: 4 sliders + 2 toggles + 1 dropdown
    // Available height after title and margins: ~500px
    
    auto topRow = bounds.removeFromTop(140);
    auto sliderWidth = 80;
    auto spacing = 15;
    
    // Calculate total width needed for 4 sliders
    auto totalSliderWidth = sliderWidth * 4 + spacing * 3;
    auto startX = (bounds.getWidth() - totalSliderWidth) / 2;
    topRow.removeFromLeft(startX);
    
    // Top row: Mid Level, Side Level, Stereo Balance, Output Level
    midLevelSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    midLevelLabel.setBounds(midLevelSlider.getX(), midLevelSlider.getBottom(), sliderWidth, 20);
    topRow.removeFromLeft(spacing);
    
    sideLevelSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    sideLevelLabel.setBounds(sideLevelSlider.getX(), sideLevelSlider.getBottom(), sliderWidth, 20);
    topRow.removeFromLeft(spacing);
    
    stereoBalanceSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    stereoBalanceLabel.setBounds(stereoBalanceSlider.getX(), stereoBalanceSlider.getBottom(), sliderWidth, 20);
    topRow.removeFromLeft(spacing);
    
    outputLevelSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    outputLevelLabel.setBounds(outputLevelSlider.getX(), outputLevelSlider.getBottom(), sliderWidth, 20);
    
    bounds.removeFromTop(10);
    
    // Second row - Matrix mode dropdown and solo buttons
    auto secondRow = bounds.removeFromTop(80);
    
    // Matrix mode in center
    auto dropdownWidth = 150;
    auto dropdownX = bounds.getX() + (bounds.getWidth() - dropdownWidth) / 2;
    matrixModeComboBox.setBounds(dropdownX, secondRow.getY() + 30, dropdownWidth, 25);
    matrixModeLabel.setBounds(dropdownX, secondRow.getY() + 5, dropdownWidth, 20);
    
    // Solo buttons on either side
    auto buttonWidth = 80;
    midSoloButton.setBounds(dropdownX - buttonWidth - 20, secondRow.getY() + 30, buttonWidth, 25);
    midSoloLabel.setBounds(dropdownX - buttonWidth - 20, secondRow.getY() + 5, buttonWidth, 20);
    
    sideSoloButton.setBounds(dropdownX + dropdownWidth + 20, secondRow.getY() + 30, buttonWidth, 25);
    sideSoloLabel.setBounds(dropdownX + dropdownWidth + 20, secondRow.getY() + 5, buttonWidth, 20);
    
    // Bottom section - XY Pad and Meter side by side (matching heights)
    bounds.removeFromTop(10);
    
    // Split remaining space horizontally for XY pad and meter
    auto bottomArea = bounds;
    auto panelHeight = 180; // Standard XY pad height
    
    // Calculate positioning to center both panels
    auto xyPadWidth = 200;
    auto meterWidth = 150;
    auto totalWidth = xyPadWidth + meterWidth + 20; // Plus spacing
    auto startXBottom = (bottomArea.getWidth() - totalWidth) / 2;
    
    // XY Pad on left (200x180 standard)
    auto xyPadBounds = bottomArea.withX(bottomArea.getX() + startXBottom).withWidth(xyPadWidth).withHeight(panelHeight);
    xyPad.setBounds(xyPadBounds);
    
    // M/S Meter on right (matching height)
    auto meterBounds = bottomArea.withX(xyPadBounds.getRight() + 20).withWidth(meterWidth).withHeight(panelHeight);
    msMeter.setBounds(meterBounds);
    
    // Align labels at the same Y position
    auto labelY = xyPadBounds.getBottom() + 5;
    xyPadLabel.setBounds(xyPadBounds.getX(), labelY, xyPadWidth, 20);
    msMeterLabel.setBounds(meterBounds.getX(), labelY, meterWidth, 20);
}

void MSMatrixEditor::setupControls()
{
    // This method is no longer used - moved into constructor
}

void MSMatrixEditor::setupXYPad()
{
    // This method is no longer used - moved into constructor
}

void MSMatrixEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void MSMatrixEditor::updateParameterColors()
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
    
    updateLabelColor(midLevelLabel, MSMatrixProcessor::MID_LEVEL_ID);
    updateLabelColor(sideLevelLabel, MSMatrixProcessor::SIDE_LEVEL_ID);
    updateLabelColor(stereoBalanceLabel, MSMatrixProcessor::STEREO_BALANCE_ID);
    updateLabelColor(outputLevelLabel, MSMatrixProcessor::OUTPUT_LEVEL_ID);
}

void MSMatrixEditor::updateXYPadFromParameters()
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

void MSMatrixEditor::updateParametersFromXYPad(float x, float y)
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

void MSMatrixEditor::showParameterMenu(juce::Label* label, const juce::String& parameterID)
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
                    xParameterIDs.add(MSMatrixProcessor::MID_LEVEL_ID);
                    
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
                    yParameterIDs.add(MSMatrixProcessor::SIDE_LEVEL_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add(MSMatrixProcessor::MID_LEVEL_ID);
                yParameterIDs.add(MSMatrixProcessor::SIDE_LEVEL_ID);
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void MSMatrixEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == MSMatrixProcessor::MID_LEVEL_ID) return "Mid Level";
        if (paramID == MSMatrixProcessor::SIDE_LEVEL_ID) return "Side Level";
        if (paramID == MSMatrixProcessor::STEREO_BALANCE_ID) return "Stereo Balance";
        if (paramID == MSMatrixProcessor::OUTPUT_LEVEL_ID) return "Output Level";
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