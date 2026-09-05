# Renaming the application

## Behaviour

The Personalize preferences page lets a person change what the title bar,
the About surface, and notifications call the application. The shipped name
stays "Material Audacity" and a reset button restores it in one click.

## Configuration

The chosen name is stored as plain text at
`<application data directory>/personalize/display-name.txt`. Nothing else
reads or writes this file.

## Failure modes

An empty name is not accepted; it falls back to the shipped default rather
than leaving the title bar blank.

## Security considerations

The rename changes only display copy. It never touches the application data
directory path, package identifiers, the installer identity, or the update
feed, all of which stay derived from the fixed default name. Diagnostics and
crash reports keep using the real shipped name so a report is never
ambiguous about what software produced it.

## Verification

Manual: set a name, confirm it is used where the setting says it is used,
restart, confirm it persisted, then reset and confirm the default returns.
