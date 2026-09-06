[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string] $Root,

    [Parameter(Mandatory = $true)]
    [string] $ToolsDir
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$lockPath = Join-Path $Root 'buildscripts\packaging\Windows\Squirrel\squirrel.lock.json'
$lock = Get-Content -LiteralPath $lockPath -Raw | ConvertFrom-Json
New-Item -ItemType Directory -Force -Path $ToolsDir | Out-Null

function Get-Sha256([string] $Path) {
    $stream = [System.IO.File]::OpenRead($Path)
    try {
        $hasher = [System.Security.Cryptography.SHA256]::Create()
        try {
            return ([System.BitConverter]::ToString($hasher.ComputeHash($stream))).Replace('-', '')
        }
        finally {
            $hasher.Dispose()
        }
    }
    finally {
        $stream.Dispose()
    }
}

function Get-PinnedFile {
    param([string] $Url, [string] $Sha256, [string] $Destination)

    if (Test-Path -LiteralPath $Destination) {
        if ((Get-Sha256 $Destination) -ieq $Sha256) {
            Write-Host "Reusing verified $Destination"
            return
        }
        Remove-Item -LiteralPath $Destination -Force
    }

    $partial = "$Destination.partial"
    Remove-Item -LiteralPath $partial -Force -ErrorAction SilentlyContinue
    try {
        Write-Host "Downloading $Url"
        Invoke-WebRequest -Uri $Url -OutFile $partial -UseBasicParsing -TimeoutSec 120
        $actual = Get-Sha256 $partial
        if ($actual -ine $Sha256) {
            throw "SHA256 mismatch for $Url"
        }
        Move-Item -LiteralPath $partial -Destination $Destination -Force
        Write-Host "Verified SHA256 $actual"
    }
    finally {
        Remove-Item -LiteralPath $partial -Force -ErrorAction SilentlyContinue
    }
}

Get-PinnedFile -Url $lock.nuget.url -Sha256 $lock.nuget.sha256 -Destination (Join-Path $ToolsDir 'nuget.exe')
Get-PinnedFile -Url $lock.squirrel.url -Sha256 $lock.squirrel.sha256 -Destination (Join-Path $ToolsDir ("squirrel.windows.{0}.nupkg" -f $lock.squirrel.version))
