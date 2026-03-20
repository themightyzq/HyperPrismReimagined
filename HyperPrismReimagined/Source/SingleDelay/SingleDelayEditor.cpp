//==============================================================================
// HyperPrism Reimagined - Single Delay Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "SingleDelayEditor.h"

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
// DelayMeter Implementation
//==============================================================================
DelayMeter::DelayMeter(SingleDelayProcessor& p) 
    : processor(p)
{
    startTimerHz(30); // 30 FPS update rate
    
    // Initialize delay buffer for visualization
    delayBuffer.resize(256);
    bufferPosition = 0;
}

DelayMeter::~DelayMeter()
{
    stopTimer();
}

void DelayMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(HyperPrismLookAndFeel::Colors::background);
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Border
    g.setColour(HyperPrismLookAndFeel::Colors::outline);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    
    // Create meter areas - split into input/output levels and delay visualization
    auto meterArea = bounds.reduced(4.0f);
    float meterHeight = meterArea.getHeight() * 0.6f;
    float visualHeight = meterArea.getHeight() * 0.4f;
    
    // I/O level meters (top section)
    auto levelArea = meterArea.removeFromTop(meterHeight);
    float halfWidth = levelArea.getWidth() / 2.0f;
    
    // Input meter (left half)
    auto inputMeterArea = juce::Rectangle<float>(levelArea.getX(), levelArea.getY(), halfWidth - 2, levelArea.getHeight());
    float inputLevelHeight = inputMeterArea.getHeight() * inputLevel;
    auto inputLevelRect = juce::Rectangle<float>(inputMeterArea.getX(), 
                                                inputMeterArea.getBottom() - inputLevelHeight, 
                                                inputMeterArea.getWidth(), 
                                                inputLevelHeight);
    
    g.setColour(HyperPrismLookAndFeel::Colors::success);
    g.fillRoundedRectangle(inputLevelRect, 2.0f);
    
    // Output meter (right half)
    auto outputMeterArea = juce::Rectangle<float>(levelArea.getX() + halfWidth + 2, levelArea.getY(), halfWidth - 2, levelArea.getHeight());
    float outputLevelHeight = outputMeterArea.getHeight() * outputLevel;
    auto outputLevelRect = juce::Rectangle<float>(outputMeterArea.getX(), 
                                                 outputMeterArea.getBottom() - outputLevelHeight, 
                                                 outputMeterArea.getWidth(), 
                                                 outputLevelHeight);
    
    g.setColour(HyperPrismLookAndFeel::Colors::primary);
    g.fillRoundedRectangle(outputLevelRect, 2.0f);
    
    // Divider line
    g.setColour(HyperPrismLookAndFeel::Colors::surface);
    g.drawVerticalLine(static_cast<int>(levelArea.getX() + halfWidth), levelArea.getY(), levelArea.getBottom());
    
    // Labels
    g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    g.setFont(9.0f);
    auto inputLabelArea = inputMeterArea.removeFromBottom(12);
    auto outputLabelArea = outputMeterArea.removeFromBottom(12);
    g.drawText("IN", inputLabelArea, juce::Justification::centred);
    g.drawText("OUT", outputLabelArea, juce::Justification::centred);
    
    // Delay visualization (bottom section)
    meterArea.removeFromTop(5); // spacing
    auto delayVisualArea = meterArea.removeFromTop(visualHeight);
    
    // Draw delay buffer as waveform
    g.setColour(HyperPrismLookAndFeel::Colors::warning);
    juce::Path delayPath;
    
    for (size_t i = 0; i < delayBuffer.size(); ++i)
    {
        float x = delayVisualArea.getX() + (i / float(delayBuffer.size() - 1)) * delayVisualArea.getWidth();
        float y = delayVisualArea.getCentreY() - delayBuffer[i] * delayVisualArea.getHeight() * 0.4f;
        
        if (i == 0)
            delayPath.startNewSubPath(x, y);
        else
            delayPath.lineTo(x, y);
    }
    
    g.strokePath(delayPath, juce::PathStrokeType(1.5f));
}

void DelayMeter::timerCallback()
{
    float newInputLevel = processor.getInputLevel();
    float newOutputLevel = processor.getOutputLevel();
    
    // Smooth the level changes
    const float smoothing = 0.7f;
    inputLevel = inputLevel * smoothing + newInputLevel * (1.0f - smoothing);
    outputLevel = outputLevel * smoothing + newOutputLevel * (1.0f - smoothing);
    
    // Update delay buffer visualization (simplified sine wave for demonstration)
    for (size_t i = 0; i < delayBuffer.size(); ++i)
    {
        float phase = (bufferPosition + i) * 0.1f;
        delayBuffer[i] = std::sin(phase) * inputLevel;
    }
    bufferPosition++;
    
    repaint();
}

//==============================================================================
// SingleDelayEditor Implementation
//==============================================================================
SingleDelayEditor::SingleDelayEditor(SingleDelayProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), delayMeter(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add(SingleDelayProcessor::DELAY_TIME_ID);
    yParameterIDs.add(SingleDelayProcessor::FEEDBACK_ID);
    
    // Title
    titleLabel.setText("SINGLE DELAY", juce::dontSendNotification);
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
    setupSlider(delayTimeSlider, delayTimeLabel, "Delay Time");
    setupSlider(feedbackSlider, feedbackLabel, "Feedback");
    setupSlider(wetDryMixSlider, wetDryMixLabel, "Mix");
    setupSlider(highCutSlider, highCutLabel, "High Cut");
    setupSlider(lowCutSlider, lowCutLabel, "Low Cut");
    setupSlider(stereoSpreadSlider, stereoSpreadLabel, "Stereo Spread");

    // Color-code knobs by parameter category
    delayTimeSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::timing);
    feedbackSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::timing);
    wetDryMixSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::output);
    highCutSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::frequency);
    lowCutSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::frequency);
    stereoSpreadSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::timing);
    
    // Set parameter ranges (example ranges - adjust based on processor)
    delayTimeSlider.setRange(1.0, 4000.0, 0.1);
    feedbackSlider.setRange(0.0, 95.0, 0.1);
    wetDryMixSlider.setRange(0.0, 100.0, 0.1);
    highCutSlider.setRange(1000.0, 20000.0, 1.0);
    lowCutSlider.setRange(20.0, 1000.0, 1.0);
    stereoSpreadSlider.setRange(0.0, 100.0, 0.1);
    
    // Set up right-click handlers for parameter assignment
    delayTimeLabel.onClick = [this]() { showParameterMenu(&delayTimeLabel, SingleDelayProcessor::DELAY_TIME_ID); };
    feedbackLabel.onClick = [this]() { showParameterMenu(&feedbackLabel, SingleDelayProcessor::FEEDBACK_ID); };
    wetDryMixLabel.onClick = [this]() { showParameterMenu(&wetDryMixLabel, SingleDelayProcessor::WETDRY_MIX_ID); };
    highCutLabel.onClick = [this]() { showParameterMenu(&highCutLabel, SingleDelayProcessor::HIGH_CUT_ID); };
    lowCutLabel.onClick = [this]() { showParameterMenu(&lowCutLabel, SingleDelayProcessor::LOW_CUT_ID); };
    stereoSpreadLabel.onClick = [this]() { showParameterMenu(&stereoSpreadLabel, SingleDelayProcessor::STEREO_SPREAD_ID); };

    // Register right-click on sliders for XY pad assignment
    delayTimeSlider.addMouseListener(this, true);
    delayTimeSlider.getProperties().set("xyParamID", SingleDelayProcessor::DELAY_TIME_ID);
    feedbackSlider.addMouseListener(this, true);
    feedbackSlider.getProperties().set("xyParamID", SingleDelayProcessor::FEEDBACK_ID);
    wetDryMixSlider.addMouseListener(this, true);
    wetDryMixSlider.getProperties().set("xyParamID", SingleDelayProcessor::WETDRY_MIX_ID);
    highCutSlider.addMouseListener(this, true);
    highCutSlider.getProperties().set("xyParamID", SingleDelayProcessor::HIGH_CUT_ID);
    lowCutSlider.addMouseListener(this, true);
    lowCutSlider.getProperties().set("xyParamID", SingleDelayProcessor::LOW_CUT_ID);
    stereoSpreadSlider.addMouseListener(this, true);
    stereoSpreadSlider.getProperties().set("xyParamID", SingleDelayProcessor::STEREO_SPREAD_ID);

    
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
    delayTimeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, SingleDelayProcessor::DELAY_TIME_ID, delayTimeSlider);
    feedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, SingleDelayProcessor::FEEDBACK_ID, feedbackSlider);
    wetDryMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, SingleDelayProcessor::WETDRY_MIX_ID, wetDryMixSlider);
    highCutAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, SingleDelayProcessor::HIGH_CUT_ID, highCutSlider);
    lowCutAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, SingleDelayProcessor::LOW_CUT_ID, lowCutSlider);
    stereoSpreadAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        apvts, SingleDelayProcessor::STEREO_SPREAD_ID, stereoSpreadSlider);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        apvts, SingleDelayProcessor::BYPASS_ID, bypassButton);
    
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
    xyPad.setTooltip("Click and drag to control assigned parameters. Right-click parameter labels to assign X/Y axes.");

    // Add delay meter
    addAndMakeVisible(delayMeter);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    delayTimeSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    feedbackSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    wetDryMixSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    highCutSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    lowCutSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    stereoSpreadSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    // Tooltips
    delayTimeSlider.setTooltip("Delay time in milliseconds");
    feedbackSlider.setTooltip("Amount of delayed signal fed back for repeating echoes");
    lowCutSlider.setTooltip("Remove low frequencies from the delayed signal");
    highCutSlider.setTooltip("Remove high frequencies from the delayed signal");
    wetDryMixSlider.setTooltip("Balance between dry and delayed signal");
    stereoSpreadSlider.setTooltip("Overall output volume");
    bypassButton.setTooltip("Bypass the effect");

    setSize(700, 550);
    setResizable(true, true);
    setResizeLimits(600, 520, 900, 750);
}

SingleDelayEditor::~SingleDelayEditor()
{
    setLookAndFeel(nullptr);
}

void SingleDelayEditor::paint(juce::Graphics& g)
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

void SingleDelayEditor::resized()
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

    // Column 1: TIMING -- Delay Time, Feedback, Stereo Spread
    int y1 = colTop + knobDiam / 2;
    centerKnob(delayTimeSlider, delayTimeLabel, col1.getX(), colWidth, y1, knobDiam);
    centerKnob(feedbackSlider, feedbackLabel, col1.getX(), colWidth, y1 + vSpace, knobDiam);
    centerKnob(stereoSpreadSlider, stereoSpreadLabel, col1.getX(), colWidth, y1 + vSpace * 2, knobDiam);

    // Column 2: TONE -- Low Cut, High Cut
    centerKnob(lowCutSlider, lowCutLabel, col2.getX(), colWidth, y1, knobDiam);
    centerKnob(highCutSlider, highCutLabel, col2.getX(), colWidth, y1 + vSpace, knobDiam);

    // --- Right side: XY pad + output + meter ---
    auto rightSide = bounds;
    rightSide.removeFromLeft(12);

    int outputHeight = 130;
    int xyHeight = juce::jmax(200, rightSide.getHeight() - outputHeight - 22);
    auto xyArea = rightSide.removeFromTop(xyHeight);
    xyPad.setBounds(xyArea);
    xyPadLabel.setBounds(xyArea.getX(), xyArea.getBottom() + 2, xyArea.getWidth(), 16);
    rightSide.removeFromTop(20);

    // Bottom right: Output knob + Delay Meter
    auto bottomRight = rightSide;
    auto outputArea = bottomRight.removeFromLeft(140);
    auto meterArea = bottomRight.reduced(4);

    int outKnob = 58;
    int outY = outputArea.getY() + 24;
    centerKnob(wetDryMixSlider, wetDryMixLabel, outputArea.getX() + 20, 100, outY + outKnob / 2, outKnob);

    outputSectionX = outputArea.getX();
    outputSectionY = outputArea.getY();

    // Meter
    delayMeter.setBounds(meterArea.reduced(4));
}

void SingleDelayEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void SingleDelayEditor::updateParameterColors()
{
    auto neutralColor = HyperPrismLookAndFeel::Colors::onSurfaceVariant;
    delayTimeLabel.setColour(juce::Label::textColourId, neutralColor);
    feedbackLabel.setColour(juce::Label::textColourId, neutralColor);
    wetDryMixLabel.setColour(juce::Label::textColourId, neutralColor);
    highCutLabel.setColour(juce::Label::textColourId, neutralColor);
    lowCutLabel.setColour(juce::Label::textColourId, neutralColor);
    stereoSpreadLabel.setColour(juce::Label::textColourId, neutralColor);

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

    updateSliderXY(delayTimeSlider, SingleDelayProcessor::DELAY_TIME_ID);
    updateSliderXY(feedbackSlider, SingleDelayProcessor::FEEDBACK_ID);
    updateSliderXY(wetDryMixSlider, SingleDelayProcessor::WETDRY_MIX_ID);
    updateSliderXY(highCutSlider, SingleDelayProcessor::HIGH_CUT_ID);
    updateSliderXY(lowCutSlider, SingleDelayProcessor::LOW_CUT_ID);
    updateSliderXY(stereoSpreadSlider, SingleDelayProcessor::STEREO_SPREAD_ID);
    repaint();
}

void SingleDelayEditor::updateXYPadFromParameters()
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

void SingleDelayEditor::updateParametersFromXYPad(float x, float y)
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


void SingleDelayEditor::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        auto* source = event.eventComponent;
        auto paramID = source->getProperties()["xyParamID"].toString();
        if (paramID.isNotEmpty())
            showParameterMenu(source, paramID);
    }
}
void SingleDelayEditor::showParameterMenu(juce::Component* target, const juce::String& parameterID)
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
                    xParameterIDs.add(SingleDelayProcessor::DELAY_TIME_ID);
                    
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
                    yParameterIDs.add(SingleDelayProcessor::FEEDBACK_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add(SingleDelayProcessor::DELAY_TIME_ID);
                yParameterIDs.add(SingleDelayProcessor::FEEDBACK_ID);
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void SingleDelayEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == SingleDelayProcessor::DELAY_TIME_ID) return "Delay Time";
        if (paramID == SingleDelayProcessor::FEEDBACK_ID) return "Feedback";
        if (paramID == SingleDelayProcessor::WETDRY_MIX_ID) return "Mix";
        if (paramID == SingleDelayProcessor::HIGH_CUT_ID) return "High Cut";
        if (paramID == SingleDelayProcessor::LOW_CUT_ID) return "Low Cut";
        if (paramID == SingleDelayProcessor::STEREO_SPREAD_ID) return "Stereo Spread";
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