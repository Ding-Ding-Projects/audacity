# Built in authenticator

## Behaviour

A local, offline TOTP (RFC 6238) authenticator, reachable from the
Personalize preferences page. Adding an entry generates a random secret,
renders it as a QR code drawn in process (no network request, no third
party service), and shows the base32 secret beside it for manual entry.
Pairing is confirmed by typing the current code back before the entry is
kept, so a mistyped or mis-scanned secret is caught immediately rather than
locking someone out later.

Each entry shows its current code, a countdown to the next period, and the
next code, refreshed once a second. SHA-1, SHA-256 and SHA-512 are
supported, at 6 to 8 digits, at any period. A crude clock sanity check warns
when the system clock looks implausible; it cannot detect an ordinary few
minutes of drift, only a badly wrong date.

An `otpauth://totp/` link can also be imported directly.

## Configuration

Entries are stored at
`<application data directory>/personalize/authenticator.dat`, obscured with
a per install key kept at `authenticator.key` in the same directory with
owner-only file permissions. This is honestly not the same guarantee an
operating system credential vault gives: it keeps a casual read of the file
from being useful, it does not defend against another program running as
the same user. Ordinary application exports never include this file.

## Failure modes

An invalid secret or a pairing confirmation that does not match the current
code is refused, and the entry is not kept.

## Security considerations

Every code is computed from the machine's own clock and the stored secret;
there is no network call anywhere in this feature. The QR encoder is a
small, purpose-built implementation supporting byte mode text up to 106
bytes (QR versions 1 through 5 at error correction level L); longer text is
refused rather than silently truncated, and manual entry remains available.

## Verification

The TOTP engine is covered by the official RFC 6238 test vectors for
SHA-1, SHA-256 and SHA-512 in `src/personalize/tests/totpengine_tests.cpp`.
The QR encoder is covered by structural tests in
`src/personalize/tests/qrencoder_tests.cpp`, and was separately verified
during development by rendering its output and decoding it with a real QR
reader (`zbarimg`) and by cross-checking its Reed-Solomon error correction
codewords against the reference implementation in the `qrcode` Python
package; both routes confirmed the encoder produces correctly decodable
codes. See the capture under `docs/design/captures/lane-m/`.
