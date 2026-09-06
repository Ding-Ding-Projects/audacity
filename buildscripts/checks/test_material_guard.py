#!/usr/bin/env python3
"""Audacity: A Digital Audio Editor

test_material_guard.py

Negative regression for material_guard.py. Builds a small throwaway QML
fixture tree, then proves the guard actually distinguishes an allow listed
legacy usage from an unaddressed one, rather than always reporting clean.

Run directly:
    python3 buildscripts/checks/test_material_guard.py
"""

from __future__ import annotations

import shutil
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import material_guard  # noqa: E402


FIXTURE_QML = """import QtQuick
import Muse.UiComponents

Item {{
    id: root

    {control} {{
        text: "example"
    }}
}}
"""

INVENTORY_HEADER = """# Material Design 3 audit fixture

| File | Identifier | Reason | Status |
| --- | --- | --- | --- |
"""


class MaterialGuardTest(unittest.TestCase):
    def setUp(self):
        self.tmp_dir = Path(tempfile.mkdtemp(prefix="material-guard-test-"))
        self.repo_root = self.tmp_dir / "repo"
        (self.repo_root / "src" / "fixture" / "qml" / "Fixture").mkdir(parents=True)
        (self.repo_root / "docs" / "inventory").mkdir(parents=True)

    def tearDown(self):
        shutil.rmtree(self.tmp_dir, ignore_errors=True)

    def _write_fixture_file(self, control: str) -> Path:
        path = self.repo_root / "src" / "fixture" / "qml" / "Fixture" / "FixtureView.qml"
        path.write_text(FIXTURE_QML.format(control=control), encoding="utf-8")
        return path

    def _write_inventory(self, rows: str) -> Path:
        path = self.repo_root / "docs" / "inventory" / "material-audit.md"
        path.write_text(INVENTORY_HEADER + rows, encoding="utf-8")
        return path

    def test_unaddressed_legacy_control_is_a_finding(self):
        """An un-allow-listed legacy control instantiation must be found."""
        self._write_fixture_file("FlatButton")
        self._write_inventory("")

        result = material_guard.run_guard(
            self.repo_root, self.repo_root / "docs" / "inventory" / "material-audit.md"
        )

        self.assertEqual(len(result.findings), 1, "expected the FlatButton to be flagged")
        self.assertEqual(result.findings[0].identifier, "FlatButton")
        self.assertEqual(len(result.allowlisted), 0)

    def test_allow_listed_legacy_control_is_not_a_finding(self):
        """The same usage, allow listed with a reason, must go green."""
        self._write_fixture_file("FlatButton")
        rel_path = "src/fixture/qml/Fixture/FixtureView.qml"
        self._write_inventory(
            f"| `{rel_path}` | FlatButton | kept: muse control already restyled by "
            "0005-m3-controls.patch | kept |\n"
        )

        result = material_guard.run_guard(
            self.repo_root, self.repo_root / "docs" / "inventory" / "material-audit.md"
        )

        self.assertEqual(len(result.findings), 0, "the allow listed usage must not be a finding")
        self.assertEqual(len(result.allowlisted), 1)

    def test_removing_the_allowlist_row_turns_it_red_again(self):
        """Proves the guard is actually reading the allow list, not merely
        exempting the fixture directory: the identical file goes from green
        to red purely because the inventory row was removed."""
        self._write_fixture_file("FlatButton")
        rel_path = "src/fixture/qml/Fixture/FixtureView.qml"
        inventory_path = self._write_inventory(
            f"| `{rel_path}` | FlatButton | kept: muse control already restyled by "
            "0005-m3-controls.patch | kept |\n"
        )

        green = material_guard.run_guard(self.repo_root, inventory_path)
        self.assertEqual(len(green.findings), 0)

        # Remove the allow list row (simulate a reverted or never-written entry).
        inventory_path.write_text(INVENTORY_HEADER, encoding="utf-8")

        red = material_guard.run_guard(self.repo_root, inventory_path)
        self.assertEqual(len(red.findings), 1)
        self.assertEqual(red.findings[0].identifier, "FlatButton")

    def test_a_wrapper_component_name_is_not_a_false_positive(self):
        """A component whose *name* merely contains a legacy identifier as a
        substring (e.g. IncrementalPropertyControlWithTitle) must not be
        flagged: only an actual instantiation of the bare legacy identifier
        counts."""
        path = self.repo_root / "src" / "fixture" / "qml" / "Fixture" / "FixtureView.qml"
        path.write_text(
            "import QtQuick\n"
            "Item {\n"
            "    IncrementalPropertyControlWithTitle {\n"
            '        text: "example"\n'
            "    }\n"
            "}\n",
            encoding="utf-8",
        )
        self._write_inventory("")

        result = material_guard.run_guard(
            self.repo_root, self.repo_root / "docs" / "inventory" / "material-audit.md"
        )

        self.assertEqual(len(result.findings), 0, "a wrapper name must not trigger the guard")

    def test_color_literal_is_flagged_outside_m3_library(self):
        path = self.repo_root / "src" / "fixture" / "qml" / "Fixture" / "FixtureView.qml"
        path.write_text(
            'import QtQuick\nRectangle {\n    color: "#112233"\n}\n', encoding="utf-8"
        )
        self._write_inventory("")

        result = material_guard.run_guard(
            self.repo_root, self.repo_root / "docs" / "inventory" / "material-audit.md"
        )

        self.assertEqual(len(result.findings), 1)
        self.assertEqual(result.findings[0].kind, "color-literal")

    def test_m3_library_files_are_excluded_entirely(self):
        """The library that defines the Material 3 primitives is allowed to
        use raw colours and legacy base names internally; it is what every
        other surface is converted to use instead."""
        m3_dir = self.repo_root / "src" / "uicomponents" / "qml" / "Audacity" / "M3"
        m3_dir.mkdir(parents=True)
        (m3_dir / "M3Button.qml").write_text(
            'import QtQuick\nRectangle {\n    color: "#112233"\n    FlatButton { }\n}\n',
            encoding="utf-8",
        )
        self._write_inventory("")

        result = material_guard.run_guard(
            self.repo_root, self.repo_root / "docs" / "inventory" / "material-audit.md"
        )

        self.assertEqual(len(result.findings), 0)


if __name__ == "__main__":
    unittest.main()
