# Dim sum surprise

## Behaviour

On roughly 10% of launches, a fresh random draw decides whether Audacity shows a small,
non-blocking card naming one random dim sum dish in English and Traditional Chinese, for
example "Shrimp dumpling · 蝦餃". The card carries the dish's catalog image when available.

The existing bundled catalog uses AI-generated illustrations, not camera-origin
photographs. The release reuses a tracked catalog image with that origin disclosed;
the image illustrates the named dish and is not photographic evidence of a meal.

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
size (16 MB for the catalog document, which is a little over 8 MB for the full 2,866-dish
catalog; 8 MB for a photo) and a 6 second timeout per request.

A GitHub release asset download always answers with a redirect from `github.com` to a
signed, time-limited URL on `objects.githubusercontent.com` or
`release-assets.githubusercontent.com`. The fetch follows at most two such redirects, and
only when the redirect target is `https` and its host is one of the exact hosts a genuine
release download can redirect through (`github.com`, `objects.githubusercontent.com`,
`release-assets.githubusercontent.com`, `raw.githubusercontent.com`); anything else is
refused rather than followed. This is deliberately narrower than an ordinary HTTP client's
redirect handling, and it is what makes a photo fetch actually able to complete at all.

The fetch also honours the desktop's own proxy configuration (an `http_proxy` or
`https_proxy` environment variable, or the platform's system proxy settings), the same way
an ordinary desktop browser or download tool would, so a machine that only reaches the
public internet through a configured proxy is not silently unable to fetch anything here.

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
`DimSumDraw` instance). `DimSumSurpriseServiceTests` exercises the redirect allowlist
(`DimSumSurpriseService::isAllowedRedirectTarget`) directly: the exact allowed hosts over
`https`, an unlisted host, plain `http` even on an allowed host, and an invalid URL.

The redirect-following fetch itself was verified live: `curl -sIL` against a real published
`catalog-v1` release asset confirmed the exact one-hop redirect chain
(`github.com` → `release-assets.githubusercontent.com`), and the built application, run
under Xvfb with `AU_DIM_SUM_FORCE=1` and a fresh application data directory, was observed to
fetch and cache a catalog image through that same redirect chain (a valid 1254x1254 PNG,
2.3 MB) and then display it in the card. The full flow is captured in
`docs/design/captures/lane-k2/08-dimsum-card-real-photo.png`.

That filename and the Xvfb record are historical. They do not establish camera
origin or current Windows runtime verification.
