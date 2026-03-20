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
    auto bandsArea = displayArea;
    
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
                    g.setColour(HyperPrismLookAndFeel::Colors::onSurface.withAlpha(0.3f));
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
    
    // Title
    titleLabel.setText("VOCODER", juce::dontSendNotification);
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
    setupSlider(carrierFreqSlider, carrierFreqLabel, "Carrier Freq");
    setupSlider(modulatorGainSlider, modulatorGainLabel, "Mod Gain");
    setupSlider(bandCountSlider, bandCountLabel, "Band Count");
    setupSlider(releaseTimeSlider, releaseTimeLabel, "Release");
    setupSlider(outputLevelSlider, outputLevelLabel, "Output");

    // Color-code knobs by category
    carrierFreqSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::frequency);
    modulatorGainSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::timing);
    bandCountSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::frequency);
    releaseTimeSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::timing);
    outputLevelSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::output);
    
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

    // Register right-click on sliders for XY pad assignment
    carrierFreqSlider.addMouseListener(this, true);
    carrierFreqSlider.getProperties().set("xyParamID", VocoderProcessor::CARRIER_FREQ_ID);
    modulatorGainSlider.addMouseListener(this, true);
    modulatorGainSlider.getProperties().set("xyParamID", VocoderProcessor::MODULATOR_GAIN_ID);
    bandCountSlider.addMouseListener(this, true);
    bandCountSlider.getProperties().set("xyParamID", VocoderProcessor::BAND_COUNT_ID);
    releaseTimeSlider.addMouseListener(this, true);
    releaseTimeSlider.getProperties().set("xyParamID", VocoderProcessor::RELEASE_TIME_ID);
    outputLevelSlider.addMouseListener(this, true);
    outputLevelSlider.getProperties().set("xyParamID", VocoderProcessor::OUTPUT_LEVEL_ID);

    
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
    xyPadLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    xyPad.setTooltip("Click and drag to control assigned parameters. Right-click parameter labels to assign X/Y axes.");

    // Add vocoder meter
    addAndMakeVisible(vocoderMeter);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    carrierFreqSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    modulatorGainSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    bandCountSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    releaseTimeSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    outputLevelSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    // Tooltips
    carrierFreqSlider.setTooltip("Base frequency of the internal carrier oscillator");
    modulatorGainSlider.setTooltip("Input gain for the modulator (your voice/audio source)");
    bandCountSlider.setTooltip("Number of frequency bands — more bands means higher resolution");
    releaseTimeSlider.setTooltip("How quickly each band responds to modulator changes");
    outputLevelSlider.setTooltip("Overall output volume");
    bypassButton.setTooltip("Bypass the effect");

    setSize(700, 550);
    setResizable(true, true);
    setResizeLimits(600, 520, 900, 750);
}

VocoderEditor::~VocoderEditor()
{
    setLookAndFeel(nullptr);
}

void VocoderEditor::paint(juce::Graphics& g)
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

    paintColumnHeader(carrierFreqSlider.getX() - 2, carrierFreqSlider.getY() - 20, 120,
                      "CARRIER", HyperPrismLookAndFeel::Colors::frequency);
    paintColumnHeader(modulatorGainSlider.getX() - 2, modulatorGainSlider.getY() - 20, 120,
                      "ENVELOPE", HyperPrismLookAndFeel::Colors::dynamics);

    // Output section header
    paintColumnHeader(outputSectionX, outputSectionY,
                      getWidth() - outputSectionX - 12,
                      "OUTPUT", HyperPrismLookAndFeel::Colors::output);
}

void VocoderEditor::resized()
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

    // Column 1: CARRIER -- Carrier Freq, Band Count
    int y1 = colTop + knobDiam / 2;
    centerKnob(carrierFreqSlider, carrierFreqLabel, col1.getX(), colWidth, y1, knobDiam);
    centerKnob(bandCountSlider, bandCountLabel, col1.getX(), colWidth, y1 + vSpace, knobDiam);

    // Column 2: ENVELOPE -- Modulator Gain, Release Time
    centerKnob(modulatorGainSlider, modulatorGainLabel, col2.getX(), colWidth, y1, knobDiam);
    centerKnob(releaseTimeSlider, releaseTimeLabel, col2.getX(), colWidth, y1 + vSpace, knobDiam);

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

    centerKnob(outputLevelSlider, outputLevelLabel, outputArea.getX() + 20, 100, outY + outKnob / 2, outKnob);

    outputSectionX = outputArea.getX();
    outputSectionY = outputArea.getY();

    // Meter
    vocoderMeter.setBounds(meterArea.reduced(4));
}

void VocoderEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void VocoderEditor::updateParameterColors()
{
    auto neutralColor = HyperPrismLookAndFeel::Colors::onSurfaceVariant;
    carrierFreqLabel.setColour(juce::Label::textColourId, neutralColor);
    modulatorGainLabel.setColour(juce::Label::textColourId, neutralColor);
    bandCountLabel.setColour(juce::Label::textColourId, neutralColor);
    releaseTimeLabel.setColour(juce::Label::textColourId, neutralColor);
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

    updateSliderXY(carrierFreqSlider, VocoderProcessor::CARRIER_FREQ_ID);
    updateSliderXY(modulatorGainSlider, VocoderProcessor::MODULATOR_GAIN_ID);
    updateSliderXY(bandCountSlider, VocoderProcessor::BAND_COUNT_ID);
    updateSliderXY(releaseTimeSlider, VocoderProcessor::RELEASE_TIME_ID);
    updateSliderXY(outputLevelSlider, VocoderProcessor::OUTPUT_LEVEL_ID);
    repaint();
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


void VocoderEditor::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        auto* source = event.eventComponent;
        auto paramID = source->getProperties()["xyParamID"].toString();
        if (paramID.isNotEmpty())
            showParameterMenu(source, paramID);
    }
}
void VocoderEditor::showParameterMenu(juce::Component* target, const juce::String& parameterID)
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