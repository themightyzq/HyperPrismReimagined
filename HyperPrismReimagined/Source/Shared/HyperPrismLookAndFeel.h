//==============================================================================
// HyperPrism Reimagined - Look and Feel
//
// HyperPrism now shares the ZQ SFX house look (docs/ZQSFX_UI_STYLE_GUIDE.md, zqsfx_ui v0.2.1).
// HyperPrismLookAndFeel is a THIN SUBCLASS of zqsfx::ui::LookAndFeel: the house LookAndFeel
// supplies rotary knobs (CC0 filmstrips, picked by dial size), combo boxes (LCD dropdowns),
// slider text-box readouts (LCD glass + glow), gradient buttons with accent hover/on, popups,
// and the keyboard-focus ring automatically, once drawRotarySlider / drawComboBox /
// positionComboBoxText / drawLabel / drawButtonBackground / drawButtonText /
// createFocusOutlineForComponent / createSliderTextBox / getLabelFont / getTextButtonFont /
// fillTextEditorBackground / drawTextEditorOutline are left un-overridden (all of those existed
// on the pre-migration HyperPrismLookAndFeel and have been deleted here for exactly this
// reason). This subclass keeps only what the house LookAndFeel has no equivalent for:
//   - drawToggleButton: a handful of plugins (AutoPan, MSMatrix, BassMaximiser, Limiter,
//     SonicDecimator, Delay) use a plain juce::ToggleButton check-box; the house's bound
//     LitToggle/TextToggle components draw themselves, so plain ToggleButton needs its own
//     override, restyled with house tokens (hard-edged box, no rounded corners).
//   - paintXYAssignmentBadges: a static helper (not a LookAndFeel virtual) that every editor
//     calls once from its own paintOverChildren() to redraw the "which knobs feed the XY pad"
//     badge that the pre-migration drawRotarySlider used to draw inline. The house filmstrip
//     drawRotarySlider consults no per-slider property at all, so this feature moved out to a
//     small accent-coloured marker drawn over the knob instead of living inside the arc/cap
//     paint -- see the .cpp for why accent is the right colour for this state.
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include <zqsfx_ui/zqsfx_ui.h>

class HyperPrismLookAndFeel : public zqsfx::ui::LookAndFeel
{
public:
    HyperPrismLookAndFeel();
    ~HyperPrismLookAndFeel() override = default;

    /*  Pins every knob in the suite to the same filmstrip.

        The 32 editors use knob diameters from 68 to 84 px, and once JUCE takes the value
        readout out of the slider's bounds the remaining dial lands either side of the house
        module's 56 px "large strip" threshold: Compressor drew the silver cap, Chorus the
        black pointer knob. Two knob styles across one plugin family reads as two products,
        so this stamps the module's documented per-slider override ("zqsfxStrip") on first
        draw and then defers to the house implementation. A slider that sets the property
        itself keeps its own choice. */
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    // Colour Scheme -- every member below is now a house token (see docs/ui_migration_report.md
    // for the full remap table). Member NAMES are unchanged on purpose: none of the 32 editors
    // need to change which Colors::x they read, only what that name now resolves to.
    struct Colors
    {
        static const juce::Colour background;          // == zqsfx::ui::colour::chassisMid
        static const juce::Colour surface;              // == zqsfx::ui::colour::panelBot
        static const juce::Colour surfaceVariant;       // == zqsfx::ui::colour::panelTop
        static const juce::Colour primary;              // == zqsfx::ui::colour::accent
        static const juce::Colour primaryVariant;       // == zqsfx::ui::colour::accentDim
        static const juce::Colour onSurface;            // == zqsfx::ui::colour::btnText
        static const juce::Colour onSurfaceVariant;     // == zqsfx::ui::colour::silkLabel
        static const juce::Colour outline;              // == zqsfx::ui::colour::ruleTitle
        static const juce::Colour outlineVariant;       // == zqsfx::ui::colour::ruleInner
        static const juce::Colour error;                // == zqsfx::ui::colour::warn
        static const juce::Colour warning;              // == zqsfx::ui::colour::meterHot
        static const juce::Colour success;              // == zqsfx::ui::colour::lcdText

        // Parameter-group colours -> the five colour-blind-safe complementary channels (style
        // guide section 3's fixed mapping for "HyperPrism, Transient Creator": dynamics/sky,
        // frequency/yellow, modulation/purple, output/green, timing/white).
        static const juce::Colour dynamics;             // == zqsfx::ui::comp::sky
        static const juce::Colour frequency;            // == zqsfx::ui::comp::yellow
        static const juce::Colour modulation;            // == zqsfx::ui::comp::purple
        static const juce::Colour output;                // == zqsfx::ui::comp::green
        static const juce::Colour timing;                // == zqsfx::ui::comp::white
    };

    // ---- Toggle buttons: kept because the house LookAndFeel has no plain-juce::ToggleButton
    // override -- restyled with house tokens: hard-edged box (no rounded corners, style guide
    // section 6), `btn` gradient off-state, `accent` fill + `accentInk` tick on-state. No hand
    // drawn focus ring here any more: the house's createFocusOutlineForComponent (inherited,
    // not overridden below) supplies that.
    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    // XY-pad assignment badges: draws a small "X" / "Y" / "XY" accent chip over any direct
    // Slider child of `parent` whose "xyAxisX" / "xyAxisY" component properties are set (see
    // each Editor's updateParameterColors()). Call once from an editor's paintOverChildren()
    // override, e.g. `HyperPrismLookAndFeel::paintXYAssignmentBadges (g, *this);`. Accent is the
    // correct colour here (never a data colour): being wired to the XY pad is a "selected"
    // state, and style guide section 2 reserves accent for exactly "active / lit / focused /
    // selected", nothing else.
    static void paintXYAssignmentBadges (juce::Graphics& g, juce::Component& parent);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HyperPrismLookAndFeel)
};
