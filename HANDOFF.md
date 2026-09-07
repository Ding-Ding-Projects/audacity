# Handoff

## Post-release converter containment integration

The reviewed containment work through `112e981e96d7def4374ff95e93113001e8a55e61`
is integrated after the `v4.0.0-m3.14` application source. It is not part of that
already-published binary. qpdf now uses a suspended LPAC process with exactly the
required `registryRead` capability, no network capability, verified kernel identity,
explicit inherited pipes, bounded copied input/runtime data and parent-owned output
publication. Original input ACLs are unchanged.

Local evidence at implementation `7da85374037def3bcc2a2ad43d74b3fa4c6add76`:
34 PDF cases, 28 native transaction cases, 14 containment cases and core smoke
passed. Three deliberate isolation mutations failed before worker execution.
The final ownership/teardown repair passed five cleanup and 14 containment cases;
independent source review refuted the remaining reported findings. Normal profile
deletion is checked, failed deletion is observable and never silently retried, and
pre-existing registrations cannot be adopted merely because storage is absent.

The operating system's registry-read resource boundary and possible leftovers
after a hard parent crash are documented. This is not a virtual-machine isolation
claim. Capability probing must run away from the GUI thread. Full converter UI,
packaged-product execution and remaining format integration are still pending.

## Published incremental release

[`v4.0.0-m3.14`](https://github.com/Ding-Ding-Projects/audacity/releases/tag/v4.0.0-m3.14)
was published as a non-draft prerelease at `2026-09-06T23:48:41Z`.
Release ID: `383755479`. Its immutable tag points directly to the freshly built
application source `e41471c2d1745acc6e1966576ecab3ce1abe0403`.

- Fresh application: 95,618,048 bytes, SHA-256 `16d393e5fb4fdaee52504e8cd12bfe787c866f7c598a59f7a55e40d26782d320`, embedded version `4.0.0`, unsigned.
- `Setup.exe`: 82,861,056 bytes, SHA-256 `329f0ff92019899040028801aa0c5236adb136c16508194a569eb732d079d84c`.
- Full `4.0.0-m3014` package: 82,004,089 bytes, SHA-256 `97c1d9d79c3c11644deb2732771266c62a6365811464716b58f21504f550f6b8`.
- Delta from the verified published `4.0.0-m3013` baseline: 8,206,341 bytes, SHA-256 `e995ca98ebbc4a1dfc27ec51f80f1ebaa925cf21fc7209565633f6c3841143a2`.
- All eleven release assets were downloaded after publication and matched their byte counts and SHA-256 digests. The package's application matched the fresh executable exactly; all ten qpdf pins passed and no bootstrap administration records were packaged.
- The release includes manifests, distinct package/release checksums, public-safe build provenance, the committed counter's line-count evidence, and the existing AI-generated catalog image with accurate origin disclosure.

The canonical local build used an ordinary short-path checkout, not a junction.
Its successful build took 506.369 seconds; packaging took 344.3209399 seconds.
Both failed path attempts were preserved. Publication used the shared create-only
tag reservation utility, whose UTF-8 transport and uncertain-receipt fixtures passed.

This incremental release fulfills publication of a real installer. It is not the
final product-completion milestone: installation/update execution, full audio and
project regression, built UI matrices, remaining feature integration, a current
successful hosted delivery, website deployment verification, and cleanup remain
unfinished. Do not use this release as evidence that those criteria passed.

## Verified packaging integration

The packaging candidate `30cda45b51fbccf2d01ca5af7ca27169478e5aab`
combines transactional Squirrel publication with pinned qpdf provisioning and
package inspection. The actual `package.cmake` route completed with exit 0,
without `RELEASE_TAG`, using run 903 and the retained run-902 package as baseline.
This integration preserves the earlier source history.

Package generation now validates coherent older baseline identity using the
version comparer in pinned Squirrel, exact feed hashes and byte counts, exact
manifest ownership, and recoverable directory activation. Unknown output files
are retained through refusal. Publication keeps package-level checksums as
`PACKAGE-SHA256SUMS`, separate from the later release-wide checksum inventory.
All four supported Windows packaging modes use Squirrel; archive-only overrides
are rejected. CI and packaging both provision qpdf through the shared hook.

Local evidence:

- Publication boundary suite: 40 cases passed at `3b7fa920c9c43c5919d177041cf4d1757a2187f6`.
- PDF operations: 34 passed; native transactions: 28 passed, no skips; three CTest executables passed at `c7f0f0f9ceb5c4aabf47de49a03a2b2753c9a0f7`.
- qpdf bootstrap: 13 cases passed; package inspector: seven synthetic ZIP cases passed at that same source.
- Actual full and delta packages generated from the frozen combined source. The full package independently passed all ten qpdf component hashes, with no bootstrap administration records included.
- All 518 original payload files, all 530 provisioned-copy files, and the prior baseline remained unchanged.
- Combined receipt SHA-256: `64A2D4E054FFF239DBBCE7955BC24960D40AA2DCD7ED57CC2B5E150FAE2A3E5B`.

| Local output | Bytes | SHA-256 |
| --- | ---: | --- |
| `Setup.exe` | 82,859,008 | `3a371c49836e3a8d12fe15dffea5f289098e436c8b54056a410cabe6b04ca764` |
| `Audacity-4.0.0-ci000903-full.nupkg` | 82,002,241 | `583907f955c4f02d8c84a00054f33ac9ad05e1dc1f390604b6ef37572765a3dd` |
| `Audacity-4.0.0-ci000903-delta.nupkg` | 3,353,073 | `d63e3056a0b919a767c826e6b3ff4936bf07724a2bd3e9d3f614cb21aed08f23` |

The executable remains the older `bbeb45e1ebbc281d051ff8a9b95f012e54a4e734`
payload identified below. These files prove real packaging and tool inclusion,
not a rebuilt combined application, installed-client update, UI behavior, or a
published release. Do not publish them as the final full-product candidate.

The converter backend is present, but its complete user-facing registration and
restricted subprocess containment remain separate implementation work. Resource
limits and pinned binaries alone do not prove network or file-access isolation.
The profile, appearance, logo, experience, Ollama and website lanes also retain
their separate implementation and runtime-evidence boundaries.

## Current delivery integration

The delivery workflow targets `main` and Windows x64. It builds and packages an unsigned Squirrel.Windows installer, then publishes one unique non-draft prerelease for the run. Tags do not trigger recursive delivery. Tests and lint remain local; their results are not workflow release prerequisites.

GitHub Pages, Issues, Discussions and the wiki are enabled. The repository homepage was verified as https://ding-ding-projects.github.io/audacity/. Documentation publication stages `docs/site` at the root and reads real release metadata. It follows the exact `Material Audacity Windows delivery` workflow name.

Tracking issue: https://github.com/Ding-Ding-Projects/audacity/issues/6
Progress discussion: https://github.com/Ding-Ding-Projects/audacity/discussions/7
Release announcement: https://github.com/Ding-Ding-Projects/audacity/discussions/5

The earlier issue #3 and Discussion #4 returned not-found responses during the
publication handoff. Fresh records carry the current verified state; those earlier
links are retained here only as historical identifiers. Discussion pinning was
not exposed by the available GraphQL schema, so no pin was claimed.

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
