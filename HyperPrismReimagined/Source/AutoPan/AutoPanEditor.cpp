//==============================================================================
// HyperPrism Revived - Auto Pan Editor
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
    
    // Title (matching Compressor style)
    titleLabel.setText("HyperPrism Reimagined Auto Pan", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions("Arial", "Bold", 24.0f)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Setup sliders with consistent style
    setupSlider(rateSlider, rateLabel, "Rate", " Hz");
    setupSlider(depthSlider, depthLabel, "Depth", "");
    setupSlider(phaseSlider, phaseLabel, "Phase", " deg");
    setupSlider(outputLevelSlider, outputLevelLabel, "Output", " dB");
    
    // Set up right-click handlers for parameter assignment
    rateLabel.onClick = [this]() { showParameterMenu(&rateLabel, AutoPanProcessor::RATE_ID); };
    depthLabel.onClick = [this]() { showParameterMenu(&depthLabel, AutoPanProcessor::DEPTH_ID); };
    phaseLabel.onClick = [this]() { showParameterMenu(&phaseLabel, AutoPanProcessor::PHASE_ID); };
    outputLevelLabel.onClick = [this]() { showParameterMenu(&outputLevelLabel, AutoPanProcessor::OUTPUT_LEVEL_ID); };
    
    // Waveform selector
    waveformComboBox.addItem("Sine", 1);
    waveformComboBox.addItem("Triangle", 2);
    waveformComboBox.addItem("Square", 3);
    waveformComboBox.addItem("Sawtooth", 4);
    waveformComboBox.addItem("Random", 5);
    waveformComboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colours::darkgrey);
    waveformComboBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    waveformComboBox.setColour(juce::ComboBox::outlineColourId, juce::Colours::grey);
    waveformComboBox.setColour(juce::ComboBox::arrowColourId, juce::Colours::cyan);
    addAndMakeVisible(waveformComboBox);
    
    waveformLabel.setText("Waveform", juce::dontSendNotification);
    waveformLabel.setJustificationType(juce::Justification::centred);
    waveformLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(waveformLabel);
    
    // Sync button
    syncButton.setButtonText("SYNC");
    syncButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    syncButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::cyan);
    syncButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
    addAndMakeVisible(syncButton);
    
    // Bypass button (top right like Compressor)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::red);
    bypassButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
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
    xyPadLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    
    // Setup pan position meter
    addAndMakeVisible(panPositionMeter);
    meterLabel.setText("Pan Position", juce::dontSendNotification);
    meterLabel.setJustificationType(juce::Justification::centred);
    meterLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(meterLabel);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();  // Show initial color coding
    
    // Listen for parameter changes - update XY pad when any parameter changes
    rateSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    depthSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    phaseSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    outputLevelSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    setSize(650, 600);
}

AutoPanEditor::~AutoPanEditor()
{
    setLookAndFeel(nullptr);
}

void AutoPanEditor::paint(juce::Graphics& g)
{
    // Dark background matching Compressor
    g.fillAll(HyperPrismLookAndFeel::Colors::background);
}

void AutoPanEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(40));
    
    // Bypass button (top right)
    bypassButton.setBounds(bounds.getWidth() - 100, 10, 80, 30);
    
    bounds.reduce(20, 10);
    
    // Top row - all knobs in a row
    auto topRow = bounds.removeFromTop(200);
    auto sliderWidth = 80;
    auto spacing = 15;
    
    // Calculate total width needed for sliders
    auto totalSliderWidth = sliderWidth * 4 + spacing * 3;
    auto startX = (bounds.getWidth() - totalSliderWidth) / 2;
    topRow.removeFromLeft(startX);
    
    rateSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    rateLabel.setBounds(rateSlider.getX(), rateSlider.getBottom(), sliderWidth, 20);
    topRow.removeFromLeft(spacing);
    
    depthSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    depthLabel.setBounds(depthSlider.getX(), depthSlider.getBottom(), sliderWidth, 20);
    topRow.removeFromLeft(spacing);
    
    phaseSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    phaseLabel.setBounds(phaseSlider.getX(), phaseSlider.getBottom(), sliderWidth, 20);
    topRow.removeFromLeft(spacing);
    
    outputLevelSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    outputLevelLabel.setBounds(outputLevelSlider.getX(), outputLevelSlider.getBottom(), sliderWidth, 20);
    
    // Middle section
    bounds.removeFromTop(20);
    auto middleSection = bounds.removeFromTop(200);
    
    // Center the XY Pad horizontally
    auto xyPadWidth = 200;
    auto xyPadX = (bounds.getWidth() - xyPadWidth) / 2;
    auto xyPadBounds = juce::Rectangle<int>(xyPadX, middleSection.getY(), xyPadWidth, 180);
    xyPad.setBounds(xyPadBounds);
    xyPadLabel.setBounds(xyPadBounds.getX(), xyPadBounds.getBottom(), xyPadWidth, 20);
    
    // Waveform and Sync to the right of XY Pad
    auto controlsX = xyPadBounds.getRight() + 30;
    auto controlsY = middleSection.getY() + 30;
    
    waveformLabel.setBounds(controlsX, controlsY, 120, 20);
    waveformComboBox.setBounds(controlsX, controlsY + 25, 120, 30);
    
    syncButton.setBounds(controlsX, controlsY + 70, 120, 30);
    
    // Bottom section - horizontal pan position meter
    bounds.removeFromTop(20);
    auto meterHeight = 60;
    auto meterBounds = bounds.removeFromTop(meterHeight);
    
    panPositionMeter.setBounds(meterBounds.reduced(10, 5));
    meterLabel.setBounds(meterBounds.getX(), meterBounds.getY() - 20, 100, 20);
}

void AutoPanEditor::setupSlider(juce::Slider& slider, juce::Label& label, 
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

void AutoPanEditor::updateParameterColors()
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
    
    updateLabelColor(rateLabel, AutoPanProcessor::RATE_ID);
    updateLabelColor(depthLabel, AutoPanProcessor::DEPTH_ID);
    updateLabelColor(phaseLabel, AutoPanProcessor::PHASE_ID);
    updateLabelColor(outputLevelLabel, AutoPanProcessor::OUTPUT_LEVEL_ID);
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

void AutoPanEditor::showParameterMenu(juce::Label* label, const juce::String& parameterID)
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