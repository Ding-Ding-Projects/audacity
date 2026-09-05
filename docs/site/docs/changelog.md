# Changelog and "What's new"

`CHANGELOG.md` at the repository root is the release facing changelog. The
application renders it in the **Help ▸ What's new** dialog.

## The format

```markdown
## 4.0.0-material.1 - 2026-09-05

### Added

- Material 3 token engine, component library, fonts and gallery
  ([`9e500daf17`](https://github.com/Ding-Ding-Projects/audacity/commit/9e500daf17c1f55f928c92d84090a335be253c97))
```

- A level two heading starts a release: the version, then `-`, then the date in
  ISO form.
- A level three heading names a group.
- Every list item ends with a link. The link text is the abbreviated hash for
  the reader; the link **target** carries the full forty character hash. That
  is the hash the application stores and the build time check verifies.

## The dialog

- One block per released version, newest first.
- A date range, so a reader can ask what changed in a period rather than in a
  release.
- An `M3SearchBar` with `showRegexBuilder: true`. The term is used as a regular
  expression when it compiles as one and as plain text otherwise.
- Every entry carries a button showing the abbreviated hash. Its tooltip is the
  full hash and the address, and activating it opens
  `https://github.com/Ding-Ding-Projects/audacity/commit/<full sha>`.
- Export writes the filtered changelog as Markdown, JSON or HTML through a file
  dialog. Every export carries the full hashes, not the abbreviations.

## The build time check

`buildscripts/cmake/ValidateChangelog.cmake` provides `au_validate_changelog`,
called from the top level `CMakeLists.txt`. It collects every forty character
hexadecimal string in `CHANGELOG.md` and runs `git cat-file -e <sha>^{commit}`
for each one. A hash that is not in the repository fails the configure step
with the list of missing hashes.

A changelog that points at a commit nobody can reach is worse than no
changelog, which is why this is a hard failure rather than a warning.

## Failure modes

| What goes wrong | What happens |
| --------------- | ------------ |
| `CHANGELOG.md` is absent | The check warns and passes; the dialog says the build carries no changelog |
| `.git` is absent, for example in a source tarball | The check warns that there is nothing to verify against and passes |
| `git` is not installed | The check warns and passes |
| A hash in the changelog is not in the repository | Configure fails, naming every missing hash |
| A filter matches nothing | The dialog says so rather than showing an empty page |

## Accessibility

- The dialog is a muse `StyledDialogView` through `M3Dialog`, so it joins the
  application's dialog stack, escape handling and navigation sections.
- The search field, the two date fields and the clear button share one vertical
  navigation panel.
- Each commit button carries an accessible name naming the full hash, so a
  screen reader announces which commit is being opened rather than only the
  abbreviation on screen.

## Verification

`src/chronicle/tests/changelog_tests.cpp` asserts the parser against a sample
changelog: releases, dates, groups, and that every entry carries a full forty
character hash rather than the abbreviation shown in the link text. It also
asserts the date and search filters, the plain text fallback for a term that is
not a valid regular expression, and that all three export formats carry the
full hashes.
