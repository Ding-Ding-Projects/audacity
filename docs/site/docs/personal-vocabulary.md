# Personal vocabulary

Settings always provides a local JSON picker, load status, and Clear action. Without a valid file the original interface wording remains unchanged.

```json
{"schemaVersion":1,"entries":{"Home":"Start"}}
```

This neutral example is documentation, not a built-in replacement table. Limits are 256 KiB of UTF-8, 4096 entries, 160 Unicode characters per key and 1000 per value. Keys must be nonempty. Unsupported versions, duplicate decoded keys, unexpected fields, nested values, invalid UTF-8, unsafe object keys, and prohibited control characters are rejected before storage or application.

Validation, replacement, and the validated cache stay in this browser. The picker does not upload anything. File names, source paths, contents, and entry counts are not included in history or notifications. A replacement that cannot be validated or stored leaves the last valid table active. A corrupt cache restores original wording and reports unavailable saved vocabulary.

Replacement operates on text nodes and accessible labels, never on HTML, styles, links, script source, input values, or DOM identifiers. Longer literal matches win; word boundaries avoid replacing pieces of other words, and replacements do not cascade. Expansion is bounded. Technical code, release provenance and asset records, and document article contents retain original wording. Search also recognizes customized labels without storing them in public metadata.

Clear invalidates pending file reads, removes the local cache and restores original text and accessible labels. If storage refuses the clear, the control reports failure instead of claiming success. This loader never exports or transmits private values or file metadata.

Run focused tests with `node --test docs/site/scripts/personal-vocabulary.test.cjs`. These do not replace real file-picker, reload, replace, clear, accessibility and no-network checks in the built website.