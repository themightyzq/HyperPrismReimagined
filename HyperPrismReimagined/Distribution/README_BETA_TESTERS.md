# Installing and testing HyperPrism Reimagined

HyperPrism Reimagined is a suite of 32 audio effect plugins recreating the 1990s Arboretum
HyperPrism suite. This document covers installing the current release and where to send
feedback. The public release is a rolling `latest` build; there is no separate beta build.

## Install

### macOS

1. Download `HyperPrism-Reimagined-macOS.zip` from
   https://github.com/themightyzq/HyperPrismReimagined/releases and unzip it.
2. Copy the `.vst3` bundles to `~/Library/Audio/Plug-Ins/VST3/` (just for you) or
   `/Library/Audio/Plug-Ins/VST3/` (all users).
3. The plugins are unsigned. If macOS blocks them, right-click each plugin and choose Open,
   then click Open again in the dialog. This is only needed once per plugin.
4. Restart your DAW and rescan plugins.

Requires macOS 11.0 or later.

The macOS download includes Audio Unit (.component) versions, installed to
~/Library/Audio/Plug-Ins/Components.

### Windows

1. Download `HyperPrism-Reimagined-Windows.zip` from the releases page above and unzip it.
2. Copy the `.vst3` folders to `C:\Program Files\Common Files\VST3\`. This may need
   administrator privileges.
3. Restart your DAW and rescan plugins.

### Linux

1. Download `HyperPrism-Reimagined-Linux.zip` from the releases page above and unzip it.
2. Copy the `.vst3` bundles to `~/.vst3/`.
3. Restart your DAW and rescan plugins.

## DAW-specific notes

- Logic Pro: does not host VST3. It hosts Audio Units (AU), which the macOS download includes;
  install the .component bundles to ~/Library/Audio/Plug-Ins/Components.
- Ableton Live: Preferences > Plug-ins > Rescan. Look under the ZQ SFX manufacturer, not
  "HyperPrism Reimagined".
- Cubase/Nuendo: Studio > VST Plug-in Manager > Refresh. Check the blocklist if plugins do
  not appear.
- Studio One: Options > Locations > VST Plug-ins > Scan, then drag from the browser to a
  track.
- REAPER: Options > Preferences > VST > Re-scan. Auto-detects the VST3 folder.
- FL Studio: Options > Manage Plugins > Find Plugins, check the Installed section.

## Reporting issues

Please include:
1. Plugin name and version
2. DAW and version
3. Operating system
4. Steps to reproduce
5. Screenshot if applicable

Email: connect@zq-sfx.com
GitHub: https://github.com/themightyzq/HyperPrismReimagined/issues

## Plugin list

- Dynamics & Compression: Compressor, Limiter, Noise Gate, Stereo Dynamics
- Filters & EQ: High Pass, Low Pass, Band Pass, Band Reject
- Delays & Time Effects: Delay, Single Delay, Echo, Multi Delay
- Modulation: Chorus, Flanger, Phaser, HyperPhaser, Tremolo, Vibrato, AutoPan
- Pitch & Frequency: Pitch Changer, Frequency Shifter, Ring Modulator, Vocoder
- Spatial & Stereo: Pan, Quasi Stereo, More Stereo, MS Matrix, Reverb
- Distortion & Enhancement: Tube/Tape Saturation, Harmonic Exciter, Sonic Decimator,
  Bass Maximiser

## Legal

HyperPrism Reimagined is licensed under the GPL-3.0-or-later. Anyone may use, modify, and
redistribute it under that licence, including for commercial purposes. It is provided as is,
without warranty.

Copyright (c) 2025-2026 ZQ SFX.
