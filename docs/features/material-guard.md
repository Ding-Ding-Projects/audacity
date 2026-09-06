# Material Design 3 audit guard

## Behaviour

`buildscripts/checks/material_guard.py` scans every `src/**/qml/**/*.qml` file for legacy
Muse control instantiations, hard coded colour and radius literals, and `ui.theme.` colour
reads outside the `Audacity.M3` component library, then compares each hit against
`docs/inventory/material-audit.md`. A hit that has a matching row whose status starts with
`kept` or `converted` is counted as addressed; every other hit is printed as an unaddressed
finding.

The goal it checks for is stated in `docs/inventory/material-audit.md`'s own introduction:
every rendered element should be a Material Design 3 primitive from `Audacity.M3`, or a Muse
control restyled through one of the numbered overlay patches under
`buildscripts/muse-patches/` so that it renders with Material 3 anatomy. The guard does not
decide which of those two is correct for a given file; the inventory row does, and a human
(or an agent auditing a lane) is expected to have looked at the file before writing that row.

Files under `src/uicomponents/qml/Audacity/M3/` are never scanned. That directory defines the
Material 3 primitives every other surface converts to, so its own internal use of a raw
colour literal, a numeric radius, or a base Muse control it restyles is not a finding, it is
the implementation.

## What the guard looks for

- **Legacy control instantiations**: `FlatButton`, `FlatToggleButton`, `RoundedRadioButton`,
  `RadioButton`, `CheckBox`, `StyledSlider`, `StyledDropdown`, `Dropdown`, `TextInputField`,
  `SearchField`, `StyledTabBar`, `StyledTabButton`, `StyledMenu`, `StyledMenuItem`,
  `StyledPopupView`, `StyledDialogView`, `StyledToolTip`, `ToolTip`, `ListItemBlank`,
  `StyledListView`, `StyledTableView`, `MenuButton`, `ValueList`, `FilePicker`, `ColorPicker`,
  `KnobControl`, `IncrementalPropertyControl`, `SpinBox`, `ProgressBar`,
  `StyledBusyIndicator`, `BusyIndicator`, `Switch`. Matched only when the identifier is
  immediately followed by `{` (an instantiation), so a component whose *name* merely
  contains one of these words as a substring (for example
  `IncrementalPropertyControlWithTitle`) is not a false positive.
- **Colour literals**: a `color:` property assigned a quoted `"#..."` hex value.
- **Radius literals**: a `radius:` property assigned a numeric literal.
- **Theme reads**: `ui.theme.` property access, outside the M3 library and the theme
  provider itself.

Functional data colours, waveform, clip, meter and spectrogram colours, are data, not
chrome, and are excluded the same way as everything else here: not by guessing at variable
names, but by an inventory row that says so with a reason, same as any other kept usage.

## Reading the inventory

`docs/inventory/material-audit.md` is a hand written table, one row per file (or per file and
disposition group, when a file mixes an addressed usage with an unaddressed one). Reasons
allowed for a `kept` status are the three the audit agreed on:

1. A dev-tools gallery comparison surface.
2. A Muse control already restyled by one of the overlay patches under
   `buildscripts/muse-patches/` (name the exact patch).
3. A control that needs behaviour `Audacity.M3` does not yet have (name the missing
   behaviour; file an `Audacity.M3` addition when one is feasible instead of leaving the row
   open ended).

A `pending` status means the usage was found and not yet converted or triaged; it is still
printed by the guard (so the gap is visible) but does not, by itself, fail configure unless
`AU_COMPLETENESS_STRICT` is on, in the same way an unfinished row in
`docs/inventory/completeness-inventory.md` does not.

The guard's row parser is intentionally forgiving about the exact column layout: it reads
the first cell as the file path, the last cell as the status, and searches the whole row's
text for the identifier (or, for `ui.theme`/colour/radius findings, for the matching
keyword). This lets every lane's section of the table use its own column headings and
still be read by the one guard, rather than requiring every contributor to agree on an exact
schema before the guard can see their rows.

## Configuration

Wired into `buildscripts/cmake/CompletenessInventory.cmake` alongside the existing
completeness inventory guard, and controlled by the same options:

- `AU_CHECK_MATERIAL_AUDIT` (default `ON`) turns the check on or off entirely.
- `AU_COMPLETENESS_STRICT` (default `OFF`, shared with the completeness inventory guard)
  turns an unaddressed finding into a configure failure. With it off, findings are printed as
  a `WARNING` at configure time and configuration proceeds.

With no `python3` interpreter available, the check is skipped with a warning rather than
failing configure; a missing interpreter is an environment fact, not a Material Design defect.

## Failure modes

- A file converted to legacy syntax without a corresponding inventory update: the guard
  finds the new legacy instantiation and reports it, because no row covers it.
- An inventory row is written for the wrong file path, or misspells the identifier: the
  guard will not match it, and the real usage stays reported. This is why the guard reads the
  literal text rather than trusting a claim that a file was handled.
- An allow listed row is deleted (for example, by an unrelated revert): the usage it covered
  reappears as an unaddressed finding on the next run. This is deliberate; see the negative
  regression test below.

## Verification

`buildscripts/checks/test_material_guard.py` is the negative regression. It builds a small
throwaway QML fixture tree under a temporary directory (never the real repository) and
proves, using `unittest`:

- An unaddressed legacy control instantiation is reported as a finding.
- The identical usage, once allow listed with a reason and a `kept` status, is not.
- Removing that allow list row (simulating a reverted or never written entry) turns the
  identical file red again, proving the guard actually reads the allow list rather than
  exempting the fixture directory by construction.
- A component whose name merely contains a legacy identifier as a substring is not a false
  positive.
- A hard coded colour literal outside the M3 library is flagged.
- Files under the `Audacity.M3` library path are excluded entirely, including their own use
  of a raw colour literal and a legacy base identifier.

Run it directly: `python3 buildscripts/checks/test_material_guard.py -v`.

Run the guard itself against the real tree: `python3 buildscripts/checks/material_guard.py
--repo-root .` (add `--strict` to get a non-zero exit code when findings remain).

## Current state

As of this guard's introduction, running it against the full tree reports both fully
converted and explicitly kept usages as allow listed, and a large number of usages across
every lane's scope as pending: most of these are `ui.theme.` colour reads and numeric
`radius:` literals that have not yet been triaged file by file against `M3.color` and
`M3.shape` tokens, plus a handful of controls (`ValueList`, some `StyledListView` delegates)
that either have no `Audacity.M3` equivalent yet or were not individually re-verified. The
guard is deliberately non-strict by default so that this real, honest gap does not block
every build while the audit continues; turning `AU_COMPLETENESS_STRICT` on is the intended
way to make the remaining work visible as a hard failure once the audit is judged complete.

## Security considerations

The guard only reads text files under `src/` and a single markdown file; it makes no network
access, executes no QML, and writes nothing to disk. It is safe to run in any environment
that has a `python3` interpreter.
