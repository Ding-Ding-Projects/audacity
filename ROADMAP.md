# Roadmap

This file tracks the Material Audacity rebuild by phase. It is a checklist:
ticked items are implemented and verified against the built application;
unticked items are open work.

## Phase 1: token engine and first surfaces

- [x] Material 3 token engine, component library, fonts and gallery
- [x] Material 3 project scene, toolbars, track headers, clips and history panel
- [x] Material 3 app shell, title bar, home, about and first launch setup
- [x] Material 3 preferences, effects, project and export dialogs
- [x] Cantonese (Hong Kong) translation baseline and coverage check
- [x] Unsigned Squirrel.Windows packaging, replacing WiX
- [x] Documentation site under docs/site
- [x] Linux smoke captures under Xvfb

## Phase 2, wave one: companion features and legacy chrome removal

- [x] Language mode setting (English, playful Hong Kong Cantonese, bilingual)
- [x] Independent English and Cantonese funny level sliders
- [x] Five attention support modes (focus, low stimulation, time awareness,
      one thing at a time, momentum), off by default
- [x] Scheduled settings rules (local, HTTPS API, Home Assistant sources)
- [x] Local personal vocabulary JSON upload with replace, clear and reset
- [x] Corner notification stack and notification centre with search, bulk
      actions and export
- [x] Super confirmation gate for destructive actions
- [x] Command palette on Ctrl+Shift+F
- [x] Regular expression builder workbench, anchored beside search fields
- [x] Background Squirrel.Windows update checker with a ready to restart banner
- [x] Per element appearance editor with typography, color, corner radius,
      animated rainbow, and per state overrides
- [x] Application display name setting
- [x] Toy locks with six credential policies and a shared PIN keypad
- [x] Support Tickets joke desk
- [x] Local offline authenticator with in process QR code and RFC 6238 codes
- [x] Local model manager for the Ollama HTTP API
- [x] Universal export service (JSON, JSON Lines, YAML, TOML, XML, CSV, TSV,
      Markdown, HTML, SQL, store only ZIP)
- [x] Reusable bulk selection control
- [x] External code editor detection and handoff
- [x] In app documentation browser
- [x] Replaced the legacy FlatButton with M3Button or M3IconButton across
      most dialogs and panels listed in CHANGELOG.md
- [x] Material Design 3 colour roles for the muse StyledTableView,
      PageIndicator and AccountAvatar

## Phase 2, wave two: open work

- [ ] Dim sum surprise (10% startup chance dish, bundled local assets, no
      opt out)
- [ ] School mode (universal rename, unlock credential, suppress advanced
      language and customization features)
- [ ] Spoken narrator for app events, off by default, English/Cantonese/Both
- [ ] Completeness inventory guard script that fails when a canonical
      feature row is missing, stale, or unimplemented
- [ ] Confirm every appearance override from the per element editor actually
      renders in the live QML tree, not only persists to settings
- [ ] Remove the remaining legacy FlatButton usages outside the list already
      converted in CHANGELOG.md
- [ ] A full capture matrix for the surfaces listed as uncaptured in
      README.md (toy lock wizard, authenticator pairing, Support Tickets,
      Ollama manager, export dialog, docs browser, attention support toggles)
- [ ] Cut and verify release v4.0.0-m3.1 once the wave two items above are
      complete
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
