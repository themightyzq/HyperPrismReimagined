//==============================================================================
// HyperPrism Revived - Modern Look and Feel
//==============================================================================

#pragma once

#include <JuceHeader.h>

class HyperPrismLookAndFeel : public juce::LookAndFeel_V4
{
public:
    HyperPrismLookAndFeel();
    ~HyperPrismLookAndFeel() override = default;

    // Color Scheme - Modern Dark Theme with Cyan Accents
    struct Colors
    {
        static const juce::Colour background;          // Deep dark background
        static const juce::Colour surface;             // Slightly lighter panels
        static const juce::Colour surfaceVariant;      // Control backgrounds
        static const juce::Colour primary;             // Main cyan accent
        static const juce::Colour primaryVariant;      // Darker cyan
        static const juce::Colour secondary;           // Purple accent
        static const juce::Colour onSurface;           // Main text
        static const juce::Colour onSurfaceVariant;    // Secondary text
        static const juce::Colour outline;             // Borders
        static const juce::Colour outlineVariant;      // Subtle borders
        static const juce::Colour error;               // Error/warning
        static const juce::Colour warning;             // Warning states
        static const juce::Colour success;             // Success states
    };

    // Custom Component Drawing
    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                         juce::Slider& slider) override;

    void drawLinearSlider(juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float minSliderPos, float maxSliderPos,
                         juce::Slider::SliderStyle style, juce::Slider& slider) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                         bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
                         
    void drawTickBox(juce::Graphics& g, juce::Component& component,
                    float x, float y, float w, float h,
                    bool ticked, bool isEnabled,
                    bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                             const juce::Colour& backgroundColour,
                             bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& button,
                       bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    juce::Label* createSliderTextBox(juce::Slider& slider) override;

    // Window and Panel Drawing
    void fillTextEditorBackground(juce::Graphics& g, int width, int height,
                                 juce::TextEditor& textEditor) override;

    void drawTextEditorOutline(juce::Graphics& g, int width, int height,
                              juce::TextEditor& textEditor) override;

    // Custom Fonts
    juce::Font getTextButtonFont(juce::TextButton& button, int buttonHeight) override;
    juce::Font getLabelFont(juce::Label& label) override;
    juce::Font getSliderPopupFont(juce::Slider& slider) override;

private:
    void setupFonts();
    
    juce::Font titleFont;
    juce::Font bodyFont;
    juce::Font captionFont;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HyperPrismLookAndFeel)
};