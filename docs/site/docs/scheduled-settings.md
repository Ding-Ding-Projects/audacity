# Scheduled settings

A persisted table of rows, each of which changes one setting either once, at
a time of day, or for the whole span of a window between a start time and an
end time. The table is stored as JSON in `experience/schedule/entries`.

## A row

| Field | Meaning |
| --- | --- |
| `id` | Stable identifier, generated when the row is created. |
| `enabled` | Whether the row fires. |
| `hour`, `minute` | Start time of day, 24 hour. |
| `endHour`, `endMinute` | End time of day. `-1` on both means the row fires once, at `hour:minute`, instead of holding a window open. |
| `startDate`, `endDate` | Optional ISO 8601 dates (`yyyy-MM-dd`) bounding the whole rule. Empty means unbounded on that side. |
| `weekdayMask` | Bit 0 is Monday, bit 6 is Sunday. |
| `key` | Which setting to change. |
| `value` | The value to set when the source is `local`. Also used as the value applied when a Home Assistant entity reads `on`. |
| `source` | `local`, `httpsApi`, or `homeAssistant`. |
| `apiUrl` | Used when `source` is `httpsApi`: an HTTPS endpoint returning a JSON object. |
| `haBaseUrl`, `haEntityId` | Used when `source` is `homeAssistant`: the base URL of the instance and the boolean entity to read. |

A row naming anything else is rejected when it is saved and never stored.

## The settings a row may change

| `key` | Values |
| --- | --- |
| `languageMode` | `english`, `cantonese`, `bilingual` |
| `theme` | `light`, `dark`, `system` |
| `density` | `0`, `-1`, `-2`, `-3` |
| `seedColor` | Any colour Qt can read, for example `#926BFF` |
| `fontFamily` | A font family name |
| `fontSize` | 8 to 32 |
| `reducedMotion` | `on`, `off` |

## One shot rows and window rows

A row with no end time behaves the way it always has: it fires exactly once,
the moment the clock passes its start time, on the days it is scheduled for.

A row with an end time holds a window open. The setting is applied the moment
the window starts, and the value the setting held immediately before is
restored the moment the window ends. A window that crosses midnight is
supported: an end time earlier than the start time means the window runs
through the night. An optional start date and end date bound the whole rule
to a date range on top of the weekday mask.

## Where the value comes from

- `local` reads the row's own `value` directly. This is the only source a row
  needs when nothing outside the application should decide the setting.
- `httpsApi` fetches `apiUrl` with a 5 second timeout, refuses to follow a
  redirect, caps the response body at 64 KB, and reads exactly the field
  named by the row's own `key` from the returned JSON object. Every other
  field in the response is ignored.
- `homeAssistant` reads the state of one boolean entity (for example an
  `input_boolean`) at `haBaseUrl`. When the entity reads `on`, the row's
  `value` is applied; when it reads `off`, nothing is applied (and a window
  row is treated as though it were not in its window).

A remote row keeps whatever it last resolved, or falls back to its own
starting `value`, until the next answer arrives, so a slow or unreachable
source never blocks the rest of the schedule and never leaves a stale value
applied indefinitely without an attempt to refresh it.

### The Home Assistant token

Reading a Home Assistant entity needs a long lived access token. There is one
token for the whole schedule table, entered once in the scheduled settings
section of the preferences page. It is kept in this module's own local
settings, is never written to a log, an export, or the local version history,
and is never echoed back to the interface once it has been entered.

This is a known, documented limitation rather than a claim of stronger
protection: the token sits in the same on-disk settings store as every other
local preference, not in the operating system's own credential vault. A
future revision that moves it into a platform credential store keeps the
same schedule format and the same field name.

## The scheduler

`au::experience::SettingsScheduler` looks at the clock once a minute.

`SettingsScheduler::nextFire(entry, from)` is a pure function used for one
shot rows: it returns the first moment at or after `from` when the row
fires, or an invalid date when the row is disabled, has no days, falls
outside its date bounds, or names an unknown setting.

`SettingsScheduler::isWithinWindow(entry, now)` is the equivalent pure
function for window rows: it reports whether `now` falls inside the row's
window, on one of its days, and inside its date bounds. Both are covered
directly by `src/experience/tests/settingsscheduler_tests.cpp`, including the
roll over to the next day and the skip over days that are not in the mask.

## The interface

The rows are a Material 3 list on the "Language and accessibility"
preferences page. Each row shows the time, the days, what it changes and when
it fires next, with a switch to turn it off and a button to remove it. Adding
or editing a row opens a Material 3 bottom sheet with `M3TimePicker` for the
start time, an optional end time for a window row, two text fields for the
start and end dates, filter chips for the weekdays, two dropdowns for the
setting and its value, and a source dropdown that reveals the matching
fields for an HTTPS API or a Home Assistant entity.

## Failure modes

- An unreachable or slow HTTPS or Home Assistant source never blocks the
  rest of the schedule; the affected row simply keeps its last known value.
- A response larger than 64 KB, a redirect, a malformed JSON body, or a
  response missing the expected field is ignored, and the row's own starting
  value is used instead.
- A row that becomes invalid (an unknown key, a bad date, an `https` URL that
  is no longer valid) is dropped the next time the schedule is saved; it is
  never silently kept half-applied.
