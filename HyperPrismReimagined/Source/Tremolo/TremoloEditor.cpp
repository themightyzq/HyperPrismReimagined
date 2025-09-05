//==============================================================================
// HyperPrism Reimagined - Tremolo Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "TremoloEditor.h"

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
// TremoloMeter Implementation
//==============================================================================
TremoloMeter::TremoloMeter(TremoloProcessor& p) 
    : processor(p)
{
    startTimerHz(30); // 30 FPS update rate
    
    // Initialize LFO waveform array
    lfoWaveform.resize(256);
    
    // Initialize parameter values from processor
    auto& apvts = processor.getValueTreeState();
    
    if (auto* waveformParam = apvts.getRawParameterValue(TremoloProcessor::WAVEFORM_ID))
        waveformType = static_cast<int>(waveformParam->load());
    
    if (auto* rateParam = apvts.getRawParameterValue(TremoloProcessor::RATE_ID))
        rate = rateParam->load();
    
    if (auto* depthParam = apvts.getRawParameterValue(TremoloProcessor::DEPTH_ID))
        depth = depthParam->load() / 100.0f;
}

TremoloMeter::~TremoloMeter()
{
    stopTimer();
}

void TremoloMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(HyperPrismLookAndFeel::Colors::background);
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Border
    g.setColour(HyperPrismLookAndFeel::Colors::outline);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    
    // Create waveform display area
    auto displayArea = bounds.reduced(10.0f);
    float waveformHeight = displayArea.getHeight() * 0.8f;
    
    // Draw LFO waveform
    g.setColour(HyperPrismLookAndFeel::Colors::primary);
    juce::Path waveformPath;
    
    for (size_t i = 0; i < lfoWaveform.size(); ++i)
    {
        float x = displayArea.getX() + (i / float(lfoWaveform.size() - 1)) * displayArea.getWidth();
        float y = displayArea.getCentreY() - lfoWaveform[i] * waveformHeight * 0.4f * depth;
        
        if (i == 0)
            waveformPath.startNewSubPath(x, y);
        else
            waveformPath.lineTo(x, y);
    }
    
    g.strokePath(waveformPath, juce::PathStrokeType(2.0f));
    
    // Draw center line
    g.setColour(HyperPrismLookAndFeel::Colors::outlineVariant.withAlpha(0.5f));
    g.drawHorizontalLine(static_cast<int>(displayArea.getCentreY()), displayArea.getX(), displayArea.getRight());
    
    // Draw current phase indicator
    float phaseX = displayArea.getX() + currentPhase * displayArea.getWidth();
    g.setColour(HyperPrismLookAndFeel::Colors::warning);
    g.drawVerticalLine(static_cast<int>(phaseX), displayArea.getY(), displayArea.getBottom());
    
    // Draw phase circle
    float phaseY = displayArea.getCentreY() - lfoWaveform[static_cast<int>(currentPhase * (lfoWaveform.size() - 1))] * waveformHeight * 0.4f * depth;
    g.fillEllipse(phaseX - 4, phaseY - 4, 8, 8);
    
    // Labels
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(10.0f);
    
    // Waveform type label
    juce::String waveformName;
    switch (waveformType)
    {
        case 0: waveformName = "SINE"; break;
        case 1: waveformName = "TRIANGLE"; break;
        case 2: waveformName = "SQUARE"; break;
        default: waveformName = "UNKNOWN"; break;
    }
    
    auto labelArea = displayArea.removeFromTop(15);
    g.drawText("LFO: " + waveformName, labelArea, juce::Justification::centredLeft);
    
    // Rate and depth display
    auto infoArea = displayArea.removeFromBottom(30);
    auto rateArea = infoArea.removeFromLeft(infoArea.getWidth() / 2);
    
    g.setFont(9.0f);
    g.drawText("Rate: " + juce::String(rate, 1) + " Hz", rateArea, juce::Justification::centredLeft);
    g.drawText("Depth: " + juce::String(static_cast<int>(depth * 100)) + "%", infoArea, juce::Justification::centredLeft);
}

void TremoloMeter::timerCallback()
{
    // Get current parameter values from processor
    auto& apvts = processor.getValueTreeState();
    
    // Get waveform type (ComboBox uses 1-based indexing, we need 0-based)
    if (auto* waveformParam = apvts.getRawParameterValue(TremoloProcessor::WAVEFORM_ID))
        waveformType = static_cast<int>(waveformParam->load());
    
    // Get rate
    if (auto* rateParam = apvts.getRawParameterValue(TremoloProcessor::RATE_ID))
        rate = rateParam->load();
    
    // Get depth (0-100 range, normalize to 0-1)
    if (auto* depthParam = apvts.getRawParameterValue(TremoloProcessor::DEPTH_ID))
        depth = depthParam->load() / 100.0f;
    
    // Simulate phase advancement based on rate
    currentPhase += rate * 0.01f; // Adjust speed based on rate
    if (currentPhase >= 1.0f)
        currentPhase -= 1.0f;
    
    // Generate waveform based on type
    for (size_t i = 0; i < lfoWaveform.size(); ++i)
    {
        float phase = i / float(lfoWaveform.size());
        
        switch (waveformType)
        {
            case 0: // Sine
                lfoWaveform[i] = std::sin(2.0f * juce::MathConstants<float>::pi * phase);
                break;
                
            case 1: // Triangle
                if (phase < 0.5f)
                    lfoWaveform[i] = 4.0f * phase - 1.0f;
                else
                    lfoWaveform[i] = 3.0f - 4.0f * phase;
                break;
                
            case 2: // Square
                lfoWaveform[i] = phase < 0.5f ? 1.0f : -1.0f;
                break;
        }
    }
    
    repaint();
}

//==============================================================================
// TremoloEditor Implementation
//==============================================================================
TremoloEditor::TremoloEditor(TremoloProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), tremoloMeter(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add(TremoloProcessor::RATE_ID);
    yParameterIDs.add(TremoloProcessor::DEPTH_ID);
    
    // Title (matching AutoPan style)
    titleLabel.setText("HyperPrism Reimagined Tremolo", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions("Arial", "Bold", 24.0f)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Setup sliders with consistent style
    setupSlider(rateSlider, rateLabel, "Rate", " Hz");
    setupSlider(depthSlider, depthLabel, "Depth", "");
    setupSlider(stereoPhaseSlider, stereoPhaseLabel, "Stereo Phase", " deg");
    setupSlider(mixSlider, mixLabel, "Mix", "");
    
    // Set parameter ranges (example ranges - adjust based on processor)
    rateSlider.setRange(0.1, 20.0, 0.1);
    depthSlider.setRange(0.0, 100.0, 0.1);
    stereoPhaseSlider.setRange(0.0, 180.0, 1.0);
    mixSlider.setRange(0.0, 100.0, 0.1);
    
    // Setup ComboBox
    setupComboBox(waveformBox, waveformLabel, "Waveform");
    
    // Add waveform options
    waveformBox.addItem("Sine", 1);
    waveformBox.addItem("Triangle", 2);
    waveformBox.addItem("Square", 3);
    
    // Set up right-click handlers for parameter assignment
    rateLabel.onClick = [this]() { showParameterMenu(&rateLabel, TremoloProcessor::RATE_ID); };
    depthLabel.onClick = [this]() { showParameterMenu(&depthLabel, TremoloProcessor::DEPTH_ID); };
    stereoPhaseLabel.onClick = [this]() { showParameterMenu(&stereoPhaseLabel, TremoloProcessor::STEREO_PHASE_ID); };
    mixLabel.onClick = [this]() { showParameterMenu(&mixLabel, TremoloProcessor::MIX_ID); };
    waveformLabel.onClick = [this]() { showParameterMenu(&waveformLabel, TremoloProcessor::WAVEFORM_ID); };
    
    // Bypass button (top right like AutoPan)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::red);
    bypassButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
    addAndMakeVisible(bypassButton);
    
    // Create attachments
    auto& apvts = audioProcessor.getValueTreeState();
    rateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, TremoloProcessor::RATE_ID, rateSlider);
    depthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, TremoloProcessor::DEPTH_ID, depthSlider);
    stereoPhaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, TremoloProcessor::STEREO_PHASE_ID, stereoPhaseSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, TremoloProcessor::MIX_ID, mixSlider);
    waveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, TremoloProcessor::WAVEFORM_ID, waveformBox);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, TremoloProcessor::BYPASS_ID, bypassButton);
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);
    xyPadLabel.setText("Rate / Depth", juce::dontSendNotification);
    xyPadLabel.setJustificationType(juce::Justification::centred);
    xyPadLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    
    // Add tremolo meter
    addAndMakeVisible(tremoloMeter);
    tremoloMeterLabel.setText("LFO Waveform", juce::dontSendNotification);
    tremoloMeterLabel.setJustificationType(juce::Justification::centred);
    tremoloMeterLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(tremoloMeterLabel);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    rateSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    depthSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    stereoPhaseSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    mixSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    setSize(650, 600);
}

TremoloEditor::~TremoloEditor()
{
    setLookAndFeel(nullptr);
}

void TremoloEditor::paint(juce::Graphics& g)
{
    // Dark background matching AutoPan
    g.fillAll(HyperPrismLookAndFeel::Colors::background);
}

void TremoloEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(40));
    
    // Bypass button (top right)
    bypassButton.setBounds(bounds.getWidth() - 100, 10, 80, 30);
    
    bounds.reduce(20, 10);
    
    // Optimized layout for 650x600 - single row with all 5 controls (4 sliders + 1 combobox)
    auto sliderWidth = 75;
    auto comboWidth = 75;
    auto spacing = 12;
    
    // Single row with all 5 controls
    auto controlsRow = bounds.removeFromTop(130);
    auto totalControlsWidth = sliderWidth * 4 + comboWidth + spacing * 4;
    auto controlsStartX = (bounds.getWidth() - totalControlsWidth) / 2;
    controlsRow.removeFromLeft(controlsStartX);
    
    // All controls in one row: Rate, Depth, Stereo Phase, Mix, Waveform
    rateSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    rateLabel.setBounds(rateSlider.getX(), rateSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    depthSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    depthLabel.setBounds(depthSlider.getX(), depthSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    stereoPhaseSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    stereoPhaseLabel.setBounds(stereoPhaseSlider.getX(), stereoPhaseSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    mixSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    mixLabel.setBounds(mixSlider.getX(), mixSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    // Waveform ComboBox at the end of the row
    waveformBox.setBounds(controlsRow.removeFromLeft(comboWidth).withHeight(25).withY(controlsRow.getY() + 40));
    waveformLabel.setBounds(waveformBox.getX(), waveformBox.getBottom(), comboWidth, 20);
    
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
    
    // Tremolo meter on right (matching height)
    auto meterBounds = bottomArea.withX(xyPadBounds.getRight() + 20).withWidth(meterSize).withHeight(panelHeight);
    tremoloMeter.setBounds(meterBounds);
    
    // Align labels at the same Y position
    auto labelY = xyPadBounds.getBottom() + 5;
    xyPadLabel.setBounds(xyPad.getX(), labelY, xyPadWidth, 20);
    tremoloMeterLabel.setBounds(tremoloMeter.getX(), labelY, meterSize, 20);
}

void TremoloEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void TremoloEditor::setupComboBox(juce::ComboBox& comboBox, ParameterLabel& label, 
                                 const juce::String& text)
{
    comboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colours::darkgrey);
    comboBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    comboBox.setColour(juce::ComboBox::arrowColourId, juce::Colours::lightgrey);
    comboBox.setColour(juce::ComboBox::outlineColourId, juce::Colours::grey);
    addAndMakeVisible(comboBox);
    
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(label);
}

void TremoloEditor::updateParameterColors()
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
    
    updateLabelColor(rateLabel, TremoloProcessor::RATE_ID);
    updateLabelColor(depthLabel, TremoloProcessor::DEPTH_ID);
    updateLabelColor(stereoPhaseLabel, TremoloProcessor::STEREO_PHASE_ID);
    updateLabelColor(mixLabel, TremoloProcessor::MIX_ID);
    updateLabelColor(waveformLabel, TremoloProcessor::WAVEFORM_ID);
}

void TremoloEditor::updateXYPadFromParameters()
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

void TremoloEditor::updateParametersFromXYPad(float x, float y)
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

void TremoloEditor::showParameterMenu(juce::Label* label, const juce::String& parameterID)
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
                    xParameterIDs.add(TremoloProcessor::RATE_ID);
                    
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
                    yParameterIDs.add(TremoloProcessor::DEPTH_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add(TremoloProcessor::RATE_ID);
                yParameterIDs.add(TremoloProcessor::DEPTH_ID);
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void TremoloEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == TremoloProcessor::RATE_ID) return "Rate";
        if (paramID == TremoloProcessor::DEPTH_ID) return "Depth";
        if (paramID == TremoloProcessor::STEREO_PHASE_ID) return "Stereo Phase";
        if (paramID == TremoloProcessor::MIX_ID) return "Mix";
        if (paramID == TremoloProcessor::WAVEFORM_ID) return "Waveform";
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