"""Candidate-bound structural evidence verification, never execution attestation."""
from __future__ import annotations

import hashlib
import json
import re
import struct
import subprocess
import zlib
from datetime import datetime
from pathlib import Path, PurePosixPath

REGISTRY = "docs/inventory/concrete-surfaces.json"
LEDGER = "docs/inventory/completion-evidence.json"
# Independent, reviewed inventory. Never discover required surfaces from evidence.
SURFACES = {
    "desktop": ("front-screen", "preferences", "schedule-editor", "experience-overlay",
                "search-popover", "notification-centre", "appearance-editor", "project-tabs",
                "documentation-browser", "command-palette", "confirmation-dialog", "version-history",
                "changelog-dialog", "external-editor-handoff", "export-sheet", "personalize-locks",
                "support-tickets", "account-recovery", "personalize-settings", "file-converter",
                "local-model-manager", "update-banner", "status-surface", "authenticator"),
    "website": ("front-page", "settings", "table-of-contents", "article", "search", "status"),
}
SHA = re.compile(r"[0-9a-f]{64}\Z")
MAX_JSON = 8 * 1024 * 1024


class Invalid(ValueError):
    pass


def require(condition, message):
    if not condition:
        raise Invalid(message)


def digest(data):
    return hashlib.sha256(data).hexdigest()


def path(root, name):
    require(isinstance(name, str) and 0 < len(name) <= 240, "invalid bounded path")
    require(re.fullmatch(r"[A-Za-z0-9_. /-]+", name) is not None, "invalid path characters")
    pure = PurePosixPath(name)
    require(not pure.is_absolute() and all(p not in (".", "..", "") for p in name.split("/")), "escaping path")
    resolved = (root / name).resolve()
    require(resolved.is_relative_to(root.resolve()), "escaping resolved path")
    require(resolved.is_file(), f"missing file: {name}")
    return resolved


def read_json(file):
    require(file.stat().st_size <= MAX_JSON, "oversized JSON")
    def unique(pairs):
        result = {}
        for key, value in pairs:
            require(key not in result, "duplicate JSON key")
            result[key] = value
        return result
    value = json.loads(file.read_text(encoding="utf-8"), object_pairs_hook=unique)
    require(isinstance(value, dict), "JSON object required")
    return value


def git(root, *args):
    result = subprocess.run(["git", "-C", str(root), *args], capture_output=True, timeout=30)
    require(result.returncode == 0, "candidate Git object unavailable")
    return result.stdout


def png_dimensions(data):
    """Decode bounded, non-interlaced 8-bit RGB/RGBA PNGs using only stdlib.

    Verify chunk CRCs, exact scanlines and reconstruct all PNG filter types.
    Deliberately refuse other encodings rather than pretending to decode them.
    """
    require(len(data) <= 32 * 1024 * 1024 and data[:8] == b"\x89PNG\r\n\x1a\n", "capture is not a PNG")
    offset, compressed, seen, width, height, channels = 8, bytearray(), [], 0, 0, 0
    while offset < len(data):
        require(offset + 12 <= len(data), "truncated PNG chunk")
        length = struct.unpack(">I", data[offset:offset + 4])[0]
        kind = data[offset + 4:offset + 8]
        end = offset + 12 + length
        require(end <= len(data), "truncated PNG data")
        payload = data[offset + 8:end - 4]
        require(zlib.crc32(kind + payload) & 0xffffffff == struct.unpack(">I", data[end - 4:end])[0], "PNG CRC mismatch")
        if not seen:
            require(kind == b"IHDR" and length == 13, "PNG header absent")
            width, height, depth, colour, compression, filtering, interlace = struct.unpack(">IIBBBBB", payload)
            require(0 < width <= 8192 and 0 < height <= 8192 and width * height <= 16_000_000, "invalid PNG dimensions")
            require(depth == 8 and colour in (2, 6) and compression == filtering == interlace == 0, "unsupported PNG encoding")
            channels = 3 if colour == 2 else 4
        elif kind == b"IDAT":
            require(b"IEND" not in seen and (seen[-1] in (b"IHDR", b"IDAT") or b"IDAT" not in seen), "invalid IDAT order")
            compressed.extend(payload)
        elif kind == b"IEND":
            require(length == 0 and b"IDAT" in seen and end == len(data), "invalid PNG end")
        else:
            require(kind != b"IHDR" and kind[0] & 32, "unsupported critical PNG chunk")
        seen.append(kind)
        offset = end
    require(seen and seen[-1] == b"IEND", "PNG end absent")
    stride = width * channels
    expected = (stride + 1) * height
    decoder = zlib.decompressobj()
    raw = decoder.decompress(bytes(compressed), expected + 1)
    require(decoder.eof and not decoder.unused_data and not decoder.unconsumed_tail and len(raw) == expected, "invalid PNG pixel stream")
    previous = bytearray(stride)
    for y in range(height):
        start = y * (stride + 1)
        method, row = raw[start], bytearray(raw[start + 1:start + 1 + stride])
        require(method <= 4, "invalid PNG filter")
        for x in range(stride):
            a, b, c = row[x - channels] if x >= channels else 0, previous[x], previous[x - channels] if x >= channels else 0
            if method == 1: predictor = a
            elif method == 2: predictor = b
            elif method == 3: predictor = (a + b) // 2
            elif method == 4:
                p = a + b - c
                distances = (abs(p - a), abs(p - b), abs(p - c))
                predictor = (a, b, c)[distances.index(min(distances))]
            else: predictor = 0
            row[x] = (row[x] + predictor) & 255
        previous = row
    return [width, height]


def validate(root, canonical, candidate, *, fail_fast=False):
    failures = []
    try:
        require(isinstance(candidate, str) and re.fullmatch(r"[0-9a-f]{40}", candidate), "--candidate requires the exact audited commit SHA")
        require(git(root, "cat-file", "-t", candidate).strip() == b"commit", "candidate must be a commit")
        registry = read_json(path(root, REGISTRY))
        ledger = read_json(path(root, LEDGER))
        for name in (REGISTRY, LEDGER):
            require(git(root, "show", f"{candidate}:{name}").replace(b"\r\n", b"\n") == path(root, name).read_bytes().replace(b"\r\n", b"\n"), "candidate inventory binding mismatch")
        expected_surfaces = {(p, s) for p, surfaces in SURFACES.items() for s in surfaces}
        registered = registry.get("surfaces")
        require(registry.get("schemaVersion") == 1 and isinstance(registered, list), "invalid surface registry")
        keys = [(r.get("product"), r.get("surface")) for r in registered if isinstance(r, dict)]
        require(len(keys) == len(registered) == len(expected_surfaces) and set(keys) == expected_surfaces, "concrete surface registry missing, duplicate or unknown surface")
        for r in registered:
            require(isinstance(r.get("route"), str) and 0 < len(r["route"]) <= 240, "surface route absent")
        rows = ledger.get("rows")
        require(ledger.get("schemaVersion") == 1 and isinstance(rows, list), "invalid evidence ledger")
        expected = {(p, s, f) for p, s in expected_surfaces for f in canonical}
        actual = [(r.get("product"), r.get("surface"), r.get("feature")) for r in rows if isinstance(r, dict)]
        require(len(actual) == len(rows) == len(expected) and set(actual) == expected, "evidence coverage missing, duplicate or unknown product/surface/feature")
    except (Invalid, OSError, ValueError, TypeError, subprocess.SubprocessError) as error:
        return [str(error)]
    used, cache, blobs = set(), {}, {}

    def ref(value, *, source=False, exclusive=False):
        require(isinstance(value, dict) and set(value) == {"path", "sha256"}, "evidence reference must be a path/hash object, not prose")
        name, checksum = value["path"], value["sha256"]
        require(isinstance(checksum, str) and SHA.fullmatch(checksum), "invalid SHA-256")
        file = path(root, name)
        require(file.stat().st_size <= 64 * 1024 * 1024, "oversized evidence file")
        if exclusive:
            require(file not in used, "receipt or capture reuse across evidence rows")
            used.add(file)
        if file not in cache: cache[file] = file.read_bytes()
        data = cache[file]
        require(digest(data) == checksum, "evidence hash mismatch")
        if source:
            if name not in blobs: blobs[name] = git(root, "show", f"{candidate}:{name}")
            require(blobs[name].replace(b"\r\n", b"\n") == data.replace(b"\r\n", b"\n"), "source differs from audited candidate")
        return file

    routes = {(r["product"], r["surface"]): r["route"] for r in registered}
    for row in rows:
        key = {k: row[k] for k in ("product", "surface", "feature")}
        label = "/".join(key.values())
        try:
            require(row.get("status") == "implemented", "delivery is unverified")
            # The committed ledger declares output locations, not output hashes.
            # Receipts are created AFTER the candidate commit, avoiding a SHA cycle.
            proof_path = f".verification/completeness/{key['product']}/{key['surface']}/feature-{canonical.index(key['feature']) + 1:02d}.json"
            require(row.get("evidence") == proof_path, "exact evidence descriptor path required")
            proof = read_json(path(root, proof_path))
            require(proof.get("key") == key and proof.get("sourceCommit") == candidate, "descriptor candidate/key mismatch")
            row = {**key, **proof}
            for field in ("implementation", "documentation", "localized", "persistence", "testSource"):
                ref(row.get(field), source=True)
            expected_tuple = row.get("tuple")
            require(isinstance(expected_tuple, dict) and set(expected_tuple) == {"product", "surface", "route", "state", "theme", "language", "viewport", "scale"}, "exact capture tuple required")
            require(expected_tuple["product"] == key["product"] and expected_tuple["surface"] == key["surface"] and expected_tuple["route"] == routes[(key["product"], key["surface"])], "capture tuple surface/route mismatch")
            require(expected_tuple["theme"] in ("light", "dark") and expected_tuple["language"] in ("en", "yue", "bilingual"), "invalid capture theme/language")
            require(isinstance(expected_tuple["state"], str) and re.fullmatch(r"[a-z0-9-]{1,80}", expected_tuple["state"]), "invalid capture state")
            viewport, scale = expected_tuple["viewport"], expected_tuple["scale"]
            require(isinstance(viewport, list) and len(viewport) == 2 and all(type(n) is int and 0 < n <= 8192 for n in viewport), "invalid viewport")
            require(type(scale) in (int, float) and scale in (1, 1.25, 1.5, 2), "invalid display scale")
            build = read_json(ref(row.get("build")))
            require(build.get("schemaVersion") == 1 and build.get("sourceCommit") == candidate and build.get("product") == key["product"], "build candidate/product mismatch")
            ref(build.get("artifact"))
            version = build.get("version")
            require(isinstance(version, str) and re.fullmatch(r"[0-9]+\.[0-9]+\.[0-9]+(?:[.+-][A-Za-z0-9.-]+)?", version), "build version provenance missing")
            require(isinstance(build.get("updatedAt"), str) and re.fullmatch(r"\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z", build["updatedAt"]), "build timestamp provenance missing")
            datetime.strptime(build["updatedAt"], "%Y-%m-%dT%H:%M:%SZ")
            provenance = read_json(ref(build.get("versionMetadata")))
            require(provenance == {"sourceCommit": candidate, "version": version, "updatedAt": build["updatedAt"], "artifact": build["artifact"]}, "build version metadata mismatch")
            records = {}
            for field in ("testResult", "interaction", "captureReceipt"):
                record = read_json(ref(row.get(field), exclusive=True))
                require(record.get("schemaVersion") == 1 and record.get("key") == key, f"{field} evidence key mismatch")
                require(record.get("sourceCommit") == candidate and record.get("build") == row["build"], f"{field} stale candidate/build mismatch")
                require(record.get("tuple") == expected_tuple, f"{field} tuple mismatch")
                records[field] = record
            test = records["testResult"]
            require(test.get("testSource") == row["testSource"] and test.get("result") == "passed", "focused test result/source mismatch")
            cases = test.get("cases")
            require(isinstance(cases, list) and cases and all(isinstance(c, dict) and isinstance(c.get("id"), str) and c["id"] and c.get("result") == "passed" for c in cases), "focused test cases absent or failed")
            require(len({c["id"] for c in cases}) == len(cases), "duplicate test case")
            output = read_json(ref(test.get("output")))
            require(output.get("cases") == cases, "test output does not match asserted cases")
            interaction = records["interaction"]
            require(interaction.get("method") == "lowlevel-headless" and interaction.get("testResult") == row["testResult"], "interaction execution/test linkage missing")
            steps = interaction.get("steps")
            require(isinstance(steps, list) and steps, "built interaction steps absent")
            for step in steps:
                require(isinstance(step, dict) and all(isinstance(step.get(f), str) and 0 < len(step[f]) <= 500 for f in ("before", "target", "input", "expected", "after")), "incomplete interaction step")
                require(step["expected"] == step["after"], "interaction expected/actual state mismatch")
            capture = ref(row.get("capture"), exclusive=True)
            capture_identity = ("capture-content", row["capture"]["sha256"])
            require(capture_identity not in used, "capture content reuse across evidence rows")
            used.add(capture_identity)
            dimensions = png_dimensions(cache[capture])
            require(dimensions == [round(n * scale) for n in viewport], "decoded capture dimensions/tuple mismatch")
            receipt = records["captureReceipt"]
            require(receipt.get("capture") == row["capture"] and receipt.get("interaction") == row["interaction"] and steps[-1].get("capture") == row["capture"], "capture interaction linkage mismatch")
            require(receipt.get("dimensions") == dimensions, "receipt dimensions mismatch")
            privacy = receipt.get("privacy")
            require(isinstance(privacy, dict) and privacy.get("verdict") == "passed" and privacy.get("method") == "isolated-profile-review", "privacy review absent")
            review = read_json(ref(privacy.get("review")))
            require(review.get("verdict") == "passed" and review.get("key") == key and review.get("sourceCommit") == candidate and review.get("capture") == row["capture"], "privacy review evidence mismatch")
        except (Invalid, OSError, ValueError, TypeError, KeyError, struct.error, zlib.error, subprocess.SubprocessError) as error:
            failures.append(f"{label}: {error}")
            if fail_fast:
                return failures
    return failures
