# Per element appearance editor

## Behaviour

Almost every rendered control offers "Edit appearance..." from its right click
context menu, and Shift+right-click opens the same editor directly. The
editor is a non-modal side sheet anchored beside the element being edited, so
the element stays visible and usable while it is open.

The editor covers typography (font family from the fonts installed on the
machine plus the ones bundled with the application, size, italic, underline,
strikethrough, small caps, letter spacing), colour through the existing
infinite colour picker (including the animated rainbow choice, with a shared
speed setting from 1 to 5 and a reduced motion fallback that settles on one
hue), corner radius, and per state overrides for normal, hover, focus,
pressed, selected and disabled. A state with no override of its own falls
back to the normal one, so a hover colour can be set without repeating the
normal colour.

Overrides can be reset per property, per element, or for everything at once.

## Configuration

Overrides are stored in a single JSON file under the application's user data
directory, in a `personalize` subdirectory, as `appearance-overrides.json`.
The rainbow speed level is stored in the same file.

## Rendering the overrides

`AppearanceOverrides` is a QML singleton owned by the personalize module and
registered into both the `Audacity.Personalize` namespace (where the editor
lives) and the `Audacity.M3` namespace, as the same instance. Registering it
into the M3 namespace as well means a Material 3 component can read the
store without the shared uicomponents module having to import or link
against personalize: a component only has to check whether the global
`AppearanceOverrides` identifier resolves at all, which it does only when
the personalize module is loaded.

A component that wants to be overridable exposes a settable `elementId`
property, defaulting to an empty string (no overrides applied). When
`elementId` is set, it resolves each overridable property through
`AppearanceOverrides.resolve(elementId, state, property, fallback)`, using
the token derived default as the fallback, and reacts live to
`AppearanceOverrides.elementChanged` for that same element id.

Every component named in the appearance editor's own description now carries
`elementId` and resolves at least colour and corner radius through the
store: `M3Button` and `M3IconButton` (container colour, content colour on
`M3Button`, radius), `M3Card` (container colour, radius, elevation),
`M3TextField` (container colour, radius), `M3Switch` (container colour,
radius), `M3Checkbox` (container colour, radius), `M3Slider` (active track
colour, handle colour, handle radius), `M3Chip` (container colour, radius),
`M3ListItem` (container colour, radius), `M3TopAppBar` (container colour),
`M3Tab` (content colour), `M3Tabs` (container colour), `M3Dialog` (container
colour, radius) and `M3Menu` (container colour, radius, elevation).

Typography (family, size, weight, italic, letter spacing) and opacity
overrides are stored by the editor but not read by any component yet,
because a QML `font` grouped property cannot be partially overridden
property by property without a larger change to how each component builds
its font. Setting one of those properties in the editor still stores it
(nothing is lost); it simply is not rendered until a later change wires
per-axis font resolution through the same store.

Set `AU_APPEARANCE_DEMO=1` before launching to seed one visible override
(an orange, near-square container) on the Home page's "New" project button
(`elementId: "home.newProject"` in `ProjectsPage.qml`), useful for capturing
the rendering path without going through the editor by hand.

`PersonalizableItem` already carries the `elementId` it was given through to
the appearance editor popover it opens; wiring a wrapped M3 element's own
`elementId` to the same value as its enclosing `PersonalizableItem` is what
makes "Edit appearance..." on that element edit the overrides that element
actually reads.

## Failure modes

A property this small editor does not yet have a control for stays visible in
the source data if it was set another way, and is simply not editable from
this surface; the editor says so rather than pretending the property does
not exist. An override is only applied once the specific component reading
it has been wired up as described above; see that section for exactly which
components and properties currently render.

## Security considerations

Overrides never leave the local machine and contain no secrets. Import and
export write and read plain JSON.

## Verification

Covered by manual interaction: opening the popover from the context menu and
from Shift+right-click, editing a property, and confirming it is applied and
persists after a restart. See the capture under
`docs/design/captures/lane-m/`.
