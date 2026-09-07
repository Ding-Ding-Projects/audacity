#!/usr/bin/env python3
"""Use a compatible installed ccache, or explicitly build without a cache.

Ccache is an optional optimization, not an application/build dependency.
This path never downloads a tool and never invokes a missing executable.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import uuid


def configure_cache(workspace: Path, *, which=shutil.which, runner=subprocess.run, environment=None):
    workspace = workspace.resolve()
    inherited = dict(os.environ if environment is None else environment)
    disabled = {"MUSE_CI_COMPILER_CACHE": "OFF", "MUSE_CI_CCACHE_PROGRAM": ""}
    receipt = {"schemaVersion": 1, "enabled": False, "program": None, "version": None,
               "reason": "No installed ccache found; using the supported build without compiler caching."}
    executable = which("ccache")
    if not executable:
        return disabled, receipt
    executable = Path(executable).resolve().as_posix()
    if any(character in executable + str(workspace) for character in ("\n", "\r", ";")):
        receipt["reason"] = "Installed cache path cannot be represented safely in CMake configuration; caching disabled."
        return disabled, receipt
    receipt["program"] = executable

    def invoke(arguments, env):
        return runner([executable, *arguments], env=env, text=True, encoding="utf-8", errors="strict",
                      capture_output=True, check=False, timeout=15)

    try:
        version = invoke(["--version"], inherited)
        match = re.match(r"^ccache version (\d+)\.(\d+)(?:\.(\d+))?(?:\s|$)", version.stdout or "")
        if version.returncode or not match:
            receipt["reason"] = "Installed ccache version probe failed; compiler caching disabled."
            return disabled, receipt
        parts = tuple(int(value or 0) for value in match.groups())
        receipt["version"] = ".".join(str(value) for value in parts)
        # MSVC is supported from 4.6; this project's conservative floor is 4.8.
        if not (parts[0] == 4 and parts >= (4, 8, 0)):
            receipt["reason"] = "Installed ccache is outside the supported 4.8 <= version < 5 range; caching disabled."
            return disabled, receipt
        cache_root = workspace / "build.tools" / "compiler-cache"
        config_root = cache_root / ("config-" + uuid.uuid4().hex)
        config_root.mkdir(parents=True)
        cache_dir = cache_root / "data"
        cache_dir.mkdir(exist_ok=True)
        config = config_root / "ccache.conf"
        config.write_text(f"base_dir = {workspace.as_posix()}\nmax_size = 1G\nsloppiness = pch_defines,time_macros\n", encoding="utf-8")
        configured = dict(inherited, CCACHE_DIR=cache_dir.as_posix(), CCACHE_CONFIGPATH=config.as_posix())
        for arguments in (["--show-stats"], ["--zero-stats"]):
            result = invoke(arguments, configured)
            if result.returncode:
                receipt["reason"] = f"Installed ccache rejected {arguments[0]} (exit {result.returncode}); compiler caching disabled."
                return disabled, receipt
        receipt.update(enabled=True, reason="Compatible installed ccache configured and probed.",
                       cacheDirectory=cache_dir.as_posix(), configPath=config.as_posix())
        return {"MUSE_CI_COMPILER_CACHE": "ON", "MUSE_CI_CCACHE_PROGRAM": executable,
                "CCACHE_DIR": cache_dir.as_posix(), "CCACHE_CONFIGPATH": config.as_posix()}, receipt
    except (OSError, subprocess.TimeoutExpired, UnicodeError):
        receipt["reason"] = "Optional compiler cache could not be configured or probed; using the supported build without caching."
        return disabled, receipt


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--workspace", required=True, type=Path)
    parser.add_argument("--github-env", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args(argv)
    try:
        updates, receipt = configure_cache(args.workspace)
        # Failure to communicate the decision is fatal. Do not continue with
        # an unknown or stale compiler-launcher configuration.
        with args.github_env.open("a", encoding="utf-8") as stream:
            for key, value in updates.items():
                stream.write(f"{key}={value}\n")
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
        print(json.dumps(receipt, sort_keys=True))
        return 0
    except OSError as error:
        print(f"Compiler-cache decision could not be recorded: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
