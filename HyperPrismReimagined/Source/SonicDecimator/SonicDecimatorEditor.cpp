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
    
    // Title
    titleLabel.setText("SONIC DECIMATOR", juce::dontSendNotification);
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
    setupSlider(bitDepthSlider, bitDepthLabel, "Bit Depth");
    setupSlider(sampleRateSlider, sampleRateLabel, "Sample Rate");
    setupSlider(mixSlider, mixLabel, "Mix");
    setupSlider(outputLevelSlider, outputLevelLabel, "Output");

    // Color-code knobs by category
    bitDepthSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::dynamics);
    sampleRateSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::dynamics);
    mixSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::output);
    outputLevelSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::output);
    
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

    // Register right-click on sliders for XY pad assignment
    bitDepthSlider.addMouseListener(this, true);
    bitDepthSlider.getProperties().set("xyParamID", SonicDecimatorProcessor::BIT_DEPTH_ID);
    sampleRateSlider.addMouseListener(this, true);
    sampleRateSlider.getProperties().set("xyParamID", SonicDecimatorProcessor::SAMPLE_RATE_ID);
    mixSlider.addMouseListener(this, true);
    mixSlider.getProperties().set("xyParamID", SonicDecimatorProcessor::MIX_ID);
    outputLevelSlider.addMouseListener(this, true);
    outputLevelSlider.getProperties().set("xyParamID", SonicDecimatorProcessor::OUTPUT_LEVEL_ID);

    
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
    xyPadLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    xyPad.setTooltip("Click and drag to control assigned parameters. Right-click parameter labels to assign X/Y axes.");

    // Add decimation meter
    addAndMakeVisible(decimationMeter);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    bitDepthSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    sampleRateSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    mixSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    outputLevelSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    // Tooltips
    bitDepthSlider.setTooltip("Reduces bit depth for lo-fi digital distortion");
    sampleRateSlider.setTooltip("Reduces sample rate for aliasing and crushed sound");
    mixSlider.setTooltip("Balance between clean and decimated signal");
    outputLevelSlider.setTooltip("Overall output volume");
    bypassButton.setTooltip("Bypass the effect");

    setSize(700, 550);
    setResizable(true, true);
    setResizeLimits(600, 520, 900, 750);
}

SonicDecimatorEditor::~SonicDecimatorEditor()
{
    setLookAndFeel(nullptr);
}

void SonicDecimatorEditor::paint(juce::Graphics& g)
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

    paintColumnHeader(bitDepthSlider.getX() - 2, bitDepthSlider.getY() - 20, 200,
                      "DECIMATION", HyperPrismLookAndFeel::Colors::dynamics);
    paintColumnHeader(outputSectionX, outputSectionY, 140,
                      "OUTPUT", HyperPrismLookAndFeel::Colors::output);
}

void SonicDecimatorEditor::resized()
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

    // Column: DECIMATION -- Bit Depth, Sample Rate
    int y1 = colTop + knobDiam / 2;
    centerKnob(bitDepthSlider, bitDepthLabel, col1.getX(), colWidth, y1, knobDiam);
    centerKnob(sampleRateSlider, sampleRateLabel, col1.getX(), colWidth, y1 + vSpace, knobDiam);

    // Toggle buttons below knobs in column
    int toggleY = y1 + vSpace + knobDiam / 2 + 36;
    antiAliasButton.setBounds(col1.getX(), toggleY, colWidth, 25);
    ditherButton.setBounds(col1.getX(), toggleY + 45, colWidth, 25);

    // --- Right side: XY pad + output ---
    auto rightSide = bounds;
    rightSide.removeFromLeft(12);

    int outputHeight = 130;
    int xyHeight = juce::jmax(200, rightSide.getHeight() - outputHeight - 22);
    auto xyArea = rightSide.removeFromTop(xyHeight);
    xyPad.setBounds(xyArea);
    xyPadLabel.setBounds(xyArea.getX(), xyArea.getBottom() + 2, xyArea.getWidth(), 16);
    rightSide.removeFromTop(20);

    // Output section: Output Level + Mix knobs side-by-side + meter
    auto bottomRight = rightSide;
    outputSectionX = bottomRight.getX();
    outputSectionY = bottomRight.getY();

    auto outputArea = bottomRight.removeFromLeft(180);
    auto meterArea = bottomRight;

    int outKnob = 54;
    int outY = outputArea.getY() + 24;
    centerKnob(outputLevelSlider, outputLevelLabel, outputArea.getX(), 90, outY + outKnob / 2, outKnob);
    centerKnob(mixSlider, mixLabel, outputArea.getX() + 90, 90, outY + outKnob / 2, outKnob);

    decimationMeter.setBounds(meterArea.reduced(4));
}

void SonicDecimatorEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void SonicDecimatorEditor::setupToggleButton(juce::ToggleButton& button, ParameterLabel& label, 
                                            const juce::String& text)
{
    button.setButtonText(text);
    button.setColour(juce::ToggleButton::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    button.setColour(juce::ToggleButton::tickColourId, HyperPrismLookAndFeel::Colors::primary);
    button.setColour(juce::ToggleButton::tickDisabledColourId, HyperPrismLookAndFeel::Colors::surfaceVariant);
    addAndMakeVisible(button);
}

void SonicDecimatorEditor::updateParameterColors()
{
    auto neutralColor = HyperPrismLookAndFeel::Colors::onSurfaceVariant;
    bitDepthLabel.setColour(juce::Label::textColourId, neutralColor);
    sampleRateLabel.setColour(juce::Label::textColourId, neutralColor);
    mixLabel.setColour(juce::Label::textColourId, neutralColor);
    outputLevelLabel.setColour(juce::Label::textColourId, neutralColor);
    antiAliasLabel.setColour(juce::Label::textColourId, neutralColor);
    ditherLabel.setColour(juce::Label::textColourId, neutralColor);

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

    updateSliderXY(bitDepthSlider, SonicDecimatorProcessor::BIT_DEPTH_ID);
    updateSliderXY(sampleRateSlider, SonicDecimatorProcessor::SAMPLE_RATE_ID);
    updateSliderXY(mixSlider, SonicDecimatorProcessor::MIX_ID);
    updateSliderXY(outputLevelSlider, SonicDecimatorProcessor::OUTPUT_LEVEL_ID);
    repaint();
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


void SonicDecimatorEditor::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        auto* source = event.eventComponent;
        auto paramID = source->getProperties()["xyParamID"].toString();
        if (paramID.isNotEmpty())
            showParameterMenu(source, paramID);
    }
}
void SonicDecimatorEditor::showParameterMenu(juce::Component* target, const juce::String& parameterID)
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
        if (paramID == SonicDecimatorProcessor::OUTPUT_LEVEL_ID) return "Output";
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