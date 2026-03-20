# Changelog

All notable changes to HyperPrism Reimagined will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

---

## [Unreleased]

### Removed
- 12 orphaned VST3 SDK example symlinks from user plugin folder
- Stray `HyperPrism_VST3_Plugins.txt` from Desktop

### Changed
- **VST3-Only Distribution** - Removed Audio Unit (AU) format entirely. Project now builds VST3 exclusively across all platforms.
- **Window Size Standardization** - All 32 plugins now use 700x550 pixel standard window size (previously 650x600)
- **Resizable Windows** - All plugin windows are now resizable (600x500 to 900x800)

### Fixed
- **Audio Buffer Bug (Critical)** - Fixed hardcoded `maximumBlockSize = 512` in FrequencyShifter, SonicDecimator, Vocoder, and MultiDelay processors. These now properly use the `samplesPerBlock` parameter from `prepareToPlay()`, fixing audio artifacts on Linux and DAWs using non-512 buffer sizes.

### Removed
- Audio Unit (AU) plugin format support
- `notarize_au_plugins.sh` script
- AU artifact upload from GitHub Actions workflow
- AU references from all documentation

---

## [1.0.0-beta] - 2025-12-17

### Added
- Initial release of 32 professional audio effect VST3 plugins
- Complete recreation of classic HyperPrism effects from the 1990s
- Universal Binary support on macOS (Apple Silicon + Intel)
- Cross-platform support: macOS, Windows, Linux
- Shared UI component system (HyperPrismLookAndFeel, StandardLayout, XYPadComponent)
- GitHub Actions CI/CD for all platforms
- Code signing and notarization pipeline for macOS

### Plugin Categories

#### Dynamics (4 plugins)
- Compressor
- Limiter
- Noise Gate
- Stereo Dynamics

#### Filters (4 plugins)
- High-Pass Filter
- Low-Pass Filter
- Band-Pass Filter
- Band-Reject Filter

#### Delays (4 plugins)
- Delay
- Single Delay
- Echo
- Multi Delay

#### Modulation (7 plugins)
- Chorus
- Flanger
- Phaser
- HyperPhaser
- Tremolo
- Vibrato
- Auto Pan

#### Pitch/Frequency (4 plugins)
- Pitch Changer
- Frequency Shifter
- Ring Modulator
- Vocoder

#### Spatial (5 plugins)
- Pan
- Quasi Stereo
- More Stereo
- M+S Matrix
- Reverb

#### Distortion/Enhancement (4 plugins)
- Tube/Tape Saturation
- Harmonic Exciter
- Sonic Decimator
- Bass Maximizer
