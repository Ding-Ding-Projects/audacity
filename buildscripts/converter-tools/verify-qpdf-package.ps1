[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$PackagePath,
    [string]$BundlePrefix = 'lib/net45/bin/converter-tools/qpdf/'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
Add-Type -AssemblyName System.IO.Compression
$lock = Get-Content -Raw -LiteralPath (Join-Path $PSScriptRoot 'qpdf.lock.json') | ConvertFrom-Json
$stream = [IO.File]::Open([IO.Path]::GetFullPath($PackagePath), [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
$zip = $null
try {
    $zip = [IO.Compression.ZipArchive]::new($stream, [IO.Compression.ZipArchiveMode]::Read, $true)
    $entries = @($zip.Entries | Where-Object {
        $name=$_.FullName.Replace('\','/')
        if ($name -match '(^|/)converter-tools/(\.qpdf-|qpdf\.activation)') {
            throw 'A qpdf bootstrap recovery record must not be shipped inside the application package.'
        }
        $name.StartsWith($BundlePrefix, [StringComparison]::OrdinalIgnoreCase) -and $name -cne $BundlePrefix
    })
    $expected = @($lock.files.psobject.Properties.Name)
    if ($entries.Count -ne $expected.Count -or $entries.Count -ne 10) { throw 'Package does not contain exactly ten qpdf runtime components.' }
    $seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($entry in $entries) {
        $name = $entry.FullName.Replace('\','/').Substring($BundlePrefix.Length)
        $mode = ($entry.ExternalAttributes -shr 16) -band 0xf000
        if ($expected -cnotcontains $name -or !$seen.Add($name) -or
            ($entry.ExternalAttributes -band 0x410) -ne 0 -or $mode -notin @(0, 0x8000)) {
            throw 'Package contains an unexpected, duplicate, directory, or reparse qpdf entry.'
        }
        $input = $entry.Open()
        $sha = [Security.Cryptography.SHA256]::Create()
        try { $actual = [BitConverter]::ToString($sha.ComputeHash($input)).Replace('-', '').ToLowerInvariant() }
        finally { $sha.Dispose(); $input.Dispose() }
        if ($actual -cne $lock.files.$name) { throw "Packaged qpdf component failed its independent hash: $name" }
        Write-Output "Verified packaged component: $name"
    }
    Write-Output "Verified exactly ten pinned qpdf components in $PackagePath"
} finally { if ($zip) { $zip.Dispose() }; $stream.Dispose() }
