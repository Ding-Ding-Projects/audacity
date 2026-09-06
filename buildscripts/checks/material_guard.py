#!/usr/bin/env python3
"""Audacity: A Digital Audio Editor

material_guard.py

Scans every src/**/qml/**/*.qml file for legacy Muse control instantiations,
hard coded colour and radius literals, and ui.theme.* colour reads outside
the Audacity.M3 library and the theme provider itself, then fails when a hit
is not allow listed with a reason in docs/inventory/material-audit.md.

The goal is that every rendered element is a Material Design 3 primitive
from Audacity.M3, or a Muse control restyled by one of the numbered patches
under buildscripts/muse-patches so that it renders with Material 3 anatomy.
Functional data colours (waveform, clip, meter and spectrogram colours) are
data, not chrome, and are excluded by directory or by an explicit allow list
reason rather than by guessing at variable names.

Usage:
    python3 material_guard.py --repo-root <path> [--strict]

Exit code is always 0 unless --strict is given and a finding remains; a
finding is always printed regardless of --strict so a warning-only configure
run still shows the work remaining.
"""

from __future__ import annotations

import argparse
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path

# Legacy Muse control identifiers this project restyles either by using an
# Audacity.M3 component instead, or by relying on the overlay patch under
# buildscripts/muse-patches that already redraws the Muse control with
# Material 3 anatomy. Matched only when instantiated (identifier followed by
# an opening brace), so a wrapper component whose *name* merely contains one
# of these words (e.g. "IncrementalPropertyControlWithTitle") is not a false
# positive.
LEGACY_IDENTIFIERS = [
    "FlatButton",
    "FlatToggleButton",
    "RoundedRadioButton",
    "RadioButton",
    "CheckBox",
    "StyledSlider",
    "StyledDropdown",
    "Dropdown",
    "TextInputField",
    "SearchField",
    "StyledTabBar",
    "StyledTabButton",
    "StyledMenu",
    "StyledMenuItem",
    "StyledPopupView",
    "StyledDialogView",
    "StyledToolTip",
    "ToolTip",
    "ListItemBlank",
    "StyledListView",
    "StyledTableView",
    "MenuButton",
    "ValueList",
    "FilePicker",
    "ColorPicker",
    "KnobControl",
    "IncrementalPropertyControl",
    "SpinBox",
    "ProgressBar",
    "StyledBusyIndicator",
    "BusyIndicator",
    "Switch",
]

# Directories whose QML defines the Material 3 primitives, or the theme
# provider they are built from. Legacy identifiers and raw colour/theme
# reads inside these are the library's own implementation, not a product
# surface using a legacy control, so they are never scanned.
EXCLUDED_DIR_PARTS = (
    ("uicomponents", "qml", "Audacity", "M3"),
)

QML_GLOB = "*.qml"

LEGACY_RE = re.compile(
    r"\b(" + "|".join(re.escape(name) for name in LEGACY_IDENTIFIERS) + r")\s*\{"
)
COLOR_LITERAL_RE = re.compile(r'color\s*:\s*"#[0-9A-Fa-f]{3,8}"')
RADIUS_LITERAL_RE = re.compile(r"\bradius\s*:\s*[0-9]")
UI_THEME_RE = re.compile(r"\bui\.theme\.")

# The real inventory table's exact column count and headings vary a little
# between lanes' sections (some read "File | Legacy usage | M3 replacement /
# disposition | Status", others add a reason column), so rows are parsed
# generically: split on "|", take the first non-empty cell as the file path,
# the last non-empty cell as the status, and the whole line as the haystack
# that a finding's identifier or kind is matched against. This is more
# permissive than a fixed column count, which is what lets one guard read
# every lane's section without them agreeing on exact column names.
FILE_CELL_RE = re.compile(r"`?([^`]+\.(?:qml))`?")


@dataclass
class Finding:
    path: Path
    line_no: int
    identifier: str
    kind: str


@dataclass
class AllowlistEntry:
    file: str
    status: str
    haystack: str


@dataclass
class GuardResult:
    findings: list = field(default_factory=list)
    allowlisted: list = field(default_factory=list)


def is_excluded(rel_path: Path) -> bool:
    parts = rel_path.parts
    for excluded in EXCLUDED_DIR_PARTS:
        n = len(excluded)
        for i in range(len(parts) - n + 1):
            if tuple(parts[i:i + n]) == excluded:
                return True
    return False


def iter_qml_files(repo_root: Path):
    src_root = repo_root / "src"
    if not src_root.is_dir():
        return
    for path in sorted(src_root.rglob(QML_GLOB)):
        if "qml" not in path.parts:
            continue
        rel = path.relative_to(repo_root)
        if is_excluded(rel):
            continue
        yield path


def scan_file(path: Path) -> list:
    findings = []
    try:
        text = path.read_text(encoding="utf-8")
    except (UnicodeDecodeError, OSError):
        return findings

    for line_no, line in enumerate(text.splitlines(), start=1):
        stripped = line.strip()
        if stripped.startswith("//") or stripped.startswith("*"):
            continue

        for match in LEGACY_RE.finditer(line):
            findings.append(Finding(path, line_no, match.group(1), "legacy-control"))

        if COLOR_LITERAL_RE.search(line):
            findings.append(Finding(path, line_no, "color-literal", "color-literal"))

        if RADIUS_LITERAL_RE.search(line):
            findings.append(Finding(path, line_no, "radius-literal", "radius-literal"))

        if UI_THEME_RE.search(line):
            findings.append(Finding(path, line_no, "ui.theme", "ui-theme-read"))

    return findings


def parse_allowlist(inventory_path: Path) -> list:
    entries = []
    if not inventory_path.exists():
        return entries

    text = inventory_path.read_text(encoding="utf-8")
    in_table = False
    for line in text.splitlines():
        if line.strip().startswith("| File") or line.strip().startswith("| file"):
            in_table = True
            continue
        if in_table and set(line.strip()) <= {"|", "-", " ", ":"}:
            continue
        if not line.strip().startswith("|"):
            in_table = False
            continue
        if not in_table:
            continue

        stripped_line = line.strip()
        cells = [cell.strip() for cell in stripped_line.strip("|").split("|")]
        cells = [c for c in cells if c != ""]
        if len(cells) < 2:
            continue

        file_match = FILE_CELL_RE.search(cells[0])
        if not file_match:
            # A row whose first cell is not a file path (a section note, a
            # continuation row) carries no allow-list information.
            continue

        file_cell = file_match.group(1).strip().strip("`")
        status_cell = cells[-1].strip().lower()

        entries.append(
            AllowlistEntry(
                file=file_cell,
                status=status_cell,
                haystack=stripped_line,
            )
        )
    return entries


def _row_covers_finding(entry: AllowlistEntry, finding: Finding) -> bool:
    if not (entry.status.startswith("kept") or entry.status.startswith("converted")):
        return False

    haystack = entry.haystack

    if finding.kind == "legacy-control":
        return re.search(r"\b" + re.escape(finding.identifier) + r"\b", haystack) is not None

    if finding.kind == "ui-theme-read":
        return "ui.theme" in haystack

    if finding.kind == "radius-literal":
        return "radius" in haystack.lower()

    if finding.kind == "color-literal":
        lowered = haystack.lower()
        return "color" in lowered and ("#" in haystack or "hex" in lowered)

    return False


def allowlist_covers(entries: list, rel_path: str, finding: Finding) -> bool:
    for entry in entries:
        if entry.file != rel_path:
            continue
        if _row_covers_finding(entry, finding):
            return True
    return False


def run_guard(repo_root: Path, inventory_path: Path) -> GuardResult:
    allowlist = parse_allowlist(inventory_path)
    result = GuardResult()

    for path in iter_qml_files(repo_root):
        rel_path = str(path.relative_to(repo_root))
        for finding in scan_file(path):
            if allowlist_covers(allowlist, rel_path, finding):
                result.allowlisted.append(finding)
            else:
                result.findings.append(finding)

    return result


def format_finding(repo_root: Path, finding: Finding) -> str:
    rel_path = finding.path.relative_to(repo_root)
    return f"{rel_path}:{finding.line_no}: {finding.kind} `{finding.identifier}` is not allow listed in docs/inventory/material-audit.md"


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo-root", required=True, type=Path)
    parser.add_argument("--strict", action="store_true")
    parser.add_argument(
        "--inventory",
        type=Path,
        default=None,
        help="Override the inventory markdown path (defaults to docs/inventory/material-audit.md under repo-root)",
    )
    args = parser.parse_args(argv)

    repo_root = args.repo_root.resolve()
    inventory_path = args.inventory or (repo_root / "docs" / "inventory" / "material-audit.md")

    result = run_guard(repo_root, inventory_path)

    print(f"Material Design 3 audit guard: scanned src/**/qml/**/*.qml under {repo_root}")
    print(f"  allow listed (kept or converted, documented): {len(result.allowlisted)}")
    print(f"  unaddressed findings: {len(result.findings)}")

    if result.findings:
        print("")
        print("Unaddressed findings (add a row to docs/inventory/material-audit.md, or convert):")
        for finding in result.findings:
            print(f"  {format_finding(repo_root, finding)}")

    if result.findings:
        if args.strict:
            return 1
        return 0

    return 0


if __name__ == "__main__":
    sys.exit(main())
