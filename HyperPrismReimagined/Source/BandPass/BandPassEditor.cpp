//==============================================================================
// HyperPrism Reimagined - Band-Pass Filter Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "BandPassEditor.h"

//==============================================================================
// XYPad Implementation (matching AutoPan style)
//==============================================================================
XYPad::XYPad()
{
    setRepaintsOnMouseActivity(true);
    setWantsKeyboardFocus(true);
    setHasFocusOutline(true);
    setTitle("X/Y Control Pad");
    setDescription("Use the arrow keys to adjust the assigned X and Y parameters.");
}

bool XYPad::keyPressed(const juce::KeyPress& key)
{
    const float step = key.getModifiers().isShiftDown() ? 0.01f : 0.05f;
    float newX = xValue;
    float newY = yValue;

    if (key.isKeyCode(juce::KeyPress::leftKey))       newX = juce::jlimit(0.0f, 1.0f, xValue - step);
    else if (key.isKeyCode(juce::KeyPress::rightKey)) newX = juce::jlimit(0.0f, 1.0f, xValue + step);
    else if (key.isKeyCode(juce::KeyPress::upKey))    newY = juce::jlimit(0.0f, 1.0f, yValue + step);
    else if (key.isKeyCode(juce::KeyPress::downKey))  newY = juce::jlimit(0.0f, 1.0f, yValue - step);
    else                                              return false;

    xValue = newX;
    yValue = newY;
    if (onValueChange)
        onValueChange(xValue, yValue);
    repaint();
    return true;
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
// BandPassEditor Implementation
//==============================================================================
BandPassEditor::BandPassEditor(BandPassProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add(BandPassProcessor::CENTER_FREQ_ID);
    yParameterIDs.add(BandPassProcessor::BANDWIDTH_ID);
    
    // Title
    titleLabel.setText("BAND-PASS FILTER", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(16.0f).withStyle("Bold")));
    titleLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurface);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    brandLabel.setText("HyperPrism Reimagined", juce::dontSendNotification);
    brandLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    brandLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    brandLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(brandLabel);

    // Setup sliders with consistent style (4 parameters in single row)
    setupSlider(centerFreqSlider, centerFreqLabel, "Center Freq");
    setupSlider(bandwidthSlider, bandwidthLabel, "Bandwidth");
    setupSlider(gainSlider, gainLabel, "Gain");
    setupSlider(mixSlider, mixLabel, "Mix");

    // NOTE: the pre-migration per-slider rotarySliderFillColourId "arc colour by category"
    // set calls were removed here: the house zqsfx::ui::LookAndFeel::drawRotarySlider
    // (filmstrip knobs) consults no per-control colour ID at all, so they had become dead
    // code. The group colour they used to paint survives through the still-live, still-
    // coloured column headers (paintColumnHeader() in paint() below), which every knob in
    // that column already matched by design.

    // Set up right-click handlers for parameter assignment
    centerFreqLabel.onClick = [this]() { showParameterMenu(&centerFreqLabel, BandPassProcessor::CENTER_FREQ_ID); };
    bandwidthLabel.onClick = [this]() { showParameterMenu(&bandwidthLabel, BandPassProcessor::BANDWIDTH_ID); };
    gainLabel.onClick = [this]() { showParameterMenu(&gainLabel, BandPassProcessor::GAIN_ID); };
    mixLabel.onClick = [this]() { showParameterMenu(&mixLabel, BandPassProcessor::MIX_ID); };

    // Register right-click on sliders for XY pad assignment
    centerFreqSlider.addMouseListener(this, true);
    centerFreqSlider.getProperties().set("xyParamID", BandPassProcessor::CENTER_FREQ_ID);
    bandwidthSlider.addMouseListener(this, true);
    bandwidthSlider.getProperties().set("xyParamID", BandPassProcessor::BANDWIDTH_ID);
    gainSlider.addMouseListener(this, true);
    gainSlider.getProperties().set("xyParamID", BandPassProcessor::GAIN_ID);
    mixSlider.addMouseListener(this, true);
    mixSlider.getProperties().set("xyParamID", BandPassProcessor::MIX_ID);

    
    // Bypass button (top right like AutoPan)
    bypassButton.setButtonText("Bypass");
    bypassButton.setTitle("Bypass");
    bypassButton.setClickingTogglesState(true);
    bypassButton.setColour(juce::TextButton::buttonOnColourId,
                            HyperPrismLookAndFeel::Colors::error.withAlpha(0.6f));
    bypassButton.setColour(juce::TextButton::textColourOnId,
                            HyperPrismLookAndFeel::Colors::onSurface);
    addAndMakeVisible(bypassButton);

    // ZQ SFX company mark (far right of the header) -- also the About-box trigger.
    addAndMakeVisible(logo);
    logo.onClick = [] { HyperPrismAbout::show(JucePlugin_Name); };
    
    // Create attachments
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        audioProcessor.getValueTreeState(), BandPassProcessor::BYPASS_ID, bypassButton);
    centerFreqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), BandPassProcessor::CENTER_FREQ_ID, centerFreqSlider);
    bandwidthAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), BandPassProcessor::BANDWIDTH_ID, bandwidthSlider);
    gainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), BandPassProcessor::GAIN_ID, gainSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getValueTreeState(), BandPassProcessor::MIX_ID, mixSlider);
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);
    xyPadLabel.setText("Center Freq / Bandwidth", juce::dontSendNotification);
    xyPadLabel.setJustificationType(juce::Justification::centred);
    xyPadLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    centerFreqSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    bandwidthSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    gainSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    mixSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    // Tooltips
    centerFreqSlider.setTooltip("Center frequency of the pass band");
    centerFreqSlider.setDescription("Center frequency of the pass band");
    bandwidthSlider.setTooltip("Width of the frequency band that passes through");
    bandwidthSlider.setDescription("Width of the frequency band that passes through");
    gainSlider.setTooltip("Boost or cut the filtered signal");
    gainSlider.setDescription("Boost or cut the filtered signal");
    mixSlider.setTooltip("Balance between dry and filtered signal");
    mixSlider.setDescription("Balance between dry and filtered signal");
    bypassButton.setTooltip("Bypass the effect");
    bypassButton.setDescription("Bypass the effect");
    xyPad.setTooltip("Click and drag to control assigned parameters. Right-click parameter labels to assign X/Y axes.");
    xyPad.setDescription("Click and drag to control assigned parameters. Right-click parameter labels to assign X/Y axes.");
    setSize(700, 550);
    setResizable(true, true);
    setResizeLimits(600, 520, 900, 750);
}

BandPassEditor::~BandPassEditor()
{
    setLookAndFeel(nullptr);
}

void BandPassEditor::paintOverChildren(juce::Graphics& g)
{
    // Redraws the "which knobs feed the XY pad" badge (see HyperPrismLookAndFeel.h)
    // that the pre-migration drawRotarySlider used to draw inline; the house filmstrip
    // drawRotarySlider consults no per-slider property, so this now lives here instead.
    HyperPrismLookAndFeel::paintXYAssignmentBadges(g, *this);
}

void BandPassEditor::paint(juce::Graphics& g)
{
    auto chassisBounds = getLocalBounds().toFloat();

    g.setGradientFill(zqsfx::ui::gradients::chassis(chassisBounds));

    g.fillRect(chassisBounds);
    g.setColour(HyperPrismLookAndFeel::Colors::primary.withAlpha(0.4f));
    g.fillRect(12, 4, getWidth() - 24, 2);
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
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

    paintColumnHeader(centerFreqSlider.getX() - 2, centerFreqSlider.getY() - 20, 200,
                      "FILTER", HyperPrismLookAndFeel::Colors::frequency);
    paintColumnHeader(outputSectionX, outputSectionY, 140,
                      "OUTPUT", HyperPrismLookAndFeel::Colors::output);
}

void BandPassEditor::resized()
{
    auto bounds = getLocalBounds();

    // === HEADER (72px) ===
    auto header = bounds.removeFromTop(72);
    titleLabel.setBounds(header.getX() + 12, 30, header.getWidth() - 146, 20);
    brandLabel.setBounds(header.getX() + 12, 50, header.getWidth() - 146, 16);
    // Logo sits at the far right (style guide section 5); bypass moves left to clear it.
    logo.setBounds(header.getRight() - 28, 8, 24, 24);
    bypassButton.setBounds(header.getRight() - 90 - 34, 36, 80, 26);

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

    // Column: FILTER -- Center Freq, Bandwidth, Gain
    int y1 = colTop + knobDiam / 2;
    centerKnob(centerFreqSlider, centerFreqLabel, col1.getX(), colWidth, y1, knobDiam);
    centerKnob(bandwidthSlider, bandwidthLabel, col1.getX(), colWidth, y1 + vSpace, knobDiam);
    centerKnob(gainSlider, gainLabel, col1.getX(), colWidth, y1 + vSpace * 2, knobDiam);

    // --- Right side: XY pad + output ---
    auto rightSide = bounds;
    rightSide.removeFromLeft(12);

    int outputHeight = 130;
    int xyHeight = juce::jmax(200, rightSide.getHeight() - outputHeight - 22);
    auto xyArea = rightSide.removeFromTop(xyHeight);
    xyPad.setBounds(xyArea);
    xyPadLabel.setBounds(xyArea.getX(), xyArea.getBottom() + 2, xyArea.getWidth(), 16);
    rightSide.removeFromTop(20);

    // Output section: Mix knob centered
    auto bottomRight = rightSide;
    auto outputArea = bottomRight;
    outputSectionX = outputArea.getX();
    outputSectionY = outputArea.getY();
    int outKnob = 58;
    int outY = outputArea.getY() + 24;
    centerKnob(mixSlider, mixLabel, outputArea.getCentreX() - 50, 100, outY + outKnob / 2, outKnob);
}

void BandPassEditor::setupControls()
{
    // This method is no longer used - moved into constructor
}

void BandPassEditor::setupXYPad()
{
    // This method is no longer used - moved into constructor
}

void BandPassEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
                               const juce::String& text)
{
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);

    addAndMakeVisible(slider);
    slider.setTitle(text);
    slider.setWantsKeyboardFocus(true);
    slider.setHasFocusOutline(true);
    slider.setMouseClickGrabsKeyboardFocus(false);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    addAndMakeVisible(label);
}

void BandPassEditor::updateParameterColors()
{
    auto neutralColor = HyperPrismLookAndFeel::Colors::onSurfaceVariant;
    centerFreqLabel.setColour(juce::Label::textColourId, neutralColor);
    bandwidthLabel.setColour(juce::Label::textColourId, neutralColor);
    gainLabel.setColour(juce::Label::textColourId, neutralColor);
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

    updateSliderXY(centerFreqSlider, BandPassProcessor::CENTER_FREQ_ID);
    updateSliderXY(bandwidthSlider, BandPassProcessor::BANDWIDTH_ID);
    updateSliderXY(gainSlider, BandPassProcessor::GAIN_ID);
    updateSliderXY(mixSlider, BandPassProcessor::MIX_ID);
    repaint();
}

void BandPassEditor::updateXYPadFromParameters()
{
    // For multiple parameters, use the average of their normalized values
    float xValue = 0.0f;
    float yValue = 0.0f;
    
    // Calculate average X value
    if (!xParameterIDs.isEmpty())
    {
        for (const auto& paramID : xParameterIDs)
        {
            if (auto* param = audioProcessor.getValueTreeState().getRawParameterValue(paramID))
            {
                if (auto* paramObj = audioProcessor.getValueTreeState().getParameter(paramID))
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
            if (auto* param = audioProcessor.getValueTreeState().getRawParameterValue(paramID))
            {
                if (auto* paramObj = audioProcessor.getValueTreeState().getParameter(paramID))
                {
                    yValue += paramObj->convertTo0to1(*param);
                }
            }
        }
        yValue /= yParameterIDs.size();
    }
    
    xyPad.setValues(xValue, yValue);
}

void BandPassEditor::updateParametersFromXYPad(float x, float y)
{
    // Update all assigned X parameters
    for (const auto& paramID : xParameterIDs)
    {
        if (auto* param = audioProcessor.getValueTreeState().getParameter(paramID))
            param->setValueNotifyingHost(x);
    }
    
    // Update all assigned Y parameters
    for (const auto& paramID : yParameterIDs)
    {
        if (auto* param = audioProcessor.getValueTreeState().getParameter(paramID))
            param->setValueNotifyingHost(y);
    }
}


void BandPassEditor::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        auto* source = event.eventComponent;
        auto paramID = source->getProperties()["xyParamID"].toString();
        if (paramID.isNotEmpty())
            showParameterMenu(source, paramID);
    }
}
void BandPassEditor::showParameterMenu(juce::Component* target, const juce::String& parameterID)
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
                    xParameterIDs.add(BandPassProcessor::CENTER_FREQ_ID);
                    
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
                    yParameterIDs.add(BandPassProcessor::BANDWIDTH_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add(BandPassProcessor::CENTER_FREQ_ID);
                yParameterIDs.add(BandPassProcessor::BANDWIDTH_ID);
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void BandPassEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == BandPassProcessor::CENTER_FREQ_ID) return "Center Freq";
        if (paramID == BandPassProcessor::BANDWIDTH_ID) return "Bandwidth";
        if (paramID == BandPassProcessor::GAIN_ID) return "Gain";
        if (paramID == BandPassProcessor::MIX_ID) return "Mix";
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