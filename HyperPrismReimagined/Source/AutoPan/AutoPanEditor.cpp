//==============================================================================
// HyperPrism Reimagined - Auto Pan Editor
// Updated to match Compressor UI template
//==============================================================================

#include "AutoPanEditor.h"

//==============================================================================
// XYPad Implementation (matching Compressor style)
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
// PanPositionMeter Implementation (horizontal meter like gain reduction meter)
//==============================================================================
PanPositionMeter::PanPositionMeter(AutoPanProcessor& p) : processor(p)
{
    startTimerHz(30);
}

PanPositionMeter::~PanPositionMeter()
{
    stopTimer();
}

void PanPositionMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(HyperPrismLookAndFeel::Colors::background);
    g.fillRoundedRectangle(bounds, 3.0f);
    
    // Meter background
    g.setColour(HyperPrismLookAndFeel::Colors::surfaceVariant);
    g.fillRoundedRectangle(bounds.reduced(2), 2.0f);
    
    // Center line
    float centerX = bounds.getCentreX();
    g.setColour(HyperPrismLookAndFeel::Colors::onSurface.withAlpha(0.5f));
    g.drawLine(centerX, bounds.getY() + 2, centerX, bounds.getBottom() - 2, 2.0f);
    
    // Draw pan position
    float panX = bounds.getCentreX() + (currentPanPosition * (bounds.getWidth() * 0.4f));
    
    // Position indicator
    g.setColour(HyperPrismLookAndFeel::Colors::primary);
    g.fillEllipse(panX - 8, bounds.getCentreY() - 8, 16, 16);
    
    // LFO wave visualization (subtle)
    g.setColour(HyperPrismLookAndFeel::Colors::primary.withAlpha(0.3f));
    juce::Path wavePath;
    for (int i = 0; i < static_cast<int>(bounds.getWidth()); ++i)
    {
        float x = bounds.getX() + i;
        float phase = static_cast<float>(i) / bounds.getWidth() * juce::MathConstants<float>::twoPi;
        float waveValue = getWaveformValue(phase + lfoPhase, currentWaveform);
        float y = bounds.getCentreY() + waveValue * 10.0f;
        
        if (i == 0)
            wavePath.startNewSubPath(x, y);
        else
            wavePath.lineTo(x, y);
    }
    g.strokePath(wavePath, juce::PathStrokeType(1.0f));
    
    // Draw scale marks
    g.setColour(HyperPrismLookAndFeel::Colors::onSurface.withAlpha(0.5f));
    g.setFont(10.0f);
    g.drawText("L", static_cast<int>(bounds.getX() + 5), static_cast<int>(bounds.getBottom() - 20), 20, 15, juce::Justification::left);
    g.drawText("C", static_cast<int>(centerX - 10), static_cast<int>(bounds.getBottom() - 20), 20, 15, juce::Justification::centred);
    g.drawText("R", static_cast<int>(bounds.getRight() - 25), static_cast<int>(bounds.getBottom() - 20), 20, 15, juce::Justification::right);
    
    // Border
    g.setColour(HyperPrismLookAndFeel::Colors::outline);
    g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
}

void PanPositionMeter::timerCallback()
{
    float newPanPosition = processor.getPanPosition();
    lfoPhase = processor.getLFOPhase();
    
    // Get current waveform
    if (auto* waveformParam = processor.getValueTreeState().getRawParameterValue(AutoPanProcessor::WAVEFORM_ID))
        currentWaveform = static_cast<int>(*waveformParam);
    
    // Smooth the meter movement
    const float smoothing = 0.8f;
    currentPanPosition = currentPanPosition + (newPanPosition - currentPanPosition) * (1.0f - smoothing);
    
    repaint();
}

float PanPositionMeter::getWaveformValue(float phase, int waveformType)
{
    switch (waveformType)
    {
        case 0: // Sine
            return std::sin(phase);
            
        case 1: // Triangle
        {
            float value = (phase / juce::MathConstants<float>::pi) - 1.0f;
            return 1.0f - 2.0f * std::abs(value - 2.0f * std::floor(value * 0.5f + 0.5f));
        }
            
        case 2: // Square
            return (phase < juce::MathConstants<float>::pi) ? 1.0f : -1.0f;
            
        case 3: // Sawtooth
            return 1.0f - 2.0f * (phase / (2.0f * juce::MathConstants<float>::pi));
            
        case 4: // Random
            // Create a pseudo-random pattern based on phase for visualization
            // Use multiple sine waves to create a chaotic pattern
            return 0.3f * std::sin(phase * 7.0f) + 
                   0.2f * std::sin(phase * 13.0f) + 
                   0.2f * std::sin(phase * 23.0f) +
                   0.3f * std::sin(phase * 31.0f);
            
        default:
            return std::sin(phase);
    }
}

//==============================================================================
// AutoPanEditor Implementation
//==============================================================================
AutoPanEditor::AutoPanEditor(AutoPanProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), panPositionMeter(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add(AutoPanProcessor::RATE_ID);
    yParameterIDs.add(AutoPanProcessor::DEPTH_ID);
    
    // Title
    titleLabel.setText("AUTO PAN", juce::dontSendNotification);
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
    
    // Setup sliders with consistent style
    setupSlider(rateSlider, rateLabel, "Rate");
    rateSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::modulation);
    setupSlider(depthSlider, depthLabel, "Depth");
    depthSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::modulation);
    setupSlider(phaseSlider, phaseLabel, "Phase");
    phaseSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::modulation);
    setupSlider(outputLevelSlider, outputLevelLabel, "Output");
    outputLevelSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::output);
    
    // Set up right-click handlers for parameter assignment
    rateLabel.onClick = [this]() { showParameterMenu(&rateLabel, AutoPanProcessor::RATE_ID); };
    depthLabel.onClick = [this]() { showParameterMenu(&depthLabel, AutoPanProcessor::DEPTH_ID); };
    phaseLabel.onClick = [this]() { showParameterMenu(&phaseLabel, AutoPanProcessor::PHASE_ID); };
    outputLevelLabel.onClick = [this]() { showParameterMenu(&outputLevelLabel, AutoPanProcessor::OUTPUT_LEVEL_ID); };

    // Register right-click on sliders for XY pad assignment
    rateSlider.addMouseListener(this, true);
    rateSlider.getProperties().set("xyParamID", AutoPanProcessor::RATE_ID);
    depthSlider.addMouseListener(this, true);
    depthSlider.getProperties().set("xyParamID", AutoPanProcessor::DEPTH_ID);
    phaseSlider.addMouseListener(this, true);
    phaseSlider.getProperties().set("xyParamID", AutoPanProcessor::PHASE_ID);
    outputLevelSlider.addMouseListener(this, true);
    outputLevelSlider.getProperties().set("xyParamID", AutoPanProcessor::OUTPUT_LEVEL_ID);

    
    // Waveform selector
    waveformComboBox.addItem("Sine", 1);
    waveformComboBox.addItem("Triangle", 2);
    waveformComboBox.addItem("Square", 3);
    waveformComboBox.addItem("Sawtooth", 4);
    waveformComboBox.addItem("Random", 5);
    waveformComboBox.setColour(juce::ComboBox::backgroundColourId, HyperPrismLookAndFeel::Colors::surfaceVariant);
    waveformComboBox.setColour(juce::ComboBox::textColourId, HyperPrismLookAndFeel::Colors::onSurface);
    waveformComboBox.setColour(juce::ComboBox::outlineColourId, HyperPrismLookAndFeel::Colors::outline);
    waveformComboBox.setColour(juce::ComboBox::arrowColourId, HyperPrismLookAndFeel::Colors::primary);
    addAndMakeVisible(waveformComboBox);
    
    waveformLabel.setText("Waveform", juce::dontSendNotification);
    waveformLabel.setJustificationType(juce::Justification::centred);
    waveformLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    addAndMakeVisible(waveformLabel);
    
    // Sync button
    syncButton.setButtonText("SYNC");
    syncButton.setColour(juce::ToggleButton::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    syncButton.setColour(juce::ToggleButton::tickColourId, HyperPrismLookAndFeel::Colors::primary);
    syncButton.setColour(juce::ToggleButton::tickDisabledColourId, HyperPrismLookAndFeel::Colors::surfaceVariant);
    addAndMakeVisible(syncButton);
    
    // Bypass button (top right)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setClickingTogglesState(true);
    addAndMakeVisible(bypassButton);
    
    // Create attachments
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getValueTreeState(), AutoPanProcessor::BYPASS_ID, bypassButton);
    rateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), AutoPanProcessor::RATE_ID, rateSlider);
    depthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), AutoPanProcessor::DEPTH_ID, depthSlider);
    waveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        audioProcessor.getValueTreeState(), AutoPanProcessor::WAVEFORM_ID, waveformComboBox);
    phaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), AutoPanProcessor::PHASE_ID, phaseSlider);
    syncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getValueTreeState(), AutoPanProcessor::SYNC_ID, syncButton);
    outputLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), AutoPanProcessor::OUTPUT_LEVEL_ID, outputLevelSlider);
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);  // Set the axis colors
    xyPadLabel.setText("Rate / Depth", juce::dontSendNotification);
    xyPadLabel.setJustificationType(juce::Justification::centred);
    xyPadLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    
    // Setup pan position meter
    addAndMakeVisible(panPositionMeter);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();  // Show initial color coding
    
    // Listen for parameter changes - update XY pad when any parameter changes
    rateSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    depthSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    phaseSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    outputLevelSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    // Tooltips
    rateSlider.setTooltip("Speed of the automatic panning movement");
    depthSlider.setTooltip("How far the sound pans left and right");
    phaseSlider.setTooltip("Phase offset between left and right channels");
    outputLevelSlider.setTooltip("Overall output volume");
    waveformComboBox.setTooltip("Shape of the panning modulation wave");
    bypassButton.setTooltip("Bypass the effect, passing audio through unchanged");
    xyPad.setTooltip("Click and drag to control two parameters at once");

    setSize(700, 550);
    setResizable(true, true);
    setResizeLimits(600, 520, 900, 750);
}

AutoPanEditor::~AutoPanEditor()
{
    setLookAndFeel(nullptr);
}

void AutoPanEditor::paint(juce::Graphics& g)
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

    paintColumnHeader(rateSlider.getX() - 2, rateSlider.getY() - 20, 120,
                      "MODULATION", HyperPrismLookAndFeel::Colors::modulation);
    paintColumnHeader(waveformComboBox.getX() - 2, waveformComboBox.getY() - 20, 120,
                      "WAVEFORM", HyperPrismLookAndFeel::Colors::modulation);

    // Output section header
    paintColumnHeader(outputSectionX, outputSectionY,
                      getWidth() - outputSectionX - 12,
                      "OUTPUT", HyperPrismLookAndFeel::Colors::output);
}

void AutoPanEditor::resized()
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

    // Column 1: MODULATION -- Rate, Depth, Phase
    int y1 = colTop + knobDiam / 2;
    centerKnob(rateSlider, rateLabel, col1.getX(), colWidth, y1, knobDiam);
    centerKnob(depthSlider, depthLabel, col1.getX(), colWidth, y1 + vSpace, knobDiam);
    centerKnob(phaseSlider, phaseLabel, col1.getX(), colWidth, y1 + vSpace * 2, knobDiam);

    // Column 2: WAVEFORM -- combo + sync toggle
    int col2Top = col2.getY() + 20;
    waveformComboBox.setBounds(col2.getX() + 5, col2Top + 4, colWidth - 10, 24);
    waveformLabel.setBounds(col2.getX(), col2Top + 30, colWidth, 14);
    syncButton.setBounds(col2.getX() + 5, col2Top + 56, colWidth - 10, 22);

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
    panPositionMeter.setBounds(meterArea.reduced(4));
}

void AutoPanEditor::setupSlider(juce::Slider& slider, juce::Label& label, 
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

void AutoPanEditor::updateParameterColors()
{
    auto neutralColor = HyperPrismLookAndFeel::Colors::onSurfaceVariant;
    rateLabel.setColour(juce::Label::textColourId, neutralColor);
    depthLabel.setColour(juce::Label::textColourId, neutralColor);
    phaseLabel.setColour(juce::Label::textColourId, neutralColor);
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

    updateSliderXY(rateSlider, AutoPanProcessor::RATE_ID);
    updateSliderXY(depthSlider, AutoPanProcessor::DEPTH_ID);
    updateSliderXY(phaseSlider, AutoPanProcessor::PHASE_ID);
    updateSliderXY(outputLevelSlider, AutoPanProcessor::OUTPUT_LEVEL_ID);
    repaint();
}

void AutoPanEditor::updateXYPadFromParameters()
{
    // For multiple parameters, use the average of their normalized values
    float xValue = 0.0f;
    float yValue = 0.0f;
    
    // Calculate average X value
    if (!xParameterIDs.isEmpty())
    {
        for (const auto& paramID : xParameterIDs)
        {
            if (auto* param = audioProcessor.getValueTreeState().getRawParameterValue(paramID))
            {
                if (auto* paramObj = audioProcessor.getValueTreeState().getParameter(paramID))
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
            if (auto* param = audioProcessor.getValueTreeState().getRawParameterValue(paramID))
            {
                if (auto* paramObj = audioProcessor.getValueTreeState().getParameter(paramID))
                {
                    yValue += paramObj->convertTo0to1(*param);
                }
            }
        }
        yValue /= yParameterIDs.size();
    }
    
    xyPad.setValues(xValue, yValue);
}

void AutoPanEditor::updateParametersFromXYPad(float x, float y)
{
    // Update all assigned X parameters
    for (const auto& paramID : xParameterIDs)
    {
        if (auto* param = audioProcessor.getValueTreeState().getParameter(paramID))
            param->setValueNotifyingHost(x);
    }
    
    // Update all assigned Y parameters
    for (const auto& paramID : yParameterIDs)
    {
        if (auto* param = audioProcessor.getValueTreeState().getParameter(paramID))
            param->setValueNotifyingHost(y);
    }
}


void AutoPanEditor::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        auto* source = event.eventComponent;
        auto paramID = source->getProperties()["xyParamID"].toString();
        if (paramID.isNotEmpty())
            showParameterMenu(source, paramID);
    }
}
void AutoPanEditor::showParameterMenu(juce::Component* target, const juce::String& parameterID)
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
                    xParameterIDs.add(AutoPanProcessor::RATE_ID);
                    
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
                    yParameterIDs.add(AutoPanProcessor::DEPTH_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add(AutoPanProcessor::RATE_ID);
                yParameterIDs.add(AutoPanProcessor::DEPTH_ID);
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void AutoPanEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == AutoPanProcessor::RATE_ID) return "Rate";
        if (paramID == AutoPanProcessor::DEPTH_ID) return "Depth";
        if (paramID == AutoPanProcessor::PHASE_ID) return "Phase";
        if (paramID == AutoPanProcessor::OUTPUT_LEVEL_ID) return "Output";
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

void AutoPanEditor::assignParameterToXYPad(const juce::String& parameterID, bool assignToX)
{
    // This method is no longer used - replaced by toggle functionality in showParameterMenu
}