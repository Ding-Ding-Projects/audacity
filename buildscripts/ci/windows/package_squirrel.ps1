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
. (Join-Path $scriptDir 'squirrel_output.ps1')
$repoRoot = (Resolve-Path (Join-Path $scriptDir "..\..\..")).Path
$squirrelInputs = Join-Path $repoRoot "buildscripts\packaging\Windows\Squirrel"

if (-not [System.IO.Path]::IsPathRooted($InstallDir)) {
    $InstallDir = Join-Path $repoRoot $InstallDir
}
if (-not [System.IO.Path]::IsPathRooted($OutDir)) {
    $OutDir = Join-Path $repoRoot $OutDir
}
if (-not [string]::IsNullOrWhiteSpace($PreviousReleasesDir) -and
    -not [System.IO.Path]::IsPathRooted($PreviousReleasesDir)) {
    $PreviousReleasesDir = Join-Path $repoRoot $PreviousReleasesDir
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

# Recover before any packaging work. The activation lock is acquired again for
# final publication, where existing output bytes are revalidated under the lock.
$outputLock = Open-SquirrelOutputLock $OutDir
try {
    Restore-SquirrelOutputTransaction $OutDir
    if ((Test-Path -LiteralPath $OutDir) -and @(Get-ChildItem -LiteralPath $OutDir -Force).Count) {
        $null = Assert-SquirrelOutput $OutDir
    }
} finally { $outputLock.Dispose() }
Assert-SquirrelLeafName $PackageId

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
$workDir = Join-Path $ToolsDir ('package-run-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $workDir | Out-Null

function Get-PinnedFile(
    [string] $Url,
    [string] $Sha256,
    [string] $Destination,
    [string] $FallbackPackageUrl = "",
    [string] $FallbackPackageEntry = ""
) {
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
    try {
        Invoke-WebRequest -Uri $Url -OutFile $Destination -UseBasicParsing
    }
    catch {
        Remove-Item -LiteralPath $Destination -Force -ErrorAction SilentlyContinue
        if ([string]::IsNullOrWhiteSpace($FallbackPackageUrl) -or
            [string]::IsNullOrWhiteSpace($FallbackPackageEntry)) {
            throw
        }

        # dist.nuget.org occasionally refuses a direct command-line executable
        # download on constrained Windows networks. The official NuGet package
        # carries the same NuGet.exe bytes, so use it only as a bounded fallback
        # and accept the extracted executable only after the lock digest matches.
        $fallbackPackage = "$Destination.fallback.nupkg"
        $fallbackDirectory = "$Destination.fallback"
        Remove-Item -LiteralPath $fallbackPackage -Force -ErrorAction SilentlyContinue
        Remove-Item -LiteralPath $fallbackDirectory -Recurse -Force -ErrorAction SilentlyContinue
        try {
            Write-Warning "Direct download failed: $($_.Exception.Message)"
            Write-Host "Downloading official NuGet package fallback $FallbackPackageUrl"
            & curl.exe --fail --location --silent --show-error --output $fallbackPackage $FallbackPackageUrl
            if ($LASTEXITCODE -ne 0) {
                throw "curl.exe failed to download official fallback package with exit code $LASTEXITCODE"
            }
            Copy-Item -LiteralPath $fallbackPackage -Destination "$fallbackPackage.zip" -Force
            Expand-Archive -LiteralPath "$fallbackPackage.zip" -DestinationPath $fallbackDirectory -Force
            Remove-Item -LiteralPath "$fallbackPackage.zip" -Force
            $fallbackBinary = Join-Path $fallbackDirectory $FallbackPackageEntry
            if (-not (Test-Path -LiteralPath $fallbackBinary)) {
                throw "Official fallback package lacks expected entry $FallbackPackageEntry"
            }
            Copy-Item -LiteralPath $fallbackBinary -Destination $Destination -Force
        }
        finally {
            Remove-Item -LiteralPath $fallbackPackage -Force -ErrorAction SilentlyContinue
            Remove-Item -LiteralPath "$fallbackPackage.zip" -Force -ErrorAction SilentlyContinue
            Remove-Item -LiteralPath $fallbackDirectory -Recurse -Force -ErrorAction SilentlyContinue
        }
    }
    $actual = (Get-FileHash -LiteralPath $Destination -Algorithm SHA256).Hash
    if ($actual -ine $Sha256) {
        throw "SHA256 mismatch for $Url. Expected $Sha256, got $actual."
    }
    Write-Host "Verified SHA256 $actual"
}

$nugetExe = Join-Path $ToolsDir "nuget.exe"
Get-PinnedFile -Url $lock.nuget.url -Sha256 $lock.nuget.sha256 -Destination $nugetExe `
    -FallbackPackageUrl "https://www.nuget.org/api/v2/package/NuGet.CommandLine/$($lock.nuget.version)" `
    -FallbackPackageEntry "tools\\NuGet.exe"

$squirrelNupkg = Join-Path $ToolsDir "squirrel.windows.$($lock.squirrel.version).nupkg"
Get-PinnedFile -Url $lock.squirrel.url -Sha256 $lock.squirrel.sha256 -Destination $squirrelNupkg

$squirrelRoot = Join-Path $workDir "squirrel.windows.$($lock.squirrel.version)"
if (Test-Path -LiteralPath $squirrelRoot) {
    Remove-Item -LiteralPath $squirrelRoot -Recurse -Force
}
$squirrelZip = Join-Path $workDir 'squirrel.zip'
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

$stageDir = Join-Path $workDir "stage"
if (Test-Path -LiteralPath $stageDir) {
    Remove-Item -LiteralPath $stageDir -Recurse -Force
}
$payloadDir = Join-Path $stageDir "lib\net45"
New-Item -ItemType Directory -Force -Path $payloadDir | Out-Null

Copy-SquirrelPayload -InstallDirectory $InstallDir -PayloadDirectory $payloadDir `
    -QpdfManifestPath (Join-Path $repoRoot 'buildscripts/converter-tools/qpdf.lock.json')

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

    $buildDir = Join-Path $workDir "launcher"
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

$packDir = Join-Path $workDir "pack"
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

$publishOutDir = $OutDir
New-Item -ItemType Directory -Force -Path $ToolsDir | Out-Null
$releasifyDir = Join-Path $workDir "releasify"
New-Item -ItemType Directory -Force -Path $releasifyDir | Out-Null

$hasPrevious = $false
if (-not [string]::IsNullOrWhiteSpace($PreviousReleasesDir) -and
    (Test-Path -LiteralPath $PreviousReleasesDir)) {
    $seedRoot = (Resolve-Path -LiteralPath $PreviousReleasesDir).Path
    if (([IO.Path]::GetFullPath($seedRoot)).TrimEnd('\\') -eq
        ([IO.Path]::GetFullPath($publishOutDir)).TrimEnd('\\')) {
        throw "PreviousReleasesDir must be separate from OutDir; seed files cannot share the publish directory."
    }
    $seedReleases = Join-Path $seedRoot "RELEASES"
    $seedEntries = Read-ReleaseEntries $seedReleases
    $semanticVersionType = ([Reflection.Assembly]::LoadFrom($squirrelExe)).GetType('NuGet.SemanticVersion')
    Assert-SquirrelSeed $seedEntries $PackageId $Version $semanticVersionType
    foreach ($entry in $seedEntries) {
        $seedPackage = Join-Path $seedRoot $entry.Name
        if (-not (Test-Path -LiteralPath $seedPackage)) {
            throw "Previous RELEASES references missing baseline package: $($entry.Name)"
        }
        Copy-Item -LiteralPath $seedPackage -Destination $releasifyDir -Force
    }
    Copy-Item -LiteralPath $seedReleases -Destination $releasifyDir -Force
    # Validate the copied snapshot too, before Squirrel consumes it.
    $null = Read-ReleaseEntries (Join-Path $releasifyDir 'RELEASES')
    $hasPrevious = $true
    Write-Host "Seeded $($seedEntries.Count) RELEASES-referenced baseline packages into a private delta workspace."
}
if (-not $hasPrevious) {
    Write-Host "No previous release supplied. Only a full package will be produced."
}

$releasifyArgs = @(
    "--releasify", $builtNupkg.FullName,
    "--releaseDir", $releasifyDir,
    "--icon", $iconPath,
    "--setupIcon", $iconPath,
    "--no-msi"
)
if (-not $hasPrevious) {
    $releasifyArgs += "--no-delta"
}

# Squirrel.exe is a Windows GUI subsystem executable. Invoking it with the call
# operator returns immediately, before any output exists, so the process must be
# awaited explicitly. Start-Process joins ArgumentList items into one command
# line, therefore quote every item before passing a path that contains spaces.
# Its diagnostics land in SquirrelSetup.log next to the tool.
function ConvertTo-CommandLineArgument([string] $Value) {
    return '"' + ($Value -replace '(\\*)"', '$1$1\\"' -replace '(\\+)$', '$1$1') + '"'
}

$squirrelLog = Join-Path (Split-Path -Parent $squirrelExe) "SquirrelSetup.log"
if (Test-Path -LiteralPath $squirrelLog) { Remove-Item -LiteralPath $squirrelLog -Force }
$releasifyCommandLine = ($releasifyArgs | ForEach-Object { ConvertTo-CommandLineArgument $_ }) -join ' '
$releasifyProcess = Start-Process -FilePath $squirrelExe -ArgumentList $releasifyCommandLine -Wait -PassThru -NoNewWindow
if (Test-Path -LiteralPath $squirrelLog) {
    Write-Host "--- SquirrelSetup.log ---"
    Get-Content -LiteralPath $squirrelLog | Select-Object -Last 80 | ForEach-Object { Write-Host $_ }
    Write-Host "--- end of SquirrelSetup.log ---"
}
if ($releasifyProcess.ExitCode -ne 0) {
    throw "Squirrel --releasify failed with exit code $($releasifyProcess.ExitCode)"
}

# ---------------------------------------------------------------------------
# Verify the private current-version output set
# ---------------------------------------------------------------------------

Write-Section "Verify"

$setupExe = Join-Path $releasifyDir "Setup.exe"
$releasesFile = Join-Path $releasifyDir "RELEASES"
foreach ($required in @($setupExe, $releasesFile)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Squirrel did not produce the required artifact: $required"
    }
}

$currentFullName = "$PackageId-$Version-full.nupkg"
$fullNupkg = @(Get-ChildItem -LiteralPath $releasifyDir -Filter $currentFullName -File)
if ($fullNupkg.Count -ne 1) {
    throw "Expected exactly one current full .nupkg named $currentFullName, found $($fullNupkg.Count)."
}

$currentDeltaName = "$PackageId-$Version-delta.nupkg"
$deltaNupkg = @(Get-ChildItem -LiteralPath $releasifyDir -Filter $currentDeltaName -File -ErrorAction SilentlyContinue)
if ($hasPrevious -and $deltaNupkg.Count -ne 1) {
    throw "A previous release was supplied but no delta .nupkg was produced."
}
if (-not $hasPrevious -and $deltaNupkg.Count -ne 0) {
    throw "No previous release was supplied, but a delta .nupkg was produced."
}

$privateReleaseEntries = Read-ReleaseEntries $releasesFile
$currentPackageNames = @($currentFullName) + @($deltaNupkg | ForEach-Object Name)
$publishedReleaseEntries = @($privateReleaseEntries | Where-Object { $_.Name -in $currentPackageNames })
if ($publishedReleaseEntries.Count -ne $currentPackageNames.Count) {
    throw "Squirrel RELEASES has no complete current-version entry set for $Version."
}
foreach ($entry in $publishedReleaseEntries) {
    if (-not (Test-Path -LiteralPath (Join-Path $releasifyDir $entry.Name))) {
        throw "Current RELEASES references missing current package: $($entry.Name)"
    }
}

$publishReleaseFile = Join-Path $releasifyDir "RELEASES.current"
$publishedReleaseEntries | ForEach-Object Line | Set-Content -LiteralPath $publishReleaseFile -Encoding ASCII
Write-Host "Published RELEASES has $($publishedReleaseEntries.Count) current-version entries; baseline files were delta inputs only."

function Get-PublishedFileName([IO.FileInfo] $File) {
    if ($File.Name -eq "RELEASES.current") { return "RELEASES" }
    return $File.Name
}

# Any MSI is a contract violation: Squirrel.Windows is the only installer.
$strayMsi = Get-ChildItem -LiteralPath $releasifyDir -Filter "*.msi" -ErrorAction SilentlyContinue
if ($strayMsi) {
    throw "An MSI was produced. Squirrel.Windows is the only supported installer."
}

# Every produced executable must be unsigned.
$executables = @(Get-ChildItem -LiteralPath $releasifyDir -Filter "*.exe" -File)
foreach ($exe in $executables) {
    $status = (Get-AuthenticodeSignature -LiteralPath $exe.FullName).Status
    Write-Host ("Signature status of {0}: {1}" -f $exe.Name, $status)
    if ($status -ne "NotSigned") {
        throw "$($exe.Name) reports signature status '$status'. Code signing is permanently prohibited."
    }
}

# Update.exe travels inside the full package. Check it too.
$updateCheckDir = Join-Path $workDir "verify-nupkg"
if (Test-Path -LiteralPath $updateCheckDir) {
    Remove-Item -LiteralPath $updateCheckDir -Recurse -Force
}
$nupkgZip = Join-Path $workDir "verify-full.zip"
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
$publishedFiles = @($setupExe, $publishReleaseFile) + @($fullNupkg | ForEach-Object FullName) + @($deltaNupkg | ForEach-Object FullName)
$checksums = Join-Path $releasifyDir "SHA256SUMS"
$publishedFiles |
    Get-Item |
    Sort-Object Name |
    ForEach-Object {
        $publishedName = Get-PublishedFileName $_
        "{0}  {1}" -f (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLower(), $publishedName
    } | Set-Content -LiteralPath $checksums -Encoding ASCII

$manifest = [ordered]@{
    schemaVersion = 1
    packageId = $PackageId
    packageVersion = $Version
    seedMode = if ($hasPrevious) { "baseline-used-for-delta-only" } else { "no-baseline" }
    releaseEntries = @($publishedReleaseEntries | ForEach-Object { [ordered]@{ name=$_.Name; sha1=$_.Sha1.ToLowerInvariant(); bytes=$_.Size } })
    files = @($publishedFiles | Get-Item | Sort-Object Name | ForEach-Object {
        [ordered]@{ name=(Get-PublishedFileName $_); bytes=$_.Length; sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant() }
    })
}
$manifestPath = Join-Path $releasifyDir "package-output-manifest.json"
$manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $manifestPath -Encoding UTF8

# Construct the exact publish set separately from the private baseline workspace.
# Publication validates both generations and retains the previous directory.
$publishCandidate = Join-Path $workDir 'publish-candidate'
New-Item -ItemType Directory -Path $publishCandidate | Out-Null
Copy-Item -LiteralPath $setupExe -Destination (Join-Path $publishCandidate "Setup.exe")
Copy-Item -LiteralPath $publishReleaseFile -Destination (Join-Path $publishCandidate "RELEASES")
foreach ($package in @($fullNupkg) + @($deltaNupkg)) {
    Copy-Item -LiteralPath $package.FullName -Destination $publishCandidate
}
Copy-Item -LiteralPath $checksums -Destination $publishCandidate
Copy-Item -LiteralPath $manifestPath -Destination $publishCandidate
Publish-SquirrelOutput -Candidate $publishCandidate -Directory $publishOutDir

$OutDir = $publishOutDir
Get-Content -LiteralPath (Join-Path $OutDir "SHA256SUMS") | Write-Host

Write-Section "Done"
Get-ChildItem -LiteralPath $OutDir -File | Select-Object Name, Length | Format-Table | Out-String | Write-Host
Write-Host "Squirrel.Windows packaging finished with only current-version outputs in $OutDir"
