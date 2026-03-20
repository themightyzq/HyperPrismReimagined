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
    float meterHeight = meterArea.getHeight() * 0.9f; // Leave space for labels
    
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
    
    // Title
    titleLabel.setText("STEREO DYNAMICS", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(16.0f).withStyle("Bold")));
    titleLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurface);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    brandLabel.setText("HyperPrism Reimagined", juce::dontSendNotification);
    brandLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    brandLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    brandLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(brandLabel);
    
    // Setup sliders with consistent style
    setupSlider(midThresholdSlider, midThresholdLabel, "Threshold");
    setupSlider(midRatioSlider, midRatioLabel, "Ratio");
    setupSlider(sideThresholdSlider, sideThresholdLabel, "Threshold");
    setupSlider(sideRatioSlider, sideRatioLabel, "Ratio");
    setupSlider(attackTimeSlider, attackTimeLabel, "Attack");
    setupSlider(releaseTimeSlider, releaseTimeLabel, "Release");
    setupSlider(outputLevelSlider, outputLevelLabel, "Output");

    // Color-code knobs by category
    midThresholdSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::dynamics);
    midRatioSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::dynamics);
    sideThresholdSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::dynamics);
    sideRatioSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::dynamics);
    attackTimeSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::timing);
    releaseTimeSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::timing);
    outputLevelSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::output);
    
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

    // Register right-click on sliders for XY pad assignment
    midThresholdSlider.addMouseListener(this, true);
    midThresholdSlider.getProperties().set("xyParamID", StereoDynamicsProcessor::MID_THRESHOLD_ID);
    midRatioSlider.addMouseListener(this, true);
    midRatioSlider.getProperties().set("xyParamID", StereoDynamicsProcessor::MID_RATIO_ID);
    sideThresholdSlider.addMouseListener(this, true);
    sideThresholdSlider.getProperties().set("xyParamID", StereoDynamicsProcessor::SIDE_THRESHOLD_ID);
    sideRatioSlider.addMouseListener(this, true);
    sideRatioSlider.getProperties().set("xyParamID", StereoDynamicsProcessor::SIDE_RATIO_ID);
    attackTimeSlider.addMouseListener(this, true);
    attackTimeSlider.getProperties().set("xyParamID", StereoDynamicsProcessor::ATTACK_TIME_ID);
    releaseTimeSlider.addMouseListener(this, true);
    releaseTimeSlider.getProperties().set("xyParamID", StereoDynamicsProcessor::RELEASE_TIME_ID);
    outputLevelSlider.addMouseListener(this, true);
    outputLevelSlider.getProperties().set("xyParamID", StereoDynamicsProcessor::OUTPUT_LEVEL_ID);

    
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
    xyPadLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    xyPad.setTooltip("Click and drag to control assigned parameters. Right-click parameter labels to assign X/Y axes.");

    // Add stereo dynamics meter
    addAndMakeVisible(stereoDynamicsMeter);
    
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
    
    // Tooltips
    midThresholdSlider.setTooltip("Compression threshold for the mid (center) channel");
    midRatioSlider.setTooltip("Compression ratio for the mid channel");
    sideThresholdSlider.setTooltip("Compression threshold for the side (stereo) channel");
    sideRatioSlider.setTooltip("Compression ratio for the side channel");
    attackTimeSlider.setTooltip("How quickly compression responds to loud signals");
    releaseTimeSlider.setTooltip("How quickly compression releases after signal drops");
    outputLevelSlider.setTooltip("Overall output volume");
    bypassButton.setTooltip("Bypass the effect");

    setSize(700, 550);
    setResizable(true, true);
    setResizeLimits(600, 520, 900, 750);
}

StereoDynamicsEditor::~StereoDynamicsEditor()
{
    setLookAndFeel(nullptr);
}

void StereoDynamicsEditor::paint(juce::Graphics& g)
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

    paintColumnHeader(midThresholdSlider.getX() - 2, midThresholdSlider.getY() - 20, 105,
                      "MID", HyperPrismLookAndFeel::Colors::dynamics);
    paintColumnHeader(sideThresholdSlider.getX() - 2, sideThresholdSlider.getY() - 20, 105,
                      "SIDE", HyperPrismLookAndFeel::Colors::dynamics);
    paintColumnHeader(attackTimeSlider.getX() - 2, attackTimeSlider.getY() - 20, 105,
                      "TIMING", HyperPrismLookAndFeel::Colors::timing);
    paintColumnHeader(outputSectionX, outputSectionY, 105,
                      "OUTPUT", HyperPrismLookAndFeel::Colors::output);
}

void StereoDynamicsEditor::resized()
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

    // --- Left: 3 columns with larger knobs, fixed 312px right-side allocation ---
    int rightSideWidth = 312;
    int columnsTotalWidth = bounds.getWidth() - rightSideWidth;
    auto columnsArea = bounds.removeFromLeft(columnsTotalWidth);
    int colWidth = (columnsArea.getWidth() - 20) / 3;
    auto col1 = columnsArea.removeFromLeft(colWidth);
    columnsArea.removeFromLeft(10);
    auto col2 = columnsArea.removeFromLeft(colWidth);
    columnsArea.removeFromLeft(10);
    auto col3 = columnsArea;

    int knobDiam = 70;
    int vSpace = 97;
    int colTop = col1.getY() + 20;

    auto centerKnob = [&](juce::Slider& slider, juce::Label& label,
                           int colX, int colW, int cy, int kd)
    {
        int kx = colX + (colW - kd) / 2;
        int ky = cy - kd / 2;
        slider.setBounds(kx, ky, kd, kd);
        label.setBounds(colX, ky + kd + 1, colW, 16);
    };

    // Column 1: MID -- Mid Threshold, Mid Ratio
    int y1 = colTop + knobDiam / 2;
    centerKnob(midThresholdSlider, midThresholdLabel, col1.getX(), colWidth, y1, knobDiam);
    centerKnob(midRatioSlider, midRatioLabel, col1.getX(), colWidth, y1 + vSpace, knobDiam);

    // Column 2: SIDE -- Side Threshold, Side Ratio
    centerKnob(sideThresholdSlider, sideThresholdLabel, col2.getX(), colWidth, y1, knobDiam);
    centerKnob(sideRatioSlider, sideRatioLabel, col2.getX(), colWidth, y1 + vSpace, knobDiam);

    // Column 3: TIMING -- Attack, Release
    centerKnob(attackTimeSlider, attackTimeLabel, col3.getX(), colWidth, y1, knobDiam);
    centerKnob(releaseTimeSlider, releaseTimeLabel, col3.getX(), colWidth, y1 + vSpace, knobDiam);

    // --- Right side: XY pad + meter + output ---
    auto rightSide = bounds;
    rightSide.removeFromLeft(12);

    // XY pad takes most of the right side
    int outputHeight = 130;
    int meterHeight = 80;
    int xyHeight = juce::jmax(200, rightSide.getHeight() - outputHeight - meterHeight - 30);
    auto xyArea = rightSide.removeFromTop(xyHeight);
    xyPad.setBounds(xyArea);
    xyPadLabel.setBounds(xyArea.getX(), xyArea.getBottom() + 2, xyArea.getWidth(), 16);
    rightSide.removeFromTop(20);

    // Meter below XY pad
    auto meterArea = rightSide.removeFromTop(meterHeight);
    stereoDynamicsMeter.setBounds(meterArea);
    rightSide.removeFromTop(20);

    // Output section: Output Level knob + meter side by side
    auto bottomRight = rightSide;
    auto outputArea = bottomRight.removeFromLeft(140);
    int outKnob = 58;
    int outY = outputArea.getY() + 24;
    centerKnob(outputLevelSlider, outputLevelLabel, outputArea.getX() + 20, 100, outY + outKnob / 2, outKnob);
    outputSectionX = outputArea.getX();
    outputSectionY = outputArea.getY();
}

void StereoDynamicsEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void StereoDynamicsEditor::updateParameterColors()
{
    auto neutralColor = HyperPrismLookAndFeel::Colors::onSurfaceVariant;
    midThresholdLabel.setColour(juce::Label::textColourId, neutralColor);
    midRatioLabel.setColour(juce::Label::textColourId, neutralColor);
    sideThresholdLabel.setColour(juce::Label::textColourId, neutralColor);
    sideRatioLabel.setColour(juce::Label::textColourId, neutralColor);
    attackTimeLabel.setColour(juce::Label::textColourId, neutralColor);
    releaseTimeLabel.setColour(juce::Label::textColourId, neutralColor);
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

    updateSliderXY(midThresholdSlider, StereoDynamicsProcessor::MID_THRESHOLD_ID);
    updateSliderXY(midRatioSlider, StereoDynamicsProcessor::MID_RATIO_ID);
    updateSliderXY(sideThresholdSlider, StereoDynamicsProcessor::SIDE_THRESHOLD_ID);
    updateSliderXY(sideRatioSlider, StereoDynamicsProcessor::SIDE_RATIO_ID);
    updateSliderXY(attackTimeSlider, StereoDynamicsProcessor::ATTACK_TIME_ID);
    updateSliderXY(releaseTimeSlider, StereoDynamicsProcessor::RELEASE_TIME_ID);
    updateSliderXY(outputLevelSlider, StereoDynamicsProcessor::OUTPUT_LEVEL_ID);
    repaint();
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


void StereoDynamicsEditor::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        auto* source = event.eventComponent;
        auto paramID = source->getProperties()["xyParamID"].toString();
        if (paramID.isNotEmpty())
            showParameterMenu(source, paramID);
    }
}
void StereoDynamicsEditor::showParameterMenu(juce::Component* target, const juce::String& parameterID)
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
        if (paramID == StereoDynamicsProcessor::OUTPUT_LEVEL_ID) return "Output";
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