# Squirrel.Windows packaging inputs

This directory holds the inputs used by
`buildscripts/ci/windows/package_squirrel.ps1`, which is the only supported way
to produce a Windows installer for Material Audacity.

| File | Purpose |
| --- | --- |
| `Audacity.nuspec.in` | Template expanded into the generated `.nuspec`. `@TOKEN@` placeholders are replaced by the packaging script. |
| `squirrel.lock.json` | Pinned download URL, version and SHA256 for `squirrel.windows` and `nuget.exe`. Both are verified before use. |
| `package-output-manifest.json` | Generated beside each package output. It records only the current package version, current feed entries, file hashes, and whether a baseline was used only for delta generation. |

There is no WiX, MSI, NSIS or Inno Setup path. Code signing is permanently
prohibited, so no signing tool is invoked and no signing input is accepted.
See `docs/design/RELEASE.md` for the full release contract.

`-PreviousReleasesDir` is a private baseline input. Its `RELEASES` file is
validated before every referenced package is staged into a private Squirrel
workspace. The publish directory receives only the current `Setup.exe`, current
full package, current delta package when applicable, a current-version
`RELEASES` feed, checksums, and the output manifest. This prevents a previous
full package from reaching a collector that requires exactly one current full
package, while keeping the installed baseline available to Squirrel's delta
builder.

Run the local output-isolation regression after packaging changes with:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File buildscripts/ci/windows/test_package_squirrel_output.ps1
```
