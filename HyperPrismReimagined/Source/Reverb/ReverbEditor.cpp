//==============================================================================
// HyperPrism Reimagined - Reverb Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "ReverbEditor.h"

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
// ReverbEditor Implementation
//==============================================================================
ReverbEditor::ReverbEditor(ReverbProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add(ReverbProcessor::ROOM_SIZE_ID);
    yParameterIDs.add(ReverbProcessor::DAMPING_ID);
    
    // Title (matching AutoPan style)
    titleLabel.setText("HyperPrism Reimagined Reverb", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions("Arial", "Bold", 24.0f)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Setup sliders with consistent style (7 parameters arranged as 4+3)
    setupSlider(roomSizeSlider, roomSizeLabel, "Room Size", "%");
    setupSlider(dampingSlider, dampingLabel, "Damping", "%");
    setupSlider(preDelaySlider, preDelayLabel, "Pre Delay", " ms");
    setupSlider(widthSlider, widthLabel, "Width", "%");
    setupSlider(lowCutSlider, lowCutLabel, "Low Cut", " Hz");
    setupSlider(highCutSlider, highCutLabel, "High Cut", " Hz");
    setupSlider(mixSlider, mixLabel, "Mix", "%");
    
    // Set up right-click handlers for parameter assignment
    roomSizeLabel.onClick = [this]() { showParameterMenu(&roomSizeLabel, ReverbProcessor::ROOM_SIZE_ID); };
    dampingLabel.onClick = [this]() { showParameterMenu(&dampingLabel, ReverbProcessor::DAMPING_ID); };
    preDelayLabel.onClick = [this]() { showParameterMenu(&preDelayLabel, ReverbProcessor::PRE_DELAY_ID); };
    widthLabel.onClick = [this]() { showParameterMenu(&widthLabel, ReverbProcessor::WIDTH_ID); };
    lowCutLabel.onClick = [this]() { showParameterMenu(&lowCutLabel, ReverbProcessor::LOW_CUT_ID); };
    highCutLabel.onClick = [this]() { showParameterMenu(&highCutLabel, ReverbProcessor::HIGH_CUT_ID); };
    mixLabel.onClick = [this]() { showParameterMenu(&mixLabel, ReverbProcessor::MIX_ID); };
    
    // Bypass button (top right like AutoPan)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::red);
    bypassButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
    addAndMakeVisible(bypassButton);
    
    // Create attachments
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getValueTreeState(), ReverbProcessor::BYPASS_ID, bypassButton);
    roomSizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), ReverbProcessor::ROOM_SIZE_ID, roomSizeSlider);
    dampingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), ReverbProcessor::DAMPING_ID, dampingSlider);
    preDelayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), ReverbProcessor::PRE_DELAY_ID, preDelaySlider);
    widthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), ReverbProcessor::WIDTH_ID, widthSlider);
    lowCutAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), ReverbProcessor::LOW_CUT_ID, lowCutSlider);
    highCutAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), ReverbProcessor::HIGH_CUT_ID, highCutSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), ReverbProcessor::MIX_ID, mixSlider);
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);
    xyPadLabel.setText("Room Size / Damping", juce::dontSendNotification);
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
    roomSizeSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    dampingSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    preDelaySlider.onValueChange = [this] { updateXYPadFromParameters(); };
    widthSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    lowCutSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    highCutSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    mixSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    setSize(650, 600);
}

ReverbEditor::~ReverbEditor()
{
    setLookAndFeel(nullptr);
}

void ReverbEditor::paint(juce::Graphics& g)
{
    // Dark background matching AutoPan
    g.fillAll(HyperPrismLookAndFeel::Colors::background);
}

void ReverbEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(40));
    
    // Bypass button (top right)
    bypassButton.setBounds(bounds.getWidth() - 100, 10, 80, 30);
    
    bounds.reduce(20, 10);
    
    // Optimized layout for 7 parameters (4+3 rows) to fit in 650x550 window
    // Available height after title and margins: ~500px
    // Distribution: 140px + 10px + 140px + 10px + 200px = 500px
    
    // Top row - first 4 knobs (Room Size, Damping, Pre Delay, Width)
    auto topRow = bounds.removeFromTop(140);
    auto sliderWidth = 80;
    auto spacing = 15;
    
    // Calculate total width needed for 4 sliders
    auto totalSliderWidth = sliderWidth * 4 + spacing * 3;
    auto startX = (bounds.getWidth() - totalSliderWidth) / 2;
    topRow.removeFromLeft(startX);
    
    // First row: Room Size, Damping, Pre Delay, Width
    roomSizeSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 10));
    roomSizeLabel.setBounds(roomSizeSlider.getX(), roomSizeSlider.getBottom(), sliderWidth, 20);
    topRow.removeFromLeft(spacing);
    
    dampingSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 10));
    dampingLabel.setBounds(dampingSlider.getX(), dampingSlider.getBottom(), sliderWidth, 20);
    topRow.removeFromLeft(spacing);
    
    preDelaySlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 10));
    preDelayLabel.setBounds(preDelaySlider.getX(), preDelaySlider.getBottom(), sliderWidth, 20);
    topRow.removeFromLeft(spacing);
    
    widthSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 10));
    widthLabel.setBounds(widthSlider.getX(), widthSlider.getBottom(), sliderWidth, 20);
    
    // Minimal spacing between rows
    bounds.removeFromTop(10);
    
    // Second row - remaining 3 knobs (Low Cut, High Cut, Mix) - centered
    auto secondRow = bounds.removeFromTop(140);
    
    // Calculate width for 3 sliders and center them
    auto secondRowWidth = sliderWidth * 3 + spacing * 2;
    auto secondRowStartX = (bounds.getWidth() - secondRowWidth) / 2;
    secondRow.removeFromLeft(secondRowStartX);
    
    lowCutSlider.setBounds(secondRow.removeFromLeft(sliderWidth).reduced(0, 10));
    lowCutLabel.setBounds(lowCutSlider.getX(), lowCutSlider.getBottom(), sliderWidth, 20);
    secondRow.removeFromLeft(spacing);
    
    highCutSlider.setBounds(secondRow.removeFromLeft(sliderWidth).reduced(0, 10));
    highCutLabel.setBounds(highCutSlider.getX(), highCutSlider.getBottom(), sliderWidth, 20);
    secondRow.removeFromLeft(spacing);
    
    mixSlider.setBounds(secondRow.removeFromLeft(sliderWidth).reduced(0, 10));
    mixLabel.setBounds(mixSlider.getX(), mixSlider.getBottom(), sliderWidth, 20);
    
    // Bottom section - XY Pad (use remaining bounds exactly)
    bounds.removeFromTop(10);
    
    // Center the XY Pad horizontally, use available height efficiently
    auto xyPadWidth = 200;
    auto availableHeight = bounds.getHeight() - 25; // Leave 25px for label + padding
    auto xyPadHeight = juce::jmin(180, availableHeight); // Cap at 180px or available space
    auto xyPadX = bounds.getX() + (bounds.getWidth() - xyPadWidth) / 2;
    auto xyPadY = bounds.getY() + 5;  // Small top padding
    
    xyPad.setBounds(xyPadX, xyPadY, xyPadWidth, xyPadHeight);
    xyPadLabel.setBounds(xyPadX, xyPadY + xyPadHeight + 5, xyPadWidth, 20);
}

void ReverbEditor::setupControls()
{
    // This method is no longer used - moved into constructor
}

void ReverbEditor::setupXYPad()
{
    // This method is no longer used - moved into constructor
}

void ReverbEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void ReverbEditor::updateParameterColors()
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
    
    updateLabelColor(roomSizeLabel, ReverbProcessor::ROOM_SIZE_ID);
    updateLabelColor(dampingLabel, ReverbProcessor::DAMPING_ID);
    updateLabelColor(preDelayLabel, ReverbProcessor::PRE_DELAY_ID);
    updateLabelColor(widthLabel, ReverbProcessor::WIDTH_ID);
    updateLabelColor(lowCutLabel, ReverbProcessor::LOW_CUT_ID);
    updateLabelColor(highCutLabel, ReverbProcessor::HIGH_CUT_ID);
    updateLabelColor(mixLabel, ReverbProcessor::MIX_ID);
}

void ReverbEditor::updateXYPadFromParameters()
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

void ReverbEditor::updateParametersFromXYPad(float x, float y)
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

void ReverbEditor::showParameterMenu(juce::Label* label, const juce::String& parameterID)
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
                    xParameterIDs.add(ReverbProcessor::ROOM_SIZE_ID);
                    
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
                    yParameterIDs.add(ReverbProcessor::DAMPING_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add(ReverbProcessor::ROOM_SIZE_ID);
                yParameterIDs.add(ReverbProcessor::DAMPING_ID);
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void ReverbEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == ReverbProcessor::ROOM_SIZE_ID) return "Room Size";
        if (paramID == ReverbProcessor::DAMPING_ID) return "Damping";
        if (paramID == ReverbProcessor::PRE_DELAY_ID) return "Pre Delay";
        if (paramID == ReverbProcessor::WIDTH_ID) return "Width";
        if (paramID == ReverbProcessor::LOW_CUT_ID) return "Low Cut";
        if (paramID == ReverbProcessor::HIGH_CUT_ID) return "High Cut";
        if (paramID == ReverbProcessor::MIX_ID) return "Mix";
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