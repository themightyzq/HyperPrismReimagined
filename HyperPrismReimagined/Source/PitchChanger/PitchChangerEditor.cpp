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
    
    // Title
    titleLabel.setText("PITCH CHANGER", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(16.0f).withStyle("Bold")));
    titleLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurface);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    brandLabel.setText("HyperPrism Reimagined", juce::dontSendNotification);
    brandLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    brandLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    brandLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(brandLabel);
    
    // Setup sliders with consistent style (5 parameters)
    setupSlider(pitchShiftSlider, pitchShiftLabel, "Pitch Shift");
    setupSlider(fineTuneSlider, fineTuneLabel, "Fine Tune");
    setupSlider(formantShiftSlider, formantShiftLabel, "Formant Shift");
    setupSlider(mixSlider, mixLabel, "Mix");
    setupSlider(outputLevelSlider, outputLevelLabel, "Output");

    // Color-code knobs by category
    pitchShiftSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::frequency);
    fineTuneSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::frequency);
    formantShiftSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::frequency);
    mixSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::output);
    outputLevelSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::output);
    
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

    // Register right-click on sliders for XY pad assignment
    pitchShiftSlider.addMouseListener(this, true);
    pitchShiftSlider.getProperties().set("xyParamID", PitchChangerProcessor::PITCH_SHIFT_ID);
    fineTuneSlider.addMouseListener(this, true);
    fineTuneSlider.getProperties().set("xyParamID", PitchChangerProcessor::FINE_TUNE_ID);
    formantShiftSlider.addMouseListener(this, true);
    formantShiftSlider.getProperties().set("xyParamID", PitchChangerProcessor::FORMANT_SHIFT_ID);
    mixSlider.addMouseListener(this, true);
    mixSlider.getProperties().set("xyParamID", PitchChangerProcessor::MIX_ID);
    outputLevelSlider.addMouseListener(this, true);
    outputLevelSlider.getProperties().set("xyParamID", PitchChangerProcessor::OUTPUT_LEVEL_ID);

    
    // Bypass button (top right like AutoPan)
    // Bypass button
    bypassButton.setButtonText("Bypass");
    bypassButton.setClickingTogglesState(true);
    bypassButton.setColour(juce::TextButton::buttonOnColourId,
                            HyperPrismLookAndFeel::Colors::error.withAlpha(0.6f));
    bypassButton.setColour(juce::TextButton::textColourOnId,
                            HyperPrismLookAndFeel::Colors::onSurface);
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
    xyPadLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    xyPad.setTooltip("Click and drag to control assigned parameters. Right-click parameter labels to assign X/Y axes.");

    // Add pitch meter
    addAndMakeVisible(pitchMeter);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    pitchShiftSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    fineTuneSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    formantShiftSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    mixSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    outputLevelSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    // Tooltips
    pitchShiftSlider.setTooltip("Pitch shift amount in semitones");
    fineTuneSlider.setTooltip("Fine pitch adjustment in cents (1/100th of a semitone)");
    formantShiftSlider.setTooltip("Shifts vocal formants independently of pitch");
    mixSlider.setTooltip("Balance between dry and pitch-shifted signal");
    outputLevelSlider.setTooltip("Overall output volume");
    bypassButton.setTooltip("Bypass the effect");

    setSize(700, 550);
    setResizable(true, true);
    setResizeLimits(600, 520, 900, 750);
}

PitchChangerEditor::~PitchChangerEditor()
{
    setLookAndFeel(nullptr);
}

void PitchChangerEditor::paint(juce::Graphics& g)
{
    g.fillAll(HyperPrismLookAndFeel::Colors::background);

    // Accent line
    g.setColour(HyperPrismLookAndFeel::Colors::primary.withAlpha(0.4f));
    g.fillRect(12, 4, getWidth() - 24, 2);

    // Version
    g.setColour(HyperPrismLookAndFeel::Colors::outline);
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.drawText("v1.0.0", getLocalBounds().removeFromBottom(20).removeFromRight(70),
               juce::Justification::centredRight);

    // Column section headers
    auto paintColumnHeader = [&](int x, int y, int width,
                                  const juce::String& title, juce::Colour color)
    {
        g.setColour(color.withAlpha(0.7f));
        g.setFont(juce::Font(juce::FontOptions(9.0f).withStyle("Bold")));
        g.drawText(title, x, y, width, 14, juce::Justification::centredLeft);
        g.setColour(HyperPrismLookAndFeel::Colors::outline.withAlpha(0.3f));
        g.drawLine(static_cast<float>(x), static_cast<float>(y + 14),
                   static_cast<float>(x + width), static_cast<float>(y + 14), 0.5f);
    };

    paintColumnHeader(pitchShiftSlider.getX() - 2, pitchShiftSlider.getY() - 20, 120,
                      "PITCH", HyperPrismLookAndFeel::Colors::frequency);

    // Output section header
    paintColumnHeader(outputSectionX, outputSectionY,
                      getWidth() - outputSectionX - 12,
                      "OUTPUT", HyperPrismLookAndFeel::Colors::output);
}

void PitchChangerEditor::resized()
{
    auto bounds = getLocalBounds();

    // === HEADER (72px) ===
    auto header = bounds.removeFromTop(72);
    titleLabel.setBounds(header.getX() + 12, 30, header.getWidth() - 112, 20);
    brandLabel.setBounds(header.getX() + 12, 50, header.getWidth() - 112, 16);
    bypassButton.setBounds(header.getRight() - 90, 36, 80, 26);

    // === FOOTER ===
    bounds.removeFromBottom(20);

    // === CONTENT ===
    bounds.reduce(12, 4);

    // --- Left: Single centered column ---
    int rightSideWidth = 312;
    int columnsTotalWidth = bounds.getWidth() - rightSideWidth;
    auto columnsArea = bounds.removeFromLeft(columnsTotalWidth);
    int colWidth = 200;
    int colOffset = (columnsArea.getWidth() - colWidth) / 2;
    columnsArea.removeFromLeft(colOffset);
    auto col1 = columnsArea.removeFromLeft(colWidth);

    int knobDiam = 84;
    int vSpace = 111;
    int colTop = col1.getY() + 20;

    auto centerKnob = [&](juce::Slider& slider, juce::Label& label,
                           int colX, int colW, int cy, int kd)
    {
        int kx = colX + (colW - kd) / 2;
        int ky = cy - kd / 2;
        slider.setBounds(kx, ky, kd, kd);
        label.setBounds(colX, ky + kd + 1, colW, 16);
    };

    // Single Column: PITCH -- Pitch Shift, Fine Tune, Formant Shift
    int y1 = colTop + knobDiam / 2;
    centerKnob(pitchShiftSlider, pitchShiftLabel, col1.getX(), colWidth, y1, knobDiam);
    centerKnob(fineTuneSlider, fineTuneLabel, col1.getX(), colWidth, y1 + vSpace, knobDiam);
    centerKnob(formantShiftSlider, formantShiftLabel, col1.getX(), colWidth, y1 + vSpace * 2, knobDiam);

    // --- Right side: XY pad + output + meter ---
    auto rightSide = bounds;
    rightSide.removeFromLeft(12);

    int outputHeight = 130;
    int xyHeight = juce::jmax(200, rightSide.getHeight() - outputHeight - 22);
    auto xyArea = rightSide.removeFromTop(xyHeight);
    xyPad.setBounds(xyArea);
    xyPadLabel.setBounds(xyArea.getX(), xyArea.getBottom() + 2, xyArea.getWidth(), 16);
    rightSide.removeFromTop(20);

    // Bottom right: Output knobs side-by-side + Meter
    auto bottomRight = rightSide;
    outputSectionX = bottomRight.getX();
    outputSectionY = bottomRight.getY();

    auto outputArea = bottomRight.removeFromLeft(180);
    auto meterArea = bottomRight;

    int outKnob = 54;
    int outY = outputArea.getY() + 24;
    centerKnob(outputLevelSlider, outputLevelLabel, outputArea.getX(), 90, outY + outKnob / 2, outKnob);
    centerKnob(mixSlider, mixLabel, outputArea.getX() + 90, 90, outY + outKnob / 2, outKnob);

    pitchMeter.setBounds(meterArea.reduced(4));
}

void PitchChangerEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
                               const juce::String& text)
{
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    slider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::primary);
        
    addAndMakeVisible(slider);
    
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    addAndMakeVisible(label);
}

void PitchChangerEditor::updateParameterColors()
{
    auto neutralColor = HyperPrismLookAndFeel::Colors::onSurfaceVariant;
    pitchShiftLabel.setColour(juce::Label::textColourId, neutralColor);
    fineTuneLabel.setColour(juce::Label::textColourId, neutralColor);
    formantShiftLabel.setColour(juce::Label::textColourId, neutralColor);
    mixLabel.setColour(juce::Label::textColourId, neutralColor);
    outputLevelLabel.setColour(juce::Label::textColourId, neutralColor);

    // Set XY assignment properties on sliders for LookAndFeel badge drawing
    auto updateSliderXY = [this](juce::Slider& slider, const juce::String& paramID)
    {
        if (xParameterIDs.contains(paramID))
            slider.getProperties().set("xyAxisX", true);
        else
            slider.getProperties().remove("xyAxisX");

        if (yParameterIDs.contains(paramID))
            slider.getProperties().set("xyAxisY", true);
        else
            slider.getProperties().remove("xyAxisY");

        slider.repaint();
    };

    updateSliderXY(pitchShiftSlider, PitchChangerProcessor::PITCH_SHIFT_ID);
    updateSliderXY(fineTuneSlider, PitchChangerProcessor::FINE_TUNE_ID);
    updateSliderXY(formantShiftSlider, PitchChangerProcessor::FORMANT_SHIFT_ID);
    updateSliderXY(mixSlider, PitchChangerProcessor::MIX_ID);
    updateSliderXY(outputLevelSlider, PitchChangerProcessor::OUTPUT_LEVEL_ID);
    repaint();
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


void PitchChangerEditor::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        auto* source = event.eventComponent;
        auto paramID = source->getProperties()["xyParamID"].toString();
        if (paramID.isNotEmpty())
            showParameterMenu(source, paramID);
    }
}
void PitchChangerEditor::showParameterMenu(juce::Component* target, const juce::String& parameterID)
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
        .withTargetComponent(target)
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
        if (paramID == PitchChangerProcessor::OUTPUT_LEVEL_ID) return "Output";
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