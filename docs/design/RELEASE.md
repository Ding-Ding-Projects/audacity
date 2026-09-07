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
| Releases | The delivery workflow runs on pushes to `main` and manual dispatch, not tag pushes; publication is non-draft |
| Tag on a plain push | Shared create-only reservation of `v<version.cmake>-m3.<sequence>`; the run number is a minimum, not the tag identity |
| Dim sum code name | Next unused dish from the public catalog, photo attached, resolved by `buildscripts/ci/tools/dim_sum_release.py` |
| Documentation site | `docs/` deployed to GitHub Pages |
| Secrets used | `GITHUB_TOKEN` only |

## Artifacts

Every release publishes the following assets, plus one dim sum photo named
`dim-sum-<dish id>-<slug>.png` when the public catalog could be reached (the
notes say so when it could not).

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

### Shared manual and workflow tag reservation

`buildscripts/ci/tools/reserve_release_tag.py` is the shared manual CLI and
workflow entry point. It uses the existing authenticated `gh` CLI, requires an
explicit repository and full candidate commit SHA, confirms that commit exists,
reads all matching tag-reference pages, and chooses the next numeric suffix for
that exact product-version prefix. `--minimum` supplies an optional sequence
floor, normally the workflow run number. A manual `v4.0.0-m3.14` therefore makes
the next workflow reserve at least `v4.0.0-m3.15`, even if its run number is 14.

After the release coordinator explicitly selects the immutable candidate:

```powershell
python buildscripts/ci/tools/reserve_release_tag.py reserve `
  --repo Ding-Ding-Projects/audacity --sha <full-candidate-commit-sha> `
  --version 4.0.0 --output <new-reservation-receipt.json>
```

The helper creates the exact tag reference through `gh api` and reads it back
to verify that it points directly to the intended commit. Creation is atomic
and never updates, deletes, force-moves or reuses a tag. Only a confirmed
reference-already-exists response permits a retry; each retry starts with a
fresh complete inventory, and three collisions stop the operation. Invalid
SHAs, malformed refs, authentication errors and uncertain network outcomes
stop immediately. An uncertain POST may already have created its tag and must
be investigated before retrying publication.

`--output` is mandatory for reservation. A timed-out POST, malformed successful
response, unconfirmed server error or failed target readback preserves an
`uncertain` receipt containing the exact repository, tag, full ref, candidate
SHA and attempt number. The CLI repeats that identity in stderr and exits
nonzero without retrying the POST. An immediate missing-ref response would not
prove that the original request cannot still complete; the coordinator must
settle that uncertainty before proceeding. The helper does not automatically
retry or treat absence as permission to reuse the name.

Both verified and uncertain receipts are written beside their destination,
flushed, and atomically linked into a previously absent output path. Existing
receipts are never overwritten. A persistence failure retains any staged record
for inspection and reports its path plus the complete attempted identity in
stderr. It does not roll back or retry a possibly completed tag creation.

Immediately before publishing, both routes use:

```powershell
python buildscripts/ci/tools/reserve_release_tag.py verify `
  --repo Ding-Ding-Projects/audacity --sha <full-candidate-commit-sha> `
  --tag <reserved-tag>
```

Then the explicitly coordinated publisher calls `gh release create` with
`--verify-tag`, the exact repository and candidate, and the verified assets.
The workflow grants its reservation job `contents: write` and uses
`secrets.RELEASE_TOKEN || secrets.ORG_TOKEN || secrets.GITHUB_TOKEN` through
`GH_TOKEN`; the helper never reads or prints credentials itself.

The reservation job provisions Python before its first command, then checks
the CLI before the workflow timing request or Configure step uses it:

| Tool | Provisioning and compatibility boundary |
| --- | --- |
| Python | `actions/setup-python@v6` supplies Python 3.12; the job confirms `sys.version_info >= (3, 12)` before configuration. |
| GitHub CLI | The job checks `gh api --paginate --slurp` and `gh release create --verify-tag` support through CLI help. If absent or incompatible, it uses Chocolatey's `gh` package and refreshes the current and subsequent-step PATH, then repeats the capability check. |
| Chocolatey | Needed only for GitHub CLI repair on the Windows delivery worker. Its absence is reported explicitly and stops the job before any reservation attempt. |
| Compiler cache | Optional. `configure_compiler_cache.py` reuses an installed ccache with `4.8 <= version < 5` only after version, statistics and statistics-reset probes succeed. Otherwise it selects the supported uncached build. No Chocolatey or other download is attempted for this optional tool. |

These checks no longer infer command availability from the worker image name.
The offline reservation fixtures do not install tools or prove a hosted worker's
network bootstrap; the workflow's actual provisioning result remains separate
evidence.

### Optional compiler-cache bootstrap

The Windows delivery job records its cache decision in
`build.artifacts/compiler-cache.json` and exports `MUSE_CI_COMPILER_CACHE` plus
the exact selected `MUSE_CI_CCACHE_PROGRAM` through `GITHUB_ENV`. A compatible
installed ccache receives a unique configuration file under
`build.tools/compiler-cache`, with a 1 GB cache limit and the existing
`pch_defines,time_macros` settings. Existing cache configuration is not replaced.
The supported installed-version range is deliberately conservative; upstream
documents MSVC support in its [4.8.2 manual](https://ccache.dev/manual/4.8.2.html).

Missing tools, unsupported versions, unsuccessful probes and probe timeouts
select `OFF` and state the reason. Later cache commands are not called after a
failed probe. Because caching only avoids repeated compilation, this fallback
can make the build slower but does not skip compilation or packaging.

The root `ci_build.cmake` forwards that decision using
`compiler_cache_options.cmake`. `OFF` explicitly disables
`MUSE_COMPILE_USE_CCACHE` and clears both compiler launchers and the old cache
discovery result, so a missing executable or another cache cannot be silently
selected later. `ON` pins the exact probed executable, and stops if it has
disappeared. An unset CI decision preserves ordinary local-build defaults.
If the decision cannot be written to `GITHUB_ENV`, the bootstrap fails rather
than allowing a build with unknown launcher settings.

Focused offline verification:

```powershell
python -m unittest discover -s buildscripts/ci/tools -p test_configure_compiler_cache.py -v
```

These fixtures cover installed/missing/incompatible tools, failed and timed-out
probes, environment recording, and actual CMake option selection and stale
launcher clearing. They are local tests only, not workflow steps. They do not
claim a complete application build or a hosted-run result.

Reservation is **not a build or publisher lock**. It does not serialize final
publication, stop an older candidate finishing after a newer candidate, or
prove that a package belongs to its tag. The release coordinator still selects
one publisher and binds the build, package version and assets to its candidate.
A failed build can leave a reserved tag without a release. That tag remains
consumed; a later attempt reserves a new suffix instead of deleting or reusing
it. Tag creation cannot trigger this workflow because only `main` pushes and
manual dispatch are configured.

Sequence ordering is numeric within one product version. Package labels keep
the existing mapping, for example `.14` becomes `-m3014` and `.15` becomes
`-m3015`. Other product-version prefixes do not participate in the suffix
counter. The sequence is bounded to Int32 because the packaging script parses
it that way; exhaustion fails instead of wrapping. This helper does not change
Squirrel's prerelease ordering or the separately validated older-seed rule.

Offline fixtures run with:

```powershell
python -m unittest discover -s buildscripts/ci/tools -p test_reserve_release_tag.py -v
```

These fixtures create no real tag or release. They exercise manual/workflow
coexistence, pagination, collisions, authentication and malformed responses,
exact target checks, foreign version prefixes, bounds and transport arguments.

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
3. `release`, which runs after every successful build. It resolves the tag
   (the pushed tag, or `v<version>-m3.<run number>` for a plain push, refusing
   to reuse an existing tag), downloads both artifact sets, collects the
   release assets, fetches the dim sum code name and photo, writes
   `SHA256SUMS`, generates the notes with
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
8. Writes `SHA256SUMS` and `package-output-manifest.json` for exactly the current
   version's setup, feed, full package and optional delta. Feed entries must use
   unique case-insensitive leaf names and match the package SHA1 and byte size.
   A seed feed additionally identifies this package and one older baseline
   version, with exactly one full package and at most its matching delta.
   Ordering uses `NuGet.SemanticVersion` from the pinned Squirrel executable,
   including its prerelease rules, rather than string or numeric-version guesses.
9. Validates a same-volume candidate directory and activates it with directory
   renames under an exclusive output lock. A durable journal precedes the first
   rename. The previous generation remains in a uniquely named sibling directory;
   packaging never deletes it. The two renames have a brief interval when `OutDir`
   is absent, but never expose partially copied files. After interruption, the
   next invocation recovers under the same lock before doing packaging work.

An existing nonempty output directory can be replaced only when its exact names,
file hashes, sizes, feed and checksums agree with its manifest. An unknown file
(including `notes.nupkg`), a directory, or modified bytes stops replacement and
preserves all existing content. Legacy output without a manifest needs a **new
output path**. Packaging does not infer ownership from a filename extension.
Interrupted candidates, transaction journals, and previous directories remain
available for inspection and separately authorized cleanup. Malformed journals
fail closed; packaging does not guess which directory to restore.

The release collector uses `squirrel_output.ps1` to validate the exact package
directory against the intended release version and package id before copying.
It publishes `package-output-manifest.json` and preserves the package checksum
record as `PACKAGE-SHA256SUMS`. The release-wide `SHA256SUMS` is separate and also
covers those records plus added release assets. It never recursively selects
packages from retained previous-generation directories.

Focused local verification commands:

```powershell
pwsh -NoProfile -File buildscripts/ci/windows/test_squirrel_output_transaction.ps1
pwsh -NoProfile -File buildscripts/ci/windows/test_package_squirrel_output.ps1 -InstallDir build.install
```

The first uses small byte fixtures for ownership, seed validation, manifest
mismatch, interruption recovery and cross-process activation contention. The
version-order cases load the existing pinned Squirrel executable under
`build.tools`, or the exact path supplied through `-SquirrelExecutable`.
The payload cases verify that only the ten hash-pinned qpdf components enter
staging. qpdf activation locks, journals, stages, quarantines and backups remain
in the input tree and are omitted from the package; no recovery evidence is
deleted. Additional files inside the qpdf input directory are not package
components and are not copied. Unknown administration-name variants fail closed.
The second builds real full and delta packages from an existing immutable application
tree and validates release collection without rebuilding the application.
Neither test proves installation, installed-client updates or UI behavior.

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
