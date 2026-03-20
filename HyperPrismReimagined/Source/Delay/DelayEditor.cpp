//==============================================================================
// HyperPrism Reimagined - Delay Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "DelayEditor.h"

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
// DelayEditor Implementation
//==============================================================================
DelayEditor::DelayEditor(DelayProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add(DelayProcessor::DELAY_TIME_ID);
    yParameterIDs.add(DelayProcessor::FEEDBACK_ID);
    
    // Title
    titleLabel.setText("DELAY", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(16.0f).withStyle("Bold")));
    titleLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurface);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    brandLabel.setText("HyperPrism Reimagined", juce::dontSendNotification);
    brandLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    brandLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    brandLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(brandLabel);

    // Setup sliders with consistent style (7 parameters - 4+3 layout)
    setupSlider(mixSlider, mixLabel, "Mix");
    setupSlider(delayTimeSlider, delayTimeLabel, "Delay Time");
    setupSlider(feedbackSlider, feedbackLabel, "Feedback");
    setupSlider(lowCutSlider, lowCutLabel, "Low Cut");
    setupSlider(highCutSlider, highCutLabel, "High Cut");
    setupSlider(stereoOffsetSlider, stereoOffsetLabel, "Stereo Offset");

    // Color-code knobs by parameter category
    delayTimeSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::timing);
    feedbackSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::timing);
    lowCutSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::frequency);
    highCutSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::frequency);
    stereoOffsetSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::timing);
    mixSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::output);
    
    // Set up right-click handlers for parameter assignment
    mixLabel.onClick = [this]() { showParameterMenu(&mixLabel, DelayProcessor::MIX_ID); };
    delayTimeLabel.onClick = [this]() { showParameterMenu(&delayTimeLabel, DelayProcessor::DELAY_TIME_ID); };
    feedbackLabel.onClick = [this]() { showParameterMenu(&feedbackLabel, DelayProcessor::FEEDBACK_ID); };
    lowCutLabel.onClick = [this]() { showParameterMenu(&lowCutLabel, DelayProcessor::LOW_CUT_ID); };
    highCutLabel.onClick = [this]() { showParameterMenu(&highCutLabel, DelayProcessor::HIGH_CUT_ID); };
    stereoOffsetLabel.onClick = [this]() { showParameterMenu(&stereoOffsetLabel, DelayProcessor::STEREO_OFFSET_ID); };

    // Register right-click on sliders for XY pad assignment
    mixSlider.addMouseListener(this, true);
    mixSlider.getProperties().set("xyParamID", DelayProcessor::MIX_ID);
    delayTimeSlider.addMouseListener(this, true);
    delayTimeSlider.getProperties().set("xyParamID", DelayProcessor::DELAY_TIME_ID);
    feedbackSlider.addMouseListener(this, true);
    feedbackSlider.getProperties().set("xyParamID", DelayProcessor::FEEDBACK_ID);
    lowCutSlider.addMouseListener(this, true);
    lowCutSlider.getProperties().set("xyParamID", DelayProcessor::LOW_CUT_ID);
    highCutSlider.addMouseListener(this, true);
    highCutSlider.getProperties().set("xyParamID", DelayProcessor::HIGH_CUT_ID);
    stereoOffsetSlider.addMouseListener(this, true);
    stereoOffsetSlider.getProperties().set("xyParamID", DelayProcessor::STEREO_OFFSET_ID);

    
    // Tempo Sync toggle
    tempoSyncButton.setButtonText("Tempo Sync");
    tempoSyncButton.setColour(juce::ToggleButton::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    tempoSyncButton.setColour(juce::ToggleButton::tickColourId, HyperPrismLookAndFeel::Colors::primary);
    addAndMakeVisible(tempoSyncButton);
    
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
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, DelayProcessor::BYPASS_ID, bypassButton);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, DelayProcessor::MIX_ID, mixSlider);
    delayTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, DelayProcessor::DELAY_TIME_ID, delayTimeSlider);
    feedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, DelayProcessor::FEEDBACK_ID, feedbackSlider);
    lowCutAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, DelayProcessor::LOW_CUT_ID, lowCutSlider);
    highCutAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, DelayProcessor::HIGH_CUT_ID, highCutSlider);
    stereoOffsetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, DelayProcessor::STEREO_OFFSET_ID, stereoOffsetSlider);
    tempoSyncAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, DelayProcessor::TEMPO_SYNC_ID, tempoSyncButton);
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);
    xyPadLabel.setText("Delay Time / Feedback", juce::dontSendNotification);
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
    mixSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    delayTimeSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    feedbackSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    lowCutSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    highCutSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    stereoOffsetSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    // Tooltips
    delayTimeSlider.setTooltip("Time between the original signal and the delayed repeat");
    feedbackSlider.setTooltip("Amount of delayed signal fed back to create multiple repeats");
    lowCutSlider.setTooltip("Remove low frequencies from the delayed signal");
    highCutSlider.setTooltip("Remove high frequencies from the delayed signal");
    stereoOffsetSlider.setTooltip("Time difference between left and right delay for stereo width");
    mixSlider.setTooltip("Balance between dry and delayed signal");
    bypassButton.setTooltip("Bypass the effect");
    xyPad.setTooltip("Click and drag to control two parameters at once");

    setSize(700, 550);
    setResizable(true, true);
    setResizeLimits(600, 520, 900, 750);
}

DelayEditor::~DelayEditor()
{
    setLookAndFeel(nullptr);
}

void DelayEditor::paint(juce::Graphics& g)
{
    g.fillAll(HyperPrismLookAndFeel::Colors::background);

    // Accent line
    g.setColour(HyperPrismLookAndFeel::Colors::primary.withAlpha(0.4f));
    g.fillRect(12, 4, getWidth() - 24, 2);

    // Version
    g.setColour(HyperPrismLookAndFeel::Colors::outline);
    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.drawText("v1.0.0", getLocalBounds().removeFromBottom(20).removeFromRight(70), juce::Justification::centredRight);

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

    paintColumnHeader(delayTimeSlider.getX() - 2, delayTimeSlider.getY() - 20, 120,
                      "TIMING", HyperPrismLookAndFeel::Colors::timing);
    paintColumnHeader(lowCutSlider.getX() - 2, lowCutSlider.getY() - 20, 120,
                      "TONE", HyperPrismLookAndFeel::Colors::frequency);

    paintColumnHeader(outputSectionX, outputSectionY,
                      getWidth() - outputSectionX - 12,
                      "OUTPUT", HyperPrismLookAndFeel::Colors::output);
}

void DelayEditor::resized()
{
    auto bounds = getLocalBounds();

    // === HEADER (72px) ===
    auto header = bounds.removeFromTop(72);
    titleLabel.setBounds(header.getX() + 12, 30, header.getWidth() - 112, 20);
    brandLabel.setBounds(header.getX() + 12, 50, header.getWidth() - 112, 16);
    bypassButton.setBounds(header.getRight() - 90, 36, 80, 26);

    // Tempo Sync toggle (top left)
    tempoSyncButton.setBounds(12, 8, 100, 24);

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

    // Column 1: TIMING -- Delay Time, Feedback, Stereo Offset
    int y1 = colTop + knobDiam / 2;
    centerKnob(delayTimeSlider, delayTimeLabel, col1.getX(), colWidth, y1, knobDiam);
    centerKnob(feedbackSlider, feedbackLabel, col1.getX(), colWidth, y1 + vSpace, knobDiam);
    centerKnob(stereoOffsetSlider, stereoOffsetLabel, col1.getX(), colWidth, y1 + vSpace * 2, knobDiam);

    // Column 2: TONE -- Low Cut, High Cut
    centerKnob(lowCutSlider, lowCutLabel, col2.getX(), colWidth, y1, knobDiam);
    centerKnob(highCutSlider, highCutLabel, col2.getX(), colWidth, y1 + vSpace, knobDiam);

    // --- Right side: XY pad + output ---
    auto rightSide = bounds;
    rightSide.removeFromLeft(12);

    int outputHeight = 130;
    int xyHeight = juce::jmax(200, rightSide.getHeight() - outputHeight - 22);
    auto xyArea = rightSide.removeFromTop(xyHeight);
    xyPad.setBounds(xyArea);
    xyPadLabel.setBounds(xyArea.getX(), xyArea.getBottom() + 2, xyArea.getWidth(), 16);
    rightSide.removeFromTop(20);

    // Bottom right: Output knob centered
    auto bottomRight = rightSide;
    auto outputArea = bottomRight;

    outputSectionX = outputArea.getX();
    outputSectionY = outputArea.getY();

    int outKnob = 58;
    int outY = outputArea.getY() + 24;
    centerKnob(mixSlider, mixLabel, outputArea.getCentreX() - 50, 100, outY + outKnob / 2, outKnob);
}

void DelayEditor::setupControls()
{
    // This method is no longer used - moved into constructor
}

void DelayEditor::setupXYPad()
{
    // This method is no longer used - moved into constructor
}

void DelayEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void DelayEditor::updateParameterColors()
{
    auto neutralColor = HyperPrismLookAndFeel::Colors::onSurfaceVariant;
    mixLabel.setColour(juce::Label::textColourId, neutralColor);
    delayTimeLabel.setColour(juce::Label::textColourId, neutralColor);
    feedbackLabel.setColour(juce::Label::textColourId, neutralColor);
    lowCutLabel.setColour(juce::Label::textColourId, neutralColor);
    highCutLabel.setColour(juce::Label::textColourId, neutralColor);
    stereoOffsetLabel.setColour(juce::Label::textColourId, neutralColor);

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

    updateSliderXY(mixSlider, DelayProcessor::MIX_ID);
    updateSliderXY(delayTimeSlider, DelayProcessor::DELAY_TIME_ID);
    updateSliderXY(feedbackSlider, DelayProcessor::FEEDBACK_ID);
    updateSliderXY(lowCutSlider, DelayProcessor::LOW_CUT_ID);
    updateSliderXY(highCutSlider, DelayProcessor::HIGH_CUT_ID);
    updateSliderXY(stereoOffsetSlider, DelayProcessor::STEREO_OFFSET_ID);
    repaint();
}

void DelayEditor::updateXYPadFromParameters()
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

void DelayEditor::updateParametersFromXYPad(float x, float y)
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


void DelayEditor::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        auto* source = event.eventComponent;
        auto paramID = source->getProperties()["xyParamID"].toString();
        if (paramID.isNotEmpty())
            showParameterMenu(source, paramID);
    }
}
void DelayEditor::showParameterMenu(juce::Component* target, const juce::String& parameterID)
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
                    xParameterIDs.add(DelayProcessor::DELAY_TIME_ID);
                    
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
                    yParameterIDs.add(DelayProcessor::FEEDBACK_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add(DelayProcessor::DELAY_TIME_ID);
                yParameterIDs.add(DelayProcessor::FEEDBACK_ID);
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void DelayEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == DelayProcessor::MIX_ID) return "Mix";
        if (paramID == DelayProcessor::DELAY_TIME_ID) return "Delay Time";
        if (paramID == DelayProcessor::FEEDBACK_ID) return "Feedback";
        if (paramID == DelayProcessor::LOW_CUT_ID) return "Low Cut";
        if (paramID == DelayProcessor::HIGH_CUT_ID) return "High Cut";
        if (paramID == DelayProcessor::STEREO_OFFSET_ID) return "Stereo Offset";
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