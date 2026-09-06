# Changelog

Every entry names the commit it came from. The full commit hash is the link
target, and a build time check refuses to configure the project when a hash in
this file is not present in the repository.

The format follows Keep a Changelog and the project uses semantic versioning.

## Unreleased

### Added

- Local history: the whole local history is now packed and embedded into the
  project's own `.aup4` save file after every save (controlled by the new
  `chronicle/embedHistoryInSaveFile` setting, on by default), so a project's
  history travels with the file to another machine. Opening a project reads
  back whatever is embedded and merges it in fast forward only, so an older
  bundle, an unrelated one, or one this machine already has can never
  discard a locally recorded revision. Embedding happens after the project
  itself has already saved and can never fail the save: if it does not go
  through, a non-blocking notification says so and the previously embedded
  copy, if any, is left in place.
- Local history: the identifier a project's history is filed under is now a
  stable id stored in the project's own `.aup4` database, created the first
  time it is needed, rather than a hash of the file's path. A project's
  history now follows it across a rename or a move instead of starting over.
  A project that cannot reach its own database falls back to the previous
  path based identifier.
- Local history: every undoable edit (cut, paste, moving a clip, applying an
  effect, adding or deleting a track, editing a label or an envelope point,
  and so on) is now recorded as its own revision, named after the edit
  itself, controlled by a new `chronicle/commitOnEveryAction` setting that
  defaults to on. A drag that Audacity's own undo stack already consolidates
  into one entry becomes exactly one revision, taken when the drag settles.
  Every recorded action is also grouped into one of ten families (Edit, Clip,
  Track, Effect, Generate, Label, Envelope, Project settings, Save, Restore)
  so the version history panel's filter chips read the way a user thinks
  about their own work, and saves and restores are marked as milestones.

- Personalize module: the per element appearance editor grew a full layered
  style workspace on top of its existing typography, colour and radius
  overrides, an ordered stack of fill (solid, two stop gradient, local
  image), stroke, shadow, glow, blur, tonal adjustment, transform and mask
  layers per element and per interaction state (normal, hover, focus,
  pressed, selected, disabled, dragged, error, loading, success, warning),
  each with its own opacity and blend mode; a new `AppearanceLayers`
  singleton stores it as versioned JSON separate from the flat overrides
  file, and a new `M3AppearanceLayers` item in the shared Material 3
  component library renders the stack live using `QtQuick.Effects`, wired
  into `M3Surface` and `M3Button` behind an opt in element id so every
  existing element that has not customised a layer stack pays nothing; the
  editor itself became a resizable, draggable side sheet with tabs for
  typography, layers, fill, stroke, effects, adjustments, transform and
  preview, an in memory undo and redo stack recorded through the personalize
  mutation history, a before/after toggle, a multi state preview strip, and
  a capability matrix in its documentation naming exactly which tools are
  implemented, partial, or not yet supported
- UI components module: every `M3Menu` and the flat list popup used by
  `M3Dropdown` now show a keyboard focusable filter field by default, not
  only for long menus, plain text matching first with the field's own
  anchored regular expression builder one click away, an announced result
  count for screen readers, an honest "No matching items" state, and Escape
  clearing the filter before closing the menu; every menu and dropdown built
  on top of these two, including the tab strip's right click menu and the
  workspace and snap toolbar dropdowns, gets this for free
- Experience module: a focused unit test for the notification centre
  covering id assignment, active versus dismissed state, history ordering
  and its size limit, and the action request channel
- Experience module: a dim sum surprise card that draws once per launch with
  a fresh 10% chance and names a random dish bilingually with a photo cached
  from the public catalog and never vendored in this repository, a universal
  School mode shared across every app on the machine with a rename path, a
  shared PIN/password unlock, live file watching, and full suppression of
  Cantonese, bilingual presentation, funny levels, personal vocabulary and
  the dim sum surprise while it is on, and an off by default narrator with
  English/Cantonese/Both speech, per-language voice pickers, rate and pitch
  controls, and a serialized, debounced, cooldown-aware queue that never
  suppresses an error
- Personalize module: a per element appearance editor reachable from every
  element's right click menu and Shift+right-click (typography, colour with
  the animated rainbow option, corner radius, and per state overrides), an
  application display name setting, toy locks with six credential policies
  and a shared PIN keypad, a joke Support Tickets desk whose one real action
  opens the application data folder, and a local offline authenticator with
  an in-process rendered QR code and RFC 6238 codes, all reachable from a
  new Personalize preferences page
- The per element appearance overrides now actually render: Material 3
  buttons read their container colour, content colour and corner radius
  through the same overrides store the editor writes to, live, without the
  shared component library depending on the personalize module
- Toolkit module: a local model manager for the Ollama HTTP API (health,
  installed models, catalog browsing, hardware fit verdicts, a batch pull
  cart with no payment concept, streaming chat, allowlisted harness
  profiles), a universal export service (JSON, JSON Lines, YAML, TOML, XML,
  CSV, TSV, Markdown, HTML, SQL and a store-only ZIP archive) with
  field-dropping disclosure, a reusable bulk selection control, external
  code editor detection and handoff, an in-app documentation browser
  bundling the feature articles for offline reading with per-article
  bookmarks (a small persisted list model with add, remove, rename and bulk
  export or removal), and a reusable failure recovery card, all reachable
  from a new Toolkit preferences page
- Language mode setting with English, playful Hong Kong Cantonese and a
  bilingual mode, applied live to the interface translator where the platform
  allows it
- Separate funny-level sliders for English and Cantonese message copy, and a
  "Show emojis in dialogs and message boxes" toggle
- Five independent attention support modes (focus, low stimulation, time
  awareness, one thing at a time, momentum), off by default
- Scheduled settings rules with local, HTTPS API and Home Assistant sources
- A local personal vocabulary JSON upload with replace, clear and reset
- A corner notification stack and notification centre with search, bulk
  actions and export
- A super confirmation gate for destructive actions, with two independent
  keys, a full range slider and an emergency exit
- A "Language and accessibility" preferences page collecting all of the above
- A command palette on Ctrl+Shift+F that indexes every action, preferences
  page, setting and appearance control, with rich inline controls and
  teleport to the exact destination
- A regular expression builder workbench with guided and raw construction,
  live matches and captures, saved test cases and snippets, anchored beside
  every search field that offers it
- A background Squirrel.Windows update checker with a non-blocking ready to
  restart banner, a manual "Check for updates" action, and an Updates
  preferences page; unsigned by design, verified only against the release
  feed's own hash
- A hand written canonical feature completeness inventory
  (`docs/inventory/completeness-inventory.md`) and a configure-time guard
  (`buildscripts/checks/completeness_guard.py`) that fails when a row's
  referenced implementation, documentation, test or capture path does not
  exist on disk, plus a negative regression proving the guard actually goes
  red before it is trusted; a design parity inventory skeleton
  (`docs/inventory/design-parity-inventory.md`) naming the Material Design 3
  specification as the project's design reference in the absence of a
  checked-in design reference folder; and a status reporting article
  (`docs/features/status-reporting.md`) explaining why this public
  repository reports status through its release workflow and changelog
  rather than a private status integration
- A debug-only `AU_OPEN_PREFERENCES` environment hook that opens the
  Preferences dialog on an exact page, and optionally scrolls an exact
  section into view by name, so a capture no longer has to click through
  the dialog by hand; documented in `docs/design/CAPTURES.md`

### Fixed

- The dim sum surprise's photo fetch now actually completes: it follows up
  to two redirects onto an explicit allowed-host list (a GitHub release
  asset download always redirects once to a signed object storage URL), it
  honours the desktop's own proxy configuration, and the catalog response
  size cap was raised from 2 MB to 16 MB to fit the real published catalog
  (a little over 8 MB for 2,866 dishes), none of which the fetch could ever
  have completed under before
- The Experience preferences page's internal Flickable can scroll again;
  the page had overridden its own height to match its full content height,
  which made the Flickable's height equal its content height and left
  nothing above the visible fold reachable by scrolling, keyboard
  focus-into-view, or a command palette teleport

### Changed

- Replaced the legacy FlatButton with M3Button or M3IconButton across the
  get effects dialog, the effect card, the track effects panel, the track
  ruler and spectrogram ruler zoom popups, the label editor top panel, the
  clip item accessibility select button, the playback level meter, the
  playback meter panel, the loop in/out dialog, the delete behaviour
  onboarding dialogs, the spectrogram settings dialog and the Nyquist prompt
- Gave the muse StyledTableView, PageIndicator and AccountAvatar Material
  Design 3 colour roles instead of raw theme colours, through a new patch in
  the muse overlay
- Stopped the Welcome dialog and the First Launch Setup wizard from opening
  on their own. The welcome dialog no longer re-arms itself after a version
  change, and the wizard no longer blocks startup; both are reachable at any
  time from Help and from the command palette, and sensible defaults apply
  immediately on a fresh profile (see docs/features/no-nagging.md)
- Replaced the legacy KnobControl with the existing M3Knob in the DTMF, tone
  and chirp generator parameter knobs, and the legacy IncrementalPropertyControl
  with a new shared M3NumberField component in the time signature popup;
  promoted M3NumberField itself into the Audacity.M3 library so every module
  can use one shared component instead of a locally duplicated wrapper
- Fixed the export sheet's card assigning a non-existent elevation property
  on M3Card, which logged a QML property error on every load; it now sets
  the variant property M3Card actually exposes
- Added a Material Design 3 audit guard (docs/features/material-guard.md)
  that scans every QML file for legacy Muse controls, hard-coded colour and
  radius literals, and raw theme colour reads outside the Audacity.M3
  library, checked against a hand-written inventory at
  docs/inventory/material-audit.md

### Documentation

- Rewrote README.md as a picture-first, tabbed document with a reviewed
  Screenshots section grouped by surface (home, first run, preferences,
  menus and tabs, dialogs, project and tracks, design system, wave two
  features, display scale), an honest "no capture yet" list, refreshed line
  counts, and a Verification section stating what has and has not been
  checked in this pass
- Added a filterable Gallery page to the documentation site
  (docs/site/screenshots/gallery.json plus a new renderer in
  docs/site/js/app.js), with the same reviewed captures grouped by surface
  and a search field wired to the regex builder
- Added the missing dim sum surprise, narrator, no unsolicited
  interruptions, school mode, and status reporting articles to
  docs/features/README.md's index

## 4.0.0-material.1 - 2026-09-05

The first Material Design 3 preview of Audacity 4.

### Added

- Material 3 token engine, component library, fonts and gallery ([`9e500daf17`](https://github.com/Ding-Ding-Projects/audacity/commit/9e500daf17c1f55f928c92d84090a335be253c97))
- Material 3 project scene, toolbars, track headers, clips and history panel ([`19f306a603`](https://github.com/Ding-Ding-Projects/audacity/commit/19f306a603814df386ef9cb06286b6dc189f13ce))
- Material 3 app shell, title bar, home, about and first-launch setup ([`bacb4d8352`](https://github.com/Ding-Ding-Projects/audacity/commit/bacb4d8352ab18fcda449d11abad44565ad68aae))
- Material 3 preferences, effects, project and export dialogs ([`6d0a79e10f`](https://github.com/Ding-Ding-Projects/audacity/commit/6d0a79e10f249053590f8653476263f16cd848d7))

### Changed

- Apply the repository uncrustify formatting to the M3 engine sources ([`637ff6ff2c`](https://github.com/Ding-Ding-Projects/audacity/commit/637ff6ff2cefdc7668d457babd5a567fac6914f4))
- Apply qmlformat to the Material 3 component library and project scene ([`5fbb2e37da`](https://github.com/Ding-Ding-Projects/audacity/commit/5fbb2e37da8054761cb844b0c6429d1d2c3c925a))
- Apply the repository qmlformat profile to every module ([`f757d45efc`](https://github.com/Ding-Ding-Projects/audacity/commit/f757d45efc71d0f16c935bd4d5bbd4f22ee6ee06))
- Merge remote-tracking branch 'origin/master' into claude/audacity-material-design-3-ui-voxy2f ([`1a55c8f685`](https://github.com/Ding-Ding-Projects/audacity/commit/1a55c8f68579671085b015cc3a5184620d857d44))

### Build and release

- Apply a Material 3 patch overlay to the muse framework at configure time ([`567fe4e6f5`](https://github.com/Ding-Ding-Projects/audacity/commit/567fe4e6f5735ef608fd87e37ccca06840be34de))
- Mark font and image files as binary in .gitattributes ([`f96c2d9c2f`](https://github.com/Ding-Ding-Projects/audacity/commit/f96c2d9c2f984654864ff717d808aa6fc032172c))
- Replace WiX with unsigned Squirrel.Windows packaging and add release workflows ([`fbb9a12230`](https://github.com/Ding-Ding-Projects/audacity/commit/fbb9a12230c1e77d4d7c228f45dc91daa55f7ddd))
- Let the Pages workflow enable GitHub Pages on first run ([`bd5ab56db7`](https://github.com/Ding-Ding-Projects/audacity/commit/bd5ab56db7dc582aa7d9fb4ac4b50cc96d59aee4))
- Wait for Squirrel.exe and surface its log during releasify ([`e5bce1839b`](https://github.com/Ding-Ding-Projects/audacity/commit/e5bce1839b7023e0e7e41d8c29f559ea302dc1a6))
- Wait for Squirrel.exe and surface its log during releasify ([`0a5af7b198`](https://github.com/Ding-Ding-Projects/audacity/commit/0a5af7b198eec2737ffeef5f3facb8dedadd7670))

### Documentation

- Add the Material Audacity documentation site under docs/site ([`ddea8d276f`](https://github.com/Ding-Ding-Projects/audacity/commit/ddea8d276f4ce203fd8c4f69118f02e1412285f9))
- Vendor Roboto Flex and Noto Sans HK with pinned hashes ([`436004d91b`](https://github.com/Ding-Ding-Projects/audacity/commit/436004d91b35d0c63677b89a13e148bd41b1c1a5))
- Store font files as exact binary blobs ([`310c9d8cd5`](https://github.com/Ding-Ding-Projects/audacity/commit/310c9d8cd5ecd6a2531ac3e04cb9c67c43e7fcbe))
- Add Phase 1 Linux smoke captures ([`908687b6b8`](https://github.com/Ding-Ding-Projects/audacity/commit/908687b6b84db5165bcd4f1d4ea3eb320d1f3c67))

### Translations

- L10n: add the Cantonese (Hong Kong) translation baseline and coverage check ([`951be087b0`](https://github.com/Ding-Ding-Projects/audacity/commit/951be087b0ceb48d53692eda8082ef012a09bd55))
- L10n: translate the remaining Cantonese (Hong Kong) strings ([`3fd6465ecb`](https://github.com/Ding-Ding-Projects/audacity/commit/3fd6465ecb266e20761f065a7e2862a02cdc16fb))

### Work in progress snapshots

- Preservation snapshot of the Material 3 token engine and component library ([`0e2cdb8bba`](https://github.com/Ding-Ding-Projects/audacity/commit/0e2cdb8bbaec7de9fc26b1f47ae7cd1f101b095c))
- Preservation snapshot of the Windows packaging rework ([`95b3ce30ba`](https://github.com/Ding-Ding-Projects/audacity/commit/95b3ce30bacf18b8c470b5dc2336ba0555fb7a39))
- Preservation snapshot of in-flight lane work ([`3ada230ba6`](https://github.com/Ding-Ding-Projects/audacity/commit/3ada230ba6c3c1a4ca4e28fccd0c97051027e53a))
- Preservation snapshot of font registration work ([`cdd5a2f024`](https://github.com/Ding-Ding-Projects/audacity/commit/cdd5a2f024fb4a73979f1d94caead63d9e13689a))
- Preservation snapshot of lanes B, C and D in flight ([`5f49577009`](https://github.com/Ding-Ding-Projects/audacity/commit/5f4957700931553e77664ed3cf36df8dee6b043a))
- Preservation snapshot of lanes B and D in flight ([`38c0b9c33d`](https://github.com/Ding-Ding-Projects/audacity/commit/38c0b9c33d67cd23b6dc5dc6faa2563ad1344b80))
- Preservation snapshot of lanes B and D in flight ([`a6429f6c26`](https://github.com/Ding-Ding-Projects/audacity/commit/a6429f6c262c127024330ca9a1b54befcb17911f))
- Preservation snapshot of lanes B and D in flight ([`08148d36de`](https://github.com/Ding-Ding-Projects/audacity/commit/08148d36decb42c04780b64342072be2174e7539))
- Preservation snapshot of lane D in flight ([`3bf4884ba7`](https://github.com/Ding-Ding-Projects/audacity/commit/3bf4884ba7317de85cdc5aef3174d311e9dd0b6b))
