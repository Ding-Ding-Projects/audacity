# Architecture overview

Material Audacity keeps Audacity's audio engine and rebuilds the shell UI on
Material 3 primitives.

## Layers

1. Audio engine (unchanged Audacity core).
2. Material 3 shell: navigation rail/bar, command palette, settings.
3. Personal vocabulary layer: client-side text substitution, never sent
   anywhere.
4. Local history: an append-only log of user actions, kept in local storage.

## Build and release

Windows packaging uses Squirrel.Windows. Code signing is permanently
disabled in this project's release pipeline; the app checks its unsigned
update feed in the background and shows an accessible "ready to restart,
this update is unsigned" banner.
