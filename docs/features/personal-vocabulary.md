# Personal vocabulary

Replace words in the interface with your own. The file you choose stays on this
computer, and its contents are never written to the log.

## The file

```json
{
  "version": 1,
  "entries": [
    { "from": "track", "to": "lane" },
    { "from": "clip", "to": "take" }
  ]
}
```

| Rule | Limit |
| --- | --- |
| File size | 256 KB |
| Entries | 2000 |
| `version` | Must be `1` |
| `from` | A non-empty string, unique in the file |
| `to` | A string, may be empty |

Anything else is refused with a short reason: the file is not valid JSON, the
top level is not an object, only version 1 is supported, there is no entries
array, an entry is not an object, an entry lacks a text `from` or `to`, a
`from` is empty, the same `from` appears more than once, or there are more than
2000 entries. The reason is shown; the words themselves are not, and are never
logged.

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
start. The original file is not copied and not watched. Choosing another file
replaces the table; Clear removes it.

## Limits

- Substitution happens where a translation is looked up, so text drawn by the
  operating system and text already copied into a property are not affected
  until the next start.
- In Cantonese mode the substitution is applied to the Cantonese text; in
  English and bilingual mode it is applied to the English text, and in
  bilingual mode to the composed line.
