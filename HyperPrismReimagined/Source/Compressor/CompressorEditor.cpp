//==============================================================================
// HyperPrism Reimagined - Compressor Editor
// Updated to match modern template
//==============================================================================

#include "CompressorEditor.h"

//==============================================================================
// XYPad Implementation (matching modern style)
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
// GainReductionMeter Implementation (horizontal meter like modern template)
//==============================================================================
GainReductionMeter::GainReductionMeter(CompressorProcessor& p) : processor(p)
{
    startTimerHz(30);
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
    g.fillRoundedRectangle(bounds, 3.0f);
    
    // Meter background
    g.setColour(HyperPrismLookAndFeel::Colors::surfaceVariant);
    g.fillRoundedRectangle(bounds.reduced(2), 2.0f);
    
    // Draw gain reduction horizontally
    if (currentReduction > 0.01f)
    {
        float meterWidth = bounds.getWidth() - 4;
        float reductionWidth = currentReduction * meterWidth;
        
        // Color gradient from green to yellow to red
        juce::Colour meterColor;
        if (currentReduction < 0.33f)
            meterColor = HyperPrismLookAndFeel::Colors::success;
        else if (currentReduction < 0.66f)
            meterColor = HyperPrismLookAndFeel::Colors::warning;
        else
            meterColor = HyperPrismLookAndFeel::Colors::error;
        
        g.setColour(meterColor);
        g.fillRoundedRectangle(bounds.getX() + 2, 
                               bounds.getY() + 2,
                               reductionWidth, 
                               bounds.getHeight() - 4, 
                               2.0f);
    }
    
    // Draw scale marks horizontally
    g.setColour(HyperPrismLookAndFeel::Colors::onSurface.withAlpha(0.5f));
    for (int db = 0; db <= 20; db += 5)
    {
        float x = bounds.getX() + 2 + (db / 20.0f) * (bounds.getWidth() - 4);
        g.drawLine(x, bounds.getY(), x, bounds.getY() + 5, 1.0f);
        
        if (db % 10 == 0)
        {
            g.setFont(10.0f);
            g.drawText(juce::String(db), x - 5, bounds.getY() + 7, 10, 20, 
                      juce::Justification::centred);
        }
    }
    
    // Border
    g.setColour(HyperPrismLookAndFeel::Colors::outline);
    g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
}

void GainReductionMeter::timerCallback()
{
    targetReduction = processor.getGainReduction();
    
    // Smooth the meter movement
    const float smoothing = 0.3f;
    currentReduction = currentReduction + (targetReduction - currentReduction) * smoothing;
    
    repaint();
}

//==============================================================================
// CompressorEditor Implementation
//==============================================================================
CompressorEditor::CompressorEditor(CompressorProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), gainReductionMeter(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add("threshold");
    yParameterIDs.add("ratio");
    
    // Title (matching modern style)
    titleLabel.setText("HyperPrism Reimagined Compressor", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions("Arial", "Bold", 24.0f)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Setup sliders with consistent style
    setupSlider(thresholdSlider, thresholdLabel, "Threshold", " dB");
    setupSlider(ratioSlider, ratioLabel, "Ratio", ":1");
    setupSlider(attackSlider, attackLabel, "Attack", " ms");
    setupSlider(releaseSlider, releaseLabel, "Release", " ms");
    setupSlider(kneeSlider, kneeLabel, "Knee", " dB");
    setupSlider(makeupGainSlider, makeupGainLabel, "Makeup", " dB");
    setupSlider(mixSlider, mixLabel, "Mix", " %");
    
    // Set up right-click handlers for parameter assignment
    thresholdLabel.onClick = [this]() { showParameterMenu(&thresholdLabel, "threshold"); };
    ratioLabel.onClick = [this]() { showParameterMenu(&ratioLabel, "ratio"); };
    attackLabel.onClick = [this]() { showParameterMenu(&attackLabel, "attack"); };
    releaseLabel.onClick = [this]() { showParameterMenu(&releaseLabel, "release"); };
    kneeLabel.onClick = [this]() { showParameterMenu(&kneeLabel, "knee"); };
    makeupGainLabel.onClick = [this]() { showParameterMenu(&makeupGainLabel, "makeupGain"); };
    mixLabel.onClick = [this]() { showParameterMenu(&mixLabel, "mix"); };
    
    // Create attachments
    thresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "threshold", thresholdSlider);
    ratioAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "ratio", ratioSlider);
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "attack", attackSlider);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "release", releaseSlider);
    kneeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "knee", kneeSlider);
    makeupGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "makeupGain", makeupGainSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "mix", mixSlider);
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);  // Set the axis colors
    xyPadLabel.setText("Threshold / Ratio", juce::dontSendNotification);
    xyPadLabel.setJustificationType(juce::Justification::centred);
    xyPadLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    
    // Setup gain reduction meter
    addAndMakeVisible(gainReductionMeter);
    meterLabel.setText("GR", juce::dontSendNotification);
    meterLabel.setJustificationType(juce::Justification::centred);
    meterLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(meterLabel);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();  // Show initial color coding
    
    // Listen for parameter changes - update XY pad when any parameter changes
    thresholdSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    ratioSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    attackSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    releaseSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    kneeSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    makeupGainSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    mixSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    setSize(650, 600);
}

CompressorEditor::~CompressorEditor()
{
    setLookAndFeel(nullptr);
}

void CompressorEditor::paint(juce::Graphics& g)
{
    // Dark background matching modern template
    g.fillAll(HyperPrismLookAndFeel::Colors::background);
}

void CompressorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(40));
    
    bounds.reduce(20, 10);
    
    // Optimized layout for 650x600 - single row with all controls
    auto sliderWidth = 75;
    auto spacing = 15;
    
    // Single row with all 7 controls
    auto controlsRow = bounds.removeFromTop(130);
    auto totalControlsWidth = sliderWidth * 7 + spacing * 6;
    auto controlsStartX = (bounds.getWidth() - totalControlsWidth) / 2;
    controlsRow.removeFromLeft(controlsStartX);
    
    // All controls in one row: Threshold, Ratio, Attack, Release, Knee, Makeup, Mix
    thresholdSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    thresholdLabel.setBounds(thresholdSlider.getX(), thresholdSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    ratioSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    ratioLabel.setBounds(ratioSlider.getX(), ratioSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    attackSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    attackLabel.setBounds(attackSlider.getX(), attackSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    releaseSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    releaseLabel.setBounds(releaseSlider.getX(), releaseSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    kneeSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    kneeLabel.setBounds(kneeSlider.getX(), kneeSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    makeupGainSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    makeupGainLabel.setBounds(makeupGainSlider.getX(), makeupGainSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    mixSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    mixLabel.setBounds(mixSlider.getX(), mixSlider.getBottom(), sliderWidth, 20);
    
    bounds.removeFromTop(15);
    
    // Bottom section - XY Pad and Meter side by side (simplified layout)
    auto bottomSection = bounds;
    auto panelHeight = 180; // Standard XY pad height
    
    // Calculate positioning to center both panels
    auto xyPadWidth = 200;
    auto meterWidth = 150; // Slightly wider meter
    auto totalBottomWidth = xyPadWidth + 20 + meterWidth;
    auto bottomStartX = (bottomSection.getWidth() - totalBottomWidth) / 2;
    
    // XY Pad on left
    auto xyPadBounds = bottomSection.withX(bottomSection.getX() + bottomStartX).withWidth(xyPadWidth).withHeight(panelHeight);
    xyPad.setBounds(xyPadBounds);
    
    // Gain reduction meter on right (matching height)
    auto meterBounds = bottomSection.withX(xyPadBounds.getRight() + 20).withWidth(meterWidth).withHeight(panelHeight);
    gainReductionMeter.setBounds(meterBounds);
    
    // Align labels at the same Y position
    auto labelY = xyPadBounds.getBottom() + 5;
    xyPadLabel.setBounds(xyPad.getX(), labelY, xyPadWidth, 20);
    meterLabel.setBounds(gainReductionMeter.getX(), labelY, meterWidth, 20);
}

void CompressorEditor::setupSlider(juce::Slider& slider, juce::Label& label, 
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

void CompressorEditor::updateParameterColors()
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
    
    updateLabelColor(thresholdLabel, "threshold");
    updateLabelColor(ratioLabel, "ratio");
    updateLabelColor(attackLabel, "attack");
    updateLabelColor(releaseLabel, "release");
    updateLabelColor(kneeLabel, "knee");
    updateLabelColor(makeupGainLabel, "makeupGain");
    updateLabelColor(mixLabel, "mix");
}

void CompressorEditor::updateXYPadFromParameters()
{
    // For multiple parameters, use the average of their normalized values
    float xValue = 0.0f;
    float yValue = 0.0f;
    
    // Calculate average X value
    if (!xParameterIDs.isEmpty())
    {
        for (const auto& paramID : xParameterIDs)
        {
            if (auto* param = audioProcessor.apvts.getRawParameterValue(paramID))
            {
                if (auto* paramObj = audioProcessor.apvts.getParameter(paramID))
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
            if (auto* param = audioProcessor.apvts.getRawParameterValue(paramID))
            {
                if (auto* paramObj = audioProcessor.apvts.getParameter(paramID))
                {
                    yValue += paramObj->convertTo0to1(*param);
                }
            }
        }
        yValue /= yParameterIDs.size();
    }
    
    xyPad.setValues(xValue, yValue);
}

void CompressorEditor::updateParametersFromXYPad(float x, float y)
{
    // Update all assigned X parameters
    for (const auto& paramID : xParameterIDs)
    {
        if (auto* param = audioProcessor.apvts.getParameter(paramID))
            param->setValueNotifyingHost(x);
    }
    
    // Update all assigned Y parameters
    for (const auto& paramID : yParameterIDs)
    {
        if (auto* param = audioProcessor.apvts.getParameter(paramID))
            param->setValueNotifyingHost(y);
    }
}

void CompressorEditor::showParameterMenu(juce::Label* label, const juce::String& parameterID)
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
                    xParameterIDs.add("threshold");
                    
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
                    yParameterIDs.add("ratio");
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add("threshold");
                yParameterIDs.add("ratio");
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void CompressorEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == "threshold") return "Threshold";
        if (paramID == "ratio") return "Ratio";
        if (paramID == "attack") return "Attack";
        if (paramID == "release") return "Release";
        if (paramID == "knee") return "Knee";
        if (paramID == "makeupGain") return "Makeup";
        if (paramID == "mix") return "Mix";
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