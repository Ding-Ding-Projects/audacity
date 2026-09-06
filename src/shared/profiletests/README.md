# Isolated profile verification

`--profile-dir <absolute-directory>` selects a new empty directory or an existing
version-1 ownership-marked fixture before command parsing, application creation,
and Muse settings initialization. The original Audacity application identity remains.
The profile argument is inherited by Muse child windows; both single-instance
activation, the multi-process resource bus, and named resource locks use the canonical root hash.

The provider source is compiled exactly once in `muse_global` by the committed
`0011-isolated-profile.patch` overlay. The Windows product uses static linkage
(`BUILD_SHARED_LIBS=OFF`), not a provider DLL. It is not compiled into each consuming module.
QSettings uses INI files in independent user and system directories. Portable settings
must not replace those directories. Framework defaults and listed direct consumers
use the provider. Existing fixtures reject symlinks and Windows reparse points.

This is application storage and known-side-effect isolation, not an operating-system
sandbox. It does not constrain arbitrary plugins, malicious concurrent filesystem
replacement, audio hardware, file-picker selections, or all legacy third-party code.
A marker establishes a deliberately selected fixture, not an authentication secret.
Never select a user's real profile, import their configuration, or claim UI safety
from these source and pure Qt tests alone.

## Verification

Configure this directory independently with CMake and Qt 6.10.1, build, then run
`profile_tests` and `profile_resource_tests` with the Qt bin directory on PATH.
The first target exercises provider export/import mechanics through separate DLLs.
It does not prove the actual Windows product layout. The second builds the provider
and its consumer with static linkage matching the product configuration, and runs
the real Muse IpcLock implementation in two processes. Both profiles must acquire
the same logical resource before either is released; each also creates a contained
QTemporaryFile with the Chronicle template. Neither target proves the complete
product binary because that binary has not been built in this lane. Child processes verify
initialization order, redirected locations, both QSettings scopes, stable IPC,
parallel profiles, ownership markers, protected folders, and Windows junctions.
Run `python src/shared/profiletests/check_inventory.py` for explicit consumer
counts and omitted-file red/restore-green checks. These are source checks, not
proof that each complete feature executed in a built application.

## Disabled side effects and remaining boundaries

OS recent-document updates and URL scheme registration are suppressed. Squirrel
checking, downloading, and applying updates are unavailable. Muse network requests,
dim-sum downloads, and the public legacy-cloud service entrypoints return failure or
an explicit unavailable state in the isolated profile. Service registration remains.
These refusals do not verify those features' successful network behavior.

The parent integration still owns toolkit storage consumers, external-editor and
Ollama process/network suppression, and any required corresponding test linkage.
External browser or shell routes include Muse `IPlatformInteractive`,
`src/personalize/internal/supporttickets.cpp`, and toolkit external editors.
Clipboard routes include `src/trackedit/internal/au3/au3trackeditclipboard.cpp`,
`src/appshell/qml/Audacity/AppShell/aboutmodel.cpp`, and framework clipboard wrappers.
The system clipboard and external browser are not touched by this test suite.

A future fixture-network permission must be explicit per endpoint, with a numeric
loopback address, exact port, allowed method/path, bounded payload and timeout,
redirect refusal, and no arbitrary hostname, proxy, wildcard, or production endpoint.
It must be fixture-owned and separate from update, registration, browser, and shell
permissions. No broad network enable switch is implemented in this change.

UI execution remains unverified: automatic approval review rejected launching the
hidden Lowlevel HTTP service. No alternative UI route was attempted.
