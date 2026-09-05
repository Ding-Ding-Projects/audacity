# Tab navigation

The main page switcher is a browser style tab strip. It hosts the fixed
application pages (Home, Project, DevTools), one tab per open project and one
tab per dockable panel.

Page switching behaviour is unchanged: choosing a tab still raises the same
`selected(uri)` signal the window content already listened to.

## Docking

A strip can be docked to the **left** (the default), the **right**, the **top**
or the **bottom**. The side is stored per surface and is changed from the
strip's context menu and from Preferences.

Changing the side changes the direction the tabs run in, never the direction
the labels read in. A vertical strip is a column of horizontal labels, not a
row of rotated ones.

## Collapsing and overflow

- The strip collapses to icons when it becomes too narrow for labels, and the
  user can ask for icons only at any width from the context menu.
- The overflow button opens a popup **window**, not an item inside the strip,
  so it is never clipped however narrow the strip is.

## Ordering, pinning and groups

- Drag a tab past its own extent to move it one place. `Shift` with an arrow
  key does the same from the keyboard.
- Pinned tabs are shown first and are excluded from bulk closing unless asked
  for.
- A tab can be put in a group. A group has a name **and** a colour, so the
  grouping is never carried by colour alone. **Edit group appearance…** opens
  an `M3ColorPicker` beside a name field.

## The four searches

The overflow popup offers four searches, each with its own `M3SearchBar` and
its own `regexBuilderRequested` hook, so a term typed for one never silently
narrows another:

1. **This strip** — the tabs of the current strip.
2. **In group** — the tabs inside one chosen group.
3. **Groups** — the groups themselves, by name.
4. **All strips** — every tab of every stored strip, labelled with its strip.

A query is treated as a regular expression when it compiles as one and as plain
text otherwise.

## Closing many tabs at once

**Close tabs containing text** and **Close tabs not containing text** are exact
inverses over the closable tabs.

- Plain text is the default. A switch turns the query into a regular
  expression.
- The command shows how many of how many tabs it would close, and lists them,
  before it closes anything.
- It is disabled on an empty query and on a regular expression that does not
  compile, so it can never take every tab by accident.
- Pinned tabs are excluded unless the user turns them on.
- Tabs that are not closable, such as the fixed application pages, are never
  taken.

## Persistence

Each strip stores its order, pinning, groups, dock side and collapsed state
under `chronicle/tabs/<surfaceId>`, and registers itself in
`chronicle/tabSurfaces` so the master search can reach every strip. The state
survives a restart.

An unknown dock side falls back to `left`, and stored text that is not valid
JSON gives an empty strip rather than a crash.

## Failure modes

| What goes wrong | What happens |
| --------------- | ------------ |
| Stored state is corrupt | The strip starts empty and rebuilds itself from the tabs the application declares |
| A stored dock side is unknown | It falls back to `left` |
| The strip is too narrow for labels | It collapses to icons; the overflow popup still lists every tab with its full label |
| A bulk close query is invalid | The command is disabled and says why, and nothing is closed |
| A stored tab no longer exists | It is dropped the next time the application declares its tabs |

## Accessibility

- The strip is one muse `NavigationPanel` whose direction follows the dock
  side, which gives roving focus: the arrow keys move along the strip and only
  the active tab is a tab stop.
- The muse accessibility roles have no dedicated tab role. A tab is reported as
  a list item inside the strip's list, together with its selected state and,
  when it is pinned, the word "pinned". That is what a screen reader announces.
  The strip itself carries the panel name "Tab list" and its direction.
- Each close button carries an accessible name naming the tab it would close.
- Group membership is announced through the group name in the tab list rather
  than through the colour bar alone.

## Verification

`src/chronicle/tests/tabstrip_tests.cpp` asserts:

- the persistence round trip preserving order, pinning, groups, colours, the
  dock side and the collapsed state;
- the fallback for an unknown dock side and for stored rubbish;
- the bulk close predicate and its exact inverse, including that the two sets
  never overlap and together cover every closable tab;
- that pinned tabs are excluded unless asked for;
- that an empty query and a broken regular expression close nothing in both
  directions;
- that regular expression matching is used only when asked for, and that
  matching is case insensitive.
