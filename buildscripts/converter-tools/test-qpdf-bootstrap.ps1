[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$ProofRoot)
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'qpdfbootstrap.psm1') -Force
$root = [IO.Path]::GetFullPath($ProofRoot)
if (Test-Path -LiteralPath $root) { throw 'Use a fresh proof directory.' }
[IO.Directory]::CreateDirectory($root) | Out-Null
$cache = Join-Path $root 'cache'
$module = Join-Path $PSScriptRoot 'qpdfbootstrap.psm1'
$runner = Join-Path $root 'bootstrap-child.ps1'
@'
param([string]$Module,[string]$Destination,[string]$Cache,[string]$Mode,[string]$Barrier)
$ErrorActionPreference='Stop'
Import-Module $Module -Force
$hooks=@{}
if($Mode -eq 'interrupt-prior') { $hooks.PriorMoved={ [Environment]::Exit(73) } }
if($Mode -eq 'interrupt-active') { $hooks.Activated={ [Environment]::Exit(74) } }
if($Mode -eq 'hold-lock') { $hooks.Locked={
    [IO.File]::WriteAllText((Join-Path $Barrier 'locked'),'ready')
    $watch=[Diagnostics.Stopwatch]::StartNew()
    while(!(Test-Path -LiteralPath (Join-Path $Barrier 'release'))) {
        if($watch.ElapsedMilliseconds -gt 8000){throw 'Barrier timeout'}
        Start-Sleep -Milliseconds 50
    }
} }
try { Invoke-QpdfBootstrap -DestinationRoot $Destination -CacheRoot $Cache -ForceReinstall:($Mode -like 'interrupt-*') -TestHooks $hooks; exit 0 }
catch { Write-Error $_; exit 1 }
'@ | Set-Content -LiteralPath $runner -Encoding utf8
function Require([bool]$Condition,[string]$Message) { if(!$Condition){throw $Message} }
function Fresh([string]$Name) {
    $destination=Join-Path $root $Name
    Invoke-QpdfBootstrap -DestinationRoot $destination -CacheRoot $cache | Out-Null
    [IO.File]::WriteAllText((Join-Path $destination 'converter-tools/qpdf/prior-marker.txt'),'prior verified bundle')
    return $destination
}
function Preserved([string]$Destination) {
    Require (Test-QpdfBundle (Join-Path $Destination 'converter-tools/qpdf')) 'All original components remain verified'
    Require ((Get-Content -Raw -LiteralPath (Join-Path $Destination 'converter-tools/qpdf/prior-marker.txt')) -ceq 'prior verified bundle') 'The actual prior bundle was retained/restored'
}
function Child([string]$Destination,[string]$Mode,[string]$Barrier=$root) {
    $id=[guid]::NewGuid().ToString('N')
    $arguments=@('-NoProfile','-ExecutionPolicy','Bypass','-File',('"'+$runner+'"'),'-Module',('"'+$module+'"'),'-Destination',('"'+$Destination+'"'),'-Cache',('"'+$cache+'"'),'-Mode',$Mode,'-Barrier',('"'+$Barrier+'"'))
    $shell=Join-Path $PSHOME 'powershell.exe'
    if (!(Test-Path -LiteralPath $shell)) { $shell=Join-Path $PSHOME 'pwsh.exe' }
    $process=Start-Process -FilePath $shell -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $root ($id+'.out.log')) -RedirectStandardError (Join-Path $root ($id+'.err.log'))
    # Cache the native handle before exit; Windows PowerShell can otherwise
    # report a null ExitCode after the child has already released its handle.
    $null=$process.Handle
    return $process
}
function Wait-Child($Process) {
    if(!$Process.WaitForExit(30000)){ $Process.Kill(); throw 'Bootstrap child exceeded 30 seconds' }
    $Process.Refresh()
    return $Process.ExitCode
}
$tests=[ordered]@{
    'cold archive miss and warm verification'={
        $destination=Fresh 'cold'
        Preserved $destination
        Require (@(Get-ChildItem -LiteralPath $cache -Filter '*.zip').Count -eq 1) 'Exactly one verified archive was downloaded'
        Invoke-QpdfBootstrap -DestinationRoot $destination -CacheRoot $cache | Out-Null
        Preserved $destination
    }
    'partial staged copy leaves active bytes intact'={
        $destination=Fresh 'partial'
        $refused=$false
        try { Invoke-QpdfBootstrap -DestinationRoot $destination -CacheRoot $cache -ForceReinstall -TestHooks @{ComponentCopied={param($count)if($count -eq 3){throw 'injected copy interruption'}}} | Out-Null }
        catch { $refused=$_.Exception.Message -like '*injected copy interruption*' }
        Require $refused 'The third-component interruption actually occurred'
        Preserved $destination
    }
    'ordinary activation failure restores prior bundle synchronously'={
        $destination=Fresh 'rollback'
        $refused=$false
        try { Invoke-QpdfBootstrap -DestinationRoot $destination -CacheRoot $cache -ForceReinstall -TestHooks @{PriorMoved={throw 'injected activation interruption'}} | Out-Null }
        catch { $refused=$_.Exception.Message -like '*injected activation interruption*' }
        Require $refused 'The activation barrier actually interrupted'
        Preserved $destination
        Require (!(Test-Path -LiteralPath (Join-Path $destination 'converter-tools/qpdf.activation.json'))) 'Recovery completed its journal'
    }
    'hard interruption after prior rename recovers on next invocation'={
        $destination=Fresh 'crash-prior'
        $process=Child $destination 'interrupt-prior'
        Require ((Wait-Child $process) -eq 73) 'Child terminated at the exact prior-rename barrier'
        Require (!(Test-Path -LiteralPath (Join-Path $destination 'converter-tools/qpdf'))) 'Interrupted active path is absent'
        Invoke-QpdfBootstrap -DestinationRoot $destination -CacheRoot $cache | Out-Null
        Preserved $destination
    }
    'hard interruption after activation retains complete bundle and backup'={
        $destination=Fresh 'crash-active'
        $process=Child $destination 'interrupt-active'
        Require ((Wait-Child $process) -eq 74) 'Child terminated after directory activation'
        Require (Test-QpdfBundle (Join-Path $destination 'converter-tools/qpdf')) 'Activated bundle is complete'
        Invoke-QpdfBootstrap -DestinationRoot $destination -CacheRoot $cache | Out-Null
        $backups=@(Get-ChildItem -LiteralPath (Join-Path $destination 'converter-tools') -Directory -Force -Filter '.qpdf-backup-*')
        Require ($backups.Count -eq 1 -and (Test-QpdfBundle $backups[0].FullName)) 'Previous complete bundle remains recoverable'
        Require ((Get-Content -Raw -LiteralPath (Join-Path $backups[0].FullName 'prior-marker.txt')) -ceq 'prior verified bundle') 'Backup is the actual prior installation'
    }
    'tampered activated component restores independent prior pins'={
        $destination=Fresh 'tampered'
        $refused=$false
        try { Invoke-QpdfBootstrap -DestinationRoot $destination -CacheRoot $cache -ForceReinstall -TestHooks @{Activated={
            [IO.File]::WriteAllText((Join-Path $destination 'converter-tools/qpdf/qpdf30.dll'),'injected invalid component')
        }} | Out-Null } catch { $refused=$_.Exception.Message -like '*Activated qpdf bundle failed verification*' }
        Require $refused 'Activated component tampering was rejected'
        Preserved $destination
        Require (@(Get-ChildItem -LiteralPath (Join-Path $destination 'converter-tools') -Directory -Force -Filter '.qpdf-invalid-*').Count -eq 1) 'Invalid candidate was quarantined without deletion'
    }
    'destination lock refuses bounded contention without mutation'={
        $destination=Fresh 'lock-timeout'
        $held=[IO.File]::Open((Join-Path $destination 'converter-tools/.qpdf-bootstrap.lock'),[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
        $timer=[Diagnostics.Stopwatch]::StartNew();$refused=$false
        try { Invoke-QpdfBootstrap -DestinationRoot $destination -CacheRoot $cache -LockTimeoutMilliseconds 200 | Out-Null }
        catch { $refused=$_.Exception.Message -like '*Timed out waiting*' }
        finally { $held.Dispose() }
        Require ($refused -and $timer.ElapsedMilliseconds -ge 200 -and $timer.ElapsedMilliseconds -lt 3000) 'Actual destination lock timeout was bounded'
        Preserved $destination
    }
    'two simultaneous processes serialize one complete activation'={
        $destination=Join-Path $root 'concurrent'
        $barrier=Join-Path $root 'barrier';[IO.Directory]::CreateDirectory($barrier)|Out-Null
        $first=Child $destination 'hold-lock' $barrier
        $timer=[Diagnostics.Stopwatch]::StartNew()
        while(!(Test-Path -LiteralPath (Join-Path $barrier 'locked'))) {
            Require ($timer.ElapsedMilliseconds -lt 5000) 'First writer acquired its lock';Start-Sleep -Milliseconds 50
        }
        $second=Child $destination 'normal'
        Start-Sleep -Milliseconds 300;$second.Refresh()
        Require (!$second.HasExited) 'Second writer waits while destination lock is held'
        [IO.File]::WriteAllText((Join-Path $barrier 'release'),'release')
        Require ((Wait-Child $first) -eq 0) 'First writer completed'
        Require ((Wait-Child $second) -eq 0) 'Second writer verified the completed activation'
        Require (Test-QpdfBundle (Join-Path $destination 'converter-tools/qpdf')) 'Concurrent activation is complete'
        Require (@(Get-ChildItem -LiteralPath (Join-Path $destination 'converter-tools') -Filter 'qpdf.activation.completed-*.json').Count -eq 1) 'Only one activation happened'
    }
    'malformed recovery journal never selects arbitrary paths'={
        $destination=Fresh 'malformed'
        [IO.File]::WriteAllText((Join-Path $destination 'converter-tools/qpdf.activation.json'),'{"schema":1,"transaction":"bad","stage":"../../unrelated","backup":"../../unrelated","archiveSha256":"bad"}')
        $refused=$false
        try { Invoke-QpdfBootstrap -DestinationRoot $destination -CacheRoot $cache | Out-Null }
        catch { $refused=$_.Exception.Message -like '*Invalid qpdf activation journal fields*' }
        Require $refused 'Malformed journal failed closed'
        Preserved $destination
    }
}
$failed=0
foreach($entry in $tests.GetEnumerator()) {
    try { & $entry.Value; Write-Output ('PASS '+$entry.Key) }
    catch { $failed++;Write-Output ('FAIL '+$entry.Key+': '+$_.Exception.Message) }
}
Write-Output ('Bootstrap cases: '+$tests.Count+' total, '+($tests.Count-$failed)+' passed, '+$failed+' failed')
if($failed){exit 1}
