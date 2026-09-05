# Toy locks

## Behaviour

Any element with the personalize helper attached can be locked from its
context menu with "Lock this element...". A wizard offers six policies: PIN,
password, PIN and password, password and a one time code, PIN and a one
time code, or all three together. A locked element is disabled: clicking it
opens the unlock prompt instead of reaching whatever is underneath.

Unlocking accepts either a manual PIN entry or a small on-screen keypad,
both feeding the same validator and the same attempt budget. After five
wrong attempts in a row, unlocking that element is refused until it is
removed and recreated. An unlock can last for a chosen number of minutes,
or until the application closes.

This is a self imposed speed bump, not a security boundary. It does not
encrypt anything, and it does not stop another program running as the same
user from reading the lock file. It says so on the wizard itself.

## Configuration

Locks are stored at
`<application data directory>/personalize/locks.json`. Each lock keeps its
own salted, repeatedly hashed PIN and password, and its own one time code
secret; there is no shared credential across locks.

## Failure modes

Recovery from a forgotten credential is deleting the application's data
folder, shown on the Personalize preferences page and on the unlock prompt
itself. There is no other reset route by design.

## Security considerations

Explicitly not a security boundary. Never described as one anywhere in the
product.

## Verification

Manual: create a lock with each policy, confirm the element becomes
disabled, confirm the unlock prompt appears on activation, confirm a wrong
credential is rejected and a correct one is accepted, and confirm removing
the lock restores normal behaviour. See the capture under
`docs/design/captures/lane-m/`.
