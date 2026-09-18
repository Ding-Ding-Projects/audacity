# Handoff

## 2026-09-18 primary-checkout integration and preservation pass

### Cleanup closeout result

The final external archive was created before cleanup at `C:\Users\cntow\OneDrive\OakKayBackups\audacity\zips\audacity-20260918T171722Z.7z`. 7-Zip read-back reported `Everything is Ok`, 4,697,455,666 bytes, 111,994 listed entries, and 1,670 Git administrative entries.

The following task-owned branches and linked checkouts were proven clean and ancestor-proven, then removed locally and from the remote: `codex/audacity-appearance`, `codex/audacity-conversion-core`, `codex/audacity-converter-containment`, `codex/audacity-experience`, `codex/audacity-front-provenance`, `codex/audacity-logo`, `codex/audacity-ollama`, `codex/audacity-packaging-integration`, and `codex/audacity-site-completion`. Their source tips are retained in `main` history and in the verified archive.

The retained branches are `codex/audacity-converter-ui`, `codex/audacity-delivery`, `codex/audacity-profile-isolation`, `codex/audacity-release-build`, `codex/audacity-release-candidate`, and `codex/audacity-tests-f9dc58a`. Each remains pushed and has either unfinished nested Muse recovery content, a newer active preservation commit, or ownership uncertainty. No stash existed.

The linked checkout metadata for `codex/audacity-experience` was removed, but Windows left the exact directory `C:\Users\cntow\Documents\GitHub\gerk tong hui\audacity-codex-audacity-experience` with 8,165 files and 1,238 directories after `git worktree remove --force` reported `Filename too long`. It is not registered as a linked checkout or branch, and it remains retained as an explicit filesystem cleanup blocker. No broader recursive removal was attempted.

### Follow-up preservation boundary

After the first preservation sweep, a second nested Muse edit appeared in four linked checkouts as the untracked file `framework/ui/qml/Muse/Ui/M3Roles.qml`. It was preserved in a second exact patch commit on each owning branch and pushed with `git ls-remote` verification:

| Branch | Follow-up preservation commit | Remote verification |
| --- | --- | --- |
| `codex/audacity-converter-ui` | `123b3620af7103e6f8e2927e4bf1653924bc8c4f` | matched |
| `codex/audacity-release-build` | `d4b7c5faf2bb5c0bfd29e333b52937e1e489b21a` | matched |
| `codex/audacity-tests-f9dc58a` | `3934ad11c945a7ce24f6e7f2120fade4715804a2` | matched |
| `codex/audacity-profile-isolation` | `93320a5759168f6fee7995bb3c42db960a38ccfb` | matched |

The delivery branch also advanced after the first sweep with `fb7c3d3bf0b16f73691916c00bd222ac85ebb43a`, titled `Preserve Muse submodule pointer after recovery boundary`. That newer activity is treated as active and ownership-uncertain, so the delivery branch and its linked checkout are retained. The six preservation-only branches therefore remain outside `main`; their recoverable work is pushed and documented rather than removed.

This pass was limited to the primary checkout at `C:\Users\cntow\Documents\GitHub\audacity`. No release build, installer publication, or unrelated release work was run.

The primary checkout began clean at `b1d8c4416625a71f2bd2398c7c03736a588a60b5` after fetching `origin` with pruning. Five completed feature lines were integrated into `main` with non-fast-forward merges:

| Integrated line | Merge commit | Source tip |
| --- | --- | --- |
| `codex/audacity-appearance` | `d48435cf91` | `6318ab8c9d6911cb1aec0021f231a6f32a62988c` |
| `codex/audacity-converter-ui` | `0b45c656ae` | `6e99ec6e2e3ae155ce745c14b903e635c9d3a3bf` |
| `codex/audacity-experience` | `ba55a7a4b7` | `7657b96084ececcc7f771d12df6cb4c96acadf35` |
| `codex/audacity-front-provenance` | `20c41e114b` | `fe5f283fad126c8a4e3be0ea87504388e68c548a` |
| `codex/audacity-ollama` | `59cbe3e712` | `e69147f111d3baad3db4f45047bcb6da9be98021` |

The merge strategy reported no conflicts. `git ls-files -u` is empty and a tracked-text scan found no `<<<<<<<`, `=======`, or `>>>>>>>` conflict markers. Because no conflict occurred, there was no side-selection decision to record. Each merge commit retains both parent histories.

Six linked checkouts contained uncommitted content inside the nested `muse` checkout. Parent Git history cannot represent those bytes as a gitlink without changing the nested repository, so each owning parent branch received an exact binary Git patch at `recovery/muse-uncommitted-20260918.patch`. The nested files were reset only after the patch was committed. The two distinct patch variants contain 2,281 and 2,058 inserted patch lines. The preservation commits were:

| Branch | Preservation commit | Remote verification |
| --- | --- | --- |
| `codex/audacity-converter-ui` | `09dc118d942c21b8633986f4e126dbc786f13b71` | `git ls-remote` matched |
| `codex/audacity-release-build` | `3e3515ca1d514224716f5dc689d635864745511a` | `git ls-remote` matched |
| `codex/audacity-tests-f9dc58a` | `a7bbe9ccba8421fcdbefb66d42c4c69d27728a79` | `git ls-remote` matched |
| `codex/audacity-delivery` | `9d41aab7762bc295ebd8613aa89fe3572df67f63` | `git ls-remote` matched |
| `codex/audacity-profile-isolation` | `5ee317655393a90c711c1f06c8960baa7fb0ef72` | `git ls-remote` matched |
| `codex/audacity-release-candidate` | `6d6519dd1a36c0cc0ebc714bf67b5db8cc69165f` | `git ls-remote` matched |

Those six preservation-only tips were deliberately not merged into `main`: their nested Muse edits were unfinished, and the recovery patches are retained on their pushed branches. The completed parent work from `codex/audacity-converter-ui` was merged at `6e99ec6e2e3ae155ce745c14b903e635c9d3a3bf`, excluding its later preservation-only commit.

At this point the integrated `main` history is locally ahead of `origin/main` and has not yet been pushed. The next safe steps are to commit this handoff and roadmap refresh, push `main`, verify the remote ref, create and read back the required external archive, then remove only proven redundant task-owned worktrees and branches. Active, user-owned, load-bearing, unfinished, or ownership-uncertain items must remain.

## Scoped preservation cleanup completed

A fresh explicitly authorized cleanup pass created and read back a dated private
archive covering all 17 then-existing working trees, initialized submodules and
Git administrative data. The integrity test and exact entry/size comparison
passed: 112,887 archived files, 962 Git administrative files and 4,777,557,853
compressed bytes. Long-path enumeration confirmed 171,665 ignored files excluded.
All 8,534 absent tracked paths were verified intentional sparse-checkout omissions,
with their Git history and index retained. No unexpected missing files were found.
The private archive path and digest remain in the local pass receipt.

Only `codex/audacity-completeness` and its linked working tree were retired.
Its source tip `9025c07df2856d10b9160b8af69c4ee37ce5b12b` was clean, inactive
and proven an ancestor of pushed main `8491e0cd7aa71964db909f1f29f6ad9963bd603e`.
The local path, local ref and remote ref were confirmed absent after removal.
No stash existed, and tags and releases were not deleted.

Other working trees remain because they contain unfinished work, expected build
overlays, or referenced local proof and build material. Historical divergent refs
also remain preserved. Main-only closeout and the full product objective remain
incomplete; this scoped cleanup does not change those acceptance criteria.

## Preservation handoff: unfinished work retained

Implementation stopped for account-allowance preservation. The overall task is
unfinished. No final release designation, complete installation/update proof,
full UI acceptance, verified filesystem archive or cleanup is claimed.

These owning branches were pushed and their exact remote tips verified:

| Branch | Preserved commit | Next action |
| --- | --- | --- |
| `codex/audacity-front-provenance` | `fe5f283fad126c8a4e3be0ea87504388e68c548a` | Reject paired timestamp-plus-digest replacement for the same build identity; add the exact negative regression before integration. |
| `codex/audacity-experience` | `7657b96084ececcc7f771d12df6cb4c96acadf35` | Evict bilingual narration as logical groups; a 63-entry queue can currently orphan one language. Add that counterexample. |
| `codex/audacity-ollama` | `e69147f111d3baad3db4f45047bcb6da9be98021` | Fix normalization/restart idempotency, then run the committed native target. Actual result: 83 of 84 cases passed. Repair official acquisition pagination and receipt handling separately. |
| `codex/audacity-appearance` | `6318ab8c9d6911cb1aec0021f231a6f32a62988c` | Continue stable consumer registration and typography/opacity coverage beyond the two primitives. Rendered behavior is unverified. |
| `codex/audacity-converter-ui` | `6e99ec6e2e3ae155ce745c14b903e635c9d3a3bf` | Resumed configuration completed and generated build.ninja; module compilation, adapter tests, navigation, localization and batch results remain pending. Shared cancellation lifetime repair is preserved but unverified. |

Native narration compiled SAPI and enumerated two local voices without speaking.
Thirty-five orchestration assertions passed using the production engine and a
silent process-backed test double. Actual SAPI speech/poll/cancel and complete
product interaction remain unverified. Front provenance passed 139 QtCore
assertions, five generator cases and four shell cases, but those checks do not
cover its known paired-edit finding and do not authorize integration.

Run `34069712872` subsequently succeeded and published non-draft incremental
`v4.0.0-m3.17` at `2026-09-07T01:06:42Z`. Its tag resolves to
`9ef1afc960be2371edcc7db48f985219c6b4be97`. All eight attached assets were then
downloaded and matched published byte counts and SHA-256 digests. `Setup.exe`
is 74,777,600 bytes and reports `NotSigned`. Installation and full product
interaction remain unverified; this is incremental delivery, not final acceptance.
Other existing delivery runs may still be active. Re-read their actual state,
never restart them solely because a local observation ended.

The operational skill is current in the canonical private catalog at commit
`575bda125904b576721ba4e1c3840f778ab2ce95`; its shared installed copy matches.
Managed synchronization for the selected supported targets completed successfully.
An unrelated unowned catalog destination remains preserved rather than overwritten.

## Current integration and verification boundaries

The canonical documentation environment now permits `main`, replacing its stale
`master` restriction without removing other protections. Deployment run
`34070563020` succeeded for `f9dc58a2564c62d02ecd1e2121e902a894a62782`;
run `34071003745` deployed the subsequent language/narrator integration at
`b2f6ae8840458ae44a2f36cd385b16b494dbf252`. Eleven served files matched each
separately staged candidate. Run `34071289666` then published the corrected
generated-image disclosure, which was confirmed in the served script.

Logo customization is integrated at `f9dc58a2564c62d02ecd1e2121e902a894a62782`.
Explicit profile paths are integrated at `2c2012dc576ad2862156d23024ffb239d3bf44e6`;
combined branding/profile tests are integrated at
`ce52a372a56b2e383e0d8f2d8d06cb12e60aaeff`. The integrated source inventory
passed 28 consumer files and 122 omission/disabled-boundary regressions. Separate
real model/store/provider tests passed 20 parent and 94 child assertions.
Actual product GlobalConfiguration and QML singleton execution remain unverified.
Direct Ollama networking remains a pending profile-isolation consumer.

Seven audio/project regression targets compiled at
`f9dc58a2564c62d02ecd1e2121e902a894a62782` in 237.467 seconds: `au3audio_tests`,
`au_project_tests`, `au_au3wrap_tests`, `playback_tests`, `record_tests`,
`trackedit_tests`, and `importer_tests`. None was executed. The build-only receipt
does not establish any runtime regression verdict.

Active repairs remain isolated: build-bound initial-screen provenance, native
Windows narration and bilingual text, strict untrusted catalog import, appearance
consumer coverage, and the converter presentation bridge. The converter scaffold
at `542d97f05483c7f2c911d0d1778c333323d52dad` is preserved on its own branch,
not integrated or verified. A full final build, installation/update execution,
complete surface interactions and final release designation remain outstanding.

The read-only cleanup snapshot at main
`be0d69adaa6ae2d41714553c69b2bebd344c3bbe` recorded 18 local branches, 65 remote
refs, 17 worktrees and zero stashes. Five local tips and 29 remote tips diverged.
This is an inventory, not deletion proof. Active work and historical unique
commits remain retained; no filesystem archive or cleanup completion is claimed.

## Website narrator source increment

The website now has opt-in narration controls, independent runtime voice choices,
rate/pitch, quiet and explicit assistive-technology yield, preview, persistent
storage-failure feedback, and exact narrator palette destinations. Its speech
adapter serializes both languages, handles late/missing/network voices, retains
missing voice choices, prioritizes important queued events, and discloses English
fallback when Cantonese copy is unavailable. Three review counterexamples were
proven failing, then passing after repair. Browser audio, accessibility and layout
remain unverified; source tests are not full built interaction evidence. See
`docs/site/docs/narrator.md` for the concrete boundaries and remaining event coverage.

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
This language increment was integrated at `b2f6ae8840458ae44a2f36cd385b16b494dbf252`
and deployed successfully in run `34071003745`. Eleven served files matched the
staged candidate hashes. It is deployed, but complete browser acceptance remains open.



## Delivery follow-up after the first incremental release

Hosted delivery is available again. Run `34067388818` successfully published
`v4.0.0-m3.15` at `2026-09-07T00:19:32Z`, from immutable tag target
`73a820e1ce80c010c99dc328bace46c66ee9e8d3`. All eight release assets were
downloaded and matched their API byte counts and SHA-256 digests. `Setup.exe`
is 74,778,112 bytes and reports `NotSigned`; its SHA-256 is
`c922523c4dbfe28531a669bb2e4f8c037969ee7897464f67678c70e50f848b1e`.
This proves publication and byte integrity, not installation or UI execution.

Run `34069117762` failed when the Chocolatey ccache feed returned HTTP 503 and
the old bootstrap subsequently invoked the absent command. The reviewed repair
`56e802abaaf83fd30c2e0fc471c5d72374c194ca` is integrated into main at
`7df272fcad1e3a591cef84578a4656ffcc99552c`. It probes compatible installed
ccache or explicitly selects uncached compilation, clearing stale launchers.
Twelve focused local fixtures passed. The actual local probe left zero eligible
untracked files, using the existing ignored `build.tools` location. A hosted
build verdict for the repaired candidate remains pending.

Reserved tags from unsuccessful runs remain consumed and must not be recycled.
Neither incremental release is designated as final completion evidence.

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
project regression, built UI matrices, remaining feature integration,
website deployment verification, and cleanup remain
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

- The earlier front-screen provenance candidate checks source identity but uses source-commit time and misses direct project startup routes. Build-bound metadata and an always-visible initial shell are under repair; the earlier candidate is not accepted as fulfilling the canonical provenance contract.
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

## 2026-09-18 verified superseding closeout state

This section supersedes earlier closeout notes that describe a different session boundary. The verified current `main` tip is `452c02f6f2ebb8c3aa74f6640484a0c0917d27c4`. Earlier local integration commits remain in history, and this section records the final state.

The archive used before removal is `C:\Users\cntow\OneDrive\OakKayBackups\audacity\zips\audacity-20260918T171722Z.7z`. 7-Zip read-back reported `Everything is Ok`, 4,697,455,666 bytes, 111,994 listed entries, and 1,670 Git administrative entries.

The following task-owned branches and linked checkouts were proven clean and ancestor-proven, then removed locally and from the remote: `codex/audacity-appearance`, `codex/audacity-conversion-core`, `codex/audacity-converter-containment`, `codex/audacity-experience`, `codex/audacity-front-provenance`, `codex/audacity-logo`, `codex/audacity-ollama`, `codex/audacity-packaging-integration`, and `codex/audacity-site-completion`.

The retained branches are `codex/audacity-converter-ui` at `123b3620af7103e6f8e2927e4bf1653924bc8c4f`, `codex/audacity-delivery` at `fb7c3d3bf0b16f73691916c00bd222ac85ebb43a`, `codex/audacity-profile-isolation` at `93320a5759168f6fee7995bb3c42db960a38ccfb`, `codex/audacity-release-build` at `d4b7c5faf2bb5c0bfd29e333b52937e1e489b21a`, `codex/audacity-release-candidate` at `6d6519dd1a36c0cc0ebc714bf67b5db8cc69165f`, and `codex/audacity-tests-f9dc58a` at `3934ad11c945a7ce24f6e7f2120fade4715804a2`. Every retained local tip equals its `git ls-remote` remote ref. No stash exists.

The linked checkout metadata for `codex/audacity-experience` was removed, but Windows left the exact directory `C:\Users\cntow\Documents\GitHub\gerk tong hui\audacity-codex-audacity-experience` with 8,165 files and 1,238 directories after `git worktree remove --force` reported `Filename too long`. It is not registered as a linked checkout or branch and remains an explicit filesystem cleanup blocker. No broader recursive removal was attempted.

## 2026-09-18 final closeout evidence

- `main` is pushed and verified at `452c02f6f2ebb8c3aa74f6640484a0c0917d27c4`.
- The delivery branch preservation commit is pushed and verified at
  `fb7c3d3bf0b16f73691916c00bd222ac85ebb43a`.
- The nested Muse preservation commit `4a83a1ba4` exists locally and is
  referenced by that delivery branch. Its push was rejected by the Muse remote
  with HTTP 403, so the nested commit remains unverified outside this machine.
- The required archive was created outside the checkout at
  `C:\Users\cntow\OneDrive\OakKayBackups\audacity-delivery-build-lane\zips\audacity-delivery-build-lane-20260918T174500Z.7z`.
  Read-back verification passed with `7z t` exit `0`; it contains 105,860
  files and 14,679 folders, is 7,136,227,751 bytes, and has SHA-256
  `00AC449357F37E1368F9600A5C5CDC7FD6BC3FDD28D1263C08453763FEB0CB46`.
  The archive includes the complete Git administrative directory and all
  Git-selected files from 17 worktrees. The manifest records 8,534 absent
  tracked paths as sparse-checkout omissions retained by Git history.
- No branch, worktree, or stash was removed in this pass. The remaining
  candidates are active, unfinished, user-owned, load-bearing, or ownership-
  uncertain, or depend on the unverified nested Muse ref. They remain
  preserved and documented rather than being deleted on a name-only guess.
- The uncommitted 41-file Muse state observed at the start of this pass could
  not be recovered from reflogs or unreachable Git objects after the failed
  preservation switch. Only `M3Roles.qml` survived in local commit `4a83a1ba4`.
- A concurrent closeout commit `452c02f6f2` also preserved the exact zero-byte
  file `recovery/concurrent-closeout-docs-20260918.patch`; it is included in
  the pushed `main` history and is retained as an audit marker, not treated as
  content recovery.
- The nine completed branches and linked checkouts listed in the earlier
  cleanup section are no longer present in the current local registry or
  remote branch list. The six branches listed as retained remain registered,
  pushed, and intentionally untouched because they contain unfinished or
  ownership-uncertain preservation state.

## 2026-09-18 delivery-lane closeout

- The main checkout contains five local integration commits not yet pushed to
  `origin/main`: `d48435cf91`, `0b45c656ae`, `ba55a7a4b7`, `20c41e114b`, and
  `59cbe3e712`. They integrate the appearance, converter UI, experience,
  front-provenance, and Ollama lanes. No unrelated release work was started.
- The delivery branch was preserved and pushed at
  `fb7c3d3bf0b16f73691916c00bd222ac85ebb43a`. Its only parent change records
  the surviving Muse submodule preservation commit `4a83a1ba4`.
- The Muse submodule began with an uncommitted 41-file state plus the new
  `framework/ui/qml/Muse/Ui/M3Roles.qml`. An attempted preservation switch
  caused the modified 41-file state to disappear before it was committed.
  Reflog and `git fsck --full --unreachable --no-reflogs` found no recovery
  objects. The surviving file is preserved in local Muse commit `4a83a1ba4`,
- but the Muse remote rejected its push with HTTP 403, so that nested ref is not
  remotely verified and must not be treated as complete.
- No root index conflict or unmerged index entry remains in the active
  checkout. No stash was present at inventory time. Other active or
  ownership-uncertain lane branches and worktrees are retained until their
  ownership and ancestry are proven.
- The external archive for this closeout is required before any cleanup
  removal. Its path, byte size, entry count, and verification output must be
  added below before deletion evidence is recorded.
