#!/usr/bin/env python3
"""Validate handwritten feature and per-surface completeness inventories.

Default and ``--strict`` modes validate inventory integrity only, so ordinary
configure, build, and release work can report an honest backlog. ``--completion``
is the separate fail-closed delivery verdict. It requires full feature delivery
and all per-surface evidence without inferring proof from source discovery.
"""
from __future__ import annotations

import argparse
import json
import re
import sys
from dataclasses import dataclass
from pathlib import Path

INVENTORY = "docs/inventory/completeness-inventory.md"
SURFACES = "docs/inventory/per-surface-completeness.md"
TRANSLATION = "share/locale/audacity_yue_HK.ts"
MATRIX = "docs/inventory/product-surface-matrix.md"
PRODUCT_SURFACES = ("Desktop application", "Documentation site")
CANONICAL = [
    "Language modes", "Funny level, English", "Funny level, Cantonese", "Emoji switch", "School mode", "Narrator (TTS)", "Scheduled settings", "External settings sources (Home Assistant)", "Dim sum surprise", "Regex builder", "Notifications", "Material 3 appearance and per element editor", "Tabs, groups and tab search", "Landing page and offline docs", "Command palette", "Destructive action super confirmation", "Local version history", "Changelog viewer", "External editor handoff", "Universal export", "Bulk actions", "Accessibility (keyboard, focus, names, contrast)", "Responsive sizing (narrow widths, 200% scale)", "Personal vocabulary JSON upload", "Toy locks", "Support Tickets", "Browser extension download capture dialogs", "Unlock ladder", "Shared link embed graphic", "ADHD modes (attention support)", "App logo customization", "Universal file converter", "Local model manager (Ollama suite)", "Version and build time on the front screen", "Automatic updates", "Status Hub row", "Docs browser bookmark export/bulk", "Renaming the application", "Built in authenticator (TOTP)",
]
VALID = {"implemented", "partial", "missing", "not applicable"}
UNVERIFIED = {"", "(none yet)", "(none)", "n/a", "unverified", "not captured", "not run"}


@dataclass
class FeatureRow:
    feature: str; implementation: str; documentation: str; localized: str; persistence: str; test: str; capture: str; status: str; notes: str; line_no: int


@dataclass
class SurfaceRow:
    surface: str; feature: str; implementation: str; documentation: str; localized: str; persistence: str; test: str; interaction: str; capture: str; provenance: str; status: str; notes: str; line_no: int


@dataclass
class MatrixRow:
    product_surface: str; feature: str; status: str; notes: str; line_no: int


def table(text: str, header: str) -> list[tuple[list[str], int]]:
    """Read one exact table header, never a descendant or substring match."""
    rows, active = [], False
    for line_no, raw in enumerate(text.splitlines(), 1):
        line = raw.strip()
        if not line.startswith("|"):
            if active: break
            continue
        cells = [cell.strip() for cell in line.strip("|").split("|")]
        if cells and cells[0] == header:
            active = True; continue
        if not active: continue
        if cells and all(re.fullmatch(r"[: -]+", cell or "") for cell in cells): continue
        rows.append((cells, line_no))
    return rows


def feature_rows(text: str) -> list[FeatureRow]:
    return [FeatureRow(*(cells + [""] * (9 - len(cells)))[:9], line_no) for cells, line_no in table(text, "Feature")]


def surface_rows(text: str) -> list[SurfaceRow]:
    return [SurfaceRow(*(cells + [""] * (12 - len(cells)))[:12], line_no) for cells, line_no in table(text, "Surface")]


def matrix_rows(text: str) -> list[MatrixRow]:
    return [MatrixRow(*(cells + [""] * (4 - len(cells)))[:4], line_no) for cells, line_no in table(text, "Product surface")]


def paths(value: str) -> list[str]: return re.findall(r"`([^`]+)`", value)
def pathlike(value: str) -> bool: return "/" in value or ("." in value and not value.startswith("<"))
def unverified(value: str) -> bool: return value.strip().lower() in UNVERIFIED


def check_paths(value: str, root: Path, field: str, label: str, line_no: int, failures: list[str]) -> None:
    for name in paths(value):
        if pathlike(name) and not (root / name).exists():
            failures.append(f"line {line_no}, {label}: {field} references '{name}' which does not exist")


def exactly_once(rows: list, label: str, failures: list[str]) -> None:
    counts: dict[str, int] = {}
    for row in rows:
        if row.feature: counts[row.feature] = counts.get(row.feature, 0) + 1
    for feature in CANONICAL:
        if counts.get(feature, 0) != 1:
            failures.append(f"{label}: canonical feature '{feature}' has {counts.get(feature, 0)} rows, expected exactly one")
    for feature, count in counts.items():
        if feature not in CANONICAL: failures.append(f"{label}: non-canonical feature '{feature}' has {count} rows")


def matrix_exactly_once(rows: list[MatrixRow], failures: list[str]) -> None:
    counts: dict[tuple[str, str], int] = {}
    for row in rows:
        key = (row.product_surface, row.feature)
        counts[key] = counts.get(key, 0) + 1
        if row.product_surface not in PRODUCT_SURFACES:
            failures.append(f"line {row.line_no}, matrix: unknown product surface '{row.product_surface}'")
        if row.feature not in CANONICAL:
            failures.append(f"line {row.line_no}, matrix: non-canonical feature '{row.feature}'")
        if row.status not in VALID:
            failures.append(f"line {row.line_no}, matrix: unrecognised status '{row.status}'")
        if row.status in {"missing", "partial", "not applicable"} and not row.notes:
            failures.append(f"line {row.line_no}, matrix: incomplete row requires Notes")
    for product_surface in PRODUCT_SURFACES:
        for feature in CANONICAL:
            count = counts.get((product_surface, feature), 0)
            if count != 1:
                failures.append(f"product-surface matrix: '{product_surface}' / '{feature}' has {count} rows, expected exactly one")


def context_check(value: str, translation: str, label: str, line_no: int, failures: list[str]) -> None:
    match = re.search(r"context\s+`([^`]+)`", value)
    if match and f"<name>{match.group(1)}</name>" not in translation:
        failures.append(f"line {line_no}, {label}: localized context '{match.group(1)}' is absent")


def validate_receipt(row: SurfaceRow, root: Path, label: str, failures: list[str]) -> None:
    """Require a machine-readable receipt when a row claims a real capture."""
    capture_paths, receipt_paths = paths(row.capture), paths(row.provenance)
    if not capture_paths and not receipt_paths:
        return
    if len(capture_paths) != 1 or len(receipt_paths) != 1:
        failures.append(f"line {row.line_no}, {label}: capture and provenance must each name one path")
        return
    capture, receipt = root / capture_paths[0], root / receipt_paths[0]
    if not capture.exists() or not receipt.exists():
        return
    try:
        value = json.loads(receipt.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        failures.append(f"line {row.line_no}, {label}: invalid capture receipt: {error}")
        return
    required_tuple = {"surface", "theme", "language", "viewport", "scale"}
    if value.get("capture") != capture_paths[0]: failures.append(f"line {row.line_no}, {label}: receipt capture path does not match")
    if not re.fullmatch(r"[0-9a-f]{40}", str(value.get("sourceCommit", ""))): failures.append(f"line {row.line_no}, {label}: receipt sourceCommit must be a full SHA")
    if not isinstance(value.get("tuple"), dict) or not required_tuple <= set(value["tuple"]): failures.append(f"line {row.line_no}, {label}: receipt tuple is incomplete")
    if value.get("privacy") is not True: failures.append(f"line {row.line_no}, {label}: receipt privacy must be true")
    if value.get("current") is not True: failures.append(f"line {row.line_no}, {label}: receipt current must be true")


def validate_features(rows: list[FeatureRow], root: Path, translation: str, failures: list[str]) -> None:
    for row in rows:
        label = f"feature '{row.feature}'"
        if row.status not in VALID:
            failures.append(f"line {row.line_no}, {label}: unrecognised status '{row.status}'"); continue
        if row.status in {"missing", "not applicable"} and not row.notes:
            failures.append(f"line {row.line_no}, {label}: {row.status} requires Notes")
        if row.status in {"implemented", "partial"}:
            for field, value in (("implementation", row.implementation), ("documentation", row.documentation), ("test", row.test), ("capture", row.capture)):
                check_paths(value, root, field, label, row.line_no, failures)
            context_check(row.localized, translation, label, row.line_no, failures)


def validate_surfaces(rows: list[SurfaceRow], root: Path, translation: str, failures: list[str]) -> None:
    for row in rows:
        label = f"surface '{row.surface}', feature '{row.feature}'"
        if not row.surface: failures.append(f"line {row.line_no}, per-surface inventory: Surface is empty")
        if row.status not in VALID:
            failures.append(f"line {row.line_no}, {label}: unrecognised status '{row.status}'"); continue
        if row.status in {"missing", "not applicable"} and not row.notes:
            failures.append(f"line {row.line_no}, {label}: {row.status} requires Notes")
        for field, value in (("implementation", row.implementation), ("documentation", row.documentation), ("test", row.test), ("capture", row.capture), ("capture provenance", row.provenance)):
            check_paths(value, root, field, label, row.line_no, failures)
        context_check(row.localized, translation, label, row.line_no, failures)
        validate_receipt(row, root, label, failures)


def completion_rows(features: list[FeatureRow], surfaces: list[SurfaceRow], matrix: list[MatrixRow]) -> list[str]:
    failures = [f"feature '{row.feature}' is '{row.status}', so completion is not proven" for row in features if row.status != "implemented"]
    failures.extend(f"product surface '{row.product_surface}', feature '{row.feature}' is '{row.status}', so completion is not proven" for row in matrix if row.status != "implemented")
    for row in surfaces:
        label = f"surface '{row.surface}', feature '{row.feature}'"
        if row.status != "implemented":
            failures.append(f"{label} is '{row.status}', so completion is not proven"); continue
        for field, value in {"implementation": row.implementation, "documentation": row.documentation, "localized copy": row.localized, "persistence": row.persistence, "focused test": row.test, "real built-artifact interaction": row.interaction, "capture": row.capture, "capture provenance": row.provenance}.items():
            if unverified(value): failures.append(f"{label}: {field} is unverified or absent")
    return failures


def run(root: Path, completion: bool) -> tuple[list[str], list[str]]:
    failures, notices = [], []
    feature_path, surface_path, matrix_path = root / INVENTORY, root / SURFACES, root / MATRIX
    if not feature_path.exists(): return [f"feature inventory missing at {feature_path}"], notices
    if not surface_path.exists(): return [f"per-surface inventory missing at {surface_path}"], notices
    if not matrix_path.exists(): return [f"product-surface matrix missing at {matrix_path}"], notices
    translation_path = root / TRANSLATION
    translation = translation_path.read_text(encoding="utf-8") if translation_path.exists() else ""
    if not translation: failures.append(f"translation file missing or empty at {translation_path}")
    features, surfaces = feature_rows(feature_path.read_text(encoding="utf-8")), surface_rows(surface_path.read_text(encoding="utf-8"))
    matrix = matrix_rows(matrix_path.read_text(encoding="utf-8"))
    exactly_once(features, "feature inventory", failures); exactly_once(surfaces, "per-surface inventory", failures)
    validate_features(features, root, translation, failures); validate_surfaces(surfaces, root, translation, failures)
    matrix_exactly_once(matrix, failures)
    notices = [f"{row.feature}: {row.status}" for row in features]
    if completion: failures.extend(completion_rows(features, surfaces, matrix))
    return failures, notices


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root"); parser.add_argument("--strict", action="store_true"); parser.add_argument("--completion", action="store_true")
    args = parser.parse_args(); root = Path(args.repo_root).resolve() if args.repo_root else Path(__file__).resolve().parents[2]
    failures, notices = run(root, args.completion); mode = "completion" if args.completion else "report-only"
    print(f"Completeness inventory guard ({mode}): {root}"); print(f"Checked {len(CANONICAL)} canonical features in two independent inventories.")
    for notice in notices: print(f"  - {notice}")
    if failures:
        print("\nFAILURES:"); [print(f"  ! {failure}") for failure in failures]
        if args.strict or args.completion: return 1
        print("\nReport-only mode never claims completion and exits zero. Use --strict for integrity or --completion for delivery proof.")
        return 0
    print("\nNo failures."); return 0


if __name__ == "__main__": sys.exit(main())
