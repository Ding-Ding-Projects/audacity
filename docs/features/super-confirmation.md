# Super confirmation

The gate in front of an action that destroys data.
`SuperConfirmationDialog.qml`, in `Audacity.Experience`.

## What the reader sees

1. The exact action, by name, and what is affected, in their own terms, plus an
   optional line about whether it can be undone.
2. **Step 1.** Two switches, one anchored to the left edge of the card and one
   to the right, so they cannot both be flicked with one gesture. Both must be
   on.
3. **Step 2.** A slider that must be dragged the whole way across. It only
   becomes active once both keys are turned.
4. **Step 3.** A progress animation that is deliberately dramatic and never
   blocks: the emergency exit stays usable while it runs. It is followed by a
   distinct completion animation, a settle rather than a sweep.

Under reduced motion both animations take zero time and the dialog lands
straight on the finished state.

An **Emergency exit** text button is present at every stage. Escape and the
back key cancel. The scrim swallows clicks but does not cancel, because leaving
should be as deliberate as continuing. When the dialog closes, focus returns to
the control that opened it.

## Calling it

Any part of the interface reaches it through the `ExperienceBridge` singleton:

```qml
import Audacity.Experience

ExperienceBridge.confirmDestructive(
    qsTrc("appshell", "Clear history"),
    qsTrc("appshell", "Every undo step for this project is removed."),
    qsTrc("appshell", "This cannot be undone."),
    theButtonThatAsked,
    function () { model.clearHistory() })
```

`confirmDestructive` returns `false` when the overlay is not available, so a
call site can fall back to its own confirmation rather than silently doing
nothing.

## Wiring

Wired today:

- the Clear list action in the notification centre, through the same overlay.

Not wired yet, because the call sites are in files owned by other parts of the
interface: clearing the history panel, resetting preferences to defaults,
deleting a preset and Remove tracks. Each is a one-call interception in QML of
the shape shown above, at the point where the action is triggered: ask
`ExperienceBridge.confirmDestructive`, and do the work in the callback instead
of straight away.
