# Material Audacity release contract

This document is the authoritative description of how Material Audacity is
built, packaged, verified and published. Anything that contradicts it is a bug.

## Summary

| Item | Decision |
| --- | --- |
| Windows installer | Squirrel.Windows, exclusively |
| Windows installer format | `Setup.exe` plus `RELEASES`, a full `.nupkg` and a delta `.nupkg` |
| Code signing | Permanently prohibited |
| Linux artifact | AppImage, produced by the existing `buildscripts/ci/linux` scripts |
| Releases | Created for tags matching `v4.0.0-m3.*` only |
| Non tag pushes to `master` | Build and upload workflow artifacts, no release |
| Documentation site | `docs/` deployed to GitHub Pages |
| Secrets used | `GITHUB_TOKEN` only |

## Artifacts

A tagged release publishes the following assets.

| Asset | Produced by | Purpose |
| --- | --- | --- |
| `Setup.exe` | `Squirrel.exe --releasify` | The Windows installer. Per user, no administrator rights. |
| `RELEASES` | `Squirrel.exe --releasify` | The update feed index that installed copies read. |
| `Audacity-<version>-full.nupkg` | `nuget pack` then releasify | The complete application payload for this version. |
| `Audacity-<version>-delta.nupkg` | releasify, when a previous release exists | The binary difference against the previous release. |
| `Audacity-<version>-x86_64.AppImage` | `buildscripts/ci/linux/package.cmake` | The Linux build. |
| `SHA256SUMS` | the release job | SHA256 of every other asset. |

The delta package is only produced when the workflow finds a previous
`v4.0.0-m3.*` release to download. The first release of the series therefore
ships a full package only, which is expected and not an error.

## Versions

`version.cmake` holds the product version, currently `4.0.0`. Squirrel.Windows
and NuGet accept SemVer 1 only, so the pre-release label cannot contain a dot or
a hyphen. The tag label is converted:

| Git tag | Package version |
| --- | --- |
| `v4.0.0-m3.1` | `4.0.0-m3001` |
| `v4.0.0-m3.12` | `4.0.0-m3012` |
| no tag, run number 837 | `4.0.0-ci000837` |

The NuGet package id stays `Audacity`, matching `MUSE_APP_NAME` in
`version.cmake`. The package id must never change: Squirrel identifies an
installed application by its id, and a new id would install a second copy
instead of upgrading the existing one. The human readable title is
`Material Audacity`.

The installer icon is `share/icons/AppIcon/AU4_AppIcon.ico`, a multi resolution
icon that is applied both to `Setup.exe` and to the packaged application.

## Why the installer is not code signed

Code signing is permanently prohibited for this project. There is no
certificate, no signing service and no plan to add one. The consequences are
stated plainly rather than hidden:

- Windows SmartScreen shows a warning the first time `Setup.exe` runs. The user
  has to choose "More info" and then "Run anyway".
- The project makes no authenticity claim based on a signature. Authenticity is
  established through the published SHA256 checksums and the public build log.

The prohibition is enforced, not merely documented:

- `buildscripts/ci/windows/package.cmake` has no `SIGN_KEY`, `SIGN_SECRET`,
  `SIGN_ENABLE` or `signtool` path. The former AWS signing helper has been
  deleted.
- `buildscripts/ci/windows/package_squirrel.ps1` deletes the `signtool.exe`
  that ships inside the `squirrel.windows` NuGet package before Squirrel runs,
  and passes no signing argument.
- Both the packaging script and a separate workflow step run
  `Get-AuthenticodeSignature` over every produced executable and fail the build
  if any status is not `NotSigned`.
- `buildscripts/packaging/Windows/SetupWindowsPackaging.cmake` no longer selects
  the WiX generator and carries no signing hook.

## How a user verifies a download

Run both checks in PowerShell in the folder holding the downloaded files.

```powershell
# 1. Checksum. The output must match the matching line in SHA256SUMS.
Get-FileHash .\Setup.exe -Algorithm SHA256

# 2. Signature status. The expected and only acceptable answer is NotSigned.
(Get-AuthenticodeSignature .\Setup.exe).Status
```

`SHA256SUMS` is plain `sha256sum` output, so on a machine with the GNU
coreutils available the whole file can be checked at once with
`sha256sum --check SHA256SUMS`.

## Build and packaging pipeline

`.github/workflows/material-audacity-release.yml` contains three jobs.

1. `windows_x64` on `windows-2022`.
   - The first step records the workflow start time in UTC and exposes it as a
     job output. This is the value that appears in the release notes.
   - Qt 6.10.1 is installed with `jurplel/install-qt-action@v4` using the same
     module list as `au4_build_windows.yml`.
   - `buildscripts/ci/windows/setup.cmake` prepares the environment and
     `buildscripts/ci/windows/ci_build.cmake` performs the real build, so the
     packaged application is the same tree the ordinary CI produces
     (`build.install`).
   - ccache is restored and saved with the existing cache actions, and the MSVC
     problem matcher is registered.
   - The previous release, if any, is downloaded so Squirrel can build a delta.
   - `buildscripts/ci/windows/package.cmake` is invoked with
     `-DPACK_TYPE_OVERRIDE=squirrel`, which runs `package_squirrel.ps1`.
   - A dedicated step re-verifies that nothing is signed.
   - Artifacts are uploaded with `if: always()` and `if-no-files-found: warn`.
2. `linux_x64` on `ubuntu-22.04`, reusing `buildscripts/ci/linux/setup.sh`,
   `ci_build.cmake` and `package.cmake` to produce the AppImage.
3. `release`, which runs only for tags. It downloads both artifact sets,
   collects the release assets, writes `SHA256SUMS`, generates the notes with
   `buildscripts/ci/tools/release_notes.py` and publishes with
   `softprops/action-gh-release@v2` using `GITHUB_TOKEN`.

## Squirrel packaging details

`buildscripts/ci/windows/package_squirrel.ps1` performs these steps.

1. Reads `version.cmake` and derives the SemVer 1 package version.
2. Downloads `nuget.exe` and the `squirrel.windows` NuGet package from the URLs
   pinned in `buildscripts/packaging/Windows/Squirrel/squirrel.lock.json` and
   verifies both against the pinned SHA256. A mismatch is a hard failure.
3. Deletes the bundled `signtool.exe`.
4. Stages `build.install` into `lib\net45` of the package payload. The default
   `Preserve` layout keeps the installed tree exactly as built, so the
   application still finds its resources at the paths it expects
   (`bin\Audacity4.exe` with the data directories as siblings of `bin`).
5. Expands `Audacity.nuspec.in` and runs `nuget pack`.
6. Runs `Squirrel.exe --releasify` with `--no-msi`, the application icon and the
   setup icon, seeding the release directory with the previous release when one
   was supplied so that a delta package is produced.
7. Fails if `Setup.exe`, `RELEASES` or the full `.nupkg` is missing, if an MSI
   appears, if a delta was expected but not produced, or if any executable,
   including the `Update.exe` inside the package, is signed.
8. Writes `SHA256SUMS` next to the artifacts.

### Shortcuts: the root launcher

Squirrel creates Start Menu and desktop shortcuts only for executables located
at the root of the package payload. Material Audacity installs its real
executable in `bin\`, because the application resolves its resource
directories relative to that layout, and moving it to the package root would
break resource resolution. `package_squirrel.ps1` solves this the way
Squirrel itself recommends: a tiny native launcher,
`buildscripts/packaging/Windows/Squirrel/launcher/MaterialAudacity.c`, is
compiled and placed at the payload root during packaging (see
`New-ShortcutLauncher` in the script). Squirrel creates the Start Menu and
desktop shortcuts against this launcher, which finds and starts
`bin\Audacity4.exe`, forwards its full command line and waits for it to exit
with the same code.

The launcher deliberately carries no `SquirrelAwareVersion` resource. An
executable that declares itself Squirrel aware is expected to handle the
`--squirrel-install`, `--squirrel-updated`, `--squirrel-obsolete` and
`--squirrel-uninstall` events itself, and Squirrel then skips its own default
shortcut management for that package. Staying Squirrel unaware is what makes
Squirrel create and remove the shortcuts automatically on install, update and
uninstall, which is the behaviour this project wants. As a defensive measure,
`src/app/main.cpp` still recognises these arguments (plus
`--squirrel-firstrun`) at the very top of `main()` and exits immediately
without opening a window, so that a future change to the packaging decision
above cannot make an install or uninstall stall on a flashed open window.

The `-Layout Flat` option lifts the application to the package root instead
and needs no launcher, but it is not the supported layout because it breaks
resource resolution; it exists only for local experimentation. `-SkipLauncher`
skips building the launcher entirely, which also skips shortcut creation, and
is for local experimentation only.

`package_squirrel.ps1` verifies the launcher itself: it must be unsigned
(code signing is permanently prohibited everywhere in this project) and it
must be present inside the full `.nupkg` that ships, or the script fails
rather than shipping a package with no Start Menu entry.

## Automatic updates

The installed application checks the feed's `RELEASES` file in the
background and offers a non-blocking restart once a package has downloaded
and verified against that file's own SHA1 and size, exactly as this document
already requires "every installed user-facing app" to do. See
`docs/features/automatic-updates.md` for the full behaviour, states and
verification. The default feed points at
`https://github.com/Ding-Ding-Projects/audacity/releases/latest/download/RELEASES`,
which is the same `RELEASES` file `package_squirrel.ps1` publishes as part of
every release's artifacts. The checker never claims signature verification;
it is an integrity check against the feed's own hash only.

## Release notes

`buildscripts/ci/tools/release_notes.py` writes the notes. Every release
contains, in addition to the download and verification instructions:

- the workflow start time in UTC, captured in the first step of the first job,
- the workflow completion time in UTC,
- the end to end duration,
- a line count table produced by `buildscripts/ci/tools/count_lines.py`.

`count_lines.py` counts source lines by language across `src/`, `docs/site` and
`buildscripts`, and excludes `muse/`, `au3/`, `thirdparty/`, `muse_deps/`,
build output directories and every binary file type.

Both tools run standalone:

```bash
python3 buildscripts/ci/tools/count_lines.py
python3 buildscripts/ci/tools/release_notes.py --tag v4.0.0-m3.1 \
    --start-time 2026-01-01T10:00:00Z --assets-dir release-assets
```

## Documentation site

`.github/workflows/material-audacity-pages.yml` uploads `docs/` with
`actions/upload-pages-artifact` and publishes it with `actions/deploy-pages` on
pushes to `master` that touch `docs/`.

## Local one-click scripts

| Script | Effect |
| --- | --- |
| `download-dependencies.bat` | Checks Python, CMake and the Visual Studio 2022 C++ toolset, installs Qt 6.10.1 with aqtinstall, installs ninja, and fetches the pinned `nuget.exe` and `squirrel.windows` package. |
| `build.bat` / `build.bat /s` | Configures and builds RelWithDebInfo into `build\windows` and installs into `build.install`. `/s` writes full logs to disk and prints only warnings, errors and the summary. |
| `build-installer.bat` / `build-installer.bat /s` | Runs the Squirrel packaging into `dist\squirrel-windows`. |
| `build.sh` / `build.sh -s` | The Linux equivalent of `build.bat`, building into `build.linux`. |

## What is deliberately absent

- No WiX, MSI, NSIS, Inno Setup or portable-only installer path. The former
  `buildscripts/packaging/Windows/Installer` sources have been deleted.
- No signing secret, signing service or `signtool` invocation anywhere.
- No workflow secret other than `GITHUB_TOKEN`.
- No release for non tag pushes.
