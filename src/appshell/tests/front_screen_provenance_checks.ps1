[CmdletBinding()]
param([string] $QtPrefix = '', [switch] $SourceOnly)
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
& python -m unittest discover -s (Join-Path $PSScriptRoot 'provenance') -p test_front_provenance.py -v
if ($LASTEXITCODE -ne 0) { throw 'Front provenance manifest and shell checks failed.' }
if ($SourceOnly) {
    Write-Host 'Source and generator checks passed. Executable formatting tests were explicitly not requested.'
    return
}
if (-not $QtPrefix) { $QtPrefix = Join-Path $repo 'build.tools/Qt/6.10.1/msvc2022_64' }
if (-not (Test-Path -LiteralPath (Join-Path $QtPrefix 'lib/cmake/Qt6/Qt6Config.cmake'))) {
    throw 'The executable tests require the verified Qt 6.10.1 prefix from the project bootstrap; pass it with -QtPrefix.'
}
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) { throw 'Project MSVC bootstrap is unavailable: vswhere.exe is missing.' }
    $vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vs) { throw 'Project MSVC bootstrap did not provide the x64 C++ toolchain.' }
    $vcvars = Join-Path $vs 'VC/Auxiliary/Build/vcvars64.bat'
    $environment = & cmd.exe /c "call `"$vcvars`" >nul && set"
    if ($LASTEXITCODE -ne 0) { throw 'MSVC environment setup failed.' }
    foreach ($entry in $environment) {
        if ($entry -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($Matches[1], $Matches[2], 'Process') }
    }
}
$env:PATH = (Join-Path $QtPrefix 'bin') + ';' + $env:PATH
$build = Join-Path $repo 'build/front-provenance-tests'
& cmake -S (Join-Path $PSScriptRoot 'provenance') -B $build -G 'NMake Makefiles' "-DCMAKE_PREFIX_PATH=$QtPrefix"
if ($LASTEXITCODE -ne 0) { throw 'Executable provenance test configuration failed.' }
& cmake --build $build
if ($LASTEXITCODE -ne 0) { throw 'Executable provenance test build failed.' }
& (Join-Path $build 'front_build_provenance_tests.exe')
if ($LASTEXITCODE -ne 0) { throw 'Executable provenance checks failed.' }
Write-Host 'Front provenance generator, shell and executable checks passed. No graphical runtime was launched.'