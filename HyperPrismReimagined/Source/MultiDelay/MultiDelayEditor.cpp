//==============================================================================
// HyperPrism Reimagined - Multi Delay Editor Implementation
// Updated to match AutoPan template with complex layout
//==============================================================================

#include "MultiDelayEditor.h"

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
// MultiDelayMeter Implementation
//==============================================================================
MultiDelayMeter::MultiDelayMeter(MultiDelayProcessor& p) 
    : processor(p)
{
    startTimerHz(30); // 30 FPS update rate
}

MultiDelayMeter::~MultiDelayMeter()
{
    stopTimer();
}

void MultiDelayMeter::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    
    // Background
    g.setColour(HyperPrismLookAndFeel::Colors::background);
    g.fillRoundedRectangle(bounds, 4.0f);
    
    // Border
    g.setColour(HyperPrismLookAndFeel::Colors::outline);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);
    
    // Create meter areas
    auto meterArea = bounds.reduced(10.0f);
    float totalWidth = meterArea.getWidth();
    float meterHeight = meterArea.getHeight() - 20; // Space for labels
    float meterWidth = totalWidth / 6.0f; // IN + 4 delays + OUT
    
    // Draw each meter
    for (int i = 0; i < 6; ++i)
    {
        auto currentMeterArea = juce::Rectangle<float>(
            meterArea.getX() + i * meterWidth, 
            meterArea.getY(), 
            meterWidth - 4, 
            meterHeight
        );
        
        float level = 0.0f;
        juce::Colour meterColour;
        juce::String label;
        
        if (i == 0) // Input
        {
            level = inputLevel;
            meterColour = HyperPrismLookAndFeel::Colors::primary;
            label = "IN";
        }
        else if (i == 5) // Output
        {
            level = outputLevel;
            meterColour = HyperPrismLookAndFeel::Colors::success;
            label = "OUT";
        }
        else // Delay lines 1-4
        {
            int delayIndex = i - 1;
            level = delayLevels[delayIndex];
            meterColour = HyperPrismLookAndFeel::Colors::warning;
            label = "D" + juce::String(delayIndex + 1);
        }
        
        // Draw meter background
        g.setColour(HyperPrismLookAndFeel::Colors::surfaceVariant);
        g.fillRoundedRectangle(currentMeterArea, 2.0f);
        
        // Draw meter level
        if (level > 0.001f)
        {
            float levelHeight = currentMeterArea.getHeight() * level;
            auto levelRect = juce::Rectangle<float>(
                currentMeterArea.getX(), 
                currentMeterArea.getBottom() - levelHeight, 
                currentMeterArea.getWidth(), 
                levelHeight
            );
            
            g.setColour(meterColour);
            g.fillRoundedRectangle(levelRect, 2.0f);
        }
        
        // Draw scale lines
        g.setColour(HyperPrismLookAndFeel::Colors::surface);
        for (int j = 1; j < 4; ++j)
        {
            float y = currentMeterArea.getY() + (currentMeterArea.getHeight() * j / 4.0f);
            g.drawHorizontalLine(static_cast<int>(y), currentMeterArea.getX(), currentMeterArea.getRight());
        }
        
        // Draw label
        g.setColour(HyperPrismLookAndFeel::Colors::onSurfaceVariant);
        g.setFont(10.0f);
        auto labelArea = juce::Rectangle<float>(
            currentMeterArea.getX(), 
            meterArea.getBottom() - 15, 
            currentMeterArea.getWidth(), 
            15
        );
        g.drawText(label, labelArea, juce::Justification::centred);
    }
}

void MultiDelayMeter::timerCallback()
{
    float newInputLevel = processor.getInputLevel();
    float newOutputLevel = processor.getOutputLevel();
    auto newDelayLevels = processor.getDelayLevels();
    
    // Smooth the level changes
    const float smoothing = 0.7f;
    inputLevel = inputLevel * smoothing + newInputLevel * (1.0f - smoothing);
    outputLevel = outputLevel * smoothing + newOutputLevel * (1.0f - smoothing);
    
    for (int i = 0; i < 4; ++i)
    {
        delayLevels[i] = delayLevels[i] * smoothing + newDelayLevels[i] * (1.0f - smoothing);
    }
    
    repaint();
}

//==============================================================================
// MultiDelayEditor Implementation
//==============================================================================
MultiDelayEditor::MultiDelayEditor(MultiDelayProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), multiDelayMeter(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments for most commonly used controls
    xParameterIDs.add(MultiDelayProcessor::DELAY1_TIME_ID); // Delay 1 Time
    yParameterIDs.add(MultiDelayProcessor::DELAY1_LEVEL_ID); // Delay 1 Level
    
    // Title
    titleLabel.setText("MULTI DELAY", juce::dontSendNotification);
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
    
    // Setup global controls
    setupSlider(masterMixSlider, masterMixLabel, "Mix");
    masterMixSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::output);
    setupSlider(globalFeedbackSlider, globalFeedbackLabel, "Feedback");
    globalFeedbackSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::timing);
    
    // Set up right-click handlers for global parameters
    masterMixLabel.onClick = [this]() { showParameterMenu(&masterMixLabel, MultiDelayProcessor::MASTER_MIX_ID); };
    globalFeedbackLabel.onClick = [this]() { showParameterMenu(&globalFeedbackLabel, MultiDelayProcessor::GLOBAL_FEEDBACK_ID); };

    // Register right-click on sliders for XY pad assignment
    masterMixSlider.addMouseListener(this, true);
    masterMixSlider.getProperties().set("xyParamID", MultiDelayProcessor::MASTER_MIX_ID);
    globalFeedbackSlider.addMouseListener(this, true);
    globalFeedbackSlider.getProperties().set("xyParamID", MultiDelayProcessor::GLOBAL_FEEDBACK_ID);

    
    // Set up tap selector buttons
    for (int i = 0; i < 4; ++i)
    {
        tapButtons[i].setButtonText("Tap " + juce::String(i + 1));
        tapButtons[i].setClickingTogglesState(false);
        tapButtons[i].setColour(juce::TextButton::buttonColourId,
                                HyperPrismLookAndFeel::Colors::surfaceVariant);
        tapButtons[i].setColour(juce::TextButton::textColourOffId,
                                HyperPrismLookAndFeel::Colors::onSurfaceVariant);
        tapButtons[i].onClick = [this, i]() { selectTap(i); };
        addAndMakeVisible(tapButtons[i]);
    }

    // Setup delay controls
    juce::StringArray delayIds[] = {
        { MultiDelayProcessor::DELAY1_TIME_ID, MultiDelayProcessor::DELAY1_LEVEL_ID, 
          MultiDelayProcessor::DELAY1_PAN_ID, MultiDelayProcessor::DELAY1_FEEDBACK_ID },
        { MultiDelayProcessor::DELAY2_TIME_ID, MultiDelayProcessor::DELAY2_LEVEL_ID, 
          MultiDelayProcessor::DELAY2_PAN_ID, MultiDelayProcessor::DELAY2_FEEDBACK_ID },
        { MultiDelayProcessor::DELAY3_TIME_ID, MultiDelayProcessor::DELAY3_LEVEL_ID, 
          MultiDelayProcessor::DELAY3_PAN_ID, MultiDelayProcessor::DELAY3_FEEDBACK_ID },
        { MultiDelayProcessor::DELAY4_TIME_ID, MultiDelayProcessor::DELAY4_LEVEL_ID, 
          MultiDelayProcessor::DELAY4_PAN_ID, MultiDelayProcessor::DELAY4_FEEDBACK_ID }
    };
    
    for (int i = 0; i < 4; ++i)
    {
        // Group label
        delayGroupLabels[i].setText("Delay " + juce::String(i + 1), juce::dontSendNotification);
        delayGroupLabels[i].setFont(juce::Font(12.0f, juce::Font::bold));
        delayGroupLabels[i].setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::primary);
        delayGroupLabels[i].setJustificationType(juce::Justification::centred);
        addAndMakeVisible(delayGroupLabels[i]);
        
        // Compact sliders - smaller size for grid layout
        setupSlider(delayTimeSliders[i], delayTimeLabels[i], "Time");
        delayTimeSliders[i].setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::timing);
        setupSlider(delayLevelSliders[i], delayLevelLabels[i], "Level");
        delayLevelSliders[i].setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::timing);
        setupSlider(delayPanSliders[i], delayPanLabels[i], "Pan");
        delayPanSliders[i].setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::timing);
        setupSlider(delayFeedbackSliders[i], delayFeedbackLabels[i], "FB");
        delayFeedbackSliders[i].setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::timing);
        
        // Set up right-click handlers
        delayTimeLabels[i].onClick = [this, i, delayIds]() { 
            showParameterMenu(&delayTimeLabels[i], delayIds[i][0]); 
        };
        delayLevelLabels[i].onClick = [this, i, delayIds]() { 
            showParameterMenu(&delayLevelLabels[i], delayIds[i][1]); 
        };
        delayPanLabels[i].onClick = [this, i, delayIds]() { 
            showParameterMenu(&delayPanLabels[i], delayIds[i][2]); 
        };
        delayFeedbackLabels[i].onClick = [this, i, delayIds]() { 
            showParameterMenu(&delayFeedbackLabels[i], delayIds[i][3]); 
        };
    }
    
    // Bypass button (top right)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setClickingTogglesState(true);
    addAndMakeVisible(bypassButton);
    
    // Create attachments
    auto& vts = audioProcessor.getValueTreeState();
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        vts, MultiDelayProcessor::BYPASS_ID, bypassButton);
    masterMixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, MultiDelayProcessor::MASTER_MIX_ID, masterMixSlider);
    globalFeedbackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        vts, MultiDelayProcessor::GLOBAL_FEEDBACK_ID, globalFeedbackSlider);
    
    // Create delay attachments
    for (int i = 0; i < 4; ++i)
    {
        delayTimeAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            vts, delayIds[i][0], delayTimeSliders[i]);
        delayLevelAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            vts, delayIds[i][1], delayLevelSliders[i]);
        delayPanAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            vts, delayIds[i][2], delayPanSliders[i]);
        delayFeedbackAttachments[i] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            vts, delayIds[i][3], delayFeedbackSliders[i]);
    }
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);
    xyPadLabel.setText("Delay 1 Time / Delay 1 Level", juce::dontSendNotification);
    xyPadLabel.setJustificationType(juce::Justification::centred);
    xyPadLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    xyPad.setTooltip("Click and drag to control assigned parameters. Right-click parameter labels to assign X/Y axes.");

    // Add multi-delay meter
    addAndMakeVisible(multiDelayMeter);
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    masterMixSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    globalFeedbackSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    for (int i = 0; i < 4; ++i)
    {
        delayTimeSliders[i].onValueChange = [this] { updateXYPadFromParameters(); };
        delayLevelSliders[i].onValueChange = [this] { updateXYPadFromParameters(); };
        delayPanSliders[i].onValueChange = [this] { updateXYPadFromParameters(); };
        delayFeedbackSliders[i].onValueChange = [this] { updateXYPadFromParameters(); };
    }
    
    // Tooltips
    for (int i = 0; i < 4; ++i)
    {
        delayTimeSliders[i].setTooltip("Delay time for tap " + juce::String(i + 1));
        delayLevelSliders[i].setTooltip("Volume level of tap " + juce::String(i + 1));
        delayPanSliders[i].setTooltip("Stereo position of tap " + juce::String(i + 1));
        delayFeedbackSliders[i].setTooltip("Feedback amount for tap " + juce::String(i + 1));
    }
    globalFeedbackSlider.setTooltip("Amount of signal fed back across all delay taps");
    masterMixSlider.setTooltip("Balance between dry and delayed signal");
    bypassButton.setTooltip("Bypass the effect");

    // Initialize with Tap 1 selected
    selectTap(0);

    // Hide unused group labels
    for (int i = 1; i < 4; ++i)
        delayGroupLabels[i].setVisible(false);

    // Optimized size while maintaining functionality
    setSize(700, 550);
    setResizable(true, true);
    setResizeLimits(600, 520, 900, 750);
}

void MultiDelayEditor::selectTap(int tapIndex)
{
    selectedTap = tapIndex;

    // Update button appearance
    for (int i = 0; i < 4; ++i)
    {
        bool selected = (i == tapIndex);
        tapButtons[i].setColour(juce::TextButton::buttonColourId,
            selected ? HyperPrismLookAndFeel::Colors::primary.withAlpha(0.3f)
                     : HyperPrismLookAndFeel::Colors::surfaceVariant);
        tapButtons[i].setColour(juce::TextButton::textColourOffId,
            selected ? HyperPrismLookAndFeel::Colors::onSurface
                     : HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    }

    // Show only the selected tap's controls
    for (int i = 0; i < 4; ++i)
    {
        bool visible = (i == tapIndex);
        delayTimeSliders[i].setVisible(visible);
        delayTimeLabels[i].setVisible(visible);
        delayLevelSliders[i].setVisible(visible);
        delayLevelLabels[i].setVisible(visible);
        delayPanSliders[i].setVisible(visible);
        delayPanLabels[i].setVisible(visible);
        delayFeedbackSliders[i].setVisible(visible);
        delayFeedbackLabels[i].setVisible(visible);
    }

    resized();
    repaint();
}

MultiDelayEditor::~MultiDelayEditor()
{
    setLookAndFeel(nullptr);
}

void MultiDelayEditor::paint(juce::Graphics& g)
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

    // Section headers
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

    // TAP header
    int t = selectedTap;
    if (delayTimeSliders[t].isVisible())
    {
        paintColumnHeader(delayTimeSliders[t].getX() - 25, delayTimeSliders[t].getY() - 20, 250,
                          "TAP " + juce::String(t + 1),
                          HyperPrismLookAndFeel::Colors::timing);
    }

    // GLOBAL header
    paintColumnHeader(globalFeedbackSlider.getX() - 2, globalFeedbackSlider.getY() - 55, 120,
                      "GLOBAL", HyperPrismLookAndFeel::Colors::timing);

    // OUTPUT header
    paintColumnHeader(outputSectionX, outputSectionY, 140,
                      "OUTPUT", HyperPrismLookAndFeel::Colors::output);
}

void MultiDelayEditor::resized()
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

    // --- Left side: dynamic width for tab bar + 2 columns of knobs + global ---
    int rightSideWidth = 312;
    int columnsTotalWidth = bounds.getWidth() - rightSideWidth;
    auto columnsArea = bounds.removeFromLeft(columnsTotalWidth);

    // Tab bar at top (4 buttons in a row)
    auto tabBar = columnsArea.removeFromTop(28);
    int tabWidth = (tabBar.getWidth() - 6) / 4;
    for (int i = 0; i < 4; ++i)
    {
        tapButtons[i].setBounds(tabBar.getX() + i * (tabWidth + 2), tabBar.getY(),
                                tabWidth, 26);
    }
    columnsArea.removeFromTop(8);

    // Two columns for the selected tap's knobs
    int colWidth = (columnsArea.getWidth() - 10) / 2;
    auto col1 = columnsArea.removeFromLeft(colWidth);
    columnsArea.removeFromLeft(10);
    auto col2 = columnsArea.removeFromLeft(colWidth);

    int knobDiam = 70;
    int vSpace = 97;
    int colTop = col1.getY() + 20;

    auto centerKnob = [&](juce::Slider& slider, juce::Label& label,
                           int colX, int colW, int cy, int kd)
    {
        int kx = colX + (colW - kd) / 2;
        int ky = cy - kd / 2;
        slider.setBounds(kx, ky, kd, kd);
        label.setBounds(colX, ky + kd + 1, colW, 16);
    };

    int y1 = colTop + knobDiam / 2;
    int t = selectedTap;

    // Section header label (reuse delayGroupLabels[0])
    delayGroupLabels[0].setText("TAP " + juce::String(t + 1), juce::dontSendNotification);
    delayGroupLabels[0].setBounds(col1.getX(), col1.getY(), columnsTotalWidth, 16);

    // Col 1: Time and Pan
    centerKnob(delayTimeSliders[t], delayTimeLabels[t], col1.getX(), colWidth, y1, knobDiam);
    centerKnob(delayPanSliders[t], delayPanLabels[t], col1.getX(), colWidth, y1 + vSpace, knobDiam);

    // Col 2: Level and Feedback
    centerKnob(delayLevelSliders[t], delayLevelLabels[t], col2.getX(), colWidth, y1, knobDiam);
    centerKnob(delayFeedbackSliders[t], delayFeedbackLabels[t], col2.getX(), colWidth, y1 + vSpace, knobDiam);

    // Global Feedback below tap knobs
    int globalY = y1 + vSpace * 2 + 20;
    centerKnob(globalFeedbackSlider, globalFeedbackLabel,
               col1.getX() + 55, colWidth, globalY, knobDiam);

    // --- Right side: XY pad + output + meter ---
    auto rightSide = bounds;
    rightSide.removeFromLeft(12);

    int outputHeight = 130;
    int xyHeight = juce::jmax(200, rightSide.getHeight() - outputHeight - 22);
    auto xyArea = rightSide.removeFromTop(xyHeight);
    xyPad.setBounds(xyArea);
    xyPadLabel.setBounds(xyArea.getX(), xyArea.getBottom() + 2, xyArea.getWidth(), 16);
    rightSide.removeFromTop(20);

    auto bottomRight = rightSide;
    outputSectionX = bottomRight.getX();
    outputSectionY = bottomRight.getY();
    auto outputArea = bottomRight.removeFromLeft(140);
    auto meterArea = bottomRight;

    int outKnob = 58;
    int outY = outputArea.getY() + 24;
    centerKnob(masterMixSlider, masterMixLabel,
               outputArea.getX() + 20, 100, outY + outKnob / 2, outKnob);

    multiDelayMeter.setBounds(meterArea.reduced(4));
}

void MultiDelayEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
                               const juce::String& text)
{
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    slider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::primary);

    addAndMakeVisible(slider);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    label.setFont(10.0f);
    addAndMakeVisible(label);
}

void MultiDelayEditor::updateParameterColors()
{
    auto neutralColor = HyperPrismLookAndFeel::Colors::onSurfaceVariant;
    masterMixLabel.setColour(juce::Label::textColourId, neutralColor);
    globalFeedbackLabel.setColour(juce::Label::textColourId, neutralColor);

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

    updateSliderXY(masterMixSlider, MultiDelayProcessor::MASTER_MIX_ID);
    updateSliderXY(globalFeedbackSlider, MultiDelayProcessor::GLOBAL_FEEDBACK_ID);
    repaint();
}

void MultiDelayEditor::updateXYPadFromParameters()
{
    // For multiple parameters, use the average of their normalized values
    float xValue = 0.0f;
    float yValue = 0.0f;
    
    auto& vts = audioProcessor.getValueTreeState();
    
    // Calculate average X value
    if (!xParameterIDs.isEmpty())
    {
        for (const auto& paramID : xParameterIDs)
        {
            if (auto* param = vts.getRawParameterValue(paramID))
            {
                if (auto* paramObj = vts.getParameter(paramID))
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
            if (auto* param = vts.getRawParameterValue(paramID))
            {
                if (auto* paramObj = vts.getParameter(paramID))
                {
                    yValue += paramObj->convertTo0to1(*param);
                }
            }
        }
        yValue /= yParameterIDs.size();
    }
    
    xyPad.setValues(xValue, yValue);
}

void MultiDelayEditor::updateParametersFromXYPad(float x, float y)
{
    auto& vts = audioProcessor.getValueTreeState();
    
    // Update all assigned X parameters
    for (const auto& paramID : xParameterIDs)
    {
        if (auto* param = vts.getParameter(paramID))
            param->setValueNotifyingHost(x);
    }
    
    // Update all assigned Y parameters
    for (const auto& paramID : yParameterIDs)
    {
        if (auto* param = vts.getParameter(paramID))
            param->setValueNotifyingHost(y);
    }
}


void MultiDelayEditor::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        auto* source = event.eventComponent;
        auto paramID = source->getProperties()["xyParamID"].toString();
        if (paramID.isNotEmpty())
            showParameterMenu(source, paramID);
    }
}
void MultiDelayEditor::showParameterMenu(juce::Component* target, const juce::String& parameterID)
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
                    xParameterIDs.add(MultiDelayProcessor::DELAY1_TIME_ID);
                    
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
                    yParameterIDs.add(MultiDelayProcessor::DELAY1_LEVEL_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add(MultiDelayProcessor::DELAY1_TIME_ID);
                yParameterIDs.add(MultiDelayProcessor::DELAY1_LEVEL_ID);
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void MultiDelayEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == MultiDelayProcessor::MASTER_MIX_ID) return "Mix";
        if (paramID == MultiDelayProcessor::GLOBAL_FEEDBACK_ID) return "Global Feedback";
        
        // Handle delay parameters
        if (paramID.contains("delay1")) {
            if (paramID.contains("time")) return "D1 Time";
            if (paramID.contains("level")) return "D1 Level";
            if (paramID.contains("pan")) return "D1 Pan";
            if (paramID.contains("feedback")) return "D1 FB";
        }
        if (paramID.contains("delay2")) {
            if (paramID.contains("time")) return "D2 Time";
            if (paramID.contains("level")) return "D2 Level";
            if (paramID.contains("pan")) return "D2 Pan";
            if (paramID.contains("feedback")) return "D2 FB";
        }
        if (paramID.contains("delay3")) {
            if (paramID.contains("time")) return "D3 Time";
            if (paramID.contains("level")) return "D3 Level";
            if (paramID.contains("pan")) return "D3 Pan";
            if (paramID.contains("feedback")) return "D3 FB";
        }
        if (paramID.contains("delay4")) {
            if (paramID.contains("time")) return "D4 Time";
            if (paramID.contains("level")) return "D4 Level";
            if (paramID.contains("pan")) return "D4 Pan";
            if (paramID.contains("feedback")) return "D4 FB";
        }
        
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