<# Behavioral tests of publication boundaries using small byte fixtures.
   These do not claim Squirrel package validity or installed-client behavior. #>
[CmdletBinding()]
param([string] $EvidenceRoot = '')
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'squirrel_output.ps1')
if (-not $EvidenceRoot) {
    $repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
    $EvidenceRoot = Join-Path $repoRoot ('build/squirrel-transaction-tests-' + [guid]::NewGuid().ToString('N'))
}
if (Test-Path -LiteralPath $EvidenceRoot) { throw 'Tests require a new evidence directory.' }
[IO.Directory]::CreateDirectory($EvidenceRoot) | Out-Null
$script:results = @()
function Check([bool] $Condition, [string] $Message) { if (-not $Condition) { throw $Message } }
function Reject([scriptblock] $Action, [string] $Pattern) {
    $caught = $null
    try { & $Action | Out-Null } catch { $caught = $_.Exception.Message }
    if (-not $caught -or $caught -notmatch $Pattern) { throw "Expected rejection /$Pattern/, got: $caught" }
}
function Case([string] $Name, [scriptblock] $Action) {
    & $Action
    $script:results += [ordered]@{ name=$Name; status='passed' }
    Write-Host "PASS $Name"
}
function New-Fixture([string] $Name, [string] $Version = '4.0.0-ci000901') {
    $dir = Join-Path $EvidenceRoot $Name
    [IO.Directory]::CreateDirectory($dir) | Out-Null
    $package = "Audacity-$Version-full.nupkg"
    [IO.File]::WriteAllText((Join-Path $dir $package), "fixture package $Version")
    [IO.File]::WriteAllText((Join-Path $dir 'Setup.exe'), "fixture setup $Version")
    $sha1 = (Get-FileHash -LiteralPath (Join-Path $dir $package) -Algorithm SHA1).Hash
    $size = (Get-Item -LiteralPath (Join-Path $dir $package)).Length
    [IO.File]::WriteAllText((Join-Path $dir 'RELEASES'), "$sha1 $package $size`n")
    $files = @(@('Setup.exe','RELEASES',$package) | Sort-Object | ForEach-Object {
        $path = Join-Path $dir $_
        [ordered]@{ name=$_; bytes=(Get-Item -LiteralPath $path).Length; sha256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() }
    })
    $manifest = [ordered]@{ schemaVersion=1; packageId='Audacity'; packageVersion=$Version; seedMode='no-baseline';
        files=$files; releaseEntries=@([ordered]@{name=$package; bytes=$size; sha1=$sha1.ToLowerInvariant()}) }
    $manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $dir 'package-output-manifest.json') -Encoding utf8
    $files | ForEach-Object { '{0}  {1}' -f $_.sha256, $_.name } | Set-Content -LiteralPath (Join-Path $dir 'SHA256SUMS') -Encoding ascii
    return $dir
}
function Fingerprint([string] $Directory) {
    return (@(Get-ChildItem -LiteralPath $Directory -File -Force | Sort-Object Name | ForEach-Object {
        $_.Name + ':' + (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    }) -join '|')
}
function Recover([string] $Directory) {
    $lock = Open-SquirrelOutputLock $Directory
    try { Restore-SquirrelOutputTransaction $Directory } finally { $lock.Dispose() }
}

$candidate = New-Fixture 'candidate' '4.0.0-ci000902'
Case 'valid exact manifest and feed' { $null = Assert-SquirrelOutput $candidate '4.0.0-ci000902' 'Audacity' }
Case 'unknown nupkg without ownership manifest is retained' {
    $dir = Join-Path $EvidenceRoot 'unknown-only'
    [IO.Directory]::CreateDirectory($dir) | Out-Null
    [IO.File]::WriteAllText((Join-Path $dir 'notes.nupkg'), 'unrelated work')
    $before = Fingerprint $dir
    Reject { Publish-SquirrelOutput $candidate $dir } 'manifest|cannot find'
    Check ((Fingerprint $dir) -ceq $before) 'Unmanaged output was modified.'
}
Case 'unknown nupkg beside a valid manifest is retained' {
    $dir = New-Fixture 'unknown-extra'
    [IO.File]::WriteAllText((Join-Path $dir 'notes.nupkg'), 'unrelated work')
    $before = Fingerprint $dir
    Reject { Publish-SquirrelOutput $candidate $dir } 'unmanaged'
    Check ((Fingerprint $dir) -ceq $before) 'Unmanaged file or prior output was modified.'
}
Case 'modified previously managed bytes are retained' {
    $dir = New-Fixture 'modified-prior'
    Add-Content -LiteralPath (Join-Path $dir 'Setup.exe') 'user edit'
    $before = Fingerprint $dir
    Reject { Publish-SquirrelOutput $candidate $dir } 'SHA256 mismatch'
    Check ((Fingerprint $dir) -ceq $before) 'Modified prior output was lost.'
}
Case 'valid previous generation is retained byte for byte' {
    $dir = New-Fixture 'replace-valid'
    $before = Fingerprint $dir
    Publish-SquirrelOutput $candidate $dir
    $null = Assert-SquirrelOutput $dir '4.0.0-ci000902'
    $backups = @(Get-ChildItem -LiteralPath $EvidenceRoot -Directory -Force | Where-Object Name -like '.replace-valid.publish.*.previous')
    Check ($backups.Count -eq 1 -and (Fingerprint $backups[0].FullName) -ceq $before) 'Prior generation was not retained.'
}
foreach ($boundary in @('staged','journaled','previous-renamed','candidate-renamed')) {
    Case "recover interrupted activation at $boundary" {
        $dir = New-Fixture "interrupt-$boundary"
        $before = Fingerprint $dir
        $hook = { param($phase) if ($phase -ceq $boundary) { throw "simulated interruption $phase" } }.GetNewClosure()
        Reject { Publish-SquirrelOutput $candidate $dir $hook } 'simulated interruption'
        Recover $dir
        if ($boundary -eq 'candidate-renamed') { $null = Assert-SquirrelOutput $dir '4.0.0-ci000902' }
        else { Check ((Fingerprint $dir) -ceq $before) 'Recovery did not restore the previous generation.' }
        Publish-SquirrelOutput $candidate $dir
        $null = Assert-SquirrelOutput $dir '4.0.0-ci000902'
    }
}
Case 'new output recovers interrupted candidate activation' {
    $dir = Join-Path $EvidenceRoot 'new-output'
    Reject { Publish-SquirrelOutput $candidate $dir { param($phase) if ($phase -eq 'previous-renamed') { throw 'interrupted new output' } } } 'interrupted'
    Recover $dir
    $null = Assert-SquirrelOutput $dir '4.0.0-ci000902'
}
foreach ($kind in @('hash','size','traversal','rooted','subpath','backslash','drive','duplicate')) {
    Case "reject invalid seed $kind" {
        $dir = New-Fixture "seed-$kind"
        $path = Join-Path $dir 'RELEASES'
        $parts = (Get-Content -LiteralPath $path -Raw).Trim() -split '\s+'
        switch ($kind) {
            hash { $parts[0] = '0' * 40 }
            size { $parts[2] = '999999' }
            traversal { $parts[1] = '../outside-full.nupkg' }
            rooted { $parts[1] = '/outside-full.nupkg' }
            subpath { $parts[1] = 'sub/outside-full.nupkg' }
            backslash { $parts[1] = '..\outside-full.nupkg' }
            drive { $parts[1] = 'C:outside-full.nupkg' }
        }
        $line = $parts -join ' '
        if ($kind -eq 'duplicate') { $line += "`n" + $parts[0] + ' ' + $parts[1].ToUpperInvariant() + ' ' + $parts[2] }
        Set-Content -LiteralPath $path -Value $line
        $before = Fingerprint $dir
        Reject { Read-ReleaseEntries $path } 'mismatch|Unsafe|duplicate'
        Check ((Fingerprint $dir) -ceq $before) 'Rejected seed was modified.'
    }
}
foreach ($kind in @('version','files','feed','checksums')) {
    Case "release collector rejects manifest $kind mismatch before copying" {
        $dir = New-Fixture "manifest-$kind"
        $path = Join-Path $dir 'package-output-manifest.json'
        $manifest = Get-Content -LiteralPath $path -Raw | ConvertFrom-Json
        switch ($kind) {
            version { $manifest.packageVersion = '4.0.0-ci000999' }
            files { $manifest.files[0].bytes = 999 }
            feed { $manifest.releaseEntries[0].sha1 = '0' * 40 }
            checksums { Set-Content -LiteralPath (Join-Path $dir 'SHA256SUMS') -Value 'wrong checksums' }
        }
        $manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $path
        $dest = Join-Path $EvidenceRoot "collected-$kind"
        Reject { Copy-SquirrelReleaseAssets $dir $dest '4.0.0-ci000901' } 'mismatch|differs|does not match'
        Check (-not (Test-Path -LiteralPath $dest)) 'Invalid output was partially collected.'
    }
}
Case 'collector retains package checksums separately from release checksums' {
    $dest = Join-Path $EvidenceRoot 'collected-valid'
    Copy-SquirrelReleaseAssets $candidate $dest '4.0.0-ci000902'
    $null = Assert-SquirrelOutput $dest '4.0.0-ci000902' 'Audacity' -ChecksumName 'PACKAGE-SHA256SUMS'
    Check ((Get-FileHash -LiteralPath (Join-Path $candidate 'SHA256SUMS')).Hash -ceq
        (Get-FileHash -LiteralPath (Join-Path $dest 'PACKAGE-SHA256SUMS')).Hash) 'Package checksums changed during collection.'
}
Case 'malformed journal fails closed without changing prior output' {
    $dir = New-Fixture 'malformed-journal'
    $before = Fingerprint $dir
    Set-Content -LiteralPath (Join-Path $EvidenceRoot '.malformed-journal.publish.json') -Value '{'
    Reject { Publish-SquirrelOutput $candidate $dir } 'JSON|convert'
    Check ((Fingerprint $dir) -ceq $before) 'Malformed journal damaged existing output.'
}
Case 'second process cannot activate while the output lock is held' {
    $dir = New-Fixture 'concurrent'
    $child = Join-Path $EvidenceRoot 'lock-child.ps1'
    @'
param($Helper, $Candidate, $Output)
$ErrorActionPreference = 'Stop'
. $Helper
try { Publish-SquirrelOutput $Candidate $Output; exit 2 }
catch { if ($_.Exception.Message -match 'locked by another publisher') { exit 0 }; throw }
'@ | Set-Content -LiteralPath $child
    $before = Fingerprint $dir
    $lock = Open-SquirrelOutputLock $dir
    try {
        $args = @('-NoProfile','-File',$child,(Join-Path $PSScriptRoot 'squirrel_output.ps1'),$candidate,$dir)
        $quoted = ($args | ForEach-Object { '"' + $_ + '"' }) -join ' '
        $process = Start-Process -FilePath (Get-Process -Id $PID).Path -ArgumentList $quoted -WindowStyle Hidden -PassThru
        if (-not $process.WaitForExit(15000)) { $process.Kill(); throw 'Concurrent publisher did not terminate within 15 seconds.' }
        Check ($process.ExitCode -eq 0) "Second publisher exit code: $($process.ExitCode)"
    } finally { $lock.Dispose() }
    Check ((Fingerprint $dir) -ceq $before) 'Concurrent publisher modified the output.'
    Publish-SquirrelOutput $candidate $dir
}
Case 'recover after publisher process terminates between directory renames' {
    $dir = New-Fixture 'terminated-publisher'
    $child = Join-Path $EvidenceRoot 'terminate-child.ps1'
    $ready = Join-Path $EvidenceRoot 'terminate-child.ready'
    @'
param($Helper, $Candidate, $Output, $Ready)
$ErrorActionPreference = 'Stop'
. $Helper
Publish-SquirrelOutput $Candidate $Output {
    param($phase)
    if ($phase -eq 'previous-renamed') {
        [IO.File]::WriteAllText($Ready, 'previous-renamed')
        Start-Sleep -Seconds 30
    }
}
'@ | Set-Content -LiteralPath $child
    $before = Fingerprint $dir
    $args = @('-NoProfile','-File',$child,(Join-Path $PSScriptRoot 'squirrel_output.ps1'),$candidate,$dir,$ready)
    $quoted = ($args | ForEach-Object { '"' + $_ + '"' }) -join ' '
    $process = Start-Process -FilePath (Get-Process -Id $PID).Path -ArgumentList $quoted -WindowStyle Hidden -PassThru
    try {
        $deadline = [DateTime]::UtcNow.AddSeconds(15)
        while (-not (Test-Path -LiteralPath $ready) -and [DateTime]::UtcNow -lt $deadline -and -not $process.HasExited) {
            Start-Sleep -Milliseconds 100
        }
        Check (Test-Path -LiteralPath $ready) 'Child did not reach the rename boundary.'
        Check (-not (Test-Path -LiteralPath $dir)) 'Child did not move the prior directory.'
        $process.Kill()
        Check ($process.WaitForExit(5000)) 'Owned child did not terminate.'
        Recover $dir
        Check ((Fingerprint $dir) -ceq $before) 'Process termination recovery lost prior bytes.'
        Publish-SquirrelOutput $candidate $dir
        $null = Assert-SquirrelOutput $dir '4.0.0-ci000902'
    } finally {
        if (-not $process.HasExited) { $process.Kill(); $null = $process.WaitForExit(5000) }
        $process.Dispose()
    }
}

[ordered]@{ scope='Synthetic byte fixtures, not installed-client update verification'; passed=$results.Count; results=$results } |
    ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $EvidenceRoot 'results.json')
Write-Host "Passed $($results.Count) publication boundary tests. Evidence retained: $EvidenceRoot"
