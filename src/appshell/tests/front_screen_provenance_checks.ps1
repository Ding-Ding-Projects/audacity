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
    Assert-Contains $text 'Build updated at %1' 'The labelled local updated-at state is required.'
    Assert-Contains $text 'anchors\.bottomMargin:\s*root\.narrowProvenanceHeight \+ 20' 'The narrow rail must reserve room for provenance.'
    Assert-Contains $text 'height:\s*root\.narrowProvenanceHeight' 'The narrow provenance area must have reserved vertical height.'
    if (($text | Select-String -AllMatches 'wrapMode:\s*Text\.WrapAnywhere').Matches.Count -lt 2) { throw 'Both narrow provenance labels must wrap at timestamp boundaries.' }
    if ($text -match 'elide:\s*Text\.ElideRight') { throw 'Front-screen provenance must never elide factual values.' }
}

& $homeAssertion $homeMenu
Assert-Contains $aboutModel '#include "appshellbuildprovenance\.h"' 'AboutModel must read generated build provenance.'
Assert-Contains $aboutModel 'AU_BUILD_SOURCE_REVISION' 'AboutModel must validate the candidate source revision.'
Assert-Contains $aboutModel 'yyyy-MM-dd HH:mm:ss t' 'Local provenance must include seconds and a timezone label.'
Assert-Contains $cmake 'add_custom_target\(appshell_build_provenance ALL' 'Build provenance must regenerate during each build.'
Assert-Contains $cmake 'add_dependencies\(appshell_qml appshell_build_provenance\)' 'The QML module must depend on generated provenance.'
Assert-Contains $generator 'NOT AU_ACTUAL_SOURCE_REVISION STREQUAL AU_EXPECTED_SOURCE_REVISION' 'The generator must reject a source candidate mismatch.'
Assert-Contains $generator 'string\(TIMESTAMP AU_BUILD_TIMESTAMP_UTC' 'The generated timestamp must be recorded during the build.'

Assert-RejectsMutation 'unavailable-state' ($homeMenu -replace 'Build provenance unavailable', 'removed') $homeAssertion
Assert-RejectsMutation 'narrow-wrap' ($homeMenu -replace 'Text\.WrapAnywhere', 'Text.WordWrap') $homeAssertion
Assert-RejectsMutation 'reserved-height' ($homeMenu -replace 'height:\s*root\.narrowProvenanceHeight', 'height: 0') $homeAssertion
Assert-RejectsMutation 'candidate-mismatch' ($generator -replace 'NOT AU_ACTUAL_SOURCE_REVISION STREQUAL AU_EXPECTED_SOURCE_REVISION', 'FALSE') {
    param($text)
    Assert-Contains $text 'NOT AU_ACTUAL_SOURCE_REVISION STREQUAL AU_EXPECTED_SOURCE_REVISION' 'The generator must reject a source candidate mismatch.'
}

Write-Host 'Front-screen provenance structural and negative-regression checks passed.'
