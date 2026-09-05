# The command palette

The command palette is one search box that reaches everything the application
can do or be configured to do. It opens on **Ctrl+Shift+F** from anywhere in
the window, and it is the only global shortcut the companion module claims.

There is no Ctrl+K binding. Ctrl+K belongs to the editing surface, and a
palette that stole it would take a key people already use.

## What it indexes

Every time the palette opens it rebuilds its index, so what it shows is what
the application is at that moment. Six sources feed it.

| Source | What a row carries |
| --- | --- |
| The muse UI actions register | The action's title, its description, its current keyboard shortcut, whether it is enabled right now, and the section its action code belongs to |
| The preferences pages | One row per page, from the same list the preferences dialog builds |
| The preferences settings | One row per control, from the hand written index in `src/companion/palette/settingsindex.json` |
| The appearance controls | Seed colour, scheme variant, density, reduced motion, theme, follow system theme, interface font, body text size, clip style, language, language mode, the English and Cantonese funny levels, low stimulation, and the palette's own size mode |
| The window | Open project tabs and dock panels, pushed in from QML because only the running window knows them |
| The documentation | Every Markdown article under `docs`, titled by its first heading |

An action row shows its shortcut on the right. A disabled action is shown, in
the disabled colour, and cannot be activated: hiding it would leave the user
wondering whether the command exists at all.

## Rich rows

A row that has a muse setting key behind it renders a working control inline:

- a **switch** row draws an `M3Switch` bound to the setting;
- a **slider** row draws an `M3Slider` with the setting's range and step;
- a **dropdown** row draws an `M3Dropdown` over the setting's options;
- a **colour** row draws a swatch that opens `M3ColorPicker`.

Changing one of these writes through `muse::settings()->setSharedValue`, which
is the same call the preferences page behind it makes. There is no second code
path and no separate copy of the value, so a change made in the palette is
visible in preferences the moment the dialog is opened, and the reverse.

Rows without a setting key behind them, which is most of the preferences whose
value lives in a page model rather than in a setting, do not draw a control.
They teleport instead.

## Teleport

Selecting a row that is not an action opens the surface that owns it:

1. the preferences dialog opens at the right page, through
   `audacity://preferences?currentPageId=<page>&highlight=<label>`;
2. `TeleportHighlighter` inside the dialog walks the loaded page looking for an
   item whose visible text contains that label;
3. it scrolls the enclosing `Flickable` so the control is 40 pixels below the
   top edge;
4. it calls `forceActiveFocus` on the control;
5. it draws a three pixel primary ring with a twelve per cent primary wash over
   the control and pulses it for 1.2 seconds.

Under reduced motion the pulse does not animate. The ring still appears and
still disappears after the same 1.2 seconds, so the affordance survives; it
simply does not move. That falls out of the token engine, where every duration
reports zero under reduced motion, plus a single loop count.

Nothing in the preferences pages has to be prepared for a teleport. The search
matches the text that is already on screen, which is the same text the index
recorded.

## Searching

The palette's own search box is a plain text box by default. Every whitespace
separated word must appear somewhere in a row, in any order, so `pref dens`
finds the density slider.

The **Regex** chip beside the box turns the query into a regular expression,
case insensitive. An invalid expression is reported under the box and the list
is left empty rather than silently falling back, so a half typed pattern never
looks like "no results". The builder button on the search bar itself opens the
[regular expression builder](regex-builder.md) anchored to the palette, whose
accepted pattern is written back into the box with the Regex chip turned on.

## Size

The palette is a card in the upper third of the window, at most 720 by 560
device independent pixels. The button beside the search box switches it to a
full window surface. The choice is persisted under
`companion/palette/fullWindow` and restored the next time the palette opens.

## Keyboard

| Key | Effect |
| --- | --- |
| Ctrl+Shift+F | Open the palette, or close it if it is already open |
| Up, Down | Move the selection |
| Page Up, Page Down | Move the selection ten rows |
| Enter | Activate the selected row |
| Tab | Move into the inline controls in the rows |
| Escape | Close |

The arrow, Enter and Escape keys are taken before the search field sees them,
so one field both types and navigates. Tab is deliberately left alone so that
it reaches the switches, sliders and dropdowns in the rows.

## Accessibility

- Every control in the palette is an `M3` component, so each one carries a
  `NavigationControl` with an accessible role and name, a three pixel focus
  ring bound to `navigation.highlight`, and a state layer.
- The result list reports its length in a line under it, which a screen reader
  announces when the count changes.
- A disabled row is announced as disabled rather than omitted.
- The teleport highlight is a visual aid only. The teleport also moves keyboard
  focus to the control, which is what a screen reader follows.
- The palette obeys the density setting, so its rows shrink with the rest of
  the interface rather than staying at one height.

## Failure modes

| What can go wrong | What happens |
| --- | --- |
| The settings index resource is missing | The palette logs a warning and shows actions and documentation only. It does not crash and it does not show empty rows. |
| The documentation directory cannot be found | No documentation rows. `documentationRoot()` returns an empty string, which is what the developer tools show. |
| A teleport target no longer exists in a page | The dialog still opens at the right page. `TeleportHighlighter` raises `cleared` and draws nothing rather than highlighting the wrong control. |
| A setting key in the index no longer exists | The inline control shows the setting's default. The guard test below fails first, in the build, so this should never reach a user. |
| The regex in the search box is invalid | The error is shown under the box and the list is empty. |

## Verification

Automated, in `src/companion/tests/palette_index_tests.cpp`:

- the index parses, and also ships inside the compiled module resources;
- every page the preferences model creates has a row, ignoring the pages that
  are commented out;
- every settings row names a page that exists, has a label, has a teleport
  target and has a known control type;
- every setting key the index names is still declared somewhere in `src` or in
  the muse framework;
- every `*Section` component a preferences page instantiates is represented;
- the appearance rows carry the documented Material settings.

By hand:

1. Press Ctrl+Shift+F. The palette opens with the search box focused.
2. Type `motion`. The "Reduce motion" row appears with a working switch.
   Toggle it and watch the interface animation stop.
3. Type `density`. The row shows a live slider from -3 to 0.
4. Type `seed`. Select the row. Preferences opens on Appearance with the seed
   colour picker outlined and pulsing.
5. Press the size button. Close and reopen the palette; it is still full
   window.
