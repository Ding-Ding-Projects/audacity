# Narrator

## Documentation website

Settings provides an off-by-default narrator for interface notifications. Enable
it explicitly, then choose English, Cantonese, or Both. Both speaks English before
Cantonese, with one utterance active at a time. Preview uses a fixed sample;
text fields and uploaded file contents are not automatically read aloud.

Each language has its own voice picker. Choices come from the browser speech
service and refresh when its voice list changes. The default is **Choose
automatically**. Selection persists by `voiceURI`, not the displayed voice name.
A missing selected voice stays saved while an available voice provides fallback.
Status explains the effective voice, missing selection, network requirement, or
unavailable language. Mandarin is not silently substituted for Cantonese.

Rate ranges from 0.1 to 10 and pitch from 0 to 2, both defaulting to 1. These are
[Web Speech API](https://webaudio.github.io/web-speech-api/#speechsynthesis)
parameter ranges; actual sound depends on the installed engine. This project does
not claim that every browser supplies a natural Cantonese voice.

Preferences live under `ma.settings.v1.narrator`. They participate in settings
search and local settings history. Narrator palette entries reveal and focus the
corresponding row. If storage fails, settings remain active for this page and a
persistent status states that they were not saved.

### Quiet operation and recovery

Ordinary events debounce for 250 milliseconds, supersede queued events in the same
category, and have a five-second category cooldown. Important failure events bypass
those timing limits and move ahead of queued ordinary events; active speech is
allowed to finish. The queue holds at most 32 events. Each language's message is
bounded to 2,000 characters. A full important-event queue reports `queue-full`;
the original visible notification remains available.

Changing narrator preferences stops old queued speech; unrelated appearance
changes do not. Quiet narration and Yield to assistive technology silence speech
without erasing choices. Browsers do not expose a dependable screen-reader-active
signal, so explicit user-controlled silence is the supported equivalent. The
website does not claim automatic screen-reader ducking.

Network-backed voices are identified through the browser's `localService` field
and stay silent when it reports offline. An online indication cannot prove service
reachability; actual speech failures remain visible in status. Missing Cantonese
copy falls back to English with a disclosed `spoken-english-fallback` result.
Missing synthesis/voices, platform refusal, and a 30-second speech deadline never
count as successful speech.

Navigation teardown cancels owned speech and removes voice/connectivity listeners.
Returning from the browser page cache reattaches listeners without replaying old
messages. Voice names and dynamic facts remain literal data.

### Verification boundary

`node --test docs/site/scripts/narrator.test.cjs` exercises the actual queue and
voice-selection module through a controlled speech adapter. Its 17 cases cover
absent/late voices, stable selection, missing-selection fallback, Cantonese choice,
serialization, supersession, priority and cooldown exceptions, network-only offline
voices, quiet/yield cancellation, parameter bounds, restored settings, deadlines,
page-cache lifecycle, untranslated-event fallback, and unrelated settings changes.
The priority, missing-translation and unchanged-settings counterexamples first
failed against the old implementation, then passed after their respective repairs.

These tests do not prove audible voice quality, actual browser enumeration,
screen-reader announcements, layout, keyboard interaction, or the full built-page
matrix. Whole-site event coverage, every failure category's localized tone, and
scheduled quiet-hours integration remain incomplete.

## Desktop application

Desktop narration is a separate Experience service and queue under
`src/experience/internal/`. Its engine, voice enumeration, settings and built
behavior need independent candidate-bound verification. Website adapter tests do
not establish desktop narration, Qt speech engine availability, or installation
of a platform voice. Consult the current feature inventory and handoff rather
than treating a declared engine enum as implemented speech.