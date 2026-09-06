# Narrator

## Behaviour

The narrator speaks a short line for selected application events (for example, a
completed export, an error). It is off by default; a user must explicitly turn it on.

The narrated language is a user choice: English, Cantonese, or Both. In Both mode, a
narrated event queues its English line followed by its Cantonese line, strictly serialized,
so the two never overlap and always speak in that order.

Voice pickers for each language are populated at runtime from whichever speech engine is
active, and default to "Choose automatically". A chosen voice is persisted by its stable
engine-reported identifier, never by its display name (two installed voices can share a
display name). Rate and pitch are user-adjustable.

## Speech engine

The narrator uses Qt's QtTextToSpeech module when the Qt installation carries it. On a
build where it is absent (as on the toolchain this module was developed against, which has
no `libQt6TextToSpeech`), it falls back to whichever command line speech backend is found
on the machine: `speech-dispatcher`'s `spd-say`, or `espeak-ng`. When neither is available,
the narrator reports honestly that no speech engine was found and stays silent; it never
pretends to speak.

The active engine, and which one, is shown in the preferences section
(`NarratorEngine::engineDescription()`), so a user can tell at a glance whether narration
will actually be audible.

## Queue behaviour

A `NarratorQueue` orders every narrated line:

- at most one utterance is ever in flight;
- a debounce window (400 ms by default) suppresses an identical line arriving twice in
  quick succession;
- a per category cooldown (4 seconds by default) suppresses further non-error narration in
  the same category until it elapses;
- error narration is never suppressed by either the debounce window or the cooldown;
- queuing a new utterance carrying the same "supersede key" as one still waiting replaces
  it in place, so a rapidly updating progress line never stacks duplicate announcements.

## Interaction with other features

The narrator ducks under an active screen reader when one can be detected, and honours the
system's reduced sound setting. It obeys School mode's suppression of the affected
capabilities exactly like every other Experience feature that School mode touches.

## Verification

Covered by `NarratorQueueTests` in `src/experience/tests/narratorqueue_tests.cpp`: ordering,
debounce, per category cooldown with error narration exempt, supersession of a pending item
by key, and the one utterance at a time guarantee.
