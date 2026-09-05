#!/usr/bin/env python3
"""
Download the user interface fonts that Audacity's Material Design 3 theme uses.

Roboto Flex is the variable font that carries the Material 3 type scale.
Noto Sans HK is the fallback that keeps Chinese, Japanese and Korean text
readable in the same interface.

Both are fetched from the canonical google/fonts repository at a pinned commit
so that the bytes are reproducible, and each download is checked against a
recorded SHA-256 digest. The Open Font License file that belongs with each
family is fetched from the same folder.

Usage:
    python3 buildscripts/tools/fetch_fonts.py
    python3 buildscripts/tools/fetch_fonts.py --verify-only
    python3 buildscripts/tools/fetch_fonts.py --print-digests

If the download fails, the exact error is reported and the caller should record
it in docs/design/FONTS.md and fall back to the system font stack.
"""

import argparse
import hashlib
import os
import sys
import urllib.error
import urllib.request

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
FONT_DIR = os.path.join(REPO_ROOT, "share", "fonts")

# Pinned google/fonts commit. Do not move this without re-recording every digest.
COMMIT = "5e35378e6bda803962ee6fd257e444a7d459660d"
BASE = "https://raw.githubusercontent.com/google/fonts/" + COMMIT + "/"

ASSETS = [
    {
        "name": "Roboto Flex",
        "path": "ofl/robotoflex/RobotoFlex%5BGRAD%2CXOPQ%2CXTRA%2CYOPQ%2CYTAS%2CYTDE%2CYTFI%2CYTLC%2CYTUC%2Copsz%2Cslnt%2Cwdth%2Cwght%5D.ttf",
        "target": "RobotoFlex.ttf",
        "sha256": "9b523f7d82593df0107173849ebb8c817471a1df4b4fb2c3cbf40cfd810c8281",
    },
    {
        "name": "Roboto Flex license",
        "path": "ofl/robotoflex/OFL.txt",
        "target": "RobotoFlex-OFL.txt",
        "sha256": "9cbaed04b20c853f99840efe5dc96956f6f6120ed83a0ade35f9281a2b63e5d0",
    },
    {
        "name": "Noto Sans HK",
        "path": "ofl/notosanshk/NotoSansHK%5Bwght%5D.ttf",
        "target": "NotoSansHK.ttf",
        "sha256": "76098ee78ec234cd4f8c950742b3f766fea2f8b43d5180d901048f4fc86c6849",
    },
    {
        "name": "Noto Sans HK license",
        "path": "ofl/notosanshk/OFL.txt",
        "target": "NotoSansHK-OFL.txt",
        "sha256": "1c05c68c34f9708415aada51f17e1b0092d2cea709bf4a94cd38114f9e73d7d9",
    },
]


def digest(data):
    return hashlib.sha256(data).hexdigest()


def fetch(url):
    request = urllib.request.Request(url, headers={"User-Agent": "audacity-font-fetch"})
    with urllib.request.urlopen(request, timeout=120) as response:
        return response.read()


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--font-dir", default=FONT_DIR)
    parser.add_argument("--verify-only", action="store_true",
                        help="check the files already on disk against the recorded digests")
    parser.add_argument("--print-digests", action="store_true",
                        help="download and print the digests instead of enforcing them")
    args = parser.parse_args()

    os.makedirs(args.font_dir, exist_ok=True)
    failures = []
    digests = {}

    for asset in ASSETS:
        target = os.path.join(args.font_dir, asset["target"])

        if args.verify_only:
            if not os.path.exists(target):
                failures.append("%s is missing from %s" % (asset["target"], args.font_dir))
                continue
            with open(target, "rb") as handle:
                actual = digest(handle.read())
            if asset["sha256"] and actual != asset["sha256"]:
                failures.append("%s digest is %s, expected %s"
                                % (asset["target"], actual, asset["sha256"]))
            continue

        url = BASE + asset["path"]
        try:
            data = fetch(url)
        except (urllib.error.URLError, urllib.error.HTTPError, OSError) as error:
            failures.append("%s could not be downloaded from %s: %s" % (asset["name"], url, error))
            continue

        actual = digest(data)
        digests[asset["target"]] = actual

        if asset["sha256"] and not args.print_digests and actual != asset["sha256"]:
            failures.append("%s digest is %s, expected %s"
                            % (asset["target"], actual, asset["sha256"]))
            continue

        with open(target, "wb") as handle:
            handle.write(data)
        print("wrote %s (%d bytes, sha256 %s)" % (target, len(data), actual))

    if args.print_digests:
        print("")
        print("Recorded digests:")
        for name, value in digests.items():
            print('    "%s": "%s",' % (name, value))

    if failures:
        print("")
        print("Font fetch failed:")
        for failure in failures:
            print("    " + failure)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
