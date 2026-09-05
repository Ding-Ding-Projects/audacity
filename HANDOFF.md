# Handoff

Factual status of the Material Audacity rebuild, as of commit `c0e1cb4` on
branch `claude/audacity-material-design-3-ui-voxy2f`.

## Branches

- `claude/audacity-material-design-3-ui-voxy2f`: the active development
  branch. Carries the full Material 3 rebuild plus the phase 2 companion
  feature set described in `CHANGELOG.md` under Unreleased.
- No merge into `master` has happened yet. `master` is the unmodified
  upstream Audacity 4 tree.

## Verified

- The tree at `c0e1cb4` configures and builds on Linux with the module
  targets used during this development pass (`cmake --build build/linux`).
- The application reaches the home page under Xvfb
  (`QT_QPA_PLATFORM=xcb AU_ALLOW_MULTIPLE_PROCESSES=1 xvfb-run`). Captures
  from this route are under `docs/design/captures/phase2/` and the earlier
  lane directories under `docs/design/captures/`.
- The C++ and QML codestyle checks
  (`buildscripts/ci/checkcodestyle/checkcodestyle.cmake` and
  `buildscripts/ci/checkcodestyle/check_qml_codestyle.cmake`) pass on the
  files touched during this development pass.
- `buildscripts/ci/tools/count_lines.py` runs and its table is reproduced
  in `README.md`.

## Not verified

- A Windows build and run of `build.bat` / `build-installer.bat` has not
  been performed on this machine (Linux only in this environment). The
  scripts exist and are documented, but a fresh Windows machine has not
  actually run them end to end during this pass.
- Real captures of several phase 2 surfaces do not exist yet: the toy lock
  wizard and PIN keypad, the built in authenticator's QR pairing screen,
  the Support Tickets desk, the local model manager for Ollama, the
  universal export dialog, the in app documentation browser, and the
  attention support mode toggles. See `README.md`, Screenshots section.
- No screen recording exists for any surface.
- No tagged release has been cut. `docs/site/data/release.json` reflects
  this honestly (`"tag": "unreleased"`, empty `assets`).
- The completeness inventory guard script referenced in `ROADMAP.md` has
  not been written; `docs/inventory/completeness-inventory.md` exists as a
  hand written table only.
- Dim sum surprise, School mode, and the spoken narrator (three canonical
  features from the shared instructions) are not yet implemented. See
  `ROADMAP.md`, wave two.

## Owner actions pending

These require repository owner action outside what an agent can do through
the GitHub CLI alone:

- Enable GitHub Pages for this repository, with the source set to GitHub
  Actions (or the `docs/site` folder on the default branch, whichever
  workflow this repository uses), so `docs/site/index.html` is reachable at
  `https://ding-ding-projects.github.io/audacity/`.
- Enable Issues and Discussions for this repository so the contribution and
  triage workflow described in the shared instructions can run here.
- Consider renaming the repository from `audacity` to `material-audacity`
  to match the product name used throughout the documentation and the
  README title. This has not been done because a rename changes the
  repository's canonical URL and should be a deliberate owner decision.
- Upload `social-preview.png` (repository root, 1280x640) under Settings,
  General, Social preview. This cannot be done through the GitHub REST API
  or the `gh` CLI; it is a manual upload step. The image already exists at
  the repository root and is also copied to `docs/site/social-preview.png`
  for the Open Graph tags in `docs/site/index.html`.

## Known gaps

- See `ROADMAP.md`, "Phase 2, wave two" for the full open items list.
- Windows runtime behaviour (build, packaging, installer, update checker
  against a real feed) is unverified in this environment.
- The capture set is not exhaustive; several newer surfaces have no
  screenshot evidence yet, listed above and in `README.md`.

## How to build and verify locally

Linux:

```bash
./build.sh
```

or, from an already configured build tree:

```bash
cmake --build build/linux -j3
QT_QPA_PLATFORM=xcb AU_ALLOW_MULTIPLE_PROCESSES=1 \
  xvfb-run -a -s "-screen 0 1600x1000x24" ./build/linux/src/app/audacity &
sleep 25
import -window root /tmp/capture.png
```

Windows, fresh machine:

```bat
git clone --recurse-submodules https://github.com/Ding-Ding-Projects/audacity.git
cd audacity
.\build.bat --run
```

Codestyle, changed files only:

```bash
cmake -P ./muse/buildscripts/ci/checkcodestyle/checkcodestyle.cmake <cpp/h files>
cmake -P ./buildscripts/ci/checkcodestyle/check_qml_codestyle.cmake <qml files>
```

Line count:

```bash
python3 buildscripts/ci/tools/count_lines.py
```
