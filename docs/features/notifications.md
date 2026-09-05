# Notifications

## The service

`au::experience::INotificationCenter` collects everything the application wants
to report.

```cpp
notificationCenter()->push(NotificationType::Success,
                           title,     // plain, names what happened
                           body,      // routed through the message styler
                           actionText,
                           actionCode);
```

The title stays plain, because it names the thing that happened. Only the body
carries the tone chosen by the funny levels and the emoji switch.

Info and success dismiss themselves after a few seconds. Warning and error stay
until the reader dismisses them. Hovering a toast holds it open.

The centre keeps the last 200 notifications, dismissed ones included.

## The toast stack

`NotificationHost.qml` draws the stack in the bottom right corner of the
window. A toast is built on the Material 3 card anatomy: an elevated container,
a leading state icon coloured by severity, a title, a body and the action and
dismiss buttons. Each toast is one accessible item whose name is the title and
the body together, so a screen reader reads it as one message rather than
stopping at the icon. The fade honours reduced motion.

## The notification centre

`NotificationCentre.qml` is a Material 3 side sheet listing everything,
newest first, with the time, the title and the body, and a mark on the ones
still on screen. Its search field is an `M3SearchBar` with
`showRegexBuilder: true`; it emits `regexBuilderRequested`, which
`ExperienceOverlay` passes on so the shared regular expression builder can be
attached to it. The filter itself accepts a regular expression when the text
compiles as one, and falls back to a plain, case-insensitive contains search
when it does not.

The centre is opened through `ExperienceBridge.openNotificationCentre()`.

## Routing existing messages

A survey of `interactive()->info(...)` across `src/` finds eight call sites:

| Call site | Verdict |
| --- | --- |
| `src/effects/nyquist/nyquistprompt/nyquistpromptviewmodel.cpp` (two) | Reports the outcome of a script the reader just ran. Non-decision. |
| `src/project/internal/projectactionscontroller.cpp` (two) | "Cloud audio preview updated" and "Audio preview is up to date". Non-decision. |
| `src/preferences/.../generalpreferencesmodel.cpp` (two) | Reports the result of a language or keyboard layout change. Non-decision. |
| `src/importexport/export/view/customffmpegpreferencesmodel.cpp` (two) | "Success" after locating the FFmpeg library. Non-decision. |

All eight are reports rather than decisions, so all eight belong in a toast.
None is modal by necessity.

They are not converted here, because each lives in a module owned elsewhere in
this rewrite and converting them means editing those files. The conversion is
one line each: replace the `interactive()->info(title, body)` call with

```cpp
notificationCenter()->push(NotificationType::Info, title, body);
```

The companion features themselves already use the service: loading a personal
vocabulary reports its result as a success toast, and the Companion
preferences page can show an example on demand.
