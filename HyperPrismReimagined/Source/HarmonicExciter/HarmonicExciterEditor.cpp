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
    
    // Title
    titleLabel.setText("HARMONIC EXCITER", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(juce::FontOptions(16.0f).withStyle("Bold")));
    titleLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurface);
    titleLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(titleLabel);

    brandLabel.setText("HyperPrism Reimagined", juce::dontSendNotification);
    brandLabel.setFont(juce::Font(juce::FontOptions(10.0f)));
    brandLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    brandLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(brandLabel);
    
    // Setup sliders with consistent style (5 parameters - 4+1 layout)
    setupSlider(driveSlider, driveLabel, "Drive");
    setupSlider(frequencySlider, frequencyLabel, "Frequency");
    setupSlider(harmonicsSlider, harmonicsLabel, "Harmonics");
    setupSlider(mixSlider, mixLabel, "Mix");

    // Color-code knobs by category
    driveSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::dynamics);
    frequencySlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::dynamics);
    harmonicsSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::dynamics);
    mixSlider.setColour(juce::Slider::rotarySliderFillColourId, HyperPrismLookAndFeel::Colors::output);
    
    // Set up right-click handlers for parameter assignment
    driveLabel.onClick = [this]() { showParameterMenu(&driveLabel, DRIVE_ID); };
    frequencyLabel.onClick = [this]() { showParameterMenu(&frequencyLabel, FREQUENCY_ID); };
    harmonicsLabel.onClick = [this]() { showParameterMenu(&harmonicsLabel, HARMONICS_ID); };
    mixLabel.onClick = [this]() { showParameterMenu(&mixLabel, MIX_ID); };

    // Register right-click on sliders for XY pad assignment
    driveSlider.addMouseListener(this, true);
    driveSlider.getProperties().set("xyParamID", DRIVE_ID);
    frequencySlider.addMouseListener(this, true);
    frequencySlider.getProperties().set("xyParamID", FREQUENCY_ID);
    harmonicsSlider.addMouseListener(this, true);
    harmonicsSlider.getProperties().set("xyParamID", HARMONICS_ID);
    mixSlider.addMouseListener(this, true);
    mixSlider.getProperties().set("xyParamID", MIX_ID);

    
    // Type selector
    typeComboBox.addItem("Warm", 1);
    typeComboBox.addItem("Bright", 2);
    typeComboBox.setColour(juce::ComboBox::backgroundColourId, HyperPrismLookAndFeel::Colors::surfaceVariant);
    typeComboBox.setColour(juce::ComboBox::textColourId, HyperPrismLookAndFeel::Colors::onSurface);
    typeComboBox.setColour(juce::ComboBox::arrowColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    typeComboBox.setColour(juce::ComboBox::outlineColourId, HyperPrismLookAndFeel::Colors::outline);
    addAndMakeVisible(typeComboBox);
    
    typeLabel.setText("Type", juce::dontSendNotification);
    typeLabel.setJustificationType(juce::Justification::centred);
    typeLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
    addAndMakeVisible(typeLabel);
    
    // Bypass button (top right like AutoPan)
    // Bypass button
    bypassButton.setButtonText("Bypass");
    bypassButton.setClickingTogglesState(true);
    bypassButton.setColour(juce::TextButton::buttonOnColourId,
                            HyperPrismLookAndFeel::Colors::error.withAlpha(0.6f));
    bypassButton.setColour(juce::TextButton::textColourOnId,
                            HyperPrismLookAndFeel::Colors::onSurface);
    addAndMakeVisible(bypassButton);
    
    // Set slider ranges with proper step sizes
    driveSlider.setRange(0.0, 100.0, 0.1);
    driveSlider.setNumDecimalPlacesToDisplay(1);
    driveSlider.setTextValueSuffix(" %");

    frequencySlider.setRange(1000.0, 20000.0, 1.0);
    frequencySlider.setSkewFactor(0.3);
    frequencySlider.setNumDecimalPlacesToDisplay(0);
    frequencySlider.setTextValueSuffix(" Hz");

    harmonicsSlider.setRange(1.0, 5.0, 0.1);
    harmonicsSlider.setNumDecimalPlacesToDisplay(1);

    mixSlider.setRange(0.0, 100.0, 0.1);
    mixSlider.setNumDecimalPlacesToDisplay(1);
    mixSlider.setTextValueSuffix(" %");

    // Set initial values from processor parameters
    driveSlider.setValue(audioProcessor.driveParam->get());
    frequencySlider.setValue(audioProcessor.frequencyParam->get());
    harmonicsSlider.setValue(audioProcessor.harmonicsParam->get());
    mixSlider.setValue(audioProcessor.mixParam->get());
    typeComboBox.setSelectedId(audioProcessor.typeParam->getIndex() + 1);

    // Add listeners to update processor parameters
    // Use convertTo0to1/convertFrom0to1 pattern for proper normalization
    driveSlider.onValueChange = [this] {
        float normalized = static_cast<float>(driveSlider.getValue()) / 100.0f;
        audioProcessor.driveParam->setValueNotifyingHost(normalized);
        updateXYPadFromParameters();
    };
    frequencySlider.onValueChange = [this] {
        auto& range = audioProcessor.frequencyParam->getNormalisableRange();
        float normalized = range.convertTo0to1(static_cast<float>(frequencySlider.getValue()));
        audioProcessor.frequencyParam->setValueNotifyingHost(normalized);
        updateXYPadFromParameters();
    };
    harmonicsSlider.onValueChange = [this] {
        auto& range = audioProcessor.harmonicsParam->getNormalisableRange();
        float normalized = range.convertTo0to1(static_cast<float>(harmonicsSlider.getValue()));
        audioProcessor.harmonicsParam->setValueNotifyingHost(normalized);
        updateXYPadFromParameters();
    };
    mixSlider.onValueChange = [this] {
        float normalized = static_cast<float>(mixSlider.getValue()) / 100.0f;
        audioProcessor.mixParam->setValueNotifyingHost(normalized);
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
    xyPadLabel.setColour(juce::Label::textColourId, HyperPrismLookAndFeel::Colors::onSurfaceVariant);
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
    
    // Tooltips
    frequencySlider.setTooltip("Crossover frequency -- harmonics are generated above this point");
    driveSlider.setTooltip("Amount of harmonic saturation added");
    harmonicsSlider.setTooltip("Blend of even and odd harmonics -- affects tonal character");
    mixSlider.setTooltip("Balance between dry and excited signal");
    bypassButton.setTooltip("Bypass the effect");
    xyPad.setTooltip("Click and drag to control two parameters at once");

    setSize(700, 550);
    setResizable(true, true);
    setResizeLimits(600, 520, 900, 750);
}

HarmonicExciterEditor::~HarmonicExciterEditor()
{
    setLookAndFeel(nullptr);
}

void HarmonicExciterEditor::paint(juce::Graphics& g)
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

    paintColumnHeader(driveSlider.getX() - 2, driveSlider.getY() - 20, 200,
                      "HARMONICS", HyperPrismLookAndFeel::Colors::dynamics);
    paintColumnHeader(outputSectionX, outputSectionY, 140,
                      "OUTPUT", HyperPrismLookAndFeel::Colors::output);
}

void HarmonicExciterEditor::resized()
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

    // Column: HARMONICS -- Drive, Frequency, Harmonics
    int y1 = colTop + knobDiam / 2;
    centerKnob(driveSlider, driveLabel, col1.getX(), colWidth, y1, knobDiam);
    centerKnob(frequencySlider, frequencyLabel, col1.getX(), colWidth, y1 + vSpace, knobDiam);
    centerKnob(harmonicsSlider, harmonicsLabel, col1.getX(), colWidth, y1 + vSpace * 2, knobDiam);

    // Type ComboBox below knobs in column
    int comboY = y1 + vSpace * 2 + knobDiam / 2 + 36;
    typeLabel.setBounds(col1.getX(), comboY, colWidth, 14);
    typeComboBox.setBounds(col1.getX() + 5, comboY + 15, colWidth - 10, 24);

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

void HarmonicExciterEditor::setupControls()
{
    // This method is no longer used - moved into constructor
}

void HarmonicExciterEditor::setupXYPad()
{
    // This method is no longer used - moved into constructor
}

void HarmonicExciterEditor::setupSlider(juce::Slider& slider, ParameterLabel& label, 
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

void HarmonicExciterEditor::updateParameterColors()
{
    auto neutralColor = HyperPrismLookAndFeel::Colors::onSurfaceVariant;
    driveLabel.setColour(juce::Label::textColourId, neutralColor);
    frequencyLabel.setColour(juce::Label::textColourId, neutralColor);
    harmonicsLabel.setColour(juce::Label::textColourId, neutralColor);
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

    updateSliderXY(driveSlider, DRIVE_ID);
    updateSliderXY(frequencySlider, FREQUENCY_ID);
    updateSliderXY(harmonicsSlider, HARMONICS_ID);
    updateSliderXY(mixSlider, MIX_ID);
    repaint();
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


void HarmonicExciterEditor::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown())
    {
        auto* source = event.eventComponent;
        auto paramID = source->getProperties()["xyParamID"].toString();
        if (paramID.isNotEmpty())
            showParameterMenu(source, paramID);
    }
}
void HarmonicExciterEditor::showParameterMenu(juce::Component* target, const juce::String& parameterID)
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