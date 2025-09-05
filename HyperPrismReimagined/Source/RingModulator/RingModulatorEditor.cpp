//==============================================================================
// HyperPrism Reimagined - Ring Modulator Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "RingModulatorEditor.h"

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
// RingModulatorMeter Implementation
//==============================================================================
RingModulatorMeter::RingModulatorMeter(RingModulatorProcessor& p) 
    : processor(p)
{
    startTimerHz(30); // 30 FPS update rate
    
    // Initialize waveform arrays
    carrierWaveform.resize(256);
    modulatorWaveform.resize(256);
    outputWaveform.resize(256);
}

RingModulatorMeter::~RingModulatorMeter()
{
    stopTimer();
}

void RingModulatorMeter::paint(juce::Graphics& g)
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
    float waveformHeight = displayArea.getHeight() / 3.0f - 5.0f;
    
    // Carrier waveform (top)
    auto carrierArea = displayArea.removeFromTop(waveformHeight);
    auto carrierWaveArea = carrierArea.withTrimmedLeft(60); // Leave space for label
    g.setColour(HyperPrismLookAndFeel::Colors::success);
    juce::Path carrierPath;
    for (size_t i = 0; i < carrierWaveform.size(); ++i)
    {
        float x = carrierWaveArea.getX() + (i / float(carrierWaveform.size() - 1)) * carrierWaveArea.getWidth();
        float y = carrierWaveArea.getCentreY() - carrierWaveform[i] * carrierWaveArea.getHeight() * 0.4f;
        
        if (i == 0)
            carrierPath.startNewSubPath(x, y);
        else
            carrierPath.lineTo(x, y);
    }
    g.strokePath(carrierPath, juce::PathStrokeType(2.0f));
    
    // Label (draw in reserved space)
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(10.0f);
    g.drawText("Carrier", carrierArea.withWidth(55), juce::Justification::centredLeft);
    
    displayArea.removeFromTop(5.0f);
    
    // Modulator waveform (middle)
    auto modulatorArea = displayArea.removeFromTop(waveformHeight);
    auto modulatorWaveArea = modulatorArea.withTrimmedLeft(60); // Leave space for label
    g.setColour(HyperPrismLookAndFeel::Colors::warning);
    juce::Path modulatorPath;
    for (size_t i = 0; i < modulatorWaveform.size(); ++i)
    {
        float x = modulatorWaveArea.getX() + (i / float(modulatorWaveform.size() - 1)) * modulatorWaveArea.getWidth();
        float y = modulatorWaveArea.getCentreY() - modulatorWaveform[i] * modulatorWaveArea.getHeight() * 0.4f;
        
        if (i == 0)
            modulatorPath.startNewSubPath(x, y);
        else
            modulatorPath.lineTo(x, y);
    }
    g.strokePath(modulatorPath, juce::PathStrokeType(2.0f));
    
    // Label (draw in reserved space)
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(10.0f);
    g.drawText("Modulator", modulatorArea.withWidth(55), juce::Justification::centredLeft);
    
    displayArea.removeFromTop(5.0f);
    
    // Output waveform (bottom)
    auto outputArea = displayArea.removeFromTop(waveformHeight);
    auto outputWaveArea = outputArea.withTrimmedLeft(60); // Leave space for label
    g.setColour(HyperPrismLookAndFeel::Colors::primary);
    juce::Path outputPath;
    for (size_t i = 0; i < outputWaveform.size(); ++i)
    {
        float x = outputWaveArea.getX() + (i / float(outputWaveform.size() - 1)) * outputWaveArea.getWidth();
        float y = outputWaveArea.getCentreY() - outputWaveform[i] * outputWaveArea.getHeight() * 0.4f;
        
        if (i == 0)
            outputPath.startNewSubPath(x, y);
        else
            outputPath.lineTo(x, y);
    }
    g.strokePath(outputPath, juce::PathStrokeType(2.0f));
    
    // Label (draw in reserved space)
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(10.0f);
    g.drawText("Output", outputArea.withWidth(55), juce::Justification::centredLeft);
}

void RingModulatorMeter::timerCallback()
{
    // Generate sample waveforms for visualization
    // In a real implementation, these would come from the processor
    
    float carrierFreq = 0.02f; // Normalized frequency for display
    float modulatorFreq = 0.005f;
    
    for (size_t i = 0; i < carrierWaveform.size(); ++i)
    {
        float phase = (i / float(carrierWaveform.size())) * juce::MathConstants<float>::twoPi;
        
        // Carrier (higher frequency)
        carrierWaveform[i] = std::sin(phase * 10.0f);
        
        // Modulator (lower frequency)
        modulatorWaveform[i] = std::sin(phase * 2.0f);
        
        // Ring modulated output (product)
        outputWaveform[i] = carrierWaveform[i] * modulatorWaveform[i];
    }
    
    repaint();
}

//==============================================================================
// RingModulatorEditor Implementation
//==============================================================================
RingModulatorEditor::RingModulatorEditor(RingModulatorProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), ringModulatorMeter(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments (using placeholder IDs)
    xParameterIDs.add("carrier_freq");
    yParameterIDs.add("modulator_freq");
    
    // Title (matching AutoPan style)
    titleLabel.setText("HyperPrism Reimagined Ring Modulator", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions("Arial", "Bold", 24.0f)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Setup sliders with consistent style
    setupSlider(carrierFreqSlider, carrierFreqLabel, "Carrier Freq", " Hz");
    setupSlider(modulatorFreqSlider, modulatorFreqLabel, "Modulator Freq", " Hz");
    setupSlider(mixSlider, mixLabel, "Mix", " %");
    
    // Set parameter ranges
    carrierFreqSlider.setRange(1.0, 8000.0, 0.1);
    modulatorFreqSlider.setRange(0.1, 1000.0, 0.1);
    mixSlider.setRange(0.0, 100.0, 0.1);
    
    // Setup ComboBoxes
    setupComboBox(carrierWaveformBox, carrierWaveformLabel, "Carrier Wave");
    setupComboBox(modulatorWaveformBox, modulatorWaveformLabel, "Modulator Wave");
    
    // Add waveform options
    carrierWaveformBox.addItem("Sine", 1);
    carrierWaveformBox.addItem("Triangle", 2);
    carrierWaveformBox.addItem("Square", 3);
    carrierWaveformBox.addItem("Sawtooth", 4);
    
    modulatorWaveformBox.addItem("Sine", 1);
    modulatorWaveformBox.addItem("Triangle", 2);
    modulatorWaveformBox.addItem("Square", 3);
    modulatorWaveformBox.addItem("Sawtooth", 4);
    
    // Set up right-click handlers for parameter assignment
    carrierFreqLabel.onClick = [this]() { showParameterMenu(&carrierFreqLabel, "carrier_freq"); };
    modulatorFreqLabel.onClick = [this]() { showParameterMenu(&modulatorFreqLabel, "modulator_freq"); };
    carrierWaveformLabel.onClick = [this]() { showParameterMenu(&carrierWaveformLabel, "carrier_waveform"); };
    modulatorWaveformLabel.onClick = [this]() { showParameterMenu(&modulatorWaveformLabel, "modulator_waveform"); };
    mixLabel.onClick = [this]() { showParameterMenu(&mixLabel, "mix"); };
    
    // Bypass button (top right like AutoPan)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::red);
    bypassButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
    addAndMakeVisible(bypassButton);
    
    // Create attachments (using placeholder parameter IDs)
    auto& apvts = audioProcessor.getAPVTS();
    carrierFreqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "carrier_freq", carrierFreqSlider);
    modulatorFreqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "modulator_freq", modulatorFreqSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "mix", mixSlider);
    carrierWaveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, "carrier_waveform", carrierWaveformBox);
    modulatorWaveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, "modulator_waveform", modulatorWaveformBox);
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);
    xyPadLabel.setText("Carrier Freq / Modulator Freq", juce::dontSendNotification);
    xyPadLabel.setJustificationType(juce::Justification::centred);
    xyPadLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    
    // Add ring modulator meter
    addAndMakeVisible(ringModulatorMeter);
    ringModulatorMeterLabel.setText("Waveform Display", juce::dontSendNotification);
    ringModulatorMeterLabel.setJustificationType(juce::Justification::centred);
    ringModulatorMeterLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(ringModulatorMeterLabel);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    carrierFreqSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    modulatorFreqSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    mixSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    setSize(650, 600);
}

RingModulatorEditor::~RingModulatorEditor()
{
    setLookAndFeel(nullptr);
}

void RingModulatorEditor::paint(juce::Graphics& g)
{
    // Dark background matching AutoPan
    g.fillAll(HyperPrismLookAndFeel::Colors::background);
}

void RingModulatorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(40));
    
    // Bypass button (top right)
    bypassButton.setBounds(bounds.getWidth() - 100, 10, 80, 30);
    
    bounds.reduce(20, 10);
    
    // Optimized layout for 650x600 - single row with all 3 sliders
    auto sliderWidth = 80;
    auto spacing = 15;
    
    // Single row with all 3 sliders
    auto controlsRow = bounds.removeFromTop(130);
    auto totalControlsWidth = sliderWidth * 3 + spacing * 2;
    auto controlsStartX = (bounds.getWidth() - totalControlsWidth) / 2;
    controlsRow.removeFromLeft(controlsStartX);
    
    // All sliders in one row: Carrier Freq, Modulator Freq, Mix
    carrierFreqSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    carrierFreqLabel.setBounds(carrierFreqSlider.getX(), carrierFreqSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    modulatorFreqSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    modulatorFreqLabel.setBounds(modulatorFreqSlider.getX(), modulatorFreqSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    mixSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    mixLabel.setBounds(mixSlider.getX(), mixSlider.getBottom(), sliderWidth, 20);
    
    bounds.removeFromTop(15);
    
    // ComboBoxes row (tighter spacing)
    auto comboRow = bounds.removeFromTop(60);
    auto comboWidth = 120;
    auto totalComboWidth = comboWidth * 2 + spacing;
    auto comboStartX = (bounds.getWidth() - totalComboWidth) / 2;
    comboRow.removeFromLeft(comboStartX);
    
    carrierWaveformLabel.setBounds(comboRow.getX() + comboStartX, comboRow.getY() + 5, comboWidth, 20);
    carrierWaveformBox.setBounds(comboRow.removeFromLeft(comboWidth).withHeight(25).withY(comboRow.getY() + 25));
    comboRow.removeFromLeft(spacing);
    
    modulatorWaveformLabel.setBounds(comboRow.getX(), comboRow.getY() + 5, comboWidth, 20);
    modulatorWaveformBox.setBounds(comboRow.removeFromLeft(comboWidth).withHeight(25).withY(comboRow.getY() + 25));
    
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
    
    // Ring modulator meter on right (matching height)
    auto meterBounds = bottomArea.withX(xyPadBounds.getRight() + 20).withWidth(meterSize).withHeight(panelHeight);
    ringModulatorMeter.setBounds(meterBounds);
    
    // Align labels at the same Y position
    auto labelY = xyPadBounds.getBottom() + 5;
    xyPadLabel.setBounds(xyPad.getX(), labelY, xyPadWidth, 20);
    ringModulatorMeterLabel.setBounds(ringModulatorMeter.getX(), labelY, meterSize, 20);
}

void RingModulatorEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void RingModulatorEditor::setupComboBox(juce::ComboBox& comboBox, ParameterLabel& label, 
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

void RingModulatorEditor::updateParameterColors()
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
    
    updateLabelColor(carrierFreqLabel, "carrier_freq");
    updateLabelColor(modulatorFreqLabel, "modulator_freq");
    updateLabelColor(carrierWaveformLabel, "carrier_waveform");
    updateLabelColor(modulatorWaveformLabel, "modulator_waveform");
    updateLabelColor(mixLabel, "mix");
}

void RingModulatorEditor::updateXYPadFromParameters()
{
    // For multiple parameters, use the average of their normalized values
    float xValue = 0.0f;
    float yValue = 0.0f;
    
    auto& apvts = audioProcessor.getAPVTS();
    
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

void RingModulatorEditor::updateParametersFromXYPad(float x, float y)
{
    auto& apvts = audioProcessor.getAPVTS();
    
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

void RingModulatorEditor::showParameterMenu(juce::Label* label, const juce::String& parameterID)
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
                    xParameterIDs.add("carrier_freq");
                    
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
                    yParameterIDs.add("modulator_freq");
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add("carrier_freq");
                yParameterIDs.add("modulator_freq");
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void RingModulatorEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == "carrier_freq") return "Carrier Freq";
        if (paramID == "modulator_freq") return "Modulator Freq";
        if (paramID == "carrier_waveform") return "Carrier Wave";
        if (paramID == "modulator_waveform") return "Modulator Wave";
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