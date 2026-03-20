//==============================================================================
// HyperPrism Reimagined - Vibrato Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "VibratoEditor.h"

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
// VibratoMeter Implementation
//==============================================================================
VibratoMeter::VibratoMeter(VibratoProcessor& p) 
    : processor(p)
{
    startTimerHz(30); // 30 FPS update rate
    
    // Initialize waveform and delay buffer arrays
    lfoWaveform.resize(256, 0.0f);
    delayBuffer.resize(64, 0.0f); // Show delay buffer state
    
    // Initialize parameter values from processor
    auto& apvts = processor.getValueTreeState();
    
    if (auto* rateParam = apvts.getRawParameterValue(VibratoProcessor::RATE_ID))
        rate = rateParam->load();
    if (auto* depthParam = apvts.getRawParameterValue(VibratoProcessor::DEPTH_ID))
        depth = depthParam->load() / 100.0f; // Convert from percentage to 0-1
    if (auto* delayParam = apvts.getRawParameterValue(VibratoProcessor::DELAY_ID))
        delay = delayParam->load();
    if (auto* feedbackParam = apvts.getRawParameterValue(VibratoProcessor::FEEDBACK_ID))
        feedback = feedbackParam->load() / 100.0f; // Convert from percentage to 0-1
}

VibratoMeter::~VibratoMeter()
{
    stopTimer();
}

void VibratoMeter::paint(juce::Graphics& g)
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
    
    // Top section - LFO waveform
    auto lfoArea = displayArea.removeFromTop(displayArea.getHeight() * 0.45f);
    
    // Draw LFO waveform
    g.setColour(HyperPrismLookAndFeel::Colors::primary);
    juce::Path waveformPath;
    
    for (size_t i = 0; i < lfoWaveform.size(); ++i)
    {
        float x = lfoArea.getX() + (i / float(lfoWaveform.size() - 1)) * lfoArea.getWidth();
        float y = lfoArea.getCentreY() - lfoWaveform[i] * lfoArea.getHeight() * 0.3f * depth;
        
        if (i == 0)
            waveformPath.startNewSubPath(x, y);
        else
            waveformPath.lineTo(x, y);
    }
    
    g.strokePath(waveformPath, juce::PathStrokeType(2.0f));
    
    // Draw center line
    g.setColour(HyperPrismLookAndFeel::Colors::outlineVariant.withAlpha(0.5f));
    g.drawHorizontalLine(static_cast<int>(lfoArea.getCentreY()), lfoArea.getX(), lfoArea.getRight());
    
    // Draw current phase indicator
    float phaseX = lfoArea.getX() + currentPhase * lfoArea.getWidth();
    g.setColour(HyperPrismLookAndFeel::Colors::warning);
    g.drawVerticalLine(static_cast<int>(phaseX), lfoArea.getY(), lfoArea.getBottom());
    
    // LFO label
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(9.0f);
    auto lfoLabelY = lfoArea.getBottom() + 2;
    g.drawText("LFO Modulation", lfoArea.withY(lfoLabelY).withHeight(12), juce::Justification::centred);
    
    displayArea.removeFromTop(15); // spacing for label
    
    // Middle section - Delay visualization
    auto delayVisualizationArea = displayArea;
    
    // Draw delay buffer as a circular visualization
    auto centerX = delayVisualizationArea.getCentreX();
    auto centerY = delayVisualizationArea.getCentreY();
    auto radius = juce::jmin(delayVisualizationArea.getWidth(), delayVisualizationArea.getHeight()) * 0.3f;
    
    // Draw outer circle for delay buffer
    g.setColour(HyperPrismLookAndFeel::Colors::outlineVariant);
    g.drawEllipse(centerX - radius, centerY - radius, radius * 2, radius * 2, 1.0f);
    
    // Draw delay buffer contents as dots around the circle
    if (!delayBuffer.empty())
    {
        for (size_t i = 0; i < delayBuffer.size(); ++i)
        {
            float angle = (i / float(delayBuffer.size())) * 2.0f * juce::MathConstants<float>::pi;
            float x = centerX + std::cos(angle) * radius;
            float y = centerY + std::sin(angle) * radius;
            
            float intensity = delayBuffer[i];
            auto dotColor = HyperPrismLookAndFeel::Colors::success.withAlpha(intensity);
            g.setColour(dotColor);
            g.fillEllipse(x - 2, y - 2, 4, 4);
        }
    }
    
    // Draw feedback indicator (inner circle)
    if (feedback > 0.01f)
    {
        float innerRadius = radius * feedback;
        g.setColour(HyperPrismLookAndFeel::Colors::error.withAlpha(0.5f));
        g.fillEllipse(centerX - innerRadius, centerY - innerRadius, innerRadius * 2, innerRadius * 2);
    }
    
    // Delay visualization label
    auto delayLabelY = delayVisualizationArea.getBottom() + 2;
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(9.0f);
    g.drawText("Delay Buffer", delayVisualizationArea.withY(delayLabelY).withHeight(12), juce::Justification::centred);
}

void VibratoMeter::timerCallback()
{
    // Read actual parameter values from the processor
    auto& apvts = processor.getValueTreeState();
    
    if (auto* rateParam = apvts.getRawParameterValue(VibratoProcessor::RATE_ID))
        rate = rateParam->load();
    if (auto* depthParam = apvts.getRawParameterValue(VibratoProcessor::DEPTH_ID))
        depth = depthParam->load() / 100.0f; // Convert from percentage to 0-1
    if (auto* delayParam = apvts.getRawParameterValue(VibratoProcessor::DELAY_ID))
        delay = delayParam->load();
    if (auto* feedbackParam = apvts.getRawParameterValue(VibratoProcessor::FEEDBACK_ID))
        feedback = feedbackParam->load() / 100.0f; // Convert from percentage to 0-1
    
    // Update phase based on actual rate
    // Timer runs at 30Hz, so each callback is ~33.33ms
    float phaseIncrement = rate * (1.0f / 30.0f); // rate in Hz * time in seconds
    currentPhase += phaseIncrement;
    while (currentPhase >= 1.0f)
        currentPhase -= 1.0f;
    
    // Generate LFO waveform (sine wave)
    for (size_t i = 0; i < lfoWaveform.size(); ++i)
    {
        float phase = i / float(lfoWaveform.size());
        lfoWaveform[i] = std::sin(2.0f * juce::MathConstants<float>::pi * phase);
    }
    
    // Simulate delay buffer activity
    for (size_t i = 0; i < delayBuffer.size(); ++i)
    {
        // Create a moving pattern in the delay buffer
        float bufferPhase = (i + currentPhase * delayBuffer.size()) / delayBuffer.size();
        delayBuffer[i] = (std::sin(bufferPhase * 4.0f * juce::MathConstants<float>::pi) + 1.0f) * 0.5f * depth;
    }
    
    repaint();
}

//==============================================================================
// VibratoEditor Implementation
//==============================================================================
VibratoEditor::VibratoEditor(VibratoProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), vibratoMeter(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add(VibratoProcessor::RATE_ID);
    yParameterIDs.add(VibratoProcessor::DEPTH_ID);
    
    // Title
    titleLabel.setText("VIBRATO", juce::dontSendNotification);
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
    setupSlider(mixSlider, mixLabel, "Mix");
    setupSlider(rateSlider, rateLabel, "Rate");
    setupSlider(depthSlider, depthLabel, "Depth");
    setupSlider(delaySlider, delayLabel, "Delay");
    setupSlider(feedbackSlider, feedbackLabel, "Feedback");

    // Color-code knobs by parameter category
    rateSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::modulation);
    depthSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::modulation);
    delaySlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::timing);
    feedbackSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::modulation);
    mixSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::output);
    
    // Set parameter ranges (example ranges - adjust based on processor)
    mixSlider.setRange(0.0, 100.0, 0.1);
    rateSlider.setRange(0.1, 20.0, 0.1);
    depthSlider.setRange(0.0, 100.0, 0.1);
    delaySlider.setRange(0.1, 50.0, 0.1);
    feedbackSlider.setRange(0.0, 95.0, 0.1);
    
    // Set up right-click handlers for parameter assignment
    mixLabel.onClick = [this]() { showParameterMenu(&mixLabel, VibratoProcessor::MIX_ID); };
    rateLabel.onClick = [this]() { showParameterMenu(&rateLabel, VibratoProcessor::RATE_ID); };
    depthLabel.onClick = [this]() { showParameterMenu(&depthLabel, VibratoProcessor::DEPTH_ID); };
    delayLabel.onClick = [this]() { showParameterMenu(&delayLabel, VibratoProcessor::DELAY_ID); };
    feedbackLabel.onClick = [this]() { showParameterMenu(&feedbackLabel, VibratoProcessor::FEEDBACK_ID); };

    // Register right-click on sliders for XY pad assignment
    mixSlider.addMouseListener(this, true);
    mixSlider.getProperties().set("xyParamID", VibratoProcessor::MIX_ID);
    rateSlider.addMouseListener(this, true);
    rateSlider.getProperties().set("xyParamID", VibratoProcessor::RATE_ID);
    depthSlider.addMouseListener(this, true);
    depthSlider.getProperties().set("xyParamID", VibratoProcessor::DEPTH_ID);
    delaySlider.addMouseListener(this, true);
    delaySlider.getProperties().set("xyParamID", VibratoProcessor::DELAY_ID);
    feedbackSlider.addMouseListener(this, true);
    feedbackSlider.getProperties().set("xyParamID", VibratoProcessor::FEEDBACK_ID);

    
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
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, VibratoProcessor::MIX_ID, mixSlider);
    rateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, VibratoProcessor::RATE_ID, rateSlider);
    depthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, VibratoProcessor::DEPTH_ID, depthSlider);
    delayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, VibratoProcessor::DELAY_ID, delaySlider);
    feedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, VibratoProcessor::FEEDBACK_ID, feedbackSlider);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, VibratoProcessor::BYPASS_ID, bypassButton);
    
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

    // Add vibrato meter
    addAndMakeVisible(vibratoMeter);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    mixSlider.onValueChange = [this] { 
        updateXYPadFromParameters(); 
        vibratoMeter.repaint(); 
    };
    rateSlider.onValueChange = [this] { 
        updateXYPadFromParameters(); 
        vibratoMeter.repaint(); 
    };
    depthSlider.onValueChange = [this] { 
        updateXYPadFromParameters(); 
        vibratoMeter.repaint(); 
    };
    delaySlider.onValueChange = [this] { 
        updateXYPadFromParameters(); 
        vibratoMeter.repaint(); 
    };
    feedbackSlider.onValueChange = [this] { 
        updateXYPadFromParameters(); 
        vibratoMeter.repaint(); 
    };
    
    // Tooltips
    rateSlider.setTooltip("Speed of the pitch modulation");
    depthSlider.setTooltip("Amount of pitch variation");
    delaySlider.setTooltip("Base delay time that the vibrato modulates around");
    feedbackSlider.setTooltip("Feeds signal back for a more complex modulated sound");
    mixSlider.setTooltip("Balance between dry and vibrato signal");
    bypassButton.setTooltip("Bypass the effect");

    setSize(700, 550);
    setResizable(true, true);
    setResizeLimits(600, 520, 900, 750);
}

VibratoEditor::~VibratoEditor()
{
    setLookAndFeel(nullptr);
}

void VibratoEditor::paint(juce::Graphics& g)
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

    paintColumnHeader(rateSlider.getX() - 2, rateSlider.getY() - 20, 120,
                      "MODULATION", HyperPrismLookAndFeel::Colors::modulation);
    paintColumnHeader(delaySlider.getX() - 2, delaySlider.getY() - 20, 120,
                      "TIMING", HyperPrismLookAndFeel::Colors::timing);

    // Output section header
    paintColumnHeader(outputSectionX, outputSectionY,
                      getWidth() - outputSectionX - 12,
                      "OUTPUT", HyperPrismLookAndFeel::Colors::output);
}

void VibratoEditor::resized()
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

    // --- Left: Two parameter columns (dynamic width) ---
    int rightSideWidth = 312;
    int columnsTotalWidth = bounds.getWidth() - rightSideWidth;
    auto columnsArea = bounds.removeFromLeft(columnsTotalWidth);
    int colWidth = (columnsArea.getWidth() - 10) / 2;
    auto col1 = columnsArea.removeFromLeft(colWidth);
    columnsArea.removeFromLeft(10);
    auto col2 = columnsArea;

    int knobDiam = 80;
    int vSpace = 107;
    int colTop = col1.getY() + 20;

    auto centerKnob = [&](juce::Slider& slider, juce::Label& label,
                           int colX, int colW, int cy, int kd)
    {
        int kx = colX + (colW - kd) / 2;
        int ky = cy - kd / 2;
        slider.setBounds(kx, ky, kd, kd);
        label.setBounds(colX, ky + kd + 1, colW, 16);
    };

    // Column 1: MODULATION -- Rate, Depth, Feedback
    int y1 = colTop + knobDiam / 2;
    centerKnob(rateSlider, rateLabel, col1.getX(), colWidth, y1, knobDiam);
    centerKnob(depthSlider, depthLabel, col1.getX(), colWidth, y1 + vSpace, knobDiam);
    centerKnob(feedbackSlider, feedbackLabel, col1.getX(), colWidth, y1 + vSpace * 2, knobDiam);

    // Column 2: TIMING -- Delay
    centerKnob(delaySlider, delayLabel, col2.getX(), colWidth, y1, knobDiam);

    // --- Right side: XY pad + output + meter ---
    auto rightSide = bounds;
    rightSide.removeFromLeft(12);

    int outputHeight = 130;
    int xyHeight = juce::jmax(200, rightSide.getHeight() - outputHeight - 22);
    auto xyArea = rightSide.removeFromTop(xyHeight);
    xyPad.setBounds(xyArea);
    xyPadLabel.setBounds(xyArea.getX(), xyArea.getBottom() + 2, xyArea.getWidth(), 16);
    rightSide.removeFromTop(20);

    // Bottom right: Output knob + Meter
    auto bottomRight = rightSide;
    auto outputArea = bottomRight.removeFromLeft(140);
    auto meterArea = bottomRight.reduced(4);

    int outKnob = 58;
    int outY = outputArea.getY() + 24;

    centerKnob(mixSlider, mixLabel, outputArea.getX() + 20, 100, outY + outKnob / 2, outKnob);

    outputSectionX = outputArea.getX();
    outputSectionY = outputArea.getY();

    // Meter
    vibratoMeter.setBounds(meterArea.reduced(4));
}

void VibratoEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void VibratoEditor::updateParameterColors()
{
    auto neutralColor = HyperPrismLookAndFeel::Colors::onSurfaceVariant;
    mixLabel.setColour(juce::Label::textColourId, neutralColor);
    rateLabel.setColour(juce::Label::textColourId, neutralColor);
    depthLabel.setColour(juce::Label::textColourId, neutralColor);
    delayLabel.setColour(juce::Label::textColourId, neutralColor);
    feedbackLabel.setColour(juce::Label::textColourId, neutralColor);

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

    updateSliderXY(mixSlider, VibratoProcessor::MIX_ID);
    updateSliderXY(rateSlider, VibratoProcessor::RATE_ID);
    updateSliderXY(depthSlider, VibratoProcessor::DEPTH_ID);
    updateSliderXY(delaySlider, VibratoProcessor::DELAY_ID);
    updateSliderXY(feedbackSlider, VibratoProcessor::FEEDBACK_ID);
    repaint();
}

void VibratoEditor::updateXYPadFromParameters()
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

void VibratoEditor::updateParametersFromXYPad(float x, float y)
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


void VibratoEditor::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        auto* source = event.eventComponent;
        auto paramID = source->getProperties()["xyParamID"].toString();
        if (paramID.isNotEmpty())
            showParameterMenu(source, paramID);
    }
}
void VibratoEditor::showParameterMenu(juce::Component* target, const juce::String& parameterID)
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
                    xParameterIDs.add(VibratoProcessor::RATE_ID);
                    
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
                    yParameterIDs.add(VibratoProcessor::DEPTH_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add(VibratoProcessor::RATE_ID);
                yParameterIDs.add(VibratoProcessor::DEPTH_ID);
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void VibratoEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == VibratoProcessor::MIX_ID) return "Mix";
        if (paramID == VibratoProcessor::RATE_ID) return "Rate";
        if (paramID == VibratoProcessor::DEPTH_ID) return "Depth";
        if (paramID == VibratoProcessor::DELAY_ID) return "Delay";
        if (paramID == VibratoProcessor::FEEDBACK_ID) return "Feedback";
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