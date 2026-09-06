# No unsolicited interruptions

Audacity never opens a dialog, banner, or notification that the user did not ask for. Anything
that only informs is a non-blocking notification the user can dismiss or ignore. A modal dialog
appears only when a real decision is required (an unsaved-changes prompt, a destructive-action
confirmation, credential entry). This page inventories every place the application used to
interrupt startup or work on its own, and states what each one became.

## Inventory

| Where it fired | What it did before | What it is now |
| --- | --- | --- |
| Startup, `StartupScenario::showStartupDialogsIfNeed` | Opened the Welcome dialog automatically on every launch until the "Don't show welcome dialog on startup" checkbox was cleared, and silently re-armed itself ("override user preference") whenever the installed version changed, even after the user had turned it off. | Never opens on its own. The setting that controlled it now defaults to off. The dialog is reachable at any time from Help, "Welcome tour", or from the `welcome-dialog` action (command palette / shortcuts). |
| Startup, `StartupScenario::showStartupDialogsIfNeed` | Blocked startup on the First Launch Setup wizard (language, theme, workspace layout, sign-in, usage info) until the user clicked through every page. | Never blocks startup. Sensible defaults apply immediately: English, funny level 5, follow the system theme, seed purple. A single non-blocking notification appears once, bottom right ("Set up Material Audacity", with a "Set up" action and a "Dismiss" action), records that it was shown so it never repeats on that profile, and auto-dismisses on its own after a few seconds like any other informational notification. The wizard itself stays reachable at any time from Help, "Set up Material Audacity...", or from the `first-launch-setup` action (command palette / shortcuts). |
| `src/usageinfo/internal/usageinfoservice.cpp` | Anonymous usage reporting. | Already off by default with no consent dialog; a Preferences toggle with plain disclosure controls it. No change was needed here; listed for completeness. |
| `src/au3cloud/internal/au3cloudactionscontroller.cpp` | Cloud/audio.com sign-in and tour pages. | Already action-triggered only (menu items, explicit buttons); nothing opens these on its own. No change was needed here; listed for completeness. |
| `src/squirrelupdate` | Update banner. | Already shows only `Checking` during a real background check and `Ready` when an update is genuinely staged; it never asks the user to "check now" as an interruption. No change was needed here; listed for completeness. |
| Help menu "What's new" | Changelog viewer. | Already opened only from the Help menu; nothing auto-opens it at startup. No change was needed here; listed for completeness. |

## What remains as a genuine decision

The only dialogs that still interrupt the user are the ones this document calls out as real
decisions: unsaved-changes prompts before closing or discarding a project, destructive-action
super confirmation, and credential or sign-in dialogs the user opened themselves. None of these
fire without the user's own action first.

## Verification

`buildscripts/checks/no_nagging_smoke.sh` launches the built application under Xvfb with a
completely fresh `XDG_DATA_HOME` and `XDG_CONFIG_HOME` (so every setting is at its shipped
default, exactly like a first install), waits for the window to settle, and greps the
application's own log for the trace lines that startup prints when it opens the welcome dialog or
the first launch setup wizard. It fails the check if either line appears. See the script for
exactly how it decides.

`docs/design/captures/lane-s/fresh-start.png` is a real capture, taken the same way, of a
completely fresh profile 28 seconds after launch: the project window is open with no dialog,
banner, or wizard on top of it.

On the very first run, a single non-blocking notification appears once, bottom right, offering
"Set up" (which opens the first launch setup pages) or "Dismiss". It auto-dismisses on its own
after a few seconds like any other informational notification, is recorded so it is never shown
again on that profile, and never blocks or delays anything. `docs/design/captures/lane-s/first-run-toast.png`
is a real capture of it.
