//==============================================================================
// HyperPrism Reimagined - Quasi Stereo Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "QuasiStereoEditor.h"

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
// StereoWidthMeter Implementation
//==============================================================================
StereoWidthMeter::StereoWidthMeter(QuasiStereoProcessor& p) 
    : processor(p)
{
    startTimerHz(30); // 30 FPS update rate
}

StereoWidthMeter::~StereoWidthMeter()
{
    stopTimer();
}

void StereoWidthMeter::paint(juce::Graphics& g)
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
    
    // Stereo width visualization (polar plot style)
    float centerX = meterArea.getCentreX();
    float centerY = meterArea.getCentreY();
    float radius = juce::jmin(meterArea.getWidth(), meterArea.getHeight()) * 0.35f;
    
    // Background circle
    g.setColour(HyperPrismLookAndFeel::Colors::surfaceVariant);
    g.fillEllipse(centerX - radius, centerY - radius, radius * 2, radius * 2);
    
    // Grid circles
    g.setColour(HyperPrismLookAndFeel::Colors::outlineVariant.withAlpha(0.3f));
    for (int i = 1; i < 4; ++i)
    {
        float r = radius * i / 4.0f;
        g.drawEllipse(centerX - r, centerY - r, r * 2, r * 2, 0.5f);
    }
    
    // Center line
    g.drawLine(centerX - radius, centerY, centerX + radius, centerY, 0.5f);
    g.drawLine(centerX, centerY - radius, centerX, centerY + radius, 0.5f);
    
    // L/R level indicators
    float leftAngle = -juce::MathConstants<float>::halfPi + (leftLevel * juce::MathConstants<float>::halfPi);
    float rightAngle = -juce::MathConstants<float>::halfPi - (rightLevel * juce::MathConstants<float>::halfPi);
    
    // Left channel
    if (leftLevel > 0.001f)
    {
        g.setColour(HyperPrismLookAndFeel::Colors::success);
        float leftX = centerX + (radius * 0.8f) * std::cos(leftAngle);
        float leftY = centerY + (radius * 0.8f) * std::sin(leftAngle);
        g.drawLine(centerX, centerY, leftX, leftY, 2.0f);
        g.fillEllipse(leftX - 3, leftY - 3, 6, 6);
    }
    
    // Right channel
    if (rightLevel > 0.001f)
    {
        g.setColour(HyperPrismLookAndFeel::Colors::primary);
        float rightX = centerX + (radius * 0.8f) * std::cos(rightAngle);
        float rightY = centerY + (radius * 0.8f) * std::sin(rightAngle);
        g.drawLine(centerX, centerY, rightX, rightY, 2.0f);
        g.fillEllipse(rightX - 3, rightY - 3, 6, 6);
    }
    
    // Stereo width indicator
    if (stereoWidth > 0.001f)
    {
        float widthRadius = radius * stereoWidth;
        g.setColour(HyperPrismLookAndFeel::Colors::primary.withAlpha(0.3f));
        g.fillEllipse(centerX - widthRadius, centerY - widthRadius, widthRadius * 2, widthRadius * 2);
    }
    
    // Labels
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(10.0f);
    g.drawText("L", centerX - radius - 15, centerY - 8, 15, 16, juce::Justification::centred);
    g.drawText("R", centerX + radius, centerY - 8, 15, 16, juce::Justification::centred);
    g.drawText("MONO", centerX - 15, centerY - radius - 20, 30, 16, juce::Justification::centred);
    
    // Width value
    g.setColour(HyperPrismLookAndFeel::Colors::primary);
    g.setFont(12.0f);
    juce::String widthText = "Width: " + juce::String(stereoWidth * 200.0f, 0) + "%";
    g.drawText(widthText, meterArea.removeFromBottom(20), juce::Justification::centred);
}

void StereoWidthMeter::timerCallback()
{
    float newLeftLevel = processor.getLeftLevel();
    float newRightLevel = processor.getRightLevel();
    float newStereoWidth = processor.getStereoWidth();
    
    // Smooth the changes
    const float smoothing = 0.7f;
    leftLevel = leftLevel * smoothing + newLeftLevel * (1.0f - smoothing);
    rightLevel = rightLevel * smoothing + newRightLevel * (1.0f - smoothing);
    stereoWidth = stereoWidth * smoothing + newStereoWidth * (1.0f - smoothing);
    
    repaint();
}

//==============================================================================
// QuasiStereoEditor Implementation
//==============================================================================
QuasiStereoEditor::QuasiStereoEditor(QuasiStereoProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), stereoWidthMeter(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add(QuasiStereoProcessor::WIDTH_ID);
    yParameterIDs.add(QuasiStereoProcessor::DELAY_TIME_ID);
    
    // Title (matching AutoPan style)
    titleLabel.setText("HyperPrism Reimagined Quasi Stereo", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions("Arial", "Bold", 24.0f)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Setup sliders with consistent style (6 parameters)
    setupSlider(widthSlider, widthLabel, "Width", "");
    setupSlider(delayTimeSlider, delayTimeLabel, "Delay Time", " ms");
    setupSlider(frequencyShiftSlider, frequencyShiftLabel, "Freq Shift", " Hz");
    setupSlider(phaseShiftSlider, phaseShiftLabel, "Phase Shift", " °");
    setupSlider(highFreqEnhanceSlider, highFreqEnhanceLabel, "HF Enhance", "");
    setupSlider(outputLevelSlider, outputLevelLabel, "Output Level", " dB");
    
    // Set parameter ranges
    widthSlider.setRange(0.0, 200.0, 0.1);
    delayTimeSlider.setRange(0.1, 100.0, 0.1);
    frequencyShiftSlider.setRange(-50.0, 50.0, 0.1);
    phaseShiftSlider.setRange(-180.0, 180.0, 1.0);
    highFreqEnhanceSlider.setRange(0.0, 100.0, 0.1);
    outputLevelSlider.setRange(-20.0, 20.0, 0.1);
    
    // Set up right-click handlers for parameter assignment
    widthLabel.onClick = [this]() { showParameterMenu(&widthLabel, QuasiStereoProcessor::WIDTH_ID); };
    delayTimeLabel.onClick = [this]() { showParameterMenu(&delayTimeLabel, QuasiStereoProcessor::DELAY_TIME_ID); };
    frequencyShiftLabel.onClick = [this]() { showParameterMenu(&frequencyShiftLabel, QuasiStereoProcessor::FREQUENCY_SHIFT_ID); };
    phaseShiftLabel.onClick = [this]() { showParameterMenu(&phaseShiftLabel, QuasiStereoProcessor::PHASE_SHIFT_ID); };
    highFreqEnhanceLabel.onClick = [this]() { showParameterMenu(&highFreqEnhanceLabel, QuasiStereoProcessor::HIGH_FREQ_ENHANCE_ID); };
    outputLevelLabel.onClick = [this]() { showParameterMenu(&outputLevelLabel, QuasiStereoProcessor::OUTPUT_LEVEL_ID); };
    
    // Bypass button (top right like AutoPan)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::red);
    bypassButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
    addAndMakeVisible(bypassButton);
    
    // Create attachments
    auto& vts = audioProcessor.getValueTreeState();
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        vts, QuasiStereoProcessor::BYPASS_ID, bypassButton);
    widthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, QuasiStereoProcessor::WIDTH_ID, widthSlider);
    delayTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, QuasiStereoProcessor::DELAY_TIME_ID, delayTimeSlider);
    frequencyShiftAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, QuasiStereoProcessor::FREQUENCY_SHIFT_ID, frequencyShiftSlider);
    phaseShiftAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, QuasiStereoProcessor::PHASE_SHIFT_ID, phaseShiftSlider);
    highFreqEnhanceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, QuasiStereoProcessor::HIGH_FREQ_ENHANCE_ID, highFreqEnhanceSlider);
    outputLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, QuasiStereoProcessor::OUTPUT_LEVEL_ID, outputLevelSlider);
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);
    xyPadLabel.setText("Width / Delay Time", juce::dontSendNotification);
    xyPadLabel.setJustificationType(juce::Justification::centred);
    xyPadLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    
    // Add stereo width meter
    addAndMakeVisible(stereoWidthMeter);
    stereoWidthMeterLabel.setText("Stereo Width", juce::dontSendNotification);
    stereoWidthMeterLabel.setJustificationType(juce::Justification::centred);
    stereoWidthMeterLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(stereoWidthMeterLabel);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    widthSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    delayTimeSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    frequencyShiftSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    phaseShiftSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    highFreqEnhanceSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    outputLevelSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    setSize(650, 600);
}

QuasiStereoEditor::~QuasiStereoEditor()
{
    setLookAndFeel(nullptr);
}

void QuasiStereoEditor::paint(juce::Graphics& g)
{
    // Dark background matching AutoPan
    g.fillAll(HyperPrismLookAndFeel::Colors::background);
}

void QuasiStereoEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(40));
    
    // Bypass button (top right)
    bypassButton.setBounds(bounds.getWidth() - 100, 10, 80, 30);
    
    bounds.reduce(20, 10);
    
    // Optimized layout for 650x600 - single row with all 6 controls
    auto sliderWidth = 75;
    auto spacing = 12;
    
    // Single row with all 6 controls
    auto controlsRow = bounds.removeFromTop(130);
    auto totalControlsWidth = sliderWidth * 6 + spacing * 5;
    auto controlsStartX = (bounds.getWidth() - totalControlsWidth) / 2;
    controlsRow.removeFromLeft(controlsStartX);
    
    // All controls in one row: Width, Delay Time, Freq Shift, Phase Shift, HF Enhance, Output Level
    widthSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    widthLabel.setBounds(widthSlider.getX(), widthSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    delayTimeSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    delayTimeLabel.setBounds(delayTimeSlider.getX(), delayTimeSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    frequencyShiftSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    frequencyShiftLabel.setBounds(frequencyShiftSlider.getX(), frequencyShiftSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    phaseShiftSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    phaseShiftLabel.setBounds(phaseShiftSlider.getX(), phaseShiftSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    highFreqEnhanceSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    highFreqEnhanceLabel.setBounds(highFreqEnhanceSlider.getX(), highFreqEnhanceSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    outputLevelSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    outputLevelLabel.setBounds(outputLevelSlider.getX(), outputLevelSlider.getBottom(), sliderWidth, 20);
    
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
    
    // Stereo width meter on right (matching height)
    auto meterBounds = bottomArea.withX(xyPadBounds.getRight() + 20).withWidth(meterSize).withHeight(panelHeight);
    stereoWidthMeter.setBounds(meterBounds);
    
    // Align labels at the same Y position
    auto labelY = xyPadBounds.getBottom() + 5;
    xyPadLabel.setBounds(xyPad.getX(), labelY, xyPadWidth, 20);
    stereoWidthMeterLabel.setBounds(stereoWidthMeter.getX(), labelY, meterSize, 20);
}

void QuasiStereoEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void QuasiStereoEditor::updateParameterColors()
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
    
    updateLabelColor(widthLabel, QuasiStereoProcessor::WIDTH_ID);
    updateLabelColor(delayTimeLabel, QuasiStereoProcessor::DELAY_TIME_ID);
    updateLabelColor(frequencyShiftLabel, QuasiStereoProcessor::FREQUENCY_SHIFT_ID);
    updateLabelColor(phaseShiftLabel, QuasiStereoProcessor::PHASE_SHIFT_ID);
    updateLabelColor(highFreqEnhanceLabel, QuasiStereoProcessor::HIGH_FREQ_ENHANCE_ID);
    updateLabelColor(outputLevelLabel, QuasiStereoProcessor::OUTPUT_LEVEL_ID);
}

void QuasiStereoEditor::updateXYPadFromParameters()
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

void QuasiStereoEditor::updateParametersFromXYPad(float x, float y)
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

void QuasiStereoEditor::showParameterMenu(juce::Label* label, const juce::String& parameterID)
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
                    xParameterIDs.add(QuasiStereoProcessor::WIDTH_ID);
                    
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
                    yParameterIDs.add(QuasiStereoProcessor::DELAY_TIME_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add(QuasiStereoProcessor::WIDTH_ID);
                yParameterIDs.add(QuasiStereoProcessor::DELAY_TIME_ID);
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void QuasiStereoEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == QuasiStereoProcessor::WIDTH_ID) return "Width";
        if (paramID == QuasiStereoProcessor::DELAY_TIME_ID) return "Delay Time";
        if (paramID == QuasiStereoProcessor::FREQUENCY_SHIFT_ID) return "Freq Shift";
        if (paramID == QuasiStereoProcessor::PHASE_SHIFT_ID) return "Phase Shift";
        if (paramID == QuasiStereoProcessor::HIGH_FREQ_ENHANCE_ID) return "HF Enhance";
        if (paramID == QuasiStereoProcessor::OUTPUT_LEVEL_ID) return "Output Level";
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