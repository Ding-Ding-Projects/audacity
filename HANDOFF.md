# Handoff

## Documentation language integration in progress

The documentation branch adds an authored Cantonese catalog, independent
English/Cantonese feedback levels, bilingual language-marked spans, reversible
language and local-vocabulary rendering, and bounded dynamic message templates.
Provider-owned release records and documentation articles remain unchanged.
Dynamic template values are treated as data. Notification and history timestamps
are separate from their translatable messages; history rows use text assignment
instead of HTML interpolation.

The focused presentation and personal-vocabulary suites pass 31 tests locally.
The presentation suite also verifies that the browser catalog exactly matches
the maintained JSON. Complete browser interaction, screen-reader, viewport,
theme, and scale evidence is still missing. Documentation article translation,
full dynamic-copy coverage, and other canonical website features remain open.
This branch's current language work has not been deployed or accepted as complete.

## Current delivery integration

The delivery workflow targets `main` and Windows x64. It builds and packages an unsigned Squirrel.Windows installer, then publishes one unique non-draft prerelease for the run. Tags do not trigger recursive delivery. Tests and lint remain local; their results are not workflow release prerequisites.

GitHub Pages, Issues, Discussions and the wiki are enabled. The repository homepage was verified as https://ding-ding-projects.github.io/audacity/. Documentation publication stages `docs/site` at the root and reads real release metadata. It follows the exact `Material Audacity Windows delivery` workflow name.

Tracking issue: https://github.com/Ding-Ding-Projects/audacity/issues/3
Progress discussion: https://github.com/Ding-Ding-Projects/audacity/discussions/4

The complete product and release-verification goal remains unfinished. This integration does not claim complete feature coverage, current full-UI captures, installer execution, or a final release verdict.

## Measured local build and package evidence

- Application source: `bbeb45e1ebbc281d051ff8a9b95f012e54a4e734`.
- The supported build script completed with Qt 6.10.1 and MSVC 19.44.35228.
- `Audacity4.exe`: 95,616,000 bytes; SHA-256 `4a2158a2440a54b150f81cdf46b22eb52302769dd3e5e56c10910fbde94f4483`.
- An immutable copy of the installed tree contains 518 files and 219,906,074 bytes. Source, copied, and post-copy manifests matched.
- Packaging-wrapper source: `31aa25adacbcc6a4c777f9589a4cc958189bd717`.
- `Setup.exe`: 79,747,072 bytes; SHA-256 `7b0fe774ea92fffd0c04bb5c346dd01a999639ada74e80979e4ebc4765002fee`; signature status `NotSigned`.
- The test package is `Audacity-4.0.0-ci000000-full.nupkg`. It is packaging proof, not the final release candidate. Its embedded application hash matches the application source above.
- A final combined rebuild is required to bind application, packaging, and integrated feature changes to one candidate.

The existing catalog illustration is explicitly AI-generated, not a camera-origin photograph. Its reuse was approved with origin disclosure. The tracked index binds its 1254 by 1254 PNG, 2,406,444 bytes, and SHA-256 `c6ff2d32938f1e4c4ea685442f69227b8cd387f302ab8f8a62e8dd96c62b5ac0`. Release publication reads and validates those local bytes without downloading a substitute.

## Completeness verification

The independent registry contains 30 concrete surfaces and 1,170 canonical surface/feature pairs. `--strict` checks report integrity. Explicit completion requires `--completion --candidate <full-SHA>` and candidate-bound implementation, documentation, localization, test, build, interaction, image and privacy evidence.

The evidence implementation passed 57 regression cases; after a final observed-test-ID linkage change, four affected checks passed. The narrative inventory suite passed nine cases. These are verifier tests, not product feature acceptance. Product completion remains red, with 1,351 missing-delivery findings at the reviewed baseline.

## Feature lanes awaiting integration and full verification

- Front-screen provenance now uses stable source-commit provenance, rejects changed tracked/staged/untracked candidates, preserves exact submodule pins, and has executable negative fixtures. Full built UI proof remains pending.
- School mode now preserves last-known state, validates and atomically persists records, and migrates valid legacy records. Real standalone Qt service checks passed.
- The canonical local vocabulary parser has a real standalone Qt test result. Browser parser tests cover 23 cases; browser upload and reset acceptance is pending.
- The converter backend has 28 native transaction cases and two passing CTest targets. Its UI, approved overwrite, decoder isolation and broader adapter set remain incomplete.
- The preserved layered appearance editor was recovered with C++ compilation and QML parsing. Complete rollout and live property evidence remain incomplete.
- Ollama streaming, inspected image capability and local sessions have focused local protocol proof. Catalog acquisition, fuller reconciliation and final UI proof remain incomplete.

Do not close the tracking issue or describe these lanes as fully accepted from compilation or narrow tests alone.

## Remaining operational work

- Confirm the default-branch setting and the new `main` delivery run after integration.
- Integrate reviewed feature lanes and complete every canonical surface contract.
- Add a real isolated-profile route before built UI verification. `--factory-settings` resets the current profile and is not isolation; changing environment home variables does not redirect Qt Windows known-folder storage.
- The current automation approval review rejected launching the hidden verification service. No alternative UI route was used. Runtime verification remains unrun at that boundary.
- Verify installation, updates, audio/project compatibility, all language/theme/scale tuples and per-click evidence on the final combined build.
- Publish and verify the designated final release, documentation, wiki and operational skill.
- Preserve historical branches with unique commits. No branch, worktree, or stash deletion has occurred. Archive and ancestry requirements still apply before cleanup.

Projects access is unavailable with the present `read:project` permission. This does not block implementation or preservation. Existing historical Linux captures remain historical evidence and do not establish current Windows UI acceptance.
