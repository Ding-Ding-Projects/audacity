# Phase 2 capture pass (Lane Q)

Every image here is a real capture of the built Linux binary
(`build/linux/src/app/audacity`), taken under Xvfb at 1600x1000x24 unless
noted, driven with `xdotool` for keyboard and mouse input and `import
-window root` for the capture itself. No image is a mockup or a hand
edit.

Two application-crashing defects were found and fixed during this pass
(see the table below); every capture in this folder was taken from a
build that already contains those fixes.

## Captures

| File | What it shows | Command used |
| --- | --- | --- |
| `01-home-light.png` | The Home page (Projects) right after dismissing the welcome dialog, light theme. | `xvfb-run -a -s "-screen 0 1600x1000x24" ./build/linux/src/app/audacity`, then `xdotool key Escape` twice, then `import -window root`. |
| `01-home-dark.png` | The same Home page with `ui/application/currentThemeCode=dark` set in `~/.config/Audacity/Audacity4Development.ini`. | Same as above with the theme key edited before launch. |
| `02-tabstrip-light.png` | The top toolbar area with a new project open. Intended to show the browser style tab strip; instead it shows only the "..." overflow affordance with no visible tabs. See the defects table. | `xdotool mousemove 71 27 click 1` (File menu), `xdotool mousemove 105 79 click 1` (New), then capture. |
| `03-generatemenu-light.png` | The Generate menu open over an empty new project. Everything below "Plugin manager" is disabled because no track exists yet, so a tone could not actually be generated from this state; the image also shows the un-Material, plain black dropdown chrome common to every application menu popup (see defects table). | `xdotool key alt+g` after opening a new project. |
| `06-preferences-appearance-light.png` | The Preferences dialog, General page, immediately after opening it from Edit > Preferences. The left-hand page list is empty (see defects table); only the current page's content renders. | `xdotool mousemove 118 27 click 1` (Edit menu), `xdotool key End`, `xdotool key Return`. |
| `12-scale2x-home-light.png` | The Home page with `QT_SCALE_FACTOR=2`. No clipping observed; the menu bar simply runs past the 1600px capture width because the scaled window is wider than the Xvfb screen, which is a capture-width limit, not an application defect. | `QT_SCALE_FACTOR=2 xvfb-run -a -s "-screen 0 1600x1000x24" ./build/linux/src/app/audacity`. |

## Not captured, and why

- **The command palette (`Ctrl+Shift+F`) and the regex builder sheet.** The
  one capture attempted was taken after the application had already
  crashed from the Preferences defect below, so the image was blank and
  was discarded rather than published as evidence of something it did
  not show. This needs a second pass once the Preferences crash is
  fixed.
- **Every other Preferences page** (Appearance, Language and accessibility,
  Personalize, Toolkit, Updates, Shortcuts, and the settings search) and
  the **super confirmation dialog**, which is reached from a Preferences
  action. Opening Preferences at all currently loads the Personalize page
  in a way that throws (see defects table); closing the dialog afterwards
  segfaulted the whole process. Capturing the remaining Preferences pages
  safely needs that fix to land first.
- **The version history panel and the changelog dialog.** Both QML files
  exist and are registered (`Audacity/Chronicle/VersionHistoryPanel.qml`,
  `Audacity/Chronicle/ChangelogDialog.qml`, reachable via the `whats-new`
  action already wired into Help), but the History dock panel referenced
  from the new tab strip (`dockPanels` in `WindowContent.qml`) has no
  `uri`, so there is nothing to actually open it from a running window
  yet. Not captured for lack of a real path to it in this build.
- **A notification toast and the notification centre**, the **Ollama page
  offline state and the docs browser**, and the **updater banner** with
  `AU_SQUIRREL_DEMO_BANNER=1`. Not reached in the time available for this
  pass; no defect is claimed for any of them one way or the other.
- **The 900x700 narrow window check.** `xdotool windowsize` against the
  running window did not change its reported size (still captured at the
  full 1600x1000 launch geometry), so no narrow-width evidence exists yet.
  This needs a different resize mechanism (for example launching Xvfb
  itself at the smaller geometry) rather than resizing the frameless
  window after the fact.

## Defects found

| # | File | Symptom | Status |
| --- | --- | --- | --- |
| 1 | `src/squirrelupdate/qml/Audacity/SquirrelUpdate/UpdateBanner.qml` (lines 43, 51) | Read `M3.type.titleSmall` / `M3.type.bodySmall`. The M3 singleton has no `type` property, only `typography` (`M3ThemeProvider::typography`, `src/uicomponents/components/m3themeprovider.h`). Every label on the update banner rendered with an undefined font and printed `TypeError: Cannot read property 'titleSmall' of undefined` on every load. | **Fixed** in this lane (commit `bd18745`): both lines now read `M3.typography.*`. |
| 2 | `src/preferences/qml/Audacity/Preferences/UpdatesPreferencesPage.qml` (lines 45, 53, 78, 86, 94, 101, 110, 119) | Same `M3.type.*` mistake, same undefined-font symptom, on the Updates preferences page. | **Fixed** in this lane (commit `bd18745`): all eight occurrences now read `M3.typography.*`. |
| 3 | `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/ClipItemPropertyButton.qml` (line 29) | Bound `Accessible.onPress: root.clicked(null)`. `Accessible` has no `press` signal; the real signal is `pressAction` (`QQuickAccessibleAttached::pressAction()`), so the handler must be `onPressAction`. Binding to a non-existent property is a hard QML load error, and because `ClipItemPropertyButton` is used inside `ClipItem`, which is used inside `TrackClipsContainer`, which is used inside `TracksItemsView`, this took down the **entire main window**: `GuiApplication::loadMainWindow` failed with `Type WindowContent unavailable ... Cannot assign to non-existent property "onPress"` every time a project with the track view loaded. | **Fixed** in this lane (commit `bd18745`): the line now reads `Accessible.onPressAction: root.clicked(null)`. Verified by rebuilding and reloading a project after the fix; the main window now loads and the track view renders. |
| 4 | `src/preferences/qml/Audacity/Preferences/PersonalizePreferencesPage.qml` (root type) | Declares `Flickable { ... }` as its root type. Every other registered Preferences page (`GeneralPreferencesPage.qml`, `AppearancePreferencesPage.qml`, `AudioPreferencesPage.qml`, `EditPreferencesPage.qml`, `PlaybackPreferencesPage.qml`, `SpectrogramPreferencesPage.qml`, `MusicPreferencesPage.qml`, `ExportPreferencesPage.qml`, `ExperiencePreferencesPage.qml`, `ShortcutsPreferencesPage.qml`, `PluginPreferencesPage.qml`, `ToolkitPreferencesPage.qml`, `UpdatesPreferencesPage.qml`) is rooted at `PreferencesPage`, which declares `signal hideRequested`. `PreferencesDialog.qml` (line 191-193) creates every registered page up front and connects to that signal on each one (`obj.hideRequested.connect(...)`); because the Personalize page has no such signal, this throws `TypeError: Cannot call method 'connect' of undefined` partway through the loop. Two visible consequences: (a) the Preferences page list on the left of the dialog stays completely empty, because the loop's final `prv.pages = known` assignment is never reached once the exception fires, and (b) the application later **segfaults** (`Segmentation fault`) when the dialog is dismissed. | **Not fixed here.** `PersonalizePreferencesPage.qml` matches the `Personalize*` exclusion in this lane's file ownership (owned by the Personalize lane). Reported for that lane to root the page at `PreferencesPage` instead of `Flickable`, or to add the `hideRequested` signal directly. |
| 5 | `src/appshell/qml/Audacity/AppShell/MainToolBar.qml` (root `Item`) | The browser-style tab strip described in the file's own header comment never showed any tabs: only the "..." overflow button rendered, at a width matching the left navigation rail rather than the top toolbar area. The `Item` root computed its own `width` from `tabStrip.implicitWidth` but never reported `implicitWidth`/`implicitHeight` upward, so the KDDockWidgets-backed dock toolbar host (`muse/framework/dockwindow`) that hosts it sized it from whatever narrow default it falls back to. | **Partially fixed** in this lane (commit pending): added `implicitWidth: root.width` / `implicitHeight: root.height` to `MainToolBar.qml`, which is directionally correct for any dock host that reads implicit size, but a rebuild and recapture (`02-tabstrip-light.png`) still shows only the overflow button. The dock toolbar's actual allotted width is set somewhere in the KDDockWidgets layout/registration path that this lane did not track down in the time available; this needs further investigation, most likely in `muse/framework/dockwindow` (out of this lane's file ownership) or in how `mainToolBar`'s `DockToolBar` is registered in `WindowContent.qml`. |
| 6 | Every application menu dropdown (for example the Generate menu in `03-generatemenu-light.png`; also seen on File, Edit, and every other top menu) | Every dropdown renders as a plain, square-cornered, solid black popup with no Material styling, rather than the light `#ECE6EF` rounded `M3Roles.surfaceContainerHigh` surface that `PopupContent.qml` (`muse/framework/uicomponents/qml/Muse/UiComponents/internal/PopupContent.qml`) is written to draw. Traced as far as: the menu bar itself (`AppMenuBar.qml`) is genuinely M3-styled and confirmed `isGlobalMenuAvailable()` is false in this environment (so it is the QML `StyledMenu` popup being used, not a native menu); `PopupContent.qml`'s background `Rectangle` reads `M3Roles.surfaceContainerHigh`, which correctly falls back to `ui.theme.popupBackgroundColor` (`#ECE6EF` in `src/app/configs/light.cfg`) since nothing yet publishes `m3_surface_container_high` into `ui.theme.extra`. The most likely explanation given the fully solid black result (not merely "wrong colour") is that the popup is a separate, nominally translucent `QQuickWindow` and Xvfb has no compositing window manager running, so X11 cannot blend its alpha channel and paints it as opaque black wherever the app did not explicitly composite over black itself; this would not reproduce on a real desktop with a compositor. **Not confirmed either way in the time available**, and not fixed: the popup's window/background handling lives in `muse/framework/uicomponents`, outside every lane's file ownership in this pass. Recorded here so a lane with time to test against a real compositor (or add `m3_*` theme keys as the documented bridge) can confirm which it is. |

## Build note

Two rebuilds were required during this pass (`cmake --build build/linux
-j3 --target audacity`, under the shared build lock): once after fixing
defects 1 and 2, and once more after fixing defect 3, since defect 3 was
only discovered by driving the app after the first rebuild.
