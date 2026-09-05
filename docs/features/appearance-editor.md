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

## Failure modes

A property this small editor does not yet have a control for stays visible in
the source data if it was set another way, and is simply not editable from
this surface; the editor says so rather than pretending the property does
not exist. Nothing here is applied unless the element that owns it actually
reads `AppearanceOverrides`.

## Security considerations

Overrides never leave the local machine and contain no secrets. Import and
export write and read plain JSON.

## Verification

Covered by manual interaction: opening the popover from the context menu and
from Shift+right-click, editing a property, and confirming it is applied and
persists after a restart. See the capture under
`docs/design/captures/lane-m/`.
