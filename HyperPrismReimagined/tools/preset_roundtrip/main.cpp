// hyperprism_preset_roundtrip_<PluginTarget>: proves hp::PresetManager (Source/Shared/
// PresetManager.h) really round-trips real parameter values through a user preset file on
// disk, and that a preset name saved via saveUser() survives an ordinary
// getStateInformation()/setStateInformation() round trip -- the way a host restores a saved
// session -- without ever calling load() again (PresetManager's ValueTree::Listener on
// apvts.state is what keeps "presetName" attached to the state instead of load() needing to be
// called a second time). Built per plugin by add_hyperprism_preset_roundtrip() in
// CMakeLists.txt, same per-plugin compile-definition pattern as tools/state_check/main.cpp and
// tools/ui_snapshot/main.cpp. Exit 0 on pass; registered with CTest.
//
// HP_PRESET_DIR is set here, before constructing anything, to a fresh temp directory so this
// test never touches (or depends on) a real ~/Library/Audio/Presets folder and never collides
// with another run of itself or another plugin's copy of this same test.
//
// An optional "--seed-init-copy <dir>" argv mode (not used by CTest's no-argument add_test)
// writes a single "Init Copy.hppreset" -- the Init parameter defaults, saved through this same
// hp::PresetManager::saveUser() -- into <dir>, to seed a plugin's Source/<Effect>/Presets/ as
// the factory-preset format exemplar. See CLAUDE.md's Presets section.

#include HP_ROUNDTRIP_PROCESSOR_HEADER
#include "PresetManager.h"
#include <iostream>
#include <cmath>
#include <cstdlib>

namespace
{
int failures = 0;

void expect (const char* what, bool ok)
{
    if (! ok)
    {
        std::cerr << "FAIL " << what << "\n";
        ++failures;
    }
    else
        std::cout << "ok   " << what << "\n";
}

void expectNear (const char* what, float actual, float expected, float tol = 1.0e-3f)
{
    if (std::abs (actual - expected) > tol)
    {
        std::cerr << "FAIL " << what << ": got " << actual << ", expected " << expected << "\n";
        ++failures;
    }
    else
        std::cout << "ok   " << what << " = " << actual << "\n";
}

juce::AudioProcessorValueTreeState& apvtsOf (HP_ROUNDTRIP_PROCESSOR_CLASS& proc)
{
   #if HP_ROUNDTRIP_APVTS_IS_MEMBER
    return proc.apvts;
   #else
    return proc.getValueTreeState();
   #endif
}
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;

    if (argc >= 3 && juce::String (argv[1]) == "--seed-init-copy")
    {
        const juce::File targetDir (argv[2]);
        targetDir.createDirectory();
       #if JUCE_WINDOWS
        _putenv_s ("HP_PRESET_DIR", targetDir.getFullPathName().toRawUTF8());
       #else
        setenv ("HP_PRESET_DIR", targetDir.getFullPathName().toRawUTF8(), 1);
       #endif

        HP_ROUNDTRIP_PROCESSOR_CLASS seedProc;
        seedProc.setPlayConfigDetails (2, 2, 48000.0, 512);
        hp::PresetManager seedManager (apvtsOf (seedProc), "Preset Seed", "PresetSeed");

        juce::String err;
        if (! seedManager.load (0, err)) // entries[0] is always Init
        {
            std::cerr << "FAIL could not load Init: " << err << "\n";
            return 1;
        }
        if (! seedManager.saveUser ("Init Copy", err))
        {
            std::cerr << "FAIL could not save Init Copy: " << err << "\n";
            return 1;
        }
        std::cout << "PASS wrote " << targetDir.getChildFile ("Init Copy.hppreset").getFullPathName() << "\n";
        return 0;
    }

    auto tempDir = juce::File::createTempFile ("hp_preset_roundtrip");
    tempDir.deleteFile();
    tempDir.createDirectory();
   #if JUCE_WINDOWS
    _putenv_s ("HP_PRESET_DIR", tempDir.getFullPathName().toRawUTF8());
   #else
    setenv ("HP_PRESET_DIR", tempDir.getFullPathName().toRawUTF8(), 1);
   #endif

    HP_ROUNDTRIP_PROCESSOR_CLASS proc;
    proc.setPlayConfigDetails (2, 2, 48000.0, 512);
    auto& apvts = apvtsOf (proc);

    auto* paramA = apvts.getParameter (HP_ROUNDTRIP_PARAM_A);
    auto* paramB = apvts.getParameter (HP_ROUNDTRIP_PARAM_B);
    if (paramA == nullptr || paramB == nullptr)
    {
        std::cerr << "FAIL parameter ID " << HP_ROUNDTRIP_PARAM_A << " or "
                   << HP_ROUNDTRIP_PARAM_B << " not found\n";
        return 1;
    }

    // Normalised values on the opposite side of 0.5 from each parameter's own default -- always
    // non-default, without needing to know that parameter's real-world range.
    const float valueA = paramA->getDefaultValue() > 0.5f ? 0.1f : 0.9f;
    const float valueB = paramB->getDefaultValue() > 0.5f ? 0.15f : 0.85f;

    paramA->setValueNotifyingHost (valueA);
    paramB->setValueNotifyingHost (valueB);

    hp::PresetManager manager (apvts, "Preset Round-trip Check", "PresetRoundtripCheck");

    juce::String err;
    expect ("saveUser succeeds", manager.saveUser ("roundtrip test", err));

    const int savedIndex = manager.getCurrentIndex();
    expect ("saved preset is current", savedIndex >= 0 && manager.getCurrentName() == "roundtrip test");
    const auto savedFile = savedIndex >= 0 ? manager.getEntries()[(size_t) savedIndex].file : juce::File();
    expect ("preset file exists on disk", savedFile.existsAsFile());

    // Mutate away from the saved values.
    paramA->setValueNotifyingHost (1.0f - valueA);
    paramB->setValueNotifyingHost (1.0f - valueB);

    expect ("load restores the saved preset", manager.load (savedIndex, err));
    // .hppreset stores each parameter's real-world (denormalised) value as decimal text, same
    // as getStateInformation's XML; converting that text back through a skewed
    // NormalisableRange (e.g. a compression ratio curve) can move the NORMALISED value by more
    // than float epsilon even though the real-world value round-trips exactly. 1% of the full
    // 0-1 range is generous for that and still catches a genuinely wrong or unrestored value.
    expectNear ("paramA restored", paramA->getValue(), valueA, 0.01f);
    expectNear ("paramB restored", paramB->getValue(), valueB, 0.01f);
    expect ("getCurrentName after load", manager.getCurrentName() == "roundtrip test");

    // getStateInformation()/setStateInformation() round trip into a fresh instance -- proves the
    // preset name travels with an ordinary session save/restore, with no call to load().
    juce::MemoryBlock block;
    proc.getStateInformation (block);

    HP_ROUNDTRIP_PROCESSOR_CLASS proc2;
    proc2.setPlayConfigDetails (2, 2, 48000.0, 512);
    proc2.setStateInformation (block.getData(), (int) block.getSize());

    hp::PresetManager manager2 (apvtsOf (proc2), "Preset Round-trip Check", "PresetRoundtripCheck");
    expect ("preset name survives getState/setState without load()",
            manager2.getCurrentName() == "roundtrip test");

    expect ("deleteUser succeeds", manager.deleteUser (savedIndex, err));
    expect ("preset file removed from disk", ! savedFile.existsAsFile());

    tempDir.deleteRecursively();

    std::cout << (failures == 0 ? "PASS" : "FAIL") << " preset round-trip check\n";
    return failures == 0 ? 0 : 1;
}
