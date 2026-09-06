# Documentation publication

The canonical public address is https://ding-ding-projects.github.io/audacity/.
The publication workflow stages `docs/site` as the root, including its relative
assets and offline articles. It runs after successful completion of the release
workflow on `main`, or by manual dispatch. A completed release checks out its exact
source SHA and requires a published release tag resolving to that SHA. Source pushes
do not start a competing documentation publisher. Publication runs are serialized
and never cancel an in-progress deployment.

`node docs/site/scripts/stage-site.mjs <new-output-directory>` stages the current
source without changing tracked metadata. The output must be a new directory inside
the checkout, with a name beginning `build.site`. Nested destinations, Git metadata,
symbolic links, and existing destinations are rejected. Node.js and authenticated GitHub CLI reads are required. Publication
uses the release credential precedence configured in the workflow.

The front screen identifies the documentation source version separately from the
downloadable application version. Its timestamp is the recorded commit timestamp,
labelled as source provenance, formatted in local time with seconds and a timezone.
It never uses launch time or a hand-entered release date. Missing or invalid data
shows an unavailable state.

Manual dispatch deliberately pairs its selected immutable documentation revision
with the latest published release in the existing release series; the two version
identities are separate. Prereleases are supported and visibly labelled.

The staging command requires Setup.exe, RELEASES, SHA256SUMS, and the selected
release's current full package, with only that version's optional delta package.
Historical packages attached for updater compatibility are not presented as current
downloads. It records actual asset
URLs and sizes. SHA-256 values are included only when GitHub provides a valid digest;
missing digests remain unavailable. Hashes are not code-signature verification.
No Linux or macOS downloads are added to the current Windows delivery list.

Staging errors stop publication rather than shipping guessed release metadata.
This packaging check does not claim installer execution, application feature
completeness, or rendered UI verification.
