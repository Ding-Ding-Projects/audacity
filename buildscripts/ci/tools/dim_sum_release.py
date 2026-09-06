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
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))
INDEX_PATH = os.path.join(REPO_ROOT, "buildscripts", "packaging", "Windows", "Squirrel", "dim-sum-release-index.json")


def decode_png(data: bytes):
    """Fully decode the indexed PNG's scanlines using only the standard library.

    The catalog contract requires 8-bit, non-interlaced RGB/RGBA/greyscale
    images. Validating each chunk CRC, inflating all IDAT data, and unfiltering
    every row catches truncation that a signature-and-IHDR-only check cannot.
    """
    if len(data) < 33 or data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("indexed photo is not a PNG")
    cursor = 8
    width = height = bit_depth = color_type = interlace = None
    idat = []
    saw_iend = False
    while cursor < len(data):
        if cursor + 12 > len(data):
            raise ValueError("truncated PNG chunk")
        length = struct.unpack(">I", data[cursor:cursor + 4])[0]
        kind = data[cursor + 4:cursor + 8]
        end = cursor + 12 + length
        if end > len(data):
            raise ValueError("truncated PNG chunk payload")
        payload = data[cursor + 8:cursor + 8 + length]
        expected_crc = struct.unpack(">I", data[cursor + 8 + length:end])[0]
        if (zlib.crc32(kind + payload) & 0xffffffff) != expected_crc:
            raise ValueError("PNG chunk CRC mismatch for {0}".format(kind.decode("ascii", "replace")))
        cursor = end
        if kind == b"IHDR":
            if len(payload) != 13 or width is not None:
                raise ValueError("invalid PNG IHDR")
            width, height, bit_depth, color_type, compression, filter_method, interlace = struct.unpack(">IIBBBBB", payload)
            if not width or not height or compression != 0 or filter_method != 0:
                raise ValueError("unsupported PNG header")
        elif kind == b"IDAT":
            idat.append(payload)
        elif kind == b"IEND":
            if payload or cursor != len(data):
                raise ValueError("invalid PNG trailer")
            saw_iend = True
            break
    if width is None or not idat or not saw_iend:
        raise ValueError("incomplete PNG")
    channels = {0: 1, 2: 3, 4: 2, 6: 4}.get(color_type)
    if bit_depth != 8 or channels is None or interlace != 0:
        raise ValueError("unsupported PNG pixel format")
    stride = width * channels
    try:
        scanlines = zlib.decompress(b"".join(idat))
    except zlib.error as exc:
        raise ValueError("PNG image data cannot be decompressed: {0}".format(exc))
    if len(scanlines) != height * (stride + 1):
        raise ValueError("PNG decoded data has an unexpected length")
    previous = bytearray(stride)
    offset = 0
    for _ in range(height):
        filter_type = scanlines[offset]
        row = bytearray(scanlines[offset + 1:offset + 1 + stride])
        offset += stride + 1
        for index in range(stride):
            left = row[index - channels] if index >= channels else 0
            above = previous[index]
            upper_left = previous[index - channels] if index >= channels else 0
            if filter_type == 1:
                row[index] = (row[index] + left) & 0xff
            elif filter_type == 2:
                row[index] = (row[index] + above) & 0xff
            elif filter_type == 3:
                row[index] = (row[index] + ((left + above) >> 1)) & 0xff
            elif filter_type == 4:
                predictor = left + above - upper_left
                distances = (abs(predictor - left), abs(predictor - above), abs(predictor - upper_left))
                row[index] = (row[index] + (left, above, upper_left)[distances.index(min(distances))]) & 0xff
            elif filter_type != 0:
                raise ValueError("unsupported PNG filter type {0}".format(filter_type))
        previous = row
    return width, height


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
        for field in ("width", "height", "bytes"):
            if not isinstance(entry.get(field), int) or entry[field] <= 0:
                raise ValueError("indexed photo lacks positive {0}".format(field))
        source = tracked_path(entry["path"])
        with open(source, "rb") as handle:
            data = handle.read()
        digest = hashlib.sha256(data).hexdigest()
        if digest.lower() != entry["sha256"].lower():
            raise ValueError("SHA-256 mismatch for indexed photo {0}".format(entry["path"]))
        dimensions = decode_png(data)
        if dimensions != (entry["width"], entry["height"]):
            raise ValueError("PNG dimensions do not match indexed metadata")
        if len(data) != entry["bytes"]:
            raise ValueError("PNG byte length does not match indexed metadata")
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
