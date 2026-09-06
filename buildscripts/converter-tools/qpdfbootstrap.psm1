Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression

function Get-QpdfLock {
    $lock = Get-Content -Raw (Join-Path $PSScriptRoot 'qpdf.lock.json') | ConvertFrom-Json
    if ($lock.version -ne '12.3.2' -or @($lock.files.psobject.Properties).Count -ne 10) { throw 'Invalid qpdf lock inventory.' }
    return $lock
}

function Assert-QpdfDirectory([string]$Path) {
    $absolute = [IO.Path]::GetFullPath($Path)
    $cursor = $absolute
    while ($cursor) {
        if (Test-Path -LiteralPath $cursor) {
            $item = Get-Item -LiteralPath $cursor -Force
            if (!$item.PSIsContainer -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
                throw 'qpdf provisioning requires ordinary directories without reparse points.'
            }
        }
        $next = Split-Path -Parent $cursor
        if ($next -eq $cursor) { break }
        $cursor = $next
    }
    [IO.Directory]::CreateDirectory($absolute) | Out-Null
    return $absolute
}

function Test-QpdfBundle([string]$Root, $Lock = (Get-QpdfLock)) {
    if (!(Test-Path -LiteralPath $Root -PathType Container)) { return $false }
    if ((Get-Item -LiteralPath $Root -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) { return $false }
    foreach ($entry in $Lock.files.psobject.Properties) {
        $path = Join-Path $Root $entry.Name
        if (!(Test-Path -LiteralPath $path -PathType Leaf)) { return $false }
        if ((Get-Item -LiteralPath $path -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) { return $false }
        if ((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $entry.Value) { return $false }
    }
    return $true
}

function Enter-QpdfLock([string]$Path, [int]$TimeoutMilliseconds) {
    if (Test-Path -LiteralPath $Path) {
        if ((Get-Item -LiteralPath $Path -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'The qpdf lock path is a reparse point.' }
    }
    $timer = [Diagnostics.Stopwatch]::StartNew()
    do {
        try { return [IO.File]::Open($Path, [IO.FileMode]::OpenOrCreate, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None) }
        catch [IO.IOException] {
            if ($timer.ElapsedMilliseconds -ge $TimeoutMilliseconds) { throw 'Timed out waiting for the qpdf destination/cache lock.' }
            Start-Sleep -Milliseconds 100
        }
    } while ($true)
}

function Move-QpdfDirectory([string]$Source, [string]$Destination) {
    # A loaded qpdf bundle holds native directory pins. Retry sharing/lock
    # violations briefly, without treating access errors as transient.
    for ($attempt = 0; $attempt -lt 10; $attempt++) {
        try { [IO.Directory]::Move($Source, $Destination); return }
        catch [IO.IOException] {
            $code = $_.Exception.HResult -band 0xffff
            if ($code -notin @(32, 33) -or $attempt -eq 9) { throw }
            Start-Sleep -Milliseconds 100
        }
    }
}

function Write-QpdfJournal([string]$Path, $Value) {
    $temporary = $Path + '.' + [guid]::NewGuid().ToString('N') + '.tmp'
    $bytes = [Text.UTF8Encoding]::new($false).GetBytes(($Value | ConvertTo-Json -Compress))
    $stream = [IO.File]::Open($temporary, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
    try { $stream.Write($bytes, 0, $bytes.Length); $stream.Flush($true) } finally { $stream.Dispose() }
    [IO.File]::Move($temporary, $Path)
}

function Complete-QpdfJournal([string]$Path, [string]$Transaction) {
    # Preserve the completed record rather than deleting a path after activation.
    [IO.File]::Move($Path, (Join-Path (Split-Path -Parent $Path) ('qpdf.activation.completed-' + $Transaction + '.json')))
}

function Repair-QpdfActivation([string]$Parent, $Lock) {
    $journalPath = Join-Path $Parent 'qpdf.activation.json'
    if (!(Test-Path -LiteralPath $journalPath)) { return }
    $journalFile = Get-Item -LiteralPath $journalPath -Force
    if ($journalFile.Length -gt 4096 -or ($journalFile.Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw 'Invalid qpdf activation journal.' }
    $journal = Get-Content -Raw -LiteralPath $journalPath | ConvertFrom-Json
    if ($journal.schema -ne 1 -or $journal.transaction -notmatch '^[a-f0-9]{32}$' -or
        $journal.stage -cne ('.qpdf-stage-' + $journal.transaction) -or
        $journal.backup -cne ('.qpdf-backup-' + $journal.transaction) -or
        $journal.archiveSha256 -cne $Lock.archiveSha256) { throw 'Invalid qpdf activation journal fields.' }
    $active = Join-Path $Parent 'qpdf'
    $backup = Join-Path $Parent $journal.backup
    $stage = Join-Path $Parent $journal.stage
    if (Test-QpdfBundle $active $Lock) {
        Complete-QpdfJournal $journalPath $journal.transaction
        Write-Output 'Recovered activation record: the active bundle is complete and verified.'
        return
    }
    $candidate = $null
    if (Test-QpdfBundle $backup $Lock) { $candidate = $backup }
    elseif (Test-QpdfBundle $stage $Lock) { $candidate = $stage }
    if (!$candidate) { throw 'Interrupted qpdf activation has no verified recovery candidate; all paths were retained.' }
    if (Test-Path -LiteralPath $active) {
        $quarantine = Join-Path $Parent ('.qpdf-invalid-' + [guid]::NewGuid().ToString('N'))
        Move-QpdfDirectory $active $quarantine
    }
    Move-QpdfDirectory $candidate $active
    if (!(Test-QpdfBundle $active $Lock)) { throw 'Restored qpdf bundle failed verification.' }
    Complete-QpdfJournal $journalPath $journal.transaction
    Write-Output 'Recovered the prior verified qpdf bundle or the verified first-install stage.'
}

function Get-QpdfArchive([string]$CacheRoot, $Lock, [int]$LockTimeoutMilliseconds) {
    $cache = Assert-QpdfDirectory $CacheRoot
    $archive = Join-Path $cache ('qpdf-' + $Lock.version + '-' + $Lock.archiveSha256 + '.zip')
    $cacheLock = Enter-QpdfLock ($archive + '.lock') $LockTimeoutMilliseconds
    try {
        if (Test-Path -LiteralPath $archive -PathType Leaf) {
            if ((Get-Item -LiteralPath $archive -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) { throw 'The qpdf archive cache is a reparse point.' }
            if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant() -ceq $Lock.archiveSha256) {
                Write-Verbose 'Using the independently verified cached qpdf archive.'
                return $archive
            }
            [IO.File]::Move($archive, ($archive + '.invalid-' + [guid]::NewGuid().ToString('N')))
        }
        $download = $archive + '.download-' + [guid]::NewGuid().ToString('N')
        Invoke-WebRequest -Uri $Lock.source -OutFile $download -UseBasicParsing -TimeoutSec 120
        if ((Get-FileHash -LiteralPath $download -Algorithm SHA256).Hash.ToLowerInvariant() -cne $Lock.archiveSha256) {
            throw 'qpdf archive SHA-256 verification failed; the download was retained and not activated.'
        }
        [IO.File]::Move($download, $archive)
        return $archive
    } finally { $cacheLock.Dispose() }
}

function Invoke-QpdfBootstrap {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string]$DestinationRoot,
        [Parameter(Mandatory = $true)][string]$CacheRoot,
        [switch]$ForceReinstall,
        [hashtable]$TestHooks = @{},
        [ValidateRange(0, 30000)][int]$LockTimeoutMilliseconds = 30000
    )
    $lock = Get-QpdfLock
    $parent = Assert-QpdfDirectory (Join-Path ([IO.Path]::GetFullPath($DestinationRoot)) 'converter-tools')
    $active = Join-Path $parent 'qpdf'
    $destinationLock = Enter-QpdfLock (Join-Path $parent '.qpdf-bootstrap.lock') $LockTimeoutMilliseconds
    try {
        if ($TestHooks.ContainsKey('Locked')) { & $TestHooks.Locked }
        Repair-QpdfActivation $parent $lock
        if (!$ForceReinstall -and (Test-QpdfBundle $active $lock)) {
            Write-Output "Verified all 10 pinned qpdf components at $active"
            return
        }
        $archive = Get-QpdfArchive $CacheRoot $lock $LockTimeoutMilliseconds
        $transaction = [guid]::NewGuid().ToString('N')
        $stageName = '.qpdf-stage-' + $transaction
        $backupName = '.qpdf-backup-' + $transaction
        $stage = Join-Path $parent $stageName
        $backup = Join-Path $parent $backupName
        [IO.Directory]::CreateDirectory($stage) | Out-Null
        # Read only the ten exact archive entries. Hold and hash the same archive
        # stream used by ZipArchive, so a cache pathname replacement cannot
        # switch the source between verification and extraction.
        $archiveStream = [IO.File]::Open($archive, [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
        $zip = $null
        try {
            $sha = [Security.Cryptography.SHA256]::Create()
            try { $actual = [BitConverter]::ToString($sha.ComputeHash($archiveStream)).Replace('-', '').ToLowerInvariant() }
            finally { $sha.Dispose() }
            if ($actual -cne $lock.archiveSha256) { throw 'The held qpdf archive failed pinned verification.' }
            $archiveStream.Position = 0
            $zip = [IO.Compression.ZipArchive]::new($archiveStream, [IO.Compression.ZipArchiveMode]::Read, $true)
            $copied = 0
            foreach ($entry in $lock.files.psobject.Properties) {
                $member = $zip.GetEntry('qpdf-12.3.2-msvc64/bin/' + $entry.Name)
                if (!$member -or $member.Length -gt 268435456) { throw 'The verified archive lacks a bounded required qpdf component.' }
                $input = $member.Open()
                $output = [IO.File]::Open((Join-Path $stage $entry.Name), [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
                try { $input.CopyTo($output); $output.Flush($true) }
                finally { $output.Dispose(); $input.Dispose() }
                $copied++
                if ($TestHooks.ContainsKey('ComponentCopied')) { & $TestHooks.ComponentCopied $copied }
            }
        } finally { if ($zip) { $zip.Dispose() }; $archiveStream.Dispose() }
        if (!(Test-QpdfBundle $stage $lock)) { throw 'The staged qpdf bundle failed pinned verification; active files were untouched.' }
        if ($TestHooks.ContainsKey('StageVerified')) { & $TestHooks.StageVerified $stage }
        $journalPath = Join-Path $parent 'qpdf.activation.json'
        Write-QpdfJournal $journalPath ([ordered]@{schema=1; transaction=$transaction; stage=$stageName; backup=$backupName; archiveSha256=$lock.archiveSha256})
        try {
            if (Test-Path -LiteralPath $active) { Move-QpdfDirectory $active $backup }
            if ($TestHooks.ContainsKey('PriorMoved')) { & $TestHooks.PriorMoved }
            Move-QpdfDirectory $stage $active
            if ($TestHooks.ContainsKey('Activated')) { & $TestHooks.Activated }
            if (!(Test-QpdfBundle $active $lock)) { throw 'Activated qpdf bundle failed verification.' }
            Complete-QpdfJournal $journalPath $transaction
        } catch {
            # Recover synchronously for ordinary errors. A terminated process
            # leaves the same bounded journal for the next invocation to recover.
            Repair-QpdfActivation $parent $lock
            throw
        }
        Write-Output "Installed all 10 pinned qpdf components atomically at $active"
        if (Test-Path -LiteralPath $backup) { Write-Output "Retained the previous bundle at $backup" }
        # Failed stages are retained as diagnostic/recovery data.
        # They are never deleted recursively during provisioning.
    } finally { $destinationLock.Dispose() }
}
Export-ModuleMember -Function Invoke-QpdfBootstrap, Test-QpdfBundle
