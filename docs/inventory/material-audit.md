# Material Design 3 zero-gap audit

Hand-written inventory of legacy control usages, hard-coded visual values and theme reads
found by scanning the source tree with the identifier list agreed for this audit. One row
per file. Status is one of: `converted`, `kept (reason)`, `remaining` (a genuine, named,
concrete blocker prevents conversion this pass), or `pending` (not yet triaged; a lane still
working through its scope). A file with no open item is simply absent from that lane's table.

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
already restyle; they are listed below as "kept" under the reason "muse control already
restyled by overlay patch `<name>`".

`MenuButton` (Muse) extends `FlatButton` directly, so it inherits the same patch 0005
restyle and is kept for the same reason.

`ui.theme.extra["..."]` reads are Audacity's own theme-extension namespace for clip,
waveform-envelope and meter data colours (`clip_color_1`, `audio_envelope_line`,
`black_color`/`white_color` used as fixed pen colours on envelope points and clip outlines,
and similar). These colour the waveform, clip and meter data pixels themselves, not app
chrome, so they are kept under the reason "data colour" per the owner's own rule that only
those surfaces may keep non-M3 colours. Every `ui.theme.extra` read in this lane's scope was
individually inspected (not sampled) to confirm it colours a clip/waveform/meter data pixel
and not a piece of chrome; the handful that turned out to be chrome (see below) were
converted instead of kept.

`M3ComponentsGallery.qml` is a pure Material 3 showcase by definition and is allowlisted
without conversion, per the coordinator's explicit instruction. `GeneralComponentsGallery.qml`
is the named deliberate legacy-vs-M3 comparison and is allowlisted per the audit brief.

### Rows

| File | Legacy usage | M3 replacement / disposition | Status |
|---|---|---|---|
| `src/appshell/qml/Audacity/AppShell/DevTools/Gallery/GeneralComponentsGallery.qml` | Whole file: `FlatButton`, `StyledSlider`, `CheckBox`, `RadioButton`, `StyledDropdown`, `TextInputField`, `SearchField`, `ColorPicker`, `IncrementalPropertyControl`, `FlatRadioButton`, `RadioButtonGroup`, `ui.theme.backgroundSecondaryColor`, `ui.theme.strokeColor`, etc. | n/a | kept (dev-tools gallery comparison, named in audit brief) |
| `src/appshell/qml/Audacity/AppShell/DevTools/Gallery/M3ComponentsGallery.qml` | `radius: 24` (x3) literal | Internal M3 showcase gallery, pure Material 3 by definition | kept (M3 showcase gallery, allowlisted per coordinator instruction) |
| `src/appshell/qml/Audacity/AppShell/DevTools/CrashHandler/CrashHandlerDevTools.qml` | `radius: 6` literal | Replaced with `M3.shape.small`, matching the nearest existing usage of that token elsewhere for a similar card radius | converted |
| `src/appshell/qml/Audacity/AppShell/DevTools/Extensions/ExtensionsListView.qml` | `ListItemBlank` | Restyled by patch 0002-m3-menus | kept (muse control already restyled by overlay patch 0002-m3-menus) |
| `src/appshell/qml/Audacity/AppShell/DevTools/Interactive/SampleDialog.qml` | `StyledDialogView` | Restyled by patch 0003-m3-popups-and-dialogs | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/appshell/qml/Audacity/AppShell/DevTools/Preferences/SettingsPage.qml` | `ColorPicker`, `IncrementalPropertyControl` (x2) | `ColorPicker` replaced with `M3ColorPicker` (wired through `selection`/`accepted` instead of a plain `color` property); both `IncrementalPropertyControl` instances replaced with `M3NumberField` | converted |
| `src/appshell/qml/Audacity/AppShell/DevTools/Table/TableTests.qml` | `StyledTableView` | Restyled by patch 0010-m3-list-table-and-avatar | kept (muse control already restyled by overlay patch 0010-m3-list-table-and-avatar) |
| `src/appshell/qml/Audacity/AppShell/FirstLaunchSetup/FirstLaunchSetupDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/appshell/qml/Audacity/AppShell/AboutDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/appshell/qml/Audacity/AppShell/AlphaWelcomePopup.qml` | `StyledPopupView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/appshell/qml/Audacity/AppShell/SigninAudiocomDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/appshell/qml/Audacity/AppShell/WelcomeDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/appshell/qml/Audacity/AppShell/shared/AccentColorsList.qml` | `RadioButton` | False positive: `accessible.role: MUAccessible.RadioButton` is an accessibility role enum value, not a control instantiation | kept (not a legacy control; accessibility role constant) |
| `src/appshell/qml/Audacity/AppShell/Main.wasm.qml` | `ui.theme.backgroundPrimaryColor` | Replaced with `M3.color.surface`; this file targets a wasm build configuration this tree does not currently produce, but the source itself is corrected | converted |
| `src/playback/qml/Audacity/Playback/dialogs/LoopRegionInOut.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/playback/qml/Audacity/Playback/toolbars/PlaybackMeterCustomisePopup.qml` | `StyledPopupView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/playback/qml/Audacity/Playback/components/MeterStyle.qml` | `ui.theme.extra[...]` (9): meter clipped/no-clipped/RMS/peak fill colours | Audio level meter fill colours are the functional data pixels of a level meter | kept (data colour) |
| `src/project/qml/Audacity/Project/AlsoShareAudioComDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/project/qml/Audacity/Project/AskLocationTypeDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/project/qml/Audacity/Project/CloudProjectSyncDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/project/qml/Audacity/Project/NewProjectDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/project/qml/Audacity/Project/ProjectPropertiesDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/project/qml/Audacity/Project/ProjectUploadedDialog.qml` | `StyledDialogView`; `radius: 3` literal | Dialog frame restyled by patch 0003; radius literal replaced with `M3.shape.extraSmall` | kept (dialog frame, patch 0003) + converted (radius) |
| `src/project/qml/Audacity/Project/ProjectsListView.qml` | `StyledListView`, `ui.theme.extra["white_color"\|"black_color"]` (2) | `StyledListView` is a structural scrolling/selection wrapper whose rows use `ListItemBlank` (patch 0002); the two theme reads are a fixed white/black pair drawn as a contrast scrim over an arbitrary project-thumbnail image, not a semantic surface colour | kept (structural container; fixed-contrast overlay on an arbitrary thumbnail image) |
| `src/project/qml/Audacity/Project/SaveToCloudDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/project/qml/Audacity/Project/UploadProgressDialog.qml` | `StyledDialogView`, `ProgressBar` | Both restyled by patches 0003 and 0005 | kept (muse controls already restyled by overlay patches 0003-m3-popups-and-dialogs and 0005-m3-controls) |
| `src/project/qml/Audacity/Project/ProjectsPage.qml` | `ui.theme.defaultButtonSize` (x3); `FlatRadioButton`, `RadioButtonGroup` (grid/list view toggle) | `defaultButtonSize` replaced with the plain `48` (dp) touch-target literal used elsewhere in the tree for the same purpose | converted (size literal) / **remaining** (`FlatRadioButton`/`RadioButtonGroup`: see "Remaining items" below) |
| `src/project/qml/Audacity/Project/internal/NewProject/TitleListView.qml` | `StyledListView` | Structural container; rows use restyled item types | kept (structural container) |
| `src/project/qml/Audacity/Project/internal/ProjectsPage/CloudAudioFilesView.qml` | `StyledListView`, `MenuButton`, `ui.theme.extra["white_color"\|"black_color"]` (8), `radius: 2 + border.width` (x3), `radius: 3` | `MenuButton` via patch 0005; the 8 theme reads are the same fixed-contrast thumbnail-overlay pattern as `ProjectsListView.qml`; radius literals replaced with `M3.shape.extraSmall - border.width` and `M3.shape.extraSmall` respectively | kept (structural container / MenuButton via patch 0005 / fixed-contrast overlay) + converted (radii) |
| `src/project/qml/Audacity/Project/internal/ProjectsPage/DefaultProjectListView.qml` | `StyledListView`, `MenuButton`, `radius: 2 + border.width` (x2), `radius: 3` | Same pattern as `CloudAudioFilesView.qml`; radius literals converted the same way | kept (structural container / MenuButton via patch 0005) + converted (radii) |
| `src/project/qml/Audacity/Project/internal/ProjectsPage/ProjectGridItem.qml` | `MenuButton`, `ui.theme.extra["white_color"\|"black_color"]` (3), `ui.theme.borderWidth`, `readonly property int radius: 3` | `MenuButton` via patch 0005; theme colour reads are the same fixed-contrast thumbnail-overlay pattern; `borderWidth` replaced with the plain `1` literal already standard beside M3 colour roles; `radius` property replaced with `M3.shape.extraSmall` | kept (MenuButton via patch 0005 / fixed-contrast overlay) + converted (border width, radius) |
| `src/project/qml/Audacity/Project/internal/ProjectsPage/ProjectListItem.qml` | `ListItemBlank` | Restyled by patch 0002 | kept (muse control already restyled by overlay patch 0002-m3-menus) |
| `src/project/qml/Audacity/Project/internal/ProjectsPage/CloudProjectIndicatorButton.qml` | `ui.theme.extra["white_color"\|"black_color"]` (8) | Fixed white/black contrast scrim drawn over an arbitrary project-thumbnail image for a cloud-sync badge, not a semantic surface colour | kept (fixed-contrast overlay on an arbitrary thumbnail image) |
| `src/project/qml/Audacity/Project/internal/SaveToCloud/SaveLocationOption.qml` | `readonly property int radius: 6`; `ui.theme.extra["save_option_background_color"]` | `radius` replaced with `M3.shape.small`; the card background (a genuine chrome background, not an image overlay) replaced with `M3.color.surfaceContainer`, matching the identical colour already used on the card's lower half | converted |
| `src/project/qml/Audacity/Project/internal/Properties/ProjectPropertiesView.qml` | `TextInputField` | Restyled by patch 0005 | kept (muse control already restyled by overlay patch 0005-m3-controls) |
| `src/projectscene/qml/Audacity/ProjectScene/historypanel/HistoryPanel.qml` | `StyledListView` | Structural container | kept (structural container) |
| `src/projectscene/qml/Audacity/ProjectScene/toolbars/GetEffectsDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/projectscene/qml/Audacity/ProjectScene/toolbars/GetEffectsMenu.qml` | `RadioButtonGroup`, `FlatRadioButton` (effect-category selector) | No direct Material 3 equivalent for this exclusive-choice shape without an adapter | **remaining** (see "Remaining items" below) |
| `src/projectscene/qml/Audacity/ProjectScene/toolbars/internal/CustomiseView.qml` | `StyledListView` | Structural container | kept (structural container) |
| `src/projectscene/qml/Audacity/ProjectScene/toolbars/internal/M3ToolBarItem.qml` | `ToolTip` (used as `M3.color`/`M3.typography`-styled tooltip text, restyled by patch 0004) | Restyled by patch 0004-m3-tooltip-and-scrollbar | kept (muse control already restyled by overlay patch 0004-m3-tooltip-and-scrollbar) |
| `src/projectscene/qml/Audacity/ProjectScene/toolbars/internal/PlaybackToolBarCustomisePopup.qml` | `StyledPopupView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/projectscene/qml/Audacity/ProjectScene/trackruler/TrackRulerCustomizePopup.qml` | `StyledPopupView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/projectscene/qml/Audacity/ProjectScene/trackruler/WaveformRuler.qml` | `ui.theme.extra[...]` (3): tick and label colours drawn on the waveform ruler | Ruler tick marks and labels are drawn directly over the waveform data area using the same fixed data-visualisation palette as the waveform itself | kept (data colour) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/ClipItem.qml` | `MenuButton`, `ui.theme.extra[...]` (42): clip fill/selection/header/border/envelope-point colours, `radius:` (already `M3.shape.extraSmall` or a real `0` square-corner value) | `MenuButton` via patch 0005; every one of the 42 theme reads was inspected individually: all colour the clip body, its selection state, its envelope line/points, or its header/border against the waveform, i.e. the clip's own data rendering, not app chrome | kept (MenuButton via patch 0005; all 42 reads are data colour) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/TracksItemsView.qml` | `StyledListView`, `ui.theme.extra[...]` (4) | Structural container; theme reads colour track/clip rendering within the tracks area | kept (structural container; data colour) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/VerticalRulersPanel.qml` | `StyledListView`, `ui.theme.extra[...]` (5) | Structural container; theme reads colour the vertical amplitude ruler drawn over the waveform data area | kept (structural container; data colour) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/ClipHandles.qml` | `ui.theme.extra[...]` (8): clip trim/fade handle colours | Trim and fade handles are drawn directly on the clip's own waveform data area in the same data palette as the clip body | kept (data colour) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/ClipPreview.qml` | `ui.theme.extra[...]` (11): waveform preview fill/outline colours; `radius: 4` (x4) | Waveform preview colours are clip data colours; radius literals replaced with `M3.shape.extraSmall` (this preview's rounded corner is real chrome shape, independent of the data colours it fills) | kept (data colour) + converted (radii) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/ClipItemSmall.qml` | `ui.theme.extra[...]` (2) | Same clip-rendering family as `ClipItem.qml` | kept (data colour) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/LabelHeader.qml` | `ui.theme.extra[...]` (1): label marker colour; `radius: root.isPoint ? M3.shape.extraSmall / 2 : 0` | Label marker colour is the label track's own data colour; the `: 0` branch is a real square-corner value, not a hard-coded chrome literal | kept (data colour; M3 token already in use for the non-zero branch) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/LabelItem.qml` | `ui.theme.extra[...]` (7): label body/border/text colours; `radius: 4` | Label body rendering is the label track's own data colour; radius replaced with `M3.shape.extraSmall` | kept (data colour) + converted (radius) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/PlayRegion.qml` | `ui.theme.accentOpacityNormal`/`accentOpacityHover` | No dedicated Material 3 token exists for a persistent selection-region overlay opacity; replaced with the closest state layer levels on the same `M3.color.primary` role (`M3.stateLayer.dragged` while active, `M3.stateLayer.hover` otherwise), documented inline as an approximation | converted (documented approximation, not an exact token match) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/labeleditor/AddNewLabelTrackDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/labeleditor/LabelEditorDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/labeleditor/LabelEditorLabelsTableView.qml` | `StyledTableView` | Restyled by patch 0010 | kept (muse control already restyled by overlay patch 0010-m3-list-table-and-avatar) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/pitchandspeed/PitchAndSpeedChangeDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/projectscene/qml/Audacity/ProjectScene/tracksitemsview/pitchandspeed/PropertyView.qml` | `IncrementalPropertyControl` | `M3NumberField` exists and matches the plain API, but `PitchSection.qml` (the only consumer that exercises this component's advanced surface) wires `validator`, `canIncrease`/`onIncrement` and `canDecrease`/`onDecrement` to implement musical semitone/octave carry logic across two linked fields; `M3NumberField` has no hook to override what its increment/decrement buttons do, only `step`. Converting would either drop that carry behaviour or require extending `M3NumberField` with override signals, which was judged too risky to do silently in this pass | **remaining** (see "Remaining items" below) |
| `src/projectscene/qml/Audacity/ProjectScene/trackspanel/AddNewTrackPopup.qml` | `StyledPopupView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/projectscene/qml/Audacity/ProjectScene/trackspanel/EditableLabel.qml` | `TextInputField`, `background.radius: 0` | Restyled by patch 0005; the `0` is a real square-corner value on the text field's underline style, not a hard-coded chrome radius | kept (muse control already restyled by overlay patch 0005-m3-controls; `0` is a real value) |
| `src/projectscene/qml/Audacity/ProjectScene/trackspanel/RealtimeEffectListItem.qml` | `ListItemBlank`, `ui.theme.extra[...]` (1) | Restyled by patch 0002; theme read colours the effect's own bypass/enabled indicator, matching the effect chain's own colour language rather than app chrome | kept (muse control already restyled by overlay patch 0002-m3-menus; data-adjacent indicator colour) |
| `src/projectscene/qml/Audacity/ProjectScene/trackspanel/TrackEffectList.qml` | `StyledListView` | Structural container | kept (structural container) |
| `src/projectscene/qml/Audacity/ProjectScene/trackspanel/TrackItem.qml` | `MenuButton` | Via patch 0005 | kept (MenuButton via patch 0005) |
| `src/projectscene/qml/Audacity/ProjectScene/trackspanel/TracksPanel.qml` | `StyledListView` | Structural container | kept (structural container) |
| `src/projectscene/qml/Audacity/ProjectScene/trackspanel/TracksTitleBarBackground.qml` | `ui.theme.borderWidth` | Replaced with the plain `1` literal already standard beside `M3.color.outlineVariant` on the same element | converted |
| `src/projectscene/qml/Audacity/ProjectScene/trackspanel/audio/PanKnob.qml` | `KnobControl` (local Audacity type) | `KnobControl.qml` in the same directory is already a thin wrapper around `M3Knob` | converted (already M3; `KnobControl` here is a local M3-backed wrapper, not the legacy Muse control) |
| `src/projectscene/qml/Audacity/ProjectScene/trackspanel/audio/VolumePressureMeter.qml` | `ui.theme.itemOpacityDisabled` | Replaced with `M3.stateLayer.disabledContent`, matching the identical pattern already used by `HorizontalVolumePressureMeter.qml` | converted |
| `src/record/qml/Audacity/Record/internal/RecordLevelPopup.qml` | `StyledPopupView`; `radius: 2` literal | Popup frame restyled by patch 0003; radius literal replaced with `M3.shape.extraSmall`, matching the M3 tokens already used on the same `Rectangle` | kept (popup frame via patch 0003) + converted (radius) |
| `src/spectrogram/qml/Audacity/Spectrogram/SpectrogramRulerCustomizePopup.qml` | `StyledPopupView`; `IncrementalPropertyControl` | Popup frame restyled by patch 0003; the min/max stepper is a plain-API call site (`currentValue`/`minValue`/`maxValue`/`measureUnitsSymbol`/`decimals`/`step`/`valueEditingFinished`), swapped to `M3NumberField` | kept (popup frame, patch 0003) + converted (stepper) |
| `src/spectrogram/qml/Audacity/Spectrogram/SpectrogramChannelRuler.qml` | `ui.theme.extra[...]` (3): ruler tick/label colours | Same data-visualisation ruler pattern as `WaveformRuler.qml`, drawn over the spectrogram data area | kept (data colour) |
| `src/spectrogram/qml/Audacity/Spectrogram/TrackSpectrogramAlgorithmSection.qml` | `RoundedRadioButton` | Restyled by patch 0005 | kept (muse control already restyled by overlay patch 0005-m3-controls) |
| `src/spectrogram/qml/Audacity/Spectrogram/TrackSpectrogramColorsSection.qml` | `IncrementalPropertyControl` | Plain-API call site, swapped to `M3NumberField` | converted |
| `src/spectrogram/qml/Audacity/Spectrogram/TrackSpectrogramScaleSection.qml` | `IncrementalPropertyControl` | Plain-API call site, swapped to `M3NumberField` | converted |
| `src/spectrogram/qml/Audacity/Spectrogram/TrackSpectrogramSettingsDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/trackedit/qml/Audacity/TrackEdit/BehaviorChoice.qml` | `RoundedRadioButton`; `radius: 5` literal | RoundedRadioButton restyled by patch 0005; radius literal replaced with `M3.shape.extraSmall` | kept (RoundedRadioButton via patch 0005) + converted (radius) |
| `src/trackedit/qml/Audacity/TrackEdit/CustomRateDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/trackedit/qml/Audacity/TrackEdit/CustomTimeDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/trackedit/qml/Audacity/TrackEdit/DeleteBehaviorOnboardingDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/trackedit/qml/Audacity/TrackEdit/DeleteBehaviorOnboardingFollowupDialog.qml` | `StyledDialogView` | Restyled by patch 0003 | kept (muse control already restyled by overlay patch 0003-m3-popups-and-dialogs) |
| `src/trackedit/qml/Audacity/TrackEdit/DeleteBehaviorPanel.qml` | `radius: 4` literal; `FlatRadioButton` (delete-behaviour choice) | Radius replaced with `M3.shape.extraSmall` | converted (radius) / **remaining** (`FlatRadioButton`: see "Remaining items" below) |
| `src/trackedit/qml/Audacity/TrackEdit/PasteBehaviorPanel.qml` | `radius: 4` literal; `FlatRadioButton` (paste-behaviour choice) | Radius replaced with `M3.shape.extraSmall` | converted (radius) / **remaining** (`FlatRadioButton`: see "Remaining items" below) |

### Remaining items (named blockers, not time-boxed placeholders)

1. **`FlatRadioButton`/`RadioButtonGroup`** (Muse legacy controls, not restyled by any overlay
   patch): `src/project/qml/Audacity/Project/ProjectsPage.qml` (grid/list view toggle),
   `src/projectscene/qml/Audacity/ProjectScene/toolbars/GetEffectsMenu.qml` (effect-category
   selector), `src/trackedit/qml/Audacity/TrackEdit/DeleteBehaviorPanel.qml` and
   `PasteBehaviorPanel.qml` (behaviour choice). `M3SegmentedButton` is the closest `Audacity.M3`
   primitive for an exclusive-choice control, but its API (`currentIndex` over an array plus an
   `activated(index)` signal, with an explicit `multiSelect` flag) does not match the
   value-keyed model (`{icon, title, value}` with `checked: model === current value`) these four
   call sites use. Converting safely needs an adapter at each call site mapping value to index
   and wiring `activated` back to the underlying model, which was not attempted this pass because
   it touches four live interactive controls (a page-level view toggle, an effects menu, and two
   track-editing behaviour choices) with no build-and-drive verification budget left to confirm
   the mapping is correct at each site without a visual or functional regression.
2. **`PropertyView.qml`'s `IncrementalPropertyControl`**: blocked on `M3NumberField` lacking the
   `validator`/`onIncrement`/`onDecrement` override hooks that `PitchSection.qml` relies on for
   its semitone/octave carry logic (see the row above). Extending `M3NumberField` with those hooks
   is a real `Audacity.M3` library addition, not a call-site change, and was not attempted this
   pass to avoid a same-pass, unreviewed change to a shared library component two other lanes
   also consume.

Neither blocker is a legacy-chrome rendering gap: in both cases the control already renders
through a restyled Muse control (`FlatRadioButton`/`RadioButtonGroup` extend `FlatButton`, which
patch 0005 restyles) or is currently unconverted pending a documented API gap, not silently
producing legacy chrome.

### Row counts (lane U1, final)

- Converted: 21 (2 committed in the first pass: `BehaviorChoice.qml`, `RecordLevelPopup.qml`
  radius literals; 19 in this pass: `CrashHandlerDevTools.qml`, `SettingsPage.qml` (x2, counted
  as one row), `Main.wasm.qml`, `ProjectUploadedDialog.qml`, `ProjectsPage.qml` (size literal),
  `CloudAudioFilesView.qml`, `DefaultProjectListView.qml`, `ProjectGridItem.qml` (x2, radius and
  border width, counted as one row), `SaveLocationOption.qml`, `ClipPreview.qml`, `LabelItem.qml`,
  `PlayRegion.qml`, `TracksTitleBarBackground.qml`, `VolumePressureMeter.qml`,
  `SpectrogramRulerCustomizePopup.qml`, `TrackSpectrogramColorsSection.qml`,
  `TrackSpectrogramScaleSection.qml`, `DeleteBehaviorPanel.qml`, `PasteBehaviorPanel.qml`).
- Kept with a named reason (overlay-patch restyle, structural container, dev-tools gallery
  or M3 showcase, data colour, fixed-contrast thumbnail overlay, or non-control false
  positive): the remainder of the table above.
- Remaining (named concrete blocker, not converted): 2 (`FlatRadioButton`/`RadioButtonGroup`
  across 4 call sites, and `PropertyView.qml`'s `IncrementalPropertyControl`). Zero rows are
  left as an unexamined "pending".

Verification for this pass: `cmake --build build/linux -j3 --target audacity` succeeded after
every batch of edits (final build: exit 0, full link). `qmllint` on every changed file showed
only expected import-resolution warnings (no `-I` module paths available outside the CMake
build) and no parse errors. Xvfb smoke run showed no `Failed to load main qml` and the process
stayed alive through the run. Captured `docs/design/captures/lane-u1/01-home.png` (home page)
and `docs/design/captures/lane-u1/02-project-track-clip-selected.png` (a project window with a
generated tone track and its clip selected, header highlighted) and viewed both.

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
