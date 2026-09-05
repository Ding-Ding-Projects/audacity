# Automatic updates

Material Audacity ships with a background update checker for its Windows
Squirrel.Windows installation, in the spirit of Chrome or GitHub Desktop:
checks happen quietly, a downloaded and verified package sits ready, and a
persistent banner offers to restart into it whenever the user is ready. On
every other platform the checker reports that it does not apply, honestly,
rather than pretending to work.

## Behaviour

- On startup, and once every `checkIntervalHours` hours afterward (default
  24, configurable from 1 to 720), the application asks the update feed for
  its `RELEASES` listing over HTTPS.
- The feed is a plain text Squirrel.Windows `RELEASES` file: one line per
  package, each holding a SHA1 hash, a file name and a byte size. The default
  feed points at the latest GitHub release of the project, and the feed URL
  can be changed in Preferences.
- When a package names a version newer than the one currently installed, it
  is downloaded into the same `SquirrelTemp` folder Squirrel's own updater
  uses, then checked against the feed's SHA1 hash and byte size. A package
  that does not match is deleted and never offered.
- Once a package verifies, a Material Design 3 banner appears in the bottom
  right corner of the main window. It never interrupts work: it does not
  steal focus, does not block input and stays until dismissed or acted on.
  The banner states the new version, an explicit warning that the build is
  unsigned, and two actions: **Restart to install update** and **Later**.
- Choosing **Restart to install update** runs Squirrel's own `Update.exe`
  with the already downloaded package, waits for it to apply, restarts the
  application through the shortcut launcher, and quits the current process.
- Choosing **Later** hides the banner until the next check finds something
  newer again; the verified package stays on disk so nothing is downloaded
  twice.
- A manual check is always available from **Help > Check for updates** and
  from the **Updates** page in Preferences.

## States

The checker moves through the following states, all visible from the
Preferences page and reflected in the banner:

| State | Meaning |
| --- | --- |
| No update | The last check found nothing newer than the installed version. |
| Checking | A check is currently in progress. |
| Ready | A newer package has downloaded and verified; the banner is showing. |
| Failed | The last check or download failed; the exact error is shown. |
| Not applicable | The platform is not Windows, or this copy is not a Squirrel.Windows installation. |

Offline operation, an invalid feed and a corrupt downloaded package all fall
under **Failed**, with the specific reason kept in the error text rather than
a generic message.

## Configuration

Preferences > Updates exposes:

- **Check for updates automatically**, a switch that enables or disables the
  background timer. Manual checks still work when this is off.
- The feed URL currently in use (read only from this page; change it through
  the underlying setting if you run a private feed).
- The configured check interval.
- The time of the last check made in the current session.
- A **Check for updates** button that runs a check on demand.

## Failure modes

- **Network unreachable**: the check fails, the exact network error is kept
  as the last error, and the timer tries again at the next interval.
- **Malformed feed**: lines that do not parse as a well formed
  `<sha1> <filename> <size>` entry are skipped and logged; the feed as a
  whole is only rejected when it produces no usable entries at all.
- **Hash or size mismatch**: the downloaded file is deleted immediately and
  never offered. Nothing partially verified is ever presented to the user.
- **Update.exe missing or fails**: `restartToUpdate` reports the exact
  failure and the application keeps running normally.

## Security considerations

This build is never code signed, and the update checker never claims
otherwise. The SHA1 hash from the feed is an integrity check only: it proves
the downloaded bytes match what the feed listed, not that the feed or the
bytes came from a trustworthy source. Anyone who can serve the configured
feed URL controls what gets installed, which is why the feed URL defaults to
GitHub's own release assets over HTTPS and is shown, not hidden, in
Preferences. The release notes for every build also publish a SHA256 of every
artifact so a user can check a download by hand.

## Verification

- Unit tests cover the `RELEASES` parser (well formed lines, malformed
  lines, delta versus full packages, version comparison), the package
  verifier (missing file, size mismatch, hash mismatch) and the banner state
  transitions, built under the `AU_BUILD_SQUIRRELUPDATE_TESTS` CMake option.
- Setting `AU_SQUIRREL_DEMO_BANNER=1` in the environment forces the banner
  and the Preferences ready state visible with a placeholder version, with no
  network access, for screenshots and manual review.
- On non-Windows platforms the Preferences page shows the "not applicable"
  explanation instead of the switch and button, and the banner never appears.
