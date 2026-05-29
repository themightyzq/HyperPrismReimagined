# HyperPrism Reimagined — End-User Review

*Reviewed as a working audio engineer/producer evaluating a new effects suite with fresh eyes and high standards. Findings are weighted by user-visible impact, not code quality. Claims were verified against the actual repo, docs, and built/installed plugin binaries — not by trusting the README.*

**What it claims to be:** a free recreation of Arboretum's HyperPrism — 32 professional VST3 audio effects, Universal Binary on macOS, "compatible with all major DAWs."

**Most demanding plausible persona:** a Mac-based producer/sound designer who already owns FabFilter/Valhalla/Soundtoys-tier plugins, works in Logic and/or REAPER, and expects to double-click an installer and be making sound in two minutes.

---

## First contact (5 minutes, mild headache)

- README explains *what it is* clearly in the first sentence. Good.
- But the only path to actually getting the plugins is **"Building from Source"** — clone with submodules, install CMake + Xcode, build 32 targets. That is not a path a plugin *user* will take; it's a developer path. There is a GitHub Releases mechanism (tag-triggered CI), but the README never mentions it, and (see Broken #2) its output doesn't pass macOS Gatekeeper anyway.
- Net: a real user cannot get from "interested" to "hearing it in my DAW" without either a long developer toolchain detour or an undocumented Terminal command.

---

## Broken — told it works, but it doesn't

### 1. "Compatible with Logic Pro" is false — Logic users can't load *any* plugin
- **Symptom:** README lists "Logic Pro 10.7+" as a compatible DAW, but the suite is VST3-only and AU was deliberately removed. Logic Pro has never supported VST3 — it loads Audio Units only. A Logic user installs the suite and sees *nothing* in their plugin list.
- **Where:** `README.md:10`; `CLAUDE.md` repeats "Logic Pro 10.7+ supports VST3 natively"; AU removal confirmed in `CHANGELOG.md` and `find ~/Library/Audio/Plug-Ins/Components` (no AU present).
- **Severity:** blocker (for the single largest Mac DAW audience)
- **Fix:** either ship an AU target for macOS, or remove Logic from the compatibility claim everywhere.

### 2. macOS downloads are blocked by Gatekeeper (ad-hoc signed, not notarized)
- **Symptom:** A downloaded build won't pass macOS security. The DAW silently ignores it or macOS says it "cannot be opened / cannot verify developer." No working plugin.
- **Where:** verified on installed binary — `codesign -dv` shows `Signature=adhoc`, `TeamIdentifier=not set`; `spctl -a` returns **rejected**. `.github/workflows/release.yml` zips and publishes VST3s with **no** signing/notarization step.
- **Severity:** blocker
- **Fix:** run the existing `build_and_notarize.sh` flow inside the release workflow so published artifacts are Developer-ID signed + notarized + stapled; until then, document the `xattr -cr` workaround in the README.

### 3. A core linked document doesn't exist
- **Symptom:** README sends the user to `JUCE_VST3_BEST_PRACTICES.md` for "signing and notarization workflow" and host compatibility — exactly what someone installing needs — and the file isn't there.
- **Where:** `README.md:129` and `README.md:143` (and `CLAUDE.md`) link it; `ls JUCE_VST3_BEST_PRACTICES.md` → missing. Only `JUCE_VST3_UI_UX_BEST_PRACTICES.md` exists.
- **Severity:** major
- **Fix:** add the document or repoint the links to the doc that actually covers signing/compatibility.

### 4. Changelog claims a notarization pipeline that doesn't produce notarized output
- **Symptom:** A careful user reads CHANGELOG "Added: Code signing and notarization pipeline for macOS" and assumes downloads are safe to install. They aren't (see #2), and TODO.md simultaneously lists signing/notarization as *not done*.
- **Where:** `CHANGELOG.md` (1.0.0-beta "Added") vs `TODO.md` High Priority (`[ ] Code signing`, `[ ] Notarization`).
- **Severity:** major (trust)
- **Fix:** reconcile — either finish notarization in CI or stop advertising it as shipped.

---

## Missing — reasonably expected, can't find

### 5. No "Download" path for a non-developer
- **Symptom:** Nowhere does the README say "go here, download, install." Only build-from-source. The Releases page exists but is invisible from the docs.
- **Where:** `README.md` "Building from Source" is the only acquisition section.
- **Severity:** major
- **Fix:** add a Download section linking GitHub Releases with step-by-step install (and the Gatekeeper note).

### 6. Zero screenshots of a visually-designed product
- **Symptom:** A suite whose headline feature is a "consistent design system / XY pad" shows the user no picture of any plugin — only an ASCII box diagram.
- **Where:** `README.md`; no image files in the repo outside `JUCE/`.
- **Severity:** major (discovery/trust — users judge plugins by look)
- **Fix:** add screenshots of 3–4 representative plugins to the README.

### 7. No Audio Unit on macOS
- **Symptom:** Logic, GarageBand, and Final Cut users (the Apple ecosystem) have no format they can load. Every comparable Mac suite (FabFilter, Valhalla, Soundtoys) ships AU + VST3.
- **Where:** AU removed per `CHANGELOG.md`; no `.component` built.
- **Severity:** major
- **Fix:** re-add an AU build target for macOS.

### 8. No factory presets
- **Symptom:** Every plugin opens at a single default. Users expect a few starting points ("Vintage," "Wide," "Subtle") per effect to learn the sound fast.
- **Where:** confirmed absent in `TODO.md` / `CLAUDE.md`.
- **Severity:** minor–major (onboarding)
- **Fix:** ship a small factory preset bank per plugin.

### 9. No uninstall instructions
- **Symptom:** Install silently copies into `~/Library/Audio/Plug-Ins/VST3/`; the user is never told how to remove 32 plugins.
- **Where:** `README.md` / install flow.
- **Severity:** minor
- **Fix:** add an uninstall note (or an uninstaller script in the installer).

### 10. Windows/Linux users have no documented build/install
- **Symptom:** README claims cross-platform and CI builds all three, but Requirements and Build Commands are macOS-only.
- **Where:** `README.md:101-120` (macOS only) vs `CHANGELOG.md` "Cross-platform support: macOS, Windows, Linux."
- **Severity:** minor
- **Fix:** add Windows (`Scripts/build_windows.bat`) and Linux build/install sections.

### 11. No per-plugin user documentation
- **Symptom:** Beyond one-line descriptions, there's no explanation of what each control does or how the right-click XY-pad assignment works — a feature a stranger won't discover.
- **Where:** `README.md` plugin list.
- **Severity:** minor
- **Fix:** a short usage page covering shared interactions (XY assignment, resizing, metering) plus per-plugin parameter notes.

---

## Confusing — works, but causes friction or self-doubt

### 12. NonCommercial license on tools meant for production work
- **Symptom:** Licensed CC BY-NC 4.0. A producer asking "can I use these on a paid track?" gets an ambiguous, probably-no answer. CC also explicitly discourages using its licenses for software, and NC conflicts with the bundled JUCE GPL terms noted in the same file.
- **Where:** `LICENSE`.
- **Severity:** major (legal uncertainty quietly blocks professional adoption)
- **Fix:** relicense under a software-appropriate license (e.g. GPLv3 to match JUCE) and/or explicitly state that audio produced with the plugins is unrestricted.

### 13. Plugin shows "v1.0.0" but the project is a beta
- **Symptom:** The UI footer reads `v1.0.0`; the changelog's latest tag is `1.0.0-beta`. Which is it?
- **Where:** editor `paint()` draws `v1.0.0`; `CHANGELOG.md` shows `[1.0.0-beta]` + `[Unreleased]`.
- **Severity:** polish
- **Fix:** drive the displayed version from the project version and tag releases consistently.

### 14. Nested same-named directory in the build steps
- **Symptom:** `cd HyperPrismReimagined/HyperPrismReimagined` looks like a typo and invites mistakes.
- **Where:** `README.md:112`.
- **Severity:** polish
- **Fix:** note the nesting explicitly or flatten the layout.

### 15. A plugin's name is spelled two ways
- **Symptom:** "Bass Maximiser" (README, CLAUDE) vs "Bass Maximizer" (CHANGELOG). Looks careless.
- **Where:** `README.md:63` vs `CHANGELOG.md`.
- **Severity:** polish
- **Fix:** pick one spelling everywhere.

### 16. Resizable-window range disagrees between docs
- **Symptom:** CHANGELOG says windows resize 600x500–900x800; the actual limit (all 32 editors) is 600x520–900x750.
- **Where:** `CHANGELOG.md` vs `setResizeLimits(600, 520, 900, 750)`.
- **Severity:** polish
- **Fix:** correct the changelog.

### 17. "Accessible / WCAG AA" is broader than what's delivered
- **Symptom:** README promises accessibility and WCAG AA; in practice there are tooltips and a dark theme, but knobs are rotary-drag with no documented keyboard control or screen-reader labeling. A keyboard/AT user can't operate it.
- **Where:** `README.md:14`.
- **Severity:** minor
- **Fix:** implement keyboard/value entry + AT labels, or soften the claim to "tooltips + high-contrast theme."

---

## Recommended (would meaningfully raise quality)
- Notarized, signed public releases wired into CI (resolves #2/#4 and the entire install story).
- Ship AU alongside VST3 on macOS (#7) — without it the Mac story is half-built.
- Screenshots + a short usage doc (#6/#11).
- Resolve the license question for commercial users (#12).

## Nice-to-have (polish)
- Factory presets, an uninstaller, MIDI-learn and undo (already noted as future in TODO).
- ~~One-click installer `.pkg`/`.dmg` and demo audio/video~~ — **declined by maintainer.** Distribution is GitHub Releases only; the latest stable CI build is promoted to the official Release on the repo page.

---

## Top User Frustrations (ordered by likelihood of making someone give up)

1. **"I'm in Logic and nothing shows up."** The README explicitly promises Logic, but VST3-only + no AU means total incompatibility with the biggest Mac DAW. Highest churn.
2. **"macOS won't let me install it / my DAW can't see it."** Un-notarized, ad-hoc-signed downloads are Gatekeeper-blocked, with no user-facing workaround in the docs.
3. **"Wait — I have to install Xcode and compile 32 plugins?"** No real download path is surfaced; the documented route is a developer build.
4. **"Am I even allowed to use these on paid work?"** A NonCommercial license on production tools creates exactly the doubt that stops a pro from adopting.
5. **"What does this even look like / sound like, and where do I start?"** No screenshots, no presets, no usage guide — nothing to lower the first-use cliff.
