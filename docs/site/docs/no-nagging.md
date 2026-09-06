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
| Startup, `StartupScenario::showStartupDialogsIfNeed` | Blocked startup on the First Launch Setup wizard (language, theme, workspace layout, sign-in, usage info) until the user clicked through every page. | Never blocks startup. Sensible defaults apply immediately: English, funny level 5, follow the system theme, seed purple. The wizard is reachable at any time from Help, "Set up Material Audacity...", or from the `first-launch-setup` action (command palette / shortcuts), and is also linked from Preferences. |
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

## Known gap

The "Set up Material Audacity" toast that should appear as a one-time non-blocking notification
on first run (instead of silently marking the wizard complete) is not wired yet: doing so cleanly
needs the appshell module to depend on the experience module's notification centre
(`INotificationCenter::push`), which is a larger cross-module change than this pass could safely
make under the shared build lock. Today, first run simply applies the defaults with no toast and
no wizard; the wizard stays reachable from Help and the command palette. Wiring the toast is
tracked as follow-up work.
