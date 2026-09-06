[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$DestinationRoot)
$ErrorActionPreference = 'Stop'
$lock = Get-Content -Raw (Join-Path $PSScriptRoot 'qpdf.lock.json') | ConvertFrom-Json
$destination = Join-Path $DestinationRoot $lock.installRelativePath
$folder = Split-Path -Parent $destination
function Test-Bundle([string]$Root) {
    foreach ($entry in $lock.files.psobject.Properties) {
        $path = Join-Path $Root $entry.Name
        if (!(Test-Path -LiteralPath $path -PathType Leaf)) { return $false }
        if ((Get-Item -LiteralPath $path).Attributes -band [IO.FileAttributes]::ReparsePoint) { return $false }
        if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $entry.Value) { return $false }
    }
    return $true
}
if (Test-Bundle $folder) { Write-Output "Verified all 10 pinned qpdf components at $destination"; exit 0 }
$temporary = Join-Path ([IO.Path]::GetTempPath()) ("audacity-qpdf-" + [guid]::NewGuid().ToString('N') + '.zip')
$stage = Join-Path ([IO.Path]::GetTempPath()) ("audacity-qpdf-" + [guid]::NewGuid().ToString('N'))
try {
    Invoke-WebRequest -Uri $lock.source -OutFile $temporary -UseBasicParsing
    if ((Get-FileHash -LiteralPath $temporary -Algorithm SHA256).Hash.ToLowerInvariant() -ne $lock.archiveSha256) {
        throw 'qpdf archive SHA-256 verification failed.'
    }
    Expand-Archive -LiteralPath $temporary -DestinationPath $stage
    $bin = Join-Path $stage 'qpdf-12.3.2-msvc64/bin'
    if (!(Test-Bundle $bin)) { throw 'The extracted qpdf executable or DLL failed pinned verification.' }
    New-Item -ItemType Directory -Force -Path $folder | Out-Null
    foreach ($entry in $lock.files.psobject.Properties) {
        Copy-Item -LiteralPath (Join-Path $bin $entry.Name) -Destination (Join-Path $folder $entry.Name) -Force
    }
    # Retain the official notices beside the installed components.
    $distribution = Split-Path -Parent $bin
    foreach ($notice in @('README.md', 'NOTICE.md', 'LICENSE.txt', 'Artistic-2.0', 'LICENSE')) {
        $noticePath = Join-Path $distribution $notice
        if (Test-Path -LiteralPath $noticePath -PathType Leaf) { Copy-Item -LiteralPath $noticePath -Destination $folder -Force }
    }
    if (!(Test-Bundle $folder)) { throw 'Installed qpdf bundle failed final component verification.' }
    Write-Output "Installed all 10 pinned qpdf components at $destination"
} finally {
    if (Test-Path -LiteralPath $temporary) { Remove-Item -LiteralPath $temporary -Force }
    # These names were generated for this invocation, outside the destination.
    $resolvedStage = [IO.Path]::GetFullPath($stage)
    $temporaryRoot = [IO.Path]::GetFullPath([IO.Path]::GetTempPath())
    if (!$resolvedStage.StartsWith($temporaryRoot, [StringComparison]::OrdinalIgnoreCase)) { throw 'Unexpected staging cleanup path.' }
    if (Test-Path -LiteralPath $resolvedStage) { Remove-Item -LiteralPath $resolvedStage -Recurse -Force }
}
