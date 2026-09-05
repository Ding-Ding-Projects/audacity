# Attention support modes

Five modes on the Companion preferences page. Every one of them is off until
it is turned on, each works on its own, and they can be combined. None of them
changes what an action does, and none of them uses medical language: they are
described by what they do to the interface.

| Mode | Setting | What it does |
| --- | --- | --- |
| Focus | `experience/modes/focus` | Dims the bands at the edges of the window, where the toolbars and side panels sit, and leaves the work surface at full strength. |
| Low stimulation | `experience/modes/lowStimulation` | Switches the Material 3 scheme variant to the desaturated `neutral` variant and turns reduced motion on. |
| Time awareness | `experience/modes/timeAwareness` | Shows the clock and how long this session has been running, in the bottom left corner. |
| One thing at a time | `experience/modes/oneThingAtATime` | The same dimming as Focus, wider and stronger, so only the current task surface stands out. |
| Momentum | `experience/modes/momentum` | A short, quiet acknowledgement when an action finishes. No streaks, no scores, no comparison with anybody. |

## Composing with reduced motion

Low stimulation **adds to** the reduced motion setting in Appearance; it never
switches it off. When the mode is turned on, the previous scheme variant and
the previous reduced motion value are stored, and turning it off puts both
back. Nothing else in the module ever writes the reduced motion setting.

Every animation in the companion surfaces reads `M3.motion.reducedMotion` and
takes zero time when it is set.

## How they are drawn, and the limits

Focus, one thing at a time, time awareness and momentum are drawn by
`ExperienceOverlay.qml`, a transparent layer over the window that never takes
pointer input. That has a consequence worth stating plainly:

- The dimming is drawn over the panels rather than applied by the panels
  themselves. It marks the edges of the window as secondary; it does not hide
  a toolbar and it does not collapse a dock panel.
- One thing at a time therefore pushes secondary surfaces into the background
  visually, rather than removing them from the layout.

Making the panels dim and collapse themselves needs changes inside the
appshell dock layout, which is outside this module.
