//==============================================================================
// HyperPrism Reimagined - Preset Bar Implementation
//==============================================================================

#include "PresetBar.h"

namespace hp
{

PresetBar::PresetBar (juce::AudioProcessorValueTreeState& apvts,
                       juce::String productName,
                       juce::String effectFolderName)
    : presetManager (apvts, std::move (productName), std::move (effectFolderName))
{
    prevButton.setTooltip ("Previous preset");
    prevButton.setTitle ("Previous preset");
    prevButton.setDescription ("Loads the preset before the current one in the list.");
    prevButton.onClick = [this]
    {
        juce::String err;
        if (! presetManager.step (-1, err))
            showError (err);
    };
    addAndMakeVisible (prevButton);

    nextButton.setTooltip ("Next preset");
    nextButton.setTitle ("Next preset");
    nextButton.setDescription ("Loads the preset after the current one in the list.");
    nextButton.onClick = [this]
    {
        juce::String err;
        if (! presetManager.step (1, err))
            showError (err);
    };
    addAndMakeVisible (nextButton);

    presetCombo.setTooltip ("Choose a preset");
    presetCombo.setTitle ("Preset");
    presetCombo.setDescription ("Lists Init, this plugin's factory presets, then your saved presets.");
    presetCombo.onChange = [this]
    {
        if (updatingCombo)
            return;
        const int index = presetCombo.getSelectedId() - 1;
        juce::String err;
        if (! presetManager.load (index, err))
            showError (err);
    };
    addAndMakeVisible (presetCombo);

    saveButton.setTooltip ("Save the current sound as a new preset");
    saveButton.setTitle ("Save preset");
    saveButton.setDescription ("Opens a dialog to save the current sound as a new user preset.");
    saveButton.onClick = [this] { showSaveDialog(); };
    addAndMakeVisible (saveButton);

    menuButton.setTooltip ("Rename, overwrite, delete, or reveal presets");
    menuButton.setTitle ("Preset options");
    menuButton.setDescription ("Opens a menu to rename, overwrite, or delete the current user "
                                "preset, or to reveal the preset folder in Finder.");
    menuButton.onClick = [this] { showMenu(); };
    addAndMakeVisible (menuButton);

    presetManager.onCurrentPresetChanged = [this] { refreshCombo(); };
    refreshCombo();
}

PresetBar::~PresetBar()
{
    presetManager.onCurrentPresetChanged = nullptr;
}

void PresetBar::resized()
{
    auto bounds = getLocalBounds();

    prevButton.setBounds (bounds.removeFromLeft (22));
    bounds.removeFromLeft (4);

    menuButton.setBounds (bounds.removeFromRight (22));
    bounds.removeFromRight (4);

    saveButton.setBounds (bounds.removeFromRight (54));
    bounds.removeFromRight (4);

    nextButton.setBounds (bounds.removeFromRight (22));
    bounds.removeFromRight (4);

    presetCombo.setBounds (bounds);
}

void PresetBar::refreshCombo()
{
    updatingCombo = true;

    presetCombo.clear (juce::dontSendNotification);
    const auto& entries = presetManager.getEntries();
    for (int i = 0; i < (int) entries.size(); ++i)
        presetCombo.addItem (entries[(size_t) i].name, i + 1);

    const int current = presetManager.getCurrentIndex();
    presetCombo.setSelectedId (current >= 0 ? current + 1 : 0, juce::dontSendNotification);

    updatingCombo = false;
}

void PresetBar::showSaveDialog()
{
    auto* aw = new juce::AlertWindow ("Save Preset",
                                      "Save the current sound as a new preset.",
                                      juce::MessageBoxIconType::NoIcon);
    aw->addTextEditor ("name", {}, "Preset name:");
    aw->addButton ("Save", 1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    aw->enterModalState (true, juce::ModalCallbackFunction::create ([this, aw] (int result)
    {
        if (result == 1)
        {
            const auto name = aw->getTextEditorContents ("name");
            juce::String err;
            if (! presetManager.saveUser (name, err))
                showError (err);
        }
    }), true);
}

void PresetBar::showRenameDialog (int index)
{
    const auto& entries = presetManager.getEntries();
    if (index < 0 || index >= (int) entries.size())
        return;

    auto* aw = new juce::AlertWindow ("Rename Preset",
                                      "Rename \"" + entries[(size_t) index].name + "\".",
                                      juce::MessageBoxIconType::NoIcon);
    aw->addTextEditor ("name", entries[(size_t) index].name, "New name:");
    aw->addButton ("Rename", 1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("Cancel", 0, juce::KeyPress (juce::KeyPress::escapeKey));

    aw->enterModalState (true, juce::ModalCallbackFunction::create ([this, aw, index] (int result)
    {
        if (result == 1)
        {
            const auto name = aw->getTextEditorContents ("name");
            juce::String err;
            if (! presetManager.renameUser (index, name, err))
                showError (err);
        }
    }), true);
}

void PresetBar::showMenu()
{
    const int index = presetManager.getCurrentIndex();
    const auto& entries = presetManager.getEntries();
    const bool isUser = index >= 0 && index < (int) entries.size() && entries[(size_t) index].isUser;

    juce::PopupMenu menu;
    menu.addItem (1, "Rename...", isUser);
    menu.addItem (2, "Overwrite", isUser);
    menu.addItem (3, "Delete", isUser);
    menu.addSeparator();
    menu.addItem (4, "Reveal Preset Folder");

    menu.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (menuButton),
        [this, index] (int result)
        {
            switch (result)
            {
                case 1:
                    showRenameDialog (index);
                    break;

                case 2:
                {
                    juce::String err;
                    if (! presetManager.overwriteUser (index, err))
                        showError (err);
                    break;
                }

                case 3:
                {
                    juce::String err;
                    if (! presetManager.deleteUser (index, err))
                        showError (err);
                    break;
                }

                case 4:
                {
                    auto dir = PresetManager::userDirectory (presetManager.getEffectFolderName());
                    dir.createDirectory();
                    dir.revealToUser();
                    break;
                }

                default:
                    break;
            }
        });
}

void PresetBar::showError (const juce::String& message)
{
    juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
                                            "Preset Error", message);
}

} // namespace hp
