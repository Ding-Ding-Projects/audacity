# Audacity Material Design 3 component library

Every component lives in `src/uicomponents/qml/Audacity/M3/` and is reached with

```qml
import Audacity.M3
```

The public API of each component is kept close to the muse component it
replaces, so that call sites elsewhere in the application can be switched over
almost mechanically. Every interactive component also carries, without being
repeated in the table below: `enabled`, `navigation` (an alias to its own
`NavigationControl`), `accessibleName`, a state layer, a ripple, a three pixel
focus ring and durations taken from `M3.motion`.

See `DESIGN.md` for the tokens these components are built from. When a binding
needs a colour role whose name is only known at run time, read the reactive map
`M3.color.roles[name]` rather than calling `M3.color.role(name)`, because a
method call does not re-evaluate when the theme changes.

## Buttons and actions

| Component | Public API | Replaces |
| --- | --- | --- |
| `M3Button` | `text`, `icon`, `variant` (`filled`, `tonal`, `outlined`, `text`, `elevated`), `loading`, `minWidth`, `horizontalPadding`, `toolTipTitle`, `toolTipDescription`, `toolTipShortcut`, `accentButton`, `buttonId`, `buttonRole`, `isLeftSide`, `accessible`, `clicked()` | `FlatButton` |
| `M3IconButton` | `icon`, `variant` (`standard`, `filled`, `tonal`, `outlined`), `checkable`, `checked`, `clicked()`, `toggled(checked)` | `FlatButton` with an icon and no text |
| `M3FAB` | `icon`, `text`, `size` (`small`, `regular`, `large`, `extended`), `variant` (`primary`, `secondary`, `tertiary`, `surface`), `lowered`, `clicked()` | `FlatButton` used as a primary accent action |
| `M3SegmentedButton` | `model`, `currentIndex`, `multiSelect`, `checkedIndexes`, `navigationPanel`, `navigationRowStart`, `activated(index)` | `RadioButtonGroup` used as a toolbar selector |
| `M3Chip` | `text`, `variant` (`assist`, `filter`, `input`, `suggestion`), `icon`, `checked`, `elevated`, `clicked()`, `toggled(checked)`, `removed()` | no direct muse equivalent |

## Selection controls

| Component | Public API | Replaces |
| --- | --- | --- |
| `M3Switch` | `checked`, `showIcon`, `text`, `toggled(checked)` | `ToggleButton` |
| `M3Checkbox` | `checked`, `indeterminate`, `text`, `touchTargetSize`, `clicked()` | `CheckBox` |
| `M3RadioButton` | `checked`, `text`, `toggled()` | `RoundedRadioButton` |

## Value entry

| Component | Public API | Replaces |
| --- | --- | --- |
| `M3Slider` | `value`, `from`, `to`, `stepSize`, `orientation`, `showTicks`, `showValueIndicator`, `valueText`, `trackThickness`, `handleWidth`, `handleLength`, `handleItem`, `setValue(value)`, `step(direction)`, `moved()` | `StyledSlider` |
| `M3RangeSlider` | `first`, `second`, `from`, `to`, `stepSize`, `orientation`, `navigationPanel`, `moved()` | no direct muse equivalent |
| `M3Knob` | `value`, `from`, `to`, `stepSize`, `radius`, `bidirectional`, `accentControl`, `valueText`, `showValueIndicator`, `mouseArea`, `newValueRequested(value)`, `moved()`, `mouseEntered()`, `mouseExited()`, `mousePressed()`, `mouseReleased()` | `KnobControl`, which was a `QtQuick.Controls` `Dial` |
| `M3TextField` | `currentText`, `label`, `placeholder`, `supportingText`, `errorText`, `hasError`, `variant` (`filled`, `outlined`), `leadingIcon`, `trailingIcon`, `isPassword`, `readOnly`, `maximumLength`, `textEdited(text)`, `textEditingFinished(text)`, `trailingIconClicked()`, `clear()` | `TextInputField` |
| `M3Dropdown` | `model`, `currentIndex`, `currentText`, `currentValue`, `textRole`, `valueRole`, `label`, `placeholder`, `menuModel`, `displayText`, `fieldHeight`, `menuNavigationPanel`, `opened`, `activated(index, value)`, `handleMenuItem(itemId)` | `StyledDropdown` |
| `M3SearchBar` | `searchText`, `placeholder`, `showRegexBuilder`, `accepted()`, `regexBuilderRequested()`, `clear()` | `SearchField` |
| `M3FilePicker` | `pickerType`, `path`, `dialogTitle`, `filter`, `dir`, `buttonText`, `buttonWidth`, `buttonOrientation`, `showPathField`, `pathFieldTitle`, `pathFieldWidth`, `spacing`, `navigation`, `navigationRowOrderStart`, `navigationColumnOrderStart`, `pathEdited(newPath)` | `FilePicker` |
| `M3DatePicker` | `selectedDate`, `displayedMonth`, `minimumDate`, `maximumDate`, `navigationPanel`, `dateSelected(date)` | no muse equivalent |
| `M3TimePicker` | `hours`, `minutes`, `use24Hour`, `navigationPanel`, `timeChanged(hours, minutes)` | no muse equivalent |
| `M3ColorPicker` | `selection`, `allowRainbow`, `rainbowSpeed`, `contrastBackground`, `format`, `alpha`, `gamutClipped`, `navigationPanel`, `accepted()` | no muse equivalent |

`M3ColorPicker` translates one colour between the named list, HEX, HEX8, RGB,
RGBA, HSL, HSLA, HSV, HWB, CIELAB, LCH, OKLab, OKLCH and CMYK. The maths lives
in `internal/M3ColorFormats.js`. The picker warns when a requested colour lies
outside the sRGB gamut and had to be clipped, and shows the WCAG contrast ratio
and grade against `contrastBackground`. Its animated rainbow choice is stored as
the sentinel string `"rainbow"` and never as a colour value, its speed is a
level from 1 to 5 rather than a duration, and under reduced motion it settles on
one hue.

## Menus

| Component | Public API | Replaces |
| --- | --- | --- |
| `M3Menu` | `model`, `searchable`, `filterText`, `navigationPanel`, `handleMenuItem(itemId)`, `regexBuilderRequested()`, `open()`, `close()` | `StyledMenu` |
| `M3MenuItem` | `text`, `icon`, `shortcut`, `checkable`, `checked`, `hasSubMenu`, `isSeparator`, `triggered()`, `subMenuRequested()` | `StyledMenuItem` |

`M3Menu` is built on `StyledPopupView`, so the application's popup stack,
escape handling and navigation sections keep working. Its model entries take
`id`, `title`, `shortcut`, `icon`, `checkable`, `checked`, `enabled`,
`separator` and `subitems`. Submenus are loaded lazily by file name because a
QML component cannot instantiate itself directly.

## Containment

| Component | Public API | Replaces |
| --- | --- | --- |
| `M3Card` | `variant` (`elevated`, `filled`, `outlined`), `clickable`, `padding`, default content slot, `clicked()` | no direct muse equivalent |
| `M3Dialog` | `icon`, `headline`, `supportingText`, `fullScreen`, `actions` slot, default body slot | `StyledDialogView` used directly |
| `M3BottomSheet` | `opened`, `headline`, `showDragHandle`, `sheetHeight`, `open()`, `close()`, `closed()` | no muse equivalent |
| `M3SideSheet` | `opened`, `modal`, `headline`, `edge`, `sheetWidth`, `open()`, `close()`, `closed()` | no muse equivalent |
| `M3ListItem` | `headline`, `supportingText`, `overline`, `leadingIcon`, `trailingText`, `leadingContent`, `trailingContent`, `selected`, `clickable`, `clicked()` | `ListItemBlank` |
| `M3Divider` | `orientation`, `inset`, `thickness` | `SeparatorLine` |

`M3Dialog` is built on `StyledDialogView` so that the application's interactive
provider still opens it by URI.

## Navigation

| Component | Public API | Replaces |
| --- | --- | --- |
| `M3TopAppBar` | `title`, `size` (`small`, `centerAligned`, `medium`, `large`), `navigationIcon`, `actions` slot, `flickable`, `navigationPanel`, `navigationIconTriggered()` | no muse equivalent |
| `M3NavigationRail` | `model`, `currentIndex`, `showLabels`, `fabIcon`, `navigationPanel`, `activated(index)`, `fabTriggered()` | no muse equivalent |
| `M3NavigationDrawer` | `model`, `currentIndex`, `headline`, `modal`, `opened`, `drawerWidth`, `navigationPanel`, `activated(index)`, `open()`, `close()` | no muse equivalent |
| `M3Tabs` | `model`, `currentIndex`, `primary`, `orientation`, `navigationPanel`, `currentItem`, `activated(index)` | `StyledTabBar` |
| `M3Tab` | `text`, `icon`, `selected`, `primary`, `orientation`, `badgeCount`, `clicked()` | `StyledTabButton` |

`M3TopAppBar` collapses from the medium or large size down to the small size as
its `flickable` scrolls, and lifts its container colour to surface container as
soon as content sits underneath. `M3Tabs` supports a vertical orientation for
dockable panel strips.

## Communication

| Component | Public API | Replaces |
| --- | --- | --- |
| `M3Snackbar` | `text`, `actionText`, `showClose`, `duration`, `actionTriggered()`, `dismissed()` | no muse equivalent |
| `M3SnackbarHost` | `show(text, actionText, duration)`, `dismissCurrent()`, `actionTriggered(message)` | no muse equivalent |
| `M3Tooltip` | `text`, `subhead`, `supportingText`, `rich`, `actionText`, `secondaryActionText`, `show()`, `hide()`, `actionTriggered()`, `secondaryActionTriggered()` | `StyledToolTip` for rich tooltips |
| `M3LinearProgress` | `value`, `from`, `to`, `indeterminate`, `wavy` | `ProgressBar` |
| `M3CircularProgress` | `value`, `from`, `to`, `indeterminate`, `wavy`, `running`, `implicitSize`, `strokeWidth`, `indicatorColor`, `trackColor` | no muse equivalent |
| `M3Badge` | `count`, `maxCount`, `showCount`, `accessibleText` | no muse equivalent |

For an ordinary one line hint, prefer `toolTipTitle` on the component, which
goes through the muse tooltip provider. `M3Tooltip` is for the rich tooltip
case, where a subhead, supporting text and actions are needed.

A badge is decorative. The component that hosts it must fold `accessibleText`
into its own accessible name so that the count is announced once, in context.

## Where the overlay draws instead of a component

Three surfaces reach the screen through the muse framework rather than through
this module. In each case the Material Design 3 anatomy is produced by the
patch overlay described in `MUSE_OVERLAY.md`, and the reason for that choice is
recorded here.

### Application menu bar

`src/appshell/qml/Audacity/AppShell/platform/AppMenuBar.qml` draws its own top
level items with Material chrome, but the dropdown itself is opened through
`StyledMenuLoader`. Driving `M3Menu` from `AppMenuModel` instead was tried and
set aside: the bar depends on `menuLoader.menu.subMenuLoader` to report the
opened area back to the model, on the loader for hover switching between top
level menus while a menu is open, and on the muse navigation section for
mnemonic and arrow key handling. None of those exist on `M3Menu`, and adding
them would duplicate the loader rather than replace it.

The chosen route is therefore the overlay. Patch `0002-m3-menus` gives
`StyledMenu` and `StyledMenuItem` the same anatomy as `M3Menu` and
`M3MenuItem`: a 4 dp container corner, elevation level 2, 8 dp vertical
padding, 48 dp rows, 12 dp horizontal padding, label large row and shortcut
text, the shortcut and the submenu arrow in the on-surface-variant role,
dividers in outline-variant, and hover, pressed and selected feedback drawn as
Material state layers by the patched `ListItemBlank`.

### Dialog button rows

The nine dialogs listed in `MUSE_OVERLAY.md` keep the muse `ButtonBox`, and
their buttons are now `M3Button`. `M3Button` carries `buttonId`, `buttonRole`,
`isLeftSide`, `accentButton` and an `accessible` alias, which is everything the
box and its C++ model read. Patch `0008-m3-button-box` removes the four
`as FlatButton` casts the box used when it inspected its own children, so the
cast no longer discards a host application button.

Replacing the box with a plain row of buttons was the alternative. It was
rejected because the box owns the platform button order, the accept and reject
key defaults, the navigation panel and the first focus button, and a hand
written row would have had to reproduce all of that.

### Shortcut preferences page

`ShortcutsPreferencesPage` hosts the muse `ShortcutsPage`, which owns the
shortcut model, the sequence editor dialog and the fuzzy search filter. Patch
`0009-m3-shortcuts-page` restyles it in place: key sequences are drawn as
Material chips through a new `valueChips` property on `ValueList`, and the
search field gained a regular expression builder action and a
`regexBuilderRequested` signal that the page forwards. Audacity answers that
signal with its own `RegexBuilderSheet`, so the shortcut search has the same
regular expression builder as every other search surface in the application.

### Table view, page indicator and account avatar

`StyledTableView`, `PageIndicator` and `AccountAvatar` are muse components
with no product level replacement: a table view, a dot row for a paged
carousel and a cloud account picture. Rather than reimplement them, patch
`0010-m3-list-table-and-avatar` replaces their raw theme colours
(`backgroundPrimaryColor`, `backgroundSecondaryColor`, `strokeColor`,
`fontPrimaryColor`) with the `M3Roles` singleton, the same bridge patch
`0001-m3-roles-singleton` already gives every other overlay patch. The table
background and header fills use `surface` and `surfaceContainer`, its rules
and borders use `outlineVariant`, the page indicator dot uses `primary` with
a clearer contrast between the current and other pages, and the avatar
placeholder background and border use `surfaceContainer` and
`outlineVariant`.

## Reusable primitives

| Component | Public API | Purpose |
| --- | --- | --- |
| `M3StateLayer` | `color`, `hovered`, `pressed`, `focused`, `dragged`, `active`, `targetOpacity` | The Material 3 state layer |
| `M3Ripple` | `color`, `maxOpacity`, `press(position)`, `pulse()` | The expanding circle, reduced motion aware |
| `M3FocusRing` | `shapeRadius`, `ringColor`, `thickness`, `offset` | The three pixel focus indicator |
| `M3Elevation` | `level`, `radius`, `shadowColor` | The key and ambient shadows for a level |
| `M3Surface` | `level`, `shadowVisible`, `outlined`, `outlineColor` | A tonal surface with its shadow and radius |

`internal/M3ToolTipHandler.qml` is internal to the module. It wraps the muse
tooltip provider so that every component gets the same delay, placement and
dismissal behaviour.

## Developer gallery

`src/appshell/qml/Audacity/AppShell/DevTools/Gallery/M3ComponentsGallery.qml`
shows every component in every variant and state, plus a token page listing all
colour roles with their contrast and the whole type scale. It is reachable from
the developer tools page under "M3 Gallery".

For a deterministic capture, set

```
AU_M3_GALLERY_ROUTE="<component>:<state>:<theme>:<scale>"
```

for example `AU_M3_GALLERY_ROUTE="M3Button:hover:dark:1.5"`. The component part
selects the gallery entry. The state part is recorded. The theme part switches
the whole application to `light`, `dark`, `high_contrast_white` or
`high_contrast_black` through `M3.applyScheme()`, and the gallery then reports
the requested theme next to the one actually applied so a harness can assert it
got what it asked for. The scale part sets the preview scale. Any part may be
left empty, and an empty theme part leaves the user's own theme alone. The
value is read once through `M3.captureRoute()`, so the route is stable for the
life of the page.

The gallery is reached by opening the DevTools page from the application's
title bar and choosing "M3 Gallery" in its list. There is no command line
switch for it; the route variable only chooses what the page shows once it is
open.
