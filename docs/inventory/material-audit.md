# Material Design 3 zero-gap audit

Hand-written inventory of legacy control usages, hard-coded visual values and theme reads
found by scanning the source tree with the identifier list agreed for this audit. One row
per file. Status is one of: `converted`, `kept (reason)`, or `pending` (found, not yet
converted or fully triaged in this pass; needs follow-up before the guard can go strict for
that file).

## Shell, project and scene (lane U1)

Scope: `src/appshell/qml`, `src/project/qml`, `src/projectscene/qml`, `src/playback/qml`,
`src/record/qml`, `src/trackedit/qml`, `src/spectrogram/qml`. No `src/au3cloud` or
`src/app/qml` QML exists in the tree at the audited commit.

### Method note

Most "legacy" Muse control identifiers in this scope (`FlatButton`, `CheckBox`,
`RoundedRadioButton`, `SearchField`, `StyledDropdown`, `StyledSlider`, `StyledTabBar`,
`StyledTabButton`, `TextInputField`, `StyledBusyIndicator`/`BusyIndicator`, `ProgressBar`,
`ListItemBlank`, internal `StyledMenu`, `StyledToolTip`/`ToolTip`, `ButtonBox`, `ValueList`,
`StyledTableView`, `StyledDialogView`, `StyledPopupView`) are not legacy chrome in this tree:
they are restyled at the framework level by the overlay patches under
`buildscripts/muse-patches/` (0002 menus, 0003 popups/dialogs, 0004 tooltip/scrollbar, 0005
controls, 0006 dock chrome, 0008 button box, 0009 shortcuts page, 0010 list/table/avatar), so
every QML file that instantiates one of these types already renders Material 3 anatomy
(M3 roles, M3 corner radii, M3 elevation) through the patched `muse` submodule. Converting
these call sites to bespoke `Audacity.M3` components would duplicate behaviour the patches
already restyle and was judged out of scope for this pass; they are listed below as "kept"
under the allowed reason "muse control already restyled by overlay patch `<name>`".

`MenuButton` (Muse) extends `FlatButton` directly, so it inherits the same patch 0005
restyle and is kept for the same reason.

Files under `src/appshell/qml/Audacity/AppShell/DevTools/**` are internal engineering
surfaces (component gallery, settings sandbox, crash-handler test harness, extension list,
table test harness), not shipped end-user UI. `GeneralComponentsGallery.qml` is the named
deliberate legacy-vs-M3 comparison and is allowlisted per the audit brief. The other
DevTools files are listed as `pending` rather than force-fit into an allowed reason: they
were not converted in this pass and still use legacy identifiers or a literal hex/radius,
but they ship inside the binary and should eventually get the same treatment or an explicit
kept reason from whichever lane owns DevTools.

### Rows

| File | Legacy usage | M3 replacement / disposition | Status |
|---|---|---|---|
| `src/appshell/qml/Audacity/AppShell/DevTools/Gallery/GeneralComponentsGallery.qml` | Whole file: `FlatButton`, `StyledSlider`, `CheckBox`, `RadioButton`, `StyledDropdown`, `TextInputField`, `SearchField`, `ColorPicker`, `IncrementalPropertyControl`, etc. | n/a | kept (dev-tools gallery comparison, named in audit brief) |
| `src/appshell/qml/Audacity/AppShell/DevTools/Gallery/M3ComponentsGallery.qml` | `radius: 24` (x3) literal | Internal M3 showcase gallery (engineering surface); literal values are local demo geometry, not chrome tokens | pending (not yet swapped to `M3.shape.*`; low priority, DevTools-only) |
| `src/appshell/qml/Audacity/AppShell/DevTools/CrashHandler/CrashHandlerDevTools.qml` | `radius: 6` literal | Internal crash-test harness | pending (DevTools-only, not converted) |
| `src/appshell/qml/Audacity/AppShell/DevTools/Extensions/ExtensionsListView.qml` | `ListItemBlank` | Restyled by patch 0002-m3-menus | kept (muse control already restyled by overlay patch 0002-m3-menus) |
| `src/appshell/qml/Audacity/AppShell/DevTools/Interactive/SampleDialog.qml` | `StyledDialogView` | Restyled by patch 0003-m3-popups-and-dialogs | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/appshell/qml/Audacity/AppShell/DevTools/Preferences/SettingsPage.qml` | `ColorPicker`, `IncrementalPropertyControl` (x2) | `M3ColorPicker` exists; no M3 numeric stepper exists yet | pending (DevTools-only; `ColorPicker` not swapped, `IncrementalPropertyControl` blocked on stepper primitive, see request below) |
| `src/appshell/qml/Audacity/AppShell/DevTools/Table/TableTests.qml` | `StyledTableView` | Restyled by patch 0010-m3-list-table-and-avatar | kept (muse control already restyled by overlay patch 0010-m3-list-table-and-avatar) |
| `src/appshell/qml/Audacity/AppShell/FirstLaunchSetup/FirstLaunchSetupDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/appshell/qml/Audacity/AppShell/AboutDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/appshell/qml/Audacity/AppShell/AlphaWelcomePopup.qml` | `StyledPopupView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/appshell/qml/Audacity/AppShell/SigninAudiocomDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/appshell/qml/Audacity/AppShell/WelcomeDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/appshell/qml/Audacity/AppShell/shared/AccentColorsList.qml` | `RadioButton` | False positive: `accessible.role: MUAccessible.RadioButton` is an accessibility role enum value, not a control instantiation | kept (not a legacy control; accessibility role constant) |
| `src/appshell/qml/Audacity/AppShell/Main.wasm.qml` | `ui.theme.` (1) | Not triaged in this pass | pending |
| `src/playback/qml/Audacity/Playback/dialogs/LoopRegionInOut.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/playback/qml/Audacity/Playback/toolbars/PlaybackMeterCustomisePopup.qml` | `StyledPopupView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/playback/qml/Audacity/Playback/components/MeterStyle.qml` | `ui.theme.` (9) | Meter fill/peak colours are functional data colours (audio level indication), not chrome | pending (not individually re-verified this pass; flagged for a colour-role audit rather than blanket conversion) |
| `src/project/qml/Audacity/Project/AlsoShareAudioComDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/project/qml/Audacity/Project/AskLocationTypeDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/project/qml/Audacity/Project/CloudProjectSyncDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/project/qml/Audacity/Project/NewProjectDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/project/qml/Audacity/Project/ProjectPropertiesDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/project/qml/Audacity/Project/ProjectUploadedDialog.qml` | `StyledDialogView`; `radius:` literal(s) | Dialog frame restyled by patch 0003; radius literals not individually re-verified | kept (dialog frame, patch 0003) / pending (radius literal audit) |
| `src/project/qml/Audacity/Project/ProjectsListView.qml` | `StyledListView`, `ui.theme.` (2) | `StyledListView` is a structural scrolling/selection wrapper; its rendered rows use `ListItemBlank`, already restyled by patch 0002 | kept (structural container; item chrome restyled by patch 0002-m3-menus) / pending (`ui.theme.` reads not re-verified) |
| `src/project/qml/Audacity/Project/SaveToCloudDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/project/qml/Audacity/Project/UploadProgressDialog.qml` | `StyledDialogView`, `ProgressBar` | Both restyled by patches 0003 and 0005 | kept (muse controls already restyled by overlay patches 0003-m3-popups-and-dialogs and 0005-m3-controls) |
| `src/project/qml/Audacity/Project/ProjectsPage.qml` | `ui.theme.` (3) | Not triaged in this pass | pending |
| `src/project/qml/Audacity/Project/internal/NewProject/TitleListView.qml` | `StyledListView` | Structural container | kept (structural container; item chrome delegated to restyled item types) |
| `src/project/qml/Audacity/Project/internal/ProjectsPage/CloudAudioFilesView.qml` | `StyledListView`, `MenuButton`, `ui.theme.` (8), `radius:` literal(s) | List container structural; `MenuButton` extends `FlatButton` (patch 0005) | kept (structural container / MenuButton via patch 0005) / pending (`ui.theme.` and radius literals not re-verified) |
| `src/project/qml/Audacity/Project/internal/ProjectsPage/DefaultProjectListView.qml` | `StyledListView`, `MenuButton`, `radius:` literal(s) | Same as above | kept (structural container / MenuButton via patch 0005) / pending (radius literals not re-verified) |
| `src/project/qml/Audacity/Project/internal/ProjectsPage/ProjectGridItem.qml` | `MenuButton`, `ui.theme.` (4), `radius:` literal(s) | `MenuButton` via patch 0005 | kept (MenuButton via patch 0005) / pending (`ui.theme.` and radius literals not re-verified) |
| `src/project/qml/Audacity/Project/internal/ProjectsPage/ProjectListItem.qml` | `ListItemBlank` | Restyled by patch 0002 | kept (muse control already restyled by overlay patch 0002-m3-menus) |
| `src/project/qml/Audacity/Project/internal/ProjectsPage/CloudProjectIndicatorButton.qml` | `ui.theme.` (8) | Not triaged in this pass | pending |
| `src/project/qml/Audacity/Project/internal/SaveToCloud/SaveLocationOption.qml` | `radius:` literal(s) | Not triaged in this pass | pending |
| `src/project/qml/Audacity/Project/internal/Properties/ProjectPropertiesView.qml` | `TextInputField` | Restyled by patch 0005 | kept (muse control already restyled by overlay patch 0005-m3-controls) |
| `src/projectscene/qml/Audacity/ProjectScene/historypanel/HistoryPanel.qml` | `StyledListView` | Structural container | kept (structural container) |
| `src/projectscene/qml/Audacity/ProjectScene/toolbars/GetEffectsDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/projectscene/qml/Audacity/ProjectScene/toolbars/internal/CustomiseView.qml` | `StyledListView` | Structural container | kept (structural container) |
| `src/projectscene/qml/Audacity/ProjectScene/toolbars/internal/M3ToolBarItem.qml` | `ToolTip` reference | Already an M3-named component; not re-verified line by line | pending (spot check only) |
| `src/projectscene/qml/Audacity/ProjectScene/toolbars/internal/PlaybackToolBarCustomisePopup.qml` | `StyledPopupView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/projectscene/qml/Audacity/ProjectScene/trackruler/TrackRulerCustomizePopup.qml` | `StyledPopupView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/projectscene/qml/Audacity/ProjectScene/trackruler/WaveformRuler.qml` | `ui.theme.` (3) | Ruler tick/label colours; functional data rendering | pending (not re-verified) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/ClipItem.qml` | `MenuButton`, `ui.theme.` (42), `radius:` (mostly `M3.shape.extraSmall` already; see method note) | `MenuButton` via patch 0005; clip fill/selection colours are functional data colours (waveform clip rendering) | kept (MenuButton via patch 0005; clip body colours are functional data) / pending (the 42 `ui.theme.` reads were not individually re-verified against the "functional data colour" exemption; needs a dedicated colour-role pass, largest remaining item in this scope) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/TracksItemsView.qml` | `StyledListView`, `ui.theme.` (4) | Structural container | kept (structural container) / pending (`ui.theme.` reads not re-verified) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/VerticalRulersPanel.qml` | `StyledListView`, `ui.theme.` (5) | Structural container | kept (structural container) / pending (`ui.theme.` reads not re-verified) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/ClipHandles.qml` | `ui.theme.` (8) | Clip trim/fade handle colours; functional data rendering candidate | pending (not re-verified) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/ClipPreview.qml` | `ui.theme.` (11), `radius:` literal(s) | Waveform preview rendering; functional data candidate | pending (not re-verified) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/ClipItemSmall.qml` | `ui.theme.` (2), `radius:` literal(s) | Same family as `ClipItem.qml` | pending (not re-verified) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/LabelHeader.qml` | `ui.theme.` (1), `radius: root.isPoint ? M3.shape.extraSmall / 2 : 0` | Already M3-tokened except the `: 0` branch, which is a legitimate square corner, not a legacy literal | kept (M3 token already in use; `0` is a real corner-radius value, not a hard-coded chrome literal) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/LabelItem.qml` | `ui.theme.` (7), `radius:` literal(s) | Label track item rendering | pending (not re-verified) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/labeleditor/AddNewLabelTrackDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/labeleditor/LabelEditorDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/labeleditor/LabelEditorLabelsTableView.qml` | `StyledTableView` | Restyled by patch 0010 | kept (muse control already restyled by overlay patch 0010-m3-list-table-and-avatar) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/pitchandspeed/PitchAndSpeedChangeDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/pitchandspeed/PropertyView.qml` | `IncrementalPropertyControl` | No M3 numeric stepper exists yet | pending (behaviour gap, see request below) |
| `src/projectscene/qml/Audacity/ProjectScene/trackspanel/AddNewTrackPopup.qml` | `StyledPopupView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/projectscene/qml/Audacity/ProjectScene/trackspanel/EditableLabel.qml` | `TextInputField`, `radius:` literal(s) | Restyled by patch 0005 | kept (muse control already restyled by overlay patch 0005-m3-controls) / pending (radius literal not re-verified) |
| `src/projectscene/qml/Audacity/ProjectScene/trackspanel/RealtimeEffectListItem.qml` | `ListItemBlank`, `ui.theme.` (1) | Restyled by patch 0002 | kept (muse control already restyled by overlay patch 0002-m3-menus) / pending (`ui.theme.` read not re-verified) |
| `src/projectscene/qml/Audacity/ProjectScene/trackspanel/TrackEffectList.qml` | `StyledListView` | Structural container | kept (structural container) |
| `src/projectscene/qml/Audacity/ProjectScene/trackspanel/TrackItem.qml` | `MenuButton` | Via patch 0005 | kept (MenuButton via patch 0005) |
| `src/projectscene/qml/Audacity/ProjectScene/trackspanel/TracksPanel.qml` | `StyledListView` | Structural container | kept (structural container) |
| `src/projectscene/qml/Audacity/ProjectScene/trackspanel/audio/PanKnob.qml` | `KnobControl` (local Audacity type) | `KnobControl.qml` in the same directory is already a thin wrapper around `M3Knob` | converted (already M3; false positive, `KnobControl` here is a local M3-backed wrapper, not the legacy Muse control) |
| `src/record/qml/Audacity/Record/internal/RecordLevelPopup.qml` | `StyledPopupView`; `radius: 2` literal | Popup frame restyled by patch 0003; radius literal was hard-coded beside otherwise-M3 tokens | kept (popup frame via patch 0003) / **converted** (radius literal replaced with `M3.shape.extraSmall`, matching the M3 tokens already used on the same `Rectangle`) |
| `src/spectrogram/qml/Audacity/Spectrogram/SpectrogramRulerCustomizePopup.qml` | `StyledPopupView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/spectrogram/qml/Audacity/Spectrogram/SpectrogramChannelRuler.qml` | `ui.theme.` (3) | Ruler colours, functional data candidate | pending (not re-verified) |
| `src/spectrogram/qml/Audacity/Spectrogram/TrackSpectrogramAlgorithmSection.qml` | `RoundedRadioButton` | Restyled by patch 0005 | kept (muse control already restyled by overlay patch 0005-m3-controls) |
| `src/spectrogram/qml/Audacity/Spectrogram/TrackSpectrogramColorsSection.qml` | `IncrementalPropertyControl` | No M3 numeric stepper exists yet | pending (behaviour gap, see request below) |
| `src/spectrogram/qml/Audacity/Spectrogram/TrackSpectrogramScaleSection.qml` | `IncrementalPropertyControl` | No M3 numeric stepper exists yet | pending (behaviour gap, see request below) |
| `src/spectrogram/qml/Audacity/Spectrogram/TrackSpectrogramSettingsDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/trackedit/qml/Audacity/TrackEdit/BehaviorChoice.qml` | `RoundedRadioButton`; `radius: 5` literal | RoundedRadioButton restyled by patch 0005; radius literal was hard-coded beside an M3 token on the same component | kept (RoundedRadioButton via patch 0005) / **converted** (radius literal replaced with `M3.shape.extraSmall`) |
| `src/trackedit/qml/Audacity/TrackEdit/CustomRateDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/trackedit/qml/Audacity/TrackEdit/CustomTimeDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/trackedit/qml/Audacity/TrackEdit/DeleteBehaviorOnboardingDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/trackedit/qml/Audacity/TrackEdit/DeleteBehaviorOnboardingFollowupDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/trackedit/qml/Audacity/TrackEdit/DeleteBehaviorPanel.qml` | `radius:` literal(s) | Not triaged in this pass | pending |
| `src/trackedit/qml/Audacity/TrackEdit/PasteBehaviorPanel.qml` | `radius:` literal(s) | Not triaged in this pass | pending |

### Behaviour-gap request to lane U2 (M3 library additions)

`IncrementalPropertyControl` (Muse legacy numeric stepper: text field plus increment and
decrement buttons) is used on real product surfaces with no overlay-patch restyle and no
existing `Audacity.M3` equivalent: `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/pitchandspeed/PropertyView.qml`,
`src/spectrogram/qml/Audacity/Spectrogram/TrackSpectrogramColorsSection.qml`,
`src/spectrogram/qml/Audacity/Spectrogram/TrackSpectrogramScaleSection.qml`, plus two
DevTools-only usages. Requesting an `M3NumberStepper` (or equivalent) addition to
`src/uicomponents/qml/Audacity/M3/` so these three product call sites can convert.

### Row counts (lane U1, this pass)

- Converted: 3 (`PanKnob.qml` verified already-M3; `RecordLevelPopup.qml` and
  `BehaviorChoice.qml` radius literals replaced with `M3.shape.extraSmall`).
- Kept with an allowed reason (overlay-patch restyle, structural container, dev-tools
  gallery comparison, or non-control false positive): the great majority of the 63 files
  that matched the scan, itemised above.
- Pending (found, not converted or not individually re-verified this pass): the
  `ui.theme.` colour reads on clip/label/meter/ruler rendering surfaces (headline item:
  `ClipItem.qml`, 42 reads) and the scattered `radius:` literals not already covered above,
  plus the three `IncrementalPropertyControl` product usages blocked on the M3 stepper
  request, plus the DevTools-only files not force-fit into an allowed reason.

## Dialogs, preferences, effects and companion modules (lane U2)

Scope: `src/effects/**/qml/**`, `src/preferences/qml/**` (excluding `PreferencesDialog.qml`,
`PersonalizePreferencesPage.qml` and `UpdatesPreferencesPage.qml`, owned by another lane),
`src/importexport/**/qml/**`, `src/uicomponents/qml/Audacity/UiComponents/**`, additions to
`src/uicomponents/qml/Audacity/M3/**` (excluding `M3SearchBar.qml`), `src/companion/qml/**`,
`src/chronicle/qml/**` (excluding `TabStrip.qml`), `src/experience/qml/**`,
`src/personalize/qml/**`, `src/toolkit/qml/**`, `src/squirrelupdate/qml/**`.

### Method note

As lane U1 found in its scope, most legacy Muse control identifiers here are not legacy
chrome: they are restyled at the framework level by the overlay patches under
`buildscripts/muse-patches/` (0002 menus, 0003 popups and dialogs, 0004 tooltip and
scroll bar, 0005 controls, 0010 list, table and avatar), so a QML file that instantiates
one of these types already renders Material 3 anatomy through the patched `muse`
submodule. These are listed below as `kept` under the reason "muse control already
restyled by overlay patch <name>".

`MenuButton` (Muse) extends `FlatButton` directly and is kept for the same reason,
matching lane U1's reasoning for the same identifier.

`StyledListView` used purely as a scrolling container, with its row content rendered by
an `Audacity.M3` delegate (typically `M3ListItem`), is kept as a structural container:
the container itself draws no chrome. This was spot checked on a sample of the files
below rather than individually re-verified for every row; a file where the delegate
turns out not to be M3-styled after all should have its row corrected to `pending`.

### Converted this pass

- `src/effects/builtin_collection/qml/Audacity/BuiltinEffectsCollection/ParameterKnob.qml`,
  `.../BigParameterKnob.qml` and `src/effects/builtin_collection/dtmfgen/DtmfView.qml`:
  `KnobControl` swapped for the existing `M3Knob` (`Audacity.M3` was already imported in
  all three files and the property and signal names, `value`/`from`/`to`/`stepSize`/
  `radius`/`mouseArea`/`navigation`/`newValueRequested`, already matched `M3Knob`'s API
  exactly, so this was a pure identifier swap). Verified: `cmake --build build/linux -j3
  --target audacity` succeeded, and the Xvfb smoke capture showed no QML load failure.
- `src/uicomponents/qml/Audacity/UiComponents/components/internal/TimeSignaturePopup.qml`:
  the raw `IncrementalPropertyControl` (Muse legacy numeric stepper) was replaced with a
  new shared `M3NumberField` component, added to the `Audacity.M3` library this pass (see
  below) because lane U1 had already asked for exactly this addition for its own
  `IncrementalPropertyControl` product usages (`PropertyView.qml`,
  `TrackSpectrogramColorsSection.qml`, `TrackSpectrogramScaleSection.qml`). The API
  (`currentValue`, `minValue`, `maxValue`, `step`, `decimals`, `valueEdited`,
  `navigation`) already matched the call site exactly, being copied from the three
  pre-existing per-module `M3NumberField.qml` wrappers already used elsewhere in
  `src/preferences`, `src/effects` and `src/importexport` (which wrap
  `IncrementalPropertyControl` with `M3TextField` plus two `M3IconButton` steppers, so
  those call sites were already Material 3, just via a locally duplicated wrapper rather
  than a shared library component).

### New `Audacity.M3` library addition

- `src/uicomponents/qml/Audacity/M3/M3NumberField.qml`: a Material 3 numeric entry built
  from `M3TextField` plus a decrement and increment `M3IconButton` pair, keeping the
  public API of Muse's `IncrementalPropertyControl` so existing call sites swap
  mechanically. Registered in `qmldir` and `au_uicomponents.qrc`. Content is the existing,
  already-shipped `src/preferences/qml/Audacity/Preferences/internal/M3NumberField.qml`
  promoted to the shared library verbatim (the three per-module copies were not
  deduplicated onto the new shared component in this pass, to keep the change small and
  low risk; that consolidation is a follow-up, not a functional gap, since all three
  copies already render Material 3).

### Coordinator-reported defect fixed this pass

- `src/toolkit/qml/Audacity/Toolkit/ExportSheet.qml`: `M3Card` was given an `elevation: 3`
  property assignment, but `M3Card` has no writable `elevation` property (elevation is
  derived internally from `variant` and an optional personalize appearance override), so
  QML logged `Cannot assign to non-existent property "elevation"` on every load. Replaced
  with the property `M3Card` actually exposes, `variant: "elevated"` (already the
  default), which yields the card's normal resting elevation. Verified: build succeeded,
  and grepping the rest of this lane's scope for `elevation:` found no other occurrence.
  The Xvfb smoke run of the home page logged no "Cannot assign to non-existent property",
  "Failed to load main qml", "is not a type" or "ReferenceError" lines; `ExportSheet.qml`
  itself is only shown from a user action this smoke pass did not reach, so its specific
  fix was verified by reading the corrected property against `M3Card.qml`'s real API
  rather than by a capture of the open sheet.

### Rows

| File | Legacy usage | M3 replacement / disposition | Status |
|---|---|---|---|
| `src/chronicle/qml/Audacity/Chronicle/CloseTabsPopup.qml` | `StyledListView`, `StyledPopupView` | plain scrolling container; row content is rendered by an Audacity.M3 delegate, not itself chrome; muse control already restyled by overlay patch 0003-m3-popups-and-dialogs.patch | kept |
| `src/chronicle/qml/Audacity/Chronicle/TabGroupAppearancePopup.qml` | `StyledPopupView` | muse control already restyled by overlay patch 0003-m3-popups-and-dialogs.patch | kept |
| `src/chronicle/qml/Audacity/Chronicle/TabSearchPopup.qml` | `StyledListView`, `StyledPopupView` | plain scrolling container; row content is rendered by an Audacity.M3 delegate, not itself chrome; muse control already restyled by overlay patch 0003-m3-popups-and-dialogs.patch | kept |
| `src/chronicle/qml/Audacity/Chronicle/TabStripItem.qml` | `radius-literal` | radius literal not triaged in this pass; needs a case by case check against M3.shape tokens | pending |
| `src/chronicle/qml/Audacity/Chronicle/VersionHistoryPanel.qml` | `StyledListView`, `StyledPopupView` | plain scrolling container; row content is rendered by an Audacity.M3 delegate, not itself chrome; muse control already restyled by overlay patch 0003-m3-popups-and-dialogs.patch | kept |
| `src/companion/qml/Audacity/Companion/RegexBuilder.qml` | `ui.theme` | ui.theme colour read not triaged in this pass; some are functional data colours for waveform, meter, spectrogram or dynamics visualisation (exempt as data), others are plain style reads that should move to M3.color / M3.typography | pending |
| `src/effects/builtin_collection/basstreble/BassTrebleView.qml` | `radius-literal` | radius literal not triaged in this pass; needs a case by case check against M3.shape tokens | pending |
| `src/effects/builtin_collection/dtmfgen/DtmfView.qml` | `radius-literal` | radius literal not triaged in this pass; needs a case by case check against M3.shape tokens | pending |
| `src/effects/builtin_collection/dynamics/compressor/CompressionCurvePainter.qml` | `ui.theme` | ui.theme colour read not triaged in this pass; some are functional data colours for waveform, meter, spectrogram or dynamics visualisation (exempt as data), others are plain style reads that should move to M3.color / M3.typography | pending |
| `src/effects/builtin_collection/dynamics/compressor/CompressorView.qml` | `radius-literal` | radius literal not triaged in this pass; needs a case by case check against M3.shape tokens | pending |
| `src/effects/builtin_collection/dynamics/limiter/LimiterView.qml` | `radius-literal` | radius literal not triaged in this pass; needs a case by case check against M3.shape tokens | pending |
| `src/effects/builtin_collection/dynamics/timeline/ClipIndicator.qml` | `radius-literal`, `ui.theme` | radius literal not triaged in this pass; needs a case by case check against M3.shape tokens; ui.theme colour read not triaged in this pass; some are functional data colours for waveform, meter, spectrogram or dynamics visualisation (exempt as data), others are plain style reads that should move to M3.color / M3.typography | pending |
| `src/effects/builtin_collection/dynamics/timeline/DynamicsPanel.qml` | `CheckBox` | muse control already restyled by overlay patch 0005-m3-controls.patch | kept |
| `src/effects/builtin_collection/dynamics/timeline/DynamicsPanel.qml` | `ui.theme` | ui.theme colour read not triaged in this pass; some are functional data colours for waveform, meter, spectrogram or dynamics visualisation (exempt as data), others are plain style reads that should move to M3.color / M3.typography | pending |
| `src/effects/builtin_collection/filtercurveeq/FilterCurveEqTooltip.qml` | `StyledPopupView` | muse control already restyled by overlay patch 0003-m3-popups-and-dialogs.patch | kept |
| `src/effects/builtin_collection/filtercurveeq/FilterCurveEqTooltip.qml` | `ui.theme` | ui.theme colour read not triaged in this pass; some are functional data colours for waveform, meter, spectrogram or dynamics visualisation (exempt as data), others are plain style reads that should move to M3.color / M3.typography | pending |
| `src/effects/builtin_collection/filtercurveeq/FilterCurveEqView.qml` | `ui.theme` | ui.theme colour read not triaged in this pass; some are functional data colours for waveform, meter, spectrogram or dynamics visualisation (exempt as data), others are plain style reads that should move to M3.color / M3.typography | pending |
| `src/effects/builtin_collection/graphiceq/GraphicEqFader.qml` | `ui.theme` | ui.theme colour read not triaged in this pass; some are functional data colours for waveform, meter, spectrogram or dynamics visualisation (exempt as data), others are plain style reads that should move to M3.color / M3.typography | pending |
| `src/effects/builtin_collection/graphiceq/GraphicEqFaderHandle.qml` | `radius-literal`, `ui.theme` | radius literal not triaged in this pass; needs a case by case check against M3.shape tokens; ui.theme colour read not triaged in this pass; some are functional data colours for waveform, meter, spectrogram or dynamics visualisation (exempt as data), others are plain style reads that should move to M3.color / M3.typography | pending |
| `src/effects/builtin_collection/graphiceq/GraphicEqGridLines.qml` | `ui.theme` | ui.theme colour read not triaged in this pass; some are functional data colours for waveform, meter, spectrogram or dynamics visualisation (exempt as data), others are plain style reads that should move to M3.color / M3.typography | pending |
| `src/effects/builtin_collection/graphiceq/GraphicEqView.qml` | `radius-literal` | radius literal not triaged in this pass; needs a case by case check against M3.shape tokens | pending |
| `src/effects/builtin_collection/noisereduction/NoiseReductionView.qml` | `radius-literal` | radius literal not triaged in this pass; needs a case by case check against M3.shape tokens | pending |
| `src/effects/builtin_collection/qml/Audacity/BuiltinEffectsCollection/DynamicsEffectBase.qml` | `ui.theme` | ui.theme colour read not triaged in this pass; some are functional data colours for waveform, meter, spectrogram or dynamics visualisation (exempt as data), others are plain style reads that should move to M3.color / M3.typography | pending |
| `src/effects/builtin_collection/qml/Audacity/BuiltinEffectsCollection/SettingKnob.qml` | `radius-literal` | radius literal not triaged in this pass; needs a case by case check against M3.shape tokens | pending |
| `src/effects/builtin_collection/reverb/ReverbView.qml` | `radius-literal` | radius literal not triaged in this pass; needs a case by case check against M3.shape tokens | pending |
| `src/effects/builtin_collection/slidingstretch/SlidingStretchCard.qml` | `radius-literal` | radius literal not triaged in this pass; needs a case by case check against M3.shape tokens | pending |
| `src/effects/builtin_collection/tonegen/ChirpView.qml` | `radius-literal` | radius literal not triaged in this pass; needs a case by case check against M3.shape tokens | pending |
| `src/effects/effects_base/qml/Audacity/Effects/EffectControlsDisablingOverlay.qml` | `ui.theme` | ui.theme colour read not triaged in this pass; some are functional data colours for waveform, meter, spectrogram or dynamics visualisation (exempt as data), others are plain style reads that should move to M3.color / M3.typography | pending |
| `src/effects/effects_base/qml/Audacity/Effects/EffectStyledDialogView.qml` | `StyledDialogView` | muse control already restyled by overlay patch 0003-m3-popups-and-dialogs.patch | kept |
| `src/effects/effects_base/qml/Audacity/Effects/MissingPluginsDialog.qml` | `StyledDialogView` | muse control already restyled by overlay patch 0003-m3-popups-and-dialogs.patch | kept |
| `src/effects/effects_base/qml/Audacity/Effects/MissingPluginsDialog.qml` | `ValueList` | no Audacity.M3 equivalent yet (multi-row editable key/value list with add/remove); a candidate M3ValueList addition was not built this pass | pending |
| `src/effects/effects_base/qml/Audacity/Effects/PluginManagerDialog.qml` | `StyledDialogView` | muse control already restyled by overlay patch 0003-m3-popups-and-dialogs.patch | kept |
| `src/effects/effects_base/qml/Audacity/Effects/PluginManagerTableView.qml` | `StyledTableView` | muse control already restyled by overlay patch 0010-m3-list-table-and-avatar.patch | kept |
| `src/effects/effects_base/qml/Audacity/Effects/PresetNameDialog.qml` | `StyledDialogView` | muse control already restyled by overlay patch 0003-m3-popups-and-dialogs.patch | kept |
| `src/effects/effects_base/qml/Audacity/Effects/PresetNameDialog.qml` | `ui.theme` | ui.theme colour read not triaged in this pass; some are functional data colours for waveform, meter, spectrogram or dynamics visualisation (exempt as data), others are plain style reads that should move to M3.color / M3.typography | pending |
| `src/effects/nyquist/nyquistprompt/NyquistPromptView.qml` | `ui.theme` | ui.theme colour read not triaged in this pass; some are functional data colours for waveform, meter, spectrogram or dynamics visualisation (exempt as data), others are plain style reads that should move to M3.color / M3.typography | pending |
| `src/experience/qml/Audacity/Experience/NotificationCentre.qml` | `StyledListView` | plain scrolling container; row content is rendered by an Audacity.M3 delegate, not itself chrome | kept |
| `src/importexport/export/qml/Export/CustomFFmpegDialog.qml` | `StyledDialogView` | muse control already restyled by overlay patch 0003-m3-popups-and-dialogs.patch | kept |
| `src/importexport/export/qml/Export/CustomMappingDialog.qml` | `StyledDialogView` | muse control already restyled by overlay patch 0003-m3-popups-and-dialogs.patch | kept |
| `src/importexport/export/qml/Export/ExportDialog.qml` | `StyledDialogView` | muse control already restyled by overlay patch 0003-m3-popups-and-dialogs.patch | kept |
| `src/importexport/export/qml/Export/MetadataDialog.qml` | `StyledDialogView` | muse control already restyled by overlay patch 0003-m3-popups-and-dialogs.patch | kept |
| `src/importexport/export/qml/Export/MetadataDialog.qml` | `ValueList` | no Audacity.M3 equivalent yet (multi-row editable key/value list with add/remove); a candidate M3ValueList addition was not built this pass | pending |
| `src/importexport/export/qml/Export/internal/ChannelMappingTableView.qml` | `StyledTableView` | muse control already restyled by overlay patch 0010-m3-list-table-and-avatar.patch | kept |
| `src/importexport/export/qml/Export/internal/FormatAndCodecSection.qml` | `ListItemBlank`, `StyledListView` | muse control already restyled by overlay patch 0002-m3-menus.patch; plain scrolling container; row content is rendered by an Audacity.M3 delegate, not itself chrome | kept |
| `src/importexport/export/qml/Export/internal/GeneralOptionsSection.qml` | `ToolTip` | muse control already restyled by overlay patch 0004-m3-tooltip-and-scrollbar.patch | kept |
| `src/importexport/labels/qml/Export/ExportLabelsDialog.qml` | `StyledDialogView` | muse control already restyled by overlay patch 0003-m3-popups-and-dialogs.patch | kept |
| `src/importexport/labels/qml/Export/internal/LabelTracksSelectionView.qml` | `ListItemBlank`, `StyledListView` | muse control already restyled by overlay patch 0002-m3-menus.patch; plain scrolling container; row content is rendered by an Audacity.M3 delegate, not itself chrome | kept |
| `src/importexport/labels/qml/Export/internal/LabelTracksSelectionView.qml` | `radius-literal` | radius literal not triaged in this pass; needs a case by case check against M3.shape tokens | pending |
| `src/preferences/qml/Audacity/Preferences/AdvancedPreferencesPage.qml` | `ValueList` | no Audacity.M3 equivalent yet (multi-row editable key/value list with add/remove); a candidate M3ValueList addition was not built this pass | pending |
| `src/preferences/qml/Audacity/Preferences/internal/AsymmetricStereoHeightsSection.qml` | `radius-literal` | radius literal not triaged in this pass; needs a case by case check against M3.shape tokens | pending |
| `src/preferences/qml/Audacity/Preferences/internal/DefaultFilesSection.qml` | `StyledListView` | plain scrolling container; row content is rendered by an Audacity.M3 delegate, not itself chrome | kept |
| `src/preferences/qml/Audacity/Preferences/internal/FoldersSection.qml` | `StyledListView` | plain scrolling container; row content is rendered by an Audacity.M3 delegate, not itself chrome | kept |
| `src/preferences/qml/Audacity/Preferences/internal/PluginLocationsSection.qml` | `ui.theme` | ui.theme colour read not triaged in this pass; some are functional data colours for waveform, meter, spectrogram or dynamics visualisation (exempt as data), others are plain style reads that should move to M3.color / M3.typography | pending |
| `src/preferences/qml/Audacity/Preferences/internal/SpectrogramAlgorithmSection.qml` | `StyledListView` | plain scrolling container; row content is rendered by an Audacity.M3 delegate, not itself chrome | kept |
| `src/preferences/qml/Audacity/Preferences/internal/UiColorsSection.qml` | `ui.theme` | ui.theme colour read not triaged in this pass; some are functional data colours for waveform, meter, spectrogram or dynamics visualisation (exempt as data), others are plain style reads that should move to M3.color / M3.typography | pending |
| `src/preferences/qml/Audacity/Preferences/internal/WorkspacesAsymmetricChannelsSection.qml` | `CheckBox`, `StyledListView` | muse control already restyled by overlay patch 0005-m3-controls.patch; plain scrolling container; row content is rendered by an Audacity.M3 delegate, not itself chrome | kept |
| `src/preferences/qml/Audacity/Preferences/internal/WorkspacesTempoDetectionSection.qml` | `CheckBox`, `StyledListView` | muse control already restyled by overlay patch 0005-m3-controls.patch; plain scrolling container; row content is rendered by an Audacity.M3 delegate, not itself chrome | kept |
| `src/toolkit/qml/Audacity/Toolkit/RecoveryCard.qml` | `ui.theme` | ui.theme colour read not triaged in this pass; some are functional data colours for waveform, meter, spectrogram or dynamics visualisation (exempt as data), others are plain style reads that should move to M3.color / M3.typography | pending |
| `src/uicomponents/qml/Audacity/UiComponents/components/ArrowButton.qml` | `FlatButton` | muse control already restyled by overlay patch 0005-m3-controls.patch | kept |
| `src/uicomponents/qml/Audacity/UiComponents/components/ArrowMenuButton.qml` | `MenuButton` | extends Muse FlatButton directly, so it inherits the patch 0005-m3-controls.patch restyle | kept |
| `src/uicomponents/qml/Audacity/UiComponents/components/GridPlot.qml` | `ui.theme` | ui.theme colour read not triaged in this pass; some are functional data colours for waveform, meter, spectrogram or dynamics visualisation (exempt as data), others are plain style reads that should move to M3.color / M3.typography | pending |
| `src/uicomponents/qml/Audacity/UiComponents/components/TimeSignature.qml` | `ui.theme` | ui.theme colour read not triaged in this pass; some are functional data colours for waveform, meter, spectrogram or dynamics visualisation (exempt as data), others are plain style reads that should move to M3.color / M3.typography | pending |
| `src/uicomponents/qml/Audacity/UiComponents/components/ValueTooltip.qml` | `StyledPopupView` | muse control already restyled by overlay patch 0003-m3-popups-and-dialogs.patch | kept |
| `src/uicomponents/qml/Audacity/UiComponents/components/ValueTooltip.qml` | `ui.theme` | ui.theme colour read not triaged in this pass; some are functional data colours for waveform, meter, spectrogram or dynamics visualisation (exempt as data), others are plain style reads that should move to M3.color / M3.typography | pending |
| `src/uicomponents/qml/Audacity/UiComponents/components/internal/NumericField.qml` | `ListItemBlank` | muse control already restyled by overlay patch 0002-m3-menus.patch | kept |
| `src/uicomponents/qml/Audacity/UiComponents/components/internal/NumericField.qml` | `ui.theme` | ui.theme colour read not triaged in this pass; some are functional data colours for waveform, meter, spectrogram or dynamics visualisation (exempt as data), others are plain style reads that should move to M3.color / M3.typography | pending |
| `src/uicomponents/qml/Audacity/UiComponents/components/internal/NumericView.qml` | `ui.theme` | ui.theme colour read not triaged in this pass; some are functional data colours for waveform, meter, spectrogram or dynamics visualisation (exempt as data), others are plain style reads that should move to M3.color / M3.typography | pending |
| `src/uicomponents/qml/Audacity/UiComponents/components/internal/TimeSignaturePopup.qml` | `StyledDropdown`, `StyledPopupView` | muse control already restyled by overlay patch 0005-m3-controls.patch; muse control already restyled by overlay patch 0003-m3-popups-and-dialogs.patch | kept |
| `src/uicomponents/qml/Audacity/UiComponents/components/internal/TimeSignaturePopup.qml` | `ui.theme` | ui.theme colour read not triaged in this pass; some are functional data colours for waveform, meter, spectrogram or dynamics visualisation (exempt as data), others are plain style reads that should move to M3.color / M3.typography | pending |

### Row counts (lane U2, this pass)

- Converted: 4 files (3 `KnobControl` to `M3Knob` swaps, 1
  `IncrementalPropertyControl` to the new shared `M3NumberField`), plus 1 defect fix
  (`ExportSheet.qml`'s invalid `elevation` property).
- Kept with an allowed reason (overlay-patch restyle, or a structural container /
  inheritance case matching lane U1's own reasoning): 31 rows.
- Pending (found, not converted or not individually re-verified this pass): 38 rows,
  overwhelmingly `ui.theme.` colour reads and `radius:` literals that were not triaged
  one by one against `M3.color`/`M3.shape` tokens in the time available for this lane,
  plus the two `ValueList` usages (no `Audacity.M3` equivalent exists yet).
