<#
.SYNOPSIS
    Package Material Audacity as a genuine Squirrel.Windows installer.

.DESCRIPTION
    Wraps the real built application tree (build.install, produced by
    buildscripts/ci/windows/ci_build.cmake) into a NuGet package and runs
    Squirrel.exe --releasify on it, producing Setup.exe, RELEASES, the full
    .nupkg and, when a previous release is supplied, a delta .nupkg.

    Code signing is permanently prohibited. This script never calls signtool,
    accepts no signing parameter, and deletes the signtool binary that ships
    inside the squirrel.windows package before Squirrel is invoked. Every
    produced executable is checked with Get-AuthenticodeSignature and the
    script fails if anything reports a status other than NotSigned.

.EXAMPLE
    pwsh -File buildscripts/ci/windows/package_squirrel.ps1 `
        -InstallDir build.install -OutDir dist/squirrel-windows -Tag v4.0.0-m3.1
#>

[CmdletBinding()]
param(
    # Built application tree to wrap.
    [string] $InstallDir = "build.install",

    # Directory that receives Setup.exe, RELEASES and the .nupkg files.
    [string] $OutDir = "dist/squirrel-windows",

    # Directory holding a previous RELEASES file and .nupkg files. When given,
    # Squirrel generates delta packages against them.
    [string] $PreviousReleasesDir = "",

    # Release tag, for example v4.0.0-m3.1. Used to derive the version label.
    [string] $Tag = "",

    # Fallback build counter used when no tag is present.
    [string] $RunNumber = "0",

    # Explicit package version. Overrides the derivation from Tag/RunNumber.
    [string] $Version = "",

    # NuGet package id. Must stay stable across releases or Squirrel will not
    # recognise installed versions as the same application.
    [string] $PackageId = "Audacity",

    [string] $PackageTitle = "Material Audacity",

    # Where pinned tools are downloaded and expanded.
    [string] $ToolsDir = "",

    # Preserve keeps the installed tree exactly as built (bin\ plus data
    # directories). Flat lifts the contents of bin\ to the package root.
    [ValidateSet("Preserve", "Flat")]
    [string] $Layout = "Preserve",

    # Skips building the shortcut launcher. Only useful for local experiments:
    # without the launcher Squirrel creates no Start Menu or desktop shortcut.
    [switch] $SkipLauncher
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Write-Section([string] $Text) {
    Write-Host ""
    Write-Host "=== $Text" -ForegroundColor Cyan
}

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = (Resolve-Path (Join-Path $scriptDir "..\..\..")).Path
$squirrelInputs = Join-Path $repoRoot "buildscripts\packaging\Windows\Squirrel"

if (-not [System.IO.Path]::IsPathRooted($InstallDir)) {
    $InstallDir = Join-Path $repoRoot $InstallDir
}
if (-not [System.IO.Path]::IsPathRooted($OutDir)) {
    $OutDir = Join-Path $repoRoot $OutDir
}
if ([string]::IsNullOrWhiteSpace($ToolsDir)) {
    $ToolsDir = Join-Path $repoRoot "build.tools\squirrel"
}
if (-not [System.IO.Path]::IsPathRooted($ToolsDir)) {
    $ToolsDir = Join-Path $repoRoot $ToolsDir
}

if (-not (Test-Path -LiteralPath $InstallDir)) {
    throw "Install tree not found: $InstallDir. Run buildscripts/ci/windows/ci_build.cmake first."
}

# ---------------------------------------------------------------------------
# Version
# ---------------------------------------------------------------------------

function Get-BaseVersion {
    $versionFile = Join-Path $repoRoot "version.cmake"
    if (-not (Test-Path -LiteralPath $versionFile)) {
        throw "version.cmake not found at $versionFile"
    }
    $text = Get-Content -LiteralPath $versionFile -Raw
    $parts = @{}
    foreach ($key in @("MAJOR", "MINOR", "PATCH")) {
        $match = [regex]::Match($text, "set\(MUSE_APP_VERSION_$key\s+`"([0-9]+)`"\)")
        if (-not $match.Success) {
            throw "Could not read MUSE_APP_VERSION_$key from version.cmake"
        }
        $parts[$key] = $match.Groups[1].Value
    }
    return "$($parts['MAJOR']).$($parts['MINOR']).$($parts['PATCH'])"
}

function ConvertTo-SemVer1PreRelease([string] $Label) {
    # NuGet and Squirrel accept SemVer 1 only: the pre-release label must be
    # alphanumeric, with no dots or hyphens. v4.0.0-m3.7 becomes 4.0.0-m3007.
    $clean = $Label -replace '[^0-9A-Za-z]', ''
    if ($clean.Length -gt 20) {
        $clean = $clean.Substring(0, 20)
    }
    return $clean
}

$baseVersion = Get-BaseVersion
if ([string]::IsNullOrWhiteSpace($Version)) {
    if ($Tag -match '^v?(?<base>\d+\.\d+\.\d+)-m3\.(?<n>\d+)$') {
        $baseVersion = $Matches['base']
        $label = "m3{0:d3}" -f [int]$Matches['n']
        $Version = "$baseVersion-$(ConvertTo-SemVer1PreRelease $label)"
    }
    elseif ($Tag -match '^v?(?<base>\d+\.\d+\.\d+)$') {
        $Version = $Matches['base']
    }
    else {
        $label = "ci{0:d6}" -f [int]$RunNumber
        $Version = "$baseVersion-$(ConvertTo-SemVer1PreRelease $label)"
    }
}

if ($Version -notmatch '^\d+\.\d+\.\d+(-[0-9A-Za-z]{1,20})?$') {
    throw "Package version '$Version' is not SemVer 1 compatible, which Squirrel.Windows requires."
}

Write-Section "Version"
Write-Host "Base version    : $baseVersion"
Write-Host "Package id      : $PackageId"
Write-Host "Package version : $Version"

# ---------------------------------------------------------------------------
# Pinned tools
# ---------------------------------------------------------------------------

Write-Section "Tools"

$lock = Get-Content -LiteralPath (Join-Path $squirrelInputs "squirrel.lock.json") -Raw | ConvertFrom-Json

New-Item -ItemType Directory -Force -Path $ToolsDir | Out-Null

function Get-PinnedFile([string] $Url, [string] $Sha256, [string] $Destination) {
    if (Test-Path -LiteralPath $Destination) {
        $existing = (Get-FileHash -LiteralPath $Destination -Algorithm SHA256).Hash
        if ($existing -ieq $Sha256) {
            Write-Host "Reusing verified $Destination"
            return
        }
        Remove-Item -LiteralPath $Destination -Force
    }
    Write-Host "Downloading $Url"
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    Invoke-WebRequest -Uri $Url -OutFile $Destination -UseBasicParsing
    $actual = (Get-FileHash -LiteralPath $Destination -Algorithm SHA256).Hash
    if ($actual -ine $Sha256) {
        throw "SHA256 mismatch for $Url. Expected $Sha256, got $actual."
    }
    Write-Host "Verified SHA256 $actual"
}

$nugetExe = Join-Path $ToolsDir "nuget.exe"
Get-PinnedFile -Url $lock.nuget.url -Sha256 $lock.nuget.sha256 -Destination $nugetExe

$squirrelNupkg = Join-Path $ToolsDir "squirrel.windows.$($lock.squirrel.version).nupkg"
Get-PinnedFile -Url $lock.squirrel.url -Sha256 $lock.squirrel.sha256 -Destination $squirrelNupkg

$squirrelRoot = Join-Path $ToolsDir "squirrel.windows.$($lock.squirrel.version)"
if (Test-Path -LiteralPath $squirrelRoot) {
    Remove-Item -LiteralPath $squirrelRoot -Recurse -Force
}
$squirrelZip = "$squirrelNupkg.zip"
Copy-Item -LiteralPath $squirrelNupkg -Destination $squirrelZip -Force
Expand-Archive -LiteralPath $squirrelZip -DestinationPath $squirrelRoot -Force
Remove-Item -LiteralPath $squirrelZip -Force

$squirrelExe = Join-Path $squirrelRoot "tools\Squirrel.exe"
if (-not (Test-Path -LiteralPath $squirrelExe)) {
    throw "Squirrel.exe not found in the squirrel.windows package at $squirrelExe"
}

# Code signing is permanently prohibited. Remove the bundled signing tool so it
# cannot be reached even by accident.
$bundledSigntool = Join-Path $squirrelRoot "tools\signtool.exe"
if (Test-Path -LiteralPath $bundledSigntool) {
    Remove-Item -LiteralPath $bundledSigntool -Force
    Write-Host "Removed the bundled signtool.exe. Code signing is permanently prohibited."
}

# ---------------------------------------------------------------------------
# Stage the package payload
# ---------------------------------------------------------------------------

Write-Section "Stage payload"

$stageDir = Join-Path $ToolsDir "stage"
if (Test-Path -LiteralPath $stageDir) {
    Remove-Item -LiteralPath $stageDir -Recurse -Force
}
$payloadDir = Join-Path $stageDir "lib\net45"
New-Item -ItemType Directory -Force -Path $payloadDir | Out-Null

Copy-Item -Path (Join-Path $InstallDir "*") -Destination $payloadDir -Recurse -Force

if ($Layout -eq "Flat") {
    $binDir = Join-Path $payloadDir "bin"
    if (Test-Path -LiteralPath $binDir) {
        Copy-Item -Path (Join-Path $binDir "*") -Destination $payloadDir -Recurse -Force
        Remove-Item -LiteralPath $binDir -Recurse -Force
    }
}

# ---------------------------------------------------------------------------
# Shortcut launcher
#
# Squirrel creates Start Menu and desktop shortcuts for executables at the root
# of the package payload only. The application itself lives in bin\, because it
# resolves its resources relative to that layout, so a small native launcher is
# compiled and placed at the payload root. Squirrel makes the shortcuts point at
# the launcher, and the launcher starts bin\Audacity4.exe.
# ---------------------------------------------------------------------------

function Import-VsDeveloperEnvironment {
    # ci_build.bat enters the Visual Studio environment through vswhere and
    # vcvars64.bat. The same route is used here so cl.exe and rc.exe are found
    # exactly where the application build finds them.
    if (Get-Command cl.exe -ErrorAction SilentlyContinue) {
        Write-Host "cl.exe is already on PATH, reusing the current environment."
        return
    }

    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path -LiteralPath $vswhere)) {
        throw "vswhere.exe not found at $vswhere, so the Visual Studio environment cannot be entered."
    }

    $installPath = & $vswhere -latest -products * `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -property installationPath
    if ([string]::IsNullOrWhiteSpace($installPath)) {
        throw "vswhere.exe found no Visual Studio installation with the x64 C++ toolset."
    }

    $vcvars = Join-Path $installPath "VC\Auxiliary\Build\vcvars64.bat"
    if (-not (Test-Path -LiteralPath $vcvars)) {
        throw "vcvars64.bat not found at $vcvars"
    }

    Write-Host "Entering the Visual Studio environment from $vcvars"
    $output = & cmd.exe /c "call `"$vcvars`" >nul && set"
    foreach ($line in $output) {
        if ($line -match '^([^=]+)=(.*)$') {
            Set-Item -Path ("Env:" + $Matches[1]) -Value $Matches[2] -ErrorAction SilentlyContinue
        }
    }

    if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
        throw "cl.exe is still not available after entering the Visual Studio environment."
    }
}

function New-ShortcutLauncher([string] $Destination) {
    $launcherSrcDir = Join-Path $squirrelInputs "launcher"
    $launcherC = Join-Path $launcherSrcDir "MaterialAudacity.c"
    $launcherRc = Join-Path $launcherSrcDir "MaterialAudacity.rc"
    foreach ($required in @($launcherC, $launcherRc)) {
        if (-not (Test-Path -LiteralPath $required)) {
            throw "Launcher source not found: $required"
        }
    }

    Import-VsDeveloperEnvironment

    $buildDir = Join-Path $ToolsDir "launcher"
    if (Test-Path -LiteralPath $buildDir) {
        Remove-Item -LiteralPath $buildDir -Recurse -Force
    }
    New-Item -ItemType Directory -Force -Path $buildDir | Out-Null

    Copy-Item -LiteralPath $launcherC -Destination $buildDir -Force
    Copy-Item -LiteralPath $iconSource -Destination (Join-Path $buildDir "AppIcon.ico") -Force

    # The version resource carries the package version. The comma form needs
    # four numeric fields, so the pre-release label is dropped from it.
    $numeric = ($Version -split '-')[0]
    $versionComma = ($numeric -replace '\.', ',') + ",0"
    $rcText = (Get-Content -LiteralPath $launcherRc -Raw).
        Replace("@VERSION_COMMA@", $versionComma).
        Replace("@VERSION_STRING@", $Version)
    $rcPath = Join-Path $buildDir "MaterialAudacity.rc"
    Set-Content -LiteralPath $rcPath -Value $rcText -Encoding UTF8

    Push-Location $buildDir
    try {
        & rc.exe /nologo /fo MaterialAudacity.res MaterialAudacity.rc
        if ($LASTEXITCODE -ne 0) {
            throw "rc.exe failed with exit code $LASTEXITCODE"
        }

        # /MT links the static CRT, so the launcher needs no Visual C++
        # runtime beside it. Code signing is permanently prohibited, and no
        # signing switch is passed here or anywhere else.
        & cl.exe /nologo /W4 /O1 /MT /DUNICODE /D_UNICODE MaterialAudacity.c MaterialAudacity.res `
            /link /SUBSYSTEM:WINDOWS /OUT:MaterialAudacity.exe kernel32.lib shell32.lib user32.lib
        if ($LASTEXITCODE -ne 0) {
            throw "cl.exe failed with exit code $LASTEXITCODE"
        }
    }
    finally {
        Pop-Location
    }

    $built = Join-Path $buildDir "MaterialAudacity.exe"
    if (-not (Test-Path -LiteralPath $built)) {
        throw "The launcher build produced no MaterialAudacity.exe"
    }

    Copy-Item -LiteralPath $built -Destination $Destination -Force
    Write-Host "Built the shortcut launcher and placed it at the package root."
}

$iconSource = Join-Path $repoRoot "share\icons\AppIcon\AU4_AppIcon.ico"
if (-not (Test-Path -LiteralPath $iconSource)) {
    throw "Application icon not found: $iconSource"
}

$launcherExe = Join-Path $payloadDir "MaterialAudacity.exe"
if ($SkipLauncher) {
    Write-Warning ("SkipLauncher was requested. Squirrel will create no Start " +
        "Menu or desktop shortcut for this package.")
} elseif ($Layout -eq "Flat") {
    Write-Host "Flat layout puts the application at the package root, so no launcher is needed."
} else {
    New-ShortcutLauncher -Destination $launcherExe
}

$appExe = Get-ChildItem -LiteralPath $payloadDir -Filter "*.exe" -Recurse |
    Where-Object { $_.Name -like "Audacity*" } |
    Select-Object -First 1
if (-not $appExe) {
    throw "No Audacity executable found under $payloadDir"
}
Write-Host "Application executable: $($appExe.FullName.Substring($payloadDir.Length + 1))"

$rootExe = Get-ChildItem -LiteralPath $payloadDir -Filter "*.exe" |
    Select-Object -First 1
if (-not $rootExe) {
    Write-Warning ("No executable is at the package root ($Layout layout). " +
        "Squirrel creates shortcuts for root level executables only, so this " +
        "package produces no shortcut. See docs/design/RELEASE.md.")
} else {
    Write-Host "Package root executable for shortcuts: $($rootExe.Name)"
}

# The launcher is shipped, so it has to be unsigned like everything else.
if (Test-Path -LiteralPath $launcherExe) {
    $launcherStatus = (Get-AuthenticodeSignature -LiteralPath $launcherExe).Status
    Write-Host ("Signature status of MaterialAudacity.exe: {0}" -f $launcherStatus)
    if ($launcherStatus -ne "NotSigned") {
        throw "MaterialAudacity.exe reports signature status '$launcherStatus'. Code signing is permanently prohibited."
    }
}

$iconPath = Join-Path $stageDir "AU4_AppIcon.ico"
Copy-Item -LiteralPath $iconSource -Destination $iconPath -Force

# ---------------------------------------------------------------------------
# Generate the .nuspec and pack
# ---------------------------------------------------------------------------

Write-Section "Pack"

$nuspecTemplate = Get-Content -LiteralPath (Join-Path $squirrelInputs "Audacity.nuspec.in") -Raw
$nuspec = $nuspecTemplate.
    Replace("@PACKAGE_ID@", $PackageId).
    Replace("@PACKAGE_TITLE@", $PackageTitle).
    Replace("@PACKAGE_VERSION@", $Version).
    Replace("@PACKAGE_AUTHORS@", "Audacity").
    Replace("@PACKAGE_PROJECT_URL@", "https://www.audacityteam.org").
    Replace("@PACKAGE_ICON_URL@", "https://www.audacityteam.org/favicon.ico").
    Replace("@PACKAGE_DESCRIPTION@", "Material Audacity, a Material Design 3 rewrite of the Audacity 4 user interface.").
    Replace("@PACKAGE_COPYRIGHT@", "Audacity contributors")

$nuspecPath = Join-Path $stageDir "$PackageId.nuspec"
Set-Content -LiteralPath $nuspecPath -Value $nuspec -Encoding UTF8

$packDir = Join-Path $ToolsDir "pack"
if (Test-Path -LiteralPath $packDir) {
    Remove-Item -LiteralPath $packDir -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $packDir | Out-Null

& $nugetExe pack $nuspecPath -BasePath $stageDir -OutputDirectory $packDir -NonInteractive
if ($LASTEXITCODE -ne 0) {
    throw "nuget pack failed with exit code $LASTEXITCODE"
}

$builtNupkg = Get-ChildItem -LiteralPath $packDir -Filter "*.nupkg" | Select-Object -First 1
if (-not $builtNupkg) {
    throw "nuget pack produced no .nupkg in $packDir"
}
Write-Host "Packed $($builtNupkg.Name)"

# ---------------------------------------------------------------------------
# Releasify
# ---------------------------------------------------------------------------

Write-Section "Releasify"

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null

$hasPrevious = $false
if (-not [string]::IsNullOrWhiteSpace($PreviousReleasesDir) -and
    (Test-Path -LiteralPath $PreviousReleasesDir)) {
    $previous = Get-ChildItem -LiteralPath $PreviousReleasesDir -File |
        Where-Object { $_.Name -eq "RELEASES" -or $_.Extension -eq ".nupkg" }
    if ($previous) {
        foreach ($item in $previous) {
            Copy-Item -LiteralPath $item.FullName -Destination $OutDir -Force
        }
        $hasPrevious = $true
        Write-Host "Seeded $OutDir with $($previous.Count) previous release files for delta generation."
    }
}
if (-not $hasPrevious) {
    Write-Host "No previous release supplied. Only a full package will be produced."
}

$releasifyArgs = @(
    "--releasify", $builtNupkg.FullName,
    "--releaseDir", $OutDir,
    "--icon", $iconPath,
    "--setupIcon", $iconPath,
    "--no-msi"
)
if (-not $hasPrevious) {
    $releasifyArgs += "--no-delta"
}

# Squirrel.exe is a Windows GUI subsystem executable. Invoking it with the call
# operator returns immediately, before any output exists, so the process must be
# awaited explicitly. Its diagnostics land in SquirrelSetup.log next to the tool.
$squirrelLog = Join-Path (Split-Path -Parent $squirrelExe) "SquirrelSetup.log"
if (Test-Path -LiteralPath $squirrelLog) { Remove-Item -LiteralPath $squirrelLog -Force }
$releasifyProcess = Start-Process -FilePath $squirrelExe -ArgumentList $releasifyArgs -Wait -PassThru -NoNewWindow
if (Test-Path -LiteralPath $squirrelLog) {
    Write-Host "--- SquirrelSetup.log ---"
    Get-Content -LiteralPath $squirrelLog | Select-Object -Last 80 | ForEach-Object { Write-Host $_ }
    Write-Host "--- end of SquirrelSetup.log ---"
}
if ($releasifyProcess.ExitCode -ne 0) {
    throw "Squirrel --releasify failed with exit code $($releasifyProcess.ExitCode)"
}

# ---------------------------------------------------------------------------
# Verify the outputs
# ---------------------------------------------------------------------------

Write-Section "Verify"

$setupExe = Join-Path $OutDir "Setup.exe"
$releasesFile = Join-Path $OutDir "RELEASES"
foreach ($required in @($setupExe, $releasesFile)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Squirrel did not produce the required artifact: $required"
    }
}

$fullNupkg = Get-ChildItem -LiteralPath $OutDir -Filter "*-full.nupkg" |
    Where-Object { $_.Name -like "$PackageId*$Version*" }
if (-not $fullNupkg) {
    throw "No full .nupkg for $PackageId $Version was produced in $OutDir"
}

$deltaNupkg = Get-ChildItem -LiteralPath $OutDir -Filter "*-delta.nupkg" -ErrorAction SilentlyContinue
if ($hasPrevious -and -not $deltaNupkg) {
    throw "A previous release was supplied but no delta .nupkg was produced."
}

# Any MSI is a contract violation: Squirrel.Windows is the only installer.
$strayMsi = Get-ChildItem -LiteralPath $OutDir -Filter "*.msi" -ErrorAction SilentlyContinue
if ($strayMsi) {
    throw "An MSI was produced. Squirrel.Windows is the only supported installer."
}

# Every produced executable must be unsigned.
$executables = @(Get-ChildItem -LiteralPath $OutDir -Filter "*.exe" -File)
foreach ($exe in $executables) {
    $status = (Get-AuthenticodeSignature -LiteralPath $exe.FullName).Status
    Write-Host ("Signature status of {0}: {1}" -f $exe.Name, $status)
    if ($status -ne "NotSigned") {
        throw "$($exe.Name) reports signature status '$status'. Code signing is permanently prohibited."
    }
}

# Update.exe travels inside the full package. Check it too.
$updateCheckDir = Join-Path $ToolsDir "verify-nupkg"
if (Test-Path -LiteralPath $updateCheckDir) {
    Remove-Item -LiteralPath $updateCheckDir -Recurse -Force
}
$nupkgZip = Join-Path $ToolsDir "verify-full.zip"
Copy-Item -LiteralPath $fullNupkg[0].FullName -Destination $nupkgZip -Force
Expand-Archive -LiteralPath $nupkgZip -DestinationPath $updateCheckDir -Force
Remove-Item -LiteralPath $nupkgZip -Force
$packagedChecks = @("Update.exe", "MaterialAudacity.exe")
foreach ($name in $packagedChecks) {
    foreach ($exe in Get-ChildItem -LiteralPath $updateCheckDir -Filter $name -Recurse -File) {
        $status = (Get-AuthenticodeSignature -LiteralPath $exe.FullName).Status
        Write-Host ("Signature status of packaged {0}: {1}" -f $exe.Name, $status)
        if ($status -ne "NotSigned") {
            throw "Packaged $name reports signature status '$status'. Code signing is permanently prohibited."
        }
    }
}

if (-not $SkipLauncher -and $Layout -eq "Preserve") {
    $packagedLauncher = Get-ChildItem -LiteralPath $updateCheckDir -Filter "MaterialAudacity.exe" -Recurse -File
    if (-not $packagedLauncher) {
        throw "The full .nupkg contains no MaterialAudacity.exe, so Squirrel would create no shortcut."
    }
}

Write-Section "Checksums"
$checksums = Join-Path $OutDir "SHA256SUMS"
Get-ChildItem -LiteralPath $OutDir -File |
    Where-Object { $_.Name -ne "SHA256SUMS" } |
    Sort-Object Name |
    ForEach-Object {
        "{0}  {1}" -f (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLower(), $_.Name
    } | Set-Content -LiteralPath $checksums -Encoding ASCII
Get-Content -LiteralPath $checksums | Write-Host

Write-Section "Done"
Get-ChildItem -LiteralPath $OutDir -File | Select-Object Name, Length | Format-Table | Out-String | Write-Host
Write-Host "Squirrel.Windows packaging finished in $OutDir"
