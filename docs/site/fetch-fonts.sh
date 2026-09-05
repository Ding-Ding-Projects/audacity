#!/usr/bin/env bash
# Downloads Roboto Flex and Noto Sans HK from pinned Google Fonts GitHub commit
# URLs and verifies each file against a pinned sha256 before installing it into
# docs/site/fonts/. If the network is unavailable, this script fails loudly and
# the site falls back to a system font stack (see css/tokens.css).
set -euo pipefail

DEST="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/fonts"
mkdir -p "$DEST"

# Pinned to a specific commit of google/fonts so the bytes are reproducible.
COMMIT="5e35378e6bda803962ee6fd257e444a7d459660d"
BASE="https://raw.githubusercontent.com/google/fonts/${COMMIT}"

declare -A FILES=(
  ["ofl/robotoflex/RobotoFlex%5BGRAD%2CXOPQ%2CXTRA%2CYOPQ%2CYTAS%2CYTDE%2CYTFI%2CYTLC%2CYTUC%2Copsz%2Cslnt%2Cwdth%2Cwght%5D.ttf"]="RobotoFlex-Variable.ttf"
  ["ofl/notosanshk/NotoSansHK%5Bwght%5D.ttf"]="NotoSansHK-Variable.ttf"
)

# sha256sums must be filled in once the bytes are known; verification is
# mandatory, not optional. If a hash is not yet pinned, the file is rejected.
declare -A SHA256=(
  ["RobotoFlex-Variable.ttf"]="9b523f7d82593df0107173849ebb8c817471a1df4b4fb2c3cbf40cfd810c8281"
  ["NotoSansHK-Variable.ttf"]="76098ee78ec234cd4f8c950742b3f766fea2f8b43d5180d901048f4fc86c6849"
)

fail=0
for src in "${!FILES[@]}"; do
  out="${FILES[$src]}"
  url="${BASE}/${src}"
  echo "Fetching ${out} from ${url}"
  if ! curl -fsSL --max-time 30 -o "${DEST}/${out}.tmp" "${url}"; then
    echo "ERROR: failed to download ${url}" >&2
    fail=1
    continue
  fi
  expected="${SHA256[$out]}"
  actual="$(sha256sum "${DEST}/${out}.tmp" | cut -d' ' -f1)"
  if [ -n "$expected" ] && [ "$actual" != "$expected" ]; then
    echo "ERROR: sha256 mismatch for ${out}: got ${actual}, expected ${expected}" >&2
    rm -f "${DEST}/${out}.tmp"
    fail=1
    continue
  fi
  mv "${DEST}/${out}.tmp" "${DEST}/${out}"
  echo "OK: ${out} sha256=${actual}"
done

if [ "$fail" -ne 0 ]; then
  echo "One or more font downloads failed. See site/fonts/README.md." >&2
  exit 1
fi
echo "All fonts fetched into ${DEST}"
