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
- [x] **Frequency ranges raised** — Ring Modulator carrier 8 kHz → 20 kHz (APVTS + editor setRange); Frequency Shifter coarse shift ±2000 Hz → ±5000 Hz with a symmetric skew (0.5) so fine resolution sits near 0 Hz (inner ~20% of travel = 0–200 Hz) while still reaching ±5000. (Typing values into knobs already worked; the limit was the parameter range.) Negative shift is genuine downward shifting, not a mirror — kept bipolar.
- [x] **Frequency Shifter oscillator phase wrap** — accumulator now wraps both directions (was positive-only), so negative shifts don't drift the phase unbounded over long sessions.
- [ ] **XY-pad custom axis bounds** (deferred) — let users constrain an axis to a sub-range (e.g. 500 Hz–3 kHz). New UI + remapping in updateParametersFromXYPad/updateXYPadFromParameters. Candidate for its own update.
- [ ] **Ring Modulator anti-aliasing** (future) — square/saw carriers are not band-limited; high carrier freqs alias. Consider oversampling or BLEP if cleaner high-freq behavior is wanted.

## End-User Review Findings

*From a fresh-eyes producer review (2026-05-29; full detail in `REVIEW-USER.md`). Each item: severity + the user-visible impact in the user's voice.*

### Broken (advertised, but doesn't work for the user)
- [ ] **Remove false Logic Pro compatibility claim / ship AU** — *blocker.* "I'm in Logic and nothing shows up." README lists Logic Pro as compatible, but Logic is AU-only and AU was removed. Fix: add a macOS AU target, or drop Logic from the compatibility claim in README + CLAUDE.md. ↳ see also Testing: "Logic Pro VST3 scanning".
- [ ] **Notarize + Developer-ID-sign released binaries** — *blocker.* "macOS won't let me install it and my DAW can't see it." Released VST3s are ad-hoc signed (`spctl` rejects them). Fix: wire `build_and_notarize.sh` into `release.yml` (sign + notarize + staple); document `xattr -cr` in the meantime. ↳ overlaps High Priority "Code signing" / "Notarization".
- [ ] **Add the missing `JUCE_VST3_BEST_PRACTICES.md` (or fix the links)** — *major.* "The doc that's supposed to explain installing/signing 404s." Linked twice in README and in CLAUDE.md but absent. Fix: create the doc or repoint links to the doc that covers it.
- [ ] **Reconcile changelog's notarization claim with reality** — *major.* "It says it's notarized, but the install is blocked." CHANGELOG advertises a shipped notarization pipeline; in reality notarization is **declined** (downloads use the `xattr -cr` workaround). Fix: correct the changelog to state macOS builds are un-notarized + the workaround.

### Missing (reasonably expected, not found)
- [~] **Distribute via GitHub Releases; promote latest stable build to the official Release** — *major.* "Wait — I have to install Xcode and compile 32 plugins?" Decision: GitHub is the sole distribution channel (no standalone installer). ✅ Added `.github/workflows/latest.yml` — every `main` push (or manual dispatch) builds all 3 platforms and updates a rolling `latest` Release promoted as the official build. Remaining: link it from the README with install steps. Notarization declined — macOS zips ship un-notarized with the `xattr -cr *.vst3` Gatekeeper workaround documented in the release notes.
- [ ] **Add plugin screenshots to the README** — *major.* "I can't tell what it looks like." A visually-designed suite shows only an ASCII diagram. Fix: add 3–4 representative screenshots.
- [ ] **Ship Audio Unit (AU) format on macOS** — *major.* "There's no format my Apple-ecosystem apps (Logic, GarageBand, Final Cut) can load." Fix: re-add an AU build target. ↳ ties to the Logic blocker above.
- [ ] **Factory presets per plugin** — *minor–major.* "Every plugin opens flat — no starting points to learn the sound." Fix: ship a small factory preset bank. ↳ overlaps Future Enhancements "Preset system".
- [ ] **Uninstall instructions** — *minor.* "How do I remove 32 plugins it silently copied into my system folder?" Fix: add an uninstall note or uninstaller.
- [ ] **Windows/Linux build & install docs** — *minor.* "Cross-platform is claimed, but only macOS is documented." Fix: add Windows (`Scripts/build_windows.bat`) and Linux sections.
- [ ] **Per-plugin usage / shared-interaction guide** — *minor.* "What does this knob do, and how does the right-click XY-pad assignment work?" Fix: add a short usage page.

### Confusing (works, but causes friction or self-doubt)
- [ ] **Resolve the NonCommercial license question** — *major.* "Am I even allowed to use these on paid work?" CC BY-NC is ambiguous for production tools and conflicts with the bundled JUCE GPL note. Fix: relicense (e.g. GPLv3 to match JUCE) and/or state explicitly that audio produced with the plugins is unrestricted.
- [ ] **Sync displayed version with the release tag** — *polish.* "The UI says v1.0.0 but the changelog says beta." Fix: drive the displayed version from the project version.
- [ ] **Clarify the nested build directory** — *polish.* "`cd HyperPrismReimagined/HyperPrismReimagined` looks like a typo." Fix: call out the nesting or flatten the layout.
- [ ] **Unify "Bass Maximiser" / "Bass Maximizer" spelling** — *polish.* "The same plugin is spelled two ways." Fix: pick one spelling everywhere.
- [ ] **Correct the resizable-window range in CHANGELOG** — *polish.* "Docs say 600x500–900x800; the real limit is 600x520–900x750." Fix: update the changelog.
- [ ] **Substantiate or soften the "Accessible / WCAG AA" claim** — *minor.* "A keyboard/screen-reader user can't operate the knobs." Fix: add keyboard/value entry + AT labels, or soften the wording to "tooltips + high-contrast theme".

*Declined: standalone one-click installer (`.pkg`/`.dmg`) and demo audio/video — distribution is GitHub Releases only.*

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
