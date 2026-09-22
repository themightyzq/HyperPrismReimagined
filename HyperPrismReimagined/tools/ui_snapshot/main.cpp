// hyperprism_ui_snapshot_<PluginTarget>: render one HyperPrism Reimagined plugin's real editor
// headlessly to a PNG.
//
//   hyperprism_ui_snapshot_<PluginTarget> <out.png> [scale] [width height]
//     (scale defaults to 2.0; width/height default to the editor's own default size, 700x550 --
//      pass 600 520 to check the minimum resize floor from setResizeLimits(600, 520, 900, 750))
//
// The look-and-feel regression gate for the ZQ SFX house-UI migration (see
// ../../../docs/ZQSFX_UI_STYLE_GUIDE.md and docs/ui_migration_report.md). Render before a UI
// change, render after, compare.
//
// HyperPrism has 32 separate plugin targets and no single shared "core" library the way
// LFlOw/DePump do: each `HyperPrismXxx` target produced by juce_add_plugin() IS that plugin's
// own shared-code target (owns its Processor/Editor/Plugin.cpp). So the pamplejuce pattern
// (link the tool against the plugin's own shared-code target instead of recompiling its
// sources a second time) is applied PER PLUGIN in CMakeLists.txt's add_hyperprism_ui_snapshot()
// function, which selects which Processor header/class this one compilation of this one file
// should construct via the HP_SNAPSHOT_* compile definitions below -- so this single source
// file serves every target (three wired up for the pilot migration: Compressor, Chorus,
// BandPass) without being duplicated 32 times. Adding a 4th plugin's snapshot tool needs one
// more add_hyperprism_ui_snapshot(...) call in CMakeLists.txt, no new .cpp.
//
// Determinism: nothing here ever pumps JUCE's message loop (no runDispatchLoop), so a plugin's
// own juce::Timer (e.g. CompressorEditor's GainReductionMeter, started at 30 Hz in its
// constructor) never actually fires before the snapshot is taken immediately after
// construction -- JUCE dispatches timer callbacks through the message queue, not directly from
// its background timer thread. Two successive renders of unchanged code are therefore
// byte-identical (verified in docs/ui_migration_report.md).

#include HP_SNAPSHOT_PROCESSOR_HEADER
#include <iostream>

int main (int argc, char** argv)
{
    if (argc < 2)
    {
        std::cerr << "usage: " << argv[0] << " <out.png> [scale] [width height]\n";
        return 2;
    }

    juce::ScopedJuceInitialiser_GUI gui;
    const juce::File out = juce::File::getCurrentWorkingDirectory().getChildFile (juce::String (argv[1]));
    const float scale = argc > 2 ? juce::String (argv[2]).getFloatValue() : 2.0f;

    // Processor declared before editor: C++ destroys locals in reverse declaration order, so
    // the editor is always torn down before the processor it references (spec requirement).
    HP_SNAPSHOT_PROCESSOR_CLASS processor;
    std::unique_ptr<juce::AudioProcessorEditor> editor (processor.createEditor());
    if (editor == nullptr)
    {
        std::cerr << "createEditor returned null\n";
        return 1;
    }

    if (argc > 4)
    {
        const int w = juce::String (argv[3]).getIntValue();
        const int h = juce::String (argv[4]).getIntValue();
        editor->setSize (w, h); // within setResizeLimits(600, 520, 900, 750) if given a valid size
    }

    const auto image = editor->createComponentSnapshot (editor->getLocalBounds(), true, scale);

    out.getParentDirectory().createDirectory();
    out.deleteFile();
    juce::FileOutputStream stream (out);
    juce::PNGImageFormat png;
    if (! stream.openedOk() || ! png.writeImageToStream (image, stream))
    {
        std::cerr << "could not write " << out.getFullPathName() << "\n";
        return 1;
    }

    std::cout << out.getFullPathName() << "  " << image.getWidth() << "x" << image.getHeight() << "\n";
    return 0;
}
