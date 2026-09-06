<#
.SYNOPSIS
    Exercise the Squirrel output-isolation contract against an existing build.install tree.

.DESCRIPTION
    Builds a baseline package, then creates a second package using that baseline
    for delta generation while deliberately pre-populating the second output
    directory with the baseline full package. The second result must publish
    only its own full package, own delta package, own RELEASES entries, Setup,
    checksums, and manifest. No app rebuild is performed.
#>

[CmdletBinding()]
param(
    [string] $InstallDir = "build.install",
    [string] $ToolsDir = "build.tools\\squirrel-regression"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\\..\\..")).Path
if (-not [IO.Path]::IsPathRooted($InstallDir)) { $InstallDir = Join-Path $repoRoot $InstallDir }
if (-not [IO.Path]::IsPathRooted($ToolsDir)) { $ToolsDir = Join-Path $repoRoot $ToolsDir }
if (-not (Test-Path -LiteralPath (Join-Path $InstallDir "bin\\Audacity4.exe"))) {
    throw "Regression requires an existing built application tree: $InstallDir"
}

$packageScript = Join-Path $PSScriptRoot "package_squirrel.ps1"
$scratch = Join-Path $repoRoot ("build\\squirrel-output-regression-" + [guid]::NewGuid().ToString("N"))
$baseline = Join-Path $scratch "baseline"
$candidate = Join-Path $scratch "candidate"
New-Item -ItemType Directory -Force -Path $baseline,$candidate | Out-Null

& $packageScript -InstallDir $InstallDir -OutDir $baseline -ToolsDir $ToolsDir -Version "4.0.0-ci000901"
if ($LASTEXITCODE -ne 0) { throw "Baseline Squirrel packaging failed with exit code $LASTEXITCODE" }

$baselineFull = Join-Path $baseline "Audacity-4.0.0-ci000901-full.nupkg"
if (-not (Test-Path -LiteralPath $baselineFull)) { throw "Baseline full package was not produced" }
Copy-Item -LiteralPath $baselineFull -Destination $candidate -Force

& $packageScript -InstallDir $InstallDir -OutDir $candidate -ToolsDir $ToolsDir `
    -PreviousReleasesDir $baseline -Version "4.0.0-ci000902"
if ($LASTEXITCODE -ne 0) { throw "Seeded Squirrel packaging failed with exit code $LASTEXITCODE" }

$expectedFiles = @(
    "Audacity-4.0.0-ci000902-full.nupkg",
    "Audacity-4.0.0-ci000902-delta.nupkg",
    "Setup.exe",
    "RELEASES",
    "SHA256SUMS",
    "package-output-manifest.json"
)
$actualFiles = @(Get-ChildItem -LiteralPath $candidate -File | ForEach-Object Name | Sort-Object)
if (@(Compare-Object -ReferenceObject ($expectedFiles | Sort-Object) -DifferenceObject $actualFiles).Count -ne 0) {
    throw "Published output contained stale or missing files: $($actualFiles -join ', ')"
}

$manifest = Get-Content -LiteralPath (Join-Path $candidate "package-output-manifest.json") -Raw | ConvertFrom-Json
if ($manifest.packageVersion -ne "4.0.0-ci000902" -or $manifest.seedMode -ne "baseline-used-for-delta-only") {
    throw "Output manifest did not describe the current seeded package truthfully"
}

foreach ($line in Get-Content -LiteralPath (Join-Path $candidate "RELEASES")) {
    $parts = $line -split '\s+'
    if ($parts.Count -ne 3 -or -not (Test-Path -LiteralPath (Join-Path $candidate $parts[1]))) {
        throw "Published RELEASES references an absent package: $line"
    }
    if ($parts[1] -like "*000901*") {
        throw "Published RELEASES leaked a baseline package reference: $line"
    }
}

Write-Host "Squirrel output-isolation regression passed: $candidate"
