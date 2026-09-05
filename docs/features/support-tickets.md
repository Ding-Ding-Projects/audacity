# Support Tickets

## Behaviour

Support Tickets is a joke support desk reachable from the unlock prompt, the
Personalize lock section, and Help. It plays the part of a real support
system: a category, a description, a made up ticket number, and a canned
first response. Its one real action opens the application's own data folder
in the platform's file manager so a locked out person can delete it
themselves.

## Configuration

Tickets are stored locally at
`<application data directory>/personalize/support-tickets.json` purely so
the page has something to show; nothing about them is ever sent anywhere.

## Failure modes

If the platform's file manager cannot be launched, the page reports that
plainly rather than pretending the folder opened.

## Security considerations

There is no network call anywhere in this feature. The disclosure line on
the page stating that nothing is sent anywhere is never styled away by the
funny level setting.

## Verification

Manual: open a ticket, confirm the canned response appears, and confirm the
button opens the correct application data folder.
