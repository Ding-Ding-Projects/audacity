# Squirrel.Windows packaging inputs

This directory holds the inputs used by
`buildscripts/ci/windows/package_squirrel.ps1`, which is the only supported way
to produce a Windows installer for Material Audacity.

| File | Purpose |
| --- | --- |
| `Audacity.nuspec.in` | Template expanded into the generated `.nuspec`. `@TOKEN@` placeholders are replaced by the packaging script. |
| `squirrel.lock.json` | Pinned download URL, version and SHA256 for `squirrel.windows` and `nuget.exe`. Both are verified before use. |

There is no WiX, MSI, NSIS or Inno Setup path. Code signing is permanently
prohibited, so no signing tool is invoked and no signing input is accepted.
See `docs/design/RELEASE.md` for the full release contract.
