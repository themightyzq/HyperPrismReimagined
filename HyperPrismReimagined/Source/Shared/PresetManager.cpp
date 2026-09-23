//==============================================================================
// HyperPrism Reimagined - Preset Manager Implementation
//==============================================================================

#include "PresetManager.h"

namespace hp
{

const juce::Identifier PresetManager::presetNamePropertyID { "presetName" };
const char* const PresetManager::kInitName = "Init";

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& apvtsToUse,
                               juce::String productNameToUse,
                               juce::String effectFolderNameToUse)
    : apvts (apvtsToUse),
      productName (std::move (productNameToUse)),
      effectFolderName (std::move (effectFolderNameToUse))
{
    apvts.state.addListener (this);
    rescan();
}

PresetManager::~PresetManager()
{
    apvts.state.removeListener (this);
}

juce::File PresetManager::userDirectory (const juce::String& effectFolderName)
{
    const auto override_ = juce::SystemStats::getEnvironmentVariable ("HP_PRESET_DIR", {});
    if (override_.isNotEmpty())
        return juce::File (override_);

   #if JUCE_MAC
    return juce::File::getSpecialLocation (juce::File::userHomeDirectory)
               .getChildFile ("Library/Audio/Presets/ZQ SFX/HyperPrism Reimagined")
               .getChildFile (effectFolderName);
   #elif JUCE_WINDOWS
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
               .getChildFile ("ZQ SFX/HyperPrism Reimagined/Presets")
               .getChildFile (effectFolderName);
   #else // Linux and other POSIX
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
               .getChildFile ("ZQ SFX/HyperPrism Reimagined/Presets")
               .getChildFile (effectFolderName);
   #endif
}

void PresetManager::rescan()
{
    entries.clear();
    entries.push_back ({ kInitName, false, -1, {} });

   #if HP_PRESET_HAS_BINARY_DATA
    std::vector<Entry> factory;
    for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
    {
        const juce::String original (BinaryData::originalFilenames[i]);
        if (! original.endsWithIgnoreCase (".hppreset"))
            continue;
        factory.push_back ({ original.dropLastCharacters (9), false, i, {} });
    }
    std::sort (factory.begin(), factory.end(),
               [] (const Entry& a, const Entry& b) { return a.name.compareIgnoreCase (b.name) < 0; });
    for (auto& e : factory)
        entries.push_back (e);
   #endif

    const auto dir = userDirectory (effectFolderName);
    if (dir.isDirectory())
    {
        auto files = dir.findChildFiles (juce::File::findFiles, false, "*.hppreset");
        files.sort();
        for (const auto& f : files)
            entries.push_back ({ f.getFileNameWithoutExtension(), true, -1, f });
    }
}

int PresetManager::getCurrentIndex() const
{
    const auto current = getCurrentName();
    for (size_t i = 0; i < entries.size(); ++i)
        if (entries[i].name == current)
            return (int) i;
    return -1;
}

juce::String PresetManager::getCurrentName() const
{
    return apvts.state.getProperty (presetNamePropertyID, kInitName).toString();
}

void PresetManager::setCurrentName (const juce::String& name)
{
    internalChangeInProgress = true;
    apvts.state.setProperty (presetNamePropertyID, name, nullptr);
    internalChangeInProgress = false;
}

void PresetManager::refreshAndNotify()
{
    rescan();
    if (onCurrentPresetChanged)
        onCurrentPresetChanged();
}

bool PresetManager::load (int index, juce::String& errorOut)
{
    if (index < 0 || index >= (int) entries.size())
    {
        errorOut = "No such preset.";
        return false;
    }

    const auto entry = entries[(size_t) index];

    if (! entry.isUser && entry.binaryIndex < 0)
    {
        // Init: every parameter to its own default. getDefaultValue() is already normalised
        // (0 to 1) for every AudioProcessorParameter, so it goes straight to
        // setValueNotifyingHost() with no further conversion.
        internalChangeInProgress = true;
        for (int i = 0; i < apvts.state.getNumChildren(); ++i)
        {
            auto child = apvts.state.getChild (i);
            if (! child.hasType ("PARAM"))
                continue;
            const auto paramID = child.getProperty ("id").toString();
            if (auto* param = apvts.getParameter (paramID))
                param->setValueNotifyingHost (param->getDefaultValue());
        }
        internalChangeInProgress = false;
        setCurrentName (kInitName);
        refreshAndNotify();
        return true;
    }

    juce::String xmlText;

    if (! entry.isUser)
    {
       #if HP_PRESET_HAS_BINARY_DATA
        int size = 0;
        const char* data = BinaryData::getNamedResource (BinaryData::namedResourceList[entry.binaryIndex], size);
        if (data == nullptr)
        {
            errorOut = "Could not read factory preset \"" + entry.name + "\".";
            return false;
        }
        xmlText = juce::String::createStringFromData (data, size);
       #else
        errorOut = "No factory presets are built into this plugin.";
        return false;
       #endif
    }
    else
    {
        if (! entry.file.existsAsFile())
        {
            errorOut = "Preset file \"" + entry.file.getFullPathName() + "\" no longer exists.";
            return false;
        }
        xmlText = entry.file.loadFileAsString();
    }

    auto xml = juce::XmlDocument::parse (xmlText);
    if (xml == nullptr)
    {
        errorOut = "Preset \"" + entry.name + "\" is not valid XML.";
        return false;
    }

    auto tree = juce::ValueTree::fromXml (*xml);
    if (! tree.isValid())
    {
        errorOut = "Preset \"" + entry.name + "\" could not be parsed.";
        return false;
    }

    internalChangeInProgress = true;
    apvts.replaceState (tree);
    internalChangeInProgress = false;

    setCurrentName (entry.name);
    refreshAndNotify();
    return true;
}

bool PresetManager::step (int delta, juce::String& errorOut)
{
    if (entries.empty())
    {
        errorOut = "No presets available.";
        return false;
    }

    const int n = (int) entries.size();
    const int current = getCurrentIndex();
    const int pivot = current < 0 ? 0 : current;
    const int next = ((pivot + delta) % n + n) % n;
    return load (next, errorOut);
}

bool PresetManager::validateNewUserName (const juce::String& name, int ignoreIndex, juce::String& errorOut) const
{
    const auto trimmed = name.trim();

    if (trimmed.isEmpty())
    {
        errorOut = "Preset name cannot be empty.";
        return false;
    }

    if (trimmed.containsChar ('/') || trimmed.containsChar ('\\'))
    {
        errorOut = "Preset name cannot contain a path separator.";
        return false;
    }

    for (size_t i = 0; i < entries.size(); ++i)
    {
        if ((int) i == ignoreIndex)
            continue;
        if (entries[i].name.equalsIgnoreCase (trimmed))
        {
            errorOut = "A preset named \"" + trimmed + "\" already exists.";
            return false;
        }
    }

    return true;
}

bool PresetManager::writeCurrentStateTo (const juce::File& file, juce::String& errorOut) const
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    if (xml == nullptr)
    {
        errorOut = "Could not serialise the current state.";
        return false;
    }

    if (! xml->writeTo (file))
    {
        errorOut = "Could not write \"" + file.getFullPathName() + "\".";
        return false;
    }

    return true;
}

bool PresetManager::saveUser (const juce::String& name, juce::String& errorOut)
{
    if (! validateNewUserName (name, -1, errorOut))
        return false;

    const auto trimmed = name.trim();
    const auto dir = userDirectory (effectFolderName);

    if (! dir.isDirectory() && ! dir.createDirectory())
    {
        errorOut = "Could not create \"" + dir.getFullPathName() + "\".";
        return false;
    }

    const auto file = dir.getChildFile (trimmed + ".hppreset");
    const auto previousName = getCurrentName();

    // Set the name BEFORE snapshotting the state, so the saved file's own "presetName"
    // attribute matches the name it is being saved under, not whatever was current before.
    setCurrentName (trimmed);
    if (! writeCurrentStateTo (file, errorOut))
    {
        setCurrentName (previousName);
        return false;
    }

    refreshAndNotify();
    return true;
}

bool PresetManager::renameUser (int index, const juce::String& newName, juce::String& errorOut)
{
    if (index < 0 || index >= (int) entries.size() || ! entries[(size_t) index].isUser)
    {
        errorOut = "Only user presets can be renamed.";
        return false;
    }

    if (! validateNewUserName (newName, index, errorOut))
        return false;

    const auto entry = entries[(size_t) index];
    const auto trimmed = newName.trim();
    const auto newFile = entry.file.getSiblingFile (trimmed + ".hppreset");
    const bool wasCurrent = (getCurrentName() == entry.name);

    if (! entry.file.moveFileTo (newFile))
    {
        errorOut = "Could not rename \"" + entry.file.getFullPathName() + "\".";
        return false;
    }

    if (wasCurrent)
        setCurrentName (trimmed);

    refreshAndNotify();
    return true;
}

bool PresetManager::deleteUser (int index, juce::String& errorOut)
{
    if (index < 0 || index >= (int) entries.size() || ! entries[(size_t) index].isUser)
    {
        errorOut = "Only user presets can be deleted.";
        return false;
    }

    const auto entry = entries[(size_t) index];

    if (! entry.file.deleteFile())
    {
        errorOut = "Could not delete \"" + entry.file.getFullPathName() + "\".";
        return false;
    }

    if (getCurrentName() == entry.name)
        setCurrentName (kInitName);

    refreshAndNotify();
    return true;
}

bool PresetManager::overwriteUser (int index, juce::String& errorOut)
{
    if (index < 0 || index >= (int) entries.size() || ! entries[(size_t) index].isUser)
    {
        errorOut = "Only user presets can be overwritten.";
        return false;
    }

    const auto entry = entries[(size_t) index];
    const auto previousName = getCurrentName();

    setCurrentName (entry.name);
    if (! writeCurrentStateTo (entry.file, errorOut))
    {
        setCurrentName (previousName);
        return false;
    }

    refreshAndNotify();
    return true;
}

void PresetManager::valueTreePropertyChanged (juce::ValueTree& tree, const juce::Identifier& property)
{
    if (internalChangeInProgress)
        return;
    if (tree == apvts.state && property == presetNamePropertyID)
        refreshAndNotify();
}

void PresetManager::valueTreeRedirected (juce::ValueTree& tree)
{
    if (internalChangeInProgress)
        return;
    if (tree == apvts.state)
        refreshAndNotify();
}

} // namespace hp
