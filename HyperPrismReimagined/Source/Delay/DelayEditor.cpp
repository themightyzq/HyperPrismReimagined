//==============================================================================
// HyperPrism Reimagined - Delay Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "DelayEditor.h"

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
// DelayEditor Implementation
//==============================================================================
DelayEditor::DelayEditor(DelayProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add(DelayProcessor::DELAY_TIME_ID);
    yParameterIDs.add(DelayProcessor::FEEDBACK_ID);
    
    // Title (matching AutoPan style)
    titleLabel.setText("HyperPrism Reimagined Delay", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions("Arial", "Bold", 24.0f)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Setup sliders with consistent style (7 parameters - 4+3 layout)
    setupSlider(mixSlider, mixLabel, "Mix", "%");
    setupSlider(delayTimeSlider, delayTimeLabel, "Delay Time", " ms");
    setupSlider(feedbackSlider, feedbackLabel, "Feedback", "%");
    setupSlider(lowCutSlider, lowCutLabel, "Low Cut", " Hz");
    setupSlider(highCutSlider, highCutLabel, "High Cut", " Hz");
    setupSlider(stereoOffsetSlider, stereoOffsetLabel, "Stereo Offset", " ms");
    
    // Set up right-click handlers for parameter assignment
    mixLabel.onClick = [this]() { showParameterMenu(&mixLabel, DelayProcessor::MIX_ID); };
    delayTimeLabel.onClick = [this]() { showParameterMenu(&delayTimeLabel, DelayProcessor::DELAY_TIME_ID); };
    feedbackLabel.onClick = [this]() { showParameterMenu(&feedbackLabel, DelayProcessor::FEEDBACK_ID); };
    lowCutLabel.onClick = [this]() { showParameterMenu(&lowCutLabel, DelayProcessor::LOW_CUT_ID); };
    highCutLabel.onClick = [this]() { showParameterMenu(&highCutLabel, DelayProcessor::HIGH_CUT_ID); };
    stereoOffsetLabel.onClick = [this]() { showParameterMenu(&stereoOffsetLabel, DelayProcessor::STEREO_OFFSET_ID); };
    
    // Tempo Sync toggle
    tempoSyncButton.setButtonText("Tempo Sync");
    tempoSyncButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    tempoSyncButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::cyan);
    addAndMakeVisible(tempoSyncButton);
    
    // Bypass button (top right like AutoPan)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::red);
    bypassButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
    addAndMakeVisible(bypassButton);
    
    // Create attachments
    auto& apvts = audioProcessor.getValueTreeState();
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, DelayProcessor::BYPASS_ID, bypassButton);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, DelayProcessor::MIX_ID, mixSlider);
    delayTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, DelayProcessor::DELAY_TIME_ID, delayTimeSlider);
    feedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, DelayProcessor::FEEDBACK_ID, feedbackSlider);
    lowCutAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, DelayProcessor::LOW_CUT_ID, lowCutSlider);
    highCutAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, DelayProcessor::HIGH_CUT_ID, highCutSlider);
    stereoOffsetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, DelayProcessor::STEREO_OFFSET_ID, stereoOffsetSlider);
    tempoSyncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, DelayProcessor::TEMPO_SYNC_ID, tempoSyncButton);
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);
    xyPadLabel.setText("Delay Time / Feedback", juce::dontSendNotification);
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
    mixSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    delayTimeSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    feedbackSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    lowCutSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    highCutSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    stereoOffsetSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    setSize(650, 600);
}

DelayEditor::~DelayEditor()
{
    setLookAndFeel(nullptr);
}

void DelayEditor::paint(juce::Graphics& g)
{
    // Dark background matching AutoPan
    g.fillAll(HyperPrismLookAndFeel::Colors::background);
}

void DelayEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(40));
    
    // Bypass button (top right)
    bypassButton.setBounds(bounds.getWidth() - 100, 10, 80, 30);
    
    // Tempo Sync toggle (shifted to left side)
    tempoSyncButton.setBounds(20, 10, 100, 30);
    
    bounds.reduce(20, 10);
    
    // Optimized layout for 650x600 - single row with all 6 controls
    auto sliderWidth = 75;
    auto spacing = 12;
    
    // Single row with all 6 controls
    auto controlsRow = bounds.removeFromTop(130);
    auto totalControlsWidth = sliderWidth * 6 + spacing * 5;
    auto controlsStartX = (bounds.getWidth() - totalControlsWidth) / 2;
    controlsRow.removeFromLeft(controlsStartX);
    
    // All controls in one row: Mix, Delay Time, Feedback, Low Cut, High Cut, Stereo Offset
    mixSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    mixLabel.setBounds(mixSlider.getX(), mixSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    delayTimeSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    delayTimeLabel.setBounds(delayTimeSlider.getX(), delayTimeSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    feedbackSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    feedbackLabel.setBounds(feedbackSlider.getX(), feedbackSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    lowCutSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    lowCutLabel.setBounds(lowCutSlider.getX(), lowCutSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    highCutSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    highCutLabel.setBounds(highCutSlider.getX(), highCutSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    stereoOffsetSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    stereoOffsetLabel.setBounds(stereoOffsetSlider.getX(), stereoOffsetSlider.getBottom(), sliderWidth, 20);
    
    bounds.removeFromTop(15);
    
    // Bottom section - XY Pad (brought up with minimal spacing)
    
    // Center the XY Pad horizontally (200x180 standard)
    auto xyPadWidth = 200;
    auto xyPadHeight = 180;
    auto xyPadX = bounds.getX() + (bounds.getWidth() - xyPadWidth) / 2;
    auto xyPadY = bounds.getY();
    
    xyPad.setBounds(xyPadX, xyPadY, xyPadWidth, xyPadHeight);
    xyPadLabel.setBounds(xyPadX, xyPadY + xyPadHeight + 5, xyPadWidth, 20);
}

void DelayEditor::setupControls()
{
    // This method is no longer used - moved into constructor
}

void DelayEditor::setupXYPad()
{
    // This method is no longer used - moved into constructor
}

void DelayEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void DelayEditor::updateParameterColors()
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
    
    updateLabelColor(mixLabel, DelayProcessor::MIX_ID);
    updateLabelColor(delayTimeLabel, DelayProcessor::DELAY_TIME_ID);
    updateLabelColor(feedbackLabel, DelayProcessor::FEEDBACK_ID);
    updateLabelColor(lowCutLabel, DelayProcessor::LOW_CUT_ID);
    updateLabelColor(highCutLabel, DelayProcessor::HIGH_CUT_ID);
    updateLabelColor(stereoOffsetLabel, DelayProcessor::STEREO_OFFSET_ID);
}

void DelayEditor::updateXYPadFromParameters()
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

void DelayEditor::updateParametersFromXYPad(float x, float y)
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

void DelayEditor::showParameterMenu(juce::Label* label, const juce::String& parameterID)
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
                    xParameterIDs.add(DelayProcessor::DELAY_TIME_ID);
                    
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
                    yParameterIDs.add(DelayProcessor::FEEDBACK_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add(DelayProcessor::DELAY_TIME_ID);
                yParameterIDs.add(DelayProcessor::FEEDBACK_ID);
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void DelayEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == DelayProcessor::MIX_ID) return "Mix";
        if (paramID == DelayProcessor::DELAY_TIME_ID) return "Delay Time";
        if (paramID == DelayProcessor::FEEDBACK_ID) return "Feedback";
        if (paramID == DelayProcessor::LOW_CUT_ID) return "Low Cut";
        if (paramID == DelayProcessor::HIGH_CUT_ID) return "High Cut";
        if (paramID == DelayProcessor::STEREO_OFFSET_ID) return "Stereo Offset";
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