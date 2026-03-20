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
    auto harmonicArea = displayArea.removeFromTop(displayArea.getHeight() * 0.6f);
    
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
    g.setFont(juce::Font(juce::FontOptions(12.0f).withStyle("Bold")));
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
    
    // Title
    titleLabel.setText("TUBE/TAPE SATURATION", juce::dontSendNotification);
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
    setupSlider(driveSlider, driveLabel, "Drive");
    setupSlider(warmthSlider, warmthLabel, "Warmth");
    setupSlider(brightnessSlider, brightnessLabel, "Brightness");
    setupSlider(outputLevelSlider, outputLevelLabel, "Output");

    // Color-code knobs by category
    driveSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::dynamics);
    warmthSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::dynamics);
    brightnessSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::dynamics);
    outputLevelSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::output);
    
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

    // Register right-click on sliders for XY pad assignment
    driveSlider.addMouseListener(this, true);
    driveSlider.getProperties().set("xyParamID", TubeTapeSaturationProcessor::DRIVE_ID);
    warmthSlider.addMouseListener(this, true);
    warmthSlider.getProperties().set("xyParamID", TubeTapeSaturationProcessor::WARMTH_ID);
    brightnessSlider.addMouseListener(this, true);
    brightnessSlider.getProperties().set("xyParamID", TubeTapeSaturationProcessor::BRIGHTNESS_ID);
    outputLevelSlider.addMouseListener(this, true);
    outputLevelSlider.getProperties().set("xyParamID", TubeTapeSaturationProcessor::OUTPUT_LEVEL_ID);

    
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
    xyPadLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    xyPad.setTooltip("Click and drag to control assigned parameters. Right-click parameter labels to assign X/Y axes.");

    // Add saturation meter
    addAndMakeVisible(saturationMeter);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    driveSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    warmthSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    brightnessSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    outputLevelSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    // Tooltips
    driveSlider.setTooltip("Amount of saturation — higher values create more distortion");
    brightnessSlider.setTooltip("Tonal character of the saturation — higher is brighter");
    warmthSlider.setTooltip("Balance between clean and saturated signal");
    outputLevelSlider.setTooltip("Overall output volume after saturation");
    bypassButton.setTooltip("Bypass the effect");

    setSize(700, 550);
    setResizable(true, true);
    setResizeLimits(600, 520, 900, 750);
}

TubeTapeSaturationEditor::~TubeTapeSaturationEditor()
{
    setLookAndFeel(nullptr);
}

void TubeTapeSaturationEditor::paint(juce::Graphics& g)
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

    paintColumnHeader(driveSlider.getX() - 2, driveSlider.getY() - 20, 120,
                      "CHARACTER", HyperPrismLookAndFeel::Colors::dynamics);

    paintColumnHeader(outputSectionX, outputSectionY,
                      getWidth() - outputSectionX - 12,
                      "OUTPUT", HyperPrismLookAndFeel::Colors::output);
}

void TubeTapeSaturationEditor::resized()
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

    // --- Type ComboBox spanning both columns above the knobs ---
    auto typeRow = bounds.removeFromTop(30);
    typeComboBox.setBounds(typeRow.getX(), typeRow.getY(), 200, 25);
    typeLabel.setBounds(typeRow.getX() + 205, typeRow.getY() + 4, 50, 16);
    bounds.removeFromTop(4);

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

    // Column 1: CHARACTER -- Drive, Warmth, Brightness
    int y1 = colTop + knobDiam / 2;
    centerKnob(driveSlider, driveLabel, col1.getX(), colWidth, y1, knobDiam);
    centerKnob(warmthSlider, warmthLabel, col1.getX(), colWidth, y1 + vSpace, knobDiam);
    centerKnob(brightnessSlider, brightnessLabel, col1.getX(), colWidth, y1 + vSpace * 2, knobDiam);

    // Column 2 is empty for this plugin (saturation type area)

    // --- Right side: XY pad + output + meter ---
    auto rightSide = bounds;
    rightSide.removeFromLeft(12);

    int outputHeight = 130;
    int xyHeight = juce::jmax(200, rightSide.getHeight() - outputHeight - 22);
    auto xyArea = rightSide.removeFromTop(xyHeight);
    xyPad.setBounds(xyArea);
    xyPadLabel.setBounds(xyArea.getX(), xyArea.getBottom() + 2, xyArea.getWidth(), 16);
    rightSide.removeFromTop(20);

    // Bottom right: Output knob + Saturation Meter
    auto bottomRight = rightSide;
    auto outputArea = bottomRight.removeFromLeft(140);
    auto meterArea = bottomRight.reduced(4);

    int outKnob = 58;
    int outY = outputArea.getY() + 24;
    centerKnob(outputLevelSlider, outputLevelLabel, outputArea.getX() + 20, 100, outY + outKnob / 2, outKnob);

    outputSectionX = outputArea.getX();
    outputSectionY = outputArea.getY();

    // Meter
    saturationMeter.setBounds(meterArea.reduced(4));
}

void TubeTapeSaturationEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void TubeTapeSaturationEditor::setupComboBox(juce::ComboBox& comboBox, ParameterLabel& label, 
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

void TubeTapeSaturationEditor::updateParameterColors()
{
    auto neutralColor = HyperPrismLookAndFeel::Colors::onSurfaceVariant;
    driveLabel.setColour(juce::Label::textColourId, neutralColor);
    warmthLabel.setColour(juce::Label::textColourId, neutralColor);
    brightnessLabel.setColour(juce::Label::textColourId, neutralColor);
    outputLevelLabel.setColour(juce::Label::textColourId, neutralColor);
    typeLabel.setColour(juce::Label::textColourId, neutralColor);

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

    updateSliderXY(driveSlider, TubeTapeSaturationProcessor::DRIVE_ID);
    updateSliderXY(warmthSlider, TubeTapeSaturationProcessor::WARMTH_ID);
    updateSliderXY(brightnessSlider, TubeTapeSaturationProcessor::BRIGHTNESS_ID);
    updateSliderXY(outputLevelSlider, TubeTapeSaturationProcessor::OUTPUT_LEVEL_ID);
    repaint();
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


void TubeTapeSaturationEditor::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        auto* source = event.eventComponent;
        auto paramID = source->getProperties()["xyParamID"].toString();
        if (paramID.isNotEmpty())
            showParameterMenu(source, paramID);
    }
}
void TubeTapeSaturationEditor::showParameterMenu(juce::Component* target, const juce::String& parameterID)
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
        if (paramID == TubeTapeSaturationProcessor::OUTPUT_LEVEL_ID) return "Output";
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