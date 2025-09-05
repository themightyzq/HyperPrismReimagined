//==============================================================================
// HyperPrism Reimagined - Pitch Changer Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "PitchChangerEditor.h"

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
// PitchMeter Implementation
//==============================================================================
PitchMeter::PitchMeter(PitchChangerProcessor& p) 
    : processor(p)
{
    startTimerHz(30); // 30 FPS update rate
}

PitchMeter::~PitchMeter()
{
    stopTimer();
}

void PitchMeter::paint(juce::Graphics& g)
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
    
    // Draw I/O level meters
    float meterHeight = meterArea.getHeight() - 50.0f; // Leave space for pitch display
    float meterWidth = 20.0f;
    float spacing = 15.0f;
    
    // Input level meter (left)
    auto inputMeterBounds = juce::Rectangle<float>(
        meterArea.getX(),
        meterArea.getY(),
        meterWidth,
        meterHeight
    );
    
    g.setColour(HyperPrismLookAndFeel::Colors::surfaceVariant);
    g.fillRoundedRectangle(inputMeterBounds, 2.0f);
    
    if (inputLevel > 0.001f)
    {
        auto levelHeight = inputMeterBounds.getHeight() * inputLevel;
        auto levelBounds = inputMeterBounds.withHeight(levelHeight)
                                           .withBottomY(inputMeterBounds.getBottom());
        g.setColour(HyperPrismLookAndFeel::Colors::success);
        g.fillRoundedRectangle(levelBounds, 2.0f);
    }
    
    // Output level meter (right)
    auto outputMeterBounds = juce::Rectangle<float>(
        meterArea.getRight() - meterWidth,
        meterArea.getY(),
        meterWidth,
        meterHeight
    );
    
    g.setColour(HyperPrismLookAndFeel::Colors::surfaceVariant);
    g.fillRoundedRectangle(outputMeterBounds, 2.0f);
    
    if (outputLevel > 0.001f)
    {
        auto levelHeight = outputMeterBounds.getHeight() * outputLevel;
        auto levelBounds = outputMeterBounds.withHeight(levelHeight)
                                            .withBottomY(outputMeterBounds.getBottom());
        g.setColour(HyperPrismLookAndFeel::Colors::primary);
        g.fillRoundedRectangle(levelBounds, 2.0f);
    }
    
    // Labels
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(10.0f);
    g.drawText("IN", inputMeterBounds.translated(0, meterHeight + 5).withHeight(15), 
               juce::Justification::centred);
    g.drawText("OUT", outputMeterBounds.translated(0, meterHeight + 5).withHeight(15), 
               juce::Justification::centred);
    
    // Pitch detection display
    auto pitchArea = juce::Rectangle<float>(
        inputMeterBounds.getRight() + spacing,
        meterArea.getY() + meterHeight * 0.3f,
        outputMeterBounds.getX() - inputMeterBounds.getRight() - spacing * 2,
        40.0f
    );
    
    // Pitch detection visualization
    if (detectedPitch > 20.0f) // Valid pitch detected
    {
        // Draw pitch indicator
        g.setColour(HyperPrismLookAndFeel::Colors::primary.withAlpha(0.3f));
        g.fillRoundedRectangle(pitchArea, 4.0f);
        
        // Map pitch to note name
        float a4Freq = 440.0f;
        float noteNum = 12.0f * std::log2(detectedPitch / a4Freq) + 69.0f;
        int midiNote = static_cast<int>(std::round(noteNum));
        float cents = (noteNum - midiNote) * 100.0f;
        
        // Note names
        const char* noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
        juce::String noteName = juce::String(noteNames[midiNote % 12]) + juce::String(midiNote / 12 - 2);
        
        g.setColour(HyperPrismLookAndFeel::Colors::primary);
        g.setFont(16.0f);
        g.drawText(noteName, pitchArea.reduced(5.0f), juce::Justification::centred);
        
        // Cents deviation
        g.setFont(10.0f);
        juce::String centsText = (cents >= 0 ? "+" : "") + juce::String(cents, 0) + " cents";
        g.drawText(centsText, pitchArea.translated(0, 25).withHeight(15), 
                   juce::Justification::centred);
    }
    else
    {
        g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant.withAlpha(0.5f));
        g.setFont(12.0f);
        g.drawText("No pitch detected", pitchArea, juce::Justification::centred);
    }
}

void PitchMeter::timerCallback()
{
    float newInputLevel = processor.getInputLevel();
    float newOutputLevel = processor.getOutputLevel();
    float newDetectedPitch = processor.getPitchDetection();
    
    // Smooth the changes
    const float smoothing = 0.7f;
    inputLevel = inputLevel * smoothing + newInputLevel * (1.0f - smoothing);
    outputLevel = outputLevel * smoothing + newOutputLevel * (1.0f - smoothing);
    
    // Less smoothing for pitch detection to keep it responsive
    const float pitchSmoothing = 0.9f;
    detectedPitch = detectedPitch * pitchSmoothing + newDetectedPitch * (1.0f - pitchSmoothing);
    
    repaint();
}

//==============================================================================
// PitchChangerEditor Implementation
//==============================================================================
PitchChangerEditor::PitchChangerEditor(PitchChangerProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), pitchMeter(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add(PitchChangerProcessor::PITCH_SHIFT_ID);
    yParameterIDs.add(PitchChangerProcessor::FORMANT_SHIFT_ID);
    
    // Title (matching AutoPan style)
    titleLabel.setText("HyperPrism Reimagined Pitch Changer", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions("Arial", "Bold", 24.0f)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Setup sliders with consistent style (5 parameters)
    setupSlider(pitchShiftSlider, pitchShiftLabel, "Pitch Shift", " st");
    setupSlider(fineTuneSlider, fineTuneLabel, "Fine Tune", " cents");
    setupSlider(formantShiftSlider, formantShiftLabel, "Formant Shift", " st");
    setupSlider(mixSlider, mixLabel, "Mix", "");
    setupSlider(outputLevelSlider, outputLevelLabel, "Output Level", " dB");
    
    // Set parameter ranges
    pitchShiftSlider.setRange(-24.0, 24.0, 1.0);
    fineTuneSlider.setRange(-100.0, 100.0, 1.0);
    formantShiftSlider.setRange(-12.0, 12.0, 0.1);
    mixSlider.setRange(0.0, 100.0, 0.1);
    outputLevelSlider.setRange(-20.0, 20.0, 0.1);
    
    // Set up right-click handlers for parameter assignment
    pitchShiftLabel.onClick = [this]() { showParameterMenu(&pitchShiftLabel, PitchChangerProcessor::PITCH_SHIFT_ID); };
    fineTuneLabel.onClick = [this]() { showParameterMenu(&fineTuneLabel, PitchChangerProcessor::FINE_TUNE_ID); };
    formantShiftLabel.onClick = [this]() { showParameterMenu(&formantShiftLabel, PitchChangerProcessor::FORMANT_SHIFT_ID); };
    mixLabel.onClick = [this]() { showParameterMenu(&mixLabel, PitchChangerProcessor::MIX_ID); };
    outputLevelLabel.onClick = [this]() { showParameterMenu(&outputLevelLabel, PitchChangerProcessor::OUTPUT_LEVEL_ID); };
    
    // Bypass button (top right like AutoPan)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::red);
    bypassButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
    addAndMakeVisible(bypassButton);
    
    // Create attachments
    auto& vts = audioProcessor.getValueTreeState();
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        vts, PitchChangerProcessor::BYPASS_ID, bypassButton);
    pitchShiftAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, PitchChangerProcessor::PITCH_SHIFT_ID, pitchShiftSlider);
    fineTuneAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, PitchChangerProcessor::FINE_TUNE_ID, fineTuneSlider);
    formantShiftAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, PitchChangerProcessor::FORMANT_SHIFT_ID, formantShiftSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, PitchChangerProcessor::MIX_ID, mixSlider);
    outputLevelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, PitchChangerProcessor::OUTPUT_LEVEL_ID, outputLevelSlider);
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);
    xyPadLabel.setText("Pitch Shift / Formant Shift", juce::dontSendNotification);
    xyPadLabel.setJustificationType(juce::Justification::centred);
    xyPadLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    
    // Add pitch meter
    addAndMakeVisible(pitchMeter);
    pitchMeterLabel.setText("Pitch Analysis", juce::dontSendNotification);
    pitchMeterLabel.setJustificationType(juce::Justification::centred);
    pitchMeterLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(pitchMeterLabel);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    pitchShiftSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    fineTuneSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    formantShiftSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    mixSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    outputLevelSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    setSize(650, 600);
}

PitchChangerEditor::~PitchChangerEditor()
{
    setLookAndFeel(nullptr);
}

void PitchChangerEditor::paint(juce::Graphics& g)
{
    // Dark background matching AutoPan
    g.fillAll(HyperPrismLookAndFeel::Colors::background);
}

void PitchChangerEditor::resized()
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
    
    // All controls in one row: Pitch Shift, Fine Tune, Formant Shift, Mix, Output Level
    pitchShiftSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    pitchShiftLabel.setBounds(pitchShiftSlider.getX(), pitchShiftSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    fineTuneSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    fineTuneLabel.setBounds(fineTuneSlider.getX(), fineTuneSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    formantShiftSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    formantShiftLabel.setBounds(formantShiftSlider.getX(), formantShiftSlider.getBottom(), sliderWidth, 20);
    controlsRow.removeFromLeft(spacing);
    
    mixSlider.setBounds(controlsRow.removeFromLeft(sliderWidth).reduced(0, 15));
    mixLabel.setBounds(mixSlider.getX(), mixSlider.getBottom(), sliderWidth, 20);
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
    
    // Pitch meter on right (matching height)
    auto meterBounds = bottomArea.withX(xyPadBounds.getRight() + 20).withWidth(meterSize).withHeight(panelHeight);
    pitchMeter.setBounds(meterBounds);
    
    // Align labels at the same Y position
    auto labelY = xyPadBounds.getBottom() + 5;
    xyPadLabel.setBounds(xyPad.getX(), labelY, xyPadWidth, 20);
    pitchMeterLabel.setBounds(pitchMeter.getX(), labelY, meterSize, 20);
}

void PitchChangerEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void PitchChangerEditor::updateParameterColors()
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
    
    updateLabelColor(pitchShiftLabel, PitchChangerProcessor::PITCH_SHIFT_ID);
    updateLabelColor(fineTuneLabel, PitchChangerProcessor::FINE_TUNE_ID);
    updateLabelColor(formantShiftLabel, PitchChangerProcessor::FORMANT_SHIFT_ID);
    updateLabelColor(mixLabel, PitchChangerProcessor::MIX_ID);
    updateLabelColor(outputLevelLabel, PitchChangerProcessor::OUTPUT_LEVEL_ID);
}

void PitchChangerEditor::updateXYPadFromParameters()
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

void PitchChangerEditor::updateParametersFromXYPad(float x, float y)
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

void PitchChangerEditor::showParameterMenu(juce::Label* label, const juce::String& parameterID)
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
                    xParameterIDs.add(PitchChangerProcessor::PITCH_SHIFT_ID);
                    
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
                    yParameterIDs.add(PitchChangerProcessor::FORMANT_SHIFT_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add(PitchChangerProcessor::PITCH_SHIFT_ID);
                yParameterIDs.add(PitchChangerProcessor::FORMANT_SHIFT_ID);
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void PitchChangerEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == PitchChangerProcessor::PITCH_SHIFT_ID) return "Pitch Shift";
        if (paramID == PitchChangerProcessor::FINE_TUNE_ID) return "Fine Tune";
        if (paramID == PitchChangerProcessor::FORMANT_SHIFT_ID) return "Formant Shift";
        if (paramID == PitchChangerProcessor::MIX_ID) return "Mix";
        if (paramID == PitchChangerProcessor::OUTPUT_LEVEL_ID) return "Output Level";
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