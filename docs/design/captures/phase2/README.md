# Phase 2 capture pass (Lane Q)

Every image here is a real capture of the built Linux binary
(`build/linux/src/app/audacity`), taken under Xvfb at 1600x1000x24 unless
noted, driven with `xdotool` for keyboard and mouse input and `import
-window root` for the capture itself. No image is a mockup or a hand
edit.

Several defects, including two that crashed the whole application, were
found and fixed across this pass and a later Lane T pass over the same
captures (see the table below). Every capture whose filename ends in
`-fixed-` or is numbered 06/13/14 was taken from a build that already
contains the Lane T fixes (defects 4, 4b, 5, 6, 7 and 8); the earlier,
superseded captures are kept alongside them as before/after evidence.

## Captures

| File | What it shows | Command used |
| --- | --- | --- |
| `01-home-light.png` | The Home page (Projects) right after dismissing the welcome dialog, light theme. | `xvfb-run -a -s "-screen 0 1600x1000x24" ./build/linux/src/app/audacity`, then `xdotool key Escape` twice, then `import -window root`. |
| `01-home-dark.png` | The same Home page with `ui/application/currentThemeCode=dark` set in `~/.config/Audacity/Audacity4Development.ini`. | Same as above with the theme key edited before launch. |
| `02-tabstrip-light.png` | (Superseded by `02-tabstrip-fixed-light.png` below; kept as before/after evidence.) The top toolbar area with a new project open, before the fix: only the "..." overflow affordance rendered, with no visible tabs. See defect 5. | `xdotool mousemove 71 27 click 1` (File menu), `xdotool mousemove 105 79 click 1` (New), then capture. |
| `02-tabstrip-fixed-light.png` | The same top toolbar area after the defect-5 fix: Home, Project, DevTools, Tracks, History and Effects tabs all render with labels, horizontally, docked to the top. | Same navigation as above, captured from the rebuilt binary. |
| `03-generatemenu-light.png` | (Superseded by `03-filemenu-fixed-light.png` below for the popup-styling defect; kept for the disabled-item behaviour it also shows.) The Generate menu open over an empty new project, before the popup fix: everything below "Plugin manager" is disabled because no track exists yet, and the dropdown itself shows the un-Material, plain black chrome described in defect 6. | `xdotool key alt+g` after opening a new project. |
| `03-filemenu-fixed-light.png` | The File menu after the defect-6 fix: a cleanly rounded, correctly coloured Material surface with no black frame. | `xdotool key alt+e` (File menu is the first opened; captured cropped to the menu itself). |
| `06-preferences-appearance-light.png` | (Superseded by `06-preferences-populated-light.png` below.) The Preferences dialog, General page, before the defect-4 fix: the left-hand page list is empty. | `xdotool mousemove 118 27 click 1` (Edit menu), `xdotool key End`, `xdotool key Return`. |
| `06-preferences-populated-light.png` | The Preferences dialog after the defect-4 fix, scrolled to the bottom of the page list: General, Appearance, Audio settings, Audio editing, Playback/Recording, Spectral display, Music, Export, Language and accessibility, Shortcuts, Plugins, Updates and Personalize are all present and clickable. The search bar's regex builder icon (defect 7) is visible at the top right as a "{" glyph. | `xdotool key alt+e`, `xdotool key End`, `xdotool key Return`, then several scroll-wheel events over the page list. |
| `12-scale2x-home-light.png` | The Home page with `QT_SCALE_FACTOR=2`. No clipping observed; the menu bar simply runs past the 1600px capture width because the scaled window is wider than the Xvfb screen, which is a capture-width limit, not an application defect. | `QT_SCALE_FACTOR=2 xvfb-run -a -s "-screen 0 1600x1000x24" ./build/linux/src/app/audacity`. |
| `13-preferences-personalize-light.png` | The Personalize preferences page, reached from the now-populated page list: Rename / Locks / Authenticator / Support Tickets tabs, the application-rename explanation text, and the "Application name" field with its current value and reset button. | Click "Personalize" in the scrolled page list. |
| `14-preferences-updates-light.png` | The Updates preferences page (defect 8): the "Automatic updates" title with its body message rendered underneath. On this Linux build `updateModel.state` reports the not-applicable state, so the message shown is the "Automatic updates are not applicable on this platform… replaced by hand" text. | Click "Updates" in the scrolled page list. |

## Not captured, and why

- **The command palette (`Ctrl+Shift+F`) and the regex builder sheet.** The
  one capture attempted was taken after the application had already
  crashed from the Preferences defect below, so the image was blank and
  was discarded rather than published as evidence of something it did
  not show. This needs a second pass once the Preferences crash is
  fixed.
- **Every Preferences page other than Personalize and Updates** (Appearance,
  Language and accessibility, Toolkit, Shortcuts, and the settings search
  results themselves) and the **super confirmation dialog**, which is
  reached from a Preferences action. The page-list and no-crash-on-close
  defects that previously blocked reaching any page safely are fixed (see
  defects 4 and 4b), so the remaining pages are now reachable; they were
  simply not captured in the time available for this lane.
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
| 4 | `src/preferences/qml/Audacity/Preferences/PersonalizePreferencesPage.qml` (root type) | Declares `Flickable { ... }` as its root type. Every other registered Preferences page (`GeneralPreferencesPage.qml`, `AppearancePreferencesPage.qml`, `AudioPreferencesPage.qml`, `EditPreferencesPage.qml`, `PlaybackPreferencesPage.qml`, `SpectrogramPreferencesPage.qml`, `MusicPreferencesPage.qml`, `ExportPreferencesPage.qml`, `ExperiencePreferencesPage.qml`, `ShortcutsPreferencesPage.qml`, `PluginPreferencesPage.qml`, `ToolkitPreferencesPage.qml`, `UpdatesPreferencesPage.qml`) is rooted at `PreferencesPage`, which declares `signal hideRequested`. `PreferencesDialog.qml` (line 191-193) creates every registered page up front and connects to that signal on each one (`obj.hideRequested.connect(...)`); because the Personalize page has no such signal, this throws `TypeError: Cannot call method 'connect' of undefined` partway through the loop. Two visible consequences: (a) the Preferences page list on the left of the dialog stays completely empty, because the loop's final `prv.pages = known` assignment is never reached once the exception fires, and (b) the application later **segfaults** (`Segmentation fault`) when the dialog is dismissed. | **Fixed** (Lane T): the page is now rooted at `PreferencesPage`, with its former `Flickable` content wrapped in a plain `Column`. The three sibling pages named in this row's exclusion note (`ExperiencePreferencesPage.qml`, `ToolkitPreferencesPage.qml`, `UpdatesPreferencesPage.qml`) were checked and were already correctly rooted at `PreferencesPage`; only Personalize had this defect. Verified with `06-preferences-populated-light.png` (page list populated) and `13-preferences-personalize-light.png` (Personalize page content visible). |
| 4b | `src/uicomponents/qml/Audacity/M3/M3BottomSheet.qml` (`NavigationSection { id: navSec }`) | A second, deeper cause of the same segfault-on-close, found while chasing defect 4: `navSec` declared no `order`, and `NavigationSection`'s own `componentComplete` asserts `order() > -1` before registering itself; a section that fails that assertion is never registered, and the dialog that had already created this bottom sheet crashed later when it was torn down and tried to find it. | **Fixed** (Lane T): added `order: 1` to `navSec`, matching the other dialog-level exclusive sections in this codebase. Verified with the smoke test in this lane's report: Preferences opened, every page visited, dialog closed with `Escape`, process confirmed still alive (`ALIVE_AFTER=yes`) afterwards. |
| 5 | `src/chronicle/qml/Audacity/Chronicle/TabStrip.qml` (`implicitWidth`) and `Component.onCompleted` | The browser-style tab strip never showed any tabs: only the "..." overflow button rendered. Traced with temporary `console.log` calls to two independent causes stacked on top of each other. First, `implicitWidth` was computed from the `ListView` that lives inside the strip, which itself fills whatever width it is handed by its parent; a dock host that sizes this item from its own implicit width therefore reads back a value that depends circularly on the size it was asked to supply, and falls back to a narrow default. Second, the strip's `sources` binding from the host toolbar can update more than once before `Component.onCompleted` runs; each such update called `reload()`, which declares tabs into `TabStripModel` and persists that declaration to disk (via `muse::Settings`) before `tabModel.load()` had ever run, so the later, real `load()` call saw non-empty stored state and believed the dock side had already been restored, silently discarding the host's `defaultDockSide: "top"` and leaving every strip permanently vertical (and, because a vertical strip's collapse threshold is checked against `width` rather than the horizontal one, effectively invisible in the horizontal toolbar). | **Fixed** (Lane T): `implicitWidth` is now computed directly from `tabModel.tabs.length` (`Math.max(M3.density.apply(160), tabCount * M3.density.apply(140) + M3.density.apply(56))`) rather than from the `ListView`, and a `modelReady` guard now makes `onSourcesChanged` a no-op until after `Component.onCompleted` has called `tabModel.load()` once. Verified with `02-tabstrip-fixed-light.png`, which shows Home, Project, DevTools, Tracks, History and Effects tabs all rendered with labels in the correct horizontal, top-docked orientation. |
| 6 | Every application menu dropdown (for example the Generate menu in `03-generatemenu-light.png`; also seen on File, Edit, and every other top menu) | Every dropdown renders as a plain, square-cornered, solid black popup with no Material styling, rather than the light `#ECE6EF` rounded `M3Roles.surfaceContainerHigh` surface that `PopupContent.qml` (`muse/framework/uicomponents/qml/Muse/UiComponents/internal/PopupContent.qml`) is written to draw. | **Root-caused and fixed** (Lane T), via the `muse` submodule patch overlay (`buildscripts/muse-patches/0003-m3-popups-and-dialogs.patch`, regenerated with `python3 buildscripts/tools/muse_patches.py regenerate`): confirmed the theory recorded in the earlier version of this row. The popup is a separate `QQuickWindow` sized a little larger than its own content so a drop shadow has room to blur outward past the content's edges; that shadow margin is only ever alpha-blended by a compositing window manager, and Xvfb runs with none, so X11 painted the margin as solid opaque black around every menu instead of a soft fade. Added an opaque `Rectangle` filling the whole popup window with `root.backgroundColor` underneath the existing shadow and content, which removes the visible black edge in both cases: a real compositor still draws the shadow on top of it as before, and a non-compositing one now shows the correct Material surface colour instead of black. Verified with `03-filemenu-fixed-light.png`, which shows a cleanly rounded, correctly coloured File menu with no black frame. |
| 7 | `src/uicomponents/qml/Audacity/M3/M3SearchBar.qml` (regex builder icon) | The button meant to open the regex builder showed a literal `[ ]` rather than an icon: it used `IconCode.BRACKET_PARENTHESES_SQUARE`, and that codepoint in the bundled `MusescoreIcon.ttf` font draws exactly that, a bracket-parenthesis shape that reads as missing-icon fallback text rather than as an icon. | **Fixed** (Lane T): swapped to `IconCode.BRACE`, which draws a single curly-brace glyph, unambiguously an icon rather than punctuation. Also gave the button `accessibleName: "Open regular expression builder"`. Confirmed the codepoint mapping against the shipped font with `fontTools`/`PIL` before making the change. Visible as the "{" glyph at the right of the search bar in `06-preferences-populated-light.png`. |
| 8 | `src/preferences/qml/Audacity/Preferences/UpdatesPreferencesPage.qml` (body content) | Reported separately from `docs/design/captures/lane-g2/preferences-updates-page.png`, which showed the "Automatic updates" title with an empty body. Every preferences page is created up front by `PreferencesDialog.qml`, long before the user navigates to it, and only becomes the visible current page later. The original page wrapped its whole body in a second `Column` whose `visible` binding flipped from false to true only once `updateModel.state` finished loading; on this build an item whose own `visible` (or an ancestor's height) starts bound to a false/zero value at creation time and is only later flipped by a state change did not reliably repaint, even though every other property, including the flipped boolean itself, reported the correct value once queried. | **Fixed** (Lane T): rewrote the body so every label stays visible, laid out and full height always, and only its own `text` switches to the empty string on a platform where updates are not applicable; only the `M3Switch` and the "Check for updates" `M3Button`, which cannot be text-blanked the same way, keep an ordinary `visible` binding. Verified with `14-preferences-updates-light.png`, which shows the "Automatic updates" title with its body message rendered underneath (this Linux build reports the not-applicable state, since Squirrel.Windows updates do not apply here). |

## Build note

Four rebuilds were required across this pass (`cmake --build build/linux
-j3 --target audacity`, under the shared build lock): once after fixing
defects 1 and 2, once more after fixing defect 3, once after fixing
defects 4/4b/5/7 together, and once more after fixing defect 8
(`UpdatesPreferencesPage.qml`), which needed its own recapture once the
Personalize/tab-strip fixes had already landed.
