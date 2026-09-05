# Vendored fonts

Roboto Flex and Noto Sans HK are downloaded by `../fetch-fonts.sh` from the
google/fonts repository at commit `5e35378e6bda803962ee6fd257e444a7d459660d`
and verified against pinned SHA-256 hashes before they are installed here.
Both fonts are licensed under the SIL Open Font License 1.1; the license
texts are `OFL-RobotoFlex.txt` and `OFL-NotoSansHK.txt`.

The site never requests fonts from a network at runtime. If a file is
missing the browser uses the system font stack declared in `css/tokens.css`.
