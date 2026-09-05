#!/usr/bin/env python3
"""Negative regression for completeness_guard.py.

Proves the guard actually catches something before anybody trusts it: it
copies the real repository tree's inventory and translation file into a
temporary directory, breaks one thing at a time (removes a row, removes a
referenced implementation file, removes a referenced doc, removes a
referenced capture, removes a localized context), and asserts the guard
turns red (--strict) for each break; then it asserts the untouched copy is
green. Run directly with `python3 test_completeness_guard.py`.
"""
from __future__ import annotations

import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[2]
GUARD_SCRIPT = Path(__file__).resolve().parent / "completeness_guard.py"
INVENTORY_RELATIVE_PATH = "docs/inventory/completeness-inventory.md"
TRANSLATION_RELATIVE_PATH = "share/locale/audacity_yue_HK.ts"


def run_guard(repo_root: Path) -> tuple[int, str]:
    result = subprocess.run(
        [sys.executable, str(GUARD_SCRIPT), "--repo-root", str(repo_root), "--strict"],
        capture_output=True,
        text=True,
        check=False,
    )
    return result.returncode, result.stdout + result.stderr


def make_sandbox() -> Path:
    """Builds a temp copy of just the files the guard reads."""
    sandbox = Path(tempfile.mkdtemp(prefix="au-completeness-guard-"))

    inventory_src = REPO_ROOT / INVENTORY_RELATIVE_PATH
    inventory_dst = sandbox / INVENTORY_RELATIVE_PATH
    inventory_dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(inventory_src, inventory_dst)

    translation_src = REPO_ROOT / TRANSLATION_RELATIVE_PATH
    translation_dst = sandbox / TRANSLATION_RELATIVE_PATH
    translation_dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(translation_src, translation_dst)

    # Copy every path the inventory references, so the "everything real"
    # baseline is genuinely green before anything is broken.
    text = inventory_src.read_text(encoding="utf-8")
    import re

    for token in re.findall(r"`([^`]+)`", text):
        if token.startswith("<") or ("/" not in token and "." not in token):
            continue
        if token.startswith("experience/") and not token.endswith((".cpp", ".h", ".qml", ".md", ".png", ".log", ".json")):
            continue
        if token in (INVENTORY_RELATIVE_PATH, TRANSLATION_RELATIVE_PATH):
            continue
        source = REPO_ROOT / token
        if not source.exists():
            continue
        dest = sandbox / token
        dest.parent.mkdir(parents=True, exist_ok=True)
        if source.is_dir():
            if not dest.exists():
                shutil.copytree(source, dest)
        else:
            dest.write_bytes(b"placeholder\n")

    return sandbox


def assert_green(sandbox: Path, label: str) -> None:
    code, output = run_guard(sandbox)
    assert code == 0, f"expected GREEN for '{label}' but guard failed:\n{output}"
    print(f"PASS (green as expected): {label}")


def assert_red(sandbox: Path, label: str) -> None:
    code, output = run_guard(sandbox)
    assert code != 0, f"expected RED for '{label}' but guard passed:\n{output}"
    print(f"PASS (red as expected): {label}")


def with_inventory_text(sandbox: Path, transform) -> None:
    path = sandbox / INVENTORY_RELATIVE_PATH
    text = path.read_text(encoding="utf-8")
    path.write_text(transform(text), encoding="utf-8")


def main() -> int:
    baseline = make_sandbox()
    try:
        assert_green(baseline, "untouched copy of the real inventory")
    finally:
        shutil.rmtree(baseline, ignore_errors=True)

    # Break 1: remove a whole row (Language modes).
    sandbox = make_sandbox()
    try:
        def remove_row(text: str) -> str:
            lines = text.splitlines(keepends=True)
            return "".join(line for line in lines if not line.startswith("| Language modes |"))
        with_inventory_text(sandbox, remove_row)
        assert_red(sandbox, "removed the 'Language modes' row entirely")
    finally:
        shutil.rmtree(sandbox, ignore_errors=True)

    # Break 2: remove a referenced implementation file.
    sandbox = make_sandbox()
    try:
        target = sandbox / "src/experience/internal/messagestyler.cpp"
        target.unlink()
        assert_red(sandbox, "deleted a referenced implementation file (messagestyler.cpp)")
    finally:
        shutil.rmtree(sandbox, ignore_errors=True)

    # Break 3: remove a referenced documentation article.
    sandbox = make_sandbox()
    try:
        target = sandbox / "docs/features/regex-builder.md"
        target.unlink()
        assert_red(sandbox, "deleted a referenced documentation article (regex-builder.md)")
    finally:
        shutil.rmtree(sandbox, ignore_errors=True)

    # Break 4: remove a referenced capture path.
    sandbox = make_sandbox()
    try:
        target = sandbox / "docs/design/captures/lane-d/15-preferences-material-theme.png"
        assert target.exists(), "sandbox setup did not copy the capture file we are about to remove"
        target.unlink()
        assert_red(sandbox, "deleted a referenced capture image")
    finally:
        shutil.rmtree(sandbox, ignore_errors=True)

    # Break 5: remove a claimed localized context from the translation file.
    sandbox = make_sandbox()
    try:
        def strip_context(text: str) -> str:
            return text.replace("<name>experience</name>", "<name>experience-renamed</name>")
        translation_path = sandbox / TRANSLATION_RELATIVE_PATH
        translation_path.write_text(
            strip_context(translation_path.read_text(encoding="utf-8")), encoding="utf-8"
        )
        assert_red(sandbox, "renamed the 'experience' translation context away")
    finally:
        shutil.rmtree(sandbox, ignore_errors=True)

    # Break 6: blank out the Notes column on a missing/not-applicable row.
    sandbox = make_sandbox()
    try:
        def strip_reason(text: str) -> str:
            lines = text.splitlines(keepends=True)
            out = []
            for line in lines:
                if line.startswith("| App logo customization |"):
                    cells = line.rstrip("\n").split("|")
                    cells[-2] = " "
                    out.append("|".join(cells) + "\n")
                else:
                    out.append(line)
            return "".join(out)
        with_inventory_text(sandbox, strip_reason)
        assert_red(sandbox, "blanked the Notes reason on the missing 'App logo customization' row")
    finally:
        shutil.rmtree(sandbox, ignore_errors=True)

    print("\nAll negative regression cases behaved correctly: green baseline, red on every break.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
