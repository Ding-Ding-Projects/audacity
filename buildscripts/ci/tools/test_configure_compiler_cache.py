"""Offline cache bootstrap and actual CMake argument fixtures; no CI mutation."""
import contextlib
import importlib.util
import io
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
SPEC = importlib.util.spec_from_file_location("configure_compiler_cache", HERE / "configure_compiler_cache.py")
cache = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(cache)


class CompilerCacheTests(unittest.TestCase):
    def configure(self, workspace, version="4.14.0", failure=None):
        calls = []
        program = workspace / "tool with spaces" / "ccache.exe"
        program.parent.mkdir(exist_ok=True)
        program.write_bytes(b"fixture, never executed")
        def runner(args, **kwargs):
            calls.append((args, kwargs))
            if failure == "timeout":
                raise subprocess.TimeoutExpired(args, 15)
            code = 17 if args[1] == failure else 0
            return subprocess.CompletedProcess(args, code, f"ccache version {version}\n" if args[1] == "--version" else "", "")
        updates, receipt = cache.configure_cache(workspace, which=lambda _: str(program), runner=runner, environment={"KEEP": "yes"})
        return updates, receipt, calls

    def test_missing_cache_never_invokes_or_downloads_a_tool(self):
        with tempfile.TemporaryDirectory() as directory:
            calls = []
            updates, receipt = cache.configure_cache(Path(directory), which=lambda _: None, runner=lambda *a, **k: calls.append(a))
            self.assertEqual(updates["MUSE_CI_COMPILER_CACHE"], "OFF")
            self.assertFalse(receipt["enabled"])
            self.assertEqual(calls, [])
            self.assertEqual(list(Path(directory).iterdir()), [])

    def test_compatible_installed_cache_gets_own_configuration_and_exact_path(self):
        with tempfile.TemporaryDirectory() as directory:
            updates, receipt, calls = self.configure(Path(directory))
            self.assertEqual(updates["MUSE_CI_COMPILER_CACHE"], "ON")
            self.assertTrue(receipt["enabled"])
            self.assertEqual([call[0][1] for call in calls], ["--version", "--show-stats", "--zero-stats"])
            self.assertTrue(all(call[0][0] == updates["MUSE_CI_CCACHE_PROGRAM"] for call in calls))
            self.assertEqual(calls[1][1]["env"]["CCACHE_CONFIGPATH"], updates["CCACHE_CONFIGPATH"])
            self.assertEqual(calls[1][1]["env"]["KEEP"], "yes")
            self.assertIn("max_size = 1G", Path(updates["CCACHE_CONFIGPATH"]).read_text(encoding="utf-8"))

    def test_old_new_major_and_unparseable_versions_disable_cache(self):
        for version in ("3.7.12", "4.7.9", "5.0.0", "unknown", "4.8.0-rc1"):
            with self.subTest(version=version), tempfile.TemporaryDirectory() as directory:
                updates, receipt, calls = self.configure(Path(directory), version=version)
                self.assertEqual(updates["MUSE_CI_COMPILER_CACHE"], "OFF")
                self.assertFalse(receipt["enabled"])
                self.assertEqual(len(calls), 1)

    def test_version_and_stats_failures_never_continue_to_later_cache_commands(self):
        for command, count in (("--version", 1), ("--show-stats", 2), ("--zero-stats", 3)):
            with self.subTest(command=command), tempfile.TemporaryDirectory() as directory:
                updates, receipt, calls = self.configure(Path(directory), failure=command)
                self.assertEqual(updates["MUSE_CI_COMPILER_CACHE"], "OFF")
                self.assertFalse(receipt["enabled"])
                self.assertEqual(len(calls), count)

    def test_probe_timeout_uses_documented_uncached_build(self):
        with tempfile.TemporaryDirectory() as directory:
            updates, receipt, calls = self.configure(Path(directory), failure="timeout")
            self.assertEqual(updates["MUSE_CI_COMPILER_CACHE"], "OFF")
            self.assertEqual(len(calls), 1)

    def test_environment_record_preserves_unrelated_values(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            env = root / "github.env"
            env.write_text("KEEP=yes\n", encoding="utf-8")
            output = root / "receipt.json"
            with patch.object(cache, "configure_cache", return_value=({"MUSE_CI_COMPILER_CACHE": "OFF", "MUSE_CI_CCACHE_PROGRAM": ""}, {"enabled": False})), contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(cache.main(["--workspace", directory, "--github-env", str(env), "--output", str(output)]), 0)
            self.assertEqual(env.read_text(encoding="utf-8"), "KEEP=yes\nMUSE_CI_COMPILER_CACHE=OFF\nMUSE_CI_CCACHE_PROGRAM=\n")
            self.assertFalse(json.loads(output.read_text(encoding="utf-8"))["enabled"])

    def test_failure_to_record_decision_is_fatal(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "receipt.json"
            with patch.object(cache, "configure_cache", return_value=({"MUSE_CI_COMPILER_CACHE": "OFF"}, {"enabled": False})), contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(cache.main(["--workspace", directory, "--github-env", directory, "--output", str(output)]), 1)
            self.assertFalse(output.exists())

    def cmake_options(self, directory, mode=None, program=""):
        root = Path(directory)
        script = root / "probe.cmake"
        output = root / "options.txt"
        script.write_text('include("@HELPER@")\naudacity_ci_compiler_cache_options(options)\nfile(WRITE "@OUTPUT@" "${options}")\n'
                          .replace("@HELPER@", (HERE / "compiler_cache_options.cmake").as_posix())
                          .replace("@OUTPUT@", output.as_posix()), encoding="utf-8")
        environment = dict(os.environ)
        environment.pop("MUSE_CI_COMPILER_CACHE", None)
        environment.pop("MUSE_CI_CCACHE_PROGRAM", None)
        if mode is not None:
            environment.update(MUSE_CI_COMPILER_CACHE=mode, MUSE_CI_CCACHE_PROGRAM=program)
        result = subprocess.run(["cmake", "-P", str(script)], env=environment, capture_output=True,
                                text=True, encoding="utf-8", errors="strict", timeout=15)
        values = output.read_text(encoding="utf-8").split(";") if output.exists() and output.stat().st_size else []
        return result, values

    def test_actual_cmake_off_clears_stale_cache_and_both_launchers(self):
        with tempfile.TemporaryDirectory() as directory:
            result, values = self.cmake_options(directory, "OFF", "missing-cache.exe")
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(values, ["-DMUSE_COMPILE_USE_CCACHE:BOOL=OFF", "-DCMAKE_C_COMPILER_LAUNCHER:STRING=",
                                      "-DCMAKE_CXX_COMPILER_LAUNCHER:STRING=", "-DCOMPILER_CACHE_PROGRAM:FILEPATH="])

    def test_actual_cmake_on_pins_the_checked_executable(self):
        with tempfile.TemporaryDirectory() as directory:
            program = Path(directory) / "cache with spaces.exe"
            program.write_bytes(b"fixture")
            result, values = self.cmake_options(directory, "ON", program.as_posix())
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn("-DMUSE_COMPILE_USE_CCACHE:BOOL=ON", values)
            self.assertIn("-DCOMPILER_CACHE_PROGRAM:FILEPATH=" + program.as_posix(), values)

    def test_actual_cmake_missing_program_or_invalid_decision_stops(self):
        for mode in ("ON", "perhaps"):
            with self.subTest(mode=mode), tempfile.TemporaryDirectory() as directory:
                result, values = self.cmake_options(directory, mode, "missing-cache.exe")
                self.assertNotEqual(result.returncode, 0)
                self.assertEqual(values, [])

    def test_actual_cmake_unset_decision_preserves_local_defaults(self):
        with tempfile.TemporaryDirectory() as directory:
            result, values = self.cmake_options(directory)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(values, [])

    def test_root_build_forwards_the_decision_inside_configure_macro(self):
        text = (ROOT / "ci_build.cmake").read_text(encoding="utf-8")
        body = text.split("macro(do_build build_type build_dir)", 1)[1].split("endmacro()", 1)[0]
        lines = [line.strip() for line in body.splitlines()]
        binding = "audacity_ci_compiler_cache_options(_cache_options)"
        append = "list(APPEND CONFIGURE_ARGS ${_cache_options})"
        self.assertEqual(lines.count(binding), 1)
        self.assertEqual(lines[lines.index(binding) + 1], append)
        self.assertNotIn(binding, [line.strip() for line in body.replace(binding, "", 1).splitlines()])


if __name__ == "__main__":
    unittest.main()
