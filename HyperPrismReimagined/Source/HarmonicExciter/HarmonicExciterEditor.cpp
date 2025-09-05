//==============================================================================
// HyperPrism Reimagined - Harmonic Exciter Editor Implementation
// Updated to match AutoPan template exactly
//==============================================================================

#include "HarmonicExciterEditor.h"

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
// HarmonicExciterEditor Implementation
//==============================================================================
HarmonicExciterEditor::HarmonicExciterEditor(HarmonicExciterProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&customLookAndFeel);
    
    // Initialize default parameter assignments
    xParameterIDs.add(DRIVE_ID);
    yParameterIDs.add(FREQUENCY_ID);
    
    // Title (matching AutoPan style)
    titleLabel.setText("HyperPrism Reimagined Harmonic Exciter", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions("Arial", "Bold", 24.0f)));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::cyan);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);
    
    // Setup sliders with consistent style (5 parameters - 4+1 layout)
    setupSlider(driveSlider, driveLabel, "Drive", "%");
    setupSlider(frequencySlider, frequencyLabel, "Frequency", " Hz");
    setupSlider(harmonicsSlider, harmonicsLabel, "Harmonics", "%");
    setupSlider(mixSlider, mixLabel, "Mix", "%");
    
    // Set up right-click handlers for parameter assignment
    driveLabel.onClick = [this]() { showParameterMenu(&driveLabel, DRIVE_ID); };
    frequencyLabel.onClick = [this]() { showParameterMenu(&frequencyLabel, FREQUENCY_ID); };
    harmonicsLabel.onClick = [this]() { showParameterMenu(&harmonicsLabel, HARMONICS_ID); };
    mixLabel.onClick = [this]() { showParameterMenu(&mixLabel, MIX_ID); };
    
    // Type selector
    typeComboBox.addItem("Warm", 1);
    typeComboBox.addItem("Bright", 2);
    typeComboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colours::darkgrey);
    typeComboBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    typeComboBox.setColour(juce::ComboBox::arrowColourId, juce::Colours::lightgrey);
    typeComboBox.setColour(juce::ComboBox::outlineColourId, juce::Colours::grey);
    addAndMakeVisible(typeComboBox);
    
    typeLabel.setText("Type", juce::dontSendNotification);
    typeLabel.setJustificationType(juce::Justification::centred);
    typeLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(typeLabel);
    
    // Bypass button (top right like AutoPan)
    bypassButton.setButtonText("BYPASS");
    bypassButton.setColour(juce::ToggleButton::textColourId, juce::Colours::lightgrey);
    bypassButton.setColour(juce::ToggleButton::tickColourId, juce::Colours::red);
    bypassButton.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colours::darkgrey);
    addAndMakeVisible(bypassButton);
    
    // Set initial values from processor parameters
    driveSlider.setValue(audioProcessor.driveParam->get() * 100.0);
    frequencySlider.setValue(audioProcessor.frequencyParam->get());
    harmonicsSlider.setValue(audioProcessor.harmonicsParam->get() * 100.0);
    mixSlider.setValue(audioProcessor.mixParam->get() * 100.0);
    typeComboBox.setSelectedId(audioProcessor.typeParam->getIndex() + 1);
    
    // Set slider ranges
    driveSlider.setRange(0.0, 100.0);
    frequencySlider.setRange(1000.0, 20000.0);
    frequencySlider.setSkewFactor(0.3);
    harmonicsSlider.setRange(0.0, 100.0);
    mixSlider.setRange(0.0, 100.0);
    
    // Add listeners to update processor parameters
    driveSlider.onValueChange = [this] { 
        audioProcessor.driveParam->setValueNotifyingHost(driveSlider.getValue() / 100.0f);
        updateXYPadFromParameters(); 
    };
    frequencySlider.onValueChange = [this] { 
        audioProcessor.frequencyParam->setValueNotifyingHost(frequencySlider.getValue());
        updateXYPadFromParameters(); 
    };
    harmonicsSlider.onValueChange = [this] { 
        audioProcessor.harmonicsParam->setValueNotifyingHost(harmonicsSlider.getValue() / 100.0f);
        updateXYPadFromParameters(); 
    };
    mixSlider.onValueChange = [this] { 
        audioProcessor.mixParam->setValueNotifyingHost(mixSlider.getValue() / 100.0f);
        updateXYPadFromParameters(); 
    };
    typeComboBox.onChange = [this] {
        audioProcessor.typeParam->setValueNotifyingHost(typeComboBox.getSelectedId() - 1);
    };
    
    // Setup XY Pad
    addAndMakeVisible(xyPad);
    xyPad.setAxisColors(xAssignmentColor, yAssignmentColor);
    xyPadLabel.setText("Drive / Frequency", juce::dontSendNotification);
    xyPadLabel.setJustificationType(juce::Justification::centred);
    xyPadLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(xyPadLabel);
    
    xyPad.onValueChange = [this](float x, float y) {
        updateParametersFromXYPad(x, y);
    };
    
    // Update XY pad position based on current parameters
    updateXYPadFromParameters();
    updateParameterColors();
    
    // Listen for parameter changes - update XY pad when any parameter changes
    driveSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    frequencySlider.onValueChange = [this] { updateXYPadFromParameters(); };
    harmonicsSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    mixSlider.onValueChange = [this] { updateXYPadFromParameters(); };
    
    setSize(650, 600);
}

HarmonicExciterEditor::~HarmonicExciterEditor()
{
    setLookAndFeel(nullptr);
}

void HarmonicExciterEditor::paint(juce::Graphics& g)
{
    // Dark background matching AutoPan
    g.fillAll(HyperPrismLookAndFeel::Colors::background);
}

void HarmonicExciterEditor::resized()
{
    auto bounds = getLocalBounds();
    
    // Title
    titleLabel.setBounds(bounds.removeFromTop(40));
    
    // Bypass button (top right)
    bypassButton.setBounds(bounds.getWidth() - 100, 10, 80, 30);
    
    bounds.reduce(20, 10);
    
    // 5 parameter layout: 4 sliders + 1 dropdown (similar to Phaser/HyperPhaser)
    // Available height after title and margins: ~500px
    // Distribution: 140px + 10px + 80px + 10px + 250px = 490px
    
    auto topRow = bounds.removeFromTop(140);
    auto sliderWidth = 80;
    auto spacing = 15;
    
    // Calculate total width needed for 4 sliders
    auto totalSliderWidth = sliderWidth * 4 + spacing * 3;
    auto startX = (bounds.getWidth() - totalSliderWidth) / 2;
    topRow.removeFromLeft(startX);
    
    // Top row: Drive, Frequency, Harmonics, Mix
    driveSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    driveLabel.setBounds(driveSlider.getX(), driveSlider.getBottom(), sliderWidth, 20);
    topRow.removeFromLeft(spacing);
    
    frequencySlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    frequencyLabel.setBounds(frequencySlider.getX(), frequencySlider.getBottom(), sliderWidth, 20);
    topRow.removeFromLeft(spacing);
    
    harmonicsSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    harmonicsLabel.setBounds(harmonicsSlider.getX(), harmonicsSlider.getBottom(), sliderWidth, 20);
    topRow.removeFromLeft(spacing);
    
    mixSlider.setBounds(topRow.removeFromLeft(sliderWidth).reduced(0, 20));
    mixLabel.setBounds(mixSlider.getX(), mixSlider.getBottom(), sliderWidth, 20);
    
    bounds.removeFromTop(10);
    
    // Second row - Type dropdown (centered)
    auto secondRow = bounds.removeFromTop(80);
    auto dropdownWidth = 120;
    auto dropdownX = bounds.getX() + (bounds.getWidth() - dropdownWidth) / 2;
    
    typeComboBox.setBounds(dropdownX, secondRow.getY() + 30, dropdownWidth, 25);
    typeLabel.setBounds(dropdownX, secondRow.getY() + 5, dropdownWidth, 20);
    
    // Bottom section - XY Pad
    bounds.removeFromTop(10);
    
    // Center the XY Pad horizontally (200x180 standard)
    auto xyPadWidth = 200;
    auto xyPadHeight = 180;
    auto xyPadX = bounds.getX() + (bounds.getWidth() - xyPadWidth) / 2;
    auto xyPadY = bounds.getY();
    
    xyPad.setBounds(xyPadX, xyPadY, xyPadWidth, xyPadHeight);
    xyPadLabel.setBounds(xyPadX, xyPadY + xyPadHeight + 5, xyPadWidth, 20);
}

void HarmonicExciterEditor::setupControls()
{
    // This method is no longer used - moved into constructor
}

void HarmonicExciterEditor::setupXYPad()
{
    // This method is no longer used - moved into constructor
}

void HarmonicExciterEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
                               const juce::String& text, const juce::String& suffix)
{
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
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
    addAndMakeVisible(label);
}

void HarmonicExciterEditor::updateParameterColors()
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
    
    updateLabelColor(driveLabel, DRIVE_ID);
    updateLabelColor(frequencyLabel, FREQUENCY_ID);
    updateLabelColor(harmonicsLabel, HARMONICS_ID);
    updateLabelColor(mixLabel, MIX_ID);
}

void HarmonicExciterEditor::updateXYPadFromParameters()
{
    // For multiple parameters, use the average of their normalized values
    float xValue = 0.0f;
    float yValue = 0.0f;
    
    // Calculate average X value
    if (!xParameterIDs.isEmpty())
    {
        for (const auto& paramID : xParameterIDs)
        {
            float normalizedValue = 0.0f;
            if (paramID == DRIVE_ID)
                normalizedValue = audioProcessor.driveParam->get();
            else if (paramID == FREQUENCY_ID)
                normalizedValue = (audioProcessor.frequencyParam->get() - 1000.0f) / 19000.0f;
            else if (paramID == HARMONICS_ID)
                normalizedValue = audioProcessor.harmonicsParam->get();
            else if (paramID == MIX_ID)
                normalizedValue = audioProcessor.mixParam->get();
                
            xValue += normalizedValue;
        }
        xValue /= xParameterIDs.size();
    }
    
    // Calculate average Y value
    if (!yParameterIDs.isEmpty())
    {
        for (const auto& paramID : yParameterIDs)
        {
            float normalizedValue = 0.0f;
            if (paramID == DRIVE_ID)
                normalizedValue = audioProcessor.driveParam->get();
            else if (paramID == FREQUENCY_ID)
                normalizedValue = (audioProcessor.frequencyParam->get() - 1000.0f) / 19000.0f;
            else if (paramID == HARMONICS_ID)
                normalizedValue = audioProcessor.harmonicsParam->get();
            else if (paramID == MIX_ID)
                normalizedValue = audioProcessor.mixParam->get();
                
            yValue += normalizedValue;
        }
        yValue /= yParameterIDs.size();
    }
    
    xyPad.setValues(xValue, yValue);
}

void HarmonicExciterEditor::updateParametersFromXYPad(float x, float y)
{
    // Update all assigned X parameters
    for (const auto& paramID : xParameterIDs)
    {
        if (paramID == DRIVE_ID)
        {
            audioProcessor.driveParam->setValueNotifyingHost(x);
            driveSlider.setValue(x * 100.0, juce::dontSendNotification);
        }
        else if (paramID == FREQUENCY_ID)
        {
            float freq = 1000.0f + (x * 19000.0f);
            audioProcessor.frequencyParam->setValueNotifyingHost(freq);
            frequencySlider.setValue(freq, juce::dontSendNotification);
        }
        else if (paramID == HARMONICS_ID)
        {
            audioProcessor.harmonicsParam->setValueNotifyingHost(x);
            harmonicsSlider.setValue(x * 100.0, juce::dontSendNotification);
        }
        else if (paramID == MIX_ID)
        {
            audioProcessor.mixParam->setValueNotifyingHost(x);
            mixSlider.setValue(x * 100.0, juce::dontSendNotification);
        }
    }
    
    // Update all assigned Y parameters
    for (const auto& paramID : yParameterIDs)
    {
        if (paramID == DRIVE_ID)
        {
            audioProcessor.driveParam->setValueNotifyingHost(y);
            driveSlider.setValue(y * 100.0, juce::dontSendNotification);
        }
        else if (paramID == FREQUENCY_ID)
        {
            float freq = 1000.0f + (y * 19000.0f);
            audioProcessor.frequencyParam->setValueNotifyingHost(freq);
            frequencySlider.setValue(freq, juce::dontSendNotification);
        }
        else if (paramID == HARMONICS_ID)
        {
            audioProcessor.harmonicsParam->setValueNotifyingHost(y);
            harmonicsSlider.setValue(y * 100.0, juce::dontSendNotification);
        }
        else if (paramID == MIX_ID)
        {
            audioProcessor.mixParam->setValueNotifyingHost(y);
            mixSlider.setValue(y * 100.0, juce::dontSendNotification);
        }
    }
}

void HarmonicExciterEditor::showParameterMenu(juce::Label* label, const juce::String& parameterID)
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
                    xParameterIDs.add(DRIVE_ID);
                    
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
                    yParameterIDs.add(FREQUENCY_ID);
                    
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
            else if (result == 3)
            {
                // Clear all and restore defaults
                xParameterIDs.clear();
                yParameterIDs.clear();
                xParameterIDs.add(DRIVE_ID);
                yParameterIDs.add(FREQUENCY_ID);
                updateXYPadLabel();
                updateParameterColors();
                updateXYPadFromParameters();
            }
        });
}

void HarmonicExciterEditor::updateXYPadLabel()
{
    auto getParameterName = [](const juce::String& paramID) -> juce::String {
        if (paramID == DRIVE_ID) return "Drive";
        if (paramID == FREQUENCY_ID) return "Frequency";
        if (paramID == HARMONICS_ID) return "Harmonics";
        if (paramID == MIX_ID) return "Mix";
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