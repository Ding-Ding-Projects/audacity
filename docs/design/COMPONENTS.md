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

See `DESIGN.md` for the tokens these components are built from.

## Buttons and actions

| Component | Public API | Replaces |
| --- | --- | --- |
| `M3Button` | `text`, `icon`, `variant` (`filled`, `tonal`, `outlined`, `text`, `elevated`), `loading`, `minWidth`, `toolTipTitle`, `toolTipDescription`, `toolTipShortcut`, `clicked()` | `FlatButton` |
| `M3IconButton` | `icon`, `variant` (`standard`, `filled`, `tonal`, `outlined`), `checkable`, `checked`, `clicked()`, `toggled(checked)` | `FlatButton` with an icon and no text |
| `M3FAB` | `icon`, `text`, `size` (`small`, `regular`, `large`, `extended`), `variant` (`primary`, `secondary`, `tertiary`, `surface`), `lowered`, `clicked()` | `FlatButton` used as a primary accent action |
| `M3SegmentedButton` | `model`, `currentIndex`, `multiSelect`, `checkedIndexes`, `navigationPanel`, `navigationRowStart`, `activated(index)` | `RadioButtonGroup` used as a toolbar selector |
| `M3Chip` | `text`, `variant` (`assist`, `filter`, `input`, `suggestion`), `icon`, `checked`, `elevated`, `clicked()`, `toggled(checked)`, `removed()` | no direct muse equivalent |

## Selection controls

| Component | Public API | Replaces |
| --- | --- | --- |
| `M3Switch` | `checked`, `showIcon`, `text`, `toggled(checked)` | `ToggleButton` |
| `M3Checkbox` | `checked`, `indeterminate`, `text`, `clicked()` | `CheckBox` |
| `M3RadioButton` | `checked`, `text`, `toggled()` | `RoundedRadioButton` |

## Value entry

| Component | Public API | Replaces |
| --- | --- | --- |
| `M3Slider` | `value`, `from`, `to`, `stepSize`, `orientation`, `showTicks`, `showValueIndicator`, `valueText`, `moved()` | `StyledSlider` |
| `M3RangeSlider` | `first`, `second`, `from`, `to`, `stepSize`, `orientation`, `navigationPanel`, `moved()` | no direct muse equivalent |
| `M3TextField` | `currentText`, `label`, `placeholder`, `supportingText`, `errorText`, `hasError`, `variant` (`filled`, `outlined`), `leadingIcon`, `trailingIcon`, `isPassword`, `readOnly`, `maximumLength`, `textEdited(text)`, `textEditingFinished(text)`, `trailingIconClicked()`, `clear()` | `TextInputField` |
| `M3Dropdown` | `model`, `currentIndex`, `currentText`, `currentValue`, `textRole`, `valueRole`, `label`, `placeholder`, `activated(index, value)` | `StyledDropdown` |
| `M3SearchBar` | `searchText`, `placeholder`, `showRegexBuilder`, `accepted()`, `regexBuilderRequested()`, `clear()` | `SearchField` |
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
| `M3Tabs` | `model`, `currentIndex`, `primary`, `orientation`, `navigationPanel`, `activated(index)` | `StyledTabBar` |
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
selects the gallery entry, the state part is recorded, the theme part is
reported back next to the theme that was actually applied so a harness can
assert it got what it asked for, and the scale part sets the preview scale. Any
part may be left empty. The value is read once through
`M3.captureRoute()`, so the route is stable for the life of the page.
