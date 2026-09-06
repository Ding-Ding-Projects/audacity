# Language modes

## Documentation website

The website has its own local setting, `ma.settings.v1.language`, with `en`,
`yue`, and `bilingual` values. It does not depend on the installed desktop
application. The maintained `locales/yue-HK.json` catalog and its checked browser
copy provide authored interface translations. `js/presentation.js` composes
language-labelled parts; the page inserts them as text, never HTML.

English remains the document's default language. Translated Cantonese spans
carry `lang="yue"`, and bilingual copy places each language in a separate span.
Untranslated original text, exact release records, and documentation article
content remain English. These retained articles are a known localization gap,
not evidence of complete Cantonese documentation.

Parameterized interface messages use bounded catalog templates. Their dynamic
values remain literal data and are excluded from personal-vocabulary changes.
The original English source stays in memory for reversible language changes;
replacement output never becomes the source for another language. Personal
vocabulary applies locally after translation to authored wording. English and
Cantonese feedback levels are independent; control labels stay direct.

Local verification uses `node --test docs/site/scripts/presentation.test.cjs
docs/site/scripts/personal-vocabulary.test.cjs`. The current suites cover the
browser/JSON catalog match, every catalog string in all three modes, feedback
levels, missing-catalog fallback, exact provenance values, bounded templates,
and vocabulary parsing/replacement. These source tests do not prove browser
layout, screen-reader behavior, focus, or the complete interaction matrix.

## Desktop application

Material Audacity offers exactly three language modes. The setting is
`experience/language/mode` and it is persisted through muse settings.

| Mode | Value | What is shown |
| --- | --- | --- |
| English | 0 | The English catalogue, unchanged. |
| Cantonese (Hong Kong) | 1 | `share/locale/audacity_yue_HK.ts`, the playful Hong Kong written Cantonese translation. |
| Bilingual | 2 | English and Cantonese together. |

The mode lives on the Companion page in Preferences.

## English and Cantonese

These two modes set the muse language service to `en_US` and `yue_HK`. The
setting the service reads is `languages/language`, so the language dropdown in
General preferences and the companion mode stay in step.

## Bilingual

Bilingual mode is not a translation file. It is a C++ `QTranslator` subclass,
`au::experience::ExperienceTranslator`, installed on top of the translators the
language service installs. For every string it is asked about it looks the
Cantonese translation up in the `yue_HK` catalogues it loaded itself and
returns

```
English / 廣東話
```

The composition rule is `ExperienceTranslator::compose()`:

- when there is no Cantonese translation, the English text is returned
  unchanged;
- when the Cantonese translation is identical to the English source, which is
  what happens for product and format names such as `MP3` or `VST3`, the text
  is shown once;
- otherwise both are shown, separated by ` / `.

Returning an empty string from `translate()` makes Qt fall through to the
translator underneath, so the module stays out of the way whenever it has
nothing to add.

### Limits of the bilingual mechanism

These are real limits, not temporary gaps:

- **Only catalogued strings are composed.** A string that has no entry in
  `audacity_yue_HK.ts` is shown in English only.
- **Only Qt translations are composed.** Text drawn by the operating system,
  such as the native file picker and the standard platform buttons, and text
  that was computed in C++ and copied into a property once, are outside the
  translator.
- **Layout.** Two languages take roughly twice the width. Labels that are
  tight in one language wrap in bilingual mode.
- **The Cantonese catalogue must be present.** When it cannot be found, the
  Companion page says so and bilingual mode shows English only.

## Live switching and the restart banner

Switching the mode calls `QQmlEngine::retranslate()`, which re-evaluates every
binding written with `qsTrc`, so most of the interface changes immediately.
muse itself still declares that a language change needs a restart, and the
platform text and any already-copied strings do not follow. The Companion page
says exactly that rather than claiming a full live switch.
