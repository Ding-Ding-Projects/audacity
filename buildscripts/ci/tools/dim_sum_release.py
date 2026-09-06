#!/usr/bin/env python3
"""Attach one verified, tracked dim-sum photo to a release.

The release photo is selected exclusively from the repository-owned index at
``buildscripts/packaging/Windows/Squirrel/dim-sum-release-index.json``. The
index binds a dish identifier and label to a repository-relative PNG plus its
SHA-256 digest. No network access, generated image, or substitute asset is
accepted. An empty or invalid index is a release blocker.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
INDEX_PATH = os.path.join(REPO_ROOT, "buildscripts", "packaging", "Windows", "Squirrel", "dim-sum-release-index.json")


def png_dimensions(data: bytes):
    if len(data) < 24 or data[:8] != b"\x89PNG\r\n\x1a\n" or data[12:16] != b"IHDR":
        return None
    return struct.unpack(">II", data[16:24])


def tracked_path(relative_path: str) -> str:
    candidate = os.path.abspath(os.path.join(REPO_ROOT, relative_path))
    if os.path.commonpath([REPO_ROOT, candidate]) != REPO_ROOT:
        raise ValueError("indexed asset escapes the repository: {0}".format(relative_path))
    subprocess.run(["git", "-C", REPO_ROOT, "ls-files", "--error-unmatch", "--", relative_path],
                   check=True, capture_output=True, text=True)
    return candidate


def fail(output_json: str, reason: str) -> int:
    with open(output_json, "w", encoding="utf-8") as handle:
        json.dump({"resolved": False, "reason": reason}, handle, indent=2)
    print(reason, file=sys.stderr)
    return 1


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", default="", help="Retained for workflow compatibility; never used for network access.")
    parser.add_argument("--assets-dir", required=True)
    parser.add_argument("--output-json", required=True)
    args = parser.parse_args()
    try:
        with open(INDEX_PATH, encoding="utf-8") as handle:
            index = json.load(handle)
        photos = index["photos"]
        if not isinstance(photos, list) or not photos:
            return fail(args.output_json, "no tracked dim-sum release photo is indexed")
        entry = photos[0]
        for field in ("id", "code_name", "path", "sha256", "origin", "origin_disclosure"):
            if not isinstance(entry.get(field), str) or not entry[field]:
                raise ValueError("indexed photo lacks {0}".format(field))
        source = tracked_path(entry["path"])
        with open(source, "rb") as handle:
            data = handle.read()
        digest = hashlib.sha256(data).hexdigest()
        if digest.lower() != entry["sha256"].lower():
            raise ValueError("SHA-256 mismatch for indexed photo {0}".format(entry["path"]))
        dimensions = png_dimensions(data)
        if not dimensions:
            raise ValueError("indexed photo is not a PNG: {0}".format(entry["path"]))
        os.makedirs(args.assets_dir, exist_ok=True)
        asset = "dim-sum-" + os.path.basename(source)
        target = os.path.join(args.assets_dir, asset)
        shutil.copyfile(source, target)
        result = {"resolved": True, "id": entry["id"], "code_name": entry["code_name"],
                  "asset": asset, "source_url": "tracked:" + entry["path"],
                  "width": dimensions[0], "height": dimensions[1], "bytes": len(data),
                  "sha256": digest, "origin": entry["origin"],
                  "origin_disclosure": entry["origin_disclosure"]}
        with open(args.output_json, "w", encoding="utf-8") as handle:
            json.dump(result, handle, ensure_ascii=False, indent=2)
        # Keep the release metadata Unicode-correct on disk while preserving a
        # usable Windows console diagnostic on hosts whose active code page
        # cannot encode Traditional Chinese.
        print(json.dumps(result, ensure_ascii=True))
        return 0
    except (OSError, ValueError, KeyError, json.JSONDecodeError, subprocess.CalledProcessError) as exc:
        return fail(args.output_json, "tracked dim-sum release photo validation failed: {0}".format(exc))


if __name__ == "__main__":
    sys.exit(main())
