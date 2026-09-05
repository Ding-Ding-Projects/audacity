# External editor integration

`ExternalEditorService` detects installed code editors and opens the current
project folder, or any exported file, in the user's chosen one.

## Behaviour

- Detection looks at the system PATH for `code`, `code-insiders`, `codium`,
  `gedit` and `kate`, plus any editor the user adds manually by picking its
  executable.
- Visual Studio Code and its variants are opened with the folder path as a
  literal argument, which opens it as the workspace root rather than a
  single loose file.
- The chosen editor is persisted so it does not need choosing again.
- When no editor is found, the service reports that plainly; the built-in
  suggestion to install one is not the only path, since a custom executable
  can always be added by hand.

## Configuration

`preferredEditorId` persists the user's choice. Custom editors are added
with `addCustomEditor(id, label, executablePath)`.

## Failure modes

Opening a folder or file returns false when the chosen editor's executable
cannot be found or the process could not be started; the caller shows the
graceful message in that case.

## Security considerations

The editor is launched directly with a literal argument list; the service
never builds or passes a shell command string.

## Verification

`externaleditorservice_tests.cpp` covers detection reporting `found: false`
for an editor that plainly is not on `PATH` in the test environment, and
that a custom editor added by path is reported correctly.
