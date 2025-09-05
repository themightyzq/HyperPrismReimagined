//==============================================================================
// HyperPrism Reimagined - Tube/Tape Saturation Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "TubeTapeSaturationEditor.h"

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
// SaturationMeter Implementation
//==============================================================================
SaturationMeter::SaturationMeter(TubeTapeSaturationProcessor& p) 
    : processor(p)
{
    startTimerHz(30); // 30 FPS update rate
    
    // Initialize harmonic spectrum array
    harmonicSpectrum.resize(8, 0.0f); // Show up to 8th harmonic
}

SaturationMeter::~SaturationMeter()
{
    stopTimer();
}

void SaturationMeter::paint(juce::Graphics& g)
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
    
    // Top section - Input/Output level meters
    auto levelArea = displayArea.removeFromTop(displayArea.getHeight() * 0.3f);
    auto inputMeterArea = levelArea.removeFromLeft(levelArea.getWidth() / 2);
    inputMeterArea.removeFromRight(5); // spacing
    auto outputMeterArea = levelArea;
    outputMeterArea.removeFromLeft(5); // spacing
    
    // Input level meter (green)
    if (inputLevel > 0.001f)
    {
        float meterHeight = inputMeterArea.getHeight() * inputLevel;
        auto meterRect = juce::Rectangle<float>(inputMeterArea.getX(), 
                                               inputMeterArea.getBottom() - meterHeight, 
                                               inputMeterArea.getWidth(), 
                                               meterHeight);
        g.setColour(HyperPrismLookAndFeel::Colors::success);
        g.fillRoundedRectangle(meterRect, 2.0f);
    }
    
    // Output level meter (cyan)
    if (outputLevel > 0.001f)
    {
        float meterHeight = outputMeterArea.getHeight() * outputLevel;
        auto meterRect = juce::Rectangle<float>(outputMeterArea.getX(), 
                                               outputMeterArea.getBottom() - meterHeight, 
                                               outputMeterArea.getWidth(), 
                                               meterHeight);
        g.setColour(HyperPrismLookAndFeel::Colors::primary);
        g.fillRoundedRectangle(meterRect, 2.0f);
    }
    
    // Level meter labels
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(9.0f);
    auto labelY = levelArea.getBottom() + 2;
    g.drawText("IN", inputMeterArea.withY(labelY).withHeight(12), juce::Justification::centred);
    g.drawText("OUT", outputMeterArea.withY(labelY).withHeight(12), juce::Justification::centred);
    
    displayArea.removeFromTop(20); // spacing for labels
    
    // Middle section - Harmonic content visualization
    auto harmonicArea = displayArea.removeFromTop(displayArea.getHeight() * 0.5f);
    
    // Draw harmonic bars
    if (!harmonicSpectrum.empty())
    {
        float barWidth = harmonicArea.getWidth() / harmonicSpectrum.size();
        
        for (size_t i = 0; i < harmonicSpectrum.size(); ++i)
        {
            float barHeight = harmonicArea.getHeight() * harmonicSpectrum[i];
            auto barRect = juce::Rectangle<float>(harmonicArea.getX() + i * barWidth + 2, 
                                                 harmonicArea.getBottom() - barHeight, 
                                                 barWidth - 4, 
                                                 barHeight);
            
            // Color based on harmonic number - even harmonics warmer, odd cooler
            if (i % 2 == 0)
                g.setColour(HyperPrismLookAndFeel::Colors::warning); // Orange for even harmonics
            else
                g.setColour(HyperPrismLookAndFeel::Colors::primary); // Cyan for odd harmonics
                
            g.fillRoundedRectangle(barRect, 1.0f);
        }
    }
    
    // Harmonic content label
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(8.0f);
    auto harmonicLabelY = harmonicArea.getBottom() + 2;
    g.drawText("Harmonics", harmonicArea.withY(harmonicLabelY).withHeight(12), juce::Justification::centred);
    
    displayArea.removeFromTop(15); // spacing for label
    
    // Bottom section - Saturation type and drive display
    auto infoArea = displayArea;
    
    // Saturation type indicator
    juce::String typeText;
    juce::Colour typeColor = HyperPrismLookAndFeel::Colors::primary;
    switch (saturationType)
    {
        case 0: // Tube
            typeText = "TUBE";
            typeColor = HyperPrismLookAndFeel::Colors::warning; // Orange
            break;
        case 1: // Tape
            typeText = "TAPE";
            typeColor = HyperPrismLookAndFeel::Colors::error; // Red
            break;
        case 2: // Transformer
            typeText = "XFRM";
            typeColor = HyperPrismLookAndFeel::Colors::success; // Green
            break;
        default:
            typeText = "---";
            break;
    }
    
    auto typeArea = infoArea.removeFromTop(infoArea.getHeight() / 2);
    g.setColour(typeColor);
    g.setFont(juce::Font(juce::FontOptions("Arial", "Bold", 12.0f)));
    g.drawText(typeText, typeArea, juce::Justification::centred);
    
    // Drive level bar
    auto driveArea = infoArea;
    if (drive > 0.001f)
    {
        float driveWidth = driveArea.getWidth() * drive;
        auto driveRect = juce::Rectangle<float>(driveArea.getX(), 
                                               driveArea.getY() + 5, 
                                               driveWidth, 
                                               driveArea.getHeight() - 10);
        g.setColour(typeColor.withAlpha(0.7f));
        g.fillRoundedRectangle(driveRect, 2.0f);
    }
    
    // Drive outline and label
    g.setColour(HyperPrismLookAndFeel::Colors::outlineVariant);
    g.drawRoundedRectangle(driveArea.reduced(0, 5), 2.0f, 1.0f);
    
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(8.0f);
    g.drawText("Drive: " + juce::String(static_cast<int>(drive * 100)) + "%", 
               driveArea.withY(driveArea.getBottom() + 2).withHeight(10), 
               juce::Justification::centred);
}

void SaturationMeter::timerCallback()
{
    // Get current harmonic content from processor
    harmonicContent = processor.getHarmonicContent();
    
    // Get actual parameters from value tree state
    auto& vts = processor.getValueTreeState();
    
    // Get saturation type (ComboBox uses 1-based indexing, we need 0-based)
    if (auto* typeParam = vts.getRawParameterValue(TubeTapeSaturationProcessor::TYPE_ID))
    {
        saturationType = static_cast<int>(typeParam->load()) - 1;
    }
    
    // Get drive parameter (0-100 range, convert to 0-1)
    if (auto* driveParam = vts.getRawParameterValue(TubeTapeSaturationProcessor::DRIVE_ID))
    {
        drive = driveParam->load() / 100.0f;
    }
    
    // Get actual input/output levels from processor
    inputLevel = processor.getInputLevel();
    outputLevel = processor.getOutputLevel();
    
    // Generate harmonic spectrum based on actual parameters
    for (size_t i = 0; i < harmonicSpectrum.size(); ++i)
    {
        // Higher harmonics are generally weaker
        float baseLevel = harmonicContent / (i + 1);
        
        // Add variation based on saturation type
        float typeInfluence = 0.0f;
        switch (saturationType)
        {
            case 0: // Tube - emphasizes even harmonics
                typeInfluence = (i % 2 == 0) ? 0.2f : 0.0f;
                break;
            case 1: // Tape - more complex harmonic structure
                typeInfluence = 0.1f * std::sin(i * 0.5f);
                break;
            case 2: // Transformer - emphasizes odd harmonics
                typeInfluence = (i % 2 == 1) ? 0.15f : 0.0f;
                break;
        }
        
        // Apply drive influence
        float driveInfluence = drive * 0.3f * (5.0f - i) / 5.0f;
        
        harmonicSpectrum[i] = juce::jlimit(0.0f, 1.0f, baseLevel + typeInfluence + driveInfluence);
    }
    
    repaint();
}

//==============================================================================
// TubeTapeSaturationEditor Implementation
//==============================================================================
TubeTapeSaturationEditor::TubeTapeSaturationEditor(TubeTapeSaturationProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), saturationMeter(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add(TubeTapeSaturationProcessor::DRIVE_ID);
    yParameterIDs.add(TubeTapeSaturationProcessor::WARMTH_ID);
    
    // Title (matching AutoPan style)
    titleLabel.setText("HyperPrism Reimagined Tube/Tape Saturation", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions("Arial", "Bold", 24.0f)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Setup sliders with consistent style
    setupSlider(driveSlider, driveLabel, "Drive", "");
    setupSlider(warmthSlider, warmthLabel, "Warmth", "");
    setupSlider(brightnessSlider, brightnessLabel, "Brightness", "");
    setupSlider(outputLevelSlider, outputLevelLabel, "Output Level", " dB");
    
    // Set parameter ranges (example ranges - adjust based on processor)
    driveSlider.setRange(0.0, 100.0, 0.1);
    warmthSlider.setRange(0.0, 100.0, 0.1);
    brightnessSlider.setRange(0.0, 100.0, 0.1);
    outputLevelSlider.setRange(-24.0, 12.0, 0.1);
    
    // Setup ComboBox
    setupComboBox(typeComboBox, typeLabel, "Type");
    
    // Add saturation type options
    typeComboBox.addItem("Tube", 1);
    typeComboBox.addItem("Tape", 2);
    typeComboBox.addItem("Transformer", 3);
    
    // Set up right-click handlers for parameter assignment
    driveLabel.onClick = [this]() { showParameterMenu(&driveLabel, TubeTapeSaturationProcessor::DRIVE_ID); };
    warmthLabel.onClick = [this]() { showParameterMenu(&warmthLabel, TubeTapeSaturationProcessor::WARMTH_ID); };
    brightnessLabel.onClick = [this]() { showParameterMenu(&brightnessLabel, TubeTapeSaturationProcessor::BRIGHTNESS_ID); };
    outputLevelLabel.onClick = [this]() { showParameterMenu(&outputLevelLabel, TubeTapeSaturationProcessor::OUTPUT_LEVEL_ID); };
    typeLabel.onClick = [this]() { showParameterMenu(&typeLabel, TubeTapeSaturationProcessor::TYPE_ID); };
    
    // Bypass button (top right like AutoPan)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::red);
    bypassButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
    addAndMakeVisible(bypassButton);
    
    // Create attachments
    auto& apvts = audioProcessor.getValueTreeState();
    driveAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, TubeTapeSaturationProcessor::DRIVE_ID, driveSlider);
    warmthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, TubeTapeSaturationProcessor::WARMTH_ID, warmthSlider);
    brightnessAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, TubeTapeSaturationProcessor::BRIGHTNESS_ID, brightnessSlider);
    outputLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, TubeTapeSaturationProcessor::OUTPUT_LEVEL_ID, outputLevelSlider);
    typeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, TubeTapeSaturationProcessor::TYPE_ID, typeComboBox);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, TubeTapeSaturationProcessor::BYPASS_ID, bypassButton);
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);
    xyPadLabel.setText("Drive / Warmth", juce::dontSendNotification);
    xyPadLabel.setJustificationType(juce::Justification::centred);
    xyPadLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    
    // Add saturation meter
    addAndMakeVisible(saturationMeter);
    saturationMeterLabel.setText("Saturation Analysis", juce::dontSendNotification);
    saturationMeterLabel.setJustificationType(juce::Justification::centred);
    saturationMeterLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(saturationMeterLabel);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    driveSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    warmthSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    brightnessSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    outputLevelSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    setSize(650, 600);
}

TubeTapeSaturationEditor::~TubeTapeSaturationEditor()
{
    setLookAndFeel(nullptr);
}

void TubeTapeSaturationEditor::paint(juce::Graphics& g)
{
    // Dark background matching AutoPan
    g.fillAll(HyperPrismLookAndFeel::Colors::background);
}

void TubeTapeSaturationEditor::resized()
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
    
    // All controls in one row: Drive, Warmth, Brightness, Output Level, Type
    driveSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    driveLabel.setBounds(driveSlider.getX(), driveSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    warmthSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    warmthLabel.setBounds(warmthSlider.getX(), warmthSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    brightnessSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    brightnessLabel.setBounds(brightnessSlider.getX(), brightnessSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    outputLevelSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    outputLevelLabel.setBounds(outputLevelSlider.getX(), outputLevelSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    // Type ComboBox at the end of the row
    typeComboBox.setBounds(controlsRow.removeFromLeft(comboWidth).withHeight(25).withY(controlsRow.getY() + 40));
    typeLabel.setBounds(typeComboBox.getX(), typeComboBox.getBottom(), comboWidth, 20);
    
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
    
    // Saturation meter on right (matching height)
    auto meterBounds = bottomArea.withX(xyPadBounds.getRight() + 20).withWidth(meterSize).withHeight(panelHeight);
    saturationMeter.setBounds(meterBounds);
    
    // Align labels at the same Y position
    auto labelY = xyPadBounds.getBottom() + 5;
    xyPadLabel.setBounds(xyPad.getX(), labelY, xyPadWidth, 20);
    saturationMeterLabel.setBounds(saturationMeter.getX(), labelY, meterSize, 20);
}

void TubeTapeSaturationEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void TubeTapeSaturationEditor::setupComboBox(juce::ComboBox& comboBox, ParameterLabel& label, 
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

void TubeTapeSaturationEditor::updateParameterColors()
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
    
    updateLabelColor(driveLabel, TubeTapeSaturationProcessor::DRIVE_ID);
    updateLabelColor(warmthLabel, TubeTapeSaturationProcessor::WARMTH_ID);
    updateLabelColor(brightnessLabel, TubeTapeSaturationProcessor::BRIGHTNESS_ID);
    updateLabelColor(outputLevelLabel, TubeTapeSaturationProcessor::OUTPUT_LEVEL_ID);
    updateLabelColor(typeLabel, TubeTapeSaturationProcessor::TYPE_ID);
}

void TubeTapeSaturationEditor::updateXYPadFromParameters()
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

void TubeTapeSaturationEditor::updateParametersFromXYPad(float x, float y)
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

void TubeTapeSaturationEditor::showParameterMenu(juce::Label* label, const juce::String& parameterID)
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
                    xParameterIDs.add(TubeTapeSaturationProcessor::DRIVE_ID);
                    
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
                    yParameterIDs.add(TubeTapeSaturationProcessor::WARMTH_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add(TubeTapeSaturationProcessor::DRIVE_ID);
                yParameterIDs.add(TubeTapeSaturationProcessor::WARMTH_ID);
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void TubeTapeSaturationEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == TubeTapeSaturationProcessor::DRIVE_ID) return "Drive";
        if (paramID == TubeTapeSaturationProcessor::WARMTH_ID) return "Warmth";
        if (paramID == TubeTapeSaturationProcessor::BRIGHTNESS_ID) return "Brightness";
        if (paramID == TubeTapeSaturationProcessor::OUTPUT_LEVEL_ID) return "Output Level";
        if (paramID == TubeTapeSaturationProcessor::TYPE_ID) return "Type";
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