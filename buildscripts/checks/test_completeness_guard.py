#!/usr/bin/env python3
"""Each claimed inventory and receipt boundary is deliberately broken here."""
from __future__ import annotations
import json, re, shutil, subprocess, sys, tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]; SCRIPT = Path(__file__).resolve().parent / "completeness_guard.py"
FILES = ("docs/inventory/completeness-inventory.md", "docs/inventory/per-surface-completeness.md", "docs/inventory/product-surface-matrix.md", "share/locale/audacity_yue_HK.ts")
FEATURE, SURFACE, MATRIX, TRANSLATION = FILES

def invoke(root: Path, *args: str) -> tuple[int, str]:
    result = subprocess.run([sys.executable, str(SCRIPT), "--repo-root", str(root), *args], capture_output=True, text=True, check=False)
    return result.returncode, result.stdout + result.stderr

def sandbox() -> Path:
    target = Path(tempfile.mkdtemp(prefix="au-completeness-"))
    for name in FILES:
        dst = target / name; dst.parent.mkdir(parents=True, exist_ok=True); shutil.copy2(ROOT / name, dst)
    for name in (FEATURE, SURFACE):
        for token in re.findall(r"`([^`]+)`", (ROOT / name).read_text(encoding="utf-8")):
            source = ROOT / token
            if token in FILES or ("/" not in token and "." not in token) or not source.exists(): continue
            dst = target / token; dst.parent.mkdir(parents=True, exist_ok=True)
            if source.is_dir(): shutil.copytree(source, dst, dirs_exist_ok=True)
            else: dst.write_bytes(b"fixture\n")
    return target

def replace(path: Path, old: str, new: str) -> None:
    text = path.read_text(encoding="utf-8"); assert old in text, old; path.write_text(text.replace(old, new, 1), encoding="utf-8")

def expect(root: Path, code: int, label: str, *args: str, contains: str | None = None) -> None:
    actual, output = invoke(root, *args); assert actual == code, f"{label}: expected {code}, got {actual}\n{output}"
    if contains: assert contains in output, f"{label}: expected {contains!r}\n{output}"
    print(f"PASS: {label}")

def case(label: str, edit, code: int, *args: str, contains: str | None = None) -> None:
    root = sandbox()
    try: edit(root); expect(root, code, label, *args, contains=contains)
    finally: shutil.rmtree(root, ignore_errors=True)

def complete(root: Path) -> None:
    for name in (FEATURE, SURFACE, MATRIX):
        path = root / name; text = path.read_text(encoding="utf-8")
        for old, new in (("| partial |", "| implemented |"), ("| missing |", "| implemented |"), ("| not applicable |", "| implemented |"), ("unverified", "verified"), ("n/a", "verified")):
            text = text.replace(old, new)
        path.write_text(text, encoding="utf-8")
    capture = root / "evidence/capture.png"; capture.parent.mkdir(parents=True); capture.write_bytes(b"png fixture")
    receipt = root / "evidence/receipt.json"
    receipt.write_text(json.dumps({"capture": "evidence/capture.png", "sourceCommit": "a" * 40, "tuple": {"surface": "Desktop application", "theme": "light", "language": "English", "viewport": "1600x1000", "scale": "100%"}, "privacy": True, "current": True}), encoding="utf-8")
    replace(root / SURFACE, "verified | real launch receipt pending", "`evidence/capture.png` | `evidence/receipt.json`")

def completed_case(label: str, edit, contains: str) -> None:
    root = sandbox()
    try:
        complete(root); expect(root, 0, "synthetic complete baseline", "--completion")
        edit(root); expect(root, 1, label, "--completion", contains=contains)
    finally: shutil.rmtree(root, ignore_errors=True)

def main() -> int:
    case("report integrity baseline", lambda root: None, 0, "--strict")
    case("completion baseline is honestly red", lambda root: None, 1, "--completion")
    case("feature-row removal", lambda root: replace(root / FEATURE, "| Language modes |", "| Removed language modes |"), 1, "--strict", contains="canonical feature 'Language modes'")
    case("desktop matrix row removal", lambda root: replace(root / MATRIX, "| Desktop application | Language modes |", "| Desktop application | Removed language modes |"), 1, "--strict", contains="Desktop application' / 'Language modes")
    case("website matrix row removal", lambda root: replace(root / MATRIX, "| Documentation site | Language modes |", "| Documentation site | Removed language modes |"), 1, "--strict", contains="Documentation site' / 'Language modes")
    case("implementation-path removal", lambda root: (root / "src/experience/internal/messagestyler.cpp").unlink(), 1, "--strict", contains="does not exist")
    case("documentation-path removal", lambda root: (root / "docs/features/emoji-switch.md").unlink(), 1, "--strict", contains="does not exist")
    case("localized-context removal", lambda root: replace(root / TRANSLATION, "<name>experience</name>", "<name>removed</name>"), 1, "--strict", contains="localized context 'experience'")
    completed_case("persistence boundary", lambda root: replace(root / SURFACE, "`experience/language/mode`", "unverified"), "persistence is unverified")
    completed_case("focused-test boundary", lambda root: replace(root / SURFACE, "verified | verified | `evidence/capture.png`", " | verified | `evidence/capture.png`"), "focused test is unverified")
    completed_case("interaction boundary", lambda root: replace(root / SURFACE, "verified | verified | `evidence/capture.png`", "verified |  | `evidence/capture.png`"), "real built-artifact interaction is unverified")
    completed_case("capture path boundary", lambda root: replace(root / SURFACE, "`evidence/capture.png`", "`evidence/missing.png`"), "does not exist")
    completed_case("receipt path boundary", lambda root: replace(root / SURFACE, "`evidence/receipt.json`", "`evidence/missing.json`"), "does not exist")
    completed_case("receipt capture binding boundary", lambda root: replace(root / "evidence/receipt.json", "evidence/capture.png", "evidence/other.png"), "receipt capture path does not match")
    completed_case("receipt SHA boundary", lambda root: replace(root / "evidence/receipt.json", "\"a" + "a" * 39 + "\"", "\"short\""), "sourceCommit must be a full SHA")
    completed_case("receipt tuple boundary", lambda root: replace(root / "evidence/receipt.json", "\"scale\": \"100%\"", "\"size\": \"100%\""), "receipt tuple is incomplete")
    completed_case("receipt privacy boundary", lambda root: replace(root / "evidence/receipt.json", "\"privacy\": true", "\"privacy\": false"), "receipt privacy must be true")
    completed_case("receipt currentness boundary", lambda root: replace(root / "evidence/receipt.json", "\"current\": true", "\"current\": false"), "receipt current must be true")
    return 0

if __name__ == "__main__": raise SystemExit(main())
