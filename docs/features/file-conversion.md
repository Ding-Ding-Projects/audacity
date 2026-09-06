# Local file conversion core

This first backend slice adds `src/converter`, an unregistered Qt module for a
future local file-converter surface. It does not provide a user interface,
application registration, translations, packaged-artifact proof, PDF tools,
audio conversion, or video conversion.

## Current adapter catalog

The catalog is categorized and deliberately honest. Its current functional
scope is Qt image plugin conversion among PNG, JPEG and BMP, only when the Qt
plugins in the running build report both source decoding and target encoding
support. PDF, audio, video, archives, structured data, code/text and binary
encodings remain visible as disabled adapters with a precise reason.

The module never searches `PATH`, launches command-line tools, runs scripts,
uses a network service, or treats a developer-machine installation as bundled
capability. A format is enabled only when the Qt plugin reports availability in
the running build.

## Safety boundaries

- Source type detection reads bounded source bytes and does not trust a file
  extension.
- Input size is limited to 256 MiB and decoded image dimensions to 100 million
  pixels. Malformed or oversized sources are rejected before output.
- The source is never modified.
- A destination that already exists is preserved unless the caller explicitly
  selects overwrite. This standalone core still refuses explicit overwrite,
  because its application integration must supply a verified Windows-safe
  atomic replacement service rather than deleting the old file first.
- New outputs are encoded in a private temporary file in the destination
  directory, reopened and dimension-verified, then renamed into a previously
  absent destination. Failed or cancelled conversions publish no output.
- Queue persistence records only versioned item state, paths, target format and
  the caller's overwrite choice. It never stores input or output file bytes.
  Queue reads are paged and capped to 500 rows per call.

## Parent integration required

The application integration lane must register `src/converter/CMakeLists.txt`
from the root build, provide QML and localized user-facing copy, add the
adapter catalog search and its regex builder, bind progress and cancellation to
a bounded-concurrency worker, add storage-capacity preflight, package and prove
the Qt image plugins, provide a Windows-safe overwrite transaction, and run
the module tests through the project build.

The visible converter must also implement the remaining product contract:
document and PDF operations, batch handling with durable crash recovery,
history and exports, accessibility and responsive behavior, notifications,
command-palette routing, real built-artifact interaction, and captures.

## Focused tests

`src/converter/tests/conversionengine_tests.cpp` covers corrupt byte input,
existing-destination preservation, verified output, pre-start cancellation,
queue restart and persisted cancellation. The image conversion test skips only
when the current Qt build lacks the necessary bundled image capability, which
is the same condition that disables the adapter in the catalog.
