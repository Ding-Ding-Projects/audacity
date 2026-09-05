#!/usr/bin/env python3
"""Fail closed guard over docs/inventory/completeness-inventory.md.

This script never invents evidence. It parses the hand written inventory
table, checks that every canonical feature named below has exactly one row,
and for every row whose status is "implemented" or "partial" verifies that
every path the row claims (implementation files, the documentation article,
the test file, and the capture path) actually exists on disk, and that any
claimed localized copy context actually appears in the Cantonese
translation file. A row whose status is "missing" or "not applicable" is
reported but does not fail the guard, as long as it carries a reason in the
Notes column; the guard's job is to keep the inventory honest today, not to
pretend every canonical feature is already built.

Usage:
    python3 completeness_guard.py [--repo-root PATH] [--strict]

Exit code is non zero, naming the exact failing row and field, whenever:
  - a canonical feature has no row at all, or more than one row;
  - an implemented or partial row references a file, doc, test or capture
    path that does not exist relative to the repository root;
  - an implemented or partial row claims a localized context that the
    Cantonese translation file does not carry;
  - a missing or not-applicable row has an empty Notes column.

--strict is accepted for the CMake AU_COMPLETENESS_STRICT switch; without it
the script still prints every finding but always exits 0, matching the
"warning only unless strict" contract in buildscripts/cmake/CompletenessInventory.cmake.
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

INVENTORY_RELATIVE_PATH = "docs/inventory/completeness-inventory.md"
TRANSLATION_RELATIVE_PATH = "share/locale/audacity_yue_HK.ts"

# The hand written list of every canonical feature this project must carry a
# row for. This list is intentionally not derived from the inventory file
# itself: if it were, a deleted row would simply vanish from both sides and
# the guard would report nothing wrong, which is exactly the failure mode a
# completeness guard exists to prevent.
CANONICAL_FEATURES = [
    "Language modes",
    "Funny level, English",
    "Funny level, Cantonese",
    "Emoji switch",
    "School mode",
    "Narrator (TTS)",
    "Scheduled settings",
    "External settings sources (Home Assistant)",
    "Dim sum surprise",
    "Regex builder",
    "Notifications",
    "Material 3 appearance and per element editor",
    "Tabs, groups and tab search",
    "Landing page and offline docs",
    "Command palette",
    "Destructive action super confirmation",
    "Local version history",
    "Changelog viewer",
    "External editor handoff",
    "Universal export",
    "Bulk actions",
    "Accessibility (keyboard, focus, names, contrast)",
    "Responsive sizing (narrow widths, 200% scale)",
    "Personal vocabulary JSON upload",
    "Toy locks",
    "Support Tickets",
    "Browser extension download capture dialogs",
    "Unlock ladder",
    "Shared link embed graphic",
    "ADHD modes (attention support)",
    "App logo customization",
    "Universal file converter",
    "Local model manager (Ollama suite)",
    "Version and build time on the front screen",
    "Automatic updates",
    "Status Hub row",
    "Docs browser bookmark export/bulk",
    "Renaming the application",
    "Built in authenticator (TOTP)",
]

STATUS_IMPLEMENTED = "implemented"
STATUS_PARTIAL = "partial"
STATUS_MISSING = "missing"
STATUS_NOT_APPLICABLE = "not applicable"
VALID_STATUSES = {STATUS_IMPLEMENTED, STATUS_PARTIAL, STATUS_MISSING, STATUS_NOT_APPLICABLE}


class Row:
    def __init__(self, cells: list[str], line_no: int):
        # feature | implementation | documentation | localized copy |
        # persistence | test | capture | status | notes
        while len(cells) < 9:
            cells.append("")
        self.feature = cells[0].strip()
        self.implementation = cells[1].strip()
        self.documentation = cells[2].strip()
        self.localized = cells[3].strip()
        self.persistence = cells[4].strip()
        self.test = cells[5].strip()
        self.capture = cells[6].strip()
        self.status = cells[7].strip().lower()
        self.notes = cells[8].strip()
        self.line_no = line_no


def parse_inventory(text: str) -> list[Row]:
    rows: list[Row] = []
    in_table = False
    for line_no, raw_line in enumerate(text.splitlines(), start=1):
        line = raw_line.strip()
        if not line.startswith("|"):
            continue
        if line.startswith("| Feature |"):
            in_table = True
            continue
        if not in_table:
            continue
        # Skip the separator row, e.g. "| --- | --- | ... |"
        if re.fullmatch(r"\|[\s:-]*\|[\s:|-]*", line):
            continue
        cells = [c.strip() for c in line.strip("|").split("|")]
        rows.append(Row(cells, line_no))
    return rows


def extract_paths(cell: str) -> list[str]:
    """Pull backtick-quoted paths out of a cell. Ignores prose in parens."""
    return re.findall(r"`([^`]+)`", cell)


def looks_like_path(token: str) -> bool:
    # A backtick span can also be a settings key, a context name, or a code
    # symbol; only check tokens that look like a real repository path.
    if token.startswith("<"):
        return False
    if "/" not in token and "." not in token:
        return False
    if token.startswith("experience/") and not token.endswith((".cpp", ".h", ".qml", ".md", ".png", ".log", ".json")):
        # a settings key such as experience/language/mode, not a path
        return False
    return True


def check_paths_exist(cell_value: str, repo_root: Path, field_name: str, row: Row, findings: list[str]) -> None:
    for token in extract_paths(cell_value):
        if not looks_like_path(token):
            continue
        candidate = repo_root / token
        if not candidate.exists():
            findings.append(
                f"line {row.line_no}, feature '{row.feature}': {field_name} references "
                f"'{token}' which does not exist at {candidate}"
            )


def check_localized_context(row: Row, translation_text: str, findings: list[str]) -> None:
    if not row.localized:
        return
    match = re.search(r"context\s+`([^`]+)`", row.localized)
    if not match:
        return
    context_name = match.group(1)
    needle = f"<name>{context_name}</name>"
    if needle not in translation_text:
        findings.append(
            f"line {row.line_no}, feature '{row.feature}': localized copy claims context "
            f"'{context_name}' which does not appear in {TRANSLATION_RELATIVE_PATH}"
        )


def run(repo_root: Path) -> tuple[list[str], list[str]]:
    """Returns (failures, notices)."""
    failures: list[str] = []
    notices: list[str] = []

    inventory_path = repo_root / INVENTORY_RELATIVE_PATH
    if not inventory_path.exists():
        failures.append(f"inventory file missing at {inventory_path}")
        return failures, notices

    text = inventory_path.read_text(encoding="utf-8")
    rows = parse_inventory(text)

    translation_path = repo_root / TRANSLATION_RELATIVE_PATH
    translation_text = translation_path.read_text(encoding="utf-8") if translation_path.exists() else ""
    if not translation_text:
        failures.append(f"translation file missing or empty at {translation_path}")

    seen_features: dict[str, int] = {}
    for row in rows:
        if not row.feature:
            continue
        seen_features[row.feature] = seen_features.get(row.feature, 0) + 1

    for feature in CANONICAL_FEATURES:
        count = seen_features.get(feature, 0)
        if count == 0:
            failures.append(f"canonical feature '{feature}' has no row in {INVENTORY_RELATIVE_PATH}")
        elif count > 1:
            failures.append(f"canonical feature '{feature}' has {count} rows in {INVENTORY_RELATIVE_PATH}, expected exactly one")

    for row in rows:
        if not row.feature:
            continue
        if row.status not in VALID_STATUSES:
            failures.append(f"line {row.line_no}, feature '{row.feature}': unrecognised status '{row.status}'")
            continue

        if row.status in (STATUS_MISSING, STATUS_NOT_APPLICABLE):
            if not row.notes:
                failures.append(
                    f"line {row.line_no}, feature '{row.feature}': status is "
                    f"'{row.status}' but the Notes column gives no reason"
                )
            else:
                notices.append(f"{row.feature}: {row.status} ({row.notes[:80]}...)" if len(row.notes) > 80 else f"{row.feature}: {row.status} ({row.notes})")
            continue

        # implemented or partial: every referenced path must be real.
        check_paths_exist(row.implementation, repo_root, "implementation", row, failures)
        check_paths_exist(row.documentation, repo_root, "documentation", row, failures)
        check_paths_exist(row.test, repo_root, "test", row, failures)
        check_paths_exist(row.capture, repo_root, "capture", row, failures)
        check_localized_context(row, translation_text, failures)

        notices.append(f"{row.feature}: {row.status}")

    return failures, notices


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", default=None, help="Repository root; defaults to three levels above this script.")
    parser.add_argument("--strict", action="store_true", help="Exit non-zero on any failure. Without this, failures are printed but exit is always 0.")
    args = parser.parse_args()

    if args.repo_root:
        repo_root = Path(args.repo_root).resolve()
    else:
        repo_root = Path(__file__).resolve().parents[2]

    failures, notices = run(repo_root)

    print(f"Completeness inventory guard: {repo_root / INVENTORY_RELATIVE_PATH}")
    print(f"Checked {len(CANONICAL_FEATURES)} canonical features.")
    for notice in notices:
        print(f"  - {notice}")

    if failures:
        print("\nFAILURES:")
        for failure in failures:
            print(f"  ! {failure}")
        if args.strict:
            return 1
        print("\n(not failing: run with --strict, or set AU_COMPLETENESS_STRICT=ON, to make this exit non-zero)")
        return 0

    print("\nNo failures.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
