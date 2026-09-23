//==============================================================================
// HyperPrism Reimagined - Preset Bar
//
// [<] [name combo] [>] [Save] [...] - the one UI piece every editor adds to get preset
// browsing (see PresetManager.h for the underlying model). Owns its own PresetManager, so an
// editor's integration is four lines: include this header, add a member, addAndMakeVisible it,
// and give it a 22px-tall strip in resized(). Plain juce::ComboBox/TextButton throughout (not
// the APVTS-bound zqsfx::ui::Combo, which cannot hold a preset name): every editor already
// installs HyperPrismLookAndFeel with setLookAndFeel(), which plain juce Components inherit
// automatically, exactly like each editor's existing bypassButton.
//==============================================================================

#pragma once

#include <JuceHeader.h>
#include "PresetManager.h"

namespace hp
{

class PresetBar  : public juce::Component
{
public:
    PresetBar (juce::AudioProcessorValueTreeState& apvts,
               juce::String productName,
               juce::String effectFolderName);
    ~PresetBar() override;

    void resized() override;

    PresetManager& getPresetManager() noexcept { return presetManager; }

private:
    PresetManager presetManager;

    juce::TextButton prevButton  { "<" };
    juce::ComboBox   presetCombo;
    juce::TextButton nextButton  { ">" };
    juce::TextButton saveButton  { "Save" };
    juce::TextButton menuButton  { "..." };

    bool updatingCombo = false;

    void refreshCombo();
    void showSaveDialog();
    void showRenameDialog (int index);
    void showMenu();
    void showError (const juce::String& message);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetBar)
};

} // namespace hp
