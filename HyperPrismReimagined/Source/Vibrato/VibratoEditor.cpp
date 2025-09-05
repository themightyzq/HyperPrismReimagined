//==============================================================================
// HyperPrism Reimagined - Vibrato Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "VibratoEditor.h"

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
// VibratoMeter Implementation
//==============================================================================
VibratoMeter::VibratoMeter(VibratoProcessor& p) 
    : processor(p)
{
    startTimerHz(30); // 30 FPS update rate
    
    // Initialize waveform and delay buffer arrays
    lfoWaveform.resize(256, 0.0f);
    delayBuffer.resize(64, 0.0f); // Show delay buffer state
    
    // Initialize parameter values from processor
    auto& apvts = processor.getValueTreeState();
    
    if (auto* rateParam = apvts.getRawParameterValue(VibratoProcessor::RATE_ID))
        rate = rateParam->load();
    if (auto* depthParam = apvts.getRawParameterValue(VibratoProcessor::DEPTH_ID))
        depth = depthParam->load() / 100.0f; // Convert from percentage to 0-1
    if (auto* delayParam = apvts.getRawParameterValue(VibratoProcessor::DELAY_ID))
        delay = delayParam->load();
    if (auto* feedbackParam = apvts.getRawParameterValue(VibratoProcessor::FEEDBACK_ID))
        feedback = feedbackParam->load() / 100.0f; // Convert from percentage to 0-1
}

VibratoMeter::~VibratoMeter()
{
    stopTimer();
}

void VibratoMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(HyperPrismLookAndFeel::Colors::background);
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Border
    g.setColour(HyperPrismLookAndFeel::Colors::outline);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    
    // Create display area
    auto displayArea = bounds.reduced(10.0f);
    
    // Top section - LFO waveform
    auto lfoArea = displayArea.removeFromTop(displayArea.getHeight() * 0.4f);
    
    // Draw LFO waveform
    g.setColour(HyperPrismLookAndFeel::Colors::primary);
    juce::Path waveformPath;
    
    for (size_t i = 0; i < lfoWaveform.size(); ++i)
    {
        float x = lfoArea.getX() + (i / float(lfoWaveform.size() - 1)) * lfoArea.getWidth();
        float y = lfoArea.getCentreY() - lfoWaveform[i] * lfoArea.getHeight() * 0.3f * depth;
        
        if (i == 0)
            waveformPath.startNewSubPath(x, y);
        else
            waveformPath.lineTo(x, y);
    }
    
    g.strokePath(waveformPath, juce::PathStrokeType(2.0f));
    
    // Draw center line
    g.setColour(HyperPrismLookAndFeel::Colors::outlineVariant.withAlpha(0.5f));
    g.drawHorizontalLine(static_cast<int>(lfoArea.getCentreY()), lfoArea.getX(), lfoArea.getRight());
    
    // Draw current phase indicator
    float phaseX = lfoArea.getX() + currentPhase * lfoArea.getWidth();
    g.setColour(HyperPrismLookAndFeel::Colors::warning);
    g.drawVerticalLine(static_cast<int>(phaseX), lfoArea.getY(), lfoArea.getBottom());
    
    // LFO label
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(9.0f);
    auto lfoLabelY = lfoArea.getBottom() + 2;
    g.drawText("LFO Modulation", lfoArea.withY(lfoLabelY).withHeight(12), juce::Justification::centred);
    
    displayArea.removeFromTop(15); // spacing for label
    
    // Middle section - Delay visualization
    auto delayVisualizationArea = displayArea.removeFromTop(displayArea.getHeight() * 0.4f);
    
    // Draw delay buffer as a circular visualization
    auto centerX = delayVisualizationArea.getCentreX();
    auto centerY = delayVisualizationArea.getCentreY();
    auto radius = juce::jmin(delayVisualizationArea.getWidth(), delayVisualizationArea.getHeight()) * 0.3f;
    
    // Draw outer circle for delay buffer
    g.setColour(HyperPrismLookAndFeel::Colors::outlineVariant);
    g.drawEllipse(centerX - radius, centerY - radius, radius * 2, radius * 2, 1.0f);
    
    // Draw delay buffer contents as dots around the circle
    if (!delayBuffer.empty())
    {
        for (size_t i = 0; i < delayBuffer.size(); ++i)
        {
            float angle = (i / float(delayBuffer.size())) * 2.0f * juce::MathConstants<float>::pi;
            float x = centerX + std::cos(angle) * radius;
            float y = centerY + std::sin(angle) * radius;
            
            float intensity = delayBuffer[i];
            auto dotColor = HyperPrismLookAndFeel::Colors::success.withAlpha(intensity);
            g.setColour(dotColor);
            g.fillEllipse(x - 2, y - 2, 4, 4);
        }
    }
    
    // Draw feedback indicator (inner circle)
    if (feedback > 0.01f)
    {
        float innerRadius = radius * feedback;
        g.setColour(HyperPrismLookAndFeel::Colors::error.withAlpha(0.5f));
        g.fillEllipse(centerX - innerRadius, centerY - innerRadius, innerRadius * 2, innerRadius * 2);
    }
    
    // Delay visualization label
    auto delayLabelY = delayVisualizationArea.getBottom() + 2;
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(9.0f);
    g.drawText("Delay Buffer", delayVisualizationArea.withY(delayLabelY).withHeight(12), juce::Justification::centred);
    
    displayArea.removeFromTop(15); // spacing for label
    
    // Bottom section - Parameter display
    auto infoArea = displayArea;
    
    // Parameter values
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(8.0f);
    
    auto paramArea = infoArea.removeFromTop(infoArea.getHeight() / 2);
    auto leftParamArea = paramArea.removeFromLeft(paramArea.getWidth() / 2);
    
    g.drawText("Rate: " + juce::String(rate, 1) + " Hz", leftParamArea, juce::Justification::centredLeft);
    g.drawText("Depth: " + juce::String(static_cast<int>(depth * 100)) + "%", paramArea, juce::Justification::centredLeft);
    
    auto bottomParamArea = infoArea;
    auto leftBottomArea = bottomParamArea.removeFromLeft(bottomParamArea.getWidth() / 2);
    
    g.drawText("Delay: " + juce::String(delay, 1) + " ms", leftBottomArea, juce::Justification::centredLeft);
    g.drawText("Feedback: " + juce::String(static_cast<int>(feedback * 100)) + "%", bottomParamArea, juce::Justification::centredLeft);
}

void VibratoMeter::timerCallback()
{
    // Read actual parameter values from the processor
    auto& apvts = processor.getValueTreeState();
    
    if (auto* rateParam = apvts.getRawParameterValue(VibratoProcessor::RATE_ID))
        rate = rateParam->load();
    if (auto* depthParam = apvts.getRawParameterValue(VibratoProcessor::DEPTH_ID))
        depth = depthParam->load() / 100.0f; // Convert from percentage to 0-1
    if (auto* delayParam = apvts.getRawParameterValue(VibratoProcessor::DELAY_ID))
        delay = delayParam->load();
    if (auto* feedbackParam = apvts.getRawParameterValue(VibratoProcessor::FEEDBACK_ID))
        feedback = feedbackParam->load() / 100.0f; // Convert from percentage to 0-1
    
    // Update phase based on actual rate
    // Timer runs at 30Hz, so each callback is ~33.33ms
    float phaseIncrement = rate * (1.0f / 30.0f); // rate in Hz * time in seconds
    currentPhase += phaseIncrement;
    while (currentPhase >= 1.0f)
        currentPhase -= 1.0f;
    
    // Generate LFO waveform (sine wave)
    for (size_t i = 0; i < lfoWaveform.size(); ++i)
    {
        float phase = i / float(lfoWaveform.size());
        lfoWaveform[i] = std::sin(2.0f * juce::MathConstants<float>::pi * phase);
    }
    
    // Simulate delay buffer activity
    for (size_t i = 0; i < delayBuffer.size(); ++i)
    {
        // Create a moving pattern in the delay buffer
        float bufferPhase = (i + currentPhase * delayBuffer.size()) / delayBuffer.size();
        delayBuffer[i] = (std::sin(bufferPhase * 4.0f * juce::MathConstants<float>::pi) + 1.0f) * 0.5f * depth;
    }
    
    repaint();
}

//==============================================================================
// VibratoEditor Implementation
//==============================================================================
VibratoEditor::VibratoEditor(VibratoProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), vibratoMeter(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add(VibratoProcessor::RATE_ID);
    yParameterIDs.add(VibratoProcessor::DEPTH_ID);
    
    // Title (matching AutoPan style)
    titleLabel.setText("HyperPrism Reimagined Vibrato", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions("Arial", "Bold", 24.0f)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Setup sliders with consistent style
    setupSlider(mixSlider, mixLabel, "Mix", " %");
    setupSlider(rateSlider, rateLabel, "Rate", " Hz");
    setupSlider(depthSlider, depthLabel, "Depth", " %");
    setupSlider(delaySlider, delayLabel, "Delay", " ms");
    setupSlider(feedbackSlider, feedbackLabel, "Feedback", " %");
    
    // Set parameter ranges (example ranges - adjust based on processor)
    mixSlider.setRange(0.0, 100.0, 0.1);
    rateSlider.setRange(0.1, 20.0, 0.1);
    depthSlider.setRange(0.0, 100.0, 0.1);
    delaySlider.setRange(0.1, 50.0, 0.1);
    feedbackSlider.setRange(0.0, 95.0, 0.1);
    
    // Set up right-click handlers for parameter assignment
    mixLabel.onClick = [this]() { showParameterMenu(&mixLabel, VibratoProcessor::MIX_ID); };
    rateLabel.onClick = [this]() { showParameterMenu(&rateLabel, VibratoProcessor::RATE_ID); };
    depthLabel.onClick = [this]() { showParameterMenu(&depthLabel, VibratoProcessor::DEPTH_ID); };
    delayLabel.onClick = [this]() { showParameterMenu(&delayLabel, VibratoProcessor::DELAY_ID); };
    feedbackLabel.onClick = [this]() { showParameterMenu(&feedbackLabel, VibratoProcessor::FEEDBACK_ID); };
    
    // Bypass button (top right like AutoPan)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::red);
    bypassButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
    addAndMakeVisible(bypassButton);
    
    // Create attachments
    auto& apvts = audioProcessor.getValueTreeState();
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, VibratoProcessor::MIX_ID, mixSlider);
    rateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, VibratoProcessor::RATE_ID, rateSlider);
    depthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, VibratoProcessor::DEPTH_ID, depthSlider);
    delayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, VibratoProcessor::DELAY_ID, delaySlider);
    feedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, VibratoProcessor::FEEDBACK_ID, feedbackSlider);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, VibratoProcessor::BYPASS_ID, bypassButton);
    
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
    
    // Add vibrato meter
    addAndMakeVisible(vibratoMeter);
    vibratoMeterLabel.setText("Vibrato Analysis", juce::dontSendNotification);
    vibratoMeterLabel.setJustificationType(juce::Justification::centred);
    vibratoMeterLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(vibratoMeterLabel);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    mixSlider.onValueChange = [this] { 
        updateXYPadFromParameters(); 
        vibratoMeter.repaint(); 
    };
    rateSlider.onValueChange = [this] { 
        updateXYPadFromParameters(); 
        vibratoMeter.repaint(); 
    };
    depthSlider.onValueChange = [this] { 
        updateXYPadFromParameters(); 
        vibratoMeter.repaint(); 
    };
    delaySlider.onValueChange = [this] { 
        updateXYPadFromParameters(); 
        vibratoMeter.repaint(); 
    };
    feedbackSlider.onValueChange = [this] { 
        updateXYPadFromParameters(); 
        vibratoMeter.repaint(); 
    };
    
    setSize(650, 600);
}

VibratoEditor::~VibratoEditor()
{
    setLookAndFeel(nullptr);
}

void VibratoEditor::paint(juce::Graphics& g)
{
    // Dark background matching AutoPan
    g.fillAll(HyperPrismLookAndFeel::Colors::background);
}

void VibratoEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(40));
    
    // Bypass button (top right)
    bypassButton.setBounds(bounds.getWidth() - 100, 10, 80, 30);
    
    bounds.reduce(20, 10);
    
    // Optimized layout for 650x600 - single row with all 5 controls
    auto sliderWidth = 75;
    auto spacing = 12;
    
    // Single row with all 5 controls
    auto controlsRow = bounds.removeFromTop(130);
    auto totalControlsWidth = sliderWidth * 5 + spacing * 4;
    auto controlsStartX = (bounds.getWidth() - totalControlsWidth) / 2;
    controlsRow.removeFromLeft(controlsStartX);
    
    // All controls in one row: Mix, Rate, Depth, Delay, Feedback
    mixSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    mixLabel.setBounds(mixSlider.getX(), mixSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    rateSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    rateLabel.setBounds(rateSlider.getX(), rateSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    depthSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    depthLabel.setBounds(depthSlider.getX(), depthSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    delaySlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    delayLabel.setBounds(delaySlider.getX(), delaySlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    feedbackSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    feedbackLabel.setBounds(feedbackSlider.getX(), feedbackSlider.getBottom(), sliderWidth, 20);
    
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
    
    // Vibrato meter on right (matching height)
    auto meterBounds = bottomArea.withX(xyPadBounds.getRight() + 20).withWidth(meterSize).withHeight(panelHeight);
    vibratoMeter.setBounds(meterBounds);
    
    // Align labels at the same Y position
    auto labelY = xyPadBounds.getBottom() + 5;
    xyPadLabel.setBounds(xyPad.getX(), labelY, xyPadWidth, 20);
    vibratoMeterLabel.setBounds(vibratoMeter.getX(), labelY, meterSize, 20);
}

void VibratoEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void VibratoEditor::updateParameterColors()
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
    
    updateLabelColor(mixLabel, VibratoProcessor::MIX_ID);
    updateLabelColor(rateLabel, VibratoProcessor::RATE_ID);
    updateLabelColor(depthLabel, VibratoProcessor::DEPTH_ID);
    updateLabelColor(delayLabel, VibratoProcessor::DELAY_ID);
    updateLabelColor(feedbackLabel, VibratoProcessor::FEEDBACK_ID);
}

void VibratoEditor::updateXYPadFromParameters()
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

void VibratoEditor::updateParametersFromXYPad(float x, float y)
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

void VibratoEditor::showParameterMenu(juce::Label* label, const juce::String& parameterID)
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
                    xParameterIDs.add(VibratoProcessor::RATE_ID);
                    
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
                    yParameterIDs.add(VibratoProcessor::DEPTH_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add(VibratoProcessor::RATE_ID);
                yParameterIDs.add(VibratoProcessor::DEPTH_ID);
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void VibratoEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == VibratoProcessor::MIX_ID) return "Mix";
        if (paramID == VibratoProcessor::RATE_ID) return "Rate";
        if (paramID == VibratoProcessor::DEPTH_ID) return "Depth";
        if (paramID == VibratoProcessor::DELAY_ID) return "Delay";
        if (paramID == VibratoProcessor::FEEDBACK_ID) return "Feedback";
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