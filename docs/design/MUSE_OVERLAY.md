# Material Design 3 overlay for the muse framework

## Why an overlay instead of a fork

Audacity 4 uses the MuseScore `muse` framework as a git submodule, pinned to commit
`ca86211f8d5e45405de999827c43bdb24e4d682e`. Parts of the user interface are drawn by muse
itself and cannot be restyled from Audacity code: dock window chrome, dock title bars,
dock tab bars, separators, drop indicators, context menus, popup views, dialog frames,
tooltips, scroll bars, and the shared controls that muse-internal dialogs use.

The normal way to change those files is to fork muse and point the submodule at the fork.
That route is blocked in this environment:

- Creating a repository under the organization through the GitHub integration returns
  HTTP 403.
- Attaching an existing repository from another organization is refused by the session
  tooling.

Without a place to publish a fork, the submodule pointer cannot be moved. The changes are
therefore kept in this repository as numbered unified diffs and applied to the submodule
working tree at configure time. The submodule pointer stays at the pinned commit and
nothing is committed inside `muse/`.

## Mechanism

- Patches live in `buildscripts/muse-patches/` as `0001-*.patch` and upwards. They are
  ordinary `git -C muse diff` output with a short subject header, applied with `-p1`.
- `buildscripts/cmake/ApplyMusePatches.cmake` is included from the top-level
  `CMakeLists.txt` immediately after `MUSE_FRAMEWORK_PATH` is set. For each patch, in
  file name order, it runs `git -C muse apply --check --reverse`. Success means the patch
  is already applied and it is skipped; otherwise the patch is verified with
  `git -C muse apply --check` and then applied. A patch that neither reverses nor applies
  fails configure with a message naming the patch and pointing at the regenerate command.
  Reconfiguring is therefore idempotent.
- The option `AU_APPLY_MUSE_PATCHES` (default `ON`) disables the overlay.
- `buildscripts/tools/muse_patches.py` provides `apply`, `revert`, `regenerate` and
  `status`. The workflow for future changes is: edit files inside `muse/`, then run
  `python3 buildscripts/tools/muse_patches.py regenerate`. The logical file groups that
  decide which change lands in which patch file are declared in that script.

## Colour roles

Patch `0001` adds a QML singleton `M3Roles` to the `Muse.Ui` module. It reads Material
Design 3 system colour roles from the theme extra map, using the key convention
`m3_<role>` (for example `ui.theme.extra["m3_surface_container_high"]`), and falls back
to the closest legacy muse theme colour when a key is absent. The Audacity theme configs
in `src/app/configs/` publish those keys. Because every lookup has a fallback, the patched
muse still renders correctly for a host that does not publish any `m3_` key.

The singleton also exposes the Material Design 3 state layer opacities (8 percent hover,
10 percent focus and pressed, 38 percent disabled content), the shape scale (4, 8, 12, 16,
28), and motion helpers. `M3Roles.reducedMotion` reads `m3_reduced_motion` from the extra
map and `M3Roles.duration(ms)` returns 0 when it is true, so every animation the overlay
touches has a complete reduced-motion path.

## What each patch changes

### 0001-m3-roles-singleton.patch

- Adds `framework/ui/qml/Muse/Ui/M3Roles.qml`, a `pragma Singleton` object exposing
  surface, on-surface, primary, secondary container, outline, inverse, error, scrim and
  shadow roles, plus state layer opacities, the shape scale and the motion helpers.
- Registers the file in `framework/ui/qml/Muse/Ui/CMakeLists.txt` with
  `QT_QML_SINGLETON_TYPE`.

### 0002-m3-menus.patch

- `internal/StyledMenu.qml`: 4 dp container corner (extra-small shape), elevation level 2,
  8 dp vertical padding of the item list, dividers drawn with the outline-variant role.
- `internal/StyledMenuItem.qml`: 48 dp item height, 12 dp horizontal padding, label large
  row and shortcut text, trailing shortcut text and submenu arrow in on-surface-variant.
- `ListItemBlank.qml`: hover, pressed and selected feedback rewritten as Material Design 3
  state layers (on-surface at 8 and 10 percent) with a secondary-container selected state.
- `SeparatorLine.qml`: dividers use outline-variant.

### 0003-m3-popups-and-dialogs.patch

- `internal/PopupContent.qml`: new `cornerRadius`, `elevationLevel`, `backgroundColor`,
  `borderColor`, `horizontalMargins` and `verticalMargins` properties; surface-container-high
  background; outline-variant border; elevation-driven shadow; open and close motion changed
  to a 150 ms standard-decelerate scale and fade that collapses to 0 ms under reduced motion.
- `StyledPopupView.qml`: exposes the new frame properties, keeping the 12 dp popup shape.
- `StyledDialogView.qml`: 28 dp extra-large shape, surface-container-high background,
  level 3 elevation.
- `PopupPanel.qml`: surface-container-high, outline-variant border, 28 dp top corners.

### 0004-m3-tooltip-and-scrollbar.patch

- `StyledToolTip.qml`: plain tooltip anatomy (inverse-surface background, inverse-on-surface
  text, 4 dp radius, 8 by 4 padding, no border, no elevation); a tooltip with a description
  is treated as a rich tooltip and keeps a 12 dp radius on surface-container-high.
- `StyledScrollBar.qml`: 4 dp thin bar that expands to 8 dp on hover, on-surface-variant at
  38 percent, 62 percent while pressed, with reduced-motion-aware transitions.

### 0005-m3-controls.patch

- `FlatButton.qml`: filled tonal button by default (secondary container, on-secondary-container
  label), filled primary when `accentButton` is set, text button when `transparent` is set;
  40 dp height, fully rounded 20 dp shape, a dedicated state layer with hover, pressed and
  focus opacities and a reduced-motion-aware transition.
- `FlatToggleButton.qml`: 40 dp round icon toggle button with a state layer and primary fill
  when checked.
- `CheckBox.qml`: 18 dp box, 2 dp corner, 2 dp outline when unselected, primary fill and
  on-primary check mark when selected, 40 dp round state layer.
- `RoundedRadioButton.qml`: 20 dp outline at 2 dp, primary when selected, 10 dp primary dot,
  40 dp round state layer.
- `StyledSlider.qml`: 4 dp track, secondary-container inactive track, primary active track,
  20 by 4 primary handle with a state layer.
- `ProgressBar.qml`: fully rounded track in secondary container with a primary indicator.
- `StyledBusyIndicator.qml`: secondary-container track, primary indicator, rotation stopped
  under reduced motion.
- `StyledTabBar.qml` and `StyledTabButton.qml`: Material Design 3 secondary tabs, 48 dp tall,
  16 dp horizontal padding, on-surface and on-surface-variant labels, 2 dp primary indicator,
  state layer, and a divider along the bar.
- `TextInputField.qml` and `SearchField.qml`: outlined text field at 40 dp with an
  outline-role border that becomes a 2 dp primary border on focus; the search field is fully
  rounded with 16 dp side padding.
- `StyledDropdown.qml`: 40 dp outlined menu button with a state layer.

### 0008-m3-button-box.patch

- `ButtonBox.qml`: the four places where the box cast its own children to `FlatButton`
  before reading `buttonId`, `buttonRole`, `isLeftSide`, `accentButton` and `navigation`
  now read those properties without a cast. A host application can therefore put its own
  button component in a button box and keep the platform button order, the accept and
  reject defaults, the navigation panel and the first focus button. The nine Audacity
  dialogs that use a button box hold `M3Button` children after this change. The button the
  box creates itself for a standard button is still a `FlatButton`, which patch `0005`
  already gives Material Design 3 anatomy.

### 0009-m3-shortcuts-page.patch

- `ValueList.qml` and `internal/ValueListItem.qml`: a new `valueChips` property draws a
  read only value as a Material Design 3 chip, a 32 dp tall outline container with a 8 dp
  corner and 12 dp side padding. It is off by default, so every other value list is
  unchanged.
- `internal/ShortcutsList.qml`: turns `valueChips` on, so a key sequence reads as a chip.
- `internal/ShortcutsTopPanel.qml`: adds a regular expression builder action beside the
  search field and a `regexBuilderRequested` signal.
- `ShortcutsPage.qml`: forwards `regexBuilderRequested` and exposes `setSearchText`, so the
  host application can open its own builder and write the accepted pattern back.
  Audacity's `ShortcutsPreferencesPage` answers it with a `RegexBuilderSheet` under the
  store name `shortcuts-preferences`.

### 0006-m3-dock-chrome.patch

- `DockFrame.qml`: surface background with an 8 dp panel shape; the drag highlight becomes a
  Material Design 3 drop indicator (2 dp primary border, primary fill at 38 percent).
- `DockTitleBar.qml`: surface-container background, on-surface title, 16 dp leading padding.
- `DockTabBar.qml`: surface-container background.
- `DockPanelTab.qml`: secondary tab metrics, on-surface and on-surface-variant labels, state
  layer, 2 dp primary indicator on the current tab.
- `DockSeparator.qml`: 1 px outline-variant.
- `DockFloatingWindow.qml`: surface-container background, outline-variant border, 12 dp shape.
- `DockingHolder.qml`: primary drop indicator at 38 percent.

### 0007-m3-interactive-dialogs.patch

- `StandardDialog.qml`: 24 dp dialog padding and 24 dp content spacing.
- `StandardDialogPanel.qml`: 24 dp hero icon coloured with the primary role (error role for
  warning and error dialogs), on-surface headline, on-surface-variant supporting text.

## Verification

- `qmllint` from `/opt/Qt/6.10.0/gcc_64/bin` was run over every touched QML file and over
  `M3Roles.qml`, against an import path assembled from the built muse QML modules. The
  patched tree produces fewer diagnostics than the unpatched tree (226 against 307), no
  errors, and no diagnostic category that the unpatched tree does not already produce. The
  remaining diagnostics are the pre-existing `unqualified` and `property-changes-parsed`
  warnings that muse emits upstream.
- `python3 buildscripts/tools/muse_patches.py revert` followed by `apply`, then `apply`
  again, proves the overlay is reversible and idempotent.
- `cmake -S . -B /tmp/au-cfgcheck -G Ninja` with the patch step enabled completes
  configuration.

## Not covered

- The 32 percent modal scrim is not implemented. Muse dialogs are separate frameless
  windows created from C++ (`DialogView`), so a scrim would have to be painted by the host
  window rather than by the dialog QML. It is left for the window-level work.
- The overlay changes appearance only. No navigation, accessibility or model behaviour in
  muse was modified.
