//==============================================================================
// HyperPrism Reimagined - Ring Modulator Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "RingModulatorEditor.h"

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
// RingModulatorMeter Implementation
//==============================================================================
RingModulatorMeter::RingModulatorMeter(RingModulatorProcessor& p) 
    : processor(p)
{
    startTimerHz(30); // 30 FPS update rate
    
    // Initialize waveform arrays
    carrierWaveform.resize(256);
    modulatorWaveform.resize(256);
    outputWaveform.resize(256);
}

RingModulatorMeter::~RingModulatorMeter()
{
    stopTimer();
}

void RingModulatorMeter::paint(juce::Graphics& g)
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
    float waveformHeight = displayArea.getHeight() / 3.0f - 5.0f;
    
    // Carrier waveform (top)
    auto carrierArea = displayArea.removeFromTop(waveformHeight);
    auto carrierWaveArea = carrierArea.withTrimmedLeft(60); // Leave space for label
    g.setColour(HyperPrismLookAndFeel::Colors::success);
    juce::Path carrierPath;
    for (size_t i = 0; i < carrierWaveform.size(); ++i)
    {
        float x = carrierWaveArea.getX() + (i / float(carrierWaveform.size() - 1)) * carrierWaveArea.getWidth();
        float y = carrierWaveArea.getCentreY() - carrierWaveform[i] * carrierWaveArea.getHeight() * 0.4f;
        
        if (i == 0)
            carrierPath.startNewSubPath(x, y);
        else
            carrierPath.lineTo(x, y);
    }
    g.strokePath(carrierPath, juce::PathStrokeType(2.0f));
    
    // Label (draw in reserved space)
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(10.0f);
    g.drawText("Carrier", carrierArea.withWidth(55), juce::Justification::centredLeft);
    
    displayArea.removeFromTop(5.0f);
    
    // Modulator waveform (middle)
    auto modulatorArea = displayArea.removeFromTop(waveformHeight);
    auto modulatorWaveArea = modulatorArea.withTrimmedLeft(60); // Leave space for label
    g.setColour(HyperPrismLookAndFeel::Colors::warning);
    juce::Path modulatorPath;
    for (size_t i = 0; i < modulatorWaveform.size(); ++i)
    {
        float x = modulatorWaveArea.getX() + (i / float(modulatorWaveform.size() - 1)) * modulatorWaveArea.getWidth();
        float y = modulatorWaveArea.getCentreY() - modulatorWaveform[i] * modulatorWaveArea.getHeight() * 0.4f;
        
        if (i == 0)
            modulatorPath.startNewSubPath(x, y);
        else
            modulatorPath.lineTo(x, y);
    }
    g.strokePath(modulatorPath, juce::PathStrokeType(2.0f));
    
    // Label (draw in reserved space)
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(10.0f);
    g.drawText("Modulator", modulatorArea.withWidth(55), juce::Justification::centredLeft);
    
    displayArea.removeFromTop(5.0f);
    
    // Output waveform (bottom)
    auto outputArea = displayArea.removeFromTop(waveformHeight);
    auto outputWaveArea = outputArea.withTrimmedLeft(60); // Leave space for label
    g.setColour(HyperPrismLookAndFeel::Colors::primary);
    juce::Path outputPath;
    for (size_t i = 0; i < outputWaveform.size(); ++i)
    {
        float x = outputWaveArea.getX() + (i / float(outputWaveform.size() - 1)) * outputWaveArea.getWidth();
        float y = outputWaveArea.getCentreY() - outputWaveform[i] * outputWaveArea.getHeight() * 0.4f;
        
        if (i == 0)
            outputPath.startNewSubPath(x, y);
        else
            outputPath.lineTo(x, y);
    }
    g.strokePath(outputPath, juce::PathStrokeType(2.0f));
    
    // Label (draw in reserved space)
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(10.0f);
    g.drawText("Output", outputArea.withWidth(55), juce::Justification::centredLeft);
}

void RingModulatorMeter::timerCallback()
{
    // Generate sample waveforms for visualization
    // In a real implementation, these would come from the processor
    
    float carrierFreq = 0.02f; // Normalized frequency for display
    float modulatorFreq = 0.005f;
    
    for (size_t i = 0; i < carrierWaveform.size(); ++i)
    {
        float phase = (i / float(carrierWaveform.size())) * juce::MathConstants<float>::twoPi;
        
        // Carrier (higher frequency)
        carrierWaveform[i] = std::sin(phase * 10.0f);
        
        // Modulator (lower frequency)
        modulatorWaveform[i] = std::sin(phase * 2.0f);
        
        // Ring modulated output (product)
        outputWaveform[i] = carrierWaveform[i] * modulatorWaveform[i];
    }
    
    repaint();
}

//==============================================================================
// RingModulatorEditor Implementation
//==============================================================================
RingModulatorEditor::RingModulatorEditor(RingModulatorProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), ringModulatorMeter(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments (using placeholder IDs)
    xParameterIDs.add("carrier_freq");
    yParameterIDs.add("modulator_freq");
    
    // Title
    titleLabel.setText("RING MODULATOR", juce::dontSendNotification);
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
    setupSlider(modulatorFreqSlider, modulatorFreqLabel, "Modulator Freq");
    setupSlider(mixSlider, mixLabel, "Mix");

    // Color-code knobs by category
    carrierFreqSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::frequency);
    modulatorFreqSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::frequency);
    mixSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::output);
    
    // Set parameter ranges
    carrierFreqSlider.setRange(1.0, 8000.0, 0.1);
    modulatorFreqSlider.setRange(0.1, 1000.0, 0.1);
    mixSlider.setRange(0.0, 100.0, 0.1);
    
    // Setup ComboBoxes
    setupComboBox(carrierWaveformBox, carrierWaveformLabel, "Carrier Wave");
    setupComboBox(modulatorWaveformBox, modulatorWaveformLabel, "Modulator Wave");
    
    // Add waveform options
    carrierWaveformBox.addItem("Sine", 1);
    carrierWaveformBox.addItem("Triangle", 2);
    carrierWaveformBox.addItem("Square", 3);
    carrierWaveformBox.addItem("Sawtooth", 4);
    
    modulatorWaveformBox.addItem("Sine", 1);
    modulatorWaveformBox.addItem("Triangle", 2);
    modulatorWaveformBox.addItem("Square", 3);
    modulatorWaveformBox.addItem("Sawtooth", 4);
    
    // Set up right-click handlers for parameter assignment
    carrierFreqLabel.onClick = [this]() { showParameterMenu(&carrierFreqLabel, "carrier_freq"); };
    modulatorFreqLabel.onClick = [this]() { showParameterMenu(&modulatorFreqLabel, "modulator_freq"); };
    carrierWaveformLabel.onClick = [this]() { showParameterMenu(&carrierWaveformLabel, "carrier_waveform"); };
    modulatorWaveformLabel.onClick = [this]() { showParameterMenu(&modulatorWaveformLabel, "modulator_waveform"); };
    mixLabel.onClick = [this]() { showParameterMenu(&mixLabel, "mix"); };

    // Register right-click on sliders for XY pad assignment
    carrierFreqSlider.addMouseListener(this, true);
    carrierFreqSlider.getProperties().set("xyParamID", "carrier_freq");
    modulatorFreqSlider.addMouseListener(this, true);
    modulatorFreqSlider.getProperties().set("xyParamID", "modulator_freq");
    mixSlider.addMouseListener(this, true);
    mixSlider.getProperties().set("xyParamID", "mix");

    
    // Bypass button (top right like AutoPan)
    // Bypass button
    bypassButton.setButtonText("Bypass");
    bypassButton.setClickingTogglesState(true);
    bypassButton.setColour(juce::TextButton::buttonOnColourId,
                            HyperPrismLookAndFeel::Colors::error.withAlpha(0.6f));
    bypassButton.setColour(juce::TextButton::textColourOnId,
                            HyperPrismLookAndFeel::Colors::onSurface);
    addAndMakeVisible(bypassButton);
    
    // Create attachments (using placeholder parameter IDs)
    auto& apvts = audioProcessor.getAPVTS();
    carrierFreqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "carrier_freq", carrierFreqSlider);
    modulatorFreqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "modulator_freq", modulatorFreqSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, "mix", mixSlider);
    carrierWaveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, "carrier_waveform", carrierWaveformBox);
    modulatorWaveformAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        apvts, "modulator_waveform", modulatorWaveformBox);
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);
    xyPadLabel.setText("Carrier Freq / Modulator Freq", juce::dontSendNotification);
    xyPadLabel.setJustificationType(juce::Justification::centred);
    xyPadLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    xyPad.setTooltip("Click and drag to control assigned parameters. Right-click parameter labels to assign X/Y axes.");

    // Add ring modulator meter
    addAndMakeVisible(ringModulatorMeter);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    carrierFreqSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    modulatorFreqSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    mixSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    // Tooltips
    carrierFreqSlider.setTooltip("Frequency of the carrier oscillator");
    modulatorFreqSlider.setTooltip("Frequency of the secondary modulator");
    carrierWaveformBox.setTooltip("Waveform shape of the carrier");
    modulatorWaveformBox.setTooltip("Waveform shape of the modulator");
    mixSlider.setTooltip("Balance between dry and ring-modulated signal");
    bypassButton.setTooltip("Bypass the effect");

    setSize(700, 550);
    setResizable(true, true);
    setResizeLimits(600, 520, 900, 750);
}

RingModulatorEditor::~RingModulatorEditor()
{
    setLookAndFeel(nullptr);
}

void RingModulatorEditor::paint(juce::Graphics& g)
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

    paintColumnHeader(carrierFreqSlider.getX() - 2, carrierFreqSlider.getY() - 20, 200,
                      "CARRIER", HyperPrismLookAndFeel::Colors::frequency);
    paintColumnHeader(outputSectionX, outputSectionY, 140,
                      "OUTPUT", HyperPrismLookAndFeel::Colors::output);
}

void RingModulatorEditor::resized()
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

    // Column: CARRIER -- carrierFreq, modulatorFreq
    int y1 = colTop + knobDiam / 2;
    centerKnob(carrierFreqSlider, carrierFreqLabel, col1.getX(), colWidth, y1, knobDiam);
    centerKnob(modulatorFreqSlider, modulatorFreqLabel, col1.getX(), colWidth, y1 + vSpace, knobDiam);

    // ComboBoxes below knobs in column
    int comboY = y1 + vSpace + knobDiam / 2 + 36;
    carrierWaveformLabel.setBounds(col1.getX(), comboY, colWidth, 14);
    carrierWaveformBox.setBounds(col1.getX() + 5, comboY + 15, colWidth - 10, 24);
    modulatorWaveformLabel.setBounds(col1.getX(), comboY + 45, colWidth, 14);
    modulatorWaveformBox.setBounds(col1.getX() + 5, comboY + 60, colWidth - 10, 24);

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

    // Ring modulator meter in remaining output area
    auto meterArea = bottomRight.reduced(4);
    ringModulatorMeter.setBounds(meterArea.reduced(4));
}

void RingModulatorEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void RingModulatorEditor::setupComboBox(juce::ComboBox& comboBox, ParameterLabel& label, 
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

void RingModulatorEditor::updateParameterColors()
{
    auto neutralColor = HyperPrismLookAndFeel::Colors::onSurfaceVariant;
    carrierFreqLabel.setColour(juce::Label::textColourId, neutralColor);
    modulatorFreqLabel.setColour(juce::Label::textColourId, neutralColor);
    carrierWaveformLabel.setColour(juce::Label::textColourId, neutralColor);
    modulatorWaveformLabel.setColour(juce::Label::textColourId, neutralColor);
    mixLabel.setColour(juce::Label::textColourId, neutralColor);

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

    updateSliderXY(carrierFreqSlider, "carrier_freq");
    updateSliderXY(modulatorFreqSlider, "modulator_freq");
    updateSliderXY(mixSlider, "mix");
    repaint();
}

void RingModulatorEditor::updateXYPadFromParameters()
{
    // For multiple parameters, use the average of their normalized values
    float xValue = 0.0f;
    float yValue = 0.0f;
    
    auto& apvts = audioProcessor.getAPVTS();
    
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

void RingModulatorEditor::updateParametersFromXYPad(float x, float y)
{
    auto& apvts = audioProcessor.getAPVTS();
    
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


void RingModulatorEditor::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        auto* source = event.eventComponent;
        auto paramID = source->getProperties()["xyParamID"].toString();
        if (paramID.isNotEmpty())
            showParameterMenu(source, paramID);
    }
}
void RingModulatorEditor::showParameterMenu(juce::Component* target, const juce::String& parameterID)
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
                    xParameterIDs.add("carrier_freq");
                    
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
                    yParameterIDs.add("modulator_freq");
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add("carrier_freq");
                yParameterIDs.add("modulator_freq");
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void RingModulatorEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == "carrier_freq") return "Carrier Freq";
        if (paramID == "modulator_freq") return "Modulator Freq";
        if (paramID == "carrier_waveform") return "Carrier Wave";
        if (paramID == "modulator_waveform") return "Modulator Wave";
        if (paramID == "mix") return "Mix";
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