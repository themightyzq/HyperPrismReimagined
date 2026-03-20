# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

HyperPrism Reimagined is a suite of 32 professional audio effect VST3 plugins built with JUCE framework. The project recreates classic Arboretum HyperPrism audio effects from the 1990s with modern technology.

**Plugin Format:** VST3 only (no Audio Unit). This decision was made to simplify maintenance and distribution. Logic Pro 10.7+ supports VST3 natively.

**Current State:** All 32 plugins are implemented with a unified vertical column UI layout, semantic color system, and consistent output sections. Prompts 1-17 have been applied (UI redesign, color coherence, spacing, formatting, documentation).

## Key Documentation

Read these before making changes:
- `JUCE_VST3_UI_UX_BEST_PRACTICES.md` — **The canonical UI layout reference.** All layout rules, color system, spacing formulas, and output section patterns.
- `JUCE_VST3_BEST_PRACTICES.md` — Build system, host compatibility, signing, notarization.
- `TODO.md` — Current development status and remaining work.

## Build Commands

### macOS Build (Universal Binary)
```bash
cd HyperPrismReimagined

# Clean build (recommended)
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(sysctl -n hw.ncpu)

# Plugins auto-install to ~/Library/Audio/Plug-Ins/VST3/ via COPY_PLUGIN_AFTER_BUILD
```

`CMAKE_OSX_ARCHITECTURES` is set in CMakeLists.txt before `project()` — do NOT pass it on the command line.

### Code signing (macOS)
```bash
codesign --force --deep --sign "AJU6TF97JM" path/to/plugin.vst3
```

## Architecture

### Directory Structure
- `HyperPrismReimagined/` - Main project directory with CMake build system
  - `Source/` - Plugin source code organized by effect type
    - `Shared/` - Common UI components (HyperPrismLookAndFeel, StandardLayout, XYPadComponent)
    - `[EffectName]/` - Each effect has Processor, Editor, and Plugin files
  - `JUCE/` - JUCE framework submodule
  - `Scripts/` - Build and distribution scripts

### Plugin Architecture
Each plugin follows JUCE's standard pattern:
- **Processor** (`*Processor.cpp/h`) — DSP processing, parameter management via APVTS
- **Editor** (`*Editor.cpp/h`) — GUI with vertical column layout
- **Plugin** (`*Plugin.cpp`) — Factory entry point

### UI Layout System (canonical — see JUCE_VST3_UI_UX_BEST_PRACTICES.md)

- **Vertical column layout** — parameters flow in columns on the left, XY pad on the right
- **Fixed right side:** 312px (300px XY pad + 12px gap)
- **Column sizing:** 1-col=200px/84-96px knobs, 2-col=177px/80px, 3-col=115px/70px
- **Vertical spacing:** `vSpace = knobDiam + 27` for 10px gap
- **Output section:** 3 categories (no meter/centered, 1 knob+meter/140px split, 2 knobs+meter/180px split)
- **Color rule:** all knobs in a column match the column header color
- **Window:** 700x550 default, 600x520 min, 900x750 max

### Color System
| Category | Color | Hex |
|----------|-------|-----|
| Dynamics | Cyan | `#00d9ff` |
| Timing | Purple | `#9b7adb` |
| Modulation | Pink | `#ff6bb5` |
| Frequency | Amber | `#ffab00` |
| Output | Green | `#00ff41` |

### Parameter Management
- Most plugins use JUCE's AudioProcessorValueTreeState (APVTS)
- **Exceptions:** HarmonicExciter and NoiseGate use old-style `addParameter` (migration planned)
- Value formatting comes from the processor's valueToText lambda — editors do NOT call `setTextValueSuffix` (except HarmonicExciter which needs it since it lacks APVTS)
- All parameter ranges must have explicit step sizes

### DSP Standards
- `ScopedNoDenormals` in every processBlock
- State save/restore via getStateInformation/setStateInformation
- Stereo bus layout enforced via isBusesLayoutSupported
- No memory allocation in processBlock
- Bypass param checked at top of processBlock

## Testing

### Validate VST3 plugins
```bash
pluginval --validate ~/Library/Audio/Plug-Ins/VST3/plugin.vst3 --strictness-level 10
```

### Test in DAW
1. Build — plugins auto-install to `~/Library/Audio/Plug-Ins/VST3/`
2. Rescan plugins in DAW (REAPER: Options > Preferences > VST > Re-scan)
3. Test audio processing, parameter automation, and preset save/load
4. Verify Universal Binary: `file plugin.vst3/Contents/MacOS/PluginName`

## Common Issues & Solutions

### Plugin not showing in DAW
- Verify architecture: `file plugin.vst3/Contents/MacOS/PluginName` (must show arm64 + x86_64)
- Check code signing: `codesign -dv --verbose=4 plugin.vst3`
- Remove quarantine: `xattr -cr plugin.vst3`
- Check DAW blocklist

### Build failures
- Ensure JUCE submodule is initialized: `git submodule update --init`
- Clean build directory before rebuilding: `rm -rf build`
- Check CMake version (requires 3.22+)

### Double value suffixes ("100.0 %%")
- The processor's valueToText lambda already adds the suffix
- Do NOT call `setTextValueSuffix` in editors that use APVTS attachments

## Plugin List

The suite includes 32 plugins:
- **Dynamics**: Compressor, Limiter, Noise Gate, Stereo Dynamics
- **Filters**: High Pass, Low Pass, Band Pass, Band Reject
- **Delays**: Delay, Single Delay, Echo, Multi Delay
- **Modulation**: Chorus, Flanger, Phaser, HyperPhaser, Tremolo, Vibrato, AutoPan
- **Pitch/Frequency**: Pitch Changer, Frequency Shifter, Ring Modulator, Vocoder
- **Spatial**: Pan, Quasi Stereo, More Stereo, M+S Matrix, Reverb
- **Distortion/Enhancement**: Tube/Tape Saturation, Harmonic Exciter, Sonic Decimator, Bass Maximiser
