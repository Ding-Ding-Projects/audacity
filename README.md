# Audacity

[![Coverage](https://s3.us-east-1.amazonaws.com/extensions.musescore.org/test/code_coverage/au_coverage_badge.svg)](https://github.com/audacity/audacity/actions/workflows/au4_check_unit_tests.yml)

[**Audacity**](https://www.audacityteam.org) is an easy-to-use, multi-track audio editor and recorder for Windows, macOS, GNU/Linux and other operating systems. More info can be found on https://www.audacityteam.org

## This repository is currently undergoing major structural change.

We're currently working on Audacity 4, which means an entirely new UI and also refactorings aplenty. As such, the `master` branch is currently not particularly friendly to new contributors. It is still possible to submit patches to Audacity 3.x; make sure you branch off `audacity3` if you choose to do so. Build instructions for 3.x can be found [here](https://github.com/audacity/audacity/blob/release-3.7.0/BUILDING.md); build instructions for Audacity 4 can be found [here](https://github.com/audacity/audacity/blob/master/BUILDING.md).

You can stay updated with our efforts on [YouTube](https://youtube.com/@audacity), [Discord](https://discord.gg/audacity) and [our blog](https://audacityteam.org/blog).

## Build and run on a fresh Windows machine

Install [Python 3](https://www.python.org/downloads/windows/),
[CMake](https://cmake.org/download/), [Git](https://git-scm.com/download/win) and
the [Visual Studio 2022 Build Tools](https://visualstudio.microsoft.com/downloads/)
with the "Desktop development with C++" workload, then open a Command Prompt and
run this sequence:

```bat
git clone --recurse-submodules https://github.com/audacity/audacity.git
cd audacity
download-dependencies.bat
build.bat /s
build-installer.bat /s
```

`download-dependencies.bat` verifies the toolchain and installs Qt 6.10.1 and
the packaging tools. `build.bat /s` configures and builds RelWithDebInfo into
`build\windows` and installs the application into `build.install`, so you can
run it straight away:

```bat
build.install\bin\Audacity4.exe
```

`build-installer.bat /s` produces the Squirrel.Windows installer in
`dist\squirrel-windows`. On Linux the equivalent single command is `./build.sh`.

## Downloads and installer

Windows builds are published as a [Squirrel.Windows](https://github.com/Squirrel/Squirrel.Windows)
installer on the [releases page](https://github.com/audacity/audacity/releases).
Download `Setup.exe` and run it. It installs per user and needs no administrator
rights, and installed copies update themselves from the `RELEASES` feed that is
published with each release.

The installer is intentionally **not code signed**. Code signing is permanently
prohibited for this project, so Windows SmartScreen shows a warning on first
run. Verify the download yourself instead, in PowerShell:

```powershell
# The hash must match the matching line in the published SHA256SUMS file.
Get-FileHash .\Setup.exe -Algorithm SHA256

# The expected and only acceptable status is NotSigned.
(Get-AuthenticodeSignature .\Setup.exe).Status
```

Linux builds are published as an AppImage in the same release. The full release
contract, artifact list and verification procedure are documented in
[docs/design/RELEASE.md](docs/design/RELEASE.md).

## License

Audacity is open source software licensed GPLv3. Most code files are GPLv2-or-later, with the notable exceptions being `/au3/lib-src` (which contains third party libraries), as well as VST3-related code. Documentation is licensed CC-by 3.0 unless otherwise noted. Details can be found in the [license file](LICENSE.txt).
