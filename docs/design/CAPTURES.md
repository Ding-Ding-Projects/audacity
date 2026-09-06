# Deterministic capture routes

This file documents the debug-only hooks that make a screen capture reproducible: given
the same hook, the same page or dialog state opens the same way every time, rather than
depending on clicking through the interface by hand under Xvfb.

These hooks are read once from an environment variable at startup. There is no setting
anywhere that exposes them; they exist only for a script that already controls the process
environment (the capture harness), exactly like `AU_ALLOW_MULTIPLE_PROCESSES` and
`AU_DIM_SUM_FORCE` already do elsewhere in this codebase.

## Opening Preferences on an exact page and section: `AU_OPEN_PREFERENCES`

Set `AU_OPEN_PREFERENCES` to `<pageId>` or `<pageId>#<sectionObjectName>` before launching
the application. Once the main window is up, `au::preferences::PreferencesModule::onStartApp()`
opens the Preferences dialog through the ordinary `audacity://preferences` URI (the same
route the `preference-dialog` action and the toolbar gear icon use), passing `currentPageId`
and, when a section name was given, `highlightObjectName`.

```
AU_OPEN_PREFERENCES="experience#ExperienceSchoolModeSection" ./audacity
AU_OPEN_PREFERENCES="experience#ExperienceNarratorSection" ./audacity
AU_OPEN_PREFERENCES="updates" ./audacity
```

`<pageId>` is one of the ids `PreferencesModel` registers (see
`src/preferences/qml/Audacity/Preferences/preferencesmodel.cpp`), for example `general`,
`appearance`, `experience`, `updates`, `personalize`. `<sectionObjectName>` is the
`objectName` of a section component inside that page, for example
`ExperienceSchoolModeSection` or `ExperienceNarratorSection` (see
`src/preferences/qml/Audacity/Preferences/internal/Experience*.qml`); add an `objectName`
to a section that does not have one yet before pointing this hook at it.

When a section name is given, `PreferencesDialog.qml` walks the loaded page looking for a
descendant whose `objectName` matches, then scrolls the nearest ancestor `Flickable` so that
descendant sits at the top of the visible area. A page freshly switched into the dialog's
`StackLayout` can still be mid-layout for a frame or two (its `Flickable` briefly reports a
zero height and a zero content height), so this retries on a short timer for up to about a
second before giving up quietly.

### Recipe

```bash
export QT_QPA_PLATFORM=xcb
export AU_ALLOW_MULTIPLE_PROCESSES=1
export AU_OPEN_PREFERENCES="experience#ExperienceSchoolModeSection"
xvfb-run -a -s "-screen 0 1600x1000x24" bash -c '
  ./build/linux/src/app/audacity &
  PID=$!
  sleep 10
  import -window root capture.png
  kill $PID
  wait $PID
'
```

Ten seconds covers the one-shot startup delay before the hook resolves the window's
`IInteractive` and opens the dialog, plus the retry timer settling the scroll position. A
"the previous session quit unexpectedly, restore?" recovery dialog can appear on top of an
already-open Preferences dialog on a profile that was not shut down cleanly (for example a
capture script's own prior run); use a fresh application data directory (a fresh `HOME`) to
avoid it, or dismiss it before capturing.

## Fetching a real dim sum photo for a capture: `AU_DIM_SUM_FORCE`

Documented in `docs/features/dim-sum-surprise.md`. Forces the one-shot draw to win instead
of leaving it to the real 10% chance, so a capture does not have to launch the application
repeatedly hoping to get lucky. It does not force which dish is picked; a real photo only
appears once that dish's photo has actually been fetched and cached (typically on a later
launch of the same profile, since the first launch that picks a given dish starts its photo
fetch in the background rather than blocking on it).
