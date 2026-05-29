# CI Build Caching & Consolidation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make GitHub Actions builds fast and non-redundant by adding compiler caching, consolidating the three workflows around one reusable build, and skipping doc-only builds — without changing any plugin code.

**Architecture:** `build.yml` becomes a reusable workflow (`workflow_call` + PR validation) that builds all 32 plugins on macOS/Windows/Linux with `ccache`/`sccache` caching (Windows switches to the Ninja generator so the compiler-launcher works). `latest.yml` and `release.yml` become thin publishers that *call* `build.yml` and then publish (rolling `latest` release / versioned release respectively). Doc-only commits are excluded via `paths-ignore`.

**Tech Stack:** GitHub Actions, CMake, JUCE, `hendrikmuhs/ccache-action`, `ilammy/msvc-dev-cmd`, `lukka/get-cmake`, `softprops/action-gh-release`.

**Reference spec:** `docs/superpowers/specs/2026-05-29-ci-build-caching-consolidation-design.md`

---

## File Structure

- **`.github/workflows/build.yml`** — reusable build. Triggers: `workflow_call` (for publishers) + `pull_request` on `main` (validation). Matrix of 3 OSes; each sets up its compiler cache, configures, builds all targets, verifies, packages a per-platform zip, uploads it as a run artifact. Publishes nothing.
- **`.github/workflows/latest.yml`** — `push: main` (+ `workflow_dispatch`). Calls `build.yml`, then publishes/updates the rolling `latest` release.
- **`.github/workflows/release.yml`** — `push: tags v*`. Calls `build.yml`, then publishes a versioned release.

All three are editing existing files (latest.yml/release.yml/build.yml all exist on this branch).

**Verification reality:** CI cannot be run locally. Per-task verification = YAML parses + structural assertions. The real integration test is Task 5 (open a PR, watch the build go green and show cache hits).

---

### Task 1: Convert `build.yml` into the reusable, cached build

**Files:**
- Modify (full rewrite): `.github/workflows/build.yml`

- [ ] **Step 1: Replace the entire file with the reusable, cached build**

Overwrite `.github/workflows/build.yml` with exactly:

```yaml
name: Build

# Reusable build: invoked by latest.yml / release.yml via workflow_call, and
# runs directly on PRs to validate. Builds all 32 plugins on every platform
# with compiler caching. Publishes nothing.
on:
  workflow_call:
  pull_request:
    branches: [main]
    paths-ignore:
      - '**.md'
      - 'docs/**'
      - 'LICENSE'
      - '.gitignore'

concurrency:
  group: build-${{ github.workflow }}-${{ github.ref }}
  cancel-in-progress: true

jobs:
  build:
    name: ${{ matrix.name }}
    runs-on: ${{ matrix.os }}
    strategy:
      fail-fast: false
      matrix:
        include:
          - os: macos-latest
            name: macOS
            variant: ccache
          - os: ubuntu-latest
            name: Linux
            variant: ccache
          - os: windows-latest
            name: Windows
            variant: sccache
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive

      - name: Install Linux dependencies
        if: matrix.os == 'ubuntu-latest'
        run: |
          sudo apt-get update
          sudo apt-get install -y \
            build-essential libasound2-dev libjack-jackd2-dev \
            libfreetype6-dev libfontconfig1-dev libcurl4-openssl-dev \
            libx11-dev libxext-dev libxrandr-dev libxinerama-dev \
            libxcursor-dev libgl1-mesa-dev libglu1-mesa-dev pkg-config

      - name: Set up MSVC environment (Windows)
        if: matrix.os == 'windows-latest'
        uses: ilammy/msvc-dev-cmd@v1

      - name: Set up CMake + Ninja (Windows)
        if: matrix.os == 'windows-latest'
        uses: lukka/get-cmake@latest

      - name: Set up compiler cache
        uses: hendrikmuhs/ccache-action@v1.2
        with:
          key: ${{ matrix.name }}
          variant: ${{ matrix.variant }}
          max-size: 1500M

      - name: Configure (macOS / Linux)
        if: matrix.os != 'windows-latest'
        run: |
          cd HyperPrismReimagined
          cmake -B build -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_C_COMPILER_LAUNCHER=ccache \
            -DCMAKE_CXX_COMPILER_LAUNCHER=ccache

      - name: Configure (Windows)
        if: matrix.os == 'windows-latest'
        run: |
          cd HyperPrismReimagined
          cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER_LAUNCHER=sccache -DCMAKE_CXX_COMPILER_LAUNCHER=sccache

      - name: Build
        run: |
          cd HyperPrismReimagined
          cmake --build build --parallel

      - name: Verify Universal Binary (macOS)
        if: matrix.os == 'macos-latest'
        run: |
          for vst in HyperPrismReimagined/build/*_artefacts/Release/VST3/*.vst3; do
            [ -d "$vst" ] || continue
            name=$(basename "$vst" .vst3)
            bin="$vst/Contents/MacOS/$name"
            [ -f "$bin" ] && echo "  $name: $(lipo -info "$bin" 2>/dev/null | grep -o 'arm64\|x86_64' | tr '\n' ' ')"
          done

      - name: Package
        shell: bash
        run: |
          cd HyperPrismReimagined
          mkdir -p dist
          for vst in build/*_artefacts/Release/VST3/*.vst3; do
            [ -d "$vst" ] && cp -R "$vst" dist/
          done
          cd dist
          if command -v zip >/dev/null 2>&1; then
            zip -r "../HyperPrism-Reimagined-${{ matrix.name }}.zip" *.vst3
          else
            7z a -tzip "../HyperPrism-Reimagined-${{ matrix.name }}.zip" *.vst3
          fi

      - name: Upload artifact
        uses: actions/upload-artifact@v4
        with:
          name: dist-${{ matrix.name }}
          path: HyperPrismReimagined/HyperPrism-Reimagined-${{ matrix.name }}.zip
          if-no-files-found: error
```

- [ ] **Step 2: Verify the YAML parses**

Run:
```bash
ruby -ryaml -e "YAML.load_file('.github/workflows/build.yml'); puts 'build.yml OK'"
```
Expected: `build.yml OK`

- [ ] **Step 3: Verify structure (triggers, caching, Windows Ninja, no publish)**

Run:
```bash
grep -qE '^\s*workflow_call:' .github/workflows/build.yml && echo "has workflow_call" || echo "MISSING workflow_call"
grep -q 'ccache-action' .github/workflows/build.yml && echo "has cache" || echo "MISSING cache"
grep -q '\-G Ninja' .github/workflows/build.yml && echo "windows ninja" || echo "MISSING ninja"
grep -q 'softprops/action-gh-release' .github/workflows/build.yml && echo "ERROR: build.yml should NOT publish" || echo "no publish (correct)"
```
Expected: `has workflow_call`, `has cache`, `windows ninja`, `no publish (correct)`

- [ ] **Step 4: Commit**

```bash
git add .github/workflows/build.yml
git commit -m "ci: make build.yml a reusable cached build (ccache/sccache, Ninja on Windows)"
```

---

### Task 2: Refactor `latest.yml` to call the reusable build

**Files:**
- Modify (full rewrite): `.github/workflows/latest.yml`

- [ ] **Step 1: Replace the entire file**

Overwrite `.github/workflows/latest.yml` with exactly:

```yaml
name: Latest Build

# On push to main (or manual dispatch): build all platforms via the reusable
# build, then update the rolling "latest" Release. Versioned releases are
# handled by release.yml.
on:
  push:
    branches: [main]
    paths-ignore:
      - '**.md'
      - 'docs/**'
      - 'LICENSE'
      - '.gitignore'
  workflow_dispatch:

concurrency:
  group: latest-build
  cancel-in-progress: true

jobs:
  build:
    uses: ./.github/workflows/build.yml

  publish:
    name: Publish Latest
    needs: build
    runs-on: ubuntu-latest
    permissions:
      contents: write
    steps:
      - uses: actions/checkout@v4

      - name: Download all artifacts
        uses: actions/download-artifact@v4
        with:
          path: artifacts

      - name: Update rolling "latest" Release
        uses: softprops/action-gh-release@v2
        with:
          tag_name: latest
          name: HyperPrism Reimagined — Latest Build
          make_latest: true
          prerelease: false
          body: |
            # HyperPrism Reimagined — Latest Build

            Auto-published from the latest `main` commit
            (`${{ github.sha }}`). 32 professional VST3 audio effect plugins.

            > This rolling build always tracks the newest stable code on `main`.
            > For pinned, versioned downloads see the tagged releases.

            ## Downloads
            | Platform | Architecture | Notes |
            |----------|-------------|-------|
            | **macOS** | Universal Binary (arm64 + x86_64) | Apple Silicon + Intel |
            | **Windows** | x64 | Windows 10/11 |
            | **Linux** | x86_64 | Ubuntu 20.04+, Fedora, Arch |

            ## Installation
            1. Download and extract the zip for your platform
            2. Copy `.vst3` files to your VST3 plugin folder:
               - **macOS**: `~/Library/Audio/Plug-Ins/VST3/`
               - **Windows**: `C:\Program Files\Common Files\VST3\`
               - **Linux**: `~/.vst3/`
            3. Rescan plugins in your DAW

            **macOS Gatekeeper**: these builds are not notarized — if macOS blocks
            them, run `xattr -cr *.vst3` on the extracted plugins before installing.
          files: artifacts/**/*.zip
```

- [ ] **Step 2: Verify the YAML parses**

Run:
```bash
ruby -ryaml -e "YAML.load_file('.github/workflows/latest.yml'); puts 'latest.yml OK'"
```
Expected: `latest.yml OK`

- [ ] **Step 3: Verify it calls build.yml and no longer builds inline**

Run:
```bash
grep -q 'uses: ./.github/workflows/build.yml' .github/workflows/latest.yml && echo "calls reusable build" || echo "MISSING uses"
grep -q 'cmake --build' .github/workflows/latest.yml && echo "ERROR: still builds inline" || echo "no inline build (correct)"
grep -q 'make_latest: true' .github/workflows/latest.yml && echo "promotes latest" || echo "MISSING make_latest"
```
Expected: `calls reusable build`, `no inline build (correct)`, `promotes latest`

- [ ] **Step 4: Commit**

```bash
git add .github/workflows/latest.yml
git commit -m "ci: latest.yml calls reusable build, then publishes rolling latest"
```

---

### Task 3: Refactor `release.yml` to call the reusable build

**Files:**
- Modify (full rewrite): `.github/workflows/release.yml`

- [ ] **Step 1: Replace the entire file**

Overwrite `.github/workflows/release.yml` with exactly:

```yaml
name: Create Release

# On a version tag (v1.2.3): build all platforms via the reusable build, then
# publish a versioned GitHub Release.
on:
  push:
    tags:
      - 'v*.*.*'

jobs:
  build:
    uses: ./.github/workflows/build.yml

  publish:
    name: Publish Release
    needs: build
    runs-on: ubuntu-latest
    permissions:
      contents: write
    steps:
      - uses: actions/checkout@v4

      - name: Download all artifacts
        uses: actions/download-artifact@v4
        with:
          path: artifacts

      - name: Create Release
        uses: softprops/action-gh-release@v2
        with:
          tag_name: ${{ github.ref_name }}
          name: HyperPrism Reimagined ${{ github.ref_name }}
          body: |
            # HyperPrism Reimagined ${{ github.ref_name }}

            32 professional VST3 audio effect plugins.

            ## Downloads
            | Platform | Architecture | Notes |
            |----------|-------------|-------|
            | **macOS** | Universal Binary (arm64 + x86_64) | Apple Silicon + Intel |
            | **Windows** | x64 | Windows 10/11 |
            | **Linux** | x86_64 | Ubuntu 20.04+, Fedora, Arch |

            ## Installation
            1. Download and extract the zip for your platform
            2. Copy `.vst3` files to your VST3 plugin folder:
               - **macOS**: `~/Library/Audio/Plug-Ins/VST3/`
               - **Windows**: `C:\Program Files\Common Files\VST3\`
               - **Linux**: `~/.vst3/`
            3. Rescan plugins in your DAW

            **macOS Gatekeeper**: these builds are not notarized — if macOS blocks
            them, run `xattr -cr *.vst3` on the extracted plugins before installing.

            See [CHANGELOG.md](https://github.com/${{ github.repository }}/blob/main/CHANGELOG.md) for details.
          files: artifacts/**/*.zip
```

- [ ] **Step 2: Verify the YAML parses**

Run:
```bash
ruby -ryaml -e "YAML.load_file('.github/workflows/release.yml'); puts 'release.yml OK'"
```
Expected: `release.yml OK`

- [ ] **Step 3: Verify it calls build.yml, tag-triggered, no inline build**

Run:
```bash
grep -q 'uses: ./.github/workflows/build.yml' .github/workflows/release.yml && echo "calls reusable build" || echo "MISSING uses"
grep -q 'cmake --build' .github/workflows/release.yml && echo "ERROR: still builds inline" || echo "no inline build (correct)"
grep -q "tags:" .github/workflows/release.yml && echo "tag-triggered" || echo "MISSING tag trigger"
```
Expected: `calls reusable build`, `no inline build (correct)`, `tag-triggered`

- [ ] **Step 4: Commit**

```bash
git add .github/workflows/release.yml
git commit -m "ci: release.yml calls reusable build, then publishes versioned release"
```

---

### Task 4: Whole-suite YAML lint + redundancy check

**Files:** none (verification only)

- [ ] **Step 1: Parse all three workflows together**

Run:
```bash
for f in .github/workflows/build.yml .github/workflows/latest.yml .github/workflows/release.yml; do
  ruby -ryaml -e "YAML.load_file('$f')" && echo "$f OK" || echo "$f FAILED"
done
```
Expected: three `OK` lines.

- [ ] **Step 2: Confirm only one place builds, and main-push no longer double-builds**

Run:
```bash
echo "inline cmake builds (should be ONLY build.yml):"
grep -l 'cmake --build' .github/workflows/*.yml
echo "push-to-main triggers (should be ONLY latest.yml):"
grep -lE '^\s*branches:\s*\[main\]' .github/workflows/*.yml
```
Expected: first list = only `build.yml`; second list = only `latest.yml` (build.yml uses `pull_request`, not push-main).

- [ ] **Step 3: Commit (if any lint fixes were needed; otherwise skip)**

```bash
git add -A .github/workflows/
git commit -m "ci: workflow lint pass" || echo "nothing to commit"
```

---

### Task 5: Live validation on a PR (the real test)

**Files:** none (push + observe). This is the only true test of CI; do it deliberately.

- [ ] **Step 1: Push the branch**

Run:
```bash
git push -u origin ci/build-caching-consolidation
```
Expected: branch pushed.

- [ ] **Step 2: Open a PR to main (this triggers build.yml's pull_request build on all 3 platforms)**

Run:
```bash
gh pr create --base main --head ci/build-caching-consolidation \
  --title "CI: build caching + workflow consolidation" \
  --body "Reusable cached build; consolidates latest/release; skips doc-only builds. See docs/superpowers/plans/2026-05-29-ci-build-caching-consolidation.md"
```
Expected: PR URL printed.

- [ ] **Step 3: Watch the build run to completion**

Run:
```bash
gh run watch "$(gh run list --branch ci/build-caching-consolidation --workflow Build --limit 1 --json databaseId --jq '.[0].databaseId')" --exit-status
```
Expected: all three matrix legs (macOS/Linux/Windows) succeed. If it exits non-zero, read the failing job log:
```bash
gh run view --log-failed
```

- [ ] **Step 4: Confirm caching engaged (Windows is the risk)**

In the run logs (Actions UI or `gh run view --log`), confirm the `ccache`/`sccache` step reports a non-zero cache and the build step compiled. First run = cold (cache stored). Push a trivial whitespace change to a single source file and re-run; the second build should be markedly faster with cache hits.

**Fallback if the Windows leg fails on the Ninja/sccache path:** edit `build.yml` — for the Windows matrix entry, remove the `-G Ninja` and the two `*_COMPILER_LAUNCHER` flags from the Windows configure step, drop the `ilammy/msvc-dev-cmd` + `lukka/get-cmake` steps, and gate the `ccache-action` step to `if: matrix.os != 'windows-latest'`. This reverts Windows to the uncached Visual Studio generator while keeping macOS + Linux cached. Commit as `ci: fall back to uncached VS generator on Windows`.

- [ ] **Step 5: Do NOT merge yet**

Leave the PR open for the user to review the green build and the speed difference before merging. Merging to `main` is what activates the rolling `latest` publish and the consolidated triggers.

---

## Self-Review

**Spec coverage:**
- ccache caching all platforms → Task 1 (ccache/sccache, Windows Ninja). ✓
- Eliminate duplicate main-push build → Tasks 1–2 (build.yml drops push-main; only latest.yml pushes main) + Task 4 Step 2 asserts it. ✓
- Skip doc-only builds → `paths-ignore` in build.yml (PR) + latest.yml (push). ✓
- One source of truth for build → reusable build.yml called by latest/release. ✓
- Rolling latest promoted as official → Task 2 (`make_latest: true`). ✓
- Versioned releases on tags → Task 3. ✓
- PRs build+verify, no publish → build.yml PR trigger, publishers gated to push events. ✓
- Windows risk + fallback → Task 5 Step 4 fallback. ✓

**Placeholder scan:** No TBD/TODO; all three files given in full; all commands concrete. ✓

**Type/name consistency:** artifact names `dist-macOS` / `dist-Linux` / `dist-Windows` produced in Task 1 and consumed via `artifacts/**/*.zip` in Tasks 2–3; zip names `HyperPrism-Reimagined-<name>.zip` consistent across upload and glob. Reusable path `./.github/workflows/build.yml` identical in Tasks 2 and 3. ✓
