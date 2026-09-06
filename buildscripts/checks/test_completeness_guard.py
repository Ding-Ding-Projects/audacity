#!/usr/bin/env python3
"""Negative regressions for report integrity and the delivery completion verdict."""
from __future__ import annotations
import re, shutil, subprocess, sys, tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]; SCRIPT = Path(__file__).resolve().parent / "completeness_guard.py"
FEATURE = "docs/inventory/completeness-inventory.md"; SURFACE = "docs/inventory/per-surface-completeness.md"; TRANSLATION = "share/locale/audacity_yue_HK.ts"

def invoke(root: Path, *args: str) -> tuple[int, str]:
    result = subprocess.run([sys.executable, str(SCRIPT), "--repo-root", str(root), *args], capture_output=True, text=True, check=False)
    return result.returncode, result.stdout + result.stderr

def sandbox() -> Path:
    result = Path(tempfile.mkdtemp(prefix="au-completeness-"))
    for name in (FEATURE, SURFACE, TRANSLATION):
        dst = result / name; dst.parent.mkdir(parents=True, exist_ok=True); shutil.copy2(ROOT / name, dst)
    for text in ((ROOT / FEATURE).read_text(encoding="utf-8"), (ROOT / SURFACE).read_text(encoding="utf-8")):
        for token in re.findall(r"`([^`]+)`", text):
            src = ROOT / token
            if token in {FEATURE, SURFACE, TRANSLATION} or ("/" not in token and "." not in token) or not src.exists(): continue
            dst = result / token; dst.parent.mkdir(parents=True, exist_ok=True)
            if src.is_dir(): shutil.copytree(src, dst, dirs_exist_ok=True)
            else: dst.write_bytes(b"fixture\n")
    return result

def expect(root: Path, code: int, label: str, *args: str) -> None:
    actual, output = invoke(root, *args); assert actual == code, f"{label}: expected {code}, got {actual}\n{output}"; print(f"PASS: {label}")

def replace(path: Path, old: str, new: str) -> None:
    text = path.read_text(encoding="utf-8"); assert old in text, old; path.write_text(text.replace(old, new, 1), encoding="utf-8")

def case(label: str, change, expected: int, *args: str) -> None:
    root = sandbox()
    try: change(root); expect(root, expected, label, *args)
    finally: shutil.rmtree(root, ignore_errors=True)

def main() -> int:
    case("report integrity baseline is green", lambda root: None, 0, "--strict")
    case("completion baseline is red until real evidence exists", lambda root: None, 1, "--completion")
    case("missing feature row is red", lambda root: replace(root / FEATURE, "| Language modes |", "| Removed language modes |"), 1, "--strict")
    case("missing per-surface row is red", lambda root: replace(root / SURFACE, "| Front screen | Language modes |", "| Front screen | Removed language modes |"), 1, "--strict")
    case("missing implementation path is red", lambda root: (root / "src/experience/internal/messagestyler.cpp").unlink(), 1, "--strict")
    def completion_fixture(root: Path) -> None:
        for name in (FEATURE, SURFACE):
            path = root / name
            text = path.read_text(encoding="utf-8")
            for old, new in (("| partial |", "| implemented |"), ("| missing |", "| implemented |"), ("| not applicable |", "| implemented |"), ("unverified", "verified"), ("n/a", "verified")):
                text = text.replace(old, new)
            path.write_text(text, encoding="utf-8")
    complete = sandbox()
    try:
        completion_fixture(complete)
        expect(complete, 0, "synthetic fully-evidenced completion fixture is green", "--completion")
        replace(complete / SURFACE, "real launch receipt pending", "")
        expect(complete, 1, "completion rejects removed capture provenance", "--completion")
    finally:
        shutil.rmtree(complete, ignore_errors=True)
    return 0

if __name__ == "__main__": raise SystemExit(main())
