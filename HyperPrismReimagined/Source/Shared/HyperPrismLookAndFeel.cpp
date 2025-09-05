//==============================================================================
// HyperPrism Revived - Modern Look and Feel Implementation
//==============================================================================

#include "HyperPrismLookAndFeel.h"

// Color Definitions - Modern Dark Theme with Cyan Accents
const juce::Colour HyperPrismLookAndFeel::Colors::background          = juce::Colour(0xff0d1117);  // Deep dark
const juce::Colour HyperPrismLookAndFeel::Colors::surface             = juce::Colour(0xff161b22);  // Panel background
const juce::Colour HyperPrismLookAndFeel::Colors::surfaceVariant      = juce::Colour(0xff21262d);  // Control backgrounds
const juce::Colour HyperPrismLookAndFeel::Colors::primary             = juce::Colour(0xff00d9ff);  // Cyan accent
const juce::Colour HyperPrismLookAndFeel::Colors::primaryVariant      = juce::Colour(0xff0099cc);  // Darker cyan
const juce::Colour HyperPrismLookAndFeel::Colors::secondary           = juce::Colour(0xff6f42c1);  // Purple accent
const juce::Colour HyperPrismLookAndFeel::Colors::onSurface           = juce::Colour(0xfff0f6fc);  // Main text
const juce::Colour HyperPrismLookAndFeel::Colors::onSurfaceVariant    = juce::Colour(0xff8b949e);  // Secondary text
const juce::Colour HyperPrismLookAndFeel::Colors::outline             = juce::Colour(0xff30363d);  // Borders
const juce::Colour HyperPrismLookAndFeel::Colors::outlineVariant      = juce::Colour(0xff21262d);  // Subtle borders
const juce::Colour HyperPrismLookAndFeel::Colors::error               = juce::Colour(0xffff4545);  // Error red
const juce::Colour HyperPrismLookAndFeel::Colors::warning             = juce::Colour(0xffffab00);  // Warning amber
const juce::Colour HyperPrismLookAndFeel::Colors::success             = juce::Colour(0xff00ff41);  // Success green

HyperPrismLookAndFeel::HyperPrismLookAndFeel()
{
    setupFonts();
    
    // Set default colors
    setColour(juce::ResizableWindow::backgroundColourId, Colors::background);
    setColour(juce::DocumentWindow::backgroundColourId, Colors::background);
    
    // Slider colors
    setColour(juce::Slider::backgroundColourId, Colors::surfaceVariant);
    setColour(juce::Slider::thumbColourId, Colors::primary);
    setColour(juce::Slider::trackColourId, Colors::primary.withAlpha(0.3f));
    setColour(juce::Slider::rotarySliderFillColourId, Colors::primary);
    setColour(juce::Slider::rotarySliderOutlineColourId, Colors::outline);
    setColour(juce::Slider::textBoxTextColourId, Colors::onSurface);
    setColour(juce::Slider::textBoxBackgroundColourId, Colors::surfaceVariant);
    setColour(juce::Slider::textBoxOutlineColourId, Colors::outline);
    
    // Button colors
    setColour(juce::TextButton::buttonColourId, Colors::surfaceVariant);
    setColour(juce::TextButton::buttonOnColourId, Colors::primary);
    setColour(juce::TextButton::textColourOnId, Colors::background);
    setColour(juce::TextButton::textColourOffId, Colors::onSurface);
    
    // Toggle button colors
    setColour(juce::ToggleButton::tickColourId, Colors::primary);
    setColour(juce::ToggleButton::tickDisabledColourId, Colors::onSurfaceVariant);
    setColour(juce::ToggleButton::textColourId, Colors::onSurface);
    
    // Label colors
    setColour(juce::Label::textColourId, Colors::onSurface);
    setColour(juce::Label::textWhenEditingColourId, Colors::onSurface);
    setColour(juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Label::backgroundWhenEditingColourId, Colors::surfaceVariant);
    setColour(juce::Label::outlineColourId, Colors::outline);
    setColour(juce::Label::outlineWhenEditingColourId, Colors::primary);
}

void HyperPrismLookAndFeel::setupFonts()
{
    // Use system fonts for now - can be customized with embedded fonts later
    titleFont = juce::Font(juce::FontOptions("Arial", "Bold", 24.0f));
    bodyFont = juce::Font(juce::FontOptions(14.0f));
    captionFont = juce::Font(juce::FontOptions(12.0f));
}

void HyperPrismLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                            float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                            juce::Slider& slider)
{
    auto radius = (float) juce::jmin(width / 2, height / 2) - 4.0f;
    auto centreX = (float) x + (float) width * 0.5f;
    auto centreY = (float) y + (float) height * 0.5f;
    auto rx = centreX - radius;
    auto ry = centreY - radius;
    auto rw = radius * 2.0f;
    auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    // Draw outer ring
    g.setColour(Colors::outline);
    g.drawEllipse(rx, ry, rw, rw, 2.0f);
    
    // Draw inner fill
    g.setColour(Colors::surfaceVariant);
    g.fillEllipse(rx + 2, ry + 2, rw - 4, rw - 4);
    
    // Draw value arc
    juce::Path valueArc;
    valueArc.addPieSegment(rx, ry, rw, rw, rotaryStartAngle, angle, 0.0f);
    g.setColour(Colors::primary.withAlpha(0.3f));
    g.fillPath(valueArc);
    
    // Draw value arc outline
    g.setColour(Colors::primary);
    g.strokePath(valueArc, juce::PathStrokeType(2.0f));
    
    // Draw pointer
    juce::Path pointer;
    auto pointerLength = radius * 0.7f;
    auto pointerThickness = 3.0f;
    pointer.addRectangle(-pointerThickness * 0.5f, -radius + 8, pointerThickness, pointerLength);
    pointer.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY));
    
    g.setColour(Colors::primary);
    g.fillPath(pointer);
    
    // Draw center dot
    g.setColour(Colors::onSurface);
    g.fillEllipse(centreX - 3, centreY - 3, 6, 6);
}

void HyperPrismLookAndFeel::drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                                            float sliderPos, float minSliderPos, float maxSliderPos,
                                            juce::Slider::SliderStyle style, juce::Slider& slider)
{
    if (slider.isBar())
    {
        g.setColour(slider.findColour(juce::Slider::trackColourId));
        g.fillRect(slider.isHorizontal() ? juce::Rectangle<float>(static_cast<float>(x), (float) y + 0.5f, sliderPos - (float) x, (float) height - 1.0f)
                                         : juce::Rectangle<float>((float) x + 0.5f, sliderPos, (float) width - 1.0f, (float) y + ((float) height - sliderPos)));
    }
    else
    {
        auto isTwoVal = (style == juce::Slider::SliderStyle::TwoValueVertical || style == juce::Slider::SliderStyle::TwoValueHorizontal);
        auto isThreeVal = (style == juce::Slider::SliderStyle::ThreeValueVertical || style == juce::Slider::SliderStyle::ThreeValueHorizontal);
        
        auto trackWidth = juce::jmin(6.0f, slider.isHorizontal() ? (float) height * 0.25f : (float) width * 0.25f);
        
        juce::Point<float> startPoint(slider.isHorizontal() ? (float) x : (float) x + (float) width * 0.5f,
                                      slider.isHorizontal() ? (float) y + (float) height * 0.5f : (float) (height + y));
        
        juce::Point<float> endPoint(slider.isHorizontal() ? (float) (width + x) : startPoint.x,
                                    slider.isHorizontal() ? startPoint.y : (float) y);
        
        juce::Path backgroundTrack;
        backgroundTrack.startNewSubPath(startPoint);
        backgroundTrack.lineTo(endPoint);
        g.setColour(Colors::outline);
        g.strokePath(backgroundTrack, { trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });
        
        juce::Path valueTrack;
        juce::Point<float> minPoint, maxPoint, thumbPoint;
        
        if (isTwoVal || isThreeVal)
        {
            minPoint = { slider.isHorizontal() ? minSliderPos : (float) width * 0.5f,
                        slider.isHorizontal() ? (float) height * 0.5f : minSliderPos };
            
            maxPoint = { slider.isHorizontal() ? maxSliderPos : (float) width * 0.5f,
                        slider.isHorizontal() ? (float) height * 0.5f : maxSliderPos };
        }
        else
        {
            auto kx = slider.isHorizontal() ? sliderPos : ((float) x + (float) width * 0.5f);
            auto ky = slider.isHorizontal() ? ((float) y + (float) height * 0.5f) : sliderPos;
            
            minPoint = startPoint;
            maxPoint = { kx, ky };
        }
        
        valueTrack.startNewSubPath(minPoint);
        valueTrack.lineTo(maxPoint);
        g.setColour(Colors::primary);
        g.strokePath(valueTrack, { trackWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded });
        
        // Draw thumb
        auto thumbWidth = getSliderThumbRadius(slider);
        
        if (!isTwoVal)
        {
            g.setColour(Colors::primary);
            g.fillEllipse(juce::Rectangle<float>(static_cast<float>(thumbWidth), static_cast<float>(thumbWidth)).withCentre(maxPoint));
            
            g.setColour(Colors::background);
            g.fillEllipse(juce::Rectangle<float>(static_cast<float>(thumbWidth) * 0.6f, static_cast<float>(thumbWidth) * 0.6f).withCentre(maxPoint));
        }
        
        if (isTwoVal || isThreeVal)
        {
            auto sr = juce::jmin(trackWidth, (slider.isHorizontal() ? (float) height : (float) width) * 0.4f);
            
            g.setColour(Colors::primary);
            g.fillEllipse(juce::Rectangle<float>(sr, sr).withCentre(minPoint));
            g.fillEllipse(juce::Rectangle<float>(sr, sr).withCentre(maxPoint));
        }
    }
}

void HyperPrismLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                            bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto fontSize = juce::jmin(15.0f, (float) button.getHeight() * 0.75f);
    auto tickWidth = fontSize * 1.1f;
    
    drawTickBox(g, button, 4.0f, ((float) button.getHeight() - tickWidth) * 0.5f,
                tickWidth, tickWidth,
                button.getToggleState(),
                button.isEnabled(),
                shouldDrawButtonAsHighlighted,
                shouldDrawButtonAsDown);
    
    g.setColour(button.findColour(juce::ToggleButton::textColourId));
    g.setFont(fontSize);
    
    if (! button.isEnabled())
        g.setOpacity(0.5f);
    
    g.drawFittedText(button.getButtonText(),
                     button.getLocalBounds().withTrimmedLeft(juce::roundToInt(tickWidth) + 10)
                                           .withTrimmedRight(2),
                     juce::Justification::centredLeft, 10);
}

void HyperPrismLookAndFeel::drawTickBox(juce::Graphics& g, juce::Component&,
                                       float x, float y, float w, float h,
                                       bool ticked, bool isEnabled,
                                       bool, bool)
{
    auto tickBounds = juce::Rectangle<float>(x, y, w, h).reduced(1);
    
    g.setColour(isEnabled ? Colors::surfaceVariant : Colors::surfaceVariant.withAlpha(0.5f));
    g.fillRoundedRectangle(tickBounds, 3.0f);
    
    g.setColour(isEnabled ? Colors::outline : Colors::outline.withAlpha(0.5f));
    g.drawRoundedRectangle(tickBounds, 3.0f, 1.5f);
    
    if (ticked)
    {
        g.setColour(isEnabled ? Colors::primary : Colors::primary.withAlpha(0.5f));
        
        juce::Path tick;
        tick.startNewSubPath(x + w * 0.28f, y + h * 0.5f);
        tick.lineTo(x + w * 0.45f, y + h * 0.75f);
        tick.lineTo(x + w * 0.72f, y + h * 0.25f);
        
        g.strokePath(tick, juce::PathStrokeType(2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

void HyperPrismLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                const juce::Colour& backgroundColour,
                                                bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto cornerSize = 6.0f;
    auto bounds = button.getLocalBounds().toFloat().reduced(0.5f, 0.5f);
    
    auto baseColour = backgroundColour.withMultipliedSaturation(button.hasKeyboardFocus(true) ? 1.3f : 0.9f)
                                     .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f);
    
    if (shouldDrawButtonAsDown || shouldDrawButtonAsHighlighted)
        baseColour = baseColour.contrasting(shouldDrawButtonAsDown ? 0.2f : 0.05f);
    
    g.setColour(baseColour);
    g.fillRoundedRectangle(bounds, cornerSize);
    
    g.setColour(Colors::outline);
    g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
}

void HyperPrismLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& button,
                                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown)
{
    auto font = getTextButtonFont(button, button.getHeight());
    g.setFont(font);
    g.setColour(button.findColour(button.getToggleState() ? juce::TextButton::textColourOnId
                                                          : juce::TextButton::textColourOffId)
                      .withMultipliedAlpha(button.isEnabled() ? 1.0f : 0.5f));
    
    auto yIndent = juce::jmin(4, button.proportionOfHeight(0.3f));
    auto cornerSize = juce::jmin(button.getHeight(), button.getWidth()) / 2;
    
    auto fontHeight = juce::roundToInt(font.getHeight() * 0.6f);
    auto leftIndent = juce::jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnLeft() ? 4 : 2));
    auto rightIndent = juce::jmin(fontHeight, 2 + cornerSize / (button.isConnectedOnRight() ? 4 : 2));
    auto textWidth = button.getWidth() - leftIndent - rightIndent;
    
    if (textWidth > 0)
        g.drawFittedText(button.getButtonText(),
                         leftIndent, yIndent, textWidth, button.getHeight() - yIndent * 2,
                         juce::Justification::centred, 2);
}

juce::Label* HyperPrismLookAndFeel::createSliderTextBox(juce::Slider& slider)
{
    auto* l = new juce::Label();
    
    l->setJustificationType(juce::Justification::centred);
    l->setKeyboardType(juce::TextInputTarget::decimalKeyboard);
    
    l->setColour(juce::Label::textColourId, slider.findColour(juce::Slider::textBoxTextColourId));
    l->setColour(juce::Label::backgroundColourId,
                 (slider.getSliderStyle() == juce::Slider::LinearBar || slider.getSliderStyle() == juce::Slider::LinearBarVertical)
                     ? juce::Colours::transparentBlack
                     : slider.findColour(juce::Slider::textBoxBackgroundColourId));
    l->setColour(juce::Label::outlineColourId, slider.findColour(juce::Slider::textBoxOutlineColourId));
    l->setFont(captionFont);
    
    return l;
}

void HyperPrismLookAndFeel::fillTextEditorBackground(juce::Graphics& g, int width, int height,
                                                     juce::TextEditor& textEditor)
{
    g.fillAll(textEditor.findColour(juce::TextEditor::backgroundColourId));
}

void HyperPrismLookAndFeel::drawTextEditorOutline(juce::Graphics& g, int width, int height,
                                                  juce::TextEditor& textEditor)
{
    if (textEditor.isEnabled())
    {
        if (textEditor.hasKeyboardFocus(true) && ! textEditor.isReadOnly())
        {
            g.setColour(Colors::primary);
            g.drawRect(0, 0, width, height, 2);
        }
        else
        {
            g.setColour(Colors::outline);
            g.drawRect(0, 0, width, height);
        }
    }
}

juce::Font HyperPrismLookAndFeel::getTextButtonFont(juce::TextButton& button, int buttonHeight)
{
    return bodyFont.withHeight(juce::jmin(16.0f, (float) buttonHeight * 0.6f));
}

juce::Font HyperPrismLookAndFeel::getLabelFont(juce::Label& label)
{
    return bodyFont;
}

juce::Font HyperPrismLookAndFeel::getSliderPopupFont(juce::Slider& slider)
{
    return captionFont;
}