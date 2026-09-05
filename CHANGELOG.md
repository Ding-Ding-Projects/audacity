# Changelog

Every entry names the commit it came from. The full commit hash is the link
target, and a build time check refuses to configure the project when a hash in
this file is not present in the repository.

The format follows Keep a Changelog and the project uses semantic versioning.

## Unreleased

### Added

- Toolkit module: a local model manager for the Ollama HTTP API (health,
  installed models, catalog browsing, hardware fit verdicts, a batch pull
  cart with no payment concept, streaming chat, allowlisted harness
  profiles), a universal export service (JSON, JSON Lines, YAML, TOML, XML,
  CSV, TSV, Markdown, HTML, SQL and a store-only ZIP archive) with
  field-dropping disclosure, a reusable bulk selection control, external
  code editor detection and handoff, an in-app documentation browser
  bundling the feature articles for offline reading, and a reusable failure
  recovery card, all reachable from a new Toolkit preferences page
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
