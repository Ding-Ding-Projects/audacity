# School mode

## Behaviour

School mode is one universal, user-renamable mode shared across every app on the machine
that honours it. Its record lives in a shared location, one level above this application's
own application data directory, at `<app data parent>/shared/school-mode.json` (documented
here rather than hidden, so a user who wants to reset it by hand can find it).

The shipped display name is "School mode". A user may rename it; once renamed, the shipped
name is never shown again anywhere in the application.

While the mode is on:

- presentation is forced to English regardless of the configured language mode;
- Cantonese, bilingual presentation, both funny level sliders, personal vocabulary, and every
  dim sum capability behave as if they were not installed at all: their controls, copy,
  routes, search and palette results, previews, and notifications are omitted, not merely
  disabled;
- the dim sum surprise never draws or shows;
- the user's prior choices for all of the above are preserved and restored exactly as they
  were the moment the mode is turned back off.

Turning the mode off requires the shared, locally verified PIN or password. A passkey
option is not offered here: Qt does not provide a supported WebAuthn/platform-authenticator
API for a desktop application, so this implementation offers only PIN/password, and says so
plainly in its own preferences section rather than silently omitting the option.

## This is a user experience lock, not security

School mode is a self-imposed speed bump, exactly like the toy locks elsewhere in this
application. It is not encryption and it is not protection from anyone else who has access
to the machine. Its preferences section and its unlock prompt both say this directly, and
both name the real recovery route: delete the shared record file at the path above, which
resets the mode (and its credential) for every app that reads it.

## Live updates

The shared file is watched with `QFileSystemWatcher`. Any app on the machine, or the file
itself, changing state (turned on, turned off, or renamed) is reflected live in every other
running app within moments, with no restart required.

## Palette and settings exposure

The Experience preferences page exposes a School mode section with the current display
name, its on/off state, a rename field, and a PIN/password field for turning the mode on
(when no credential exists yet) or off. The settings index used by the command palette and
settings search carries a "School mode" row that jumps directly to this section.

## Verification

Covered by `SchoolModeStoreTests` in `src/experience/tests/schoolmode_tests.cpp`: parsing an
absent/empty record as off with the shipped name, round-tripping a record through
serialization, rejecting malformed JSON, rejecting an empty display name, verifying a
correct and an incorrect credential against a salted hash, and confirming two generated
salts differ.

The runtime service also owns a live `SchoolModeService`, so a file-watcher
update forces English, clears only the live personal-vocabulary table, and
suppresses funny-level decoration. The stored language, levels, and vocabulary
file are never rewritten, so turning the shared record off restores the exact
prior choices.

If the shared record becomes unreadable or malformed after a valid read, the
running application retains its last known mode and surfaces an unavailable
state with the read error. At startup without a usable record, the runtime uses
the conservative English/plain presentation until the unavailable state is
resolved; it never silently reports that the shared control is off.
