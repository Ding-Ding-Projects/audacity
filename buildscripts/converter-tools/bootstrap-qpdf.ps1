[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$DestinationRoot)
$ErrorActionPreference = 'Stop'
$lock = Get-Content -Raw (Join-Path $PSScriptRoot 'qpdf.lock.json') | ConvertFrom-Json
$destination = Join-Path $DestinationRoot $lock.installRelativePath
$folder = Split-Path -Parent $destination
New-Item -ItemType Directory -Force -Path $folder | Out-Null
if (Test-Path -LiteralPath $destination) {
    $actual = (Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -eq $lock.binarySha256 -and (Test-Path -LiteralPath (Join-Path $folder 'qpdf30.dll'))) { Write-Output "Verified bundled qpdf at $destination"; exit 0 }
    Remove-Item -LiteralPath $destination -Force
}
$temporary = Join-Path ([System.IO.Path]::GetTempPath()) ("audacity-qpdf-" + [guid]::NewGuid().ToString('N') + '.zip')
$stage = Join-Path ([System.IO.Path]::GetTempPath()) ("audacity-qpdf-" + [guid]::NewGuid().ToString('N'))
try {
    Invoke-WebRequest -Uri $lock.source -OutFile $temporary -UseBasicParsing
    $actual = (Get-FileHash -LiteralPath $temporary -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $lock.archiveSha256) { throw "qpdf archive SHA-256 verification failed." }
    Expand-Archive -LiteralPath $temporary -DestinationPath $stage
    $binary = Get-ChildItem -LiteralPath $stage -Filter qpdf.exe -Recurse | Select-Object -First 1
    if ($null -eq $binary) { throw "The verified qpdf archive did not contain qpdf.exe." }
    $actual = (Get-FileHash -LiteralPath $binary.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $lock.binarySha256) { throw "Extracted qpdf binary SHA-256 verification failed." }
    # qpdf's official portable release carries its DLL See Futs beside qpdf.exe.
    # Copy that exact bin directory only after both the release archive and
    # executable hashes have validated.
    Get-ChildItem -LiteralPath $binary.Directory.FullName -Force | Copy-Item -Destination $folder -Recurse -Force
    Write-Output "Installed verified bundled qpdf at $destination"
} finally {
    if (Test-Path -LiteralPath $temporary) { Remove-Item -LiteralPath $temporary -Force }
    if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
}
