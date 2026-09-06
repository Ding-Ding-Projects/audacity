# Local version history

Material Audacity keeps its own version history of your work, separate from
the undo stack. The undo stack ends when the application closes. The version
history does not.

## Where it lives

The history is an isolated repository beside the application data directory:

```
<appDataDir>/history/<project-id>.git
<appDataDir>/history/stage/<project-id>/
```

It is never placed inside your project folder. Opening a project in Material
Audacity does not add anything to the folder you keep your recordings in.

The staging directory holds the copy the history records: the project file,
the settings file and the preset files. That is why a revision reads as one
coherent picture and why the file list of a revision names files you
recognise.

### The project id

`<project-id>` is a stable identifier stored in a small table of the
project's own `.aup4` database (`chronicle_meta`), not a hash of the file's
path. It is created the first time it is needed and then read back unchanged
every time after that, so a project's history follows the project across a
rename or a move rather than starting over, and two different files that
happen to sit at the same path at different times never share one history.

A project that has never reached its own database (a genuine disk or I/O
failure reaching it) falls back to a hash of the file's path instead, exactly
as every project's history worked before this identifier existed. That
fallback history does not follow a rename, which is the one difference a
user might notice.

## The two stores

| Store | When it is used | What it does |
| ----- | --------------- | ------------ |
| Git | A `git` executable is on `PATH` | Drives that executable through `QProcess` against an isolated bare repository |
| Content addressed | No `git` on `PATH` | Stores each file once under the SHA-256 of its content, with a JSON manifest per revision |

Both stores satisfy the same interface and the same test suite, so nothing
above them behaves differently. No git library is linked into the application.
The History panel tells you which store your history is kept in.

## What triggers a revision

| Trigger | Action recorded | Example label |
| ------- | --------------- | ------------- |
| Any undoable edit (cut, paste, move a clip, apply an effect, add or delete a track, edit a label, an envelope point, and so on) | the real name the undo stack gave the edit, for example `Cut` or `Apply Amplify` | Cut |
| The project is saved | `project-save` | Saved Interview take 2 |
| The settings file changes | `settings-change` | Changed theme to dark |
| A preset is saved | `preset-save` | Saved preset Warm vocals |
| A preset is deleted | `preset-delete` | Deleted preset Warm vocals |
| A revision is restored | `restore` | Restored 9e500daf17 |
| Unsaved work is discarded on close | `discard-unsaved` | Discarded unsaved work in Interview take 2 |

Settings changes are detected by watching the settings file and are debounced,
so a page of preference changes produces one revision rather than one per key.

### Every action commits

Every edit pushed onto the project's own undo stack is recorded as a revision,
named after the edit itself, in addition to the fixed triggers above. This is
controlled by the `chronicle/commitOnEveryAction` setting, which defaults to
on. Turning it off leaves saves, settings changes, presets and restores being
recorded as before; only the per-edit revisions stop.

The application already gives each undoable edit a real name (the same name
shown next to Undo in the Edit menu), and `ProjectHistoryWatcher` listens to
the project's own history channel for a `NewState` event and records that name
directly, so a revision reads "Cut" or "Move clip" rather than a generic
"Snapshot".

A drag that Audacity's own undo stack consolidates into one entry (a
`UndoPushType::CONSOLIDATE` push, the shape a clip drag or a fader drag
already uses) produces only one `NewState` event, at the point the drag
settles, so it becomes exactly one revision rather than one revision per
intermediate frame. `Undo` and `Redo` themselves are not recorded; the states
they move between are already on record, so recording the move too would only
clutter the history.

### Action families

Every recorded action is grouped into one of ten families, so the filter
chips read the way a user thinks about their own work rather than by a raw
action name: **Edit, Clip, Track, Effect, Generate, Label, Envelope, Project
settings, Save, Restore**. The fixed triggers map onto their family exactly
(a save is always in the Save family); a free form undo action name is
matched against the family it is most likely to belong to by the words in its
name (`actionFamily()` in `versionhistoryservice.cpp`, with its mapping
covered by `snapshotstore_tests.cpp`). An action that matches nothing
recognised stays in Edit rather than being dropped, so it is still reachable
through some filter chip.

A save and a restore are also marked as milestones (`isMilestoneAction()`),
which the panel can use to give them their own icon in the list.

Discarding unsaved work is recorded **before** the work is gone, so the entry
that describes the loss is itself part of the history.

## Append only

The history is never rewritten.

- A restore writes the old content back into the staging directory and is then
  recorded as a **new** revision. The state before the restore is still there,
  so a restore can always be undone by restoring the revision before it.
- Editing a label changes the label only. In the git store labels are kept in
  `chronicle-labels.json` beside the repository rather than in the commit
  messages, so a label edit never rewrites a commit.
- Retention hides revisions outside the window. The content addressed store
  also deletes their payload and any object nothing else refers to. The git
  store does not rewrite commits, so its object payload is reclaimed only when
  the whole history repository is removed. That is the price of a history that
  is honestly append only, and it is stated here rather than hidden.

## The History panel

The panel sits beside the undo history, behind the "Versions" segment.

- Each row shows the time, the label describing what changed, and the action
  that triggered the snapshot.
- Opening a row shows the diff summary: every file in the revision, its status
  against the previous revision, and its size.
- The row's actions are **Restore**, **Edit label** and **Export…**, which
  writes the content of the revision into a folder you choose.

### Filters

- A date range with an anchored `M3DatePicker` at each end, presets for today,
  the last 7 days, the last 30 days and all time, and typed ISO dates in the
  two fields.
- Action filter chips, derived from the actions actually recorded, each with
  its count.
- An `M3SearchBar` with `showRegexBuilder: true`. The search term is used as a
  regular expression when it compiles as one and as plain text otherwise, so a
  typed bracket never empties the list.

### Retention

Two settings control pruning:

| Setting key | Meaning | Default |
| ----------- | ------- | ------- |
| `chronicle/retentionCount` | Keep at most this many revisions | 200 |
| `chronicle/retentionDays` | Keep revisions younger than this many days | 90 |

The newest revision is never pruned. Retention is applied at start up and from
the **Apply retention** button in the panel.

## The history travels inside the save file

Turning `chronicle/embedHistoryInSaveFile` on (the default) packs the whole
local history and writes it into a `chronicle_bundle` table in the project's
own aup4 database, right after the project itself finishes saving. The
history a project has recorded is then part of the file: copy the file to
another machine, and its history comes with it.

### What is embedded, and how

| Store | Format written | How it is produced |
| ----- | --------------- | ------------------- |
| Git | `git-bundle` | `git bundle create <file> --all` |
| Content addressed | `chronicle-file-store-v1` | A single JSON document holding the manifest plus every object, base64 encoded |

Both are produced by `ISnapshotStore::packHistory()` and consumed by
`ISnapshotStore::unpackHistory()`, so the rest of the application never has to
know which one is in use; only the format string travels alongside the bytes
so a future reader knows which store to hand them to.

### When it is read back

Opening a project reads whatever is embedded and offers it to the local
store through `unpackHistory()`. The merge is **fast forward only**: it is
adopted when it is at least as advanced as what this machine already has for
that project id, and otherwise the embedded copy is quietly ignored and the
local history is left exactly as it was. This is what makes it safe to open
the same project on two machines without either one's history ever
overwriting or discarding the other's revisions; the one thing it does not
attempt is combining two histories that have genuinely diverged into one.

### The save itself always succeeds

Embedding happens after the project has already saved, and it can fail on
its own: `writeChronicleBundle()` returns false when the project's database
cannot be reached or the write itself does not go through. A failure there
never touches the save's own result. The user instead sees a non-blocking
notification saying that the project saved correctly but its history could
not be embedded this time, and whatever was embedded before, if anything, is
left in place untouched.

## Failure modes

| What goes wrong | What happens |
| --------------- | ------------ |
| `git` is not installed | The content addressed store is used and the panel says so |
| `git` is installed but the repository cannot be created | The application falls back to the content addressed store and logs a warning |
| A git command times out (30 s) | The process is killed, the snapshot fails and the failure is logged; the panel simply shows no new revision |
| A history object is missing during a restore | The restore fails and reports it, and nothing in the working copy is left half written |
| The staging copy fails | The revision records what could be staged; an empty revision is still recorded rather than silently skipped |
| Embedding the history into the save file fails | The save itself has already succeeded; a non-blocking notification says the history was not embedded this time, and the previously embedded copy, if any, is left untouched |
| An embedded bundle is older, unrelated, or already present | It is not adopted; the local history is left exactly as it was, and nothing is reported since nothing went wrong |

## Accessibility

- The revision list is a muse navigation panel with vertical direction, so the
  arrow keys move between revisions and every row carries an accessible name
  combining its label, its action and its time.
- The filter chips, the date fields, the calendar buttons and the retention
  fields are all in the same navigation panel and reachable from the keyboard.
- Status is never carried by colour alone: every file row spells its status out
  in words next to the size.

## Verification

- `src/chronicle/tests/snapshotstore_tests.cpp` runs the whole store contract
  against both backends in a temporary directory: commit, list, file sizes and
  statuses, restore, restore recorded as a new revision with the older
  identifiers unchanged, label edit, export and retention.
- The same file asserts the label derivation, which is what turns an action
  into "Deleted track Vocals" or "Changed theme to dark".
- The git backed test skips itself when `git` is not on `PATH`, and says so, so
  a skipped test is never mistaken for a passing one.
- `ActionFamilyGroupsFixedActionsTheWayAUserThinksAboutThem`,
  `ActionFamilyGuessesTheFamilyOfAFreeFormUndoActionName`,
  `ActionFamilyTitlesAreHumanReadable` and `OnlySaveAndRestoreAreMilestones`
  cover the family mapping and the milestone flag added above.
- `Au3ProjectMetadata.*` in `src/au3wrap/tests/projectmetadata_tests.cpp`
  covers the stable project id (generated once, read back unchanged, distinct
  per database, an unreachable database returning empty rather than crashing)
  and the embedded bundle table (write and read round trip exactly, a second
  write replaces rather than appends, reading with nothing embedded yet
  returns empty, an unreachable database and an empty byte array are both
  refused rather than crashing).
- `FallbackStorePackAndUnpackRoundTripsTheWholeHistory` and
  `GitBackedStorePackAndUnpackRoundTripsTheWholeHistory` in
  `snapshotstore_tests.cpp` pack a store with two real revisions, unpack it
  into a freshly opened second store, and assert every revision id and label
  came across along with a genuinely restorable file (exported and read back
  byte for byte), then re-apply the same bundle and assert nothing was lost
  or duplicated by doing so again.

## What this feature does not do yet

Stated plainly rather than left as a silent gap:

- The panel does not yet show a waveform thumbnail of the affected region, a
  day by day timeline chip rail, a two revision compare view, or a repository
  size and per track sample data storage panel. It does not yet let a version
  be starred or pinned against retention, or opened as a separate project
  without touching the one that is open.
- These are the parts of the local history contract still open. The action
  recording, the family grouping and the append only storage described above
  are real and tested; the richer panel surfaces are not built.
