# Local file conversion core

This backend slice adds `src/converter`, an unregistered Qt module for a future
local file-converter surface. It does not provide a user interface, application
registration, translations, packaged-artifact proof, full PDF product integration, audio
conversion, or video conversion.

## Current adapter catalog

The current functional scope is Qt image plugin conversion from PNG to JPEG or
BMP, and from JPEG or BMP to PNG. Each pair requires the running Qt plugins to
report both decoding and encoding support. Audio, video, archives,
structured data, code/text and binary encodings remain visible as disabled
adapters with precise reasons.

The PDF catalog entry is enabled only when the packaged `converter-tools/qpdf/qpdf.exe`
and all nine required DLLs match the independent SHA-256 inventory in
`buildscripts/converter-tools/qpdf.lock.json`. The runtime inventory is mirrored
in `src/converter/qpdfbundle.h`, with an exact equality regression. The bootstrap
checks every component on warm installations and after extraction of the pinned
official qpdf 12.3.2 archive. It
never searches `PATH` or accepts a developer tool. The adapter runs fixed qpdf
arguments only, has a shared 60-second operation deadline, caps source and output files,
rejects all encrypted inputs without accepting credentials, streams into a private temporary file,
reopens it with `--check` and `--show-npages`, and never overwrites an existing
destination. qpdf currently covers inspect, merge, extract, reorder, rotate,
and split requests. Metadata uses qpdf's official JSON update format and only
permits bounded Title, Author, Subject, and Keywords values.
It creates the document information dictionary when absent and preserves fields
not included in the update. Unicode values are passed through qpdf's JSON format.

Each split request supplies a positive integer page-group size in `pageSpec`.
Split results use zero-padded source-page ranges in their names and return one
ordered output record with the exact page count for each committed file.
Extract and reorder accept positive page numbers, ranges, commas, and repeated
pages. Merge preserves both source-list and within-source order. Rotation adds
90, 180, or 270 degrees to all pages. Inspect and all non-merge operations require
exactly one source rather than silently ignoring additional inputs.

## Reproducible qpdf provisioning

`download-dependencies.bat` provisions qpdf before the other build dependencies.
Its optional `/qpdf` mode runs only that same provisioning step, without changing
the source, compiler, or Qt requirements of a complete application build. The
verified archive cache is `build.tools/downloads`; the development bundle lives
at `build.tools/converter-tools/qpdf`. `build.bat` invokes the same bootstrap after
`cmake --install`, targeting `build.install/bin`. The installed executable therefore
finds `build.install/bin/converter-tools/qpdf/qpdf.exe` through its application
directory. A missing archive is fetched from the pinned official URL and hashed
before extraction. Cache hits are independently hashed again, not trusted through
a receipt alone. Hashing uses framework cryptography directly, without relying
on child PowerShell command auto-loading. Both entry points fail if qpdf provisioning fails.

`bootstrap-qpdf.ps1` calls the shared `qpdfbootstrap.psm1` implementation. An
exclusive destination lock serializes every warm check, recovery, staging, and
activation. The archive cache has its own lock, so different destinations cannot
race one cache download. Lock acquisition is bounded to 30 seconds. All ten files
are copied and verified in a sibling staging directory; live components are
never overwritten one by one. A flushed, bounded activation journal records exact
transaction-owned sibling names before the old directory is renamed aside and
the staged directory is renamed into place. Each directory rename is atomic;
the two-rename transition may briefly have no active directory, so this is not a
zero-downtime update protocol. Runtime bundle validation fails closed during that
interval. Native sharing violations receive at most ten 100 ms rename attempts.

Ordinary activation errors immediately restore a prior bundle that still matches
the independent current pins. Hard interruption leaves the same journal for the
next invocation, which prefers the verified prior bundle over the staged candidate.
For an interrupted first installation with no prior bundle, a verified completed
stage can be activated. Malformed journals or absent verified recovery candidates
fail closed. Invalid candidates are quarantined, previous bundles and completed
journals are retained, and no provisioning path recursively deletes these records.
Failed stages are also retained for diagnosis rather
than mistaken for disposable user paths. Their later removal is an explicit build
cache maintenance task. Recovery currently covers the pinned qpdf 12.3.2 inventory;
a different-version migration needs that version's independently trusted inventory.

The bootstrap test executable script, `test-qpdf-bootstrap.ps1`, uses fresh
directories, the real official archive, three-component interruption, synchronous
rollback, process termination at both rename boundaries, component corruption,
bounded lock contention, and two simultaneous bootstrap processes. Test fault
barriers are module API inputs used by that script; the production command-line
wrapper does not accept or load them.

The module does not search `PATH`, launch arbitrary command-line tools or scripts, or use
network services. Runtime plugin presence is not packaged-application proof;
the application integration must validate bundled plugin provenance before
presenting these adapters as installed-product capabilities.

## Native file transaction

On Windows, the core accepts absolute drive paths on fixed local NTFS volumes.
It rejects relative paths, device and network paths, alternate data streams,
reserved DOS names, ambiguous trailing dots/spaces, and reparse points in both
source and destination ancestor chains. Unsupported filesystem semantics fail
closed; no copy-and-delete fallback is attempted.

Every ancestor is opened with `FILE_FLAG_OPEN_REPARSE_POINT`, checked through
its directory handle, and held with `FILE_LIST_DIRECTORY` access and without
delete sharing. Read and write sharing are necessary because native rename
opens the destination directory for writing. Attribute-only opens do not
participate in ordinary sharing enforcement, so sharing alone is not the
reparse defense. Every subsequent directory or file handle must resolve to its
expected volume-GUID path. An empty parent redirected before temporary creation
produces a mismatched handle that is rejected and removed by that handle. Once
the exclusive child is open, its parent remains nonempty, and NTFS refuses to
turn it into a reparse point. Ancestor deletion/renaming remains blocked by the
open directory handles. Volume-GUID paths prevent drive-letter remapping from
redirecting later operations.

The source is opened once with `FILE_FLAG_OPEN_REPARSE_POINT` and without write
or delete sharing. The opened handle determines regular-file type, input size,
and file identity. Signature inspection and image decoding use the same bounded,
seekable QIODevice. Source identity, size and last-write metadata are checked
again before publication. The source is never opened for writing. Case aliases
and hard links identifying the source are refused as destinations.

All existing destinations are preserved, including requests that explicitly
approve overwrite. Safe overwrite is still unavailable in this core and is
not a completed feature.

A random temporary output is created with exclusive `CREATE_NEW` access in the
pinned parent. Encoding and full decode verification use that same handle, with
no path reopen. `FlushFileBuffers` precedes the atomic commit:
`SetFileInformationByHandle(FileRenameInfo)` with `ReplaceIfExists = FALSE`.
A destination introduced during conversion wins, including a link, and is
preserved. The result never falls back to copying over the target. Cancellation
or failure marks the unpublished temporary for deletion through its own handle,
never through a filename that another process could substitute. A filesystem IO
failure can prevent deletion; crash-orphan recovery remains integration work.

The documented native contracts are
[FILE_RENAME_INFO](https://learn.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-file_rename_info),
[SetFileInformationByHandle](https://learn.microsoft.com/en-us/windows/win32/api/fileapi/nf-fileapi-setfileinformationbyhandle),
and [reparse-point validation](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-fsa/4aeefef8-92c3-4abc-af7a-a610caf8a165).
The implementation does not invoke undocumented native system calls.

PDF subprocess readers require a different sharing transition from the image
decoder. The output writer is first flushed, while an exact-file read pin keeps
the object and path alive. The writer is then replaced through `ReOpenFile` by
a read-only handle. One handle blocks writes and another blocks renames while
qpdf validates the path. Publication and unpublished deletion acquire access
through `ReOpenFile` on that held object, never by reopening a potentially
replaced filename. qpdf receives the equivalent `\\.\Volume{...}` device path
because its wildcard expansion misinterprets the question mark in `\\?\Volume{...}`.
Source files and all ten verified bundle components stay pinned for the entire
request. Existing mappings and privileged modifications remain outside this boundary.

Split publication is intentionally incremental. If a later destination collides
or cancellation arrives, already committed results are retained and reported
with `ok = false`, their exact paths, page counts, and `committed = true`.
The adapter never rolls back by deleting a published pathname. Such a path may
already hold a user's replacement by the time a later split fails. A committed
record describes the publication event, not perpetual ownership of the path.

## Resource and cancellation boundaries

Input is limited to 256 MiB, decoded dimensions to 100 million pixels, and
encoded output to 512 MiB. Invalid signatures, malformed dimensions and truncated
images are rejected. Source and temporary reads use bounded chunks. Cancellation
is checked during IO and immediately before commit. A cancellation that arrives
after the atomic commit does not undo the completed output.

The image decoder remains in-process, without a hard CPU deadline or process memory
sandbox. The file transaction does not protect against privileged kernel
tampering or writes through a mapping created before the source handle opened.
This is a backend namespace-race boundary, not a complete decoder sandbox or a
claim of power-loss durability. A process crash may leave an unpublished
temporary; originals are not overwritten by the create-if-absent commit.

PDF execution uses a Windows job assigned during child creation, before the
child executes. It limits the job to one process, bounds process committed memory
to 512 MiB, kills the child on job closure, and refuses launch if these controls
cannot be installed. Children are hidden and receive only a bundled-tool `PATH`
and the actual `SystemRoot`, rather than inheriting the caller's environment; their
working directory is the verified bundled tool directory. The shared deadline
includes all qpdf calls in one request, with bounded startup and termination
waits of five seconds each. Cancellation reaches inspection and output validation
as well as transformation. Standard output streams to a bounded file handle;
diagnostic/query output is capped at 1 MiB and parser diagnostic content is not
reflected into user-visible messages. Sources are limited to 256 MiB, each output
to 512 MiB, inspected documents to 10,000 pages, merge requests to 32 sources,
and split requests to 1,000 outputs. Metadata JSON shares the 1 MiB query-output
cap. This is resource containment, not a security sandbox: the child retains
the launching account's access rights. Source pinning and no-overwrite publication
do not establish resistance to a compromised operating system or kernel.

Queue persistence records versioned item state, paths, target format and the
caller's overwrite choice, without input or output bytes. Queue reads are paged
and capped to 500 rows per call.

## Parent integration required

Application integration must register `src/converter/CMakeLists.txt`, provide
QML and localization, add adapter searches and their regex builders, bind
progress/cancellation to bounded workers, provide storage-capacity preflight,
prove bundled Qt plugins, implement safe approved overwrite, and integrate
process isolation, execution limits and durable crash recovery.

The full converter still requires packaged-artifact proof of the qpdf
distribution and full application integration. It also requires audio/video and other
category adapters, batch history/exports, accessibility, responsive behavior,
notifications, command-palette routing, and real packaged-application
interaction and capture evidence. These are not implemented by this backend.

## Focused verification

`src/converter/tests/standalone` builds three real Qt console executables:

- `converter_core_smoke`: successful JPEG conversion and full reopen, source
  collision, cancellation, queue persistence and restart.
- `converter_native_transactions`: independently reported deterministic cases
  for signature detection, file identity, symlinks, directory junctions,
  destination insertion, source and temporary substitution, attribute-only
  directory redirection, native IO errors, resource limits, malformed input,
  cancellation, and unpublished temporary deletion.
- `converter_pdf_operations`: direct `PdfProcessor` integration using the real
  hash-pinned qpdf executable and synthetic PDFs with unique content, identities,
  and page dimensions. It checks inspection; split counts, ordering, and partial
  outcomes; extraction, duplicate/reverse reordering, merge order; all three
  rotations; exact Unicode Title/Author/Subject/Keywords; new and existing Info
  dictionaries; corruption, encryption, cancellation, and collisions; native
  source/bundle pinning; process deadline, memory and byte limits; and every
  required bundle component's removal plus DLL content tampering.

Test barriers are compiled only with `AU_CONVERTER_TEST_HOOKS`; product builds
contain no injection callback. Required plugin or fixture unavailability fails
these executables rather than counting rejection as a successful conversion.
The image/native executables have a 90-second CTest timeout and the PDF executable
has a 180-second timeout. PDF test-only budgets can only reduce production limits;
they never replace qpdf with a stub or permit an unverified tool. Results go directly to stderr so
host Qt logging rules cannot suppress their verdicts.

The separate project GoogleTest file additionally covers the catalog and queue
schema rejection. Its unavailable-plugin skip is restricted to a catalog check,
never an arbitrary rejected conversion. Standalone console evidence does not
claim that the GoogleTest target, full application or installer was exercised.
