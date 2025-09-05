# HyperPrism Reimagined - Beta Test Distribution

Thank you for testing HyperPrism Reimagined! This document contains installation instructions and important information for beta testers.

## Version 1.0.0 Beta

### What's Included
- 32 professional audio effect VST3 plugins
- Unified 650x600 interface across all plugins
- XY Pad control on applicable effects
- Real-time visual feedback and metering

## Installation Instructions

### macOS

#### Option 1: Installer Package (Recommended)
1. Double-click `HyperPrism_Reimagined_v1.0_Mac_Installer.pkg`
2. Follow the installation wizard
3. Plugins will be installed to `/Library/Audio/Plug-Ins/VST3/`
4. Restart your DAW and rescan plugins

#### Option 2: Manual Installation
1. Extract `HyperPrism_Reimagined_v1.0_Mac.zip`
2. Copy all `.vst3` bundles to `/Library/Audio/Plug-Ins/VST3/`
3. If you see a security warning:
   - Right-click the plugin and select "Open"
   - Click "Open" in the dialog
   - This only needs to be done once per plugin

### Windows

#### Option 1: Installer (Recommended)
1. Run `HyperPrism_Reimagined_v1.0_Setup.exe`
2. Follow the installation wizard
3. Choose full or custom installation
4. Plugins will be installed to `C:\Program Files\Common Files\VST3\`
5. Restart your DAW and rescan plugins

#### Option 2: Manual Installation
1. Extract `HyperPrism_Reimagined_v1.0_Windows.zip`
2. Copy all `.vst3` folders to `C:\Program Files\Common Files\VST3\`
3. You may need administrator privileges

## DAW-Specific Notes

### Logic Pro
- Use Audio Units (AU) version if available, or enable VST support
- Rescan: Logic Pro → Preferences → Plug-in Manager → Reset & Rescan

### Ableton Live
- Rescan: Preferences → Plug-ins → Rescan
- Look under "HyperPrism Reimagined" manufacturer

### Cubase/Nuendo
- Rescan: Studio → VST Plug-in Manager → Refresh
- Check blocklist if plugins don't appear

### Studio One
- Rescan: Studio One → Options → Locations → VST Plug-ins → Scan
- Drag from browser to track

### REAPER
- Rescan: Options → Preferences → VST → Re-scan
- Auto-detects VST3 folder

### FL Studio
- Rescan: Options → Manage Plugins → Find Plugins
- Check "Installed" section

## Known Issues (Beta)

1. **All Platforms**
   - First launch may take a few seconds to initialize
   - Some parameter changes may cause brief audio interruption

2. **macOS Specific**
   - Unsigned plugins will show security warning (see installation instructions)
   - M1/M2 Macs: Running under Rosetta if DAW is Intel-only

3. **Windows Specific**
   - May require Visual C++ Redistributables (included in installer)
   - Windows Defender may scan plugins on first use

## Reporting Issues

Please report bugs with the following information:
1. Plugin name and version
2. DAW and version
3. Operating system
4. Steps to reproduce
5. Screenshot if applicable

Email: beta@hyperprism.com
GitHub: https://github.com/hyperprism/reimagined/issues

## Plugin List

### Dynamics & Compression
- Compressor
- Limiter  
- Noise Gate
- Stereo Dynamics

### Filters & EQ
- High Pass
- Low Pass
- Band Pass
- Band Reject

### Delays & Time Effects
- Delay
- Single Delay
- Echo
- Multi Delay

### Modulation
- Chorus
- Flanger
- Phaser
- HyperPhaser
- Tremolo
- Vibrato
- AutoPan

### Pitch & Frequency
- Pitch Changer
- Frequency Shifter
- Ring Modulator
- Vocoder

### Spatial & Stereo
- Pan
- Quasi Stereo
- More Stereo
- MS Matrix
- Reverb

### Distortion & Enhancement
- Tube/Tape Saturation
- Harmonic Exciter
- Sonic Decimator
- Bass Maximizer

## Legal

This is BETA software provided for testing purposes only. By installing, you agree to:
- Not distribute the software without permission
- Report bugs and issues found during testing
- Not use in commercial productions without authorization
- Accept that the software is provided "as is" without warranty

© 2024 HyperPrism Reimagined. All rights reserved.