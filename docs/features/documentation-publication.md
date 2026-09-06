# Documentation publication

The canonical public address is https://ding-ding-projects.github.io/audacity/.
The publication workflow stages `docs/site` as the root, including its relative
assets and offline articles. It runs for changes on `main`, manual dispatch, and
successful completion of the release workflow so download metadata follows a release.

`node docs/site/scripts/stage-site.mjs <new-output-directory>` stages the current
source without changing tracked metadata. The output must be a new directory inside
the checkout. Node.js and authenticated GitHub CLI reads are required. Publication
uses the release credential precedence configured in the workflow.

The front screen identifies the documentation source version separately from the
downloadable application version. Its timestamp is the recorded commit timestamp,
labelled as source provenance, formatted in local time with seconds and a timezone.
It never uses launch time or a hand-entered release date. Missing or invalid data
shows an unavailable state.

The staging command selects the latest published release in the existing release
series and requires Setup.exe, RELEASES, and a full package. It records actual asset
URLs and sizes. SHA-256 values are included only when GitHub provides a valid digest;
missing digests remain unavailable. Hashes are not code-signature verification.
No Linux or macOS downloads are added to the current Windows delivery list.

Staging errors stop publication rather than shipping guessed release metadata.
This packaging check does not claim installer execution, application feature
completeness, or rendered UI verification.
