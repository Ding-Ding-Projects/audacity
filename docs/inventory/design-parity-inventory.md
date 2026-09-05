# Design parity inventory

This project has no checked-in design reference folder (no `design/`,
`designs/`, or `design-reference/` directory exists in this repository), so
there is nothing to render a dedicated reference application against. The
reference for every screen listed below is the Material Design 3
specification itself, applied through the shared token and component
library at `src/uicomponents/qml/Audacity/M3/`. The reference column for
every row therefore reads `none: Material 3 spec` rather than naming a file
that does not exist; this is a deliberate, documented deviation from the
checked-in-reference-folder contract, recorded here rather than left as a
silent gap.

Every screen the application has is listed once. The audit column names any
control that still comes from `Muse.Ui` or `Muse.UiComponents` (the
pre-Material chrome this project is replacing) rather than from the local
`Audacity.M3` library; a screen with an empty audit finding is not
necessarily fully audited, it means the routine grep in this pass found
nothing suspicious, not that a full one-control-at-a-time review has been
done.

Every capture, comparison, and diff column is empty. No headless capture
route has been run for this repository yet; that is real, uncompleted work
and is recorded honestly rather than invented.

## Columns

- **Screen** the human name of the surface.
- **Reference** always `none: Material 3 spec` here (see above).
- **Route** the entry point in the running application (menu path or QML
  file loaded).
- **State** which state of the surface this row covers (its default state,
  unless noted).
- **Theme** light or dark; a row without both is not yet audited in both.
- **Viewport** the window size the row was checked at, empty if not yet run.
- **Scale** the display scale factor, empty if not yet run.
- **M3 primitive audit** legacy (`Muse.Ui` / `Muse.UiComponents`) controls
  found by inspecting the QML file's imports and control usage; "none found"
  means the routine grep found nothing, not that every pixel has been
  reviewed against the specification.
- **Raw capture** path to a raw screenshot, empty if none exists.
- **Side by side** path to a labelled comparison image, empty if none exists.
- **Diff** path to machine readable visual diff evidence, empty if none exists.
- **Deviation** any intentional, reviewed difference from the M3 spec and
  its reason; empty means none recorded yet, not that none exist.

## The screens

| Screen | Reference | Route | State | Theme | Viewport | Scale | M3 primitive audit | Raw capture | Side by side | Diff | Deviation |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| Home page | none: Material 3 spec | `src/appshell/qml/Audacity/AppShell/HomePage/HomePage.qml` | default | | | | not yet audited | `docs/design/captures/lane-a/00-home-welcome.png`, `docs/design/captures/phase1/00-home.png`, `docs/design/captures/phase2/home-light.png` | | | |
| Home page, projects list | none: Material 3 spec | `src/appshell/qml/Audacity/AppShell/HomePage/HomePage.qml` | projects tab | | | | not yet audited | `docs/design/captures/lane-a/01-home-projects.png` | | | |
| Home page, plugins page | none: Material 3 spec | `src/appshell/qml/Audacity/AppShell/HomePage/PluginsPage.qml` | default | | | | not yet audited | | | | |
| Home page, home menu | none: Material 3 spec | `src/appshell/qml/Audacity/AppShell/HomePage/HomeMenu.qml` | open | | | | not yet audited | | | | |
| Project page | none: Material 3 spec | `src/project/qml/Audacity/Project/ProjectsPage.qml` | default | | | | not yet audited | `docs/design/captures/lane-b/05-project-page.png`, `docs/design/captures/lane-d/06-project.png` | | | |
| Project page, empty project | none: Material 3 spec | (project scene root) | empty | | | | not yet audited | `docs/design/captures/lane-c/01-empty-project.png` | | | |
| Project page, one track | none: Material 3 spec | (project scene root) | one track | | | | not yet audited | `docs/design/captures/lane-c/03-one-track.png` | | | |
| Project page, track with clip | none: Material 3 spec | (project scene root) | track with clip | | | | not yet audited | `docs/design/captures/lane-c/05-track-with-clip.png` | | | |
| Project page, track context menu | none: Material 3 spec | (project scene root) | context menu open | | | | not yet audited | `docs/design/captures/lane-c/04-track-context-menu.png` | | | |
| Project page, add track popup | none: Material 3 spec | (project scene root) | popup open | | | | not yet audited | `docs/design/captures/lane-c/02-add-track-popup.png` | | | |
| History panel | none: Material 3 spec | `src/projectscene/qml/Audacity/ProjectScene/historypanel/HistoryPanel.qml` | default | | | | not yet audited | `docs/design/captures/lane-c/06-history-panel.png` | | | |
| Version history panel | none: Material 3 spec | `src/chronicle/qml/Audacity/Chronicle/VersionHistoryPanel.qml` | default | | | | not yet audited | | | | |
| Preferences dialog, shell | none: Material 3 spec | `src/preferences/qml/Audacity/Preferences/PreferencesDialog.qml` | default | | | | not yet audited | `docs/design/captures/phase1/01-preferences.png`, `docs/design/captures/lane-d/01-preferences.png` | | | |
| Preferences, page list | none: Material 3 spec | `src/preferences/qml/Audacity/Preferences/PreferencesPage.qml` | default | | | | none found in routine grep | | | | |
| Preferences, search results | none: Material 3 spec | `src/preferences/qml/Audacity/Preferences/PreferencesDialog.qml` | searching | | | | not yet audited | `docs/design/captures/lane-d/05-preferences-search.png` | | | |
| Preferences, General page | none: Material 3 spec | `src/preferences/qml/Audacity/Preferences/GeneralPreferencesPage.qml` | default | | | | not yet audited | | | | |
| Preferences, Appearance page | none: Material 3 spec | `src/preferences/qml/Audacity/Preferences/AppearancePreferencesPage.qml` | default | | | | not yet audited | `docs/design/captures/lane-d/15-preferences-material-theme.png` | | | |
| Preferences, Audio page | none: Material 3 spec | `src/preferences/qml/Audacity/Preferences/AudioPreferencesPage.qml` | default | | | | not yet audited | `docs/design/captures/lane-d/04-preferences-audio.png` | | | |
| Preferences, Playback page | none: Material 3 spec | `src/preferences/qml/Audacity/Preferences/PlaybackPreferencesPage.qml` | default | | | | not yet audited | | | | |
| Preferences, Edit page | none: Material 3 spec | `src/preferences/qml/Audacity/Preferences/EditPreferencesPage.qml` | default | | | | not yet audited | | | | |
| Preferences, Export page | none: Material 3 spec | `src/preferences/qml/Audacity/Preferences/ExportPreferencesPage.qml` | default | | | | not yet audited | `docs/design/captures/lane-d/16-preferences-export.png` | | | |
| Preferences, Experience page | none: Material 3 spec | `src/preferences/qml/Audacity/Preferences/ExperiencePreferencesPage.qml` | default | | | | not yet audited | | | | |
| Preferences, Personalize page | none: Material 3 spec | `src/preferences/qml/Audacity/Preferences/PersonalizePreferencesPage.qml` | default | | | | not yet audited | | | | |
| Preferences, Toolkit page | none: Material 3 spec | `src/preferences/qml/Audacity/Preferences/ToolkitPreferencesPage.qml` | default | | | | not yet audited | | | | |
| Preferences, Plugin page | none: Material 3 spec | `src/preferences/qml/Audacity/Preferences/PluginPreferencesPage.qml` | default | | | | not yet audited | | | | |
| Preferences, Shortcuts page | none: Material 3 spec | `src/preferences/qml/Audacity/Preferences/ShortcutsPreferencesPage.qml` | default | | | | imports `Muse.Shortcuts` for shortcut editing widgets; no `Muse.UiComponents` legacy chrome found by routine grep | | | | |
| Preferences, Advanced page | none: Material 3 spec | `src/preferences/qml/Audacity/Preferences/AdvancedPreferencesPage.qml` | default | | | | not yet audited | | | | |
| Preferences, Updates page | none: Material 3 spec | `src/preferences/qml/Audacity/Preferences/UpdatesPreferencesPage.qml` | default | | | | not yet audited | | | | |
| Preferences, Spectrogram page | none: Material 3 spec | `src/preferences/qml/Audacity/Preferences/SpectrogramPreferencesPage.qml` | default | | | | not yet audited | | | | |
| Preferences, MIDI device mapping page | none: Material 3 spec | `src/preferences/qml/Audacity/Preferences/MidiDeviceMappingPreferencesPage.qml` | default | | | | not yet audited | | | | |
| Preferences, Music page | none: Material 3 spec | `src/preferences/qml/Audacity/Preferences/MusicPreferencesPage.qml` | default | | | | not yet audited | | | | |
| About dialog, Audacity tab | none: Material 3 spec | `src/appshell/qml/Audacity/AppShell/AboutDialog.qml`, `AboutDialogAudacityTab.qml` | default | | | | imports `Muse.Ui`, `Muse.UiComponents`; uses `StyledDialogView` and `NavigationPanel` from that legacy library rather than an `M3Dialog` | `docs/design/captures/lane-b/09-about-dialog.png` | | | |
| About dialog, Privacy tab | none: Material 3 spec | `src/appshell/qml/Audacity/AppShell/AboutDialogPrivacyTab.qml` | privacy tab | | | | shares the legacy `StyledDialogView` shell above | | | | |
| Welcome dialog | none: Material 3 spec | `src/appshell/qml/Audacity/AppShell/WelcomeDialog.qml` | default | | | | imports `Muse.Ui`, `Muse.UiComponents`, `Muse.GraphicalEffects`; uses `StyledDialogView` rather than an `M3Dialog` | `docs/design/captures/lane-b/04-welcome-dialog.png` | | | |
| Sign in to audio.com dialog | none: Material 3 spec | `src/appshell/qml/Audacity/AppShell/SigninAudiocomDialog.qml` | default | | | | imports `Muse.Ui`, `Muse.UiComponents`; uses `StyledDialogView` rather than an `M3Dialog` | `docs/design/captures/lane-b/09-signin-audiocom-dialog.png` | | | |
| Alpha welcome popup | none: Material 3 spec | `src/appshell/qml/Audacity/AppShell/AlphaWelcomePopup.qml` | default | | | | not yet audited | | | | |
| First launch setup, language page | none: Material 3 spec | `src/appshell/qml/Audacity/AppShell/FirstLaunchSetup/LanguagePage.qml` | default | | | | not yet audited | `docs/design/captures/lane-b/01-first-launch-language.png` | | | |
| First launch setup, themes page | none: Material 3 spec | `src/appshell/qml/Audacity/AppShell/FirstLaunchSetup/FirstLaunchSetupDialog.qml` | themes step | | | | not yet audited | `docs/design/captures/lane-b/02-first-launch-themes.png` | | | |
| First launch setup, seed colour page | none: Material 3 spec | `src/appshell/qml/Audacity/AppShell/FirstLaunchSetup/FirstLaunchSetupDialog.qml` | seed colour step | | | | not yet audited | `docs/design/captures/lane-b/03-first-launch-seed-colour.png` | | | |
| Main window content, top toolbar | none: Material 3 spec | `src/appshell/qml/Audacity/AppShell/MainToolBar.qml` | default | | | | not yet audited | | | | |
| Main window content, title bar | none: Material 3 spec | `src/appshell/qml/Audacity/AppShell/M3AppTitleBar.qml` | default | | | | none found in routine grep (name suggests M3 native) | | | | |
| DevTools page | none: Material 3 spec | `src/appshell/qml/Audacity/AppShell/DevTools/DevToolsPage.qml` | default | | | | not yet audited | `docs/design/captures/lane-b/11-devtools.png` | | | |
| DevTools, sample dialog | none: Material 3 spec | `src/appshell/qml/Audacity/AppShell/DevTools/Interactive/SampleDialog.qml` | default | | | | not yet audited | | | | |
| Edit menu | none: Material 3 spec | (application menu, Edit) | open | | | | not yet audited | `docs/design/captures/lane-d/02-editmenu.png` | | | |
| File menu | none: Material 3 spec | (application menu, File) | open | | | | not yet audited | `docs/design/captures/lane-d/10-filemenu.png` | | | |
| Generate menu | none: Material 3 spec | (application menu, Generate) | open | | | | not yet audited | `docs/design/captures/lane-d/07-generate-menu.png` | | | |
| Help menu | none: Material 3 spec | (application menu, Help) | open | | | | not yet audited | `docs/design/captures/lane-b/07-help-menu.png` | | | |
| Effect menu and submenu | none: Material 3 spec | (application menu, Effect) | open | | | | not yet audited | `docs/design/captures/lane-d/12-effect-menu.png`, `docs/design/captures/lane-d/12-effect-submenu.png` | | | |
| Effect, Amplify dialog | none: Material 3 spec | (effect dialog) | default | | | | not yet audited | `docs/design/captures/lane-d/13-effect-amplify.png` | | | |
| Generate tone dialog | none: Material 3 spec | (generate dialog) | default | | | | not yet audited | `docs/design/captures/lane-d/08-generate-tone.png` | | | |
| Plugin manager top panel | none: Material 3 spec | `src/effects/effects_base/qml/Audacity/Effects/PluginManagerTopPanel.qml` | default | | | | not yet audited | `docs/design/captures/lane-d/09-plugin-manager.png` | | | |
| Export dialog | none: Material 3 spec | `src/toolkit/qml/Audacity/Toolkit/ExportSheet.qml` | default | | | | not yet audited | `docs/design/captures/lane-d/14-export.png` | | | |
| Command palette | none: Material 3 spec | `src/companion/qml/Audacity/Companion/CommandPalette.qml` | open | | | | not yet audited | | | | |
| Regex builder sheet | none: Material 3 spec | `src/companion/qml/Audacity/Companion/RegexBuilderSheet.qml` | open | | | | not yet audited | | | | |
| Local model manager (Ollama) page | none: Material 3 spec | `src/toolkit/qml/Audacity/Toolkit/OllamaPage.qml` | default | | | | not yet audited | | | | |
| Documentation browser page | none: Material 3 spec | `src/toolkit/qml/Audacity/Toolkit/DocsBrowserPage.qml` | default | | | | not yet audited | | | | |
| Toy lock, wizard popover | none: Material 3 spec | `src/personalize/qml/Audacity/Personalize/LockWizardPopover.qml` | open | | | | not yet audited | | | | |
| Toy lock, unlock popover | none: Material 3 spec | `src/personalize/qml/Audacity/Personalize/LockUnlockPopover.qml` | open | | | | not yet audited | | | | |
| Appearance editor popover | none: Material 3 spec | `src/personalize/qml/Audacity/Personalize/AppearanceEditorPopover.qml` | open | | | | not yet audited | | | | |
| Support Tickets page | none: Material 3 spec | `src/personalize/qml/Audacity/Personalize/SupportTicketsPage.qml` | default | | | | not yet audited | | | | |
| Authenticator page | none: Material 3 spec | `src/personalize/qml/Audacity/Personalize/AuthenticatorPage.qml` | default | | | | not yet audited | | | | |
| Update banner | none: Material 3 spec | `src/squirrelupdate/qml/Audacity/SquirrelUpdate/UpdateBanner.qml` | update ready | | | | not yet audited | | | | |
| Recovery card | none: Material 3 spec | `src/toolkit/qml/Audacity/Toolkit/RecoveryCard.qml` | default | | | | not yet audited | | | | |
| Notification centre | none: Material 3 spec | `src/experience/qml/Audacity/Experience/NotificationCentre.qml` | open | | | | not yet audited | | | | |
| Notification toast | none: Material 3 spec | `src/experience/qml/Audacity/Experience/ExperienceToast.qml` | shown | | | | not yet audited | | | | |
| Super confirmation dialog | none: Material 3 spec | `src/experience/qml/Audacity/Experience/SuperConfirmationDialog.qml` | open | | | | not yet audited | | | | |
| Schedule edit sheet | none: Material 3 spec | `src/experience/qml/Audacity/Experience/ScheduleEditSheet.qml` | open | | | | not yet audited | | | | |
| Changelog dialog | none: Material 3 spec | `src/chronicle/qml/Audacity/Chronicle/ChangelogDialog.qml` | open | | | | not yet audited | | | | |
| Tab strip | none: Material 3 spec | `src/chronicle/qml/Audacity/Chronicle/TabStrip.qml` | default | | | | not yet audited | | | | |
| Tab group appearance popup | none: Material 3 spec | `src/chronicle/qml/Audacity/Chronicle/TabGroupAppearancePopup.qml` | open | | | | not yet audited | | | | |
| Close tabs popup | none: Material 3 spec | `src/chronicle/qml/Audacity/Chronicle/CloseTabsPopup.qml` | open | | | | not yet audited | | | | |
| Component gallery | none: Material 3 spec | (Component gallery, DevTools) | default | | | | not yet audited | `docs/design/captures/lane-a/02-gallery-m3button-light.png`, `docs/design/captures/lane-a/03-gallery-colorpicker-dark.png`, `docs/design/captures/lane-a/04-gallery-tokens-light.png` | | | |
| Component gallery, colour picker | none: Material 3 spec | (Component gallery, colour picker sample) | dark theme | dark | | | not yet audited | `docs/design/captures/lane-a/03-gallery-colorpicker-dark.png` | | | |

## What remains before this inventory satisfies the full contract

- No route in the built application yet corresponds one to one with every
  row (some routes above are described rather than named, for example menu
  paths); those are marked so rather than invented as file paths.
- No theme, viewport, or display scale column has been filled in yet for
  any row: filling them requires the deterministic capture route described
  in the shared instructions (a headless build driven at addressable
  states), which has not been run for this repository.
- No side by side comparison image or machine readable visual diff exists
  for any row, because there is no second, independently rendered reference
  to diff against; the M3 specification itself is the reference, and a
  human review against it (the audit column) stands in for a pixel diff
  until a rendered reference exists.
- Every "not yet audited" M3 primitive audit cell is a real gap: the routine
  grep pass in this update only checked a sample of screens for legacy
  `Muse.Ui` / `Muse.UiComponents` imports and controls (the About dialog,
  Welcome dialog, Sign in dialog, and the Preferences and Shortcuts pages);
  the rest of the table has not been reviewed control by control.
