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
    
    // Title (matching AutoPan style)
    titleLabel.setText("HyperPrism Reimagined Multi Delay", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions("Arial", "Bold", 24.0f)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Setup global controls
    setupSlider(masterMixSlider, masterMixLabel, "Master Mix", "");
    setupSlider(globalFeedbackSlider, globalFeedbackLabel, "Global Feedback", "");
    
    // Set up right-click handlers for global parameters
    masterMixLabel.onClick = [this]() { showParameterMenu(&masterMixLabel, MultiDelayProcessor::MASTER_MIX_ID); };
    globalFeedbackLabel.onClick = [this]() { showParameterMenu(&globalFeedbackLabel, MultiDelayProcessor::GLOBAL_FEEDBACK_ID); };
    
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
        setupSlider(delayTimeSliders[i], delayTimeLabels[i], "Time", " ms");
        setupSlider(delayLevelSliders[i], delayLevelLabels[i], "Level", "");
        setupSlider(delayPanSliders[i], delayPanLabels[i], "Pan", "");
        setupSlider(delayFeedbackSliders[i], delayFeedbackLabels[i], "FB", "");
        
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
    
    // Bypass button (top right like AutoPan)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::red);
    bypassButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
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
    xyPadLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    
    // Add multi-delay meter
    addAndMakeVisible(multiDelayMeter);
    multiDelayMeterLabel.setText("Delay Levels", juce::dontSendNotification);
    multiDelayMeterLabel.setJustificationType(juce::Justification::centred);
    multiDelayMeterLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(multiDelayMeterLabel);
    
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
    
    // Optimized size while maintaining functionality 
    setSize(650, 600);
}

MultiDelayEditor::~MultiDelayEditor()
{
    setLookAndFeel(nullptr);
}

void MultiDelayEditor::paint(juce::Graphics& g)
{
    // Dark background matching AutoPan
    g.fillAll(HyperPrismLookAndFeel::Colors::background);
}

void MultiDelayEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(50));
    
    // Bypass button (top right)
    bypassButton.setBounds(bounds.getWidth() - 100, 15, 80, 30);
    
    bounds.reduce(25, 15);
    
    // Global controls at top - tighter spacing
    auto globalRow = bounds.removeFromTop(110);
    auto sliderWidth = 80;
    auto globalSpacing = 30;
    
    // Center the global controls
    auto globalControlsWidth = sliderWidth * 2 + globalSpacing;
    auto globalStartX = (globalRow.getWidth() - globalControlsWidth) / 2;
    globalRow.removeFromLeft(globalStartX);
    
    masterMixSlider.setBounds(globalRow.removeFromLeft(sliderWidth).reduced(0, 15));
    masterMixLabel.setBounds(masterMixSlider.getX(), masterMixSlider.getBottom(), sliderWidth, 20);
    globalRow.removeFromLeft(globalSpacing);
    
    globalFeedbackSlider.setBounds(globalRow.removeFromLeft(sliderWidth).reduced(0, 15));
    globalFeedbackLabel.setBounds(globalFeedbackSlider.getX(), globalFeedbackSlider.getBottom(), sliderWidth, 20);
    
    bounds.removeFromTop(15);
    
    // Delay controls with tighter spacing - 2x2 layout 
    auto delaySection = bounds.removeFromTop(250);
    auto delayPairWidth = delaySection.getWidth() / 2;
    auto delayPairHeight = delaySection.getHeight() / 2;
    
    for (int row = 0; row < 2; ++row)
    {
        for (int col = 0; col < 2; ++col)
        {
            int i = row * 2 + col;
            auto pairBounds = juce::Rectangle<int>(
                delaySection.getX() + col * delayPairWidth,
                delaySection.getY() + row * delayPairHeight,
                delayPairWidth,
                delayPairHeight
            ).reduced(10);
            
            // Group label
            delayGroupLabels[i].setBounds(pairBounds.removeFromTop(20));
            pairBounds.removeFromTop(5);
            
            // 2x2 grid of controls with tighter spacing
            auto controlSize = 65;
            auto controlSpacing = 12;
            auto gridWidth = controlSize * 2 + controlSpacing;
            auto gridStartX = pairBounds.getX() + (pairBounds.getWidth() - gridWidth) / 2;
            
            // Row 1: Time and Level
            auto controlRow1 = pairBounds.removeFromTop(75);
            controlRow1.removeFromLeft(gridStartX - pairBounds.getX());
            
            delayTimeSliders[i].setBounds(controlRow1.removeFromLeft(controlSize).reduced(0, 8));
            delayTimeLabels[i].setBounds(delayTimeSliders[i].getX(), delayTimeSliders[i].getBottom() + 2, controlSize, 15);
            controlRow1.removeFromLeft(controlSpacing);
            
            delayLevelSliders[i].setBounds(controlRow1.removeFromLeft(controlSize).reduced(0, 8));
            delayLevelLabels[i].setBounds(delayLevelSliders[i].getX(), delayLevelSliders[i].getBottom() + 2, controlSize, 15);
            
            // Row 2: Pan and Feedback
            auto controlRow2 = pairBounds.removeFromTop(75);
            controlRow2.removeFromLeft(gridStartX - pairBounds.getX());
            
            delayPanSliders[i].setBounds(controlRow2.removeFromLeft(controlSize).reduced(0, 8));
            delayPanLabels[i].setBounds(delayPanSliders[i].getX(), delayPanSliders[i].getBottom() + 2, controlSize, 15);
            controlRow2.removeFromLeft(controlSpacing);
            
            delayFeedbackSliders[i].setBounds(controlRow2.removeFromLeft(controlSize).reduced(0, 8));
            delayFeedbackLabels[i].setBounds(delayFeedbackSliders[i].getX(), delayFeedbackSliders[i].getBottom() + 2, controlSize, 15);
        }
    }
    
    bounds.removeFromTop(15);
    
    // Bottom section - XY Pad and Meter side by side with tighter spacing
    auto bottomArea = bounds;
    
    // Calculate positioning to center both panels
    auto xyPadWidth = 180;
    auto xyPadHeight = 160;
    auto meterWidth = 300;
    auto meterHeight = 160; // Match XY pad height  
    auto panelSpacing = 25;
    auto totalWidth = xyPadWidth + meterWidth + panelSpacing;
    auto startX = (bottomArea.getWidth() - totalWidth) / 2;
    
    // XY Pad on left
    xyPad.setBounds(bottomArea.getX() + startX, bottomArea.getY(), xyPadWidth, xyPadHeight);
    
    // Multi-delay meter on right (matching height)
    multiDelayMeter.setBounds(xyPad.getRight() + panelSpacing, bottomArea.getY(), meterWidth, meterHeight);
    
    // Align labels at the same Y position
    auto labelY = xyPad.getBottom() + 5;
    xyPadLabel.setBounds(xyPad.getX(), labelY, xyPadWidth, 20);
    multiDelayMeterLabel.setBounds(multiDelayMeter.getX(), labelY, meterWidth, 20);
}

void MultiDelayEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
                               const juce::String& text, const juce::String& suffix)
{
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 15);
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::darkgrey);
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::grey);
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::cyan);
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::lightgrey);
    slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    
    if (!suffix.isEmpty())
        slider.setTextValueSuffix(suffix);
    
    addAndMakeVisible(slider);
    
    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    label.setFont(10.0f);
    addAndMakeVisible(label);
}

void MultiDelayEditor::updateParameterColors()
{
    // Update label colors based on X/Y assignments
    auto updateLabelColor = [this](ParameterLabel& label, const juce::String& paramID) {
        bool isAssignedToX = xParameterIDs.contains(paramID);
        bool isAssignedToY = yParameterIDs.contains(paramID);
        
        if (isAssignedToX && isAssignedToY)
            label.setColour(juce::Label::textColourId, xAssignmentColor.interpolatedWith(yAssignmentColor, 0.5f));
        else if (isAssignedToX)
            label.setColour(juce::Label::textColourId, xAssignmentColor);
        else if (isAssignedToY)
            label.setColour(juce::Label::textColourId, yAssignmentColor);
        else
            label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    };
    
    updateLabelColor(masterMixLabel, MultiDelayProcessor::MASTER_MIX_ID);
    updateLabelColor(globalFeedbackLabel, MultiDelayProcessor::GLOBAL_FEEDBACK_ID);
    
    // Update all delay parameter labels
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
        updateLabelColor(delayTimeLabels[i], delayIds[i][0]);
        updateLabelColor(delayLevelLabels[i], delayIds[i][1]);
        updateLabelColor(delayPanLabels[i], delayIds[i][2]);
        updateLabelColor(delayFeedbackLabels[i], delayIds[i][3]);
    }
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

void MultiDelayEditor::showParameterMenu(juce::Label* label, const juce::String& parameterID)
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
        .withTargetComponent(label)
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
        if (paramID == MultiDelayProcessor::MASTER_MIX_ID) return "Master Mix";
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