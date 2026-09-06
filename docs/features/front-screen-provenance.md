# Front-screen version and provenance

The Home menu presents the version of the running application and the local
updated-at value before a user chooses another destination. The provenance
target generates `AU_BUILD_TIMESTAMP_UTC` while it builds the configured Git
candidate, then `AboutModel` formats it locally with seconds and the local
timezone label.

The generator compares the current Git revision with the revision CMake
configured. A mismatch stops the build and asks for a reconfigure, so an
incremental build cannot label a newer source tree with stale provenance.

The candidate must also have no staged, unstaged, or untracked source changes.
Changed submodule pins are rejected with `--ignore-submodules=dirty`; managed
overlay content is allowed only when its separately tracked patch recipe is
already part of the candidate, never by treating arbitrary nested dirt as
clean provenance.

The Home menu never uses launch time or a file timestamp.  If the configured
timestamp is absent or invalid, it presents **Build provenance unavailable**.
This is an honest state, rather than a guessed timestamp.

The full text wraps in both the expanded Home menu and its narrow rail, so the
version and provenance are not elided.  Accessible names include both values.

## Verification

Run the focused source check from the repository root:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File src/appshell/tests/front_screen_provenance_checks.ps1
```

The check verifies the exact Home-menu contract, its unavailable state, both
responsive layouts, accessible provenance text, generated candidate-bound
provenance, and deliberate negative mutations. It removes required boundaries
in memory and verifies that every mutation turns the check red.
