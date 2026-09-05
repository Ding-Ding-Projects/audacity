# Local model manager

The toolkit module ships a local model manager that talks only to a locally
running model runtime over its HTTP API, at `127.0.0.1:11434` by default.
The host is configurable, but only to another loopback or private network
address; a public address is refused before any request is sent.

## Behaviour

- Health and version are read from `/api/version` and shown at the top of the
  page, with an honest "Not connected" state when the runtime is not running.
- Installed and running models are read from `/api/tags`.
- The catalog view is meant to be exhaustive: every model and every published
  variant from the runtime's own public library pages, fetched with
  pagination, carrying a revision identifier, a refresh timestamp and a
  completeness verdict, so a partial fetch is visible rather than silently
  presented as the whole catalog.
- Every model gets one of four hardware fit verdicts: Runs well, Runs with
  limits, Unlikely, or Unknown. The verdict is computed only from measured
  evidence (system RAM from `/proc/meminfo`, free disk space at the download
  destination, and GPU memory from `nvidia-smi` when it is present). A model
  is never judged from its name; missing evidence always produces Unknown.
- The batch pull cart schedules local downloads only. It has no price, no
  checkout, no account and no payment concept anywhere in its model or its
  wording, because adding a model to the cart never buys anything.
- Chat streams over `/api/chat`, with an editable system prompt and
  documented parameters, and sessions are kept locally.
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
failure message.

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
