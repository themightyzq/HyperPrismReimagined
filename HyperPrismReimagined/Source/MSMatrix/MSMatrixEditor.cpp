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
    
    g.setColour(HyperPrismLookAndFeel::Colors::onSurface);
    g.fillEllipse(xPos - 4, yPos - 4, 8, 8);
    
    // Labels
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(10.0f);
    g.drawText("M", centerX - 5, centerY - radius - 25, 10, 15, juce::Justification::centred);
    g.drawText("S", centerX + radius + 5, centerY - 7, 15, 15, juce::Justification::left);
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
    
    // Title
    titleLabel.setText("M/S MATRIX", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(16.0f).withStyle("Bold")));
    titleLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurface);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    brandLabel.setText("HyperPrism Reimagined", juce::dontSendNotification);
    brandLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    brandLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    brandLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(brandLabel);
    
    // Setup sliders with consistent style (4 sliders)
    setupSlider(midLevelSlider, midLevelLabel, "Mid Level");
    setupSlider(sideLevelSlider, sideLevelLabel, "Side Level");
    setupSlider(stereoBalanceSlider, stereoBalanceLabel, "Stereo Balance");
    setupSlider(outputLevelSlider, outputLevelLabel, "Output");

    // Color-code knobs by category
    midLevelSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::dynamics);
    sideLevelSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::dynamics);
    stereoBalanceSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::dynamics);
    outputLevelSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::output);
    
    // Set up right-click handlers for parameter assignment
    midLevelLabel.onClick = [this]() { showParameterMenu(&midLevelLabel, MSMatrixProcessor::MID_LEVEL_ID); };
    sideLevelLabel.onClick = [this]() { showParameterMenu(&sideLevelLabel, MSMatrixProcessor::SIDE_LEVEL_ID); };
    stereoBalanceLabel.onClick = [this]() { showParameterMenu(&stereoBalanceLabel, MSMatrixProcessor::STEREO_BALANCE_ID); };
    outputLevelLabel.onClick = [this]() { showParameterMenu(&outputLevelLabel, MSMatrixProcessor::OUTPUT_LEVEL_ID); };

    // Register right-click on sliders for XY pad assignment
    midLevelSlider.addMouseListener(this, true);
    midLevelSlider.getProperties().set("xyParamID", MSMatrixProcessor::MID_LEVEL_ID);
    sideLevelSlider.addMouseListener(this, true);
    sideLevelSlider.getProperties().set("xyParamID", MSMatrixProcessor::SIDE_LEVEL_ID);
    stereoBalanceSlider.addMouseListener(this, true);
    stereoBalanceSlider.getProperties().set("xyParamID", MSMatrixProcessor::STEREO_BALANCE_ID);
    outputLevelSlider.addMouseListener(this, true);
    outputLevelSlider.getProperties().set("xyParamID", MSMatrixProcessor::OUTPUT_LEVEL_ID);

    
    // Matrix mode selector
    matrixModeComboBox.addItem("L/R to M/S", 1);
    matrixModeComboBox.addItem("M/S to L/R", 2);
    matrixModeComboBox.addItem("M/S Through", 3);
    matrixModeComboBox.setColour(juce::ComboBox::backgroundColourId, HyperPrismLookAndFeel::Colors::surfaceVariant);
    matrixModeComboBox.setColour(juce::ComboBox::textColourId, HyperPrismLookAndFeel::Colors::onSurface);
    matrixModeComboBox.setColour(juce::ComboBox::arrowColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    matrixModeComboBox.setColour(juce::ComboBox::outlineColourId, HyperPrismLookAndFeel::Colors::outline);
    addAndMakeVisible(matrixModeComboBox);
    
    matrixModeLabel.setText("Matrix Mode", juce::dontSendNotification);
    matrixModeLabel.setJustificationType(juce::Justification::centred);
    matrixModeLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    addAndMakeVisible(matrixModeLabel);
    
    // Solo buttons
    midSoloButton.setButtonText("Mid Solo");
    midSoloButton.setColour(juce::ToggleButton::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    midSoloButton.setColour(juce::ToggleButton::tickColourId, HyperPrismLookAndFeel::Colors::primary);
    addAndMakeVisible(midSoloButton);
    
    sideSoloButton.setButtonText("Side Solo");
    sideSoloButton.setColour(juce::ToggleButton::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    sideSoloButton.setColour(juce::ToggleButton::tickColourId, HyperPrismLookAndFeel::Colors::primary);
    addAndMakeVisible(sideSoloButton);
    
    // Bypass button (top right like AutoPan)
    // Bypass button
    bypassButton.setButtonText("Bypass");
    bypassButton.setClickingTogglesState(true);
    bypassButton.setColour(juce::TextButton::buttonOnColourId,
                            HyperPrismLookAndFeel::Colors::error.withAlpha(0.6f));
    bypassButton.setColour(juce::TextButton::textColourOnId,
                            HyperPrismLookAndFeel::Colors::onSurface);
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
    xyPadLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    
    // Add M/S meter
    addAndMakeVisible(msMeter);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    midLevelSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    sideLevelSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    stereoBalanceSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    outputLevelSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    // Tooltips
    midLevelSlider.setTooltip("Volume of the mid (center) signal");
    sideLevelSlider.setTooltip("Volume of the side (stereo difference) signal");
    stereoBalanceSlider.setTooltip("Overall stereo width -- 0% is mono, 200% is extra wide");
    outputLevelSlider.setTooltip("Overall output volume after M/S processing");
    bypassButton.setTooltip("Bypass the effect");
    xyPad.setTooltip("Click and drag to control two parameters at once");

    setSize(700, 550);
    setResizable(true, true);
    setResizeLimits(600, 520, 900, 750);
}

MSMatrixEditor::~MSMatrixEditor()
{
    setLookAndFeel(nullptr);
}

void MSMatrixEditor::paint(juce::Graphics& g)
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

    paintColumnHeader(midLevelSlider.getX() - 2, midLevelSlider.getY() - 20, 120,
                      "MATRIX", HyperPrismLookAndFeel::Colors::dynamics);

    // Output section header
    paintColumnHeader(outputSectionX, outputSectionY,
                      getWidth() - outputSectionX - 12,
                      "OUTPUT", HyperPrismLookAndFeel::Colors::output);
}

void MSMatrixEditor::resized()
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

    // Matrix mode combo above columns
    auto modeRow = bounds.removeFromTop(30);
    matrixModeLabel.setBounds(modeRow.getX(), modeRow.getY(), 80, 25);
    matrixModeComboBox.setBounds(modeRow.getX() + 82, modeRow.getY(), 128, 25);
    bounds.removeFromTop(6);

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

    // Column 1: MATRIX -- Mid Level, Side Level, Stereo Balance
    int y1 = colTop + knobDiam / 2;
    centerKnob(midLevelSlider, midLevelLabel, col1.getX(), colWidth, y1, knobDiam);
    centerKnob(sideLevelSlider, sideLevelLabel, col1.getX(), colWidth, y1 + vSpace, knobDiam);
    centerKnob(stereoBalanceSlider, stereoBalanceLabel, col1.getX(), colWidth, y1 + vSpace * 2, knobDiam);

    // Column 2: Solo buttons
    midSoloButton.setBounds(col2.getX() + 5, y1 - 12, colWidth - 10, 22);
    sideSoloButton.setBounds(col2.getX() + 5, y1 + vSpace - 12, colWidth - 10, 22);

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
    auto meterArea = bottomRight;

    outputSectionX = outputArea.getX();
    outputSectionY = outputArea.getY();

    int outKnob = 58;
    int outY = outputArea.getY() + 24;

    centerKnob(outputLevelSlider, outputLevelLabel, outputArea.getX() + 20, 100, outY + outKnob / 2, outKnob);

    // Meter
    msMeter.setBounds(meterArea.reduced(4));
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

void MSMatrixEditor::updateParameterColors()
{
    auto neutralColor = HyperPrismLookAndFeel::Colors::onSurfaceVariant;
    midLevelLabel.setColour(juce::Label::textColourId, neutralColor);
    sideLevelLabel.setColour(juce::Label::textColourId, neutralColor);
    stereoBalanceLabel.setColour(juce::Label::textColourId, neutralColor);
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

    updateSliderXY(midLevelSlider, MSMatrixProcessor::MID_LEVEL_ID);
    updateSliderXY(sideLevelSlider, MSMatrixProcessor::SIDE_LEVEL_ID);
    updateSliderXY(stereoBalanceSlider, MSMatrixProcessor::STEREO_BALANCE_ID);
    updateSliderXY(outputLevelSlider, MSMatrixProcessor::OUTPUT_LEVEL_ID);
    repaint();
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


void MSMatrixEditor::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        auto* source = event.eventComponent;
        auto paramID = source->getProperties()["xyParamID"].toString();
        if (paramID.isNotEmpty())
            showParameterMenu(source, paramID);
    }
}
void MSMatrixEditor::showParameterMenu(juce::Component* target, const juce::String& parameterID)
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
        if (paramID == MSMatrixProcessor::OUTPUT_LEVEL_ID) return "Output";
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