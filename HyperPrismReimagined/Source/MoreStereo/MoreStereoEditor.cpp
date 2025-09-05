//==============================================================================
// HyperPrism Reimagined - More Stereo Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "MoreStereoEditor.h"

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
// EnhancedStereoMeter Implementation
//==============================================================================
EnhancedStereoMeter::EnhancedStereoMeter(MoreStereoProcessor& p) 
    : processor(p)
{
    startTimerHz(30); // 30 FPS update rate
}

EnhancedStereoMeter::~EnhancedStereoMeter()
{
    stopTimer();
}

void EnhancedStereoMeter::paint(juce::Graphics& g)
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
    
    // Draw stereo field visualization (similar to MSMatrix)
    float centerX = meterArea.getCentreX();
    float centerY = meterArea.getCentreY();
    float radius = juce::jmin(meterArea.getWidth(), meterArea.getHeight()) * 0.4f;
    
    // Draw background circle
    g.setColour(HyperPrismLookAndFeel::Colors::surfaceVariant);
    g.fillEllipse(centerX - radius, centerY - radius, radius * 2, radius * 2);
    
    // Draw axes
    g.setColour(HyperPrismLookAndFeel::Colors::outlineVariant);
    g.drawLine(centerX - radius - 10, centerY, centerX + radius + 10, centerY, 1.0f);
    g.drawLine(centerX, centerY - radius - 10, centerX, centerY + radius + 10, 1.0f);
    
    // Draw stereo width visualization
    float widthRadius = radius * stereoWidth;
    g.setColour(HyperPrismLookAndFeel::Colors::primary.withAlpha(0.3f));
    g.fillEllipse(centerX - widthRadius, centerY - widthRadius, widthRadius * 2, widthRadius * 2);
    
    // Draw L/R balance
    float xPos = centerX + (rightLevel - leftLevel) * radius * 0.8f;
    float yPos = centerY - (ambienceLevel - 0.5f) * radius * 1.6f;
    
    // Draw level dot with glow effect
    g.setColour(HyperPrismLookAndFeel::Colors::primary.withAlpha(0.3f));
    g.fillEllipse(xPos - 12, yPos - 12, 24, 24);
    
    g.setColour(HyperPrismLookAndFeel::Colors::primary);
    g.fillEllipse(xPos - 8, yPos - 8, 16, 16);
    
    g.setColour(juce::Colours::white);
    g.fillEllipse(xPos - 4, yPos - 4, 8, 8);
    
    // Labels
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(10.0f);
    g.drawText("L", centerX - radius - 25, centerY - 7, 15, 15, juce::Justification::right);
    g.drawText("R", centerX + radius + 10, centerY - 7, 15, 15, juce::Justification::left);
    g.drawText("AMB", centerX - 20, centerY - radius - 25, 40, 15, juce::Justification::centred);
    
    // Level values
    g.setFont(9.0f);
    g.setColour(HyperPrismLookAndFeel::Colors::primary);
    auto valueArea = bounds.removeFromBottom(20).reduced(5, 0);
    g.drawText("Width: " + juce::String(static_cast<int>(stereoWidth * 100)) + "% | " +
               "Ambience: " + juce::String(static_cast<int>(ambienceLevel * 100)) + "%", 
               valueArea, juce::Justification::centred);
}

void EnhancedStereoMeter::timerCallback()
{
    float newLeftLevel = processor.getLeftLevel();
    float newRightLevel = processor.getRightLevel();
    float newStereoWidth = processor.getStereoWidth();
    float newAmbienceLevel = processor.getAmbienceLevel();
    
    // Smooth the level changes
    const float smoothing = 0.7f;
    leftLevel = leftLevel * smoothing + newLeftLevel * (1.0f - smoothing);
    rightLevel = rightLevel * smoothing + newRightLevel * (1.0f - smoothing);
    stereoWidth = stereoWidth * smoothing + newStereoWidth * (1.0f - smoothing);
    ambienceLevel = ambienceLevel * smoothing + newAmbienceLevel * (1.0f - smoothing);
    
    repaint();
}

//==============================================================================
// MoreStereoEditor Implementation
//==============================================================================
MoreStereoEditor::MoreStereoEditor(MoreStereoProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), enhancedStereoMeter(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add(MoreStereoProcessor::WIDTH_ID);
    yParameterIDs.add(MoreStereoProcessor::STEREO_ENHANCE_ID);
    
    // Title (matching AutoPan style)
    titleLabel.setText("HyperPrism Reimagined More Stereo", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions("Arial", "Bold", 24.0f)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Setup sliders with consistent style (6 parameters)
    setupSlider(widthSlider, widthLabel, "Stereo Width", "");
    setupSlider(bassMonoSlider, bassMonoLabel, "Bass Mono", "");
    setupSlider(crossoverFreqSlider, crossoverFreqLabel, "Crossover", " Hz");
    setupSlider(stereoEnhanceSlider, stereoEnhanceLabel, "Stereo Enhance", "");
    setupSlider(ambienceSlider, ambienceLabel, "Ambience", "");
    setupSlider(outputLevelSlider, outputLevelLabel, "Output Level", " dB");
    
    // Set up right-click handlers for parameter assignment
    widthLabel.onClick = [this]() { showParameterMenu(&widthLabel, MoreStereoProcessor::WIDTH_ID); };
    bassMonoLabel.onClick = [this]() { showParameterMenu(&bassMonoLabel, MoreStereoProcessor::BASS_MONO_ID); };
    crossoverFreqLabel.onClick = [this]() { showParameterMenu(&crossoverFreqLabel, MoreStereoProcessor::CROSSOVER_FREQ_ID); };
    stereoEnhanceLabel.onClick = [this]() { showParameterMenu(&stereoEnhanceLabel, MoreStereoProcessor::STEREO_ENHANCE_ID); };
    ambienceLabel.onClick = [this]() { showParameterMenu(&ambienceLabel, MoreStereoProcessor::AMBIENCE_ID); };
    outputLevelLabel.onClick = [this]() { showParameterMenu(&outputLevelLabel, MoreStereoProcessor::OUTPUT_LEVEL_ID); };
    
    // Bypass button (top right like AutoPan)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::red);
    bypassButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
    addAndMakeVisible(bypassButton);
    
    // Create attachments
    auto& vts = audioProcessor.getValueTreeState();
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        vts, MoreStereoProcessor::BYPASS_ID, bypassButton);
    widthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, MoreStereoProcessor::WIDTH_ID, widthSlider);
    bassMonoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, MoreStereoProcessor::BASS_MONO_ID, bassMonoSlider);
    crossoverFreqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, MoreStereoProcessor::CROSSOVER_FREQ_ID, crossoverFreqSlider);
    stereoEnhanceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, MoreStereoProcessor::STEREO_ENHANCE_ID, stereoEnhanceSlider);
    ambienceAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, MoreStereoProcessor::AMBIENCE_ID, ambienceSlider);
    outputLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, MoreStereoProcessor::OUTPUT_LEVEL_ID, outputLevelSlider);
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);
    xyPadLabel.setText("Stereo Width / Stereo Enhance", juce::dontSendNotification);
    xyPadLabel.setJustificationType(juce::Justification::centred);
    xyPadLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    
    // Add enhanced stereo meter
    addAndMakeVisible(enhancedStereoMeter);
    enhancedStereoMeterLabel.setText("Stereo Field", juce::dontSendNotification);
    enhancedStereoMeterLabel.setJustificationType(juce::Justification::centred);
    enhancedStereoMeterLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(enhancedStereoMeterLabel);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    widthSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    bassMonoSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    crossoverFreqSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    stereoEnhanceSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    ambienceSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    outputLevelSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    setSize(650, 600);
}

MoreStereoEditor::~MoreStereoEditor()
{
    setLookAndFeel(nullptr);
}

void MoreStereoEditor::paint(juce::Graphics& g)
{
    // Dark background matching AutoPan
    g.fillAll(HyperPrismLookAndFeel::Colors::background);
}

void MoreStereoEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(40));
    
    // Bypass button (top right)
    bypassButton.setBounds(bounds.getWidth() - 100, 10, 80, 30);
    
    bounds.reduce(20, 10);
    
    // 6 parameter layout: 3+3 sliders in two rows (similar to MSMatrix)
    // Available height after title and margins: ~500px
    
    auto topRow = bounds.removeFromTop(140);
    auto sliderWidth = 80;
    auto spacing = 20;
    
    // Calculate total width needed for 3 sliders
    auto totalSliderWidth = sliderWidth * 3 + spacing * 2;
    auto startX = (bounds.getWidth() - totalSliderWidth) / 2;
    topRow.removeFromLeft(startX);
    
    // Top row: Width, Bass Mono, Crossover Freq
    widthSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    widthLabel.setBounds(widthSlider.getX(), widthSlider.getBottom(), sliderWidth, 20);
    topRow.removeFromLeft(spacing);
    
    bassMonoSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    bassMonoLabel.setBounds(bassMonoSlider.getX(), bassMonoSlider.getBottom(), sliderWidth, 20);
    topRow.removeFromLeft(spacing);
    
    crossoverFreqSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    crossoverFreqLabel.setBounds(crossoverFreqSlider.getX(), crossoverFreqSlider.getBottom(), sliderWidth, 20);
    
    bounds.removeFromTop(10);
    
    // Second row: Stereo Enhance, Ambience, Output Level
    auto secondRow = bounds.removeFromTop(140);
    secondRow.removeFromLeft(startX);
    
    stereoEnhanceSlider.setBounds(secondRow.removeFromLeft(sliderWidth).reduced(0, 20));
    stereoEnhanceLabel.setBounds(stereoEnhanceSlider.getX(), stereoEnhanceSlider.getBottom(), sliderWidth, 20);
    secondRow.removeFromLeft(spacing);
    
    ambienceSlider.setBounds(secondRow.removeFromLeft(sliderWidth).reduced(0, 20));
    ambienceLabel.setBounds(ambienceSlider.getX(), ambienceSlider.getBottom(), sliderWidth, 20);
    secondRow.removeFromLeft(spacing);
    
    outputLevelSlider.setBounds(secondRow.removeFromLeft(sliderWidth).reduced(0, 20));
    outputLevelLabel.setBounds(outputLevelSlider.getX(), outputLevelSlider.getBottom(), sliderWidth, 20);
    
    // Bottom section - XY Pad and Meter side by side
    bounds.removeFromTop(10);
    
    // Split remaining space horizontally for XY pad and meter
    auto bottomArea = bounds;
    auto componentHeight = 180;
    
    // XY Pad on left (200x180 standard)
    auto xyPadWidth = 200;
    auto xyPadHeight = 180;
    auto xyPadBounds = bottomArea.withWidth(xyPadWidth).withHeight(xyPadHeight);
    auto xyPadX = bottomArea.getX() + (bottomArea.getWidth() - xyPadWidth - 150 - 20) / 2; // center both components
    xyPadBounds.translate(xyPadX - bottomArea.getX(), 0);
    
    xyPad.setBounds(xyPadBounds);
    xyPadLabel.setBounds(xyPadBounds.getX(), xyPadBounds.getBottom() + 5, xyPadWidth, 20);
    
    // Enhanced Stereo Meter on right
    auto meterWidth = 150;
    auto meterBounds = bottomArea.withX(xyPadBounds.getRight() + 20).withWidth(meterWidth).withHeight(componentHeight);
    
    enhancedStereoMeter.setBounds(meterBounds);
    enhancedStereoMeterLabel.setBounds(meterBounds.getX(), meterBounds.getBottom() + 5, meterWidth, 20);
}

void MoreStereoEditor::setupControls()
{
    // This method is no longer used - moved into constructor
}

void MoreStereoEditor::setupXYPad()
{
    // This method is no longer used - moved into constructor
}

void MoreStereoEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void MoreStereoEditor::updateParameterColors()
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
    
    updateLabelColor(widthLabel, MoreStereoProcessor::WIDTH_ID);
    updateLabelColor(bassMonoLabel, MoreStereoProcessor::BASS_MONO_ID);
    updateLabelColor(crossoverFreqLabel, MoreStereoProcessor::CROSSOVER_FREQ_ID);
    updateLabelColor(stereoEnhanceLabel, MoreStereoProcessor::STEREO_ENHANCE_ID);
    updateLabelColor(ambienceLabel, MoreStereoProcessor::AMBIENCE_ID);
    updateLabelColor(outputLevelLabel, MoreStereoProcessor::OUTPUT_LEVEL_ID);
}

void MoreStereoEditor::updateXYPadFromParameters()
{
    // For multiple parameters, use the average of their normalized values
    float xValue = 0.0f;
    float yValue = 0.0f;
    
    auto& vts = audioProcessor.getValueTreeState();
    
    // Calculate average X value
    if (!xParameterIDs.isEmpty())
    {
        for (const auto& paramID : xParameterIDs)
        {
            if (auto* param = vts.getRawParameterValue(paramID))
            {
                if (auto* paramObj = vts.getParameter(paramID))
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
            if (auto* param = vts.getRawParameterValue(paramID))
            {
                if (auto* paramObj = vts.getParameter(paramID))
                {
                    yValue += paramObj->convertTo0to1(*param);
                }
            }
        }
        yValue /= yParameterIDs.size();
    }
    
    xyPad.setValues(xValue, yValue);
}

void MoreStereoEditor::updateParametersFromXYPad(float x, float y)
{
    auto& vts = audioProcessor.getValueTreeState();
    
    // Update all assigned X parameters
    for (const auto& paramID : xParameterIDs)
    {
        if (auto* param = vts.getParameter(paramID))
            param->setValueNotifyingHost(x);
    }
    
    // Update all assigned Y parameters
    for (const auto& paramID : yParameterIDs)
    {
        if (auto* param = vts.getParameter(paramID))
            param->setValueNotifyingHost(y);
    }
}

void MoreStereoEditor::showParameterMenu(juce::Label* label, const juce::String& parameterID)
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
                    xParameterIDs.add(MoreStereoProcessor::WIDTH_ID);
                    
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
                    yParameterIDs.add(MoreStereoProcessor::STEREO_ENHANCE_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add(MoreStereoProcessor::WIDTH_ID);
                yParameterIDs.add(MoreStereoProcessor::STEREO_ENHANCE_ID);
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void MoreStereoEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == MoreStereoProcessor::WIDTH_ID) return "Stereo Width";
        if (paramID == MoreStereoProcessor::BASS_MONO_ID) return "Bass Mono";
        if (paramID == MoreStereoProcessor::CROSSOVER_FREQ_ID) return "Crossover";
        if (paramID == MoreStereoProcessor::STEREO_ENHANCE_ID) return "Stereo Enhance";
        if (paramID == MoreStereoProcessor::AMBIENCE_ID) return "Ambience";
        if (paramID == MoreStereoProcessor::OUTPUT_LEVEL_ID) return "Output Level";
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