//==============================================================================
// HyperPrism Reimagined - Preset Manager
//
// One preset engine shared by every plugin's PresetBar (see PresetBar.h). A preset is the
// plugin's whole APVTS state (the same tree getStateInformation() already serialises),
// written as plain XML with the extension ".hppreset". Three tiers, always in this order:
//   1. "Init"   - synthesised, not a file: every parameter reset to its own default.
//   2. Factory  - this plugin's own presets, compiled into its BinaryData library by
//                 CMake's per-plugin juce_add_binary_data() block from Source/<Effect>/Presets/
//                 *.hppreset, so they travel inside the AU/VST3 bundle. A plugin with no
//                 Presets/ folder yet still builds: BinaryData.h is only pulled in when it
//                 exists (see the __has_include guard below).
//   3. User     - the same format, written to userDirectory(effectFolderName) on disk.
//
// Message-thread only: rescan() touches the filesystem and BinaryData lookups, load()/save()
// touch parameters through setValueNotifyingHost() and the APVTS ValueTree. Nothing in this
// file is reachable from processBlock.
//==============================================================================

#pragma once

#include <JuceHeader.h>

#if __has_include(<BinaryData.h>)
#include <BinaryData.h>
#define HP_PRESET_HAS_BINARY_DATA 1
#else
#define HP_PRESET_HAS_BINARY_DATA 0
#endif

namespace hp
{

class PresetManager  : private juce::ValueTree::Listener
{
public:
    PresetManager (juce::AudioProcessorValueTreeState& apvtsToUse,
                   juce::String productNameToUse,
                   juce::String effectFolderNameToUse);
    ~PresetManager() override;

    //==============================================================================
    // ~/Library/Audio/Presets/ZQ SFX/HyperPrism Reimagined/<effectFolderName>/ on macOS, and
    // the platform-equivalent user documents/config location elsewhere. Overridable for tests
    // (and for one-off tooling that seeds a factory preset) by setting the HP_PRESET_DIR
    // environment variable, which -- when present -- IS the returned directory verbatim,
    // regardless of effectFolderName.
    static juce::File userDirectory (const juce::String& effectFolderName);

    struct Entry
    {
        juce::String name;
        bool isUser = false;    // true: a file in userDirectory(); false: Init or factory
        int binaryIndex = -1;   // factory: index into BinaryData::namedResourceList; else -1
        juce::File file;        // user: the file on disk; else an invalid (default) File
    };

    // Rebuilds the entry list: Init, then this plugin's factory presets (sorted by name), then
    // its user presets (sorted by name). Called by the constructor and after every mutation.
    void rescan();

    const std::vector<Entry>& getEntries() const noexcept { return entries; }

    // Index into getEntries() whose name matches getCurrentName(), or -1 if the currently
    // loaded/saved preset name (or a name set externally, e.g. by a host restoring a session
    // whose preset isn't in this list) doesn't match any current entry.
    int getCurrentIndex() const;

    // The "presetName" property stored directly on apvts.state; "Init" if never set.
    juce::String getCurrentName() const;

    // Loads entries[index]. Init resets every parameter to its default; factory/user parse the
    // stored XML and replaceState() with it. Returns false and fills errorOut on failure,
    // leaving the current state untouched.
    bool load (int index, juce::String& errorOut);

    // load (getCurrentIndex() + delta), wrapped; false if there are no entries.
    bool step (int delta, juce::String& errorOut);

    // Writes the current APVTS state as "<name>.hppreset" in userDirectory(). Refuses an empty
    // name, a name containing '/' or '\', and a name that clashes case-insensitively with any
    // existing entry (Init, factory, or user).
    bool saveUser (const juce::String& name, juce::String& errorOut);

    // The following three refuse Init and factory entries (isUser == false).
    bool renameUser (int index, const juce::String& newName, juce::String& errorOut);
    bool deleteUser (int index, juce::String& errorOut);
    bool overwriteUser (int index, juce::String& errorOut); // writes the current sound over it

    // Fired after every successful load/save/rename/delete/overwrite (list and/or current name
    // may have changed), and after an external AudioProcessorValueTreeState::replaceState() --
    // e.g. a host restoring a saved session -- via the ValueTree::Listener below. Message
    // thread only; PresetBar rebuilds its combo from getEntries()/getCurrentIndex() here.
    std::function<void()> onCurrentPresetChanged;

    const juce::String& getProductName() const noexcept { return productName; }
    const juce::String& getEffectFolderName() const noexcept { return effectFolderName; }

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::String productName;
    juce::String effectFolderName;
    std::vector<Entry> entries;

    static const juce::Identifier presetNamePropertyID;
    static const char* const kInitName;

    // Guards against double-firing onCurrentPresetChanged when one of OUR OWN methods below
    // touches apvts.state (via setProperty() or replaceState()), since that synchronously
    // re-enters this listener. Left false, the listener is exactly the external-change path
    // the header comment above describes (host-driven setStateInformation/replaceState).
    bool internalChangeInProgress = false;

    void setCurrentName (const juce::String& name);
    void refreshAndNotify();
    bool validateNewUserName (const juce::String& name, int ignoreIndex, juce::String& errorOut) const;
    bool writeCurrentStateTo (const juce::File& file, juce::String& errorOut) const;

    // juce::ValueTree::Listener
    void valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property) override;
    void valueTreeRedirected (juce::ValueTree& tree) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};

} // namespace hp
