# Audacity user interface fonts

The Material Design 3 interface ships two variable fonts.

| Family        | File                        | Role                                              |
| ------------- | --------------------------- | ------------------------------------------------- |
| Roboto Flex   | `share/fonts/RobotoFlex.ttf`| The Material 3 type scale for the whole interface |
| Noto Sans HK  | `share/fonts/NotoSansHK.ttf`| Chinese, Japanese and Korean fallback             |

Both are licensed under the SIL Open Font License. The licence text that belongs
with each family is stored next to it as `RobotoFlex-OFL.txt` and
`NotoSansHK-OFL.txt`.

## Fetching the fonts

`buildscripts/tools/fetch_fonts.py` downloads both families and both licence
files from the canonical `google/fonts` repository at a pinned commit and checks
every download against a recorded SHA-256 digest.

```
python3 buildscripts/tools/fetch_fonts.py
python3 buildscripts/tools/fetch_fonts.py --verify-only
```

Pinned commit: `5e35378e6bda803962ee6fd257e444a7d459660d`.

Recorded digests:

| File                 | Bytes      | SHA-256 |
| -------------------- | ---------- | ------- |
| `RobotoFlex.ttf`     | 1787292    | `9b523f7d82593df0107173849ebb8c817471a1df4b4fb2c3cbf40cfd810c8281` |
| `RobotoFlex-OFL.txt` | 4488       | `9cbaed04b20c853f99840efe5dc96956f6f6120ed83a0ade35f9281a2b63e5d0` |
| `NotoSansHK.ttf`     | 11905808   | `76098ee78ec234cd4f8c950742b3f766fea2f8b43d5180d901048f4fc86c6849` |
| `NotoSansHK-OFL.txt` | 4388       | `1c05c68c34f9708415aada51f17e1b0092d2cea709bf4a94cd38114f9e73d7d9` |

The download and the digest check were run and both passed, so no fallback was
needed. If a future fetch fails, record the exact error here and fall back to
the system font stack. The application already does that safely: if either font
cannot be registered, a warning is logged and the platform font is used, and the
default family is only changed when Roboto Flex is actually present.

Note on the Roboto Flex file name. The upstream file encodes its variation axes
in the name, and the axis order matters:

```
ofl/robotoflex/RobotoFlex[GRAD,XOPQ,XTRA,YOPQ,YTAS,YTDE,YTFI,YTLC,YTUC,opsz,slnt,wdth,wght].ttf
```

The script stores it locally under the simpler name `RobotoFlex.ttf`.

## How the fonts reach the application

1. `share/fonts/fonts.qrc` compiles both fonts and both licence files into the
   application under the `/fonts` resource prefix. `src/app/CMakeLists.txt`
   adds it next to `app.qrc`.
2. `app_register_fonts()` in `src/app/main.cpp` registers both files with
   `QFontDatabase::addApplicationFont` before the application starts, so the
   theme can name them straight away.
3. `UiComponentsModule::onInit` in `src/uicomponents/uicomponentsmodule.cpp`
   makes `Roboto Flex` the default value of the muse setting
   `ui/theme/fontFamily`, but only when the family is actually available.

Point 3 is how the framework default is overridden from Audacity code rather
than by editing muse. The framework sets its own default in
`UiConfiguration::init` from `QFontDatabase::systemFont`. Audacity replaces the
default value afterwards. A family the user chose themselves is stored as a user
value and still wins over the default, so no preference is overwritten.

`M3.typography` reads `IUiConfiguration::fontFamily()`, so the whole Material 3
type scale follows whichever family is in effect.
