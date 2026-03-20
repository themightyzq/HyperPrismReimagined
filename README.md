# HyperPrism Reimagined

A complete recreation of the classic Arboretum HyperPrism audio effect suite from the 1990s, rebuilt as 32 professional VST3 plugins using the JUCE framework.

## Overview

HyperPrism Reimagined brings legendary effects into the modern era with:

- **32 high-quality audio effect plugins** covering dynamics, modulation, filters, delays, spatial processing, and more
- **VST3 format** — compatible with all major DAWs (Logic Pro 10.7+, REAPER, Ableton, Cubase, etc.)
- **Universal Binary** on macOS (Apple Silicon + Intel)
- **Consistent design system** — vertical column layout, XY pad interaction, semantic color coding
- **Full automation support** with right-click XY pad parameter assignment
- **Accessible** — tooltips on every control, WCAG AA contrast compliance

## Plugin Suite

### Dynamics (4 plugins)
- **Compressor** — Threshold/ratio/knee compressor with makeup gain and parallel mix
- **Limiter** — Brick-wall limiter with ceiling, lookahead, and soft clip mode
- **Noise Gate** — Gate with threshold, attack, hold, release, range, and lookahead
- **Stereo Dynamics** — Independent mid/side compression with separate threshold and ratio

### Modulation (7 plugins)
- **Chorus** — Multi-voice chorus with rate, depth, delay, feedback, and tone controls
- **Flanger** — Classic flanger with rate, depth, feedback, delay, phase, and tone shaping
- **Phaser** — Multi-stage phaser with rate, depth, feedback, and variable stages
- **HyperPhaser** — Extended phaser with base frequency sweep, bandwidth, and feedback
- **Tremolo** — Amplitude modulation with rate, depth, stereo phase, and waveform selection
- **Vibrato** — Pitch modulation with rate, depth, delay, and feedback
- **AutoPan** — Automatic stereo panning with rate, depth, phase, and waveform control

### Filters (4 plugins)
- **High-Pass Filter** — Resonant high-pass with frequency, resonance, and gain
- **Low-Pass Filter** — Resonant low-pass with frequency, resonance, and gain
- **Band-Pass Filter** — Variable-width band-pass with center frequency, bandwidth, and gain
- **Band-Reject Filter** — Notch/band-reject with center frequency, Q, and gain

### Delay & Reverb (5 plugins)
- **Delay** — Stereo delay with feedback, tone shaping, stereo offset, and tempo sync
- **Single Delay** — Simple delay with feedback, stereo spread, and high/low cut filters
- **Echo** — Classic echo with delay time and feedback
- **Multi Delay** — 4-tap delay with per-tap time, level, pan, and feedback (tab-based UI)
- **Reverb** — Algorithmic reverb with room size, damping, pre-delay, width, and tone

### Stereo & Spatial (5 plugins)
- **Pan** — Stereo panner with position, width, balance, and pan law selection
- **Quasi Stereo** — Mono-to-stereo widening with delay, frequency shift, and phase manipulation
- **More Stereo** — Stereo enhancement with width, bass mono, crossover, and ambience
- **M+S Matrix** — Mid/side encoding/decoding with level control, balance, and solo
- **Stereo Dynamics** — (listed under Dynamics)

### Pitch & Frequency (4 plugins)
- **Pitch Changer** — Pitch shifting with semitone/cent control and formant preservation
- **Frequency Shifter** — Linear frequency shifting with fine control
- **Ring Modulator** — Ring modulation with carrier/modulator frequency and waveform selection
- **Vocoder** — Multi-band vocoder with carrier frequency, band count, and envelope control

### Distortion & Enhancement (4 plugins)
- **Tube/Tape Saturation** — Analog-style saturation with drive, warmth, and brightness
- **Harmonic Exciter** — Harmonic generation with drive, frequency, harmonics, and type selection
- **Sonic Decimator** — Bit crusher with bit depth, sample rate reduction, anti-alias, and dither
- **Bass Maximiser** — Low-end enhancement with frequency, boost, harmonics, and tightness

## Architecture

### UI Design System

Every plugin uses a vertical column layout with a fixed-width XY pad on the right:

```
┌──────────────────────┬──────────────────────────────┐
│  COLUMN 1  │ COL 2   │                              │
│  ○ Knob    │ ○ Knob  │        XY PAD (300px)        │
│  ○ Knob    │ ○ Knob  │                              │
│  ○ Knob    │         ├──────────┬───────────────────┤
│            │         │ OUTPUT   │      METER        │
│            │         │ ○ Mix    │                   │
└────────────┴─────────┴──────────┴───────────────────┘
```

- **Semantic color system** — 5 colors (cyan/purple/pink/amber/green) mapped to parameter categories, with all knobs in a column matching the column header color
- **XY Pad** — 2D control surface with right-click parameter assignment (blue X-axis, yellow Y-axis)
- **Unified output section** — consistent knob sizes and meter placement across all plugins
- **10px vertical gaps** between knobs for comfortable visual separation

### Plugin Architecture

Each plugin follows JUCE's standard pattern:
- **Processor** (`*Processor.cpp/h`) — DSP, parameter management via APVTS
- **Editor** (`*Editor.cpp/h`) — GUI with vertical column layout
- **Plugin** (`*Plugin.cpp`) — Factory entry point

### Shared Components
- `HyperPrismLookAndFeel` — Custom dark theme with semantic color system
- `StandardLayout` — Layout constants and helpers
- `XYPadComponent` — Interactive 2D parameter control pad

## Building from Source

### Requirements
- macOS 12+ (Universal Binary: Apple Silicon + Intel)
- CMake 3.22+
- C++17 compiler (Xcode 14+ recommended)
- Git (for JUCE submodule)

### Build Commands

```bash
# Clone with submodules
git clone --recursive https://github.com/themightyzq/HyperPrismReimagined.git
cd HyperPrismReimagined/HyperPrismReimagined

# Clean Universal Binary build
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j$(sysctl -n hw.ncpu)
```

`CMAKE_OSX_ARCHITECTURES` is set in CMakeLists.txt before `project()` per best practices. `COPY_PLUGIN_AFTER_BUILD TRUE` auto-installs plugins to `~/Library/Audio/Plug-Ins/VST3/`.

### Code Signing & Notarization

```bash
codesign --force --deep --options runtime --timestamp \
    --sign "Developer ID Application: Your Name (TEAM_ID)" plugin.vst3
```

See `JUCE_VST3_BEST_PRACTICES.md` for full signing and notarization workflow.

## Testing

- **Primary DAW:** REAPER (excellent VST3 support, detailed plugin info)
- **Validation:** `pluginval --validate plugin.vst3 --strictness-level 10`
- **Architecture:** `file plugin.vst3/Contents/MacOS/PluginName` (verify Universal Binary)
- **Signature:** `codesign -v plugin.vst3`

## Documentation

| Document | Description |
|----------|-------------|
| `CLAUDE.md` | AI operations manual for code changes |
| `JUCE_VST3_BEST_PRACTICES.md` | Build system, host compatibility, signing |
| `JUCE_VST3_UI_UX_BEST_PRACTICES.md` | UI design system, layout rules, color system |
| `CHANGELOG.md` | Version history |

## License

Copyright 2025-2026 ZQ SFX

## Credits

Original HyperPrism concept by Arboretum Systems (1990s).
Modern recreation by ZQ SFX using the JUCE framework.
