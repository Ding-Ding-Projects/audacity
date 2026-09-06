# Handoff

Factual status of the Material Audacity rebuild, as of commit `0b7c0f3` on
`master` and `main` (2026-09-06 07:10 UTC).

## Branches

- `master` and `main`: identical, both at `0b7c0f3`. Every verified lane
  commit is carried here by cherry-pick after both codestyle checks and
  `lrelease` pass on a clean worktree. Every push to `master` publishes a
  release (see Releases below).
- `claude/audacity-material-design-3-ui-voxy2f`: the working branch and
  draft PR record (PR #2). It carries the same content as `master` plus one
  preservation commit (`1ae5390`, an unfinished layered appearance editor
  lane that the owner stopped). Do not merge that commit into `master` as
  is; the lane needs to be finished or dropped first.

## Releases

Every push to `master` runs `.github/workflows/material-audacity-release.yml`
and publishes a non-draft release tagged `v4.0.0-m3.<run number>`:

- `v4.0.0-m3.10` (commit `337add6`): first automatic release. Setup.exe,
  RELEASES, full nupkg, AppImage, SHA256SUMS, dim sum photo. The
  `Get-AuthenticodeSignature` step confirmed `NotSigned`. No delta package,
  which is expected for the first release of the series.
- `v4.0.0-m3.11` (commit `7530142`): first release with a delta package
  (`Audacity-4.0.0-m3011-delta.nupkg`, 4.7 MB against the m3.10 full).
- Run 9 (commit `b4303ba`) failed on both builds: `lrelease` rejected
  `share/locale/audacity_yue_HK.ts` because a plural message lacked
  `numerus="yes"`. Fixed in `337add6`. Always run
  `lrelease share/locale/audacity_yue_HK.ts -qm /tmp/x.qm` before pushing a
  translation change.

## Verified

- `master` at `0b7c0f3` builds on GitHub Actions for Windows (MSVC 2022,
  Qt 6.10.1) and Linux (Ubuntu 22.04), and packages through Squirrel.Windows
  and AppImage (runs 10 and 11 green end to end).
- Both codestyle checkers and `lrelease` pass on the `master` tree.
- Local history (`src/chronicle`): 31/31 `chronicle_tests` and 4/4
  `au3wrap` project metadata tests pass. Every project gets a stable id in
  its own `.aup4` database, the complete history bundle is embedded in the
  save file, starring and pinning survive retention, export and render are
  milestones, and the panel has open-as-new-project, timeline, storage and
  compare surfaces.
- The Cantonese catalog now has a `chronicle` context (109 strings); it had
  none before `32de312`.
- The application reaches the home page under Xvfb; captures live under
  `docs/design/captures/`.

## Not verified

- The new history panel surfaces (timeline rail, storage card, compare view,
  Star, Pin and Open as new project buttons) have no screenshot: the panel
  opens as a separate top-level window that the headless session could not
  raise. `AU_OPEN_HISTORY=versions` exists as a capture hook but has not yet
  produced a viewed capture.
- Six in-flight lanes (Material 3 residual controls U1 and U2, menu and
  dropdown search X1, notifications and bulk actions X4, fixes Y, landing
  page Z) were stopped by the owner before committing. Their edits exist
  only as uncommitted files in the development checkout and are not on any
  branch.
- A fresh Windows machine has not run `build.bat` / `build-installer.bat`
  end to end; only the CI path is proven.
- No screen recording exists for any surface.
- Remaining uncaptured surfaces are listed in `README.md`.

## Owner actions pending

These require repository owner action outside what an agent can do through
the GitHub CLI alone:

- Enable GitHub Pages for this repository, with the source set to GitHub
  Actions (or the `docs/site` folder on the default branch, whichever
  workflow this repository uses), so `docs/site/index.html` is reachable at
  `https://ding-ding-projects.github.io/audacity/`.
- Set the repository homepage (Settings, General, Website) to
  `https://ding-ding-projects.github.io/audacity/` and upload the root
  `social-preview.png` under Settings, General, Social preview. An attempt to
  set the homepage through the API on 2026-09-06 was refused with "Repository
  settings writes are not permitted through this proxy", so both are owner
  clicks.
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
