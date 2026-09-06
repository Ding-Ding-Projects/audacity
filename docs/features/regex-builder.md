# The regular expression builder

Every search field in the application has a builder button beside it, and every
one of them opens its own builder. The builder is a workbench: it constructs a
pattern, explains it, runs it against a sample, times it, and warns when it is
the kind of pattern that can be made to run forever.

Plain text is what a search field does by default. Regular expression matching
is an explicit choice, made by a visible toggle, never inferred from what was
typed.

## Where it appears

`docs/inventory/search-inventory.md` lists every search field and the builder
that serves it, and `src/companion/tests/search_inventory_tests.cpp` fails the
build when the two disagree. In summary: the command palette, the preferences
settings search, the advanced preferences search, the history panel, the
version history panel, the plugin manager, the home plugins page, the projects
page and the new project template list each anchor their own builder. The
popups and dialogs, which cannot host an anchored sheet without being clipped
by their own window, raise `regexBuilderRequested` for the surface that opened
them.

## Every menu and every dropdown

`M3Menu` and the flat list popup used by `M3Dropdown` carry the filter field
and the builder by default, not as something a call site opts into. A menu of
two items gets the same filter field as a menu of two hundred, because a short
menu today is not a promise it stays short. Plain text matches the visible
title of each item; the match never reorders items, never changes what an
item does, and never touches a separator.

Each of the two carries its own `RegexBuilderSheet`, so neither one needs an
outside surface to answer a request for the builder: opening the builder from
inside a menu that is itself a popup does not need a second popup floating
somewhere else to catch the request. `M3Menu` still raises
`regexBuilderRequested` afterwards for a host that keeps one shared builder
across a whole page, such as a settings dialog with several fields, but
nothing in the application currently listens for it, and the search inventory
records that plainly rather than pretending a listener exists.

Escape clears the filter first and closes the menu on a second press. The
result count is announced to a screen reader on every change, whether or not
it is shown visually, and an honest "No matching items" message replaces the
item list rather than leaving it blank when nothing survives the filter.
Every widget built from these two, including the browser style tab strip's
right click menu, the workspace and snap toolbar dropdowns and every
`DropdownWithTitle`, gets the filter field and the builder for free because
they are built from `M3Menu` or `M3Dropdown` underneath.

The muse framework's own native menu (used for the application menu bar and
for context menus that have not yet moved to `M3Menu`) already carries a
plain text filter field through its `isSearchable` property, but nothing in
this application currently turns it on, and it has no builder button of its
own. Wiring that native menu is deliberately out of this pass: the muse
submodule working tree is shared with other work in flight at the same time,
and editing a submodule that several changes are touching at once risks
corrupting somebody else's patch. It is recorded here as the next step for the
remaining native menus, rather than left as a silent gap.

## Layout

The builder is a non-modal `M3SideSheet` on the trailing edge of the surface
that owns the field, at most 480 pixels wide or 45 per cent of the surface,
whichever is smaller. Non-modal is the point: the field it belongs to stays
usable while the builder is open, so a pattern can be tried against the real
list rather than only against a sample.

Below 900 device independent pixels a side sheet would leave nothing beside it,
so the builder takes the whole surface instead, with its own headline and close
button.

## What it holds

**Pattern and flags.** The raw pattern is a text field and is always the truth.
The guided chips write into it, and typing into it re-derives everything else.
The five flags are `M3Chip` filter chips: ignore case, multiline, dot matches
newline, extended and Unicode properties. They map onto
`QRegularExpression::CaseInsensitiveOption`, `MultilineOption`,
`DotMatchesEverythingOption`, `ExtendedPatternSyntaxOption` and
`UseUnicodePropertiesOption`.

**Guided construction.** Around forty chips grouped as character classes,
anchors, groups, quantifiers, alternation, lookaround, references and
modifiers. Each one names what it inserts and what it means. Two buttons wrap
the whole pattern in a capture group or an atomic group; the engine also
supports named, non-capturing, atomic, lookahead, negative lookahead,
lookbehind and negative lookbehind wrapping through `wrapSelection`.

**Explanation.** The pattern is tokenised and each token gets one line in
plain English, indented by its nesting depth. A quantifier is folded into the
line for the thing it repeats, and lazy and possessive forms are named as such:
"repeated one or more times, as few times as possible".

**Parse tree.** The same tokens as a nested tree, with each group's quantifier
beside it. The tree also reports whether the brackets balance, which is often a
clearer answer than the PCRE2 error message.

**Matches and captures.** The pattern runs against the sample and every match
is listed with its offset and its text, and under it every capture group with
its number, its name if it has one, its text, and whether it participated at
all. A group that did not participate is said so rather than shown as empty.

**Replacement.** A replacement template with a live preview of the whole
sample after replacement. `\1` and named references work as PCRE2 defines them.

**Timing.** Every run is timed with `QElapsedTimer` and the result is reported
in milliseconds to three decimal places next to the match count.

**Saved test cases.** A named test case stores the pattern, the sample, the
replacement, all five flags and the match count the pattern produced. Cases are
persisted as JSON under
`<app data>/companion/regex/<store>.json`, where `<store>` is the field's own
store name, so no two fields share saved cases. The whole workbench can be
exported to JSON and imported back, which is how a pattern moves between
machines.

**Dialect.** The builder names its dialect as
`PCRE2 via QRegularExpression <Qt version>` and lists an eighteen row
capability matrix: what this dialect supports, what it does not, and the note
that matters. Variable length lookbehind, for instance, is listed as
unsupported with `\K` named as the way round it.

## Bounded input and backtracking risk

Two things keep a pattern from freezing the interface.

**Bounded input.** The sample is truncated to 20 000 characters and the match
list stops at 500 matches. Either one sets a `truncated` flag, which the
builder shows under the sample field. Nothing is hidden: the user is told the
run was shortened.

**Risk heuristics.** The token tree is walked for the shapes that cause
catastrophic backtracking:

| Shape | Severity | Example |
| --- | --- | --- |
| A group with an unbounded quantifier whose body itself contains a quantifier | high | `(a+)+` |
| A repeated alternation whose branches can start with the same text | high | `(a\|a)*` |
| Two unbounded wildcards next to each other | moderate | `.*.*` |
| Two repeated character classes next to each other over the same characters | moderate | `[a-z]*[a-z]*` |
| Four or more unbounded quantifiers with none of the above | low | |

Each finding names the fragment, explains why it is slow and says what to do
instead: bound the quantifier, make it possessive, or wrap it in an atomic
group. At moderate and above the builder adds an adversarial input warning:
this pattern can be made to take an unbounded amount of time by input chosen to
defeat it, so do not run it over text someone else controls.

The heuristics are a warning, not a verdict. They do not claim to find every
slow pattern, and a pattern they pass can still be slow on a large enough
input. That is what the timing readout is for.

## Isolation

Each search field's builder is created fresh every time it opens, so its
engine, its pattern, its flags and its sample never carry over from another
field or from a previous session. Only the saved test cases persist, and those
are keyed by the field's store name.

## Accessibility

- Every control is an `M3` component, with a `NavigationControl`, an
  accessible name, a focus ring bound to `navigation.highlight` and a state
  layer.
- The flags are toggle chips rather than an opaque flag string, so a screen
  reader announces "Ignore case, checked" rather than reading `(?i)`.
- The explanation is text, not colour: a pattern is understandable with the
  colours turned off.
- An invalid pattern is reported as supporting text on the pattern field, which
  is announced with the field.
- The sheet is non-modal, so keyboard focus is never trapped inside it. Escape
  closes it.
- Under reduced motion the sheet's slide becomes an instant change, because
  every duration token reports zero.

## Failure modes

| What can go wrong | What happens |
| --- | --- |
| The pattern does not compile | `valid` is false, the message and the offset are shown on the field, and every derived view is empty. Nothing throws. |
| The sample is enormous | It is truncated to 20 000 characters and the builder says so. |
| The pattern matches more than 500 times | The list stops at 500 and the builder says so. |
| Imported JSON is not a workbench document | `importJson` returns false, the field shows an error and nothing is changed. |
| The saved test case file cannot be written | The save is dropped silently at the file layer; the in-memory list still shows the case for the session. |
| A saved test case file is corrupt | It is treated as empty. Existing cases in it are lost; nothing crashes. |

## Verification

Automated, in `src/companion/tests/regexengine_tests.cpp`, twenty two tests
covering: the empty and invalid pattern paths, matches and numbered and named
captures, each flag changing the result, the explanation naming tokens and lazy
and possessive quantifiers, the parse tree nesting and its balance report, the
replacement preview, the nested quantifier and overlapping alternation risks
and the absence of a false positive on a plain pattern, sample truncation, the
timing readout, guided insertion and wrapping, literal escaping, the dialect
string and capability matrix, the JSON round trip and its rejection of
rubbish, and the completeness of the token catalogue.

By hand:

1. Open preferences and press the builder button beside the settings search.
   The sheet slides in from the trailing edge and the dialog stays usable.
2. Type `(?<name>\d+)`. The explanation names the group and the digits, the
   parse tree nests them, and the match list shows the capture under its name.
3. Type `(a+)+`. The risk section turns red and names the nested quantifier.
4. Save the pattern as a test case, close the sheet, reopen it, and load the
   case back.
5. Narrow the window below 900 pixels and open the builder again. It fills the
   surface instead of docking.
