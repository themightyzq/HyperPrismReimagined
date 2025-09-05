//==============================================================================
// HyperPrism Reimagined - Bass Maximiser Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "BassMaximiserEditor.h"

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
// BassMaximiserEditor Implementation
//==============================================================================
BassMaximiserEditor::BassMaximiserEditor(BassMaximiserProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add(FREQUENCY_ID);
    yParameterIDs.add(BOOST_ID);
    
    // Title (matching AutoPan style)
    titleLabel.setText("HyperPrism Reimagined Bass Maximizer", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions("Arial", "Bold", 24.0f)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Setup sliders with consistent style (6 parameters - 4+2 layout)
    setupSlider(frequencySlider, frequencyLabel, "Frequency", " Hz");
    setupSlider(boostSlider, boostLabel, "Boost", " dB");
    setupSlider(harmonicsSlider, harmonicsLabel, "Harmonics", "%");
    setupSlider(tightnessSlider, tightnessLabel, "Tightness", "%");
    setupSlider(outputGainSlider, outputGainLabel, "Output Gain", " dB");
    
    // Set up right-click handlers for parameter assignment
    frequencyLabel.onClick = [this]() { showParameterMenu(&frequencyLabel, FREQUENCY_ID); };
    boostLabel.onClick = [this]() { showParameterMenu(&boostLabel, BOOST_ID); };
    harmonicsLabel.onClick = [this]() { showParameterMenu(&harmonicsLabel, HARMONICS_ID); };
    tightnessLabel.onClick = [this]() { showParameterMenu(&tightnessLabel, TIGHTNESS_ID); };
    outputGainLabel.onClick = [this]() { showParameterMenu(&outputGainLabel, OUTPUT_GAIN_ID); };
    
    // Phase Invert toggle
    phaseInvertButton.setButtonText("Phase Invert");
    phaseInvertButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    phaseInvertButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::cyan);
    addAndMakeVisible(phaseInvertButton);
    
    // Bypass button (top right like AutoPan)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::red);
    bypassButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
    addAndMakeVisible(bypassButton);
    
    // Create attachments
    auto& apvts = audioProcessor.getValueTreeState();
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, BYPASS_ID, bypassButton);
    frequencyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, FREQUENCY_ID, frequencySlider);
    boostAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, BOOST_ID, boostSlider);
    harmonicsAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, HARMONICS_ID, harmonicsSlider);
    tightnessAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, TIGHTNESS_ID, tightnessSlider);
    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, OUTPUT_GAIN_ID, outputGainSlider);
    phaseInvertAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, PHASE_INVERT_ID, phaseInvertButton);
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);
    xyPadLabel.setText("Frequency / Boost", juce::dontSendNotification);
    xyPadLabel.setJustificationType(juce::Justification::centred);
    xyPadLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    frequencySlider.onValueChange = [this] { updateXYPadFromParameters(); };
    boostSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    harmonicsSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    tightnessSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    outputGainSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    setSize(650, 600);
}

BassMaximiserEditor::~BassMaximiserEditor()
{
    setLookAndFeel(nullptr);
}

void BassMaximiserEditor::paint(juce::Graphics& g)
{
    // Dark background matching AutoPan
    g.fillAll(HyperPrismLookAndFeel::Colors::background);
}

void BassMaximiserEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(40));
    
    // Bypass button (top right)
    bypassButton.setBounds(bounds.getWidth() - 100, 10, 80, 30);
    
    bounds.reduce(20, 10);
    
    // Single row layout: All 5 knobs on one row
    // Available height after title and margins: ~550px
    // Distribution: 140px + 50px + 180px = 370px
    
    auto topRow = bounds.removeFromTop(140);
    auto sliderWidth = 80;
    auto spacing = 15;
    
    // Calculate total width needed for 5 sliders
    auto totalSliderWidth = sliderWidth * 5 + spacing * 4;
    auto startX = (bounds.getWidth() - totalSliderWidth) / 2;
    topRow.removeFromLeft(startX);
    
    // Single row: Frequency, Boost, Harmonics, Tightness, Output Gain
    frequencySlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    frequencyLabel.setBounds(frequencySlider.getX(), frequencySlider.getBottom(), sliderWidth, 20);
    topRow.removeFromLeft(spacing);
    
    boostSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    boostLabel.setBounds(boostSlider.getX(), boostSlider.getBottom(), sliderWidth, 20);
    topRow.removeFromLeft(spacing);
    
    harmonicsSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    harmonicsLabel.setBounds(harmonicsSlider.getX(), harmonicsSlider.getBottom(), sliderWidth, 20);
    topRow.removeFromLeft(spacing);
    
    tightnessSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    tightnessLabel.setBounds(tightnessSlider.getX(), tightnessSlider.getBottom(), sliderWidth, 20);
    topRow.removeFromLeft(spacing);
    
    outputGainSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    outputGainLabel.setBounds(outputGainSlider.getX(), outputGainSlider.getBottom(), sliderWidth, 20);
    
    bounds.removeFromTop(10);
    
    // Phase Invert toggle row (under Frequency knob, keep where it is)
    auto toggleRow = bounds.removeFromTop(50);
    
    // Align Phase Invert under Frequency knob
    auto phaseInvertX = startX; // Same X position as frequency knob
    phaseInvertButton.setBounds(phaseInvertX, toggleRow.getY() + 10, sliderWidth, 30);
    
    // Bottom section - XY Pad (brought up with less spacing)
    bounds.removeFromTop(20); // Reduced spacing to bring XY pad up
    
    // Center the XY Pad horizontally (200x180 standard)
    auto xyPadWidth = 200;
    auto xyPadHeight = 180;
    auto xyPadX = bounds.getX() + (bounds.getWidth() - xyPadWidth) / 2;
    auto xyPadY = bounds.getY();
    
    xyPad.setBounds(xyPadX, xyPadY, xyPadWidth, xyPadHeight);
    xyPadLabel.setBounds(xyPadX, xyPadY + xyPadHeight + 5, xyPadWidth, 20);
}

void BassMaximiserEditor::setupControls()
{
    // This method is no longer used - moved into constructor
}

void BassMaximiserEditor::setupXYPad()
{
    // This method is no longer used - moved into constructor
}

void BassMaximiserEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void BassMaximiserEditor::updateParameterColors()
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
    
    updateLabelColor(frequencyLabel, FREQUENCY_ID);
    updateLabelColor(boostLabel, BOOST_ID);
    updateLabelColor(harmonicsLabel, HARMONICS_ID);
    updateLabelColor(tightnessLabel, TIGHTNESS_ID);
    updateLabelColor(outputGainLabel, OUTPUT_GAIN_ID);
}

void BassMaximiserEditor::updateXYPadFromParameters()
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

void BassMaximiserEditor::updateParametersFromXYPad(float x, float y)
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

void BassMaximiserEditor::showParameterMenu(juce::Label* label, const juce::String& parameterID)
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
                    xParameterIDs.add(FREQUENCY_ID);
                    
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
                    yParameterIDs.add(BOOST_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add(FREQUENCY_ID);
                yParameterIDs.add(BOOST_ID);
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void BassMaximiserEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == FREQUENCY_ID) return "Frequency";
        if (paramID == BOOST_ID) return "Boost";
        if (paramID == HARMONICS_ID) return "Harmonics";
        if (paramID == TIGHTNESS_ID) return "Tightness";
        if (paramID == OUTPUT_GAIN_ID) return "Output Gain";
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