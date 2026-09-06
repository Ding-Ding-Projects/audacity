#!/usr/bin/env python3
"""Pick the next unused dim sum code name for a release and fetch its photo.

Every release carries a dim sum code name resolved from the public catalog at
https://github.com/Ding-Ding-Projects/dim-sum-photos. The dish is used once per
project: previous release bodies are scanned for the ``Code name:`` line and the
first dish not yet used is chosen. The photo is downloaded from the published
``catalog-v1`` release asset, verified to decode as a PNG, and written beside the
other release assets so it can be attached to the release.

Usage:
    dim_sum_release.py --repo OWNER/REPO --assets-dir release-assets \
        --output-json dim-sum.json

The script never generates an image and never falls back to a local file. When
the catalog or the photo cannot be fetched it exits 0 with an empty result so
the release still ships, and the release notes say that no code name could be
resolved.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import struct
import sys
import urllib.error
import urllib.request

CATALOG_URL = (
    "https://raw.githubusercontent.com/Ding-Ding-Projects/dim-sum-photos/main/catalog/index.json"
)
ASSET_BASE = "https://github.com/Ding-Ding-Projects/dim-sum-photos/releases/download/catalog-v1/"
MAX_PHOTO_BYTES = 32 * 1024 * 1024
TIMEOUT = 60


def fetch(url: str, token: str = "", limit: int = MAX_PHOTO_BYTES, accept: str = "") -> bytes:
    headers = {"User-Agent": "material-audacity-release"}
    if token:
        headers["Authorization"] = "Bearer " + token
    if accept:
        headers["Accept"] = accept
    request = urllib.request.Request(url, headers=headers)
    with urllib.request.urlopen(request, timeout=TIMEOUT) as response:
        data = response.read(limit + 1)
    if len(data) > limit:
        raise ValueError("response exceeded {0} bytes".format(limit))
    return data


def used_code_names(repo: str, token: str) -> set:
    used = set()
    if not repo:
        return used
    page = 1
    while page <= 10:
        url = "https://api.github.com/repos/{0}/releases?per_page=100&page={1}".format(repo, page)
        try:
            data = json.loads(fetch(url, token, accept="application/vnd.github+json"))
        except (urllib.error.URLError, ValueError, json.JSONDecodeError) as exc:
            print("Could not read previous releases: {0}".format(exc), file=sys.stderr)
            return used
        if not data:
            break
        for release in data:
            body = release.get("body") or ""
            for match in re.finditer(r"Code name:\s*\*{0,2}([^*\n]+?)\*{0,2}\s*(?:\(|\n|$)", body):
                used.add(match.group(1).strip())
            match = re.search(r"dim-sum-id:\s*(hk-dish-\d+)", body)
            if match:
                used.add(match.group(1))
        page += 1
    return used


def png_dimensions(data: bytes):
    if len(data) < 24 or data[:8] != b"\x89PNG\r\n\x1a\n" or data[12:16] != b"IHDR":
        return None
    width, height = struct.unpack(">II", data[16:24])
    return width, height


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", default=os.environ.get("GITHUB_REPOSITORY", ""))
    parser.add_argument("--assets-dir", required=True)
    parser.add_argument("--output-json", required=True)
    args = parser.parse_args()
    token = os.environ.get("GH_TOKEN") or os.environ.get("GITHUB_TOKEN") or ""

    result = {"resolved": False, "reason": ""}

    try:
        catalog = json.loads(fetch(CATALOG_URL, limit=64 * 1024 * 1024))
        dishes = catalog.get("dishes") or []
    except (urllib.error.URLError, ValueError, json.JSONDecodeError) as exc:
        result["reason"] = "catalog unavailable: {0}".format(exc)
        dishes = []

    used = used_code_names(args.repo, token) if dishes else set()

    chosen = None
    for dish in dishes:
        name = dish.get("name") or {}
        code_name = "{0} · {1}".format(name.get("en", "").strip(), name.get("zhHant", "").strip())
        image = (dish.get("image") or {}).get("path") or ""
        if not name.get("en") or not image:
            continue
        if dish.get("id") in used or code_name in used:
            continue
        file_name = os.path.basename(image)
        try:
            data = fetch(ASSET_BASE + file_name)
        except (urllib.error.URLError, ValueError) as exc:
            print("Photo not published for {0}: {1}".format(dish.get("id"), exc), file=sys.stderr)
            continue
        dims = png_dimensions(data)
        if not dims:
            print("Asset {0} is not a PNG, skipping".format(file_name), file=sys.stderr)
            continue
        os.makedirs(args.assets_dir, exist_ok=True)
        target = os.path.join(args.assets_dir, "dim-sum-" + file_name)
        with open(target, "wb") as handle:
            handle.write(data)
        chosen = {
            "resolved": True,
            "id": dish.get("id"),
            "code_name": code_name,
            "name_en": name.get("en"),
            "name_zh": name.get("zhHant"),
            "asset": os.path.basename(target),
            "source_url": ASSET_BASE + file_name,
            "width": dims[0],
            "height": dims[1],
            "bytes": len(data),
        }
        break

    if chosen:
        result = chosen
    elif not result["reason"]:
        result["reason"] = "no unused dish with a published photo was found"

    with open(args.output_json, "w", encoding="utf-8") as handle:
        json.dump(result, handle, ensure_ascii=False, indent=2)
    print(json.dumps(result, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    sys.exit(main())
