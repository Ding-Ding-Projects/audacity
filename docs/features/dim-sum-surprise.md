# Dim sum surprise

## Behaviour

On roughly 10% of launches, a fresh random draw decides whether Audacity shows a small,
non-blocking card naming one random dim sum dish in English and Traditional Chinese, for
example "Shrimp dumpling · 蝦餃". The card carries the dish's photo when one is available.

The draw happens once per launch and is never repeated within that launch. It is never
shown on the very first run, during an error path, or while a dialog or a background task
is active. The card auto-dismisses after a short delay and never steals keyboard focus or
blocks startup.

There is no setting that turns this off. It is a small delight, not a feature to manage.

## Data source

Dish names and metadata come from the public catalog at
`https://raw.githubusercontent.com/Ding-Ding-Projects/dim-sum-photos/main/catalog/index.json`.
Photos are fetched only from that project's published `catalog-v1*` release assets. This
repository never bundles, vendors, or generates dim sum photos; the fetch and cache happen
entirely on the native side, into the application data directory, with a bounded response
size (2 MB), a 6 second timeout, and manual redirect handling (no redirects are followed).

## Offline behaviour

When the catalog or the selected photo cannot be fetched, the card still shows the dish
name (from whatever was cached on a previous successful fetch, or is skipped entirely if
nothing has ever been cached) with an honest "photo unavailable offline" placeholder
instead of a broken image.

## Accessibility

The card's image carries alt text naming the dish. The card respects the reduced motion
setting: with reduced motion on, it appears and disappears without an animated transition.

## Funny levels and language modes

The card's surrounding copy (for example, a short line introducing the surprise) is styled
by the active funny level and language mode. The dish's own name is never altered, translated
loosely, or made playful; it is shown exactly as the catalog records it, in both languages.

## School mode

While School mode is on, the dim sum surprise is fully suppressed, as if the capability
were not installed at all: no draw happens, and no card can appear.

## Security and privacy

The only network requests this capability makes are a GET to the public catalog URL and a
GET to a public release asset URL, both over HTTPS, both read-only. No user data is sent.
Nothing is written outside the application data cache directory.

## Verification

Covered by `DimSumCatalogTests` and `DimSumDrawTests` in
`src/experience/tests/dimsumsurprise_tests.cpp`, which exercise catalog parsing (valid
document, an entry missing either name, malformed JSON) and the draw itself (drawing true
under the threshold, false at or above it, and never drawing a second time within one
`DimSumDraw` instance).
