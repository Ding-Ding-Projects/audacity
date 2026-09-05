#!/usr/bin/env python3
"""Count source lines by language for the Material Audacity release notes.

The counter walks a fixed set of roots (src/, docs/site, buildscripts by
default), skips vendored trees and binaries, and prints a Markdown table.

Usage:
    python3 buildscripts/ci/tools/count_lines.py [--root REPO_ROOT]
                                                 [--path PATH ...]
                                                 [--format markdown|json]
"""

from __future__ import annotations

import argparse
import json
import os
import sys

# Directory names that are never counted, wherever they appear.
EXCLUDED_DIR_NAMES = {
    ".git",
    ".github",
    "__pycache__",
    "node_modules",
    "build",
    "build.release",
    "build.install",
    "build.artifacts",
    "dist",
}

# Repository relative prefixes that are never counted.
EXCLUDED_PREFIXES = (
    "muse/",
    "au3/",
    "thirdparty/",
    "muse_deps/",
)

DEFAULT_PATHS = (
    "src",
    "docs/site",
    "buildscripts",
)

# Extension to language name. Anything not listed is ignored, which keeps
# binaries (png, ico, zip, ttf, ...) out of the table automatically.
LANGUAGES = {
    ".c": "C",
    ".h": "C/C++ header",
    ".hpp": "C/C++ header",
    ".cpp": "C++",
    ".cc": "C++",
    ".cxx": "C++",
    ".mm": "Objective-C++",
    ".qml": "QML",
    ".js": "JavaScript",
    ".ts": "TypeScript",
    ".py": "Python",
    ".ps1": "PowerShell",
    ".sh": "Shell",
    ".bat": "Batch",
    ".cmake": "CMake",
    ".yml": "YAML",
    ".yaml": "YAML",
    ".json": "JSON",
    ".html": "HTML",
    ".css": "CSS",
    ".md": "Markdown",
    ".qrc": "Qt resource",
    ".ui": "Qt designer",
    ".nuspec": "NuSpec",
    ".rc": "Windows resource",
}

SPECIAL_FILENAMES = {
    "CMakeLists.txt": "CMake",
}


def language_for(filename: str) -> str | None:
    if filename in SPECIAL_FILENAMES:
        return SPECIAL_FILENAMES[filename]
    _, ext = os.path.splitext(filename)
    return LANGUAGES.get(ext.lower())


def is_excluded(rel_path: str) -> bool:
    normalized = rel_path.replace(os.sep, "/")
    return any(normalized.startswith(prefix) for prefix in EXCLUDED_PREFIXES)


def count_file(path: str) -> tuple[int, int]:
    """Return (total lines, non blank lines) for a text file."""
    total = 0
    non_blank = 0
    try:
        with open(path, "r", encoding="utf-8", errors="replace") as handle:
            for line in handle:
                total += 1
                if line.strip():
                    non_blank += 1
    except OSError:
        return (0, 0)
    return (total, non_blank)


def collect(root: str, paths: list[str]) -> dict:
    stats: dict[str, dict[str, int]] = {}
    for rel_root in paths:
        abs_root = os.path.join(root, rel_root)
        if not os.path.isdir(abs_root):
            continue
        for dirpath, dirnames, filenames in os.walk(abs_root):
            dirnames[:] = sorted(d for d in dirnames if d not in EXCLUDED_DIR_NAMES)
            for filename in sorted(filenames):
                abs_file = os.path.join(dirpath, filename)
                if os.path.islink(abs_file):
                    continue
                rel_file = os.path.relpath(abs_file, root)
                if is_excluded(rel_file):
                    continue
                language = language_for(filename)
                if language is None:
                    continue
                total, non_blank = count_file(abs_file)
                bucket = stats.setdefault(
                    language, {"files": 0, "lines": 0, "code_lines": 0}
                )
                bucket["files"] += 1
                bucket["lines"] += total
                bucket["code_lines"] += non_blank
    return stats


def render_markdown(stats: dict, paths: list[str]) -> str:
    rows = sorted(stats.items(), key=lambda item: item[1]["lines"], reverse=True)
    lines = []
    lines.append("| Language | Files | Lines | Non blank lines |")
    lines.append("| --- | ---: | ---: | ---: |")
    for language, bucket in rows:
        lines.append(
            "| {0} | {1} | {2} | {3} |".format(
                language, bucket["files"], bucket["lines"], bucket["code_lines"]
            )
        )
    total_files = sum(b["files"] for b in stats.values())
    total_lines = sum(b["lines"] for b in stats.values())
    total_code = sum(b["code_lines"] for b in stats.values())
    lines.append(
        "| **Total** | **{0}** | **{1}** | **{2}** |".format(
            total_files, total_lines, total_code
        )
    )
    lines.append("")
    lines.append(
        "Counted paths: {0}. Excluded: {1}.".format(
            ", ".join("`{0}`".format(p) for p in paths),
            ", ".join("`{0}`".format(p) for p in EXCLUDED_PREFIXES),
        )
    )
    return "\n".join(lines)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--root",
        default=os.path.abspath(
            os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..")
        ),
        help="Repository root (default: the repository this script lives in)",
    )
    parser.add_argument(
        "--path",
        action="append",
        dest="paths",
        default=None,
        help="Repository relative path to count. May be repeated.",
    )
    parser.add_argument(
        "--format", choices=("markdown", "json"), default="markdown"
    )
    args = parser.parse_args(argv)

    paths = args.paths if args.paths else list(DEFAULT_PATHS)
    stats = collect(args.root, paths)

    if args.format == "json":
        print(json.dumps({"paths": paths, "languages": stats}, indent=2, sort_keys=True))
    else:
        print(render_markdown(stats, paths))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
