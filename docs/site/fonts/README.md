# Vendored fonts

`../fetch-fonts.sh` attempts to download Roboto Flex and Noto Sans HK from a
pinned commit of the `google/fonts` GitHub repository, verifying each file's
sha256 before installing it here.

## Last run result: FAILED

Running the script in this environment produced:

```
Fetching NotoSansHK-Variable.ttf from https://raw.githubusercontent.com/google/fonts/a0d69dd2ea7990a5c542d6ed57a9ea9fdf76b5d7/ofl/notosanshk/NotoSansHK%5Bwght%5D.ttf
curl: (22) The requested URL returned error: 404
ERROR: failed to download https://raw.githubusercontent.com/google/fonts/a0d69dd2ea7990a5c542d6ed57a9ea9fdf76b5d7/ofl/notosanshk/NotoSansHK%5Bwght%5D.ttf
Fetching RobotoFlex-Variable.ttf from https://raw.githubusercontent.com/google/fonts/a0d69dd2ea7990a5c542d6ed57a9ea9fdf76b5d7/ofl/robotoflex/RobotoFlex%5Bslnt%2Cwdth%2Cwght%2CGRAD%2CXOPQ%2CXTRA%2CYOPQ%2CYTAS%2CYTDE%2CYTFI%2CYTLC%2CYTUC%5D.ttf
curl: (22) The requested URL returned error: 404
ERROR: failed to download https://raw.githubusercontent.com/google/fonts/a0d69dd2ea7990a5c542d6ed57a9ea9fdf76b5d7/ofl/robotoflex/RobotoFlex%5Bslnt%2Cwdth%2Cwght%2CGRAD%2CXOPQ%2CXTRA%2CYOPQ%2CYTAS%2CYTDE%2CYTFI%2CYTLC%2CYTUC%5D.ttf
One or more font downloads failed. See site/fonts/README.md.
```

The pinned commit/path combination returned HTTP 404 in this sandbox (the
outbound network is proxied and GitHub raw content may be filtered or the
exact file path at that commit has since moved). No font binaries were
written to this directory.

## Fallback in effect

Because the fetch failed, `css/tokens.css` and `css/base.css` use a system
font stack instead of the vendored variable fonts:

```
--md-font-body: "Atkinson Hyperlegible Next", "Noto Sans HK", "Segoe UI", system-ui, sans-serif;
--md-font-display: "Fraunces", Georgia, "Noto Serif HK", serif;
```

To retry: re-run `./fetch-fonts.sh` from a network that can reach
`raw.githubusercontent.com`, fill in the `SHA256` values in the script once
the real bytes are known, and the site will pick the local `@font-face`
files up automatically (see `css/fonts.css`, gated on file presence).
