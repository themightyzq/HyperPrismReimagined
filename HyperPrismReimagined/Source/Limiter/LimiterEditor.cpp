//==============================================================================
// HyperPrism Reimagined - Limiter Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "LimiterEditor.h"

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
// GainReductionMeter Implementation
//==============================================================================
GainReductionMeter::GainReductionMeter(LimiterProcessor& p) 
    : processor(p)
{
    startTimerHz(30); // 30 FPS update rate
}

GainReductionMeter::~GainReductionMeter()
{
    stopTimer();
}

void GainReductionMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(HyperPrismLookAndFeel::Colors::background);
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Border
    g.setColour(HyperPrismLookAndFeel::Colors::outline);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    
    // Create meter area
    auto meterArea = bounds.reduced(4.0f);
    
    // Draw gain reduction
    if (currentGainReduction > 0.001f)
    {
        float meterHeight = meterArea.getHeight() * currentGainReduction;
        auto meterRect = juce::Rectangle<float>(meterArea.getX(), 
                                               meterArea.getBottom() - meterHeight, 
                                               meterArea.getWidth(), 
                                               meterHeight);
        
        // Gradient based on amount
        if (currentGainReduction < 0.3f)
            g.setColour(HyperPrismLookAndFeel::Colors::success);
        else if (currentGainReduction < 0.7f)
            g.setColour(HyperPrismLookAndFeel::Colors::warning);
        else
            g.setColour(HyperPrismLookAndFeel::Colors::error);
            
        g.fillRoundedRectangle(meterRect, 2.0f);
    }
    
    // Draw scale lines
    g.setColour(HyperPrismLookAndFeel::Colors::surface);
    for (int i = 1; i < 4; ++i)
    {
        float y = meterArea.getY() + (meterArea.getHeight() * i / 4.0f);
        g.drawHorizontalLine(static_cast<int>(y), meterArea.getX(), meterArea.getRight());
    }
    
    // dB labels
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(9.0f);
    
    // Draw labels on the left side
    for (int db = 0; db <= 20; db += 5)
    {
        float y = meterArea.getBottom() - (db / 20.0f * meterArea.getHeight());
        g.drawText(juce::String(-db), bounds.getX() - 25, y - 6, 20, 12, 
                   juce::Justification::centredRight);
    }
}

void GainReductionMeter::timerCallback()
{
    float newGainReduction = processor.getCurrentGainReduction();
    
    // Smooth the meter display
    const float smoothing = 0.7f;
    currentGainReduction = currentGainReduction * smoothing + newGainReduction * (1.0f - smoothing);
    
    repaint();
}

//==============================================================================
// LimiterEditor Implementation
//==============================================================================
LimiterEditor::LimiterEditor(LimiterProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), gainReductionMeter(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add(CEILING_ID);
    yParameterIDs.add(RELEASE_ID);
    
    // Title (matching AutoPan style)
    titleLabel.setText("HyperPrism Reimagined Limiter", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions("Arial", "Bold", 24.0f)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Setup sliders with consistent style (4 sliders + 1 toggle layout)
    setupSlider(ceilingSlider, ceilingLabel, "Ceiling", " dB");
    setupSlider(releaseSlider, releaseLabel, "Release", " ms");
    setupSlider(lookaheadSlider, lookaheadLabel, "Lookahead", " ms");
    setupSlider(inputGainSlider, inputGainLabel, "Input Gain", " dB");
    
    // Set up right-click handlers for parameter assignment
    ceilingLabel.onClick = [this]() { showParameterMenu(&ceilingLabel, CEILING_ID); };
    releaseLabel.onClick = [this]() { showParameterMenu(&releaseLabel, RELEASE_ID); };
    lookaheadLabel.onClick = [this]() { showParameterMenu(&lookaheadLabel, LOOKAHEAD_ID); };
    inputGainLabel.onClick = [this]() { showParameterMenu(&inputGainLabel, INPUT_GAIN_ID); };
    
    // Soft Clip toggle
    softClipButton.setButtonText("Soft Clip");
    softClipButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    softClipButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::cyan);
    addAndMakeVisible(softClipButton);
    
    softClipLabel.setText("Soft Clip", juce::dontSendNotification);
    softClipLabel.setJustificationType(juce::Justification::centred);
    softClipLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(softClipLabel);
    
    // Bypass button (top right like AutoPan)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::red);
    bypassButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
    addAndMakeVisible(bypassButton);
    
    // Create attachments
    auto& apvts = audioProcessor.getStateInformation();
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, BYPASS_ID, bypassButton);
    ceilingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, CEILING_ID, ceilingSlider);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, RELEASE_ID, releaseSlider);
    lookaheadAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, LOOKAHEAD_ID, lookaheadSlider);
    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, INPUT_GAIN_ID, inputGainSlider);
    softClipAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, SOFTCLIP_ID, softClipButton);
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);
    xyPadLabel.setText("Ceiling / Release", juce::dontSendNotification);
    xyPadLabel.setJustificationType(juce::Justification::centred);
    xyPadLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    
    // Add gain reduction meter
    addAndMakeVisible(gainReductionMeter);
    meterLabel.setText("Gain Reduction", juce::dontSendNotification);
    meterLabel.setJustificationType(juce::Justification::centred);
    meterLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(meterLabel);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    ceilingSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    releaseSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    lookaheadSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    inputGainSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    setSize(650, 600);
}

LimiterEditor::~LimiterEditor()
{
    setLookAndFeel(nullptr);
}

void LimiterEditor::paint(juce::Graphics& g)
{
    // Dark background matching AutoPan
    g.fillAll(HyperPrismLookAndFeel::Colors::background);
}

void LimiterEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(40));
    
    // Bypass button (top right)
    bypassButton.setBounds(bounds.getWidth() - 100, 10, 80, 30);
    
    bounds.reduce(20, 10);
    
    // 5 parameter layout: 4 sliders + 1 toggle (similar to Phaser/HyperPhaser)
    // Available height after title and margins: ~500px
    // Distribution: 140px + 10px + 80px + 10px + 180px + 25px = 435px
    
    auto topRow = bounds.removeFromTop(140);
    auto sliderWidth = 80;
    auto spacing = 15;
    
    // Calculate total width needed for 4 sliders
    auto totalSliderWidth = sliderWidth * 4 + spacing * 3;
    auto startX = (bounds.getWidth() - totalSliderWidth) / 2;
    topRow.removeFromLeft(startX);
    
    // Top row: Ceiling, Release, Lookahead, Input Gain
    ceilingSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    ceilingLabel.setBounds(ceilingSlider.getX(), ceilingSlider.getBottom(), sliderWidth, 20);
    topRow.removeFromLeft(spacing);
    
    releaseSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    releaseLabel.setBounds(releaseSlider.getX(), releaseSlider.getBottom(), sliderWidth, 20);
    topRow.removeFromLeft(spacing);
    
    lookaheadSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    lookaheadLabel.setBounds(lookaheadSlider.getX(), lookaheadSlider.getBottom(), sliderWidth, 20);
    topRow.removeFromLeft(spacing);
    
    inputGainSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    inputGainLabel.setBounds(inputGainSlider.getX(), inputGainSlider.getBottom(), sliderWidth, 20);
    
    bounds.removeFromTop(10);
    
    // Second row - Soft Clip toggle (centered)
    auto secondRow = bounds.removeFromTop(80);
    auto buttonWidth = 100;
    auto buttonX = bounds.getX() + (bounds.getWidth() - buttonWidth) / 2;
    
    softClipButton.setBounds(buttonX, secondRow.getY() + 30, buttonWidth, 25);
    softClipLabel.setBounds(buttonX, secondRow.getY() + 5, buttonWidth, 20);
    
    // Bottom section - XY Pad and Meter side by side (matching heights)
    bounds.removeFromTop(10);
    
    // Split remaining space horizontally for XY pad and meter
    auto bottomArea = bounds;
    auto panelHeight = 180; // Standard XY pad height
    
    // Calculate positioning to center both panels
    auto xyPadWidth = 200;
    auto meterWidth = 100;
    auto totalWidth = xyPadWidth + meterWidth + 20; // Plus spacing
    auto startXBottom = (bottomArea.getWidth() - totalWidth) / 2;
    
    // XY Pad on left (200x180 standard)
    auto xyPadBounds = bottomArea.withX(bottomArea.getX() + startXBottom).withWidth(xyPadWidth).withHeight(panelHeight);
    xyPad.setBounds(xyPadBounds);
    
    // Gain Reduction Meter on right (matching height)
    auto meterBounds = bottomArea.withX(xyPadBounds.getRight() + 20).withWidth(meterWidth).withHeight(panelHeight);
    gainReductionMeter.setBounds(meterBounds);
    
    // Align labels at the same Y position
    auto labelY = xyPadBounds.getBottom() + 5;
    xyPadLabel.setBounds(xyPadBounds.getX(), labelY, xyPadWidth, 20);
    meterLabel.setBounds(meterBounds.getX(), labelY, meterWidth, 20);
}

void LimiterEditor::setupControls()
{
    // This method is no longer used - moved into constructor
}

void LimiterEditor::setupXYPad()
{
    // This method is no longer used - moved into constructor
}

void LimiterEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void LimiterEditor::updateParameterColors()
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
    
    updateLabelColor(ceilingLabel, CEILING_ID);
    updateLabelColor(releaseLabel, RELEASE_ID);
    updateLabelColor(lookaheadLabel, LOOKAHEAD_ID);
    updateLabelColor(inputGainLabel, INPUT_GAIN_ID);
}

void LimiterEditor::updateXYPadFromParameters()
{
    // For multiple parameters, use the average of their normalized values
    float xValue = 0.0f;
    float yValue = 0.0f;
    
    auto& apvts = audioProcessor.getStateInformation();
    
    // Calculate average X value
    if (!xParameterIDs.isEmpty())
    {
        for (const auto& paramID : xParameterIDs)
        {
            if (auto* param = apvts.getRawParameterValue(paramID))
            {
                if (auto* paramObj = apvts.getParameter(paramID))
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
            if (auto* param = apvts.getRawParameterValue(paramID))
            {
                if (auto* paramObj = apvts.getParameter(paramID))
                {
                    yValue += paramObj->convertTo0to1(*param);
                }
            }
        }
        yValue /= yParameterIDs.size();
    }
    
    xyPad.setValues(xValue, yValue);
}

void LimiterEditor::updateParametersFromXYPad(float x, float y)
{
    auto& apvts = audioProcessor.getStateInformation();
    
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

void LimiterEditor::showParameterMenu(juce::Label* label, const juce::String& parameterID)
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
                    xParameterIDs.add(CEILING_ID);
                    
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
                    yParameterIDs.add(RELEASE_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add(CEILING_ID);
                yParameterIDs.add(RELEASE_ID);
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void LimiterEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == CEILING_ID) return "Ceiling";
        if (paramID == RELEASE_ID) return "Release";
        if (paramID == LOOKAHEAD_ID) return "Lookahead";
        if (paramID == INPUT_GAIN_ID) return "Input Gain";
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