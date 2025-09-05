//==============================================================================
// HyperPrism Reimagined - Frequency Shifter Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "FrequencyShifterEditor.h"

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
// FrequencyShiftMeter Implementation
//==============================================================================
FrequencyShiftMeter::FrequencyShiftMeter(FrequencyShifterProcessor& processor)
    : audioProcessor(processor)
{
    startTimerHz(30); // 30 FPS update rate
}

FrequencyShiftMeter::~FrequencyShiftMeter()
{
    stopTimer();
}

void FrequencyShiftMeter::paint(juce::Graphics& g)
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
    float meterWidth = meterArea.getWidth();
    float meterHeight = meterArea.getHeight() - 40; // Space for labels
    
    // Divide into input and output sections
    float sectionWidth = meterWidth / 2.0f;
    
    // Input Level
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
    
    // Output Level
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
    
    // Draw scale lines
    g.setColour(HyperPrismLookAndFeel::Colors::surface);
    for (int i = 1; i < 4; ++i)
    {
        float y = meterArea.getY() + (meterHeight * i / 4.0f);
        g.drawHorizontalLine(static_cast<int>(y), meterArea.getX(), meterArea.getRight());
    }
    
    // Draw section divider
    float x = meterArea.getX() + sectionWidth;
    g.drawVerticalLine(static_cast<int>(x), meterArea.getY(), meterArea.getY() + meterHeight);
    
    // Labels
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(9.0f);
    
    auto labelArea = juce::Rectangle<float>(meterArea.getX(), meterArea.getY() + meterHeight + 5, sectionWidth, 15);
    g.drawText("INPUT", labelArea, juce::Justification::centred);
    
    labelArea.translate(sectionWidth, 0);
    g.drawText("OUTPUT", labelArea, juce::Justification::centred);
    
    // Title
    g.setFont(10.0f);
    g.setColour(HyperPrismLookAndFeel::Colors::primary);
    auto titleArea = juce::Rectangle<float>(meterArea.getX(), meterArea.getY() + meterHeight + 25, meterWidth, 15);
    g.drawText("FREQUENCY SHIFT PROCESSING", titleArea, juce::Justification::centred);
}

void FrequencyShiftMeter::resized()
{
    // Nothing to do here - all drawing is based on bounds
}

void FrequencyShiftMeter::timerCallback()
{
    float newInputLevel = audioProcessor.getInputLevel();
    float newOutputLevel = audioProcessor.getOutputLevel();
    
    // Smooth the level changes
    const float smoothing = 0.7f;
    inputLevel = inputLevel * smoothing + newInputLevel * (1.0f - smoothing);
    outputLevel = outputLevel * smoothing + newOutputLevel * (1.0f - smoothing);
    
    repaint();
}

//==============================================================================
// FrequencyShifterEditor Implementation
//==============================================================================
FrequencyShifterEditor::FrequencyShifterEditor(FrequencyShifterProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), frequencyShiftMeter(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add(FrequencyShifterProcessor::FREQUENCY_SHIFT_ID);
    yParameterIDs.add(FrequencyShifterProcessor::MIX_ID);
    
    // Title (matching AutoPan style)
    titleLabel.setText("HyperPrism Reimagined Frequency Shifter", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions("Arial", "Bold", 24.0f)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Setup sliders with consistent style (4 parameters in single row)
    setupSlider(frequencyShiftSlider, frequencyShiftLabel, "Freq Shift", " Hz");
    setupSlider(fineShiftSlider, fineShiftLabel, "Fine Shift", " Hz");
    setupSlider(mixSlider, mixLabel, "Mix", "%");
    setupSlider(outputLevelSlider, outputLevelLabel, "Output Level", " dB");
    
    // Set up right-click handlers for parameter assignment
    frequencyShiftLabel.onClick = [this]() { showParameterMenu(&frequencyShiftLabel, FrequencyShifterProcessor::FREQUENCY_SHIFT_ID); };
    fineShiftLabel.onClick = [this]() { showParameterMenu(&fineShiftLabel, FrequencyShifterProcessor::FINE_SHIFT_ID); };
    mixLabel.onClick = [this]() { showParameterMenu(&mixLabel, FrequencyShifterProcessor::MIX_ID); };
    outputLevelLabel.onClick = [this]() { showParameterMenu(&outputLevelLabel, FrequencyShifterProcessor::OUTPUT_LEVEL_ID); };
    
    // Bypass button (top right like AutoPan)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::red);
    bypassButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
    addAndMakeVisible(bypassButton);
    
    // Create attachments
    auto& apvts = audioProcessor.getValueTreeState();
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, FrequencyShifterProcessor::BYPASS_ID, bypassButton);
    frequencyShiftAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, FrequencyShifterProcessor::FREQUENCY_SHIFT_ID, frequencyShiftSlider);
    fineShiftAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, FrequencyShifterProcessor::FINE_SHIFT_ID, fineShiftSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, FrequencyShifterProcessor::MIX_ID, mixSlider);
    outputLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, FrequencyShifterProcessor::OUTPUT_LEVEL_ID, outputLevelSlider);
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);
    xyPadLabel.setText("Freq Shift / Mix", juce::dontSendNotification);
    xyPadLabel.setJustificationType(juce::Justification::centred);
    xyPadLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    
    // Add frequency shift meter
    addAndMakeVisible(frequencyShiftMeter);
    frequencyShiftMeterLabel.setText("Frequency Shift Meter", juce::dontSendNotification);
    frequencyShiftMeterLabel.setJustificationType(juce::Justification::centred);
    frequencyShiftMeterLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(frequencyShiftMeterLabel);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    frequencyShiftSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    fineShiftSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    mixSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    outputLevelSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    setSize(650, 600);
}

FrequencyShifterEditor::~FrequencyShifterEditor()
{
    setLookAndFeel(nullptr);
}

void FrequencyShifterEditor::paint(juce::Graphics& g)
{
    // Dark background matching AutoPan
    g.fillAll(HyperPrismLookAndFeel::Colors::background);
}

void FrequencyShifterEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(40));
    
    // Bypass button (top right)
    bypassButton.setBounds(bounds.getWidth() - 100, 10, 80, 30);
    
    bounds.reduce(20, 10);
    
    // Single row layout for 4 parameters (like AutoPan/BandPass/BandReject)
    // Available height after title and margins: ~500px
    // Distribution: 160px knobs + 20px spacing + 180px panels + 25px labels = 385px
    
    auto topRow = bounds.removeFromTop(160);
    auto sliderWidth = 80;
    auto spacing = 15;
    
    // Calculate total width needed for 4 sliders
    auto totalSliderWidth = sliderWidth * 4 + spacing * 3;
    auto startX = (bounds.getWidth() - totalSliderWidth) / 2;
    topRow.removeFromLeft(startX);
    
    // Single row: Freq Shift, Fine Shift, Mix, Output Level
    frequencyShiftSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    frequencyShiftLabel.setBounds(frequencyShiftSlider.getX(), frequencyShiftSlider.getBottom(), sliderWidth, 20);
    topRow.removeFromLeft(spacing);
    
    fineShiftSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    fineShiftLabel.setBounds(fineShiftSlider.getX(), fineShiftSlider.getBottom(), sliderWidth, 20);
    topRow.removeFromLeft(spacing);
    
    mixSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    mixLabel.setBounds(mixSlider.getX(), mixSlider.getBottom(), sliderWidth, 20);
    topRow.removeFromLeft(spacing);
    
    outputLevelSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    outputLevelLabel.setBounds(outputLevelSlider.getX(), outputLevelSlider.getBottom(), sliderWidth, 20);
    
    // Bottom section - XY Pad and Meter side by side (matching sizes)
    bounds.removeFromTop(20);
    
    // Split remaining space horizontally for XY pad and meter
    auto bottomArea = bounds;
    auto panelSize = 180;  // Both panels same height
    auto panelWidth = 200; // Both panels same width
    auto totalWidth = panelWidth * 2 + 20; // Plus spacing
    auto startXBottom = (bottomArea.getWidth() - totalWidth) / 2;
    
    // XY Pad on left (200x180 standard)
    auto xyPadBounds = bottomArea.withX(startXBottom).withWidth(panelWidth).withHeight(panelSize);
    xyPad.setBounds(xyPadBounds);
    
    // Frequency Shift Meter on right (matching size)
    auto meterBounds = bottomArea.withX(startXBottom + panelWidth + 20).withWidth(panelWidth).withHeight(panelSize);
    frequencyShiftMeter.setBounds(meterBounds);
    
    // Align labels at the same Y position
    auto labelY = xyPadBounds.getBottom() + 5;
    xyPadLabel.setBounds(xyPadBounds.getX(), labelY, panelWidth, 20);
    frequencyShiftMeterLabel.setBounds(meterBounds.getX(), labelY, panelWidth, 20);
}

void FrequencyShifterEditor::setupControls()
{
    // This method is no longer used - moved into constructor
}

void FrequencyShifterEditor::setupXYPad()
{
    // This method is no longer used - moved into constructor
}

void FrequencyShifterEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void FrequencyShifterEditor::updateParameterColors()
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
    
    updateLabelColor(frequencyShiftLabel, FrequencyShifterProcessor::FREQUENCY_SHIFT_ID);
    updateLabelColor(fineShiftLabel, FrequencyShifterProcessor::FINE_SHIFT_ID);
    updateLabelColor(mixLabel, FrequencyShifterProcessor::MIX_ID);
    updateLabelColor(outputLevelLabel, FrequencyShifterProcessor::OUTPUT_LEVEL_ID);
}

void FrequencyShifterEditor::updateXYPadFromParameters()
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

void FrequencyShifterEditor::updateParametersFromXYPad(float x, float y)
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

void FrequencyShifterEditor::showParameterMenu(juce::Label* label, const juce::String& parameterID)
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
                    xParameterIDs.add(FrequencyShifterProcessor::FREQUENCY_SHIFT_ID);
                    
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
                    yParameterIDs.add(FrequencyShifterProcessor::MIX_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add(FrequencyShifterProcessor::FREQUENCY_SHIFT_ID);
                yParameterIDs.add(FrequencyShifterProcessor::MIX_ID);
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void FrequencyShifterEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == FrequencyShifterProcessor::FREQUENCY_SHIFT_ID) return "Freq Shift";
        if (paramID == FrequencyShifterProcessor::FINE_SHIFT_ID) return "Fine Shift";
        if (paramID == FrequencyShifterProcessor::MIX_ID) return "Mix";
        if (paramID == FrequencyShifterProcessor::OUTPUT_LEVEL_ID) return "Output Level";
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