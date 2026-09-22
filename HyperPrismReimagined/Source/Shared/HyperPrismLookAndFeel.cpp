//==============================================================================
// HyperPrism Reimagined - Look and Feel Implementation
//==============================================================================

#include "HyperPrismLookAndFeel.h"

namespace tok = zqsfx::ui::colour;

// Base colours -- byte-identical to the corresponding zqsfx::ui::colour token (see header).
const juce::Colour HyperPrismLookAndFeel::Colors::background       = tok::chassisMid;
const juce::Colour HyperPrismLookAndFeel::Colors::surface          = tok::panelBot;
const juce::Colour HyperPrismLookAndFeel::Colors::surfaceVariant   = tok::panelTop;
const juce::Colour HyperPrismLookAndFeel::Colors::primary          = tok::accent;
const juce::Colour HyperPrismLookAndFeel::Colors::primaryVariant   = tok::accentDim;
const juce::Colour HyperPrismLookAndFeel::Colors::onSurface        = tok::btnText;
const juce::Colour HyperPrismLookAndFeel::Colors::onSurfaceVariant = tok::silkLabel;
const juce::Colour HyperPrismLookAndFeel::Colors::outline          = tok::ruleTitle;
const juce::Colour HyperPrismLookAndFeel::Colors::outlineVariant   = tok::ruleInner;
const juce::Colour HyperPrismLookAndFeel::Colors::error            = tok::warn;
const juce::Colour HyperPrismLookAndFeel::Colors::warning          = tok::meterHot;
const juce::Colour HyperPrismLookAndFeel::Colors::success          = tok::lcdText;

// Parameter-group colours -- the five colour-blind-safe complementary channels.
const juce::Colour HyperPrismLookAndFeel::Colors::dynamics   = zqsfx::ui::comp::sky;
const juce::Colour HyperPrismLookAndFeel::Colors::frequency  = zqsfx::ui::comp::yellow;
const juce::Colour HyperPrismLookAndFeel::Colors::modulation = zqsfx::ui::comp::purple;
const juce::Colour HyperPrismLookAndFeel::Colors::output     = zqsfx::ui::comp::green;
const juce::Colour HyperPrismLookAndFeel::Colors::timing     = zqsfx::ui::comp::white;

HyperPrismLookAndFeel::HyperPrismLookAndFeel()
{
    // The base zqsfx::ui::LookAndFeel constructor already sets every colour ID the old ctor used
    // to set by hand: ResizableWindow/Label -> chassis/silk, ComboBox/PopupMenu -> LCD glass,
    // TextButton -> btn gradient / accent-on, Slider textbox -> LCD glass + glow,
    // TooltipWindow/AlertWindow/TextEditor -> house tokens. Nothing here needs to re-set any of
    // that. rotarySliderFillColourId / rotarySliderOutlineColourId / thumbColourId /
    // trackColourId are gone too: the house's filmstrip knobs carry their own pointer and
    // consult no per-slider colour ID at all (zqsfx::ui::LookAndFeel::drawRotarySlider /
    // drawVectorKnob) -- see each editor's remaining `rotarySliderFillColourId` cleanup in
    // docs/ui_migration_report.md.
}

// ---------------------------------------------------------------------- Toggle buttons
void HyperPrismLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                              bool /*shouldDrawButtonAsHighlighted*/, bool /*shouldDrawButtonAsDown*/)
{
    namespace colour = zqsfx::ui::colour;

    const bool enabled = button.isEnabled();
    const float alphaMul = enabled ? 1.0f : zqsfx::ui::geom::dimAlpha;

    const auto fontSize = juce::jmin (15.0f, (float) button.getHeight() * 0.75f);
    const auto tickWidth = fontSize * 1.1f;
    const juce::Rectangle<float> box (4.0f, ((float) button.getHeight() - tickWidth) * 0.5f, tickWidth, tickWidth);

    // Hard-edged box -- no rounded corners (style guide section 6), `btn` gradient off-state.
    g.setGradientFill (zqsfx::ui::gradients::button (box, enabled));
    g.fillRect (box);
    g.setColour (colour::btnBorder.withAlpha (alphaMul));
    g.drawRect (box, 1.0f);

    if (button.getToggleState())
    {
        g.setColour (colour::accent.withAlpha (alphaMul));
        g.fillRect (box.reduced (2.0f));

        juce::Path tick;
        tick.startNewSubPath (box.getX() + box.getWidth() * 0.22f, box.getY() + box.getHeight() * 0.52f);
        tick.lineTo (box.getX() + box.getWidth() * 0.42f, box.getY() + box.getHeight() * 0.75f);
        tick.lineTo (box.getX() + box.getWidth() * 0.80f, box.getY() + box.getHeight() * 0.25f);
        g.setColour (colour::accentInk.withAlpha (alphaMul));
        g.strokePath (tick, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    g.setColour ((enabled ? colour::btnText : colour::silkCaption).withAlpha (alphaMul));
    g.setFont (silkFont (fontSize, true));
    g.drawFittedText (button.getButtonText(),
                      button.getLocalBounds().withTrimmedLeft (juce::roundToInt (tickWidth) + 10).withTrimmedRight (2),
                      juce::Justification::centredLeft, 1);
}

// ---------------------------------------------------------------------- XY-pad badges
void HyperPrismLookAndFeel::paintXYAssignmentBadges (juce::Graphics& g, juce::Component& parent)
{
    namespace colour = zqsfx::ui::colour;

    for (auto* child : parent.getChildren())
    {
        auto* slider = dynamic_cast<juce::Slider*> (child);
        if (slider == nullptr || ! slider->isVisible())
            continue;

        const bool isX = slider->getProperties().contains ("xyAxisX");
        const bool isY = slider->getProperties().contains ("xyAxisY");
        if (! isX && ! isY)
            continue;

        const juce::String text = (isX && isY) ? "XY" : (isX ? "X" : "Y");
        const auto b = slider->getBounds();
        const auto chip = juce::Rectangle<int> (b.getRight() - (text == "XY" ? 22 : 16), b.getY(),
                                                (text == "XY" ? 22 : 16), 13);

        g.setColour (colour::accent);
        g.fillRect (chip);
        g.setColour (colour::accentInk);
        g.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
        g.drawText (text, chip, juce::Justification::centred);
    }
}

void HyperPrismLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                              float sliderPosProportional, float rotaryStartAngle,
                                              float rotaryEndAngle, juce::Slider& slider)
{
    if (! slider.getProperties().contains ("zqsfxStrip"))
        slider.getProperties().set ("zqsfxStrip", "xl");

    zqsfx::ui::LookAndFeel::drawRotarySlider (g, x, y, width, height, sliderPosProportional,
                                              rotaryStartAngle, rotaryEndAngle, slider);
}
