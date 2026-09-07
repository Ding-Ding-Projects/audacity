# Local model manager

The toolkit module ships a local model manager that talks only to a locally
running model runtime over its HTTP API, at `127.0.0.1:11434` by default.
The host is configurable, but only to another loopback or private network
address; a public address is refused before any request is sent.

## Behaviour

- Health and version are read from `/api/version` and shown at the top of the
  page, with an honest "Not connected" state when the runtime is not running.
- Installed models are read from `/api/tags`. The documented local API does
  not provide an exhaustive public-library listing, so this surface never
  presents the installed list as a catalog. It states that complete catalog
  coverage is unknown until a separately verified official catalog snapshot
  is available.
- Every model gets one of four hardware fit verdicts: Runs well, Runs with
  limits, Unlikely, or Unknown. The verdict is computed only from measured
  evidence (system RAM from `/proc/meminfo`, free disk space at the download
  destination, and GPU memory from `nvidia-smi` when it is present). A model
  is never judged from its name; missing evidence always produces Unknown.
- Pulls use `/api/pull` and are scheduled by a bounded local queue (default
  two concurrent requests, maximum four). Queued tag names persist locally
  and are reissued only after the local runtime is reachable. The queue has
  no price, checkout, account, or payment concept.
- Chat streams over `/api/chat`, with an editable system prompt, the documented
  `temperature` option, a bounded twenty-message request history, explicit
  stop, and retry-from-last-message recovery. Prompt input is bounded to
  16 KiB and the system prompt to 4 KiB. Attachments remain visibly unavailable
  until verified capability metadata has been inspected from the local runtime.
- Harness profiles describe one allowlisted executable and its literal
  arguments, never a shell command line. An argument or executable path that
  looks like a shell command (containing `|`, `&&`, `;`, backticks, `$(`, and
  so on) is rejected before launch.

## Configuration

The host address is a plain text setting, validated against loopback and
private network ranges before it is accepted.

## Failure modes

When the runtime is not reachable, the page shows a recovery card explaining
the situation and offering Retry. Requests that fail for any other reason
report the exact operation and the underlying error rather than a generic
failure message. A cancelled streamed chat is retained as a visibly stopped
partial local response instead of being represented as a successful answer.

## Security considerations

- No request ever reaches a public host; the client rejects a public address
  before opening a connection.
- Harness profiles cannot carry a shell string, so this surface cannot be
  used to run an arbitrary command line.

## Verification

- `hardwarefit_tests.cpp` proves the four verdicts follow only from measured
  evidence, never a model's name.
- `harnessprofile_tests.cpp` proves a shell-shaped argument or executable
  path is rejected.
- `pullcart_tests.cpp` proves the cart's data model carries no payment
  field of any kind.
- The focused toolkit build validates the client interface and QML imports.
  Runtime verification still requires a local Ollama service because the
  client intentionally does not invent installed tags, catalog metadata, or
  capability claims.

## Import hardening checkpoint, incomplete

The current importer work is preserved for continuation. It adds exact parsed
Ollama catalog URL boundaries, strict scalar types and identifiers, duplicate
key/receipt/model/tag rejection, a pre-parse depth limit of 12, scalar and aggregate
limits, bounded local-file reads, allowlisted persistence, and restart validation.
Local imports remain `untrusted-local-import` with `originVerified=false`.
Self-consistent hashes and receipt metadata do not establish official origin,
and no signing is performed. Acquisition-shaped claims retain index/tag-page
items, transitions, terminal markers, counts, timestamps and a graph digest;
these are consistency checks on untrusted local claims, not fetched-origin proof.

A committed console test route now exists at `src/toolkit/tests/ollama`:

```powershell
# Run in an MSVC environment with the Qt 6.10 Core and Network development files.
cmake -S src/toolkit/tests/ollama -B build/ollama-catalog-hardening -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="<Qt prefix>"
cmake --build build/ollama-catalog-hardening --parallel 2
ctest --test-dir build/ollama-catalog-hardening -V
```

The executable uses QCoreApplication and isolated INI settings. It opens no GUI
and performs no network requests. The first compile attempt exposed a numeric
formatting mismatch in test labels; that source issue was corrected. The next
compile and link succeeded, and direct execution reported **84 cases: 83 passed,
1 failed**. The failing case is `metadata needs no verified-looking claim and
always remains untrusted`, at the restart-preservation assertion. On reload,
normalization currently turns its own `untrusted-local-import` state into a new
`declaredCompleteness` field, so the restored map differs. Fix that idempotency
problem first, then rebuild and run the committed route at a pinned commit.
The full application, GoogleTest target, and acquisition Python suite were not
run during this checkpoint. The prior temporary probe remains preserved and is
not current verification evidence.

Executed binary SHA-256:
`13E5596F55AE9A548148A8D5340879F3C5CF3614D31618B51696860CC657FD9D`.
Direct-run log `build/ollama-catalog-hardening/iteration1.log` SHA-256:
`C49741F261EB832DDCA5385984EE5FB7821B075063356366EF0A2813208AE9DC`.
Compiled client source SHA-256:
`E7006EFD0113C20BBB1CDD1BE84FD8640783BDAB9DB61CB7177D6D6E998B8A27`.
Compiled catalog validation header SHA-256:
`BA78E70F9E38CB103767D55EEFBC7EAE5C815DEDDF39E62560E660C5EFD742DE`.
Compiled standalone test source SHA-256:
`5A229F2F86AC439832E66FF6BDED281173CB2A3C6D2E9515E136B7C46E69ACD6`.

The acquisition tool is still unchanged and incomplete. Live bounded reads of
the official index and one official tag page showed that its generic `hx-get`
handling mistakes search/filter controls for pagination. It also still needs
strict URL/path/query validation, redirect refusal before an unintended request,
explicit request/response/time/aggregate bounds, complete index and tag receipt
chains with actual byte hashes and observed counts, atomic output publication,
and committed negative acquisition tests. Keep actual acquisition observations
separate from what an imported local document merely claims. No new complete
official catalog acquisition or receipt was produced during this checkpoint.
