[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$BundleRoot,[Parameter(Mandatory=$true)][string]$ProofRoot)
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.IO.Compression
$root=[IO.Path]::GetFullPath($ProofRoot)
if(Test-Path -LiteralPath $root){throw 'Use a fresh package-verifier proof directory.'}
[IO.Directory]::CreateDirectory($root)|Out-Null
$bundle=[IO.Path]::GetFullPath($BundleRoot)
$verifier=Join-Path $PSScriptRoot 'verify-qpdf-package.ps1'
$lock=Get-Content -Raw -LiteralPath (Join-Path $PSScriptRoot 'qpdf.lock.json')|ConvertFrom-Json
$cases=@('exact','missing','extra-dll','extra-directory','tampered','administrative','symlink')
$failed=0
foreach($case in $cases){
    $path=Join-Path $root ($case+'.zip')
    $stream=[IO.File]::Open($path,[IO.FileMode]::CreateNew,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
    $zip=[IO.Compression.ZipArchive]::new($stream,[IO.Compression.ZipArchiveMode]::Create,$true)
    try {
        foreach($component in $lock.files.psobject.Properties){
            if($case -eq 'missing' -and $component.Name -eq 'qpdf30.dll'){continue}
            $entry=$zip.CreateEntry('lib/net45/bin/converter-tools/qpdf/'+$component.Name,[IO.Compression.CompressionLevel]::Fastest)
            if($case -eq 'symlink' -and $component.Name -eq 'qpdf.exe'){$entry.ExternalAttributes=-1610612736}
            $target=$entry.Open()
            try{
                if($case -eq 'tampered' -and $component.Name -eq 'qpdf30.dll'){
                    $bytes=[Text.Encoding]::UTF8.GetBytes('altered fixture component');$target.Write($bytes,0,$bytes.Length)
                } else {
                    $input=[IO.File]::OpenRead((Join-Path $bundle $component.Name))
                    try{$input.CopyTo($target)}finally{$input.Dispose()}
                }
            }finally{$target.Dispose()}
        }
        if($case -eq 'extra-dll'){$zip.CreateEntry('lib/net45/bin/converter-tools/qpdf/untrusted.dll')|Out-Null}
        if($case -eq 'extra-directory'){$zip.CreateEntry('lib/net45/bin/converter-tools/qpdf/untrusted/')|Out-Null}
        if($case -eq 'administrative'){$zip.CreateEntry('lib/net45/bin/converter-tools/.qpdf-backup-deadbeef/qpdf.exe')|Out-Null}
    }finally{$zip.Dispose();$stream.Dispose()}
    # Windows PowerShell turns a native stderr line into NativeCommandError.
    # Capture expected negative-case diagnostics without aborting the test loop.
    $ErrorActionPreference='Continue'
    try { & powershell -NoProfile -ExecutionPolicy Bypass -File $verifier -PackagePath $path *> (Join-Path $root ($case+'.log')); $verdict=$LASTEXITCODE }
    finally { $ErrorActionPreference='Stop' }
    $passed=if($case -eq 'exact'){$verdict -eq 0}else{$verdict -ne 0}
    if($passed){Write-Output ('PASS synthetic ZIP package-verifier case: '+$case)}
    else{$failed++;Write-Output ('FAIL synthetic ZIP package-verifier case: '+$case)}
}
Write-Output ('Package verifier fixture cases: '+$cases.Count+' total, '+($cases.Count-$failed)+' passed, '+$failed+' failed')
if($failed){exit 1}
