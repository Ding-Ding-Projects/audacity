# Audacity Material Design 3 contract

This is the design contract for the Material Design 3 rewrite of the Audacity 4
interface. Everything below is enforced by the token engine in
`src/uicomponents/components/m3themeprovider.{h,cpp}`, the colour maths in
`src/uicomponents/components/m3hct.{h,cpp}`, and the component library in
`src/uicomponents/qml/Audacity/M3/`.

## The token engine

QML reaches the tokens through a single singleton:

```qml
import Audacity.M3

Rectangle {
    color: M3.color.surfaceContainer
    radius: M3.shape.medium
}
```

The singleton is registered from C++ in `src/uicomponents/uicomponentsmodule.cpp`
and exposes seven grouped objects: `color`, `typography`, `shape`, `motion`,
`stateLayer`, `elevation` and `density`.

Nothing in the component library reads a raw hexadecimal colour, a hard coded
duration or a hard coded corner radius. If a value is needed that the tokens do
not carry, the token set is extended rather than the value inlined.

## Seed and scheme

The whole palette is generated from one seed colour by a port of
material-color-utilities: sRGB to CAM16 to HCT, then tonal palettes, then the
Material 3 role table.

- Default seed: `#926BFF`.
- Default scheme variant: `tonal_spot`.
- Other variants: `vibrant`, `expressive`, `neutral`, `monochrome`, `fidelity`.

Both are persisted through muse settings:

| Setting key            | Meaning                                  | Default      |
| ---------------------- | ---------------------------------------- | ------------ |
| `ui/m3/seedColor`      | Seed colour as a hexadecimal string      | `#926BFF`    |
| `ui/m3/variant`        | Scheme variant name                      | `tonal_spot` |
| `ui/m3/reducedMotion`  | User preference for reduced motion       | `false`      |
| `ui/m3/density`        | Density level, `-3` to `0`               | `0`          |

Changing any of them re-emits `M3.themeChanged`, so every binding refreshes.

The active scheme follows the existing muse theme selection rather than
replacing it. `M3ThemeProvider` reads
`IUiConfiguration::currentTheme().codeKey` and picks the light, dark, high
contrast white or high contrast black tone table, so the theme switch that
already exists in preferences keeps working.

## Colour roles

Every Material 3 system colour role is available on `M3.color`, in camelCase:

- Primary: `primary`, `onPrimary`, `primaryContainer`, `onPrimaryContainer`,
  `inversePrimary`, `primaryFixed`, `primaryFixedDim`, `onPrimaryFixed`,
  `onPrimaryFixedVariant`.
- Secondary and tertiary: the same nine roles for each.
- Error: `error`, `onError`, `errorContainer`, `onErrorContainer`.
- Neutral: `background`, `onBackground`, `surface`, `surfaceDim`,
  `surfaceBright`, `surfaceContainerLowest`, `surfaceContainerLow`,
  `surfaceContainer`, `surfaceContainerHigh`, `surfaceContainerHighest`,
  `onSurface`, `surfaceVariant`, `onSurfaceVariant`, `outline`,
  `outlineVariant`, `inverseSurface`, `inverseOnSurface`, `surfaceTint`,
  `scrim`, `shadow`.

`M3.color.role("surfaceContainerHigh")` looks a role up by name and
`M3.color.roleNames()` lists them. A method call is not a binding dependency,
so a binding whose role name is chosen at run time must read the reactive map
`M3.color.roles[name]` instead, which updates when the theme changes. The
developer gallery uses `roleNames()` for its model and `roles` for its
swatches.

### Contrast guarantee

Every "on" role reaches at least a 4.5 to 1 contrast ratio against its pair, in
all four schemes and all six variants. This is asserted by
`src/uicomponents/tests/m3theme_tests.cpp` and is the reason the tone table is
data rather than hand picked colours. `M3.contrastRatio(a, b)` exposes the same
WCAG 2.x calculation to QML.

## Legacy theme files

The four files in `src/app/configs` are generated, not hand edited. Regenerate
them with:

```
python3 buildscripts/tools/generate_m3_theme_cfg.py
python3 buildscripts/tools/generate_m3_theme_cfg.py --check
```

The generator maps the muse `ThemeStyleKey` colours onto Material 3 roles:

| muse key                        | Material 3 role            |
| ------------------------------- | -------------------------- |
| `background_primary_color`      | `surface`                  |
| `background_secondary_color`    | `surface_container`        |
| `background_tertiary_color`     | `surface_container_high`   |
| `background_quarternary_color`  | `inverse_surface`          |
| `popup_background_color`        | `surface_container_high`   |
| `project_tab_color`             | `secondary_container`      |
| `text_field_color`              | `surface_container_highest`|
| `accent_color`                  | `primary`                  |
| `stroke_color`                  | `outline_variant`          |
| `stroke_secondary_color`        | `outline`                  |
| `button_color`                  | `secondary_container`      |
| `font_primary_color`            | `on_surface`               |
| `font_secondary_color`          | `on_secondary_container`   |
| `link_color`                    | `primary`                  |
| `focus_color`                   | `primary`                  |
| `error_text_color`              | `error`                    |

It also writes an `m3_<role>` key for every Material 3 role into the same files.
Those land in `ui.theme.extra` so that QML outside this module, including the
`Muse.Ui` `M3Roles` singleton, can read the same tokens without depending on
`Audacity.M3`.

## Data colour exemption list

Some colours carry meaning rather than brand. They are not generated from the
seed and they must not be replaced by a role. The generator copies them through
unchanged in the light and dark themes and only raises their contrast in the two
high contrast themes:

- Clip colours: `classic_clip_header_color`, `classic_clip_header_hover_color`,
  `classic_clip_background_color`.
- Selection and playback: `focus_state_color`, `selection_highlight_color`,
  `play_region_active_color`, `play_region_inactive_color`,
  `accessibility_clip_select_button_color`.
- Rulers: `waveform_ruler_tick_extension_color`, `waveform_ruler_label_color`,
  `waveform_ruler_small_step_color`, `spectrogram_ruler_guide_color`,
  `timeline_ruler_stroke_color`.
- Guidelines: `guideline_snap_enabled_color`, `guideline_snap_disabled_color`,
  `guideline_split_color`.
- Track headers: `track_header_color`, `track_header_hover_color`,
  `track_header_active_color`, `track_header_separator_color`.
- Meters: `meter_background_color`, `meter_stroke_color`,
  `meter_rms_overlay_color`, `meter_gradient_green_color`,
  `meter_gradient_yellow_color`, `meter_max_peak_marker_color`,
  `meter_clipped_color`.
- Dynamics and equaliser: every `dynamics_*`, `compression_curve_*` and
  `graphic_eq_*` key.
- Branding: `logo_main_color`.

The transport colours `play_color` and `record_color` are a middle case. They
keep their functional hue but take a scheme aware tone so that they stay legible
on light, dark and high contrast backgrounds.

## Type scale

`M3.typography` carries the fifteen Material 3 roles. Each role is a ready to
use `font`, with `<role>LineHeight` and `<role>LetterSpacing` next to it.

| Role            | Size | Line height | Tracking | Weight |
| --------------- | ---- | ----------- | -------- | ------ |
| displayLarge    | 57   | 64          | -0.25    | 400    |
| displayMedium   | 45   | 52          | 0        | 400    |
| displaySmall    | 36   | 44          | 0        | 400    |
| headlineLarge   | 32   | 40          | 0        | 400    |
| headlineMedium  | 28   | 36          | 0        | 400    |
| headlineSmall   | 24   | 32          | 0        | 400    |
| titleLarge      | 22   | 28          | 0        | 400    |
| titleMedium     | 16   | 24          | 0.15     | 500    |
| titleSmall      | 14   | 20          | 0.1      | 500    |
| bodyLarge       | 16   | 24          | 0.5      | 400    |
| bodyMedium      | 14   | 20          | 0.25     | 400    |
| bodySmall       | 12   | 16          | 0.4      | 400    |
| labelLarge      | 14   | 20          | 0.1      | 500    |
| labelMedium     | 12   | 16          | 0.5      | 500    |
| labelSmall      | 11   | 16          | 0.5      | 500    |

The family follows `IUiConfiguration::fontFamily()`, so the preference the user
already has keeps working. The shipped default is Roboto Flex, with Noto Sans HK
as the Chinese, Japanese and Korean fallback. See `FONTS.md`.

## Shape

`M3.shape` carries `none` 0, `extraSmall` 4, `small` 8, `medium` 12, `large` 16,
`extraLarge` 28 and `full`. `full` is a large number that a component clamps to
half of its own height, which is how the fully rounded shapes are drawn.

## Elevation

`M3.elevation` carries `level0` to `level5` at 0, 1, 3, 6, 8 and 12 device
independent pixels, plus `dp(level)`, `keyBlur(level)`, `keyOffset(level)`,
`ambientBlur(level)` and `tintOpacity(level)`.

Two things happen at a raised elevation. `M3Elevation` draws the key and ambient
shadows using layered rectangles, which works on every graphics backend Audacity
ships on. `M3.surfaceAt(level)` returns the surface colour tinted with
`surfaceTint` at the opacity for that level, which is the tonal half of Material
elevation. `M3Surface` does both.

## State layers

`M3.stateLayer` carries the Material opacities: `hover` 0.08, `focus` 0.10,
`pressed` 0.10, `dragged` 0.16, `disabledContent` 0.38 and `disabledContainer`
0.12.

Every interactive component draws an `M3StateLayer` in its content colour over
its container, above the background and below the content. Disabled components
do not draw a state layer at all. They fade their content to
`disabledContent` and their container to `disabledContainer`.

## Focus

The focus indicator is a three pixel ring in the primary colour drawn two pixels
outside the component's own shape, exactly as `M3FocusRing` implements it.
`M3.focusIndicatorThickness` and `M3.focusIndicatorOffset` carry those numbers.

The ring is bound to the muse navigation system, not to Qt's `activeFocus`. It
appears when `navigation.highlight` is true, which is what the keyboard
navigation system sets, so the ring shows for keyboard users and stays out of
the way of pointer users.

Every interactive component exposes `navigation` as an alias to its own
`NavigationControl`, carries an accessible role and an accessible name, and
works inside a `NavigationPanel`.

## Motion

`M3.motion` carries every Material 3 duration token:

- `short1` 50, `short2` 100, `short3` 150, `short4` 200
- `medium1` 250, `medium2` 300, `medium3` 350, `medium4` 400
- `long1` 450, `long2` 500, `long3` 550, `long4` 600
- `extraLong1` 700, `extraLong2` 800, `extraLong3` 900, `extraLong4` 1000

and six easing curves, each as a ready to assign `QEasingCurve` and as a bezier
control point list:

| Curve                  | Control points               |
| ---------------------- | ---------------------------- |
| `standard`             | 0.2, 0.0, 0.0, 1.0           |
| `standardAccelerate`   | 0.3, 0.0, 1.0, 1.0           |
| `standardDecelerate`   | 0.0, 0.0, 0.0, 1.0           |
| `emphasized`           | 0.2, 0.0, 0.0, 1.0           |
| `emphasizedAccelerate` | 0.3, 0.0, 0.8, 0.15          |
| `emphasizedDecelerate` | 0.05, 0.7, 0.1, 1.0          |

### Reduced motion

`M3.motion.reducedMotion` is true when any of these hold:

1. The `ui/m3/reducedMotion` setting is on.
2. The environment variable `QT_M3_REDUCED_MOTION` is `1`.
3. `QStyleHints` reports a `reducedMotion` property that is true, read
   reflectively so the build keeps working on Qt versions without it.
4. On Linux, `GTK_ENABLE_ANIMATIONS` is `0`, or `gtk-enable-animations` is
   false in `gtk-4.0/settings.ini` or `gtk-3.0/settings.ini`.

When it is true every duration token reports `0` and `M3.motion.travel(distance)`
returns `0`. That is the whole reduced motion path: an ordinary `Behavior` with
a token duration becomes an instant change, and a transition that would slide
becomes a plain cross fade, without any component needing a second code path.

Three components go slightly further because a zero duration is not enough on
its own:

- `M3Ripple` fades in place instead of expanding.
- `M3LinearProgress` and `M3CircularProgress` pulse gently instead of sweeping
  or spinning when indeterminate, and the wavy variant settles to a straight
  line.
- `M3ColorPicker` settles the animated rainbow on a single hue.

## Density

`M3.density.level` runs from `-3` to `0`. Level `0` is the comfortable Material
default and each step removes four device independent pixels from a control's
height. `M3.density.apply(baseHeight)` applies the current level and never
returns less than 24, which keeps the minimum accessible target size.

Components that have a Material default height, such as buttons, list items,
text fields, tabs and chips, take their height from `M3.density.apply(...)`
rather than a constant.

## Writing a component

A new component in this library must have all of the following:

1. A documented header comment naming the component, the muse component it
   replaces and its public API.
2. Material 3 anatomy: a container, a state layer, content, in that order.
3. A ripple started from the pointer position, and `pulse()` on keyboard
   activation.
4. A three pixel focus ring bound to `navigation.highlight`.
5. `property alias navigation` on its own `NavigationControl`, with an
   accessible role, an accessible name and `accessible.visualItem`.
6. Tooltips through the muse tooltip provider, which the internal
   `M3ToolTipHandler` wraps.
7. Every duration and easing from `M3.motion`, every colour from `M3.color`,
   every radius from `M3.shape`.
8. A public API close to the muse component it replaces, so that other work can
   swap the call sites mechanically.
9. A clean `qmllint` run.
