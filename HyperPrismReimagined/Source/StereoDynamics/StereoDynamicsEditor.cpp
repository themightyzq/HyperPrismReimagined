//==============================================================================
// HyperPrism Reimagined - Stereo Dynamics Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "StereoDynamicsEditor.h"

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
// StereoDynamicsMeter Implementation
//==============================================================================
StereoDynamicsMeter::StereoDynamicsMeter(StereoDynamicsProcessor& p) 
    : processor(p)
{
    startTimerHz(30); // 30 FPS update rate
}

StereoDynamicsMeter::~StereoDynamicsMeter()
{
    stopTimer();
}

void StereoDynamicsMeter::paint(juce::Graphics& g)
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
    float meterWidth = meterArea.getWidth();
    float meterHeight = meterArea.getHeight() * 0.75f; // Leave space for labels
    
    // Divide into four sections
    float sectionWidth = meterWidth / 4.0f;
    
    // Left Output Level (green)
    auto leftMeterArea = juce::Rectangle<float>(meterArea.getX(), meterArea.getY(), sectionWidth - 2, meterHeight);
    if (leftLevel > 0.001f)
    {
        float levelHeight = leftMeterArea.getHeight() * leftLevel;
        auto levelRect = juce::Rectangle<float>(leftMeterArea.getX(), 
                                               leftMeterArea.getBottom() - levelHeight, 
                                               leftMeterArea.getWidth(), 
                                               levelHeight);
        g.setColour(HyperPrismLookAndFeel::Colors::success);
        g.fillRoundedRectangle(levelRect, 2.0f);
    }
    
    // Right Output Level (cyan)
    auto rightMeterArea = juce::Rectangle<float>(meterArea.getX() + sectionWidth, meterArea.getY(), sectionWidth - 2, meterHeight);
    if (rightLevel > 0.001f)
    {
        float levelHeight = rightMeterArea.getHeight() * rightLevel;
        auto levelRect = juce::Rectangle<float>(rightMeterArea.getX(), 
                                               rightMeterArea.getBottom() - levelHeight, 
                                               rightMeterArea.getWidth(), 
                                               levelHeight);
        g.setColour(HyperPrismLookAndFeel::Colors::primary);
        g.fillRoundedRectangle(levelRect, 2.0f);
    }
    
    // Mid Signal Level (yellow/orange)
    auto midMeterArea = juce::Rectangle<float>(meterArea.getX() + sectionWidth * 2, meterArea.getY(), sectionWidth - 2, meterHeight);
    if (midLevel > 0.001f)
    {
        float levelHeight = midMeterArea.getHeight() * midLevel;
        auto levelRect = juce::Rectangle<float>(midMeterArea.getX(), 
                                               midMeterArea.getBottom() - levelHeight, 
                                               midMeterArea.getWidth(), 
                                               levelHeight);
        g.setColour(HyperPrismLookAndFeel::Colors::warning);
        g.fillRoundedRectangle(levelRect, 2.0f);
    }
    
    // Side Signal Level (purple/magenta)
    auto sideMeterArea = juce::Rectangle<float>(meterArea.getX() + sectionWidth * 3, meterArea.getY(), sectionWidth - 2, meterHeight);
    if (sideLevel > 0.001f)
    {
        float levelHeight = sideMeterArea.getHeight() * sideLevel;
        auto levelRect = juce::Rectangle<float>(sideMeterArea.getX(), 
                                               sideMeterArea.getBottom() - levelHeight, 
                                               sideMeterArea.getWidth(), 
                                               levelHeight);
        g.setColour(juce::Colour(200, 100, 255)); // Purple for side signal
        g.fillRoundedRectangle(levelRect, 2.0f);
    }
    
    // Gain Reduction indicators (overlaid on mid/side meters)
    // Mid Gain Reduction (red overlay)
    if (midGainReduction > 0.1f)
    {
        float grHeight = midMeterArea.getHeight() * (midGainReduction / 20.0f); // Scale to 20dB max
        auto grRect = juce::Rectangle<float>(midMeterArea.getX() + 2, 
                                            midMeterArea.getY(), 
                                            midMeterArea.getWidth() - 4, 
                                            grHeight);
        g.setColour(HyperPrismLookAndFeel::Colors::error.withAlpha(0.7f));
        g.fillRoundedRectangle(grRect, 1.0f);
    }
    
    // Side Gain Reduction (red overlay)
    if (sideGainReduction > 0.1f)
    {
        float grHeight = sideMeterArea.getHeight() * (sideGainReduction / 20.0f); // Scale to 20dB max
        auto grRect = juce::Rectangle<float>(sideMeterArea.getX() + 2, 
                                            sideMeterArea.getY(), 
                                            sideMeterArea.getWidth() - 4, 
                                            grHeight);
        g.setColour(HyperPrismLookAndFeel::Colors::error.withAlpha(0.7f));
        g.fillRoundedRectangle(grRect, 1.0f);
    }
    
    // Draw scale lines
    g.setColour(HyperPrismLookAndFeel::Colors::outlineVariant.withAlpha(0.3f));
    for (int i = 1; i < 4; ++i)
    {
        float y = meterArea.getY() + (meterHeight * i / 4.0f);
        g.drawHorizontalLine(static_cast<int>(y), meterArea.getX(), meterArea.getRight());
    }
    
    // Draw section dividers
    for (int i = 1; i < 4; ++i)
    {
        float x = meterArea.getX() + (sectionWidth * i);
        g.drawVerticalLine(static_cast<int>(x), meterArea.getY(), meterArea.getY() + meterHeight);
    }
    
    // Labels
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(9.0f);
    
    auto labelY = meterArea.getY() + meterHeight + 5;
    auto labelArea = juce::Rectangle<float>(meterArea.getX(), labelY, sectionWidth, 15);
    g.drawText("L", labelArea, juce::Justification::centred);
    
    labelArea.translate(sectionWidth, 0);
    g.drawText("R", labelArea, juce::Justification::centred);
    
    labelArea.translate(sectionWidth, 0);
    g.drawText("MID", labelArea, juce::Justification::centred);
    
    labelArea.translate(sectionWidth, 0);
    g.drawText("SIDE", labelArea, juce::Justification::centred);
    
    // Gain reduction values for mid/side
    g.setFont(8.0f);
    auto grY = labelY + 15;
    
    // Mid gain reduction
    auto midGRArea = juce::Rectangle<float>(meterArea.getX() + sectionWidth * 2, grY, sectionWidth, 10);
    juce::String midGRText = midGainReduction > 0.1f ? "-" + juce::String(midGainReduction, 1) + "dB" : "0dB";
    g.drawText(midGRText, midGRArea, juce::Justification::centred);
    
    // Side gain reduction
    auto sideGRArea = juce::Rectangle<float>(meterArea.getX() + sectionWidth * 3, grY, sectionWidth, 10);
    juce::String sideGRText = sideGainReduction > 0.1f ? "-" + juce::String(sideGainReduction, 1) + "dB" : "0dB";
    g.drawText(sideGRText, sideGRArea, juce::Justification::centred);
    
    // Level percentages
    g.setFont(7.0f);
    auto percentY = grY + 15;
    auto percentArea = juce::Rectangle<float>(meterArea.getX(), percentY, sectionWidth, 10);
    
    g.drawText(juce::String(static_cast<int>(leftLevel * 100)) + "%", percentArea, juce::Justification::centred);
    
    percentArea.translate(sectionWidth, 0);
    g.drawText(juce::String(static_cast<int>(rightLevel * 100)) + "%", percentArea, juce::Justification::centred);
    
    percentArea.translate(sectionWidth, 0);
    g.drawText(juce::String(static_cast<int>(midLevel * 100)) + "%", percentArea, juce::Justification::centred);
    
    percentArea.translate(sectionWidth, 0);
    g.drawText(juce::String(static_cast<int>(sideLevel * 100)) + "%", percentArea, juce::Justification::centred);
}

void StereoDynamicsMeter::timerCallback()
{
    float newLeftLevel = processor.getLeftLevel();
    float newRightLevel = processor.getRightLevel();
    float newMidLevel = processor.getMidLevel();
    float newSideLevel = processor.getSideLevel();
    float newMidGainReduction = processor.getMidGainReduction();
    float newSideGainReduction = processor.getSideGainReduction();
    
    // Smooth the level changes
    const float smoothing = 0.7f;
    leftLevel = leftLevel * smoothing + newLeftLevel * (1.0f - smoothing);
    rightLevel = rightLevel * smoothing + newRightLevel * (1.0f - smoothing);
    midLevel = midLevel * smoothing + newMidLevel * (1.0f - smoothing);
    sideLevel = sideLevel * smoothing + newSideLevel * (1.0f - smoothing);
    
    // Gain reduction updates immediately for responsiveness
    midGainReduction = newMidGainReduction;
    sideGainReduction = newSideGainReduction;
    
    repaint();
}

//==============================================================================
// StereoDynamicsEditor Implementation
//==============================================================================
StereoDynamicsEditor::StereoDynamicsEditor(StereoDynamicsProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), stereoDynamicsMeter(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add(StereoDynamicsProcessor::MID_THRESHOLD_ID);
    yParameterIDs.add(StereoDynamicsProcessor::SIDE_THRESHOLD_ID);
    
    // Title (matching AutoPan style)
    titleLabel.setText("HyperPrism Reimagined Stereo Dynamics", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions("Arial", "Bold", 24.0f)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Setup sliders with consistent style
    setupSlider(midThresholdSlider, midThresholdLabel, "Mid Threshold", " dB");
    setupSlider(midRatioSlider, midRatioLabel, "Mid Ratio", ":1");
    setupSlider(sideThresholdSlider, sideThresholdLabel, "Side Threshold", " dB");
    setupSlider(sideRatioSlider, sideRatioLabel, "Side Ratio", ":1");
    setupSlider(attackTimeSlider, attackTimeLabel, "Attack Time", " ms");
    setupSlider(releaseTimeSlider, releaseTimeLabel, "Release Time", " ms");
    setupSlider(outputLevelSlider, outputLevelLabel, "Output Level", " dB");
    
    // Set parameter ranges (example ranges - adjust based on processor)
    midThresholdSlider.setRange(-60.0, 0.0, 0.1);
    midRatioSlider.setRange(1.0, 20.0, 0.1);
    sideThresholdSlider.setRange(-60.0, 0.0, 0.1);
    sideRatioSlider.setRange(1.0, 20.0, 0.1);
    attackTimeSlider.setRange(0.1, 100.0, 0.1);
    releaseTimeSlider.setRange(10.0, 1000.0, 1.0);
    outputLevelSlider.setRange(-24.0, 12.0, 0.1);
    
    // Set up right-click handlers for parameter assignment
    midThresholdLabel.onClick = [this]() { showParameterMenu(&midThresholdLabel, StereoDynamicsProcessor::MID_THRESHOLD_ID); };
    midRatioLabel.onClick = [this]() { showParameterMenu(&midRatioLabel, StereoDynamicsProcessor::MID_RATIO_ID); };
    sideThresholdLabel.onClick = [this]() { showParameterMenu(&sideThresholdLabel, StereoDynamicsProcessor::SIDE_THRESHOLD_ID); };
    sideRatioLabel.onClick = [this]() { showParameterMenu(&sideRatioLabel, StereoDynamicsProcessor::SIDE_RATIO_ID); };
    attackTimeLabel.onClick = [this]() { showParameterMenu(&attackTimeLabel, StereoDynamicsProcessor::ATTACK_TIME_ID); };
    releaseTimeLabel.onClick = [this]() { showParameterMenu(&releaseTimeLabel, StereoDynamicsProcessor::RELEASE_TIME_ID); };
    outputLevelLabel.onClick = [this]() { showParameterMenu(&outputLevelLabel, StereoDynamicsProcessor::OUTPUT_LEVEL_ID); };
    
    // Bypass button (top right like AutoPan)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::red);
    bypassButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
    addAndMakeVisible(bypassButton);
    
    // Create attachments
    auto& apvts = audioProcessor.getValueTreeState();
    midThresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, StereoDynamicsProcessor::MID_THRESHOLD_ID, midThresholdSlider);
    midRatioAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, StereoDynamicsProcessor::MID_RATIO_ID, midRatioSlider);
    sideThresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, StereoDynamicsProcessor::SIDE_THRESHOLD_ID, sideThresholdSlider);
    sideRatioAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, StereoDynamicsProcessor::SIDE_RATIO_ID, sideRatioSlider);
    attackTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, StereoDynamicsProcessor::ATTACK_TIME_ID, attackTimeSlider);
    releaseTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, StereoDynamicsProcessor::RELEASE_TIME_ID, releaseTimeSlider);
    outputLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, StereoDynamicsProcessor::OUTPUT_LEVEL_ID, outputLevelSlider);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, StereoDynamicsProcessor::BYPASS_ID, bypassButton);
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);
    xyPadLabel.setText("Mid Threshold / Side Threshold", juce::dontSendNotification);
    xyPadLabel.setJustificationType(juce::Justification::centred);
    xyPadLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    
    // Add stereo dynamics meter
    addAndMakeVisible(stereoDynamicsMeter);
    stereoDynamicsMeterLabel.setText("Stereo Dynamics Analysis", juce::dontSendNotification);
    stereoDynamicsMeterLabel.setJustificationType(juce::Justification::centred);
    stereoDynamicsMeterLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(stereoDynamicsMeterLabel);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    midThresholdSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    midRatioSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    sideThresholdSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    sideRatioSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    attackTimeSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    releaseTimeSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    outputLevelSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    setSize(650, 600);
}

StereoDynamicsEditor::~StereoDynamicsEditor()
{
    setLookAndFeel(nullptr);
}

void StereoDynamicsEditor::paint(juce::Graphics& g)
{
    // Dark background matching AutoPan
    g.fillAll(HyperPrismLookAndFeel::Colors::background);
}

void StereoDynamicsEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(40));
    
    // Bypass button (top right)
    bypassButton.setBounds(bounds.getWidth() - 100, 10, 80, 30);
    
    bounds.reduce(20, 10);
    
    // Optimized layout for 650x600 - single row with all 7 controls
    auto sliderWidth = 70;
    auto spacing = 10;
    
    // Single row with all 7 controls
    auto controlsRow = bounds.removeFromTop(130);
    auto totalControlsWidth = sliderWidth * 7 + spacing * 6;
    auto controlsStartX = (bounds.getWidth() - totalControlsWidth) / 2;
    controlsRow.removeFromLeft(controlsStartX);
    
    // All controls in one row: Mid Threshold, Mid Ratio, Side Threshold, Side Ratio, Attack Time, Release Time, Output Level
    midThresholdSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    midThresholdLabel.setBounds(midThresholdSlider.getX(), midThresholdSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    midRatioSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    midRatioLabel.setBounds(midRatioSlider.getX(), midRatioSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    sideThresholdSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    sideThresholdLabel.setBounds(sideThresholdSlider.getX(), sideThresholdSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    sideRatioSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    sideRatioLabel.setBounds(sideRatioSlider.getX(), sideRatioSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    attackTimeSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    attackTimeLabel.setBounds(attackTimeSlider.getX(), attackTimeSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    releaseTimeSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    releaseTimeLabel.setBounds(releaseTimeSlider.getX(), releaseTimeSlider.getBottom(), sliderWidth, 20);
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
    
    // Stereo dynamics meter on right (matching height)
    auto meterBounds = bottomArea.withX(xyPadBounds.getRight() + 20).withWidth(meterSize).withHeight(panelHeight);
    stereoDynamicsMeter.setBounds(meterBounds);
    
    // Align labels at the same Y position
    auto labelY = xyPadBounds.getBottom() + 5;
    xyPadLabel.setBounds(xyPad.getX(), labelY, xyPadWidth, 20);
    stereoDynamicsMeterLabel.setBounds(stereoDynamicsMeter.getX(), labelY, meterSize, 20);
}

void StereoDynamicsEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void StereoDynamicsEditor::updateParameterColors()
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
    
    updateLabelColor(midThresholdLabel, StereoDynamicsProcessor::MID_THRESHOLD_ID);
    updateLabelColor(midRatioLabel, StereoDynamicsProcessor::MID_RATIO_ID);
    updateLabelColor(sideThresholdLabel, StereoDynamicsProcessor::SIDE_THRESHOLD_ID);
    updateLabelColor(sideRatioLabel, StereoDynamicsProcessor::SIDE_RATIO_ID);
    updateLabelColor(attackTimeLabel, StereoDynamicsProcessor::ATTACK_TIME_ID);
    updateLabelColor(releaseTimeLabel, StereoDynamicsProcessor::RELEASE_TIME_ID);
    updateLabelColor(outputLevelLabel, StereoDynamicsProcessor::OUTPUT_LEVEL_ID);
}

void StereoDynamicsEditor::updateXYPadFromParameters()
{
    // For multiple parameters, use the average of their normalized values
    float xValue = 0.0f;
    float yValue = 0.0f;
    
    auto& apvts = audioProcessor.getValueTreeState();
    
    // Calculate average X value
    if (!xParameterIDs.isEmpty())
    {
        for (const auto& paramID : xParameterIDs)
        {
            if (auto* param = apvts.getParameter(paramID))
            {
                xValue += param->getValue();
            }
        }
        xValue /= xParameterIDs.size();
    }
    
    // Calculate average Y value
    if (!yParameterIDs.isEmpty())
    {
        for (const auto& paramID : yParameterIDs)
        {
            if (auto* param = apvts.getParameter(paramID))
            {
                yValue += param->getValue();
            }
        }
        yValue /= yParameterIDs.size();
    }
    
    xyPad.setValues(xValue, yValue);
}

void StereoDynamicsEditor::updateParametersFromXYPad(float x, float y)
{
    auto& apvts = audioProcessor.getValueTreeState();
    
    // Update all assigned X parameters
    for (const auto& paramID : xParameterIDs)
    {
        if (auto* param = apvts.getParameter(paramID))
            param->setValueNotifyingHost(x);
    }
    
    // Update all assigned Y parameters
    for (const auto& paramID : yParameterIDs)
    {
        if (auto* param = apvts.getParameter(paramID))
            param->setValueNotifyingHost(y);
    }
}

void StereoDynamicsEditor::showParameterMenu(juce::Label* label, const juce::String& parameterID)
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
                    xParameterIDs.add(StereoDynamicsProcessor::MID_THRESHOLD_ID);
                    
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
                    yParameterIDs.add(StereoDynamicsProcessor::SIDE_THRESHOLD_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add(StereoDynamicsProcessor::MID_THRESHOLD_ID);
                yParameterIDs.add(StereoDynamicsProcessor::SIDE_THRESHOLD_ID);
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void StereoDynamicsEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == StereoDynamicsProcessor::MID_THRESHOLD_ID) return "Mid Threshold";
        if (paramID == StereoDynamicsProcessor::MID_RATIO_ID) return "Mid Ratio";
        if (paramID == StereoDynamicsProcessor::SIDE_THRESHOLD_ID) return "Side Threshold";
        if (paramID == StereoDynamicsProcessor::SIDE_RATIO_ID) return "Side Ratio";
        if (paramID == StereoDynamicsProcessor::ATTACK_TIME_ID) return "Attack Time";
        if (paramID == StereoDynamicsProcessor::RELEASE_TIME_ID) return "Release Time";
        if (paramID == StereoDynamicsProcessor::OUTPUT_LEVEL_ID) return "Output Level";
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