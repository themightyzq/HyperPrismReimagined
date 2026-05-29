# HyperPrism Reimagined — Development Status

## Completed

- [x] Initial implementation of all 32 plugins (Processor + Editor + Plugin)
- [x] DSP audit: ScopedNoDenormals, state save/restore, stereo bus layouts, bypass params
- [x] UI redesign: horizontal rows replaced with vertical column layout
- [x] Color system: 5 semantic categories (dynamics/timing/modulation/frequency/output)
- [x] Column color coherence: all knobs in a column match the header color
- [x] Accessibility: tooltips on all sliders, buttons, and XY pads
- [x] XY pad: fixed 312px right-side allocation, consistent across all plugins
- [x] Output section unification: 3 category patterns (A/B/C) with consistent knob sizes
- [x] Vertical spacing: 10px gaps (vSpace = knobDiam + 27)
- [x] Knob scaling: 1-col 84-96px, 2-col 80px, 3-col 70px
- [x] MultiDelay: tab-based interface (4 taps, one visible at a time)
- [x] Flanger: redistributed from 5+2 to 4+3 columns
- [x] Value formatting: removed double suffixes, fixed Harmonic Exciter step sizes
- [x] Harmonic Exciter: corrected slider ranges (harmonics 1-5, drive/mix 0-100)
- [x] Label consistency: "Output" for all output knobs, "Mix" for all mix knobs, " %" suffix
- [x] Brand text: "HyperPrism Reimagined" on all 32 plugins
- [x] Minimum window height raised to 520px
- [x] Hardcoded colors eliminated (juce::Colours:: replaced with LookAndFeel system)
- [x] Redundant toggle labels removed (Limiter, MSMatrix, SonicDecimator)
- [x] HyperPhaser: std::pow replaced with std::exp2f
- [x] Universal Binary builds (arm64 + x86_64)
- [x] COPY_PLUGIN_AFTER_BUILD enabled for auto-install
- [x] Documentation: UI/UX best practices guide, updated README, CLAUDE.md

## Bug Fixes & Enhancements (user-reported)

- [x] **Frequency Shifter artifacting** — root-caused via offline FFT harness (`HyperPrismReimagined/Tests/freqshifter_harness.cpp`):
  - Dry/wet path was delay-misaligned → comb filtering at mix < 100% (−22.6 dB notch → −3.0 dB after fix). Fixed by delay-compensating the dry path to match the analytic-path group delay (filterOrder/2).
  - Single shared Hilbert FIR / delay line / oscillator phase across stereo channels → cross-channel contamination (−8.4 dB → −240 dB) and per-channel phase drift. Fixed with per-channel `std::array<HilbertTransform,2>` + per-channel dry delay, and a single oscillator advanced once per sample (sample-outer/channel-inner loop).
  - Now reports latency via `setLatencySamples()` so the host can compensate.
  - Opposite-sideband rejection measured at −74.8 dB (already inaudible) → filter order/window left unchanged on purpose.
- [x] **Frequency ranges raised** — Ring Modulator carrier 8 kHz → 20 kHz (APVTS + editor setRange); Frequency Shifter coarse shift ±2000 Hz → ±5000 Hz. (Typing values into knobs already worked; the limit was the parameter range.)
- [ ] **XY-pad custom axis bounds** (deferred) — let users constrain an axis to a sub-range (e.g. 500 Hz–3 kHz). New UI + remapping in updateParametersFromXYPad/updateXYPadFromParameters. Candidate for its own update.
- [ ] **Ring Modulator anti-aliasing** (future) — square/saw carriers are not band-limited; high carrier freqs alias. Consider oversampling or BLEP if cleaner high-freq behavior is wanted.

## Remaining / Future Work

### High Priority
- [ ] Harmonic Exciter: migrate from old `addParameter` to APVTS for consistency
- [ ] NoiseGate: migrate from old `addParameter` to APVTS for consistency
- [ ] pluginval validation pass at strictness level 10 (all 32 plugins)
- [ ] Code signing with Developer ID certificate
- [ ] Notarization for Gatekeeper compatibility

### Testing
- [ ] Multi-sample-rate testing (44.1k, 48k, 96k, 192k)
- [ ] Buffer size testing (64, 128, 256, 512, 1024, 2048)
- [ ] Soundminer v6 compatibility verification
- [ ] Logic Pro VST3 scanning and automation test
- [ ] REAPER full automation roundtrip test
- [ ] Preset save/load across DAW sessions

### Performance
- [ ] CPU profiling pass — identify hot spots
- [ ] Parameter smoothing audit (SmoothedValue where needed)
- [ ] FFT optimization for Vocoder (consider JUCE's dsp::FFT)

### Future Enhancements
- [ ] Limiter: inter-sample peak detection / oversampling
- [ ] Preset system with factory presets per plugin
- [ ] Undo/redo support
- [ ] MIDI learn for parameter control
- [ ] Resizable UI with proportional scaling
