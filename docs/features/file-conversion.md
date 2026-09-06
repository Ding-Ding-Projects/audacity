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

## Native transaction boundary

On Windows, this core accepts absolute drive paths on fixed local NTFS volumes.
It rejects relative paths, device and network paths, alternate data streams,
reserved DOS names, ambiguous trailing dots/spaces, and every reparse point in
both source and destination ancestor chains. Unsupported filesystem semantics
fail closed; no copy-and-delete fallback is attempted.

Each ancestor is opened with `FILE_FLAG_OPEN_REPARSE_POINT` and held without
write or delete sharing. The opened root is resolved to its volume GUID so a
subsequent drive-letter remapping cannot redirect the transaction. The source
is opened once without write or delete sharing, checked by handle for regular
file type, size and identity, and decoded through that same bounded QIODevice.
Hard-link identity, case aliases and existing destinations are refused.

The temporary output uses an exclusive `CREATE_NEW` handle in the pinned
parent. Encoding and full decode verification use that same handle. The output
is bounded to 512 MiB. `FlushFileBuffers` precedes
`SetFileInformationByHandle(FileRenameInfo)` with `ReplaceIfExists = FALSE`.
The native rename is the atomic create-if-absent commit: a destination created
at any earlier point wins, including a link, and its bytes are preserved.
On cancellation or failure the unpublished temporary is removed by its own
handle, never by a filename that another process could substitute.

The implementation protects ordinary filesystem namespace races, not privileged
kernel tampering or writes through a mapping created before the source handle
was opened. Cancellation is checked during bounded IO and immediately before
commit; cancellation arriving after the atomic commit does not roll it back.
Decoder execution remains in-process and has no hard CPU deadline or process
memory sandbox. Process isolation and crash-orphan recovery remain integration
work. A process crash can leave an unpublished temporary, never an overwritten
original. Explicit overwrite is still unavailable, not a completed feature.

`src/converter/tests/standalone` builds a real Qt console transaction executable
with deterministic test-only barriers. It exercises source and destination
reparse paths, hard links and case aliases, destination creation races, blocked
source/directory/temporary substitutions, input/output limits, malformed images,
and cancellation at source, output and commit boundaries. The executable fails
if required image plugins or filesystem fixtures are unavailable; it does not
count refusal to convert as success. These tests prove this backend on the
host, not the full application or a packaged installer.
