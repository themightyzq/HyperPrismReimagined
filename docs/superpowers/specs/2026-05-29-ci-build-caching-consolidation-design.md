# CI Build Caching & Consolidation — Design

**Date:** 2026-05-29
**Status:** Approved (design), pending implementation plan
**Scope:** GitHub Actions workflows only. No plugin/DSP/source changes. The 32 plugins remain separate, independently-loadable VST3s.

## Background & Motivation

The original prompt was whether to merge the 32 plugins into a single "rack/host" instance (Snap Heap style). That was **rejected**: the lightweight, independently-insertable nature of each plugin is a genuine user benefit, and a meta-host would sacrifice it. The real motivation behind the question was **build pain** — "an easier way of compiling them all at once."

Investigation showed the suite **already** compiles all 32 in one step (`cmake --build build` with no `--target`, per platform). The actual friction is **speed/cost**: every CI run does a cold build — recompiling the entire JUCE framework plus all 32 plugins from scratch, on three platforms, with no cache. Additionally, a previously-added `latest.yml` and the existing `build.yml` both run a full 3-platform build on every push to `main`, doubling the work.

## Goals

1. Make CI builds fast and cheap via compiler caching (JUCE should not recompile when unchanged).
2. Eliminate the duplicate full build on `main` pushes.
3. Stop spending build minutes on doc-only commits.
4. Keep one source of truth for "how to build," reused by every trigger.

## Non-Goals (out of scope)

- The rack/host "pedalboard" plugin (rejected).
- macOS notarization (declined; downloads keep the documented `xattr -cr` Gatekeeper workaround).
- README "Download" section and other user-review findings (tracked separately in TODO.md).

## Target Architecture

### Reusable build workflow (single source of truth)

`.github/workflows/build.yml` becomes a **reusable** workflow:

- Triggers: `workflow_call` (invoked by the publishers) **and** `pull_request` on `main` (PR validation), with `paths-ignore` for docs.
- A 3-entry matrix (macOS / Windows / Linux) that: checks out with submodules, sets up the compiler cache, configures, builds **all** targets, verifies, packages a per-platform zip of every `.vst3`, and uploads it as a run artifact.
- It does **not** publish anything and **no longer** triggers independently on `push: main` (that removes the double build).

### Thin publisher workflows

- `.github/workflows/latest.yml` — `on: push: branches:[main]` (+ `workflow_dispatch`), `paths-ignore` for docs. Calls `build.yml`, then a `publish` job (`needs: build`) downloads the artifacts and updates the rolling `latest` release (`tag_name: latest`, `make_latest: true`) with the `xattr` Gatekeeper note in the body.
- `.github/workflows/release.yml` — `on: push: tags: ['v*.*.*']`. Calls `build.yml`, then publishes a **versioned** release (`tag_name: ${{ github.ref_name }}`).

Artifacts uploaded inside the reusable workflow are available to the caller's `publish` job within the same run via `download-artifact`.

### Caching

Use `hendrikmuhs/ccache-action` (manages the compiler cache **and** its `actions/cache` storage across runs, on all three OSes), with CMake routing the compiler through it:

```
-DCMAKE_C_COMPILER_LAUNCHER=ccache -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
```

(On Windows the action's `sccache` variant is used for MSVC.) Cache key includes the OS and the JUCE submodule commit, with `restore-keys` fallback so a near-miss still warms most objects. After the first run, JUCE stays cached and only changed files recompile.

**Windows requires the Ninja generator** for the compiler-launcher trick to work (the Visual Studio generator ignores it). So the Windows matrix entry:
- sets up the MSVC dev environment (`ilammy/msvc-dev-cmd`),
- installs Ninja + a recent CMake (`lukka/get-cmake` or equivalent),
- configures with `-G Ninja -DCMAKE_BUILD_TYPE=Release` (single-config; drop `--config`).

macOS and Linux keep their current default (Unix Makefiles); ccache works there via the launcher with no generator change.

### Triggers / behavior summary

| Event | Build | Publish |
|-------|-------|---------|
| PR → main | yes (validate) | no |
| Push → main | yes (once) | update rolling `latest` |
| Push tag `v*` | yes | versioned release |
| Doc-only commit (`**.md`, `docs/**`, `LICENSE`, `.gitignore`) | skipped | — |
| Manual dispatch | yes | `latest` (via latest.yml dispatch) |

`concurrency` with `cancel-in-progress` on each top-level workflow so rapid pushes cancel superseded runs.

## Risks & Mitigations

- **Windows Ninja + MSVC + sccache** is the least-trodden path and the main unknown. *Mitigation:* validate on a PR/test branch first (the PR trigger builds all platforms), confirm the Windows job goes green and shows cache hits, before merging to `main`. Fallback: cache macOS + Linux only, leave Windows on the VS generator uncached.
- **Cache eviction** (GitHub's ~10 GB/repo limit) → occasional cold build. Acceptable.
- **Reusable-workflow artifact passing** is standard, low risk.

## Success Criteria

1. A doc-only commit triggers **no** build.
2. A code push to `main` runs **one** 3-platform build (not two) and updates the `latest` release.
3. A warm build is materially faster than a cold one (ccache/sccache stats show hits; wall-clock drops from tens of minutes to single digits on typical commits).
4. A `v*` tag push publishes a versioned release; PRs build + verify without publishing.
5. Each platform zip contains all 32 `.vst3`s; macOS binaries verify as Universal (arm64 + x86_64).
