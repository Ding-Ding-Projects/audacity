$ErrorActionPreference = 'Stop'

function Remove-QmlComments([string] $text) {
    $withoutBlockComments = [regex]::Replace($text, '/\*.*?\*/', '', [System.Text.RegularExpressions.RegexOptions]::Singleline)
    return [regex]::Replace($withoutBlockComments, '(?m)//.*$', '')
}

function Assert-Contains([string] $text, [string] $pattern, [string] $message) {
    if (-not [regex]::IsMatch($text, $pattern, [System.Text.RegularExpressions.RegexOptions]::Singleline)) { throw $message }
}

function Assert-RejectsMutation([string] $name, [string] $mutatedText, [scriptblock] $assertion) {
    $accepted = $true
    try { & $assertion $mutatedText } catch { $accepted = $false }
    if ($accepted) { throw "Negative regression accepted mutation: $name" }
}

$homeMenuPath = Join-Path $PSScriptRoot '..\qml\Audacity\AppShell\HomePage\HomeMenu.qml'
$aboutModelPath = Join-Path $PSScriptRoot '..\qml\Audacity\AppShell\aboutmodel.cpp'
$cmakePath = Join-Path $PSScriptRoot '..\qml\Audacity\AppShell\CMakeLists.txt'
$generatorPath = Join-Path $PSScriptRoot '..\qml\Audacity\AppShell\GenerateBuildProvenance.cmake'
$homeMenu = Remove-QmlComments (Get-Content -Raw $homeMenuPath)
$aboutModel = Get-Content -Raw $aboutModelPath
$cmake = Get-Content -Raw $cmakePath
$generator = Get-Content -Raw $generatorPath

$homeAssertion = {
    param($text)
    Assert-Contains $text 'runningVersionLine:\s*qsTrc\("appshell",\s*"Version %1"\)\.arg\(aboutModel\.appVersion\(\)\)' 'The visible version must come from the running application.'
    Assert-Contains $text 'Build provenance unavailable' 'The unavailable provenance state is required.'
    Assert-Contains $text 'Version source updated at %1' 'The labelled source updated-at state is required.'
    Assert-Contains $text 'anchors\.bottomMargin:\s*narrowProvenance\.implicitHeight \+ 20' 'The narrow rail must reserve room from its live labels.'
    if (($text | Select-String -AllMatches 'wrapMode:\s*Text\.WrapAnywhere').Matches.Count -lt 2) { throw 'Both narrow provenance labels must wrap at timestamp boundaries.' }
    if ($text -match 'elide:\s*Text\.ElideRight') { throw 'Front-screen provenance must never elide factual values.' }
}

& $homeAssertion $homeMenu
Assert-Contains $aboutModel '#include "appshellbuildprovenance\.h"' 'AboutModel must read generated build provenance.'
Assert-Contains $aboutModel '\^\[0-9a-f\]\{40\}\$' 'AboutModel must validate an exact hexadecimal source revision.'
Assert-Contains $aboutModel 'yyyy-MM-dd HH:mm:ss t' 'Local provenance must include seconds and a timezone label.'
Assert-Contains $cmake 'add_custom_target\(appshell_build_provenance ALL' 'Build provenance must regenerate during each build.'
Assert-Contains $cmake 'add_dependencies\(appshell_qml appshell_build_provenance\)' 'The QML module must depend on generated provenance.'
Assert-Contains $generator 'NOT AU_ACTUAL_SOURCE_REVISION STREQUAL AU_EXPECTED_SOURCE_REVISION' 'The generator must reject a source candidate mismatch.'
Assert-Contains $generator 'show -s --format=%cI HEAD' 'The generated timestamp must come from the recorded candidate source.'
Assert-Contains $generator 'diff --quiet --ignore-submodules=dirty' 'The generator must reject changed submodule pins and unstaged source dirt.'
Assert-Contains $generator 'diff --cached --quiet --ignore-submodules=dirty' 'The generator must reject staged changed submodule pins and source dirt.'
Assert-Contains $generator 'ls-files --others --exclude-standard' 'The generator must reject untracked source dirt.'
Assert-Contains $generator 'RESULT_VARIABLE AU_UNTRACKED_FILES_RESULT' 'The generator must reject an untracked-file scan failure.'

Assert-RejectsMutation 'unavailable-state' ($homeMenu -replace 'Build provenance unavailable', 'removed') $homeAssertion
Assert-RejectsMutation 'narrow-wrap' ($homeMenu -replace 'Text\.WrapAnywhere', 'Text.WordWrap') $homeAssertion
Assert-RejectsMutation 'live-reserve' ($homeMenu -replace 'narrowProvenance\.implicitHeight \+ 20', '0') $homeAssertion
Assert-RejectsMutation 'candidate-mismatch' ($generator -replace 'NOT AU_ACTUAL_SOURCE_REVISION STREQUAL AU_EXPECTED_SOURCE_REVISION', 'FALSE') {
    param($text)
    Assert-Contains $text 'NOT AU_ACTUAL_SOURCE_REVISION STREQUAL AU_EXPECTED_SOURCE_REVISION' 'The generator must reject a source candidate mismatch.'
}

$fixture = Join-Path $env:TEMP ('audacity-front-provenance-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $fixture | Out-Null
try {
    git -C $fixture init -q
    git -C $fixture config user.name 'fixture'
    git -C $fixture config user.email 'fixture@example.invalid'
    Set-Content -LiteralPath (Join-Path $fixture '.gitignore') -Value 'build/' -NoNewline
    Set-Content -LiteralPath (Join-Path $fixture 'tracked.txt') -Value 'baseline' -NoNewline
    git -C $fixture add .gitignore tracked.txt
    git -C $fixture commit -q -m fixture
    $fixtureRevision = (git -C $fixture rev-parse HEAD).Trim()
    $fixtureHeader = Join-Path $fixture 'build/appshelldisplayprovenance.h'

    function Invoke-FixtureGeneration {
        param([bool] $ExpectSuccess)
        $previousErrorActionPreference = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        & cmake "-DAU_SOURCE_DIR=$fixture" "-DAU_EXPECTED_SOURCE_REVISION=$fixtureRevision" "-DAU_OUTPUT_HEADER=$fixtureHeader" -P $generatorPath
        $exitCode = $LASTEXITCODE
        $ErrorActionPreference = $previousErrorActionPreference
        if ($ExpectSuccess -and $exitCode -ne 0) { throw 'Clean fixture provenance generation failed.' }
        if (-not $ExpectSuccess -and $exitCode -eq 0) { throw 'Dirty fixture provenance generation unexpectedly succeeded.' }
    }

    Invoke-FixtureGeneration $true
    $firstBytes = [IO.File]::ReadAllBytes($fixtureHeader)
    $firstWrite = (Get-Item $fixtureHeader).LastWriteTimeUtc
    Start-Sleep -Milliseconds 30
    Invoke-FixtureGeneration $true
    if ([Convert]::ToBase64String($firstBytes) -ne [Convert]::ToBase64String([IO.File]::ReadAllBytes($fixtureHeader))) { throw 'Unchanged candidate rewrote header bytes.' }
    if ((Get-Item $fixtureHeader).LastWriteTimeUtc -ne $firstWrite) { throw 'Unchanged candidate rewrote the header.' }

    Set-Content -LiteralPath (Join-Path $fixture 'tracked.txt') -Value 'unstaged' -NoNewline
    Invoke-FixtureGeneration $false
    git -C $fixture restore tracked.txt
    Set-Content -LiteralPath (Join-Path $fixture 'tracked.txt') -Value 'staged' -NoNewline
    git -C $fixture add tracked.txt
    Invoke-FixtureGeneration $false
    git -C $fixture reset -q HEAD -- tracked.txt
    git -C $fixture restore tracked.txt
    Set-Content -LiteralPath (Join-Path $fixture 'untracked.txt') -Value 'untracked' -NoNewline
    Invoke-FixtureGeneration $false
    Remove-Item -LiteralPath (Join-Path $fixture 'untracked.txt') -Force
    Invoke-FixtureGeneration $true
} finally {
    if (Test-Path -LiteralPath $fixture) { Remove-Item -LiteralPath $fixture -Recurse -Force }
}

Write-Host 'Front-screen provenance structural and negative-regression checks passed.'
