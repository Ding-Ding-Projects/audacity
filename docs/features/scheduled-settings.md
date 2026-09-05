# Scheduled settings

A persisted table of rows, each of which changes one setting at a time of day
on the days you choose. The table is stored as JSON in
`experience/schedule/entries`.

## A row

| Field | Meaning |
| --- | --- |
| `id` | Stable identifier, generated when the row is created. |
| `enabled` | Whether the row fires. |
| `hour`, `minute` | Time of day, 24 hour. |
| `weekdayMask` | Bit 0 is Monday, bit 6 is Sunday. |
| `key` | Which setting to change. |
| `value` | The value to set. |

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

A row naming anything else is rejected when it is saved and never stored.

## The scheduler

`au::experience::SettingsScheduler` looks at the clock once a minute. A row
fires when its next occurrence falls inside the minute that has just passed,
so a row fires once per occurrence and a missed minute is not replayed later.

`SettingsScheduler::nextFire(entry, from)` is a pure function: it returns the
first moment at or after `from` when the row fires, or an invalid date when the
row is disabled, has no days or names an unknown setting. It is covered
directly by `src/experience/tests/settingsscheduler_tests.cpp`, including the
roll over to the next day and the skip over days that are not in the mask.

## The interface

The rows are a Material 3 list on the Companion preferences page. Each row
shows the time, the days, what it changes and when it fires next, with a switch
to turn it off and a button to remove it. Adding or editing a row opens a
Material 3 bottom sheet with `M3TimePicker` for the time, filter chips for the
weekdays and two dropdowns for the setting and its value.
