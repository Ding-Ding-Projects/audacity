# Local file conversion core

This backend slice adds `src/converter`, an unregistered Qt module for a future
local file-converter surface. It does not provide a user interface, application
registration, translations, packaged-artifact proof, full PDF product integration, audio
conversion, or video conversion.

## Current adapter catalog

The current functional scope is Qt image plugin conversion from PNG to JPEG or
BMP, and from JPEG or BMP to PNG. Each pair requires the running Qt plugins to
report both decoding and encoding support. PDF, audio, video, archives,
structured data, code/text and binary encodings remain visible as disabled
adapters with precise reasons.

The PDF catalog entry is enabled only when the packaged `converter-tools/qpdf/qpdf.exe`
matches the pinned SHA-256 in `buildscripts/converter-tools/qpdf.lock.json`. It
never searches `PATH` or accepts a developer tool. The adapter runs fixed qpdf
arguments only, has a 60-second process deadline, caps source and output files,
rejects encrypted inputs that need credentials, writes a private temporary file,
reopens it with `--check` and `--show-npages`, and never overwrites an existing
destination. qpdf currently covers inspect, merge, extract, reorder, rotate,
and split requests. Metadata uses qpdf's official JSON update format and only
permits bounded Title, Author, Subject, and Keywords values.

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

## Resource and cancellation boundaries

Input is limited to 256 MiB, decoded dimensions to 100 million pixels, and
encoded output to 512 MiB. Invalid signatures, malformed dimensions and truncated
images are rejected. Source and temporary reads use bounded chunks. Cancellation
is checked during IO and immediately before commit. A cancellation that arrives
after the atomic commit does not undo the completed output.

The decoder remains in-process, without a hard CPU deadline or process memory
sandbox. The file transaction does not protect against privileged kernel
tampering or writes through a mapping created before the source handle opened.
This is a backend namespace-race boundary, not a complete decoder sandbox or a
claim of power-loss durability. A process crash may leave an unpublished
temporary; originals are not overwritten by the create-if-absent commit.

Queue persistence records versioned item state, paths, target format and the
caller's overwrite choice, without input or output bytes. Queue reads are paged
and capped to 500 rows per call.

## Parent integration required

Application integration must register `src/converter/CMakeLists.txt`, provide
QML and localization, add adapter searches and their regex builders, bind
progress/cancellation to bounded workers, provide storage-capacity preflight,
prove bundled Qt plugins, implement safe approved overwrite, and integrate
process isolation, execution limits and durable crash recovery.

The full converter still requires package installation of the verified qpdf
distribution, output-order/rotation/metadata semantic assertions, and full application integration. It also requires audio/video and other
category adapters, batch history/exports, accessibility, responsive behavior,
notifications, command-palette routing, and real packaged-application
interaction and capture evidence. These are not implemented by this backend.

## Focused verification

`src/converter/tests/standalone` builds two real Qt console executables:

- `converter_core_smoke`: successful JPEG conversion and full reopen, source
  collision, cancellation, queue persistence and restart.
- `converter_native_transactions`: independently reported deterministic cases
  for signature detection, file identity, symlinks, directory junctions,
  destination insertion, source and temporary substitution, attribute-only
  directory redirection, native IO errors, resource limits, malformed input,
  cancellation, and unpublished temporary deletion.

Test barriers are compiled only with `AU_CONVERTER_TEST_HOOKS`; product builds
contain no injection callback. Required plugin or fixture unavailability fails
these executables rather than counting rejection as a successful conversion.
Each executable has a 90-second CTest timeout. Results go directly to stderr so
host Qt logging rules cannot suppress their verdicts.

The separate project GoogleTest file additionally covers the catalog and queue
schema rejection. Its unavailable-plugin skip is restricted to a catalog check,
never an arbitrary rejected conversion. Standalone console evidence does not
claim that the GoogleTest target, full application or installer was exercised.
