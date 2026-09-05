# Funny levels

Two independent sliders, both from 1 to 5 and both at 5 by default:

| Setting | Meaning |
| --- | --- |
| `experience/funny/english` | Tone of English message bodies. |
| `experience/funny/cantonese` | Tone of Cantonese message bodies. |

The settings page states the contract next to the sliders:

> These sliders change tone only; facts stay unchanged.

## What the levels touch

Only the tone of message bodies: dialog bodies, notification bodies and the
help text of tooltips. Never:

- numbers, times, durations, sample rates or file sizes;
- keyboard shortcuts;
- file names and paths;
- control labels, button text, field labels or accessible names.

## How it works

`au::experience::MessageStyler` is the service. Any code that shows a message
calls

```cpp
styler()->style(MessageKind::Warning, plainText);
```

The styler never rewrites the text it is given. It only puts a phrase in front
of it, taken from a bounded table:

- three tone groups: neutral (info, dialog, tooltip), positive (success) and
  careful (warning and error), so a problem never reads as a joke;
- one table per group per language, with two phrases for each of levels 2 to 5;
- **level 1 adds nothing at all**.

The phrase is chosen by an FNV-1a hash of the plain text, so the same message
always reads the same way. `qHash` is deliberately not used, because Qt does
not promise it is stable between runs.

In bilingual mode the English phrase goes in front and the Cantonese phrase
goes behind, because the text in the middle already carries both languages.

The guarantee that the facts survive is asserted directly:
`src/experience/tests/messagestyler_tests.cpp` checks that a factual string
containing a count, a timecode and a keyboard shortcut appears verbatim in the
result at every level, for every message kind and in every language mode.

## Where the tone is applied

- Notification toasts, through `NotificationCenter::push()`, which routes the
  body through the styler before the toast is built.
- The preview on the Companion preferences page.

Message boxes shown by the muse interactive provider are plain today. Routing
those through the styler needs a change in the shared muse overlay, which
belongs to the overlay contract in `docs/design/MUSE_OVERLAY.md`.
