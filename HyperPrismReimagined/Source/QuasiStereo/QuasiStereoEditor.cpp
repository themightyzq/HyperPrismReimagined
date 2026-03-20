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
    
    // Title
    titleLabel.setText("QUASI STEREO", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(16.0f).withStyle("Bold")));
    titleLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurface);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    brandLabel.setText("HyperPrism Reimagined", juce::dontSendNotification);
    brandLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    brandLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    brandLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(brandLabel);
    
    // Setup sliders with consistent style (6 parameters)
    setupSlider(widthSlider, widthLabel, "Width");
    setupSlider(delayTimeSlider, delayTimeLabel, "Delay Time");
    setupSlider(frequencyShiftSlider, frequencyShiftLabel, "Freq Shift");
    setupSlider(phaseShiftSlider, phaseShiftLabel, "Phase Shift");
    setupSlider(highFreqEnhanceSlider, highFreqEnhanceLabel, "HF Enhance");
    setupSlider(outputLevelSlider, outputLevelLabel, "Output");

    // Color-code knobs by category
    widthSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::modulation);
    delayTimeSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::timing);
    frequencyShiftSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::modulation);
    phaseShiftSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::timing);
    highFreqEnhanceSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::modulation);
    outputLevelSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::output);
    
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

    // Register right-click on sliders for XY pad assignment
    widthSlider.addMouseListener(this, true);
    widthSlider.getProperties().set("xyParamID", QuasiStereoProcessor::WIDTH_ID);
    delayTimeSlider.addMouseListener(this, true);
    delayTimeSlider.getProperties().set("xyParamID", QuasiStereoProcessor::DELAY_TIME_ID);
    frequencyShiftSlider.addMouseListener(this, true);
    frequencyShiftSlider.getProperties().set("xyParamID", QuasiStereoProcessor::FREQUENCY_SHIFT_ID);
    phaseShiftSlider.addMouseListener(this, true);
    phaseShiftSlider.getProperties().set("xyParamID", QuasiStereoProcessor::PHASE_SHIFT_ID);
    highFreqEnhanceSlider.addMouseListener(this, true);
    highFreqEnhanceSlider.getProperties().set("xyParamID", QuasiStereoProcessor::HIGH_FREQ_ENHANCE_ID);
    outputLevelSlider.addMouseListener(this, true);
    outputLevelSlider.getProperties().set("xyParamID", QuasiStereoProcessor::OUTPUT_LEVEL_ID);

    
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
    xyPadLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    xyPad.setTooltip("Click and drag to control assigned parameters. Right-click parameter labels to assign X/Y axes.");

    // Add stereo width meter
    addAndMakeVisible(stereoWidthMeter);
    
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
    
    // Tooltips
    widthSlider.setTooltip("Amount of stereo widening applied");
    phaseShiftSlider.setTooltip("Phase difference between channels to create stereo image");
    delayTimeSlider.setTooltip("Small delay between channels for Haas-effect stereo");
    frequencyShiftSlider.setTooltip("Frequency shift between channels");
    highFreqEnhanceSlider.setTooltip("Boost high frequency content for added brightness and air");
    outputLevelSlider.setTooltip("Overall output volume");
    bypassButton.setTooltip("Bypass the effect");

    setSize(700, 550);
    setResizable(true, true);
    setResizeLimits(600, 520, 900, 750);
}

QuasiStereoEditor::~QuasiStereoEditor()
{
    setLookAndFeel(nullptr);
}

void QuasiStereoEditor::paint(juce::Graphics& g)
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

    paintColumnHeader(widthSlider.getX() - 2, widthSlider.getY() - 20, 120,
                      "STEREO", HyperPrismLookAndFeel::Colors::modulation);
    paintColumnHeader(delayTimeSlider.getX() - 2, delayTimeSlider.getY() - 20, 120,
                      "TIMING", HyperPrismLookAndFeel::Colors::timing);

    paintColumnHeader(outputSectionX, outputSectionY,
                      getWidth() - outputSectionX - 12,
                      "OUTPUT", HyperPrismLookAndFeel::Colors::output);
}

void QuasiStereoEditor::resized()
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

    // Column 1: STEREO -- Width, Frequency Shift, HF Enhance
    int y1 = colTop + knobDiam / 2;
    centerKnob(widthSlider, widthLabel, col1.getX(), colWidth, y1, knobDiam);
    centerKnob(frequencyShiftSlider, frequencyShiftLabel, col1.getX(), colWidth, y1 + vSpace, knobDiam);
    centerKnob(highFreqEnhanceSlider, highFreqEnhanceLabel, col1.getX(), colWidth, y1 + vSpace * 2, knobDiam);

    // Column 2: TIMING -- Delay Time, Phase Shift
    centerKnob(delayTimeSlider, delayTimeLabel, col2.getX(), colWidth, y1, knobDiam);
    centerKnob(phaseShiftSlider, phaseShiftLabel, col2.getX(), colWidth, y1 + vSpace, knobDiam);

    // --- Right side: XY pad + output + meter ---
    auto rightSide = bounds;
    rightSide.removeFromLeft(12);

    int outputHeight = 130;
    int xyHeight = juce::jmax(200, rightSide.getHeight() - outputHeight - 22);
    auto xyArea = rightSide.removeFromTop(xyHeight);
    xyPad.setBounds(xyArea);
    xyPadLabel.setBounds(xyArea.getX(), xyArea.getBottom() + 2, xyArea.getWidth(), 16);
    rightSide.removeFromTop(20);

    // Bottom right: Output knob + Stereo Width Meter
    auto bottomRight = rightSide;
    auto outputArea = bottomRight.removeFromLeft(140);
    auto meterArea = bottomRight.reduced(4);

    int outKnob = 58;
    int outY = outputArea.getY() + 24;
    centerKnob(outputLevelSlider, outputLevelLabel, outputArea.getX() + 20, 100, outY + outKnob / 2, outKnob);

    outputSectionX = outputArea.getX();
    outputSectionY = outputArea.getY();

    // Meter
    stereoWidthMeter.setBounds(meterArea.reduced(4));
}

void QuasiStereoEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void QuasiStereoEditor::updateParameterColors()
{
    auto neutralColor = HyperPrismLookAndFeel::Colors::onSurfaceVariant;
    widthLabel.setColour(juce::Label::textColourId, neutralColor);
    delayTimeLabel.setColour(juce::Label::textColourId, neutralColor);
    frequencyShiftLabel.setColour(juce::Label::textColourId, neutralColor);
    phaseShiftLabel.setColour(juce::Label::textColourId, neutralColor);
    highFreqEnhanceLabel.setColour(juce::Label::textColourId, neutralColor);
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

    updateSliderXY(widthSlider, QuasiStereoProcessor::WIDTH_ID);
    updateSliderXY(delayTimeSlider, QuasiStereoProcessor::DELAY_TIME_ID);
    updateSliderXY(frequencyShiftSlider, QuasiStereoProcessor::FREQUENCY_SHIFT_ID);
    updateSliderXY(phaseShiftSlider, QuasiStereoProcessor::PHASE_SHIFT_ID);
    updateSliderXY(highFreqEnhanceSlider, QuasiStereoProcessor::HIGH_FREQ_ENHANCE_ID);
    updateSliderXY(outputLevelSlider, QuasiStereoProcessor::OUTPUT_LEVEL_ID);
    repaint();
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


void QuasiStereoEditor::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        auto* source = event.eventComponent;
        auto paramID = source->getProperties()["xyParamID"].toString();
        if (paramID.isNotEmpty())
            showParameterMenu(source, paramID);
    }
}
void QuasiStereoEditor::showParameterMenu(juce::Component* target, const juce::String& parameterID)
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
        if (paramID == QuasiStereoProcessor::OUTPUT_LEVEL_ID) return "Output";
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