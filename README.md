# HyperPrism Reimagined

A complete recreation of the classic HyperPrism audio effect suite, featuring 32 professional VST3/AU plugins built with the JUCE framework.

## Overview

HyperPrism Reimagined brings the legendary 1990s HyperPrism effects into the modern era with:
- 32 high-quality audio effect plugins
- VST3 and Audio Unit (AU) formats
- Universal Binary support (Apple Silicon + Intel)
- Professional UI with consistent design language
- Full automation support
- Preset management

## Plugin Categories

### Dynamics (4 plugins)
- Compressor
- Limiter  
- Noise Gate
- Stereo Dynamics

### Filters (4 plugins)
- High-Pass Filter
- Low-Pass Filter
- Band-Pass Filter
- Band-Reject Filter

### Delays (4 plugins)
- Delay
- Single Delay
- Echo
- Multi Delay

### Modulation (7 plugins)
- Chorus
- Flanger
- Phaser
- HyperPhaser
- Tremolo
- Vibrato
- Auto Pan

### Pitch/Frequency (4 plugins)
- Pitch Changer
- Frequency Shifter
- Ring Modulator
- Vocoder

### Spatial (5 plugins)
- Pan
- Quasi Stereo
- More Stereo
- M+S Matrix
- Reverb

### Distortion/Enhancement (4 plugins)
- Tube/Tape Saturation
- Harmonic Exciter
- Sonic Decimator
- Bass Maximizer

## Building from Source

### Requirements
- macOS 10.13+ or Windows 10+
- CMake 3.25+
- Xcode 12+ (macOS) or Visual Studio 2019+ (Windows)
- Git (for cloning with submodules)

### Clone the Repository

```bash
# Clone with submodules (recommended)
git clone --recursive https://github.com/themightyzq/HyperPrismReimagined.git
cd HyperPrismReimagined

# OR if you already cloned without --recursive:
git clone https://github.com/themightyzq/HyperPrismReimagined.git
cd HyperPrismReimagined
git submodule update --init --recursive
```

### Build Instructions

#### macOS
```bash
cd HyperPrismReimagined
./Scripts/build_mac_cmake.sh
```

#### Windows
```batch
cd HyperPrismReimagined
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

## Installation

### macOS
- **VST3**: Copy to `/Library/Audio/Plug-Ins/VST3/`
- **AU**: Copy to `/Library/Audio/Plug-Ins/Components/`

#### macOS Security (Gatekeeper)
Since these plugins are not notarized, macOS may show an "unidentified developer" warning. To bypass:

1. **First time opening** (for each plugin):
   - Right-click on the plugin file in `/Library/Audio/Plug-Ins/VST3/`
   - Select "Open" from the context menu
   - Click "Open" in the security dialog
   
2. **Alternative method**:
   - Go to System Preferences → Security & Privacy
   - Click "Open Anyway" after the warning appears
   
3. **For multiple plugins** (Terminal):
   ```bash
   # Remove quarantine attribute from all plugins
   sudo xattr -cr /Library/Audio/Plug-Ins/VST3/*.vst3
   ```

### Windows
- **VST3**: Copy to `C:\Program Files\Common Files\VST3\`

## Documentation

See `HyperPrismReimagined/PLUGIN_UX_UI_AUDIT.md` for detailed technical documentation and future enhancement plans.

## Scripts

- `build_and_notarize.sh` - Complete build and notarization pipeline for macOS
- `notarize_plugins.sh` - Notarize VST3 plugins for distribution
- `notarize_au_plugins.sh` - Notarize AU plugins for distribution

## License

Copyright © 2025 ZQ SFX

## Credits

Original HyperPrism concept by Arboretum Systems (1990s)
Modern recreation by ZQ SFX using JUCE framework
