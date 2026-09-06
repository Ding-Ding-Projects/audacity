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

The narrator honours two gates before it actually speaks anything, both checked inside
`NarratorEngine::speak()` so no call site can forget them:

- **Quiet mode** is the narrator's own reduced-sound setting, a "Quiet mode" toggle in its
  preferences section, backed by `IExperienceConfiguration::quietModeEnabled()`. While it is
  on, the narrator stays completely silent even when otherwise enabled. This is a real
  persisted setting, not a placeholder: it round-trips through muse settings exactly like
  every other narrator preference.
- **Screen reader ducking** uses `QAccessible::isActive()` as the detectable signal, exposed
  as `NarratorEngine::screenReaderActive()`. Qt sets this to true the moment any assistive
  technology (a screen reader) queries the application, and it has no separate on/off
  setting: the narrator simply stays quiet for as long as it reports true, on the reasoning
  that a screen reader already reading the interface should not be talked over.

Both gates apply to every narrated category, error narration included: they are about
whether any sound happens at all, not about how often it happens, so they are not the same
kind of limit as the debounce and cooldown windows below (which do exempt error narration).

It obeys School mode's suppression of the affected capabilities exactly like every other
Experience feature that School mode touches.

## Verification

Covered by `NarratorQueueTests` in `src/experience/tests/narratorqueue_tests.cpp`: ordering,
debounce, per category cooldown with error narration exempt, supersession of a pending item
by key, and the one utterance at a time guarantee. The quiet mode setting round-trips
through the same configuration test pattern as the rest of the narrator settings; the queue
tests do not exercise `QAccessible::isActive()` directly since that is a live platform signal
rather than pure logic, and is instead verified by reading the built application (the
Narrator preferences section shows the Quiet mode toggle and its explanatory text).

The notification centre is a real narrator event consumer. When narration is
enabled, each application notification enters the serialized queue before the
selected backend starts. The next notification waits for the active backend
process to exit, or for an utterance to be truthfully suppressed by Quiet mode,
a screen reader, or no available engine.
