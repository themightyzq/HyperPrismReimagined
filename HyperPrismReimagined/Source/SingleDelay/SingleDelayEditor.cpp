//==============================================================================
// HyperPrism Reimagined - Single Delay Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "SingleDelayEditor.h"

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
// DelayMeter Implementation
//==============================================================================
DelayMeter::DelayMeter(SingleDelayProcessor& p) 
    : processor(p)
{
    startTimerHz(30); // 30 FPS update rate
    
    // Initialize delay buffer for visualization
    delayBuffer.resize(256);
    bufferPosition = 0;
}

DelayMeter::~DelayMeter()
{
    stopTimer();
}

void DelayMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(HyperPrismLookAndFeel::Colors::background);
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Border
    g.setColour(HyperPrismLookAndFeel::Colors::outline);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    
    // Create meter areas - split into input/output levels and delay visualization
    auto meterArea = bounds.reduced(4.0f);
    float meterHeight = meterArea.getHeight() * 0.6f;
    float visualHeight = meterArea.getHeight() * 0.4f;
    
    // I/O level meters (top section)
    auto levelArea = meterArea.removeFromTop(meterHeight);
    float halfWidth = levelArea.getWidth() / 2.0f;
    
    // Input meter (left half)
    auto inputMeterArea = juce::Rectangle<float>(levelArea.getX(), levelArea.getY(), halfWidth - 2, levelArea.getHeight());
    float inputLevelHeight = inputMeterArea.getHeight() * inputLevel;
    auto inputLevelRect = juce::Rectangle<float>(inputMeterArea.getX(), 
                                                inputMeterArea.getBottom() - inputLevelHeight, 
                                                inputMeterArea.getWidth(), 
                                                inputLevelHeight);
    
    g.setColour(HyperPrismLookAndFeel::Colors::success);
    g.fillRoundedRectangle(inputLevelRect, 2.0f);
    
    // Output meter (right half)
    auto outputMeterArea = juce::Rectangle<float>(levelArea.getX() + halfWidth + 2, levelArea.getY(), halfWidth - 2, levelArea.getHeight());
    float outputLevelHeight = outputMeterArea.getHeight() * outputLevel;
    auto outputLevelRect = juce::Rectangle<float>(outputMeterArea.getX(), 
                                                 outputMeterArea.getBottom() - outputLevelHeight, 
                                                 outputMeterArea.getWidth(), 
                                                 outputLevelHeight);
    
    g.setColour(HyperPrismLookAndFeel::Colors::primary);
    g.fillRoundedRectangle(outputLevelRect, 2.0f);
    
    // Divider line
    g.setColour(HyperPrismLookAndFeel::Colors::surface);
    g.drawVerticalLine(static_cast<int>(levelArea.getX() + halfWidth), levelArea.getY(), levelArea.getBottom());
    
    // Labels
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(9.0f);
    auto inputLabelArea = inputMeterArea.removeFromBottom(12);
    auto outputLabelArea = outputMeterArea.removeFromBottom(12);
    g.drawText("IN", inputLabelArea, juce::Justification::centred);
    g.drawText("OUT", outputLabelArea, juce::Justification::centred);
    
    // Delay visualization (bottom section)
    meterArea.removeFromTop(5); // spacing
    auto delayVisualArea = meterArea.removeFromTop(visualHeight);
    
    // Draw delay buffer as waveform
    g.setColour(HyperPrismLookAndFeel::Colors::warning);
    juce::Path delayPath;
    
    for (size_t i = 0; i < delayBuffer.size(); ++i)
    {
        float x = delayVisualArea.getX() + (i / float(delayBuffer.size() - 1)) * delayVisualArea.getWidth();
        float y = delayVisualArea.getCentreY() - delayBuffer[i] * delayVisualArea.getHeight() * 0.4f;
        
        if (i == 0)
            delayPath.startNewSubPath(x, y);
        else
            delayPath.lineTo(x, y);
    }
    
    g.strokePath(delayPath, juce::PathStrokeType(1.5f));
    
    // Delay visualization label
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(8.0f);
    g.drawText("Delay Buffer", delayVisualArea.removeFromBottom(10), juce::Justification::centred);
}

void DelayMeter::timerCallback()
{
    float newInputLevel = processor.getInputLevel();
    float newOutputLevel = processor.getOutputLevel();
    
    // Smooth the level changes
    const float smoothing = 0.7f;
    inputLevel = inputLevel * smoothing + newInputLevel * (1.0f - smoothing);
    outputLevel = outputLevel * smoothing + newOutputLevel * (1.0f - smoothing);
    
    // Update delay buffer visualization (simplified sine wave for demonstration)
    for (size_t i = 0; i < delayBuffer.size(); ++i)
    {
        float phase = (bufferPosition + i) * 0.1f;
        delayBuffer[i] = std::sin(phase) * inputLevel;
    }
    bufferPosition++;
    
    repaint();
}

//==============================================================================
// SingleDelayEditor Implementation
//==============================================================================
SingleDelayEditor::SingleDelayEditor(SingleDelayProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), delayMeter(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add(SingleDelayProcessor::DELAY_TIME_ID);
    yParameterIDs.add(SingleDelayProcessor::FEEDBACK_ID);
    
    // Title (matching AutoPan style)
    titleLabel.setText("HyperPrism Reimagined Single Delay", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions("Arial", "Bold", 24.0f)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Setup sliders with consistent style
    setupSlider(delayTimeSlider, delayTimeLabel, "Delay Time", " ms");
    setupSlider(feedbackSlider, feedbackLabel, "Feedback", "");
    setupSlider(wetDryMixSlider, wetDryMixLabel, "Wet/Dry Mix", "");
    setupSlider(highCutSlider, highCutLabel, "High Cut", " Hz");
    setupSlider(lowCutSlider, lowCutLabel, "Low Cut", " Hz");
    setupSlider(stereoSpreadSlider, stereoSpreadLabel, "Stereo Spread", "");
    
    // Set parameter ranges (example ranges - adjust based on processor)
    delayTimeSlider.setRange(1.0, 4000.0, 0.1);
    feedbackSlider.setRange(0.0, 95.0, 0.1);
    wetDryMixSlider.setRange(0.0, 100.0, 0.1);
    highCutSlider.setRange(1000.0, 20000.0, 1.0);
    lowCutSlider.setRange(20.0, 1000.0, 1.0);
    stereoSpreadSlider.setRange(0.0, 100.0, 0.1);
    
    // Set up right-click handlers for parameter assignment
    delayTimeLabel.onClick = [this]() { showParameterMenu(&delayTimeLabel, SingleDelayProcessor::DELAY_TIME_ID); };
    feedbackLabel.onClick = [this]() { showParameterMenu(&feedbackLabel, SingleDelayProcessor::FEEDBACK_ID); };
    wetDryMixLabel.onClick = [this]() { showParameterMenu(&wetDryMixLabel, SingleDelayProcessor::WETDRY_MIX_ID); };
    highCutLabel.onClick = [this]() { showParameterMenu(&highCutLabel, SingleDelayProcessor::HIGH_CUT_ID); };
    lowCutLabel.onClick = [this]() { showParameterMenu(&lowCutLabel, SingleDelayProcessor::LOW_CUT_ID); };
    stereoSpreadLabel.onClick = [this]() { showParameterMenu(&stereoSpreadLabel, SingleDelayProcessor::STEREO_SPREAD_ID); };
    
    // Bypass button (top right like AutoPan)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::red);
    bypassButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
    addAndMakeVisible(bypassButton);
    
    // Create attachments
    auto& apvts = audioProcessor.getValueTreeState();
    delayTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, SingleDelayProcessor::DELAY_TIME_ID, delayTimeSlider);
    feedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, SingleDelayProcessor::FEEDBACK_ID, feedbackSlider);
    wetDryMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, SingleDelayProcessor::WETDRY_MIX_ID, wetDryMixSlider);
    highCutAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, SingleDelayProcessor::HIGH_CUT_ID, highCutSlider);
    lowCutAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, SingleDelayProcessor::LOW_CUT_ID, lowCutSlider);
    stereoSpreadAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, SingleDelayProcessor::STEREO_SPREAD_ID, stereoSpreadSlider);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, SingleDelayProcessor::BYPASS_ID, bypassButton);
    
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
    
    // Add delay meter
    addAndMakeVisible(delayMeter);
    delayMeterLabel.setText("I/O Levels & Delay", juce::dontSendNotification);
    delayMeterLabel.setJustificationType(juce::Justification::centred);
    delayMeterLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(delayMeterLabel);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    delayTimeSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    feedbackSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    wetDryMixSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    highCutSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    lowCutSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    stereoSpreadSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    setSize(650, 600);
}

SingleDelayEditor::~SingleDelayEditor()
{
    setLookAndFeel(nullptr);
}

void SingleDelayEditor::paint(juce::Graphics& g)
{
    // Dark background matching AutoPan
    g.fillAll(HyperPrismLookAndFeel::Colors::background);
}

void SingleDelayEditor::resized()
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
    
    // All controls in one row: Delay Time, Feedback, Wet/Dry Mix, High Cut, Low Cut, Stereo Spread
    delayTimeSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    delayTimeLabel.setBounds(delayTimeSlider.getX(), delayTimeSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    feedbackSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    feedbackLabel.setBounds(feedbackSlider.getX(), feedbackSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    wetDryMixSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    wetDryMixLabel.setBounds(wetDryMixSlider.getX(), wetDryMixSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    highCutSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    highCutLabel.setBounds(highCutSlider.getX(), highCutSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    lowCutSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    lowCutLabel.setBounds(lowCutSlider.getX(), lowCutSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    stereoSpreadSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    stereoSpreadLabel.setBounds(stereoSpreadSlider.getX(), stereoSpreadSlider.getBottom(), sliderWidth, 20);
    
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
    
    // Delay meter on right (matching height)
    auto meterBounds = bottomArea.withX(xyPadBounds.getRight() + 20).withWidth(meterSize).withHeight(panelHeight);
    delayMeter.setBounds(meterBounds);
    
    // Align labels at the same Y position
    auto labelY = xyPadBounds.getBottom() + 5;
    xyPadLabel.setBounds(xyPad.getX(), labelY, xyPadWidth, 20);
    delayMeterLabel.setBounds(delayMeter.getX(), labelY, meterSize, 20);
}

void SingleDelayEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void SingleDelayEditor::updateParameterColors()
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
    
    updateLabelColor(delayTimeLabel, SingleDelayProcessor::DELAY_TIME_ID);
    updateLabelColor(feedbackLabel, SingleDelayProcessor::FEEDBACK_ID);
    updateLabelColor(wetDryMixLabel, SingleDelayProcessor::WETDRY_MIX_ID);
    updateLabelColor(highCutLabel, SingleDelayProcessor::HIGH_CUT_ID);
    updateLabelColor(lowCutLabel, SingleDelayProcessor::LOW_CUT_ID);
    updateLabelColor(stereoSpreadLabel, SingleDelayProcessor::STEREO_SPREAD_ID);
}

void SingleDelayEditor::updateXYPadFromParameters()
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

void SingleDelayEditor::updateParametersFromXYPad(float x, float y)
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

void SingleDelayEditor::showParameterMenu(juce::Label* label, const juce::String& parameterID)
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
                    xParameterIDs.add(SingleDelayProcessor::DELAY_TIME_ID);
                    
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
                    yParameterIDs.add(SingleDelayProcessor::FEEDBACK_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add(SingleDelayProcessor::DELAY_TIME_ID);
                yParameterIDs.add(SingleDelayProcessor::FEEDBACK_ID);
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void SingleDelayEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == SingleDelayProcessor::DELAY_TIME_ID) return "Delay Time";
        if (paramID == SingleDelayProcessor::FEEDBACK_ID) return "Feedback";
        if (paramID == SingleDelayProcessor::WETDRY_MIX_ID) return "Wet/Dry Mix";
        if (paramID == SingleDelayProcessor::HIGH_CUT_ID) return "High Cut";
        if (paramID == SingleDelayProcessor::LOW_CUT_ID) return "Low Cut";
        if (paramID == SingleDelayProcessor::STEREO_SPREAD_ID) return "Stereo Spread";
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