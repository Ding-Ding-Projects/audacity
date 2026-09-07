# Front-screen version and build provenance

Every initial page uses the `WindowContent` shell. Its permanently present
`FrontBuildProvenance` component displays the version and recorded build time
above the docked page content. The page container reserves the component's
actual height, and both labels wrap without eliding factual values. The shell
retains the existing DockWindow instance and delegates its `init()` entry point.
It does not change application identity or startup routing.

The explicit startup-mode inventory covers `StartEmpty`, `StartWithNewProject`
and `Recovery` through `HOME_URI`, plus `StartWithProject`,
`ContinueLastSession` and `FirstLaunch` through `PROJECT_URI`. Provenance is a
sibling of that page container, not a Home-only child or a conditional loader.

## One recorded build manifest

CMake assigns a build identity when it configures the candidate. Before the QML
module compiles, `appshell_build_provenance` verifies the configured revision,
rejects staged, unstaged and eligible untracked source changes, and records a
JSON manifest under the ignored binary-output `generated/manifests/` directory.
The record contains:

| Field | Source |
| --- | --- |
| `schemaVersion` | The supported manifest schema, currently 1. |
| `version` | The application's configured `MUSE_APP_VERSION`, with no fallback value. |
| `versionLabel` | The configured application version label; empty is permitted. |
| `buildNumber` | The configured application build number; empty is permitted. |
| `buildId` | The identity of this CMake configuration instance. |
| `sourceRevision` | The exact configured and rechecked Git commit. |
| `sourceTree` | That commit's recorded tree identity. |
| `buildStartedAtUtc` | The build producer's UTC time when this record is first created. |
| `timestampKind` | `build-start`, identifying the recorded event precisely. |

The timestamp is not a commit date, application launch time, filesystem time,
release publication time or a hand-entered label. The generator deliberately
does not treat `SOURCE_DATE_EPOCH` as a build-start clock. A repeated build of
the same configured instance reuses its record without changing its bytes or
header timestamp. Reconfiguration creates a new build identity and a separate
record; prior records remain available.

The generator uses an exclusive lock, writes each manifest once, and checks its
stored SHA-256 before reuse. Missing paired evidence, changed bytes or metadata
mismatches fail closed without relabelling the prior record. An interrupted
incomplete record remains for inspection and requires a fresh build directory.
The generated header embeds the exact file bytes as hexadecimal plus their
SHA-256, using binary reads so native line endings cannot break the binding.

`AboutModel` delegates version and timestamp formatting to the same Qt Core
manifest parser. The version includes the recorded optional label and build
number, for example `4.0.0-beta.14`. Missing, malformed or mismatched manifest
fields return empty values, and the front shell shows **Version unavailable**
and **Build provenance unavailable**. It never substitutes another version or
clock. UTC output converts an explicit source offset before adding its UTC
label. Local output includes seconds, numeric UTC offset and timezone identity,
including daylight-saving differences.

The existing candidate guard rejects changed submodule pins with
`--ignore-submodules=dirty`. It does not itself certify nested working content:
managed overlay recipes and their separate verification remain required. This
feature must not be described as proof of arbitrary nested source cleanliness.

## Verification and limits

The focused entry point runs the real CMake generator fixtures, common-shell
and startup-mode checks, then compiles and executes the actual Qt Core parser
and formatter. It locates the project MSVC tools and uses the verified Qt prefix
from the project bootstrap; an alternate already-provisioned prefix can be
passed explicitly.

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File src/appshell/tests/front_screen_provenance_checks.ps1
```

`-SourceOnly` explicitly omits executable tests and reports that limitation.
`-QtPrefix <verified-Qt-prefix>` selects the available Qt 6.10.1 toolchain.

Generator fixtures prove build-clock recording, exact embedded bytes, unchanged
record reuse, separate configuration identities, no output after candidate
rejection, manifest tampering refusal, and missing-version recording without
invention. Executable checks cover missing version, revision, tree, build id,
date and timezone; invalid calendar values; strict whole-string validation;
manifest hash mismatches; UTC conversion; and winter/summer local formatting.
Exact source negatives remove visible labels, unavailable states, height
reservation and startup-mode entries and must be rejected.

These are executable formatting, generator and source-structure proofs. They do
not establish rendered visibility, accessibility behavior, layout at supported
scales, or complete startup behavior in the built graphical application. Those
runtime interactions and captures remain separate evidence.