# Narrator

## Language and voice behavior

Narration is disabled by default. The supported Windows build uses native SAPI,
with no QtTextToSpeech package or command-line speech program required. The Windows
SDK supplies `sapi.h`, `sapi.lib`, and `ole32.lib`; the existing MSVC/Windows SDK
bootstrap supplies these development dependencies. Runtime synthesis uses locally
registered SAPI voices. Modern voices not registered with SAPI are not advertised.

The engine enumerates real token IDs and language attributes on every voice query.
English locale IDs map to English. Hong Kong (`0x0c04`) and Macao (`0x1404`)
Chinese locale IDs map to Cantonese; Mandarin is never treated as Cantonese.
If the selected ID disappeared, an installed voice of the same language is selected
and the engine records the fallback. If no matching voice exists, that line stays
silent. Preferences report the installed English and Cantonese voice counts.

`pushLocalized()` accepts separate English and Cantonese narration strings. Both
queues the actual English text followed by the actual Cantonese translation under
one event-level cooldown decision. The example notification and vocabulary-loaded
notification supply these separate strings. Existing `push()` callers carry no
translation metadata and remain English-only; missing translations are never
invented or spoken with a falsely labeled voice. Narration uses the explicit source
strings, separate from the decorated display body.

## Lifecycle and bounds

SAPI speech starts asynchronously. A 20 ms timer polls completion and a 10 second
watchdog purges a stuck utterance. Startup failure, backend failure, normal finish,
timeout, and stop share one generation-bound completion path. Completion is posted
once through the event loop, after cancellation, and the engine rejects another
start until that completion is delivered. Destruction stops timers, purges speech,
releases COM references, and balances COM initialization owned by this instance.

The queue holds at most 64 pending lines and bounds each to 2048 UTF-16 code units.
The engine bounds speech to 1000 code units without splitting a surrogate pair.
Rate and pitch accept finite values between -1 and 1, mapped to SAPI's -10 through
10 ranges. User text is XML-escaped before the pitch wrapper is applied.

Quiet mode and Qt's active-accessibility-client signal suppress sound while still
completing the line. Qt's signal is a conservative accessibility heuristic, not a
claim to detect every running screen reader. Errors bypass category cooldowns;
logical event supersession replaces both pending language lines together.

## Reproducible local verification

From an MSVC x64 developer shell with the supported Qt 6.10 prefix:

```powershell
cmake -S src/experience/tests/narratorstandalone -B build.narrator -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$env:QT_ROOT_DIR"
cmake --build build.narrator
$env:PATH = "$env:QT_ROOT_DIR/bin;$env:PATH"
./build.narrator/narrator_engine_tests.exe
```

The executable uses QCoreApplication, compiles the actual SAPI backend, enumerates
installed voices, and exercises the production NarratorEngine with a silent fake
backend backed by real child processes. Cases include exit, crash, failed process
startup, timeout, repeated cancellation, stale timer races, missing voices,
re-enumeration, bounds, nonfinite settings, quiet mode, no engine, localized pair
supersession, and exact distinct English/Cantonese text and voice order.

No audible synthesis or GUI interaction is performed by this test. It proves engine
orchestration and native compilation/enumeration, not acoustic output, preferences
interaction, or complete application integration. Those require separate built
application evidence. No tests are added to GitHub Actions.

SAPI pitch and asynchronous flags follow Microsoft's documentation:
[XML TTS tutorial](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee431815(v=vs.85))
and [SPEAKFLAGS](https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ms717252(v=vs.85)).
