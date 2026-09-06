# Shared packaging/publication boundaries. Dot-source only; no work on import.
Set-StrictMode -Version Latest

function Assert-SquirrelLeafName([string] $Name) {
    # Restrict to portable leaf names, including on hosts with different path rules.
    if ($Name -notmatch '^[A-Za-z0-9][A-Za-z0-9._-]*$' -or $Name.EndsWith('.') -or
        $Name -match '^(?i:CON|PRN|AUX|NUL|COM[0-9]|LPT[0-9])(?:\.|$)') {
        throw "Unsafe Squirrel leaf name: $Name"
    }
}

function Assert-SquirrelPlainPath([string] $Path) {
    $cursor = [IO.Path]::GetFullPath($Path)
    while ($cursor) {
        if (Test-Path -LiteralPath $cursor) {
            if ((Get-Item -LiteralPath $cursor -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) {
                throw "Reparse points are not allowed in Squirrel output paths: $cursor"
            }
        }
        $cursor = Split-Path -Parent $cursor
    }
}

function Read-ReleaseEntries([string] $Path) {
    Assert-SquirrelPlainPath $Path
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Squirrel RELEASES file is missing: $Path" }
    $names = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    $entries = @(foreach ($line in Get-Content -LiteralPath $Path) {
        if ([string]::IsNullOrWhiteSpace($line)) { continue }
        $match = [regex]::Match($line, '^(?<sha>[0-9A-Fa-f]{40})\s+(?<name>\S+)\s+(?<size>[0-9]+)$')
        if (-not $match.Success) { throw "Squirrel RELEASES contains an invalid entry: $line" }
        $name = $match.Groups['name'].Value
        Assert-SquirrelLeafName $name
        if ($name -notmatch '-(?:full|delta)\.nupkg$' -or -not $names.Add($name)) {
            throw "Invalid or duplicate Squirrel package name: $name"
        }
        $file = Join-Path (Split-Path -Parent $Path) $name
        Assert-SquirrelPlainPath $file
        if (-not (Test-Path -LiteralPath $file -PathType Leaf)) { throw "RELEASES references missing package: $name" }
        $size = [long]$match.Groups['size'].Value
        $sha = $match.Groups['sha'].Value
        if ($size -le 0 -or (Get-Item -LiteralPath $file).Length -ne $size -or
            (Get-FileHash -LiteralPath $file -Algorithm SHA1).Hash -ine $sha) {
            throw "RELEASES size or SHA1 mismatch: $name"
        }
        [pscustomobject]@{ Sha1=$sha.ToUpperInvariant(); Name=$name; Size=$size; Line=$line.Trim() }
    })
    if ($entries.Count -eq 0) { throw "Squirrel RELEASES contains no entries: $Path" }
    return $entries
}

function Assert-SquirrelSeed($Entries, [string] $PackageId, [string] $CurrentVersion, [type] $SemanticVersionType) {
    # Use the comparator embedded in the verified Squirrel executable itself.
    # Its prerelease ordering is not lexical string ordering or System.Version.
    if (-not $SemanticVersionType -or $SemanticVersionType.FullName -cne 'NuGet.SemanticVersion') {
        throw 'Seed validation requires the pinned Squirrel semantic-version implementation.'
    }
    $parse = $SemanticVersionType.GetMethod('Parse', [type[]]@([string]))
    $pattern = '^' + [regex]::Escape($PackageId) + '-(?<version>\d+\.\d+\.\d+(?:-[0-9A-Za-z]{1,20})?)-(?<kind>full|delta)\.nupkg$'
    $baselineVersion = $null
    $fullCount = 0
    $deltaCount = 0
    foreach ($entry in $Entries) {
        $match = [regex]::Match($entry.Name, $pattern, [Text.RegularExpressions.RegexOptions]::IgnoreCase)
        if (-not $match.Success) { throw 'Seed feed contains a foreign package id or unsupported version.' }
        $version = $match.Groups['version'].Value
        if ($null -eq $baselineVersion) { $baselineVersion = $version }
        elseif ($baselineVersion -cne $version) { throw 'Seed feed mixes baseline versions.' }
        if ($match.Groups['kind'].Value -ieq 'full') { $fullCount++ } else { $deltaCount++ }
    }
    if ($fullCount -ne 1 -or $deltaCount -gt 1) { throw 'Seed feed requires exactly one full package and at most one matching delta.' }
    $baseline = $parse.Invoke($null, @($baselineVersion))
    $current = $parse.Invoke($null, @($CurrentVersion))
    if ($baseline.CompareTo($current) -ge 0) { throw 'Seed baseline must be strictly older than the current Squirrel version.' }
}

function Copy-SquirrelPayload([string] $InstallDirectory, [string] $PayloadDirectory, [string] $QpdfManifestPath) {
    # Preserve the input tree. Administration records never enter the staged
    # payload, and a qpdf bundle is copied only from the committed hash inventory.
    function Copy-QpdfComponents([string] $Source, [string] $Destination) {
        Assert-SquirrelPlainPath $Source
        $lock = Get-Content -LiteralPath $QpdfManifestPath -Raw | ConvertFrom-Json
        $records = @($lock.files.PSObject.Properties)
        if ($lock.name -cne 'qpdf' -or $records.Count -ne 10 -or $lock.version -cne '12.3.2') { throw 'Invalid pinned qpdf component inventory.' }
        [IO.Directory]::CreateDirectory($Destination) | Out-Null
        foreach ($record in $records) {
            Assert-SquirrelLeafName $record.Name
            $path = Join-Path $Source $record.Name
            Assert-SquirrelPlainPath $path
            if ($record.Value -notmatch '^[a-f0-9]{64}$' -or
                (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ine $record.Value) {
                throw "Pinned qpdf component hash mismatch: $($record.Name)"
            }
            $target = Join-Path $Destination $record.Name
            Copy-Item -LiteralPath $path -Destination $target
            if ((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -ine $record.Value) {
                throw "Staged qpdf component hash mismatch: $($record.Name)"
            }
        }
    }
    function Copy-ConverterTools([string] $Source, [string] $Destination) {
        [IO.Directory]::CreateDirectory($Destination) | Out-Null
        foreach ($item in Get-ChildItem -LiteralPath $Source -Force) {
            if ($item.Name -ieq 'qpdf') {
                Copy-QpdfComponents $item.FullName (Join-Path $Destination 'qpdf')
            } elseif ($item.Name -ceq '.qpdf-bootstrap.lock' -or
                $item.Name -cmatch '^\.qpdf-(stage|backup|invalid)-[a-f0-9]{32}$' -or
                $item.Name -ceq 'qpdf.activation.json' -or
                $item.Name -cmatch '^qpdf\.activation\.completed-[a-f0-9]{32}\.json$' -or
                $item.Name -cmatch '^qpdf\.activation\.json\.[a-f0-9]{32}\.tmp$') {
                Write-Host "Omitted qpdf administration entry from package staging: $($item.Name)"
            } elseif ($item.Name -imatch '^(\.qpdf-|qpdf\.activation)') {
                throw "Unknown qpdf administration entry; source retained: $($item.Name)"
            } else { Copy-Item -LiteralPath $item.FullName -Destination $Destination -Recurse }
        }
    }
    [IO.Directory]::CreateDirectory($PayloadDirectory) | Out-Null
    if (@(Get-ChildItem -LiteralPath $PayloadDirectory -Force).Count) { throw 'Payload staging requires an empty directory.' }
    foreach ($item in Get-ChildItem -LiteralPath $InstallDirectory -Force) {
        if ($item.Name -ieq 'bin' -and $item.PSIsContainer) {
            $bin = Join-Path $PayloadDirectory 'bin'
            [IO.Directory]::CreateDirectory($bin) | Out-Null
            foreach ($child in Get-ChildItem -LiteralPath $item.FullName -Force) {
                if ($child.Name -ieq 'converter-tools' -and $child.PSIsContainer) {
                    Copy-ConverterTools $child.FullName (Join-Path $bin 'converter-tools')
                } else { Copy-Item -LiteralPath $child.FullName -Destination $bin -Recurse }
            }
        } else { Copy-Item -LiteralPath $item.FullName -Destination $PayloadDirectory -Recurse }
    }
}

function Assert-SquirrelOutput([string] $Directory, [string] $ExpectedVersion = '', [string] $ExpectedPackageId = '',
    [ValidateSet('SHA256SUMS','PACKAGE-SHA256SUMS')][string] $ChecksumName = 'SHA256SUMS') {
    Assert-SquirrelPlainPath $Directory
    $manifestPath = Join-Path $Directory 'package-output-manifest.json'
    Assert-SquirrelPlainPath $manifestPath
    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    if ($manifest.schemaVersion -ne 1 -or $manifest.packageVersion -notmatch '^\d+\.\d+\.\d+(-[0-9A-Za-z]{1,20})?$' -or
        ($ExpectedVersion -and $manifest.packageVersion -cne $ExpectedVersion) -or
        ($ExpectedPackageId -and $manifest.packageId -cne $ExpectedPackageId)) { throw 'Squirrel output manifest identity mismatch.' }
    Assert-SquirrelLeafName $manifest.packageId
    $full = "$($manifest.packageId)-$($manifest.packageVersion)-full.nupkg"
    $delta = "$($manifest.packageId)-$($manifest.packageVersion)-delta.nupkg"
    $packages = @($full)
    if ($manifest.seedMode -eq 'baseline-used-for-delta-only') { $packages += $delta }
    elseif ($manifest.seedMode -ne 'no-baseline') { throw 'Invalid Squirrel seed mode.' }
    $expected = @('Setup.exe', 'RELEASES') + $packages
    $names = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($record in $manifest.files) {
        Assert-SquirrelLeafName $record.name
        if (-not $names.Add($record.name) -or $record.name -cnotin $expected -or $record.sha256 -notmatch '^[0-9a-fA-F]{64}$') {
            throw 'Squirrel output manifest has an unexpected or duplicate file.'
        }
        $path = Join-Path $Directory $record.name
        Assert-SquirrelPlainPath $path
        if (-not (Test-Path -LiteralPath $path -PathType Leaf) -or [long]$record.bytes -le 0 -or
            (Get-Item -LiteralPath $path).Length -ne [long]$record.bytes -or
            (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash -ine $record.sha256) {
            throw "Squirrel output manifest bytes or SHA256 mismatch: $($record.name)"
        }
    }
    if ($names.Count -ne $expected.Count) { throw 'Squirrel output manifest is missing required files.' }
    $actual = @(Get-ChildItem -LiteralPath $Directory -Force)
    $allowed = $expected + @($ChecksumName, 'package-output-manifest.json')
    if ($actual.Count -ne $allowed.Count -or @($actual | Where-Object { $_.PSIsContainer -or $_.Name -cnotin $allowed }).Count) {
        throw 'Squirrel output contains unmanaged or missing entries; all existing bytes are retained.'
    }
    $entries = @(Read-ReleaseEntries (Join-Path $Directory 'RELEASES'))
    if ($entries.Count -ne $packages.Count -or @($entries | Where-Object { $_.Name -cnotin $packages }).Count -or
        @($manifest.releaseEntries).Count -ne $entries.Count) { throw 'Squirrel output feed does not match the current version.' }
    $feedNames = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
    foreach ($record in $manifest.releaseEntries) {
        if (-not $feedNames.Add($record.name)) { throw 'Duplicate manifest release entry.' }
        $entry = @($entries | Where-Object { $_.Name -ceq $record.name })
        if ($entry.Count -ne 1 -or $entry[0].Sha1 -ine $record.sha1 -or $entry[0].Size -ne [long]$record.bytes) {
            throw 'Squirrel manifest release entry differs from RELEASES.'
        }
    }
    $expectedChecksums = @($manifest.files | Sort-Object name | ForEach-Object { '{0}  {1}' -f $_.sha256.ToLowerInvariant(), $_.name })
    $checksumsPath = Join-Path $Directory $ChecksumName
    Assert-SquirrelPlainPath $checksumsPath
    $actualChecksums = @(Get-Content -LiteralPath $checksumsPath)
    if (($actualChecksums -join "`n") -cne ($expectedChecksums -join "`n")) { throw 'Package SHA256SUMS does not match the manifest.' }
    return $manifest
}

function Copy-SquirrelReleaseAssets([string] $Source, [string] $Destination, [string] $Version, [string] $PackageId = 'Audacity') {
    if (-not $Version) { throw 'Release collection requires the intended package version.' }
    $manifest = Assert-SquirrelOutput $Source $Version $PackageId
    Assert-SquirrelPlainPath $Destination
    if ((Test-Path -LiteralPath $Destination) -and @(Get-ChildItem -LiteralPath $Destination -Force).Count) {
        throw 'Release collection requires a new or empty destination.'
    }
    [IO.Directory]::CreateDirectory([IO.Path]::GetFullPath($Destination)) | Out-Null
    foreach ($name in @($manifest.files | ForEach-Object name) + @('package-output-manifest.json', 'SHA256SUMS')) {
        $targetName = if ($name -ceq 'SHA256SUMS') { 'PACKAGE-SHA256SUMS' } else { $name }
        Copy-Item -LiteralPath (Join-Path $Source $name) -Destination (Join-Path $Destination $targetName)
    }
    $null = Assert-SquirrelOutput $Destination $Version $PackageId -ChecksumName 'PACKAGE-SHA256SUMS'
}

function Open-SquirrelOutputLock([string] $Directory) {
    $full = [IO.Path]::GetFullPath($Directory).TrimEnd([IO.Path]::DirectorySeparatorChar)
    Assert-SquirrelPlainPath $full
    $parent = Split-Path -Parent $full
    if (-not $parent) { throw 'A filesystem root cannot be a Squirrel output directory.' }
    [IO.Directory]::CreateDirectory($parent) | Out-Null
    $path = Join-Path $parent ('.' + (Split-Path -Leaf $full) + '.publish.lock')
    Assert-SquirrelPlainPath $path
    try { return [IO.File]::Open($path, [IO.FileMode]::OpenOrCreate, [IO.FileAccess]::ReadWrite, [IO.FileShare]::None) }
    catch { throw "Squirrel output is locked by another publisher, or the lock is inaccessible: $path" }
}

function Assert-SquirrelPriorOutput([string] $Directory, [string] $ManifestHash) {
    if ($ManifestHash -eq 'empty') {
        if (-not (Test-Path -LiteralPath $Directory -PathType Container) -or @(Get-ChildItem -LiteralPath $Directory -Force).Count) {
            throw 'Previously empty Squirrel directory has changed.'
        }
        Assert-SquirrelPlainPath $Directory
    } else {
        $null = Assert-SquirrelOutput $Directory
        if ((Get-FileHash -LiteralPath (Join-Path $Directory 'package-output-manifest.json') -Algorithm SHA256).Hash -cne $ManifestHash) {
            throw 'Squirrel transaction manifest has changed.'
        }
    }
}

# Caller holds the exclusive lock. A journal is immutable and exists before the
# first rename. Recovery never deletes a tree or guesses ownership from a suffix.
function Restore-SquirrelOutputTransaction([string] $Directory) {
    $Directory = [IO.Path]::GetFullPath($Directory).TrimEnd([IO.Path]::DirectorySeparatorChar)
    $parent = Split-Path -Parent $Directory
    $prefix = '.' + (Split-Path -Leaf $Directory) + '.publish'
    $journalPath = Join-Path $parent "$prefix.json"
    if (-not (Test-Path -LiteralPath $journalPath)) { return }
    Assert-SquirrelPlainPath $journalPath
    $journal = Get-Content -LiteralPath $journalPath -Raw | ConvertFrom-Json
    if ($journal.schemaVersion -ne 1 -or $journal.id -notmatch '^[a-f0-9]{32}$' -or $journal.output -cne $Directory -or
        $journal.candidateHash -notmatch '^[A-F0-9]{64}$' -or
        $journal.priorHash -notmatch '^(absent|empty|[A-F0-9]{64})$') { throw 'Invalid Squirrel activation journal; retained for inspection.' }
    $candidate = Join-Path $parent "$prefix.$($journal.id).candidate"
    $backup = Join-Path $parent "$prefix.$($journal.id).previous"
    foreach ($path in @($candidate,$backup,$Directory)) { Assert-SquirrelPlainPath $path }
    if (Test-Path -LiteralPath $Directory) {
        try { Assert-SquirrelPriorOutput $Directory $journal.candidateHash }
        catch {
            if ($journal.priorHash -eq 'absent') { throw }
            Assert-SquirrelPriorOutput $Directory $journal.priorHash
        }
    } elseif ($journal.priorHash -ne 'absent') {
        Assert-SquirrelPriorOutput $backup $journal.priorHash
        [IO.Directory]::Move($backup, $Directory)
    } else {
        Assert-SquirrelPriorOutput $candidate $journal.candidateHash
        [IO.Directory]::Move($candidate, $Directory)
    }
    [IO.File]::Move($journalPath, (Join-Path $parent "$prefix.$($journal.id).completed.json"))
}

function Publish-SquirrelOutput([string] $Candidate, [string] $Directory, [scriptblock] $Boundary = {}) {
    $Directory = [IO.Path]::GetFullPath($Directory).TrimEnd([IO.Path]::DirectorySeparatorChar)
    $Candidate = [IO.Path]::GetFullPath($Candidate)
    $lock = Open-SquirrelOutputLock $Directory
    try {
        Restore-SquirrelOutputTransaction $Directory
        $manifest = Assert-SquirrelOutput $Candidate
        $priorHash = 'absent'
        if (Test-Path -LiteralPath $Directory) {
            if (@(Get-ChildItem -LiteralPath $Directory -Force).Count -eq 0) { $priorHash = 'empty' }
            else {
                $null = Assert-SquirrelOutput $Directory
                $priorHash = (Get-FileHash -LiteralPath (Join-Path $Directory 'package-output-manifest.json') -Algorithm SHA256).Hash
            }
        }
        $parent = Split-Path -Parent $Directory
        $prefix = '.' + (Split-Path -Leaf $Directory) + '.publish'
        $id = [guid]::NewGuid().ToString('N')
        $stage = Join-Path $parent "$prefix.$id.candidate"
        $backup = Join-Path $parent "$prefix.$id.previous"
        [IO.Directory]::CreateDirectory($stage) | Out-Null
        foreach ($file in Get-ChildItem -LiteralPath $Candidate -File -Force) {
            Copy-Item -LiteralPath $file.FullName -Destination (Join-Path $stage $file.Name)
        }
        $null = Assert-SquirrelOutput $stage $manifest.packageVersion $manifest.packageId
        & $Boundary 'staged'
        $candidateHash = (Get-FileHash -LiteralPath (Join-Path $stage 'package-output-manifest.json') -Algorithm SHA256).Hash
        $journal = [ordered]@{ schemaVersion=1; id=$id; output=$Directory; candidateHash=$candidateHash; priorHash=$priorHash }
        $journalPath = Join-Path $parent "$prefix.json"
        $temp = Join-Path $parent "$prefix.$id.journal.tmp"
        $bytes = [Text.Encoding]::UTF8.GetBytes(($journal | ConvertTo-Json -Compress))
        $stream = [IO.File]::Open($temp, [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
        try { $stream.Write($bytes, 0, $bytes.Length); $stream.Flush($true) } finally { $stream.Dispose() }
        [IO.File]::Move($temp, $journalPath)
        & $Boundary 'journaled'
        if ($priorHash -ne 'absent') {
            Assert-SquirrelPriorOutput $Directory $priorHash
            [IO.Directory]::Move($Directory, $backup)
        }
        & $Boundary 'previous-renamed'
        [IO.Directory]::Move($stage, $Directory)
        & $Boundary 'candidate-renamed'
        Assert-SquirrelPriorOutput $Directory $candidateHash
        [IO.File]::Move($journalPath, (Join-Path $parent "$prefix.$id.completed.json"))
        Write-Host "Activated verified Squirrel output: $Directory"
        if ($priorHash -ne 'absent') { Write-Host "Retained previous output: $backup" }
    } finally { $lock.Dispose() }
}
