//==============================================================================
// HyperPrism Reimagined - shared About box
//
// One helper for all 32 editors' zqsfx::ui::LogoMark onClick handler, so the credit text is
// written once instead of 32 times (style guide section 5: the logo is a real control with an
// About-box trigger). ASCII-only per the migration spec.
//==============================================================================

#pragma once

#include <JuceHeader.h>

namespace HyperPrismAbout
{
    // productName should be the plugin's own PRODUCT_NAME, e.g. "HyperPrism Reimagined Compressor".
    inline void show (const juce::String& productName)
    {
        juce::String text;
        text << productName << " " << JucePlugin_VersionString << juce::newLine << juce::newLine
             << "ZQ SFX - https://www.zq-sfx.com - connect@zq-sfx.com" << juce::newLine
             << "Free software under GPL-3.0-or-later. Built with JUCE." << juce::newLine
             << "Fonts: Barlow Condensed, VT323, IBM Plex Mono (SIL OFL)." << juce::newLine
             << "Knobs: CC0 designs from the g200kg KnobGallery.";

        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::InfoIcon,
                                                "About " + productName, text);
    }
}
