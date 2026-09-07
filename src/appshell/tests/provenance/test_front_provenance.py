"""Candidate-bound build-manifest and common-shell structural regressions."""
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
QML = ROOT / "src/appshell/qml/Audacity/AppShell"
GENERATOR = QML / "GenerateBuildProvenance.cmake"


def uncomment(text):
    return re.sub(r'''("(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*')|//[^\n]*|/\*[\s\S]*?\*/''',
                  lambda match: match.group(1) or "", text)


def block(text, name):
    text = uncomment(text)
    matches = list(re.finditer(r"\b" + re.escape(name) + r"\s*\{", text))
    if len(matches) != 1:
        raise AssertionError(f"Expected exactly one {name} object")
    opening = text.index("{", matches[0].start())
    depth = 0
    quote = None
    escape = False
    start_depth = None
    for index, character in enumerate(text):
        if quote:
            if escape: escape = False
            elif character == "\\": escape = True
            elif character == quote: quote = None
            continue
        if character in "\"'": quote = character; continue
        if character == "{":
            if index == opening: start_depth = depth
            depth += 1
        elif character == "}":
            depth -= 1
            if start_depth is not None and index > opening and depth == start_depth:
                return text[opening + 1:index], start_depth
    raise AssertionError("Unclosed QML object")


def assert_shell(shell, component):
    front, depth = block(shell, "FrontBuildProvenance")
    dock, dock_depth = block(shell, "DockWindow")
    assert depth == dock_depth == 1, "Provenance must be a common-shell sibling of every page container"
    assert re.search(r"anchors\.top:\s*parent\.top\b", front)
    assert not re.search(r"\b(?:visible|opacity|active)\s*:", front)
    assert re.search(r"anchors\.top:\s*frontBuildProvenance\.bottom\b", dock)
    assert re.search(r"function\s+init\(\)\s*\{\s*root\.init\(\);?\s*\}", uncomment(shell))
    component = uncomment(component)
    assert "aboutModel.buildVersion()" in component and "aboutModel.appVersion()" not in component
    assert "aboutModel.buildUpdatedAtLocal()" in component
    for text in ('"Version unavailable"', '"Build provenance unavailable"', '"Build recorded at %1"'):
        assert text in component
    for identifier, binding in (("FrontBuildVersion", "runningVersionLine"), ("FrontBuildTimestamp", "buildProvenanceLine")):
        assert f'objectName: "{identifier}"' in component
        assert re.search(r"accessible\.name:\s*root\." + binding + r"\b", component)
        assert re.search(r"text:\s*root\." + binding + r"\b", component)
    assert len(re.findall(r"wrapMode:\s*Text\.WrapAnywhere\b", component)) == 2
    assert not re.search(r"\belide\s*:", component)
    assert "implicitHeight: labels.implicitHeight + 12" in component


class FrontShellTests(unittest.TestCase):
    def test_common_shell_and_exact_negative_boundaries(self):
        shell = (QML / "WindowContent.qml").read_text(encoding="utf-8")
        component = (QML / "shared/FrontBuildProvenance.qml").read_text(encoding="utf-8")
        assert_shell(shell, component)
        mutations = [(shell.replace("FrontBuildProvenance {", "RemovedProvenance {", 1), component),
                     (shell.replace("anchors.top: frontBuildProvenance.bottom", "anchors.top: parent.top", 1), component),
                     (shell.replace("id: frontBuildProvenance", "id: frontBuildProvenance\nvisible: false", 1), component)]
        for needle in ("Version unavailable", "Build provenance unavailable", "Build recorded at %1", "FrontBuildVersion", "FrontBuildTimestamp"):
            mutations.append((shell, component.replace(needle, "removed", 1)))
        mutations.append((shell, component.replace("Text.WrapAnywhere", "Text.WordWrap", 1)))
        for changed_shell, changed_component in mutations:
            with self.assertRaises(AssertionError): assert_shell(changed_shell, changed_component)

    def test_all_declared_startup_modes_have_common_shell_provenance(self):
        inventory = json.loads((HERE / "startup-mode-inventory.json").read_text())
        expected = {"StartEmpty": "HOME_URI", "StartWithNewProject": "HOME_URI", "Recovery": "HOME_URI",
                    "StartWithProject": "PROJECT_URI", "ContinueLastSession": "PROJECT_URI", "FirstLaunch": "PROJECT_URI"}
        self.assertEqual(inventory["modes"], expected)
        types = uncomment((ROOT / "src/appshell/appshelltypes.h").read_text())
        enum = re.search(r"enum class StartupModeType\s*\{([^}]+)\}", types).group(1)
        self.assertEqual(set(re.findall(r"\b[A-Za-z][A-Za-z0-9]*\b", enum)), set(expected))
        scenario = uncomment((ROOT / "src/appshell/internal/startupscenario.cpp").read_text())
        body = scenario.split("muse::Uri StartupScenario::startupPageUri", 1)[1].split("\n}", 1)[0]
        actual, pending = {}, []
        for line in body.splitlines():
            case = re.search(r"case StartupModeType::([A-Za-z]+):", line)
            if case: pending.append(case.group(1))
            returned = re.search(r"return (HOME_URI|PROJECT_URI);", line)
            if returned:
                actual.update({name: returned.group(1) for name in pending}); pending.clear()
        self.assertEqual(actual, expected)
        for mode in expected:
            changed = dict(actual); del changed[mode]
            self.assertNotEqual(changed, expected)
        self.assertNotIn("runningVersionLine", (QML / "HomePage/HomeMenu.qml").read_text())

    def test_manifest_consumer_and_generator_binding_are_exact(self):
        model = (QML / "aboutmodel.cpp").read_text()
        self.assertIn('QByteArray::fromHex(AU_BUILD_MANIFEST_HEX), AU_BUILD_MANIFEST_SHA256', model)
        version_body = model.split("QString AboutModel::buildVersion() const", 1)[1].split("\n}", 1)[0]
        self.assertNotIn("application()", version_body)
        cmake = (QML / "CMakeLists.txt").read_text()
        for line in ("buildprovenance.cpp", "buildprovenance.h", "shared/FrontBuildProvenance.qml"):
            self.assertEqual([x.strip() for x in cmake.splitlines()].count(line), 1)
        self.assertIn('"-DAU_BUILD_VERSION=${MUSE_APP_VERSION}"', cmake)
        self.assertIn('"-DAU_BUILD_ID=${AU_CONFIGURED_BUILD_ID}"', cmake)
        generator = GENERATOR.read_text()
        self.assertNotIn("%cI", generator)
        self.assertNotIn("file(TIMESTAMP", generator)
        self.assertLess(generator.index('diff --quiet --ignore-submodules=dirty'), generator.index('file(MAKE_DIRECTORY'))
        self.assertLess(generator.index('ls-files --others --exclude-standard'), generator.index('file(MAKE_DIRECTORY'))

    def test_cantonese_unavailable_and_timestamp_copy_exists(self):
        document = ET.parse(ROOT / "share/locale/audacity_yue_HK.ts")
        contexts = [context for context in document.findall("context") if context.findtext("name") == "appshell"]
        messages = {message.findtext("source"): message.findtext("translation") for context in contexts for message in context.findall("message")}
        for source in ("Version unavailable", "Version %1", "Build provenance unavailable", "Build recorded at %1"):
            self.assertTrue(messages.get(source), source)


class BuildManifestTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="audacity-build-provenance-")
        self.root = Path(self.temporary.name)
        self.env = dict(os.environ, GIT_AUTHOR_DATE="2001-01-01T00:00:00+00:00", GIT_COMMITTER_DATE="2001-01-01T00:00:00+00:00")
        self.git("init", "-q")
        self.git("config", "user.name", "fixture")
        self.git("config", "user.email", "fixture@example.invalid")
        (self.root / ".gitignore").write_text("build/\n")
        (self.root / "tracked.txt").write_text("baseline")
        self.git("add", ".gitignore", "tracked.txt")
        self.git("commit", "-qm", "fixture")
        self.revision = self.git("rev-parse", "HEAD").strip()
        self.header = self.root / "build/appshelldisplayprovenance.h"

    def tearDown(self): self.temporary.cleanup()

    def git(self, *args):
        result = subprocess.run(["git", "-C", str(self.root), *args], env=self.env, capture_output=True, text=True, encoding="utf-8", timeout=20)
        self.assertEqual(result.returncode, 0, result.stderr)
        return result.stdout

    def generate(self, expected=True, *, version="4.0.0", build_id="c" * 32):
        result = subprocess.run(["cmake", f"-DAU_SOURCE_DIR={self.root}", f"-DAU_EXPECTED_SOURCE_REVISION={self.revision}",
                                 f"-DAU_OUTPUT_HEADER={self.header}", f"-DAU_BUILD_VERSION={version}", "-DAU_BUILD_NUMBER=14",
                                 f"-DAU_BUILD_ID={build_id}", "-P", str(GENERATOR)], env=dict(self.env, SOURCE_DATE_EPOCH="1"),
                                capture_output=True, text=True, encoding="utf-8", timeout=20)
        self.assertEqual(result.returncode == 0, expected, result.stdout + result.stderr)
        return result

    def manifest(self):
        files = list((self.root / "build/manifests").glob("*.json"))
        self.assertEqual(len(files), 1)
        return files[0]

    def test_recorded_build_clock_and_exact_embedded_bytes(self):
        before = datetime.now(timezone.utc).replace(microsecond=0)
        self.generate()
        after = datetime.now(timezone.utc)
        path = self.manifest()
        data = path.read_bytes()
        manifest = json.loads(data)
        self.assertEqual(manifest["version"], "4.0.0")
        self.assertEqual(manifest["sourceRevision"], self.revision)
        self.assertEqual(manifest["sourceTree"], self.git("rev-parse", "HEAD^{tree}").strip())
        recorded = datetime.fromisoformat(manifest["buildStartedAtUtc"].replace("Z", "+00:00"))
        self.assertLessEqual(before, recorded); self.assertLessEqual(recorded, after)
        self.assertEqual(manifest["timestampKind"], "build-start")
        header = self.header.read_text()
        embedded = re.search(r'#define AU_BUILD_MANIFEST_HEX "([0-9a-f]+)"', header).group(1)
        digest = re.search(r'#define AU_BUILD_MANIFEST_SHA256 "([0-9a-f]{64})"', header).group(1)
        self.assertEqual(bytes.fromhex(embedded), data)
        self.assertEqual(digest, hashlib.sha256(data).hexdigest())
        stamp = self.header.stat().st_mtime_ns
        self.generate()
        self.assertEqual(self.header.stat().st_mtime_ns, stamp)
        self.assertEqual(path.read_bytes(), data)

    def test_dirty_and_moved_candidates_write_no_output(self):
        (self.root / "tracked.txt").write_text("unstaged")
        self.generate(False)
        self.assertFalse(self.header.exists())
        self.git("add", "tracked.txt"); self.generate(False)
        self.git("reset", "-q", "HEAD", "--", "tracked.txt"); self.git("restore", "tracked.txt")
        extra = self.root / "untracked.txt"; extra.write_text("untracked"); self.generate(False); extra.unlink()
        self.git("commit", "--allow-empty", "-qm", "changed candidate"); self.generate(False)
        self.assertFalse(self.header.exists())

    def test_manifest_tamper_or_missing_digest_fails_without_rewriting_header(self):
        self.generate()
        path = self.manifest(); original = path.read_bytes(); header = self.header.read_bytes()
        path.write_bytes(original.replace(b'"4.0.0"', b'"9.9.9"'))
        self.generate(False)
        self.assertEqual(self.header.read_bytes(), header)
        path.write_bytes(original)
        digest = Path(str(path) + ".sha256"); saved = digest.read_bytes(); digest.unlink()
        self.generate(False); self.assertEqual(self.header.read_bytes(), header)
        digest.write_bytes(saved); self.generate()

    def test_new_configured_build_retains_prior_manifest(self):
        self.generate(); first = self.manifest(); original = first.read_bytes()
        self.generate(build_id="d" * 32)
        self.assertEqual(len(list(first.parent.glob("*.json"))), 2)
        self.assertEqual(first.read_bytes(), original)

    def test_missing_version_is_recorded_empty_not_invented(self):
        self.generate(version="")
        self.assertEqual(json.loads(self.manifest().read_bytes())["version"], "")


if __name__ == "__main__": unittest.main()
