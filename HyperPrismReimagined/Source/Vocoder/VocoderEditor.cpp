//==============================================================================
// HyperPrism Reimagined - Vocoder Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "VocoderEditor.h"

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
// VocoderMeter Implementation
//==============================================================================
VocoderMeter::VocoderMeter(VocoderProcessor& p) 
    : processor(p)
{
    startTimerHz(30); // 30 FPS update rate
    
    // Initialize band levels array (max 16 bands)
    smoothedBandLevels.resize(16, 0.0f);
}

VocoderMeter::~VocoderMeter()
{
    stopTimer();
}

void VocoderMeter::paint(juce::Graphics& g)
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
    
    // Top section - Carrier/Modulator/Output levels
    auto levelArea = displayArea.removeFromTop(displayArea.getHeight() * 0.25f);
    auto carrierArea = levelArea.removeFromLeft(levelArea.getWidth() / 3);
    auto modulatorArea = levelArea.removeFromLeft(levelArea.getWidth() / 2);
    auto outputArea = levelArea;
    
    // Carrier level (orange)
    if (carrierLevel > 0.001f)
    {
        float meterHeight = carrierArea.getHeight() * carrierLevel;
        auto meterRect = juce::Rectangle<float>(carrierArea.getX() + 2, 
                                               carrierArea.getBottom() - meterHeight, 
                                               carrierArea.getWidth() - 4, 
                                               meterHeight);
        g.setColour(HyperPrismLookAndFeel::Colors::warning);
        g.fillRoundedRectangle(meterRect, 2.0f);
    }
    
    // Modulator level (cyan)
    if (modulatorLevel > 0.001f)
    {
        float meterHeight = modulatorArea.getHeight() * modulatorLevel;
        auto meterRect = juce::Rectangle<float>(modulatorArea.getX() + 2, 
                                               modulatorArea.getBottom() - meterHeight, 
                                               modulatorArea.getWidth() - 4, 
                                               meterHeight);
        g.setColour(HyperPrismLookAndFeel::Colors::primary);
        g.fillRoundedRectangle(meterRect, 2.0f);
    }
    
    // Output level (green)
    if (outputLevel > 0.001f)
    {
        float meterHeight = outputArea.getHeight() * outputLevel;
        auto meterRect = juce::Rectangle<float>(outputArea.getX() + 2, 
                                               outputArea.getBottom() - meterHeight, 
                                               outputArea.getWidth() - 4, 
                                               meterHeight);
        g.setColour(HyperPrismLookAndFeel::Colors::success);
        g.fillRoundedRectangle(meterRect, 2.0f);
    }
    
    // Level meter labels
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(8.0f);
    auto labelY = levelArea.getBottom() + 2;
    g.drawText("CAR", carrierArea.withY(labelY).withHeight(10), juce::Justification::centred);
    g.drawText("MOD", modulatorArea.withY(labelY).withHeight(10), juce::Justification::centred);
    g.drawText("OUT", outputArea.withY(labelY).withHeight(10), juce::Justification::centred);
    
    displayArea.removeFromTop(15); // spacing for labels
    
    // Main section - Vocoder band visualization
    auto bandsArea = displayArea.removeFromTop(displayArea.getHeight() * 0.7f);
    
    // Draw vocoder bands
    if (bandCount > 0)
    {
        float bandWidth = bandsArea.getWidth() / bandCount;
        
        for (int i = 0; i < bandCount && i < static_cast<int>(smoothedBandLevels.size()); ++i)
        {
            auto bandArea = juce::Rectangle<float>(bandsArea.getX() + i * bandWidth + 1, 
                                                  bandsArea.getY(), 
                                                  bandWidth - 2, 
                                                  bandsArea.getHeight());
            
            if (smoothedBandLevels[i] > 0.001f)
            {
                float levelHeight = bandArea.getHeight() * smoothedBandLevels[i];
                auto levelRect = juce::Rectangle<float>(bandArea.getX(), 
                                                       bandArea.getBottom() - levelHeight, 
                                                       bandArea.getWidth(), 
                                                       levelHeight);
                
                // Color coding based on frequency band (low = red, mid = green, high = blue)
                float hue = static_cast<float>(i) / bandCount;
                juce::Colour bandColor = juce::Colour::fromHSV(hue * 0.8f, 0.9f, 0.9f, 1.0f);
                g.setColour(bandColor);
                g.fillRoundedRectangle(levelRect, 1.0f);
                
                // Highlight active bands
                if (smoothedBandLevels[i] > 0.7f)
                {
                    g.setColour(juce::Colours::white.withAlpha(0.3f));
                    g.fillRoundedRectangle(levelRect.reduced(1), 1.0f);
                }
            }
            
            // Draw band separators
            g.setColour(HyperPrismLookAndFeel::Colors::outlineVariant.withAlpha(0.3f));
            g.drawVerticalLine(static_cast<int>(bandArea.getRight()), bandsArea.getY(), bandsArea.getBottom());
        }
    }
    
    // Draw scale lines
    g.setColour(HyperPrismLookAndFeel::Colors::outlineVariant.withAlpha(0.2f));
    for (int i = 1; i < 4; ++i)
    {
        float y = bandsArea.getY() + (bandsArea.getHeight() * i / 4.0f);
        g.drawHorizontalLine(static_cast<int>(y), bandsArea.getX(), bandsArea.getRight());
    }
    
    // Bands area label
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(9.0f);
    auto bandsLabelY = bandsArea.getBottom() + 2;
    g.drawText("Vocoder Bands", bandsArea.withY(bandsLabelY).withHeight(12), juce::Justification::centred);
    
    displayArea.removeFromTop(15); // spacing for label
    
    // Bottom section - Parameter display
    auto infoArea = displayArea;
    
    // Parameter values
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(8.0f);
    
    auto paramArea = infoArea.removeFromTop(infoArea.getHeight() / 2);
    auto leftParamArea = paramArea.removeFromLeft(paramArea.getWidth() / 2);
    
    g.drawText("Carrier: " + juce::String(carrierFreq, 0) + " Hz", leftParamArea, juce::Justification::centredLeft);
    g.drawText("Bands: " + juce::String(bandCount), paramArea, juce::Justification::centredLeft);
    
    auto bottomArea = infoArea;
    g.drawText("Analysis Rate: 30 Hz", bottomArea, juce::Justification::centred);
}

void VocoderMeter::timerCallback()
{
    // Get current levels from processor
    carrierLevel = processor.getCarrierLevel();
    modulatorLevel = processor.getModulatorLevel();
    outputLevel = processor.getOutputLevel();
    
    // Get band levels
    const auto& newBandLevels = processor.getBandLevels();
    
    // Smooth the band level changes
    const float smoothing = 0.6f;
    for (size_t i = 0; i < smoothedBandLevels.size() && i < newBandLevels.size(); ++i)
    {
        smoothedBandLevels[i] = smoothedBandLevels[i] * smoothing + newBandLevels[i] * (1.0f - smoothing);
    }
    
    // Get current band count (simulate if not available)
    bandCount = 8; // Default to 8 bands
    carrierFreq = 440.0f; // Default carrier frequency
    
    repaint();
}

//==============================================================================
// VocoderEditor Implementation
//==============================================================================
VocoderEditor::VocoderEditor(VocoderProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), vocoderMeter(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add(VocoderProcessor::CARRIER_FREQ_ID);
    yParameterIDs.add(VocoderProcessor::RELEASE_TIME_ID);
    
    // Title (matching AutoPan style)
    titleLabel.setText("HyperPrism Reimagined Vocoder", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions("Arial", "Bold", 24.0f)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Setup sliders with consistent style
    setupSlider(carrierFreqSlider, carrierFreqLabel, "Carrier Freq", " Hz");
    setupSlider(modulatorGainSlider, modulatorGainLabel, "Mod Gain", " dB");
    setupSlider(bandCountSlider, bandCountLabel, "Band Count", "");
    setupSlider(releaseTimeSlider, releaseTimeLabel, "Release", " ms");
    setupSlider(outputLevelSlider, outputLevelLabel, "Output", " dB");
    
    // Set parameter ranges (example ranges - adjust based on processor)
    carrierFreqSlider.setRange(80.0, 1000.0, 1.0);
    modulatorGainSlider.setRange(-24.0, 12.0, 0.1);
    bandCountSlider.setRange(4.0, 16.0, 1.0);
    releaseTimeSlider.setRange(1.0, 200.0, 0.1);
    outputLevelSlider.setRange(-24.0, 12.0, 0.1);
    
    // Set up right-click handlers for parameter assignment
    carrierFreqLabel.onClick = [this]() { showParameterMenu(&carrierFreqLabel, VocoderProcessor::CARRIER_FREQ_ID); };
    modulatorGainLabel.onClick = [this]() { showParameterMenu(&modulatorGainLabel, VocoderProcessor::MODULATOR_GAIN_ID); };
    bandCountLabel.onClick = [this]() { showParameterMenu(&bandCountLabel, VocoderProcessor::BAND_COUNT_ID); };
    releaseTimeLabel.onClick = [this]() { showParameterMenu(&releaseTimeLabel, VocoderProcessor::RELEASE_TIME_ID); };
    outputLevelLabel.onClick = [this]() { showParameterMenu(&outputLevelLabel, VocoderProcessor::OUTPUT_LEVEL_ID); };
    
    // Bypass button (top right like AutoPan)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::red);
    bypassButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
    addAndMakeVisible(bypassButton);
    
    // Create attachments
    auto& apvts = audioProcessor.getValueTreeState();
    carrierFreqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, VocoderProcessor::CARRIER_FREQ_ID, carrierFreqSlider);
    modulatorGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, VocoderProcessor::MODULATOR_GAIN_ID, modulatorGainSlider);
    bandCountAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, VocoderProcessor::BAND_COUNT_ID, bandCountSlider);
    releaseTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, VocoderProcessor::RELEASE_TIME_ID, releaseTimeSlider);
    outputLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, VocoderProcessor::OUTPUT_LEVEL_ID, outputLevelSlider);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, VocoderProcessor::BYPASS_ID, bypassButton);
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);
    xyPadLabel.setText("Carrier Freq / Release", juce::dontSendNotification);
    xyPadLabel.setJustificationType(juce::Justification::centred);
    xyPadLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    
    // Add vocoder meter
    addAndMakeVisible(vocoderMeter);
    vocoderMeterLabel.setText("Vocoder Analysis", juce::dontSendNotification);
    vocoderMeterLabel.setJustificationType(juce::Justification::centred);
    vocoderMeterLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(vocoderMeterLabel);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    carrierFreqSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    modulatorGainSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    bandCountSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    releaseTimeSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    outputLevelSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    setSize(650, 600);
}

VocoderEditor::~VocoderEditor()
{
    setLookAndFeel(nullptr);
}

void VocoderEditor::paint(juce::Graphics& g)
{
    // Dark background matching AutoPan
    g.fillAll(HyperPrismLookAndFeel::Colors::background);
}

void VocoderEditor::resized()
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
    
    // All controls in one row: Carrier Freq, Mod Gain, Band Count, Release, Output Level
    carrierFreqSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    carrierFreqLabel.setBounds(carrierFreqSlider.getX(), carrierFreqSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    modulatorGainSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    modulatorGainLabel.setBounds(modulatorGainSlider.getX(), modulatorGainSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    bandCountSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    bandCountLabel.setBounds(bandCountSlider.getX(), bandCountSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    releaseTimeSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    releaseTimeLabel.setBounds(releaseTimeSlider.getX(), releaseTimeSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    outputLevelSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    outputLevelLabel.setBounds(outputLevelSlider.getX(), outputLevelSlider.getBottom(), sliderWidth, 20);
    
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
    
    // Vocoder meter on right (matching height)
    auto meterBounds = bottomArea.withX(xyPadBounds.getRight() + 20).withWidth(meterSize).withHeight(panelHeight);
    vocoderMeter.setBounds(meterBounds);
    
    // Align labels at the same Y position
    auto labelY = xyPadBounds.getBottom() + 5;
    xyPadLabel.setBounds(xyPad.getX(), labelY, xyPadWidth, 20);
    vocoderMeterLabel.setBounds(vocoderMeter.getX(), labelY, meterSize, 20);
}

void VocoderEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void VocoderEditor::updateParameterColors()
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
    
    updateLabelColor(carrierFreqLabel, VocoderProcessor::CARRIER_FREQ_ID);
    updateLabelColor(modulatorGainLabel, VocoderProcessor::MODULATOR_GAIN_ID);
    updateLabelColor(bandCountLabel, VocoderProcessor::BAND_COUNT_ID);
    updateLabelColor(releaseTimeLabel, VocoderProcessor::RELEASE_TIME_ID);
    updateLabelColor(outputLevelLabel, VocoderProcessor::OUTPUT_LEVEL_ID);
}

void VocoderEditor::updateXYPadFromParameters()
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

void VocoderEditor::updateParametersFromXYPad(float x, float y)
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

void VocoderEditor::showParameterMenu(juce::Label* label, const juce::String& parameterID)
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
                    xParameterIDs.add(VocoderProcessor::CARRIER_FREQ_ID);
                    
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
                    yParameterIDs.add(VocoderProcessor::RELEASE_TIME_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add(VocoderProcessor::CARRIER_FREQ_ID);
                yParameterIDs.add(VocoderProcessor::RELEASE_TIME_ID);
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void VocoderEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == VocoderProcessor::CARRIER_FREQ_ID) return "Carrier Freq";
        if (paramID == VocoderProcessor::MODULATOR_GAIN_ID) return "Mod Gain";
        if (paramID == VocoderProcessor::BAND_COUNT_ID) return "Band Count";
        if (paramID == VocoderProcessor::RELEASE_TIME_ID) return "Release";
        if (paramID == VocoderProcessor::OUTPUT_LEVEL_ID) return "Output";
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