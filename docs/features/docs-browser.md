# In-app documentation browser

The toolkit module bundles every feature article under `docs/features` into
its own Qt resources at build time, and renders them inside the application
through `DocsBrowserPage`, so the documentation works fully offline.

## Behaviour

- Every Markdown file under `docs/features` is packed into the module's
  resources automatically at configure time; nothing needs to be added by
  hand to a manifest.
- A search field, backed by the shared regular expression builder, searches
  both article titles and bodies.
- Article to article links resolve inside the browser by article id, rather
  than opening a system browser or dead-ending.
- Each article ends with a short list of suggested articles.
- A CMake configure-time check compares the article files present on disk
  against the generated resource manifest and fails configuration if any
  article on disk did not make it into the bundle.
- Any article can be bookmarked from a star toggle beside its row in the
  article list. Bookmarks are backed by `BookmarkModel`, a small
  `QAbstractListModel` that stores an id, the bookmarked article's id, and
  an editable title (defaulting to the article's own title, but renameable
  independently of it). Bookmarks persist in a JSON file under the
  application's user data directory, in a `toolkit` subdirectory, as
  `docs-bookmarks.json`.
- A bookmarks list is shown below the article list whenever there is at
  least one bookmark, with its own `BulkSelectionController` (multi-select,
  select all on page, select all matches, invert, clear) scoped to the
  bookmark count rather than the full article count. Its "Export selected"
  action opens the shared `ExportSheet`, and its "Remove bookmark" action
  removes every selected bookmark through `BookmarkModel.removeMany`.
- Clicking a bookmark's row opens that article, exactly like clicking it in
  the main article list.

## Configuration

The article bundle is fixed at build time from the repository's own
documentation. Bookmarks are the one piece of user configurable state this
feature has, and they live entirely in the local `docs-bookmarks.json` file
described above; there is no separate settings surface for them.

## Failure modes

An article whose id cannot be resolved (a stale link) is simply not opened;
the browser keeps showing the previously open article rather than a blank
page. A bookmark whose article id no longer resolves to a real article (for
example, an article was renamed since the bookmark was created) still shows
its stored title and can still be removed; opening it shows the browser's
own "choose an article" empty state rather than a blank page.

## Security considerations

The browser renders local, repository-authored Markdown only; it never
fetches or renders remote content. Bookmarks contain only an article id and
a title the user chose; nothing sensitive is stored, and the export path
writes plain JSON, CSV, or another chosen format through the same shared
export service every other list in the application uses.

## Verification

The CMake completeness check itself is the primary guard: deleting an
article from `docs/features` without removing its reference elsewhere, or
adding one without it appearing in the generated resource list, fails the
build. `docsindex_tests.cpp` additionally covers title-and-body search and
the suggested-articles list excluding the current article.
`bookmarkmodel_tests.cpp` covers adding, idempotent re-adding, removing by
article id, toggling, renaming (which changes only the title, not the
article id), bulk removal by row indexes regardless of order, and the shape
of the rows handed to the export service.
