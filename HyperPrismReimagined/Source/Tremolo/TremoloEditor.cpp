//==============================================================================
// HyperPrism Reimagined - Tremolo Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "TremoloEditor.h"

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
// TremoloMeter Implementation
//==============================================================================
TremoloMeter::TremoloMeter(TremoloProcessor& p) 
    : processor(p)
{
    startTimerHz(30); // 30 FPS update rate
    
    // Initialize LFO waveform array
    lfoWaveform.resize(256);
    
    // Initialize parameter values from processor
    auto& apvts = processor.getValueTreeState();
    
    if (auto* waveformParam = apvts.getRawParameterValue(TremoloProcessor::WAVEFORM_ID))
        waveformType = static_cast<int>(waveformParam->load());
    
    if (auto* rateParam = apvts.getRawParameterValue(TremoloProcessor::RATE_ID))
        rate = rateParam->load();
    
    if (auto* depthParam = apvts.getRawParameterValue(TremoloProcessor::DEPTH_ID))
        depth = depthParam->load() / 100.0f;
}

TremoloMeter::~TremoloMeter()
{
    stopTimer();
}

void TremoloMeter::paint(juce::Graphics& g)
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

    // Waveform type label
    juce::String waveformName;
    switch (waveformType)
    {
        case 0: waveformName = "SINE"; break;
        case 1: waveformName = "TRIANGLE"; break;
        case 2: waveformName = "SQUARE"; break;
        default: waveformName = "UNKNOWN"; break;
    }

    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(10.0f);
    auto labelArea = displayArea.removeFromTop(15);
    g.drawText("LFO: " + waveformName, labelArea, juce::Justification::centredLeft);

    // Use remaining area for waveform visualization
    float waveformHeight = displayArea.getHeight();

    // Draw center line
    g.setColour(HyperPrismLookAndFeel::Colors::outlineVariant.withAlpha(0.5f));
    g.drawHorizontalLine(static_cast<int>(displayArea.getCentreY()), displayArea.getX(), displayArea.getRight());

    // Draw LFO waveform
    g.setColour(HyperPrismLookAndFeel::Colors::primary);
    juce::Path waveformPath;

    for (size_t i = 0; i < lfoWaveform.size(); ++i)
    {
        float x = displayArea.getX() + (i / float(lfoWaveform.size() - 1)) * displayArea.getWidth();
        float y = displayArea.getCentreY() - lfoWaveform[i] * waveformHeight * 0.4f * depth;

        if (i == 0)
            waveformPath.startNewSubPath(x, y);
        else
            waveformPath.lineTo(x, y);
    }

    g.strokePath(waveformPath, juce::PathStrokeType(2.0f));

    // Draw current phase indicator
    float phaseX = displayArea.getX() + currentPhase * displayArea.getWidth();
    g.setColour(HyperPrismLookAndFeel::Colors::warning);
    g.drawVerticalLine(static_cast<int>(phaseX), displayArea.getY(), displayArea.getBottom());

    // Draw phase circle
    float phaseY = displayArea.getCentreY() - lfoWaveform[static_cast<int>(currentPhase * (lfoWaveform.size() - 1))] * waveformHeight * 0.4f * depth;
    g.fillEllipse(phaseX - 4, phaseY - 4, 8, 8);
}

void TremoloMeter::timerCallback()
{
    // Get current parameter values from processor
    auto& apvts = processor.getValueTreeState();
    
    // Get waveform type (ComboBox uses 1-based indexing, we need 0-based)
    if (auto* waveformParam = apvts.getRawParameterValue(TremoloProcessor::WAVEFORM_ID))
        waveformType = static_cast<int>(waveformParam->load());
    
    // Get rate
    if (auto* rateParam = apvts.getRawParameterValue(TremoloProcessor::RATE_ID))
        rate = rateParam->load();
    
    // Get depth (0-100 range, normalize to 0-1)
    if (auto* depthParam = apvts.getRawParameterValue(TremoloProcessor::DEPTH_ID))
        depth = depthParam->load() / 100.0f;
    
    // Simulate phase advancement based on rate
    currentPhase += rate * 0.01f; // Adjust speed based on rate
    if (currentPhase >= 1.0f)
        currentPhase -= 1.0f;
    
    // Generate waveform based on type
    for (size_t i = 0; i < lfoWaveform.size(); ++i)
    {
        float phase = i / float(lfoWaveform.size());
        
        switch (waveformType)
        {
            case 0: // Sine
                lfoWaveform[i] = std::sin(2.0f * juce::MathConstants<float>::pi * phase);
                break;
                
            case 1: // Triangle
                if (phase < 0.5f)
                    lfoWaveform[i] = 4.0f * phase - 1.0f;
                else
                    lfoWaveform[i] = 3.0f - 4.0f * phase;
                break;
                
            case 2: // Square
                lfoWaveform[i] = phase < 0.5f ? 1.0f : -1.0f;
                break;
        }
    }
    
    repaint();
}

//==============================================================================
// TremoloEditor Implementation
//==============================================================================
TremoloEditor::TremoloEditor(TremoloProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), tremoloMeter(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add(TremoloProcessor::RATE_ID);
    yParameterIDs.add(TremoloProcessor::DEPTH_ID);
    
    // Title
    titleLabel.setText("TREMOLO", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(16.0f).withStyle("Bold")));
    titleLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurface);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    brandLabel.setText("HyperPrism Reimagined", juce::dontSendNotification);
    brandLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    brandLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    brandLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(brandLabel);

    // Setup sliders with consistent style
    setupSlider(rateSlider, rateLabel, "Rate");
    setupSlider(depthSlider, depthLabel, "Depth");
    setupSlider(stereoPhaseSlider, stereoPhaseLabel, "Stereo Phase");
    setupSlider(mixSlider, mixLabel, "Mix");

    // Color-code knobs by parameter category
    rateSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::modulation);
    depthSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::modulation);
    stereoPhaseSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::modulation);
    mixSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::output);
    
    // Set parameter ranges (example ranges - adjust based on processor)
    rateSlider.setRange(0.1, 20.0, 0.1);
    depthSlider.setRange(0.0, 100.0, 0.1);
    stereoPhaseSlider.setRange(0.0, 180.0, 1.0);
    mixSlider.setRange(0.0, 100.0, 0.1);
    
    // Setup ComboBox
    setupComboBox(waveformBox, waveformLabel, "Waveform");
    
    // Add waveform options
    waveformBox.addItem("Sine", 1);
    waveformBox.addItem("Triangle", 2);
    waveformBox.addItem("Square", 3);
    
    // Set up right-click handlers for parameter assignment
    rateLabel.onClick = [this]() { showParameterMenu(&rateLabel, TremoloProcessor::RATE_ID); };
    depthLabel.onClick = [this]() { showParameterMenu(&depthLabel, TremoloProcessor::DEPTH_ID); };
    stereoPhaseLabel.onClick = [this]() { showParameterMenu(&stereoPhaseLabel, TremoloProcessor::STEREO_PHASE_ID); };
    mixLabel.onClick = [this]() { showParameterMenu(&mixLabel, TremoloProcessor::MIX_ID); };
    waveformLabel.onClick = [this]() { showParameterMenu(&waveformLabel, TremoloProcessor::WAVEFORM_ID); };

    // Register right-click on sliders for XY pad assignment
    rateSlider.addMouseListener(this, true);
    rateSlider.getProperties().set("xyParamID", TremoloProcessor::RATE_ID);
    depthSlider.addMouseListener(this, true);
    depthSlider.getProperties().set("xyParamID", TremoloProcessor::DEPTH_ID);
    stereoPhaseSlider.addMouseListener(this, true);
    stereoPhaseSlider.getProperties().set("xyParamID", TremoloProcessor::STEREO_PHASE_ID);
    mixSlider.addMouseListener(this, true);
    mixSlider.getProperties().set("xyParamID", TremoloProcessor::MIX_ID);

    
    // Bypass button (top right like AutoPan)
    bypassButton.setButtonText("Bypass");
    bypassButton.setClickingTogglesState(true);
    bypassButton.setColour(juce::TextButton::buttonOnColourId,
                            HyperPrismLookAndFeel::Colors::error.withAlpha(0.6f));
    bypassButton.setColour(juce::TextButton::textColourOnId,
                            HyperPrismLookAndFeel::Colors::onSurface);
    addAndMakeVisible(bypassButton);
    
    // Create attachments
    auto& apvts = audioProcessor.getValueTreeState();
    rateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, TremoloProcessor::RATE_ID, rateSlider);
    depthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, TremoloProcessor::DEPTH_ID, depthSlider);
    stereoPhaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, TremoloProcessor::STEREO_PHASE_ID, stereoPhaseSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, TremoloProcessor::MIX_ID, mixSlider);
    waveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, TremoloProcessor::WAVEFORM_ID, waveformBox);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, TremoloProcessor::BYPASS_ID, bypassButton);
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);
    xyPadLabel.setText("Rate / Depth", juce::dontSendNotification);
    xyPadLabel.setJustificationType(juce::Justification::centred);
    xyPadLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    xyPad.setTooltip("Click and drag to control assigned parameters. Right-click parameter labels to assign X/Y axes.");

    // Add tremolo meter
    addAndMakeVisible(tremoloMeter);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    rateSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    depthSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    stereoPhaseSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    mixSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    // Tooltips
    rateSlider.setTooltip("Speed of the volume modulation");
    depthSlider.setTooltip("Intensity of the volume change");
    stereoPhaseSlider.setTooltip("Phase offset between left and right for stereo tremolo");
    waveformBox.setTooltip("Shape of the modulation waveform");
    mixSlider.setTooltip("Balance between dry and tremolo signal");
    bypassButton.setTooltip("Bypass the effect");

    setSize(700, 550);
    setResizable(true, true);
    setResizeLimits(600, 520, 900, 750);
}

TremoloEditor::~TremoloEditor()
{
    setLookAndFeel(nullptr);
}

void TremoloEditor::paint(juce::Graphics& g)
{
    g.fillAll(HyperPrismLookAndFeel::Colors::background);
    g.setColour(HyperPrismLookAndFeel::Colors::primary.withAlpha(0.4f));
    g.fillRect(12, 4, getWidth() - 24, 2);
    g.setColour(HyperPrismLookAndFeel::Colors::outline);
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.drawText("v1.0.0", getLocalBounds().removeFromBottom(20).removeFromRight(70),
               juce::Justification::centredRight);

    auto paintColumnHeader = [&](int x, int y, int width,
                                  const juce::String& title, juce::Colour color) {
        g.setColour(color.withAlpha(0.7f));
        g.setFont(juce::Font(juce::FontOptions(9.0f).withStyle("Bold")));
        g.drawText(title, x, y, width, 14, juce::Justification::centredLeft);
        g.setColour(HyperPrismLookAndFeel::Colors::outline.withAlpha(0.3f));
        g.drawLine(static_cast<float>(x), static_cast<float>(y + 14),
                   static_cast<float>(x + width), static_cast<float>(y + 14), 0.5f);
    };

    paintColumnHeader(rateSlider.getX() - 2, rateSlider.getY() - 20, 200,
                      "MODULATION", HyperPrismLookAndFeel::Colors::modulation);
    paintColumnHeader(outputSectionX, outputSectionY, 140,
                      "OUTPUT", HyperPrismLookAndFeel::Colors::output);
}

void TremoloEditor::resized()
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

    // --- Left: 1 column with larger knobs ---
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

    // Column: MODULATION -- Rate, Depth, Stereo Phase
    int y1 = colTop + knobDiam / 2;
    centerKnob(rateSlider, rateLabel, col1.getX(), colWidth, y1, knobDiam);
    centerKnob(depthSlider, depthLabel, col1.getX(), colWidth, y1 + vSpace, knobDiam);
    centerKnob(stereoPhaseSlider, stereoPhaseLabel, col1.getX(), colWidth, y1 + vSpace * 2, knobDiam);

    // Waveform ComboBox below knobs in column
    int comboY = y1 + vSpace * 2 + knobDiam / 2 + 36;
    waveformLabel.setBounds(col1.getX(), comboY, colWidth, 14);
    waveformBox.setBounds(col1.getX() + 5, comboY + 15, colWidth - 10, 24);

    // --- Right side: XY pad + output ---
    auto rightSide = bounds;
    rightSide.removeFromLeft(12);

    int outputHeight = 130;
    int xyHeight = juce::jmax(200, rightSide.getHeight() - outputHeight - 22);
    auto xyArea = rightSide.removeFromTop(xyHeight);
    xyPad.setBounds(xyArea);
    xyPadLabel.setBounds(xyArea.getX(), xyArea.getBottom() + 2, xyArea.getWidth(), 16);
    rightSide.removeFromTop(20);

    // Output section: Mix knob + meter side by side
    auto bottomRight = rightSide;
    outputSectionX = bottomRight.getX();
    outputSectionY = bottomRight.getY();
    auto outputArea = bottomRight.removeFromLeft(140);
    int outKnob = 58;
    int outY = outputArea.getY() + 24;
    centerKnob(mixSlider, mixLabel, outputArea.getCentreX() - 50, 100, outY + outKnob / 2, outKnob);

    // Tremolo meter in remaining output area
    auto meterArea = bottomRight.reduced(4);
    tremoloMeter.setBounds(meterArea.reduced(4));
}

void TremoloEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void TremoloEditor::setupComboBox(juce::ComboBox& comboBox, ParameterLabel& label, 
                                 const juce::String& text)
{
    comboBox.setColour(juce::ComboBox::backgroundColourId, HyperPrismLookAndFeel::Colors::surfaceVariant);
    comboBox.setColour(juce::ComboBox::textColourId, HyperPrismLookAndFeel::Colors::onSurface);
    comboBox.setColour(juce::ComboBox::arrowColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    comboBox.setColour(juce::ComboBox::outlineColourId, HyperPrismLookAndFeel::Colors::outline);
    addAndMakeVisible(comboBox);
    
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    addAndMakeVisible(label);
}

void TremoloEditor::updateParameterColors()
{
    auto neutralColor = HyperPrismLookAndFeel::Colors::onSurfaceVariant;
    rateLabel.setColour(juce::Label::textColourId, neutralColor);
    depthLabel.setColour(juce::Label::textColourId, neutralColor);
    stereoPhaseLabel.setColour(juce::Label::textColourId, neutralColor);
    mixLabel.setColour(juce::Label::textColourId, neutralColor);
    waveformLabel.setColour(juce::Label::textColourId, neutralColor);

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

    updateSliderXY(rateSlider, TremoloProcessor::RATE_ID);
    updateSliderXY(depthSlider, TremoloProcessor::DEPTH_ID);
    updateSliderXY(stereoPhaseSlider, TremoloProcessor::STEREO_PHASE_ID);
    updateSliderXY(mixSlider, TremoloProcessor::MIX_ID);
    repaint();
}

void TremoloEditor::updateXYPadFromParameters()
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

void TremoloEditor::updateParametersFromXYPad(float x, float y)
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


void TremoloEditor::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        auto* source = event.eventComponent;
        auto paramID = source->getProperties()["xyParamID"].toString();
        if (paramID.isNotEmpty())
            showParameterMenu(source, paramID);
    }
}
void TremoloEditor::showParameterMenu(juce::Component* target, const juce::String& parameterID)
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
                    xParameterIDs.add(TremoloProcessor::RATE_ID);
                    
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
                    yParameterIDs.add(TremoloProcessor::DEPTH_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add(TremoloProcessor::RATE_ID);
                yParameterIDs.add(TremoloProcessor::DEPTH_ID);
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void TremoloEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == TremoloProcessor::RATE_ID) return "Rate";
        if (paramID == TremoloProcessor::DEPTH_ID) return "Depth";
        if (paramID == TremoloProcessor::STEREO_PHASE_ID) return "Stereo Phase";
        if (paramID == TremoloProcessor::MIX_ID) return "Mix";
        if (paramID == TremoloProcessor::WAVEFORM_ID) return "Waveform";
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