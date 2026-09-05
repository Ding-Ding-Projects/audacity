#!/usr/bin/env python3
"""Generate the Material Audacity GitHub Release notes.

The notes describe the artifacts, explain how to verify them, and append the
verified workflow timing (UTC start, UTC completion, end to end duration) plus
the line count table produced by count_lines.py.

Usage:
    python3 buildscripts/ci/tools/release_notes.py \
        --tag v4.0.0-m3.1 \
        --version 4.0.0-m3001 \
        --start-time 2026-01-01T10:00:00Z \
        --repo audacity/audacity \
        --run-id 123456 \
        --assets-dir release-assets \
        --output RELEASE_NOTES.md
"""

from __future__ import annotations

import argparse
import datetime as dt
import os
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.abspath(os.path.join(HERE, "..", "..", ".."))

ISO_FORMATS = (
    "%Y-%m-%dT%H:%M:%SZ",
    "%Y-%m-%dT%H:%M:%S",
    "%Y-%m-%d %H:%M:%S",
)


def parse_utc(value: str) -> dt.datetime:
    text = value.strip().replace("+00:00", "Z")
    for fmt in ISO_FORMATS:
        try:
            return dt.datetime.strptime(text, fmt).replace(tzinfo=dt.timezone.utc)
        except ValueError:
            continue
    raise argparse.ArgumentTypeError(
        "could not parse '{0}' as a UTC timestamp".format(value)
    )


def format_utc(value: dt.datetime) -> str:
    return value.astimezone(dt.timezone.utc).strftime("%Y-%m-%d %H:%M:%S UTC")


def format_duration(delta: dt.timedelta) -> str:
    total = int(delta.total_seconds())
    if total < 0:
        total = 0
    hours, remainder = divmod(total, 3600)
    minutes, seconds = divmod(remainder, 60)
    parts = []
    if hours:
        parts.append("{0}h".format(hours))
    if hours or minutes:
        parts.append("{0}m".format(minutes))
    parts.append("{0}s".format(seconds))
    return " ".join(parts) + " ({0} seconds)".format(total)


def line_count_table() -> str:
    script = os.path.join(HERE, "count_lines.py")
    try:
        result = subprocess.run(
            [sys.executable, script, "--root", REPO_ROOT],
            check=True,
            capture_output=True,
            text=True,
        )
    except (OSError, subprocess.CalledProcessError) as exc:
        return "Line count unavailable: {0}".format(exc)
    return result.stdout.strip()


def asset_table(assets_dir: str | None) -> str:
    if not assets_dir or not os.path.isdir(assets_dir):
        return "_No asset directory was provided to the notes generator._"
    rows = []
    for name in sorted(os.listdir(assets_dir)):
        path = os.path.join(assets_dir, name)
        if not os.path.isfile(path):
            continue
        size = os.path.getsize(path)
        rows.append("| `{0}` | {1:,} bytes |".format(name, size))
    if not rows:
        return "_No assets were found._"
    return "\n".join(["| Asset | Size |", "| --- | ---: |"] + rows)


def build_notes(args: argparse.Namespace) -> str:
    start = parse_utc(args.start_time)
    end = parse_utc(args.end_time) if args.end_time else dt.datetime.now(dt.timezone.utc)
    duration = end - start

    run_url = ""
    if args.repo and args.run_id:
        run_url = "https://github.com/{0}/actions/runs/{1}".format(
            args.repo, args.run_id
        )

    sections = []
    sections.append("# Material Audacity {0}".format(args.tag))
    sections.append("")
    sections.append(
        "Material Audacity is a Material Design 3 rewrite of the Audacity 4 "
        "user interface, built on Qt 6.10 and the muse framework."
    )
    sections.append("")
    sections.append("## Downloads")
    sections.append("")
    sections.append(
        "Windows x64 users should download `Setup.exe` and run it. The "
        "installer is a Squirrel.Windows installer: it installs per user, "
        "needs no administrator rights, and updates in place from the "
        "`RELEASES` feed published with this release."
    )
    sections.append("")
    sections.append(asset_table(args.assets_dir))
    sections.append("")
    sections.append("## The installer is not code signed")
    sections.append("")
    sections.append(
        "Code signing is permanently disabled for this project. No build step "
        "calls `signtool`, and the release workflow fails if any produced "
        "`Setup.exe` or `Update.exe` reports anything other than `NotSigned`. "
        "Windows SmartScreen will therefore warn on first run. Verify the "
        "download yourself instead of relying on a signature:"
    )
    sections.append("")
    sections.append("```powershell")
    sections.append("# 1. Confirm the file matches the published checksum.")
    sections.append("Get-FileHash .\\Setup.exe -Algorithm SHA256")
    sections.append("# Compare the result with the matching line in SHA256SUMS.")
    sections.append("")
    sections.append("# 2. Confirm the file is unsigned, as documented.")
    sections.append("(Get-AuthenticodeSignature .\\Setup.exe).Status")
    sections.append("# Expected output: NotSigned")
    sections.append("```")
    sections.append("")
    sections.append("## Build provenance")
    sections.append("")
    sections.append("| Item | Value |")
    sections.append("| --- | --- |")
    sections.append("| Tag | `{0}` |".format(args.tag))
    sections.append("| Package version | `{0}` |".format(args.version))
    if args.commit:
        sections.append("| Commit | `{0}` |".format(args.commit))
    if run_url:
        sections.append("| Workflow run | {0} |".format(run_url))
    sections.append("| Workflow start (UTC) | {0} |".format(format_utc(start)))
    sections.append("| Workflow completion (UTC) | {0} |".format(format_utc(end)))
    sections.append("| End to end duration | {0} |".format(format_duration(duration)))
    sections.append("")
    sections.append("## Source line counts")
    sections.append("")
    sections.append(line_count_table())
    sections.append("")
    return "\n".join(sections)


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tag", required=True, help="Release tag, for example v4.0.0-m3.1")
    parser.add_argument("--version", default="", help="Package version used by Squirrel")
    parser.add_argument("--start-time", required=True, help="Workflow start time in UTC")
    parser.add_argument("--end-time", default="", help="Workflow completion time in UTC (default: now)")
    parser.add_argument("--repo", default=os.environ.get("GITHUB_REPOSITORY", ""))
    parser.add_argument("--run-id", default=os.environ.get("GITHUB_RUN_ID", ""))
    parser.add_argument("--commit", default=os.environ.get("GITHUB_SHA", ""))
    parser.add_argument("--assets-dir", default="")
    parser.add_argument("--output", default="", help="Write to this file instead of stdout")
    args = parser.parse_args(argv)

    notes = build_notes(args)
    if args.output:
        with open(args.output, "w", encoding="utf-8") as handle:
            handle.write(notes)
        print("Wrote release notes to {0}".format(args.output))
    else:
        print(notes)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
