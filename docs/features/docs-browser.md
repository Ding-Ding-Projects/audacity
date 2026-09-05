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

## Configuration

None; the bundle is fixed at build time from the repository's own
documentation.

## Failure modes

An article whose id cannot be resolved (a stale link) is simply not opened;
the browser keeps showing the previously open article rather than a blank
page.

## Security considerations

The browser renders local, repository-authored Markdown only; it never
fetches or renders remote content.

## Verification

The CMake completeness check itself is the primary guard: deleting an
article from `docs/features` without removing its reference elsewhere, or
adding one without it appearing in the generated resource list, fails the
build. `docsindex_tests.cpp` additionally covers title-and-body search and
the suggested-articles list excluding the current article.
