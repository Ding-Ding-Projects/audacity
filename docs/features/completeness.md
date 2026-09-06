# Completeness delivery boundary

Ordinary configure, build, and release work remains report-only and nonblocking.
`--strict` checks narrative inventory integrity. Complete delivery additionally
uses a candidate-bound evidence verifier:

```sh
python buildscripts/checks/completeness_guard.py --completion --candidate <full-audited-commit-sha>
python buildscripts/checks/test_completeness_guard.py
```

The canonical feature inventory and desktop/website matrix are narrative records,
not substitutes for evidence. The explicit `concrete-surfaces.json` registry names
24 desktop surfaces and six website surfaces, including nested preferences,
editors, dialogs, website settings, articles, and the table of contents. Routes
in this registry are required deterministic route contracts, not claims that the
current product exposes them. Unimplemented routes remain incomplete work.
The independently maintained surface constants in `completion_evidence.py` catch
a removed registry entry. New visible surfaces require an explicit reviewed
addition to both inventories. Source discovery never defines the required set.

`completion-evidence.json` contains exactly one row for each of the 39 canonical
features on each of the 30 concrete surfaces, 1,170 rows. Every row currently
remains `missing`: legacy source-level implementation claims have not been
promoted into candidate-bound delivery evidence. A feature that cannot apply
literally still needs a documented, accessible equivalent and its evidence.
A completed desktop row never discharges any website row. The legacy narrative
per-surface table permits multiple surface rows for one feature.

## Candidate binding without circular hashes

The source candidate is an explicit full SHA naming an existing Git commit.
Both JSON inventories, all three Markdown inventories, and the Cantonese catalog
must match their blobs in that exact commit. Completion cannot combine old build
evidence with optimistic edits to the current narrative files. The committed
ledger records output locations rather than post-build receipt hashes. Each row
expects `.verification/completeness/<product>/<surface>/feature-NN.json`, where
`NN` is its one-based canonical feature index. Commit the planned ledger first,
build that exact source, then produce the external descriptors and receipts.
This prevents the impossible cycle where committing a receipt changes its source
SHA. The output directory is never evidence merely because it exists, and output
files must not be committed as a claimed proof of their own containing commit.

Each descriptor carries the exact product/surface/feature key and candidate SHA.
All evidence references are bounded relative path/SHA-256 objects. They must
resolve to files inside the audited root, including after symlink resolution.
Absolute paths, traversal, arbitrary prose, missing files, and hash mismatches
are rejected. Implementation, documentation, localization, persistence contracts,
and test source also match the candidate's Git blobs. Each source role is a
`{file: {path, sha256}, id}` object with its own format, root, and exact identifier.
Desktop implementation IDs resolve to declarations under `src/`; website IDs
resolve to declarations under `docs/site/`. Documentation IDs resolve to exact
headings in `docs/features/*.md`. Localization IDs use `context::exact-source`
and resolve to a completed Cantonese message in the desktop TS catalog or a
website JSON catalog under `docs/site/locales/`. Test IDs resolve to Python test
functions, C++ test macros, or JavaScript test declarations in supported test
roots. Persistence IDs resolve to explicit versioned JSON contracts below
`docs/inventory/persistence/`. A shared source file can legitimately support
several rows, but one arbitrary text file cannot impersonate all these roles. A persistence contract
file describes the real storage location or the supported stateless equivalent.
Line endings are normalized only when comparing source blobs; evidence hashes
always cover the exact bytes.

The descriptor links a build receipt, test result, interaction ledger, PNG, and
capture receipt. The build receipt binds an actual file hash, product, candidate,
version, and recorded UTC build time to a separate version-provenance record.
Desktop files must have an `.exe` path, bounded PE executable headers and an
executable entry point, plus an RT_VERSION resource whose embedded numeric
product version agrees with the build version. A hashed text file is refused.
A separate launch receipt binds the observed executable path/hash, embedded
version, process identity and creation time to that exact build. Interaction
records reference the same launch receipt. This checks the observation record;
it does not inspect a past process or prove the producer really launched it.

Website builds use a staged-tree manifest with an HTML entry point, exact file
hashes and complete coverage of a bounded `outputs/` directory. A separate
served-identity receipt records the origin and observed response hashes for
those same files. Missing staged files and mismatched served hashes fail closed.
These product checks are structural validation, not executable or deployment
smoke tests.
Test records include uniquely named passing cases, matching structured output,
and their exact test source. The observed cases must include the exact declared
test-source ID. Interaction records link those results and contain
bounded before/target/input/expected/after steps. Capture and interaction records
must share the exact product, surface, route, state, theme, language, viewport,
and display scale. The capture receipt links the interaction and image, and its
privacy review links that same image and candidate. Test, interaction, and capture-receipt paths cannot be reused across rows.
A single image can support multiple feature rows on the same concrete surface
when each row retains a distinct interaction and capture receipt, a bounded
pixel region, and an explanatory annotation. Image content cannot be reused
across different concrete surfaces, even under a different filename. Currentness is derived from candidate, source, build, tuple,
and hash agreement, never from `current: true`.

Chronology requires candidate commit time <= actual production time <= launch
or serve time <= capture time <= current UTC time. Capture must occur within
30 days of production. The build declares `timestampMode`. In `recorded` mode,
its visible updated-at value equals actual production time. In `reproducible`
mode, that value equals its explicit integer `sourceDateEpoch`, which may
predate the candidate, while actual production and capture still obey the
chronology checks. File modification time is never treated as provenance.

The decoder verifies PNG signature, chunk CRCs, ordering, bounded dimensions,
zlib stream, scanlines, and reconstructed pixels for non-interlaced 8-bit RGB
and RGBA images. It rejects unsupported encodings explicitly. Decoded pixel
size must equal the viewport multiplied by display scale. A filename ending
in `.png` and a valid-looking SHA are insufficient.

## Evidence limits

This verifier checks structural integrity and candidate binding. It is not an
independent execution attestation, a visual quality audit, or proof that a
producer truthfully recorded execution. An owner who fabricates an internally
consistent set of files can fabricate the claims within them. SHA-256 proves
byte identity, not origin or authenticity. The owner must still inspect genuine
headless capture receipts and actual product runs before accepting delivery.
One complete tuple per feature/surface is the minimum evidence schema here;
this does not claim the complete language/theme/scale interaction matrix has
been exercised. The registry is a manually reviewed minimum, not automatic
proof that every future surface has been discovered.

Tests create a real temporary Git source commit, synthetic PE-shaped versioned bytes and staged HTML trees,
and decodable synthetic PNGs for all 1,170 combinations. None is a real product
build or UI capture. Each negative test changes one relevant boundary, often
recomputing its wrapper hash so semantic validation must reject it. Cases cover
nonexistent and valid-but-wrong source SHAs, corrupt/non-image captures, wrong
build hashes, missing version provenance, stale source/builds, mismatched tuples,
receipt reuse, arbitrary prose, website evidence omission, nested surface
removal, feature-pair removal, escaping paths, failed tests, absent interactions,
and absent privacy review. The real product completion verdict remains red.

The follow-up regressions also cover edits to every consumed narrative/catalog
input, arbitrary text reused for every source role, exact role IDs, text posing
as a PE, embedded version mismatch, observed launch mismatch, served-response
mismatch, missing staged files with updated hashes, future or premature captures,
production before the candidate, reproducible timestamp mismatch, missing image
regions and annotations, and valid image sharing within one concrete surface.
