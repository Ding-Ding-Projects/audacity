# Personal vocabulary

Replace words in the interface with your own. The file you choose stays on this
computer, and its contents are never written to the log.

## The file

```json
{
  "schemaVersion": 1,
  "entries": {
    "track": "lane",
    "clip": "take"
  }
}
```

| Rule | Limit |
| --- | --- |
| File size | 256 KB |
| Entries | 4096 |
| `schemaVersion` | Must be `1` |
| Entry key | A non-empty string of at most 160 Unicode code units, excluding unsafe names and control characters |
| Entry value | A string of at most 1000 Unicode code units, with no unsafe control characters. It may be empty. |

Anything else is refused with a short reason. The loader checks strict UTF-8,
the 256 KB byte limit, bounded JSON depth, duplicate decoded keys, the exact
two-field root object, and every entry before it changes the active table. The
reason is shown; the words themselves are not, and are never logged. A rejected
upload leaves the last valid vocabulary active.

## How the substitution works

Substitution is applied by `ExperienceTranslator`, the same extra `QTranslator`
that bilingual mode uses, so it reaches visible interface text rather than one
particular widget.

Matches are whole words. A term that begins or ends with a letter, digit or
underscore is bounded by a look-around on both sides, so `track` matches
`Add a track` but not `Tracking` and not `backtrack`. A term made of Chinese
characters has no word boundary to speak of and is matched literally.

Longer terms are applied first, so `audio track` is never cut in half by a
shorter `audio` entry.

## Where it is stored

The parsed table is written to
`<user application data>/experience/vocabulary.json` and read again on the next
start. The original file is not copied and not watched. A validated local cache
from the earlier array format is migrated to this canonical form during startup;
that legacy shape is never accepted as a newly chosen import. Choosing another
file replaces the table; Clear removes it.

## Limits

- Substitution happens where a translation is looked up, so text drawn by the
  operating system and text already copied into a property are not affected
  until the next start.
- In Cantonese mode the substitution is applied to the Cantonese text; in
  English and bilingual mode it is applied to the English text, and in
  bilingual mode to the composed line.
