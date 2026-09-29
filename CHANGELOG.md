# Changelog

All notable changes to HyperPrism Reimagined will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

---

## [Unreleased]

### Added
- Preset system (`hp::PresetManager` / `hp::PresetBar`, `Source/Shared/`): Init, per-plugin
  factory presets compiled from `Source/<Effect>/Presets/*.hppreset`, and user presets saved to
  `~/Library/Audio/Presets/ZQ SFX/HyperPrism Reimagined/<Effect>/`. Wired into all 32 editors
  (`Source/Compressor/Presets/Init Copy.hppreset` is the format exemplar; sound presets are still
  to be authored). Every editor also remembers its size across close and reopen. Covered by
  three CTest round-trip checks (`ctest --test-dir build`, 9/9 with the state, null and
  oversampling checks).

- **Upgrading from v1.0.0** - v1.0.0 and the old nightly build used manufacturer code `ZQFX`;
  current builds use `ZQSF`, so hosts treat them as different plugins. Each VST3 now declares
  its v1.0.0 class as compatible (`JUCE_VST3_COMPATIBLE_CLASSES`, listed in the bundle's
  `moduleinfo.json`), so VST3 hosts that support plugin compatibility (for example Cubase and
  Nuendo) substitute the new plugin automatically. Other VST3 hosts, and all AU hosts, show the
  old plugin as missing: insert the current plugin in its place and re-apply the settings.
  Plugin codes and parameter IDs are unchanged.

### Removed
- 12 orphaned VST3 SDK example symlinks from user plugin folder
- Stray `HyperPrism_VST3_Plugins.txt` from Desktop

### Changed
- **Audio Unit restored** - macOS builds ship VST3, AU and Standalone for all 32 plugins. An
  earlier unreleased change had removed AU, which made the suite unloadable in Logic Pro (it
  hosts Audio Units only). Windows and Linux builds are VST3 and Standalone.
- **Window Size Standardization** - All 32 plugins now use 700x550 pixel standard window size (previously 650x600)
- **Resizable Windows** - All plugin windows are now resizable (600x500 to 900x800)

### Changed
- HarmonicExciter and NoiseGate parameters moved to an AudioProcessorValueTreeState, the same
  system as the other 30 plugins. Parameter IDs, ranges, defaults and order are unchanged;
  sessions saved by earlier versions restore through a legacy path covered by a new CTest
  check (the suite's first tests).

### Fixed
- **Audio Buffer Bug (Critical)** - Fixed hardcoded `maximumBlockSize = 512` in FrequencyShifter, SonicDecimator, Vocoder, and MultiDelay processors. These now properly use the `samplesPerBlock` parameter from `prepareToPlay()`, fixing audio artifacts on Linux and DAWs using non-512 buffer sizes.
- macOS deployment target pinned to 11.0; earlier builds declared 15.0 and would not load on macOS 13/14.
- **Compiler warnings (107 to 0)** - Resolved every remaining first-party warning left over from
  the earlier mechanical pass: float/double-to-float narrowing made explicit with
  `static_cast<float>`, signed-to-unsigned array-index conversions made explicit with
  `static_cast<size_t>`/`static_cast<juce::uint32>` (NoiseGate and Vocoder had the bulk of
  these), parameter names that shadowed a member renamed in Chorus/Vibrato/Tremolo/Phaser's
  delay-line and filter `prepare()` methods, two genuinely dead methods removed
  (`AutoPanEditor`/`ChorusEditor::assignParameterToXYPad`, superseded by `showParameterMenu`),
  one genuinely-unused local removed (`TubeTapeSaturationEditor`'s `col2`), one genuinely-unused
  function parameter removed (`HyperPhaserProcessor::processPeakNotchDepth`'s `input` -- the
  caller already applies the returned gain itself), and a deprecated two-argument `juce::Font`
  constructor pair replaced with `Font(FontOptions(...))` in `MultiDelayEditor`. Verified with a
  full rebuild: 0 first-party warnings, 0 build errors, `ctest` 9/9, pluginval strictness 10 on
  every plugin touched by more than a cast or rename.
  Two of the warnings turned out to flag real, pre-existing behaviour bugs; left in place with
  an explanatory comment instead of being silently fixed or deleted, since fixing them changes
  audible behaviour and is out of scope here: Limiter's Release parameter is read every block
  but never applied (`processLimiting()`, which correctly turns it into a release coefficient,
  is dead code -- `processBlock()` uses hardcoded `0.01f`/`0.999f` coefficients instead), and
  BassMaximiser's `processBassCompression()` takes the crossover `frequency` but never uses it
  (attack/release/threshold are fixed constants). Also found, and left for the same reason:
  `SonicDecimatorEditor::setupToggleButton()` never makes the Anti-Alias/Dither labels visible
  or positions them, so right-click-to-assign-to-XY-pad silently does nothing for those two
  parameters even though their `onClick` handlers are wired up.
- **MultiDelay GLOBAL/Pan label collision** - The GLOBAL section heading was positioned at a
  hard-coded `slider.getY() - 55` offset that no longer matched the current knob/label spacing,
  overlapping the Tap column's Pan label at both the default (700x550) and minimum (600x520)
  editor size. Re-anchored the header to the Pan label's actual bottom edge (the same pattern
  already used for this editor's OUTPUT header), so it can't drift back into the label if the
  spacing above changes again. Verified before/after with
  `hyperprism_ui_snapshot_HyperPrismMultiDelay` renders at both sizes, pluginval strictness 10,
  and `auval -v aufx Hmdl ZQSF`.
- **Limiter Release parameter had no audible effect** - `processBlock()` used hard-coded
  `0.999f`/`0.01f` release/attack coefficients regardless of the Release knob; the correct
  release-time formula existed only in the unused, now-removed `processLimiting()`. Release now
  drives that same formula (`coeff = exp(-1000 / (release_ms * sampleRate))`), smoothed at block
  rate, no audio-thread allocation. The Release parameter's default changed from 50ms to 20.8ms
  -- the release time the old hardcoded `0.999f` implied at 48kHz -- so a freshly-created
  instance (no saved session) sounds the same as before. A saved session with a non-default
  Release value will now sound different: that value was previously ignored (every session
  always got the same ~20.8ms release regardless of what Release was set to), and will now
  actually be honoured. Verified with a new CTest check, `hp_release_time_Limiter`
  (`tools/release_time_check/main.cpp`): a -20dB step at Release=50ms vs Release=300ms measures
  a 90%-gain-recovery-time ratio of 6.0009 against an expected 6.0 (300/50).
- **BassMaximiser `processBassCompression()` unused `frequency` argument** - investigated and
  found to be a redundant argument, not an audible defect: the Frequency (crossover) parameter
  is already applied earlier in the chain, in `updateFilters()`, which builds the
  low-pass/high-pass filter pair this function's `input` has already passed through. The unused
  argument is removed; no behaviour change. Verified with a new CTest check,
  `hp_frequency_sweep_BassMaximiser` (`tools/frequency_sweep_check/main.cpp`): sweeping Frequency
  from 30Hz to 400Hz with a fixed 40Hz+2000Hz probe signal changes output RMS by 45.2%.
- **SonicDecimator Anti-Alias/Dither right-click-to-assign did nothing** -
  `setupToggleButton()` never called `addAndMakeVisible()` on the parameter label or gave it
  bounds in `resized()`, even though each label's `onClick` handler (`showParameterMenu`) was
  wired up in the constructor -- the label existed but was invisible and unhittable. Now mirrors
  `setupSlider()`'s handling of its own label: visible, positioned below its toggle button, 22px
  tall (house minimum hit target) at both the default (700x550) and minimum (600x520) editor
  size. Verified with a new CTest check, `hp_label_visibility_SonicDecimator`
  (`tools/label_visibility_check/main.cpp`): constructs the real editor, finds both labels,
  asserts visible/non-empty/>=22px bounds at both sizes, and exercises each `onClick` handler
  directly. Before/after renders via `hyperprism_ui_snapshot_HyperPrismSonicDecimator` (new
  target, added for this fix) confirm the labels are now drawn.

- **MultiDelay echoes landed at the wrong times** - the global feedback sum read every other
  tap with `popSample(..., true)`, which advances that tap's read pointer, so each tap's read
  position drifted several samples per sample and echo times smeared, even with Global
  Feedback at 0. Each tap is now read exactly once per sample and the other taps' feedback
  uses those values; gains and topology (including the 0.25 global attenuation) are unchanged.
  A tap at level 0 is silent and adds nothing to global feedback; its line keeps recording
  the dry input, so raising its level echoes recent input rather than stale audio.
  The per-block dry-buffer copy, which reallocated when the block size changed, is gone.
  Verified by a new CTest check, `hp_echo_timing_MultiDelay`.
- **Harmonic Exciter knobs did nothing** - the editor replaced the knobs' parameter callbacks
  with XY-pad-only callbacks, so Drive, Frequency, Harmonics and Mix never reached the
  processor, and the Bypass button was not connected. All controls now use APVTS attachments
  (they follow automation and presets), and the XY pad reads and writes normalised parameter
  values, so its Frequency axis covers 1-20 kHz instead of pinning Frequency at the maximum.
- **Harmonic Exciter band filter was a low-pass** - the filter named `highPassFilter` never had
  its type set and ran as JUCE's default low-pass; it is now a high-pass, so harmonics are
  generated from the band above Frequency as the control describes.
- **Harmonic Exciter comb filtering** - the dry signal was added undelayed to the oversampled
  (delayed) harmonic signal. The oversampler now uses integer latency and the dry path is
  delayed by exactly the reported latency; bypass goes through the same delay, so timing no
  longer jumps when toggling bypass. Host blocks larger than the prepared size are processed
  in chunks instead of overrunning the oversampler's buffers.
- **Noise Gate Bypass button did nothing** - now attached to the `bypass` parameter.
- The HarmonicExciter changes are covered by new checks in `hp_oversampling_HarmonicExciter`
  (dry alignment at mix 0, bypass latency, impulse peak at the reported latency, high-pass
  behaviour, 64 vs 1000 block-size invariance, 4096-sample host block into a 512-sample
  preparation).

### Added
- `HyperPrismReimagined/ThirdParty/signalsmith-stretch/LICENSE.txt` and
  `HyperPrismReimagined/ThirdParty/signalsmith-stretch/signalsmith-linear/LICENSE.txt`: the
  bundled Signalsmith Audio `signalsmith-stretch` and `linear` libraries (used by PitchChanger)
  had no licence file in this repo. Both are MIT-licensed upstream (fetched from
  github.com/Signalsmith-Audio/signalsmith-stretch and github.com/Signalsmith-Audio/linear),
  which is compatible with this project's GPL-3.0-or-later. Credited in README.md.

### Removed
- `notarize_au_plugins.sh` script

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
