# Universal export

The toolkit module ships a reusable export service and an `ExportSheet`
surface that any list model can open. It offers every coding file format
that can carry the data: JSON, JSON Lines, YAML, TOML, XML, CSV, TSV,
Markdown, HTML and SQL, plus a store-only ZIP archive.

## Behaviour

- Before a tabular export (CSV or TSV) runs, the sheet lists exactly which
  fields would be dropped because a flat table cannot carry a nested value.
  Nothing is silently dropped without that disclosure.
- All text output is UTF-8.
- JSON and CSV exports are re-importable: the same field names and types
  round-trip.
- ZIP archives are written by the module's own small store-only writer, with
  no external compression library dependency. The export sheet says plainly
  that 7z packaging is not available in this build; only ZIP is offered.

## Configuration

The export sheet takes the rows to export and the destination path chosen
through a native file dialog; there is no other configuration.

## Failure modes

A write that cannot complete (an unwritable destination, for instance)
reports the exact path and reason rather than a bare failure.

## Security considerations

Export never contacts the network. Values are escaped per format (CSV/TSV
quoting, XML and HTML entity escaping, SQL single-quote doubling) so an
exported value cannot corrupt the file it is written into.

## Verification

`exportservice_tests.cpp` covers field-dropping detection, JSON/CSV
round-tripping, and that the store ZIP writer produces a structurally valid
archive (correct local file headers, central directory and end-of-central-
directory record).
