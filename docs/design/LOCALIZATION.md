# Localization: language modes and the Cantonese (Hong Kong) baseline

## Language modes

Material Audacity plans to offer three language modes:

1. **English** — the existing `share/locale/audacity_en.ts` / `audacity_en_GB.ts`
   baseline, unchanged.
2. **Cantonese (Hong Kong)** — `share/locale/audacity_yue_HK.ts`, a playful,
   colloquial Hong Kong-style written Cantonese translation (Traditional
   characters). This is the file added by this change.
3. **Bilingual** — a future display mode that shows the English string and the
   Cantonese string together (e.g. stacked or side-by-side) rather than a
   separate `.ts` file. It is implemented at the UI layer by loading both the
   `en` and `yue_HK` catalogs at once and rendering both strings for a label;
   it does not require new translation content beyond what is in
   `audacity_yue_HK.ts`.

## Tone contract

The Cantonese translation is playful and a little cheeky, but it never changes
what the software actually does or says. Concretely:

- Every number, unit, keyboard shortcut, file pattern, placeholder (`%1`,
  `%2`, `%n`), `&` accelerator marker, and HTML tag in a `<source>` string is
  preserved exactly in the `<translation>`.
- Well-known product and technical/format names are kept untranslated:
  Audacity, VST3, LV2, MP3, FLAC, dB, Hz, kHz, WAV, OGG, MIDI, Nyquist, and
  similar terms.
- Playfulness lives only in phrasing and grammar — natural colloquial
  Cantonese particles and sentence patterns (唔, 係, 喺, 嘅, 咗, 啲, 冇, 點樣,
  呢個, and similar), a friendly and lively register, no emoji — never in the
  facts being communicated.
- Menu and button labels stay short, matching the length and register of
  their English counterparts so layouts do not break.

## Funny-level 1-5 (future work)

The product plan calls for a "funny-level" setting from 1 (driest,
closest to literal/standard usage) to 5 (most playful/cheeky Hong Kong
colloquial flavor). `audacity_yue_HK.ts` as added in this change is the
**level 3 baseline**: clearly colloquial and friendly Cantonese, but not
pushed to maximum slang. Future levels are expected to be generated as
alternate translation sets (or runtime phrase substitutions layered on top of
this baseline) that:

- **Level 1-2**: reduce colloquial particles, lean closer to formal/standard
  written Cantonese or Traditional Chinese phrasing, for users who want a
  plainer tone.
- **Level 4-5**: increase colloquial density, idiom, and cheekiness, while
  still going through the same tone contract above — facts, numbers, units,
  shortcuts, and technical terms never change with the funny-level.

This baseline file does not itself implement the funny-level slider; it is
the source-of-truth level-3 wording that other levels are expected to be
derived from.

## Running the coverage check

`buildscripts/tools/check_translation_coverage.py` parses any Qt Linguist
`.ts` file and reports:

- total messages
- translated
- unfinished (`type="unfinished"`)
- empty (missing or blank `<translation>`)
- placeholder mismatches (differing `%1`/`%2`/`%n` counts between `<source>`
  and `<translation>`)

Usage:

```sh
python3 buildscripts/tools/check_translation_coverage.py share/locale/audacity_yue_HK.ts
python3 buildscripts/tools/check_translation_coverage.py share/locale/audacity_yue_HK.ts --strict
```

With `--strict`, the script exits non-zero if there is any unfinished, empty,
or placeholder-mismatched message — suitable for CI gating on a translation
file.

To additionally verify the file compiles cleanly with Qt's own tooling:

```sh
lrelease share/locale/audacity_yue_HK.ts -qm /tmp/yue.qm
```

A clean run reports `N finished and 0 unfinished` translations for the file's
message count.
