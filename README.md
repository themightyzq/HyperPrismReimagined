# HyperPrism Reimagined

HyperPrism Reimagined is a suite of 32 audio effect plugins that recreate the 1990s Arboretum
HyperPrism suite: dynamics, modulation, filters, delay and reverb, stereo and spatial tools,
pitch and frequency effects, and distortion and enhancement. It ships as VST3, AU, and
Standalone on macOS (a universal binary covering Apple Silicon and Intel), and VST3 and
Standalone on Windows and Linux. Every plugin shares the same layout: parameter knobs, an XY
pad you can assign any two parameters to, and a tooltip on every control. Built with JUCE. The
original HyperPrism concept is by Arboretum Systems.

## Install

Download the current release from
https://github.com/themightyzq/HyperPrismReimagined/releases:
`HyperPrism-Reimagined-macOS.zip`, `HyperPrism-Reimagined-Windows.zip`, or
`HyperPrism-Reimagined-Linux.zip`.

These release builds are VST3 only. AU support was added to the source on 2026-09-22 and is
not in a release yet. If you need AU, for example to use HyperPrism in Logic Pro, build from
source (below) until the next release.

The binaries are unsigned. On macOS, right-click the plugin and choose Open the first time,
since Gatekeeper blocks a plain double-click.

## Use

Load any of the 32 plugins as an effect on a track in your DAW. Every plugin has the same
controls: knobs for its parameters, and an XY pad on the right. Right-click the XY pad to
assign any two parameters to its X and Y axes, then drag the pad to control both at once.
Hover any control to see a tooltip describing it.

Logic Pro hosts Audio Unit plugins only. Use the AU build there once one is available (see
Install above). Pro Tools is not supported: it requires AAX, which is not built.

## The 32 plugins

### Dynamics
- Compressor: threshold/ratio/knee compressor with makeup gain and parallel mix
- Limiter: brick-wall limiter with ceiling, lookahead, and soft clip mode
- Noise Gate: gate with threshold, attack, hold, release, range, and lookahead
- Stereo Dynamics: independent mid/side compression with separate threshold and ratio

### Modulation
- Chorus: multi-voice chorus with rate, depth, delay, feedback, and tone controls
- Flanger: classic flanger with rate, depth, feedback, delay, phase, and tone shaping
- Phaser: multi-stage phaser with rate, depth, feedback, and variable stages
- HyperPhaser: extended phaser with base frequency sweep, bandwidth, and feedback
- Tremolo: amplitude modulation with rate, depth, stereo phase, and waveform selection
- Vibrato: pitch modulation with rate, depth, delay, and feedback
- AutoPan: automatic stereo panning with rate, depth, phase, and waveform control

### Filters
- High-Pass Filter: resonant high-pass with frequency, resonance, and gain
- Low-Pass Filter: resonant low-pass with frequency, resonance, and gain
- Band-Pass Filter: variable-width band-pass with center frequency, bandwidth, and gain
- Band-Reject Filter: notch/band-reject with center frequency, Q, and gain

### Delay and Reverb
- Delay: stereo delay with feedback, tone shaping, stereo offset, and tempo sync
- Single Delay: simple delay with feedback, stereo spread, and high/low cut filters
- Echo: classic echo with delay time and feedback
- Multi Delay: 4-tap delay with per-tap time, level, pan, and feedback, tab-based UI
- Reverb: algorithmic reverb with room size, damping, pre-delay, width, and tone

### Stereo and Spatial
- Pan: stereo panner with position, width, balance, and pan law selection
- Quasi Stereo: mono-to-stereo widening with delay, frequency shift, and phase manipulation
- More Stereo: stereo enhancement with width, bass mono, crossover, and ambience
- M+S Matrix: mid/side encoding and decoding with level control, balance, and solo

### Pitch and Frequency
- Pitch Changer: pitch shifting with semitone/cent control and formant preservation
- Frequency Shifter: linear frequency shifting with fine control
- Ring Modulator: ring modulation with carrier/modulator frequency and waveform selection
- Vocoder: multi-band vocoder with carrier frequency, band count, and envelope control

### Distortion and Enhancement
- Tube/Tape Saturation: analog-style saturation with drive, warmth, and brightness
- Harmonic Exciter: harmonic generation with drive, frequency, harmonics, and type selection
- Sonic Decimator: bit crusher with bit depth, sample rate reduction, anti-alias, and dither
- Bass Maximiser: low-end enhancement with frequency, boost, harmonics, and tightness

## Build from source

Requirements: CMake 3.22 or newer, a C++17 compiler (Xcode command-line tools on macOS), and
Git for the JUCE submodule.

```bash
git clone --recursive https://github.com/themightyzq/HyperPrismReimagined.git
cd HyperPrismReimagined/HyperPrismReimagined
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j 8
```

The macOS build produces a universal binary (Apple Silicon and Intel) and installs plugins to
`~/Library/Audio/Plug-Ins/VST3/` and `~/Library/Audio/Plug-Ins/Components/` automatically.

## Licence

GPL-3.0-or-later. Built with JUCE. Releases before 2026-09-21 were offered under CC BY-NC 4.0;
that licence still applies to those copies.

ZQ SFX, https://www.zq-sfx.com, connect@zq-sfx.com.
