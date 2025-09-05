//==============================================================================
// HyperPrism Reimagined - Sonic Decimator Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "SonicDecimatorEditor.h"

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
// DecimationMeter Implementation
//==============================================================================
DecimationMeter::DecimationMeter(SonicDecimatorProcessor& p) 
    : processor(p)
{
    startTimerHz(30); // 30 FPS update rate
}

DecimationMeter::~DecimationMeter()
{
    stopTimer();
}

void DecimationMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(HyperPrismLookAndFeel::Colors::background);
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Border
    g.setColour(HyperPrismLookAndFeel::Colors::outline);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    
    // Create meter area
    auto meterArea = bounds.reduced(10.0f);
    float meterWidth = meterArea.getWidth();
    float meterHeight = meterArea.getHeight() * 0.8f; // Leave space for labels
    
    // Divide into four sections
    float sectionWidth = meterWidth / 4.0f;
    
    // Input Level (green)
    auto inputMeterArea = juce::Rectangle<float>(meterArea.getX(), meterArea.getY(), sectionWidth - 2, meterHeight);
    if (inputLevel > 0.001f)
    {
        float levelHeight = inputMeterArea.getHeight() * inputLevel;
        auto levelRect = juce::Rectangle<float>(inputMeterArea.getX(), 
                                               inputMeterArea.getBottom() - levelHeight, 
                                               inputMeterArea.getWidth(), 
                                               levelHeight);
        g.setColour(HyperPrismLookAndFeel::Colors::success);
        g.fillRoundedRectangle(levelRect, 2.0f);
    }
    
    // Output Level (cyan)
    auto outputMeterArea = juce::Rectangle<float>(meterArea.getX() + sectionWidth, meterArea.getY(), sectionWidth - 2, meterHeight);
    if (outputLevel > 0.001f)
    {
        float levelHeight = outputMeterArea.getHeight() * outputLevel;
        auto levelRect = juce::Rectangle<float>(outputMeterArea.getX(), 
                                               outputMeterArea.getBottom() - levelHeight, 
                                               outputMeterArea.getWidth(), 
                                               levelHeight);
        g.setColour(HyperPrismLookAndFeel::Colors::primary);
        g.fillRoundedRectangle(levelRect, 2.0f);
    }
    
    // Bit Reduction (orange/yellow)
    auto bitMeterArea = juce::Rectangle<float>(meterArea.getX() + sectionWidth * 2, meterArea.getY(), sectionWidth - 2, meterHeight);
    if (bitReduction > 0.001f)
    {
        float reductionHeight = bitMeterArea.getHeight() * bitReduction;
        auto reductionRect = juce::Rectangle<float>(bitMeterArea.getX(), 
                                                   bitMeterArea.getY(), 
                                                   bitMeterArea.getWidth(), 
                                                   reductionHeight);
        g.setColour(HyperPrismLookAndFeel::Colors::warning);
        g.fillRoundedRectangle(reductionRect, 2.0f);
    }
    
    // Sample Rate Reduction (red)
    auto sampleMeterArea = juce::Rectangle<float>(meterArea.getX() + sectionWidth * 3, meterArea.getY(), sectionWidth - 2, meterHeight);
    if (sampleReduction > 0.001f)
    {
        float reductionHeight = sampleMeterArea.getHeight() * sampleReduction;
        auto reductionRect = juce::Rectangle<float>(sampleMeterArea.getX(), 
                                                   sampleMeterArea.getY(), 
                                                   sampleMeterArea.getWidth(), 
                                                   reductionHeight);
        g.setColour(HyperPrismLookAndFeel::Colors::error);
        g.fillRoundedRectangle(reductionRect, 2.0f);
    }
    
    // Draw scale lines
    g.setColour(HyperPrismLookAndFeel::Colors::outlineVariant.withAlpha(0.3f));
    for (int i = 1; i < 4; ++i)
    {
        float y = meterArea.getY() + (meterHeight * i / 4.0f);
        g.drawHorizontalLine(static_cast<int>(y), meterArea.getX(), meterArea.getRight());
    }
    
    // Draw section dividers
    for (int i = 1; i < 4; ++i)
    {
        float x = meterArea.getX() + (sectionWidth * i);
        g.drawVerticalLine(static_cast<int>(x), meterArea.getY(), meterArea.getY() + meterHeight);
    }
    
    // Labels
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(9.0f);
    
    auto labelY = meterArea.getY() + meterHeight + 5;
    auto labelArea = juce::Rectangle<float>(meterArea.getX(), labelY, sectionWidth, 15);
    g.drawText("IN", labelArea, juce::Justification::centred);
    
    labelArea.translate(sectionWidth, 0);
    g.drawText("OUT", labelArea, juce::Justification::centred);
    
    labelArea.translate(sectionWidth, 0);
    g.drawText("BITS", labelArea, juce::Justification::centred);
    
    labelArea.translate(sectionWidth, 0);
    g.drawText("RATE", labelArea, juce::Justification::centred);
    
    // Percentage values for reduction meters
    g.setFont(8.0f);
    auto percentY = labelY + 15;
    
    // Bit reduction percentage
    auto bitPercArea = juce::Rectangle<float>(meterArea.getX() + sectionWidth * 2, percentY, sectionWidth, 10);
    g.drawText(juce::String(static_cast<int>(bitReduction * 100)) + "%", bitPercArea, juce::Justification::centred);
    
    // Sample rate reduction percentage
    auto srPercArea = juce::Rectangle<float>(meterArea.getX() + sectionWidth * 3, percentY, sectionWidth, 10);
    g.drawText(juce::String(static_cast<int>(sampleReduction * 100)) + "%", srPercArea, juce::Justification::centred);
}

void DecimationMeter::timerCallback()
{
    float newInputLevel = processor.getInputLevel();
    float newOutputLevel = processor.getOutputLevel();
    float newBitReduction = processor.getBitReduction();
    float newSampleReduction = processor.getSampleReduction();
    
    // Smooth the level changes
    const float smoothing = 0.7f;
    inputLevel = inputLevel * smoothing + newInputLevel * (1.0f - smoothing);
    outputLevel = outputLevel * smoothing + newOutputLevel * (1.0f - smoothing);
    
    // Reduction values update immediately for responsiveness
    bitReduction = newBitReduction;
    sampleReduction = newSampleReduction;
    
    repaint();
}

//==============================================================================
// SonicDecimatorEditor Implementation
//==============================================================================
SonicDecimatorEditor::SonicDecimatorEditor(SonicDecimatorProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), decimationMeter(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add(SonicDecimatorProcessor::BIT_DEPTH_ID);
    yParameterIDs.add(SonicDecimatorProcessor::SAMPLE_RATE_ID);
    
    // Title (matching AutoPan style)
    titleLabel.setText("HyperPrism Reimagined Sonic Decimator", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions("Arial", "Bold", 24.0f)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Setup sliders with consistent style
    setupSlider(bitDepthSlider, bitDepthLabel, "Bit Depth", " bits");
    setupSlider(sampleRateSlider, sampleRateLabel, "Sample Rate", " Hz");
    setupSlider(mixSlider, mixLabel, "Mix", "");
    setupSlider(outputLevelSlider, outputLevelLabel, "Output Level", " dB");
    
    // Set parameter ranges (example ranges - adjust based on processor)
    bitDepthSlider.setRange(1.0, 24.0, 1.0);
    sampleRateSlider.setRange(500.0, 48000.0, 1.0);
    mixSlider.setRange(0.0, 100.0, 0.1);
    outputLevelSlider.setRange(-24.0, 12.0, 0.1);
    
    // Setup toggle buttons
    setupToggleButton(antiAliasButton, antiAliasLabel, "Anti-Alias");
    setupToggleButton(ditherButton, ditherLabel, "Dither");
    
    // Set up right-click handlers for parameter assignment
    bitDepthLabel.onClick = [this]() { showParameterMenu(&bitDepthLabel, SonicDecimatorProcessor::BIT_DEPTH_ID); };
    sampleRateLabel.onClick = [this]() { showParameterMenu(&sampleRateLabel, SonicDecimatorProcessor::SAMPLE_RATE_ID); };
    mixLabel.onClick = [this]() { showParameterMenu(&mixLabel, SonicDecimatorProcessor::MIX_ID); };
    outputLevelLabel.onClick = [this]() { showParameterMenu(&outputLevelLabel, SonicDecimatorProcessor::OUTPUT_LEVEL_ID); };
    antiAliasLabel.onClick = [this]() { showParameterMenu(&antiAliasLabel, SonicDecimatorProcessor::ANTI_ALIAS_ID); };
    ditherLabel.onClick = [this]() { showParameterMenu(&ditherLabel, SonicDecimatorProcessor::DITHER_ID); };
    
    // Bypass button (top right like AutoPan)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::red);
    bypassButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
    addAndMakeVisible(bypassButton);
    
    // Create attachments
    auto& apvts = audioProcessor.getValueTreeState();
    bitDepthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, SonicDecimatorProcessor::BIT_DEPTH_ID, bitDepthSlider);
    sampleRateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, SonicDecimatorProcessor::SAMPLE_RATE_ID, sampleRateSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, SonicDecimatorProcessor::MIX_ID, mixSlider);
    outputLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, SonicDecimatorProcessor::OUTPUT_LEVEL_ID, outputLevelSlider);
    antiAliasAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, SonicDecimatorProcessor::ANTI_ALIAS_ID, antiAliasButton);
    ditherAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, SonicDecimatorProcessor::DITHER_ID, ditherButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, SonicDecimatorProcessor::BYPASS_ID, bypassButton);
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);
    xyPadLabel.setText("Bit Depth / Sample Rate", juce::dontSendNotification);
    xyPadLabel.setJustificationType(juce::Justification::centred);
    xyPadLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    
    // Add decimation meter
    addAndMakeVisible(decimationMeter);
    decimationMeterLabel.setText("Decimation Analysis", juce::dontSendNotification);
    decimationMeterLabel.setJustificationType(juce::Justification::centred);
    decimationMeterLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(decimationMeterLabel);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    bitDepthSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    sampleRateSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    mixSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    outputLevelSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    setSize(650, 600);
}

SonicDecimatorEditor::~SonicDecimatorEditor()
{
    setLookAndFeel(nullptr);
}

void SonicDecimatorEditor::paint(juce::Graphics& g)
{
    // Dark background matching AutoPan
    g.fillAll(HyperPrismLookAndFeel::Colors::background);
}

void SonicDecimatorEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(40));
    
    // Bypass button (top right)
    bypassButton.setBounds(bounds.getWidth() - 100, 10, 80, 30);
    
    bounds.reduce(20, 10);
    
    // Optimized layout for 650x600 - single row with all 6 controls (4 sliders + 2 toggles)
    auto sliderWidth = 70;
    auto spacing = 12;
    auto toggleWidth = 70;
    
    // Single row with all controls
    auto controlsRow = bounds.removeFromTop(130);
    auto totalControlsWidth = sliderWidth * 4 + toggleWidth * 2 + spacing * 5;
    auto controlsStartX = (bounds.getWidth() - totalControlsWidth) / 2;
    controlsRow.removeFromLeft(controlsStartX);
    
    // All sliders in one row: Bit Depth, Sample Rate, Mix, Output Level
    bitDepthSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    bitDepthLabel.setBounds(bitDepthSlider.getX(), bitDepthSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    sampleRateSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    sampleRateLabel.setBounds(sampleRateSlider.getX(), sampleRateSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    mixSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    mixLabel.setBounds(mixSlider.getX(), mixSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    outputLevelSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    outputLevelLabel.setBounds(outputLevelSlider.getX(), outputLevelSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    // Toggle buttons at the end of the row
    antiAliasButton.setBounds(controlsRow.removeFromLeft(toggleWidth).withHeight(30).withY(controlsRow.getY() + 30));
    antiAliasLabel.setBounds(antiAliasButton.getX(), antiAliasButton.getBottom(), toggleWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    ditherButton.setBounds(controlsRow.removeFromLeft(toggleWidth).withHeight(30).withY(controlsRow.getY() + 30));
    ditherLabel.setBounds(ditherButton.getX(), ditherButton.getBottom(), toggleWidth, 20);
    
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
    
    // Decimation meter on right (matching height)
    auto meterBounds = bottomArea.withX(xyPadBounds.getRight() + 20).withWidth(meterSize).withHeight(panelHeight);
    decimationMeter.setBounds(meterBounds);
    
    // Align labels at the same Y position
    auto labelY = xyPadBounds.getBottom() + 5;
    xyPadLabel.setBounds(xyPad.getX(), labelY, xyPadWidth, 20);
    decimationMeterLabel.setBounds(decimationMeter.getX(), labelY, meterSize, 20);
}

void SonicDecimatorEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void SonicDecimatorEditor::setupToggleButton(juce::ToggleButton& button, ParameterLabel& label, 
                                            const juce::String& text)
{
    button.setButtonText(text);
    button.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    button.setColour(juce::ToggleButton::tickColourId, juce::Colours::cyan);
    button.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
    addAndMakeVisible(button);
    
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(label);
}

void SonicDecimatorEditor::updateParameterColors()
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
    
    updateLabelColor(bitDepthLabel, SonicDecimatorProcessor::BIT_DEPTH_ID);
    updateLabelColor(sampleRateLabel, SonicDecimatorProcessor::SAMPLE_RATE_ID);
    updateLabelColor(mixLabel, SonicDecimatorProcessor::MIX_ID);
    updateLabelColor(outputLevelLabel, SonicDecimatorProcessor::OUTPUT_LEVEL_ID);
    updateLabelColor(antiAliasLabel, SonicDecimatorProcessor::ANTI_ALIAS_ID);
    updateLabelColor(ditherLabel, SonicDecimatorProcessor::DITHER_ID);
}

void SonicDecimatorEditor::updateXYPadFromParameters()
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

void SonicDecimatorEditor::updateParametersFromXYPad(float x, float y)
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

void SonicDecimatorEditor::showParameterMenu(juce::Label* label, const juce::String& parameterID)
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
                    xParameterIDs.add(SonicDecimatorProcessor::BIT_DEPTH_ID);
                    
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
                    yParameterIDs.add(SonicDecimatorProcessor::SAMPLE_RATE_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add(SonicDecimatorProcessor::BIT_DEPTH_ID);
                yParameterIDs.add(SonicDecimatorProcessor::SAMPLE_RATE_ID);
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void SonicDecimatorEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == SonicDecimatorProcessor::BIT_DEPTH_ID) return "Bit Depth";
        if (paramID == SonicDecimatorProcessor::SAMPLE_RATE_ID) return "Sample Rate";
        if (paramID == SonicDecimatorProcessor::MIX_ID) return "Mix";
        if (paramID == SonicDecimatorProcessor::OUTPUT_LEVEL_ID) return "Output Level";
        if (paramID == SonicDecimatorProcessor::ANTI_ALIAS_ID) return "Anti-Alias";
        if (paramID == SonicDecimatorProcessor::DITHER_ID) return "Dither";
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