//==============================================================================
// HyperPrism Reimagined - Compressor Editor
// Updated to match modern template
//==============================================================================

#include "CompressorEditor.h"

//==============================================================================
// XYPad Implementation (matching modern style)
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
// GainReductionMeter Implementation (horizontal meter like modern template)
//==============================================================================
GainReductionMeter::GainReductionMeter(CompressorProcessor& p) : processor(p)
{
    startTimerHz(30);
}

GainReductionMeter::~GainReductionMeter()
{
    stopTimer();
}

void GainReductionMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(HyperPrismLookAndFeel::Colors::background);
    g.fillRoundedRectangle(bounds, 3.0f);
    
    // Meter background
    g.setColour(HyperPrismLookAndFeel::Colors::surfaceVariant);
    g.fillRoundedRectangle(bounds.reduced(2), 2.0f);
    
    // Draw gain reduction horizontally
    if (currentReduction > 0.01f)
    {
        float meterWidth = bounds.getWidth() - 4;
        float reductionWidth = currentReduction * meterWidth;
        
        // Color gradient from green to yellow to red
        juce::Colour meterColor;
        if (currentReduction < 0.33f)
            meterColor = HyperPrismLookAndFeel::Colors::success;
        else if (currentReduction < 0.66f)
            meterColor = HyperPrismLookAndFeel::Colors::warning;
        else
            meterColor = HyperPrismLookAndFeel::Colors::error;
        
        g.setColour(meterColor);
        g.fillRoundedRectangle(bounds.getX() + 2, 
                               bounds.getY() + 2,
                               reductionWidth, 
                               bounds.getHeight() - 4, 
                               2.0f);
    }
    
    // Draw scale marks horizontally
    g.setColour(HyperPrismLookAndFeel::Colors::onSurface.withAlpha(0.5f));
    for (int db = 0; db <= 20; db += 5)
    {
        float x = bounds.getX() + 2 + (db / 20.0f) * (bounds.getWidth() - 4);
        g.drawLine(x, bounds.getY(), x, bounds.getY() + 5, 1.0f);
        
        if (db % 10 == 0)
        {
            g.setFont(10.0f);
            g.drawText(juce::String(db), x - 5, bounds.getY() + 7, 10, 20, 
                      juce::Justification::centred);
        }
    }
    
    // Border
    g.setColour(HyperPrismLookAndFeel::Colors::outline);
    g.drawRoundedRectangle(bounds, 3.0f, 1.0f);
}

void GainReductionMeter::timerCallback()
{
    targetReduction = processor.getGainReduction();
    
    // Smooth the meter movement
    const float smoothing = 0.3f;
    currentReduction = currentReduction + (targetReduction - currentReduction) * smoothing;
    
    repaint();
}

//==============================================================================
// CompressorEditor Implementation
//==============================================================================
CompressorEditor::CompressorEditor(CompressorProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), gainReductionMeter(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add("threshold");
    yParameterIDs.add("ratio");
    
    // Title
    titleLabel.setText("COMPRESSOR", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(16.0f).withStyle("Bold")));
    titleLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurface);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    brandLabel.setText("HyperPrism Reimagined", juce::dontSendNotification);
    brandLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    brandLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    brandLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(brandLabel);

    // Bypass button
    bypassButton.setButtonText("Bypass");
    bypassButton.setClickingTogglesState(true);
    bypassButton.setColour(juce::TextButton::buttonOnColourId,
                            HyperPrismLookAndFeel::Colors::error.withAlpha(0.6f));
    bypassButton.setColour(juce::TextButton::textColourOnId,
                            HyperPrismLookAndFeel::Colors::onSurface);
    addAndMakeVisible(bypassButton);

    // Setup sliders with consistent style
    setupSlider(thresholdSlider, thresholdLabel, "Threshold");
    setupSlider(ratioSlider, ratioLabel, "Ratio");
    setupSlider(attackSlider, attackLabel, "Attack");
    setupSlider(releaseSlider, releaseLabel, "Release");
    setupSlider(kneeSlider, kneeLabel, "Knee");
    setupSlider(makeupGainSlider, makeupGainLabel, "Makeup");
    setupSlider(mixSlider, mixLabel, "Mix");

    // Color-code knobs by category
    thresholdSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::dynamics);
    ratioSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::dynamics);
    kneeSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::dynamics);
    attackSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::timing);
    releaseSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::timing);
    makeupGainSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::output);
    mixSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::output);
    
    // Set up right-click handlers for parameter assignment
    thresholdLabel.onClick = [this]() { showParameterMenu(&thresholdLabel, "threshold"); };
    ratioLabel.onClick = [this]() { showParameterMenu(&ratioLabel, "ratio"); };
    attackLabel.onClick = [this]() { showParameterMenu(&attackLabel, "attack"); };
    releaseLabel.onClick = [this]() { showParameterMenu(&releaseLabel, "release"); };
    kneeLabel.onClick = [this]() { showParameterMenu(&kneeLabel, "knee"); };
    makeupGainLabel.onClick = [this]() { showParameterMenu(&makeupGainLabel, "makeupGain"); };
    mixLabel.onClick = [this]() { showParameterMenu(&mixLabel, "mix"); };

    // Register right-click on sliders for XY pad assignment
    thresholdSlider.addMouseListener(this, true);
    thresholdSlider.getProperties().set("xyParamID", "threshold");
    ratioSlider.addMouseListener(this, true);
    ratioSlider.getProperties().set("xyParamID", "ratio");
    attackSlider.addMouseListener(this, true);
    attackSlider.getProperties().set("xyParamID", "attack");
    releaseSlider.addMouseListener(this, true);
    releaseSlider.getProperties().set("xyParamID", "release");
    kneeSlider.addMouseListener(this, true);
    kneeSlider.getProperties().set("xyParamID", "knee");
    makeupGainSlider.addMouseListener(this, true);
    makeupGainSlider.getProperties().set("xyParamID", "makeupGain");
    mixSlider.addMouseListener(this, true);
    mixSlider.getProperties().set("xyParamID", "mix");

    
    // Create attachments
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.apvts, "bypass", bypassButton);
    thresholdAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "threshold", thresholdSlider);
    ratioAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "ratio", ratioSlider);
    attackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "attack", attackSlider);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "release", releaseSlider);
    kneeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "knee", kneeSlider);
    makeupGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "makeupGain", makeupGainSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "mix", mixSlider);
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);  // Set the axis colors
    xyPadLabel.setText("Threshold / Ratio", juce::dontSendNotification);
    xyPadLabel.setJustificationType(juce::Justification::centred);
    xyPadLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    
    // Setup gain reduction meter
    addAndMakeVisible(gainReductionMeter);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();  // Show initial color coding
    
    // Listen for parameter changes - update XY pad when any parameter changes
    thresholdSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    ratioSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    attackSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    releaseSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    kneeSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    makeupGainSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    mixSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    // Tooltips
    thresholdSlider.setTooltip("Signal level above which compression begins");
    ratioSlider.setTooltip("Amount of gain reduction -- higher ratio means more compression");
    attackSlider.setTooltip("How quickly compression responds to loud signals");
    releaseSlider.setTooltip("How quickly compression releases after signal drops");
    kneeSlider.setTooltip("Softness of the compression onset -- higher values are more gradual");
    makeupGainSlider.setTooltip("Boost to compensate for volume lost during compression");
    mixSlider.setTooltip("Balance between dry and compressed signal (parallel compression)");
    bypassButton.setTooltip("Bypass the effect");
    xyPad.setTooltip("Click and drag to control two parameters at once");

    setSize(700, 550);
    setResizable(true, true);
    setResizeLimits(600, 520, 900, 750);
}

CompressorEditor::~CompressorEditor()
{
    setLookAndFeel(nullptr);
}

void CompressorEditor::paint(juce::Graphics& g)
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

    // Headers anchored to first slider in each column
    paintColumnHeader(thresholdSlider.getX() - 2, thresholdSlider.getY() - 20, 120,
                      "DYNAMICS", HyperPrismLookAndFeel::Colors::dynamics);
    paintColumnHeader(attackSlider.getX() - 2, attackSlider.getY() - 20, 120,
                      "TIMING", HyperPrismLookAndFeel::Colors::timing);

    // Output section header
    paintColumnHeader(outputSectionX, outputSectionY,
                      getWidth() - outputSectionX - 12,
                      "OUTPUT", HyperPrismLookAndFeel::Colors::output);
}

void CompressorEditor::resized()
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
    int colTop = col1.getY() + 20; // room for section header

    auto centerKnob = [&](juce::Slider& slider, juce::Label& label,
                           int colX, int colW, int cy, int kd)
    {
        int kx = colX + (colW - kd) / 2;
        int ky = cy - kd / 2;
        slider.setBounds(kx, ky, kd, kd);
        label.setBounds(colX, ky + kd + 1, colW, 16);
    };

    // Column 1: DYNAMICS -- Threshold, Ratio, Knee
    int y1 = colTop + knobDiam / 2;
    centerKnob(thresholdSlider, thresholdLabel, col1.getX(), colWidth, y1, knobDiam);
    centerKnob(ratioSlider, ratioLabel, col1.getX(), colWidth, y1 + vSpace, knobDiam);
    centerKnob(kneeSlider, kneeLabel, col1.getX(), colWidth, y1 + vSpace * 2, knobDiam);

    // Column 2: TIMING -- Attack, Release
    centerKnob(attackSlider, attackLabel, col2.getX(), colWidth, y1, knobDiam);
    centerKnob(releaseSlider, releaseLabel, col2.getX(), colWidth, y1 + vSpace, knobDiam);

    // --- Right side: XY pad + output + meter ---
    auto rightSide = bounds;
    rightSide.removeFromLeft(12);

    // XY pad takes top portion
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
    centerKnob(makeupGainSlider, makeupGainLabel, outputArea.getX(), 90, outY + outKnob / 2, outKnob);
    centerKnob(mixSlider, mixLabel, outputArea.getX() + 90, 90, outY + outKnob / 2, outKnob);

    gainReductionMeter.setBounds(meterArea.reduced(4));
}

void CompressorEditor::setupSlider(juce::Slider& slider, juce::Label& label, 
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

void CompressorEditor::updateParameterColors()
{
    auto neutralColor = HyperPrismLookAndFeel::Colors::onSurfaceVariant;
    thresholdLabel.setColour(juce::Label::textColourId, neutralColor);
    ratioLabel.setColour(juce::Label::textColourId, neutralColor);
    attackLabel.setColour(juce::Label::textColourId, neutralColor);
    releaseLabel.setColour(juce::Label::textColourId, neutralColor);
    kneeLabel.setColour(juce::Label::textColourId, neutralColor);
    makeupGainLabel.setColour(juce::Label::textColourId, neutralColor);
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

    updateSliderXY(thresholdSlider, "threshold");
    updateSliderXY(ratioSlider, "ratio");
    updateSliderXY(attackSlider, "attack");
    updateSliderXY(releaseSlider, "release");
    updateSliderXY(kneeSlider, "knee");
    updateSliderXY(makeupGainSlider, "makeupGain");
    updateSliderXY(mixSlider, "mix");
    repaint();
}

void CompressorEditor::updateXYPadFromParameters()
{
    // For multiple parameters, use the average of their normalized values
    float xValue = 0.0f;
    float yValue = 0.0f;
    
    // Calculate average X value
    if (!xParameterIDs.isEmpty())
    {
        for (const auto& paramID : xParameterIDs)
        {
            if (auto* param = audioProcessor.apvts.getRawParameterValue(paramID))
            {
                if (auto* paramObj = audioProcessor.apvts.getParameter(paramID))
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
            if (auto* param = audioProcessor.apvts.getRawParameterValue(paramID))
            {
                if (auto* paramObj = audioProcessor.apvts.getParameter(paramID))
                {
                    yValue += paramObj->convertTo0to1(*param);
                }
            }
        }
        yValue /= yParameterIDs.size();
    }
    
    xyPad.setValues(xValue, yValue);
}

void CompressorEditor::updateParametersFromXYPad(float x, float y)
{
    // Update all assigned X parameters
    for (const auto& paramID : xParameterIDs)
    {
        if (auto* param = audioProcessor.apvts.getParameter(paramID))
            param->setValueNotifyingHost(x);
    }
    
    // Update all assigned Y parameters
    for (const auto& paramID : yParameterIDs)
    {
        if (auto* param = audioProcessor.apvts.getParameter(paramID))
            param->setValueNotifyingHost(y);
    }
}


void CompressorEditor::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        auto* source = event.eventComponent;
        auto paramID = source->getProperties()["xyParamID"].toString();
        if (paramID.isNotEmpty())
            showParameterMenu(source, paramID);
    }
}
void CompressorEditor::showParameterMenu(juce::Component* target, const juce::String& parameterID)
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
                    xParameterIDs.add("threshold");
                    
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
                    yParameterIDs.add("ratio");
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add("threshold");
                yParameterIDs.add("ratio");
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void CompressorEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == "threshold") return "Threshold";
        if (paramID == "ratio") return "Ratio";
        if (paramID == "attack") return "Attack";
        if (paramID == "release") return "Release";
        if (paramID == "knee") return "Knee";
        if (paramID == "makeupGain") return "Makeup";
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