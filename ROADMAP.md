# Current completion pass

Previous completion ticks below are reopened: source presence and historical captures do not prove the final integrated behavior. Items remain visible so earlier work is preserved and can be reverified.

- [x] Build the Windows application through the project entry point at `bbeb45e1`.
- [x] Produce and inspect a genuine unsigned Squirrel package, with the application/package source distinction recorded in `HANDOFF.md`.
- [x] Replace permissive completion claims with a candidate-bound verifier and negative regressions.
- [x] Set the repository About URL to the canonical documentation address.
- [ ] Verify main migration and its delivery run.
- [ ] Integrate and verify all feature lanes on one candidate.
- [ ] Complete every canonical feature on every registered surface.
- [ ] Complete isolated built UI, installer/update and audio/project verification.
- [ ] Publish and verify the final release, documentation, wiki and operational skill.
- [ ] Complete preservation-proven cleanup without losing historical work.

---
# Roadmap

This file tracks the Material Audacity rebuild by phase. It is a checklist:
ticked items are implemented and verified against the built application;
unticked items are open work.

## Phase 1: token engine and first surfaces

- [ ] Material 3 token engine, component library, fonts and gallery
- [ ] Material 3 project scene, toolbars, track headers, clips and history panel
- [ ] Material 3 app shell, title bar, home, about and first launch setup
- [ ] Material 3 preferences, effects, project and export dialogs
- [ ] Cantonese (Hong Kong) translation baseline and coverage check
- [ ] Unsigned Squirrel.Windows packaging, replacing WiX
- [ ] Documentation site under docs/site
- [ ] Linux smoke captures under Xvfb

## Phase 2, wave one: companion features and legacy chrome removal

- [ ] Language mode setting (English, playful Hong Kong Cantonese, bilingual)
- [ ] Independent English and Cantonese funny level sliders
- [ ] Five attention support modes (focus, low stimulation, time awareness,
      one thing at a time, momentum), off by default
- [ ] Scheduled settings rules (local, HTTPS API, Home Assistant sources)
- [ ] Local personal vocabulary JSON upload with replace, clear and reset
- [ ] Corner notification stack and notification centre with search, bulk
      actions and export
- [ ] Super confirmation gate for destructive actions
- [ ] Command palette on Ctrl+Shift+F
- [ ] Regular expression builder workbench, anchored beside search fields
- [ ] Background Squirrel.Windows update checker with a ready to restart banner
- [ ] Per element appearance editor with typography, color, corner radius,
      animated rainbow, and per state overrides
- [ ] Application display name setting
- [ ] Toy locks with six credential policies and a shared PIN keypad
- [ ] Support Tickets joke desk
- [ ] Local offline authenticator with in process QR code and RFC 6238 codes
- [ ] Local model manager for the Ollama HTTP API
- [ ] Universal export service (JSON, JSON Lines, YAML, TOML, XML, CSV, TSV,
      Markdown, HTML, SQL, store only ZIP)
- [ ] Reusable bulk selection control
- [ ] External code editor detection and handoff
- [ ] In app documentation browser
- [ ] Replaced the legacy FlatButton with M3Button or M3IconButton across
      most dialogs and panels listed in CHANGELOG.md
- [ ] Material Design 3 colour roles for the muse StyledTableView,
      PageIndicator and AccountAvatar

## Phase 2, wave two: open work

- [ ] Dim sum surprise (10% startup chance dish, bundled local assets, no
      opt out)
- [ ] School mode (universal rename, unlock credential, suppress advanced
      language and customization features)
- [ ] Spoken narrator for app events, off by default, English/Cantonese/Both
- [ ] Completeness inventory guard script that fails when a canonical
      feature row is missing, stale, or unimplemented
      (`buildscripts/checks/completeness_guard.py`, proved red then green)
- [ ] Confirm every appearance override from the per element editor actually
      renders in the live QML tree, not only persists to settings
- [ ] Remove the remaining legacy FlatButton usages outside the list already
      converted in CHANGELOG.md
- [ ] A full capture matrix for the surfaces listed as uncaptured in
      README.md (toy lock wizard, authenticator pairing, Support Tickets,
      Ollama manager, export dialog, docs browser, attention support toggles)
- [ ] Cut and verify a release: every push to `master` now publishes
      `v4.0.0-m3.<run>`; `v4.0.0-m3.10` and `v4.0.0-m3.11` are verified
      unsigned Squirrel releases with a delta package from m3.11 on
- [ ] Local history: real captures of the timeline, storage, compare and
      star/pin surfaces (code landed, nobody has seen a screenshot yet)
- [ ] A verification matrix covering every feature article against its
      built artifact interaction proof

## Deliberately not doing (for this fork)

- Browser extension download capture dialogs: Material Audacity is a
  desktop application with no browser extension surface, so this canonical
  feature does not apply here.
- The unlock ladder (dim sum, sums, whack a mole recovery game): this
  applies to apps and pages that authenticate a user account. Material
  Audacity's toy locks are a local, single user, no account convenience
  feature with recovery by deleting the local application data folder, so
  there is no login lockout for the ladder to unlock.
- Code signing: permanently prohibited for this project. Every Squirrel.Windows
  installer stays unsigned by policy, not by omission.
