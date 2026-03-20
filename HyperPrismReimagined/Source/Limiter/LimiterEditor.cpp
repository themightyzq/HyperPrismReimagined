//==============================================================================
// HyperPrism Reimagined - Limiter Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "LimiterEditor.h"

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
// GainReductionMeter Implementation
//==============================================================================
GainReductionMeter::GainReductionMeter(LimiterProcessor& p) 
    : processor(p)
{
    startTimerHz(30); // 30 FPS update rate
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
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Border
    g.setColour(HyperPrismLookAndFeel::Colors::outline);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    
    // Create meter area
    auto meterArea = bounds.reduced(4.0f);
    
    // Draw gain reduction
    if (currentGainReduction > 0.001f)
    {
        float meterHeight = meterArea.getHeight() * currentGainReduction;
        auto meterRect = juce::Rectangle<float>(meterArea.getX(), 
                                               meterArea.getBottom() - meterHeight, 
                                               meterArea.getWidth(), 
                                               meterHeight);
        
        // Gradient based on amount
        if (currentGainReduction < 0.3f)
            g.setColour(HyperPrismLookAndFeel::Colors::success);
        else if (currentGainReduction < 0.7f)
            g.setColour(HyperPrismLookAndFeel::Colors::warning);
        else
            g.setColour(HyperPrismLookAndFeel::Colors::error);
            
        g.fillRoundedRectangle(meterRect, 2.0f);
    }
    
    // Draw scale lines
    g.setColour(HyperPrismLookAndFeel::Colors::surface);
    for (int i = 1; i < 4; ++i)
    {
        float y = meterArea.getY() + (meterArea.getHeight() * i / 4.0f);
        g.drawHorizontalLine(static_cast<int>(y), meterArea.getX(), meterArea.getRight());
    }
    
    // dB labels
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(9.0f);
    
    // Draw labels on the left side
    for (int db = 0; db <= 20; db += 5)
    {
        float y = meterArea.getBottom() - (db / 20.0f * meterArea.getHeight());
        g.drawText(juce::String(-db), bounds.getX() - 25, y - 6, 20, 12, 
                   juce::Justification::centredRight);
    }
}

void GainReductionMeter::timerCallback()
{
    float newGainReduction = processor.getCurrentGainReduction();
    
    // Smooth the meter display
    const float smoothing = 0.7f;
    currentGainReduction = currentGainReduction * smoothing + newGainReduction * (1.0f - smoothing);
    
    repaint();
}

//==============================================================================
// LimiterEditor Implementation
//==============================================================================
LimiterEditor::LimiterEditor(LimiterProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), gainReductionMeter(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add(CEILING_ID);
    yParameterIDs.add(RELEASE_ID);
    
    // Title
    titleLabel.setText("LIMITER", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(16.0f).withStyle("Bold")));
    titleLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurface);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    // Brand label
    brandLabel.setText("HyperPrism Reimagined", juce::dontSendNotification);
    brandLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    brandLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    brandLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(brandLabel);
    
    // Setup sliders with consistent style (4 sliders + 1 toggle layout)
    setupSlider(ceilingSlider, ceilingLabel, "Ceiling");
    ceilingSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::dynamics);
    setupSlider(releaseSlider, releaseLabel, "Release");
    releaseSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::dynamics);
    setupSlider(lookaheadSlider, lookaheadLabel, "Lookahead");
    lookaheadSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::dynamics);
    setupSlider(inputGainSlider, inputGainLabel, "Input Gain");
    inputGainSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::dynamics);
    
    // Set up right-click handlers for parameter assignment
    ceilingLabel.onClick = [this]() { showParameterMenu(&ceilingLabel, CEILING_ID); };
    releaseLabel.onClick = [this]() { showParameterMenu(&releaseLabel, RELEASE_ID); };
    lookaheadLabel.onClick = [this]() { showParameterMenu(&lookaheadLabel, LOOKAHEAD_ID); };
    inputGainLabel.onClick = [this]() { showParameterMenu(&inputGainLabel, INPUT_GAIN_ID); };

    // Register right-click on sliders for XY pad assignment
    ceilingSlider.addMouseListener(this, true);
    ceilingSlider.getProperties().set("xyParamID", CEILING_ID);
    releaseSlider.addMouseListener(this, true);
    releaseSlider.getProperties().set("xyParamID", RELEASE_ID);
    lookaheadSlider.addMouseListener(this, true);
    lookaheadSlider.getProperties().set("xyParamID", LOOKAHEAD_ID);
    inputGainSlider.addMouseListener(this, true);
    inputGainSlider.getProperties().set("xyParamID", INPUT_GAIN_ID);

    
    // Soft Clip toggle
    softClipButton.setButtonText("Soft Clip");
    softClipButton.setColour(juce::ToggleButton::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    softClipButton.setColour(juce::ToggleButton::tickColourId, HyperPrismLookAndFeel::Colors::primary);
    addAndMakeVisible(softClipButton);
    
    // Bypass button (top right)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setClickingTogglesState(true);
    addAndMakeVisible(bypassButton);
    
    // Create attachments
    auto& apvts = audioProcessor.getStateInformation();
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, BYPASS_ID, bypassButton);
    ceilingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, CEILING_ID, ceilingSlider);
    releaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, RELEASE_ID, releaseSlider);
    lookaheadAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, LOOKAHEAD_ID, lookaheadSlider);
    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, INPUT_GAIN_ID, inputGainSlider);
    softClipAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, SOFTCLIP_ID, softClipButton);
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);
    xyPadLabel.setText("Ceiling / Release", juce::dontSendNotification);
    xyPadLabel.setJustificationType(juce::Justification::centred);
    xyPadLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    
    // Add gain reduction meter
    addAndMakeVisible(gainReductionMeter);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    ceilingSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    releaseSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    lookaheadSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    inputGainSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    // Tooltips
    ceilingSlider.setTooltip("Maximum output level -- signal will not exceed this");
    releaseSlider.setTooltip("How quickly the limiter releases after catching a peak");
    lookaheadSlider.setTooltip("Look ahead time to catch transients before they clip");
    inputGainSlider.setTooltip("Boost input signal to drive the limiter harder");
    bypassButton.setTooltip("Bypass the effect");
    xyPad.setTooltip("Click and drag to control two parameters at once");

    setSize(700, 550);
    setResizable(true, true);
    setResizeLimits(600, 520, 900, 750);
}

LimiterEditor::~LimiterEditor()
{
    setLookAndFeel(nullptr);
}

void LimiterEditor::paint(juce::Graphics& g)
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

    paintColumnHeader(ceilingSlider.getX() - 2, ceilingSlider.getY() - 20, 200,
                      "DYNAMICS", HyperPrismLookAndFeel::Colors::dynamics);
    paintColumnHeader(outputSectionX, outputSectionY, 140,
                      "METER", HyperPrismLookAndFeel::Colors::dynamics);
}

void LimiterEditor::resized()
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

    int knobDiam = 68;
    int vSpace = 95;
    int colTop = col1.getY() + 20;

    auto centerKnob = [&](juce::Slider& slider, juce::Label& label,
                           int colX, int colW, int cy, int kd)
    {
        int kx = colX + (colW - kd) / 2;
        int ky = cy - kd / 2;
        slider.setBounds(kx, ky, kd, kd);
        label.setBounds(colX, ky + kd + 1, colW, 16);
    };

    // Column: DYNAMICS -- Ceiling, Release, Lookahead, Input Gain
    int y1 = colTop + knobDiam / 2;
    centerKnob(ceilingSlider, ceilingLabel, col1.getX(), colWidth, y1, knobDiam);
    centerKnob(releaseSlider, releaseLabel, col1.getX(), colWidth, y1 + vSpace, knobDiam);
    centerKnob(lookaheadSlider, lookaheadLabel, col1.getX(), colWidth, y1 + vSpace * 2, knobDiam);
    centerKnob(inputGainSlider, inputGainLabel, col1.getX(), colWidth, y1 + vSpace * 3, knobDiam);

    // Soft Clip toggle below knobs in column
    int toggleY = y1 + vSpace * 3 + knobDiam / 2 + 36;
    softClipButton.setBounds(col1.getX(), toggleY, colWidth, 25);

    // --- Right side: XY pad + meter ---
    auto rightSide = bounds;
    rightSide.removeFromLeft(12);

    // XY pad takes top portion, meter below
    int outputHeight = 130;
    int xyHeight = juce::jmax(200, rightSide.getHeight() - outputHeight - 22);
    auto xyArea = rightSide.removeFromTop(xyHeight);
    xyPad.setBounds(xyArea);
    xyPadLabel.setBounds(xyArea.getX(), xyArea.getBottom() + 2, xyArea.getWidth(), 16);
    rightSide.removeFromTop(20);

    // Gain reduction meter below XY pad
    auto bottomRight = rightSide;
    outputSectionX = bottomRight.getX();
    outputSectionY = bottomRight.getY();
    bottomRight.removeFromTop(20);  // Space for "METER" section header
    auto meterArea = bottomRight.reduced(4);
    gainReductionMeter.setBounds(meterArea.reduced(4));
}

void LimiterEditor::setupControls()
{
    // This method is no longer used - moved into constructor
}

void LimiterEditor::setupXYPad()
{
    // This method is no longer used - moved into constructor
}

void LimiterEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void LimiterEditor::updateParameterColors()
{
    auto neutralColor = HyperPrismLookAndFeel::Colors::onSurfaceVariant;
    ceilingLabel.setColour(juce::Label::textColourId, neutralColor);
    releaseLabel.setColour(juce::Label::textColourId, neutralColor);
    lookaheadLabel.setColour(juce::Label::textColourId, neutralColor);
    inputGainLabel.setColour(juce::Label::textColourId, neutralColor);

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

    updateSliderXY(ceilingSlider, CEILING_ID);
    updateSliderXY(releaseSlider, RELEASE_ID);
    updateSliderXY(lookaheadSlider, LOOKAHEAD_ID);
    updateSliderXY(inputGainSlider, INPUT_GAIN_ID);
    repaint();
}

void LimiterEditor::updateXYPadFromParameters()
{
    // For multiple parameters, use the average of their normalized values
    float xValue = 0.0f;
    float yValue = 0.0f;
    
    auto& apvts = audioProcessor.getStateInformation();
    
    // Calculate average X value
    if (!xParameterIDs.isEmpty())
    {
        for (const auto& paramID : xParameterIDs)
        {
            if (auto* param = apvts.getRawParameterValue(paramID))
            {
                if (auto* paramObj = apvts.getParameter(paramID))
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
            if (auto* param = apvts.getRawParameterValue(paramID))
            {
                if (auto* paramObj = apvts.getParameter(paramID))
                {
                    yValue += paramObj->convertTo0to1(*param);
                }
            }
        }
        yValue /= yParameterIDs.size();
    }
    
    xyPad.setValues(xValue, yValue);
}

void LimiterEditor::updateParametersFromXYPad(float x, float y)
{
    auto& apvts = audioProcessor.getStateInformation();
    
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


void LimiterEditor::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        auto* source = event.eventComponent;
        auto paramID = source->getProperties()["xyParamID"].toString();
        if (paramID.isNotEmpty())
            showParameterMenu(source, paramID);
    }
}
void LimiterEditor::showParameterMenu(juce::Component* target, const juce::String& parameterID)
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
                    xParameterIDs.add(CEILING_ID);
                    
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
                    yParameterIDs.add(RELEASE_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add(CEILING_ID);
                yParameterIDs.add(RELEASE_ID);
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void LimiterEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == CEILING_ID) return "Ceiling";
        if (paramID == RELEASE_ID) return "Release";
        if (paramID == LOOKAHEAD_ID) return "Lookahead";
        if (paramID == INPUT_GAIN_ID) return "Input Gain";
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