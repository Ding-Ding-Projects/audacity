"""Synthetic verifier fixtures only. These are never production UI evidence."""
import copy
import json
import re
import shutil
import struct
import subprocess
import tempfile
import unittest
import zlib
from datetime import datetime, timezone, timedelta
from pathlib import Path

import completeness_guard as guard
import completion_evidence as evidence

ROOT = Path(__file__).resolve().parents[2]


def reference(root, name):
    return {"path": name, "sha256": evidence.digest((root / name).read_bytes())}


def write(root, name, value):
    file = root / name
    file.parent.mkdir(parents=True, exist_ok=True)
    data = (json.dumps(value, sort_keys=True) + "\n").encode("utf-8")
    file.write_bytes(data)
    return {"path": name, "sha256": evidence.digest(data)}


def image(number):
    def chunk(kind, payload):
        return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", zlib.crc32(kind + payload) & 0xffffffff)
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 1, 1, 8, 2, 0, 0, 0)) + chunk(b"IDAT", zlib.compress(b"\0" + bytes((number & 255, number >> 8, 42)))) + chunk(b"IEND", b"")


def git(root, *args):
    return subprocess.run(["git", "-C", str(root), "-c", "user.name=Evidence test fixture", "-c", "user.email=fixture@example.invalid", "-c", "commit.gpgsign=false", *args], capture_output=True, check=True).stdout.decode().strip()


def pe_fixture():
    """Minimal PE-shaped synthetic test bytes with a real RT_VERSION layout."""
    data = bytearray(1536); data[:2] = b"MZ"; struct.pack_into("<I", data, 60, 128)
    data[128:132] = b"PE\0\0"; struct.pack_into("<HHIIIHH", data, 132, 0x8664, 2, 0, 0, 0, 240, 2)
    opt = 152; struct.pack_into("<H", data, opt, 0x20b); struct.pack_into("<I", data, opt + 16, 0x1000)
    struct.pack_into("<II", data, opt + 112 + 16, 0x2000, 512)
    for i, (name, rva, raw, flags) in enumerate(((b".text", 0x1000, 512, 0x60000020), (b".rsrc", 0x2000, 1024, 0x40000040))):
        section = opt + 240 + i * 40; data[section:section + len(name)] = name
        struct.pack_into("<IIII", data, section + 8, 512, rva, 512, raw); struct.pack_into("<I", data, section + 36, flags)
    data[512] = 0xc3
    for offset, name, target in ((0, 16, 0x80000018), (24, 1, 0x80000030), (48, 0x409, 72)):
        struct.pack_into("<HH", data, 1024 + offset + 12, 0, 1); struct.pack_into("<II", data, 1024 + offset + 16, name, target)
    struct.pack_into("<IIII", data, 1024 + 72, 0x2080, 92, 0, 0)
    start = 1024 + 128; struct.pack_into("<HHH", data, start, 92, 52, 0)
    key = "VS_VERSION_INFO\0".encode("utf-16-le"); data[start + 6:start + 6 + len(key)] = key
    struct.pack_into("<IIIIII", data, start + 40, 0xfeef04bd, 0x10000, 0x10000, 0, 0x10000, 0)
    return bytes(data)


def fixture(root):
    for name in evidence.CONSUMED_INPUTS:
        target = root / name; target.parent.mkdir(parents=True, exist_ok=True); shutil.copyfile(ROOT / name, target)
    ledger = json.loads((root / evidence.LEDGER).read_text())
    for row in ledger["rows"]: row["status"] = "implemented"
    write(root, evidence.LEDGER, ledger)
    sources = {"src/completion_fixture.cpp": "void CompletionFixture() {}\n",
               "src/test-fixture.txt": "Arbitrary source-role counterexample.\n",
               "docs/features/completion-fixture.md": "# Synthetic feature\nTest-only explanation.\n",
               "buildscripts/checks/test_fixture.py": "def test_feature():\n    assert True\n",
               "docs/site/completion-fixture.html": '<html><body id="completion-fixture">Synthetic feature</body></html>\n'}
    for name, text in sources.items():
        file = root / name; file.parent.mkdir(parents=True, exist_ok=True); file.write_text(text, encoding="utf-8")
    catalog = root / guard.TRANSLATION
    catalog.write_text(catalog.read_text(encoding="utf-8").replace("</TS>", "<context><name>CompletionFixture</name><message><source>Synthetic test message</source><translation>測試訊息</translation></message></context></TS>"), encoding="utf-8")
    write(root, "docs/site/locales/fixture.json", {"language": "yue", "contexts": {"fixture": {"message": "測試訊息"}}})
    for product in evidence.SURFACES:
        write(root, f"docs/inventory/persistence/{product}.json", {"schemaVersion": 1, "id": "fixture-state", "product": product, "mode": "stateless", "reason": "Synthetic test surface retains no state."})
    git(root, "init", "-q"); git(root, "add", "docs", "src", "share", "buildscripts"); git(root, "commit", "-qm", "Synthetic verifier source")
    candidate, builds, launches, roles = git(root, "rev-parse", "HEAD"), {}, {}, {}
    stamp = datetime.fromtimestamp(int(git(root, "show", "-s", "--format=%ct", candidate)), timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    for product in evidence.SURFACES:
        (root / "outputs").mkdir(exist_ok=True)
        if product == "desktop":
            name = "outputs/desktop.exe"; (root / name).write_bytes(pe_fixture()); artifact = reference(root, name)
            identity = {"executable": artifact, "peVersion": [1, 0, 0, 0], "processId": 1234, "createdAt": stamp}
            implementation, symbol, localized, message = "src/completion_fixture.cpp", "CompletionFixture", guard.TRANSLATION, "CompletionFixture::Synthetic test message"
        else:
            name = "outputs/site/index.html"; (root / name).parent.mkdir(exist_ok=True); (root / name).write_text(sources["docs/site/completion-fixture.html"])
            entry = reference(root, name)
            artifact = write(root, "outputs/site-manifest.json", {"schemaVersion": 1, "sourceCommit": candidate, "version": "1.0.0", "root": "outputs/site", "entrypoint": entry, "files": [entry]})
            identity = {"origin": "http://127.0.0.1:18000", "manifest": artifact, "responses": [entry]}
            implementation, symbol, localized, message = "docs/site/completion-fixture.html", "completion-fixture", "docs/site/locales/fixture.json", "fixture::message"
        roles[product] = {role: {"file": reference(root, name), "id": identifier} for role, name, identifier in (
            ("implementation", implementation, symbol), ("documentation", "docs/features/completion-fixture.md", "Synthetic feature"),
            ("localized", localized, message), ("testSource", "buildscripts/checks/test_fixture.py", "test_feature"),
            ("persistence", f"docs/inventory/persistence/{product}.json", "fixture-state"))}
        metadata = {"sourceCommit": candidate, "version": "1.0.0", "updatedAt": stamp, "artifact": artifact}
        version = write(root, f"outputs/{product}-version.json", metadata)
        builds[product] = write(root, f"outputs/{product}-build.json", {**metadata, "schemaVersion": 1, "product": product, "versionMetadata": version, "producedAt": stamp, "timestampMode": "recorded"})
        launches[product] = write(root, f"outputs/{product}-launch.json", {"schemaVersion": 1, "sourceCommit": candidate, "product": product, "build": builds[product], "artifact": artifact, "version": "1.0.0", "startedAt": stamp, "observer": "lowlevel-headless", "identity": identity})
    for index, row in enumerate(ledger["rows"]):
        key = {k: row[k] for k in ("product", "surface", "feature")}; base = f"outputs/row-{index:04d}"
        (root / f"{base}.png").write_bytes(image(index)); capture = reference(root, f"{base}.png")
        tuple_ = {"product": key["product"], "surface": key["surface"], "route": ("app://" if key["product"] == "desktop" else "/") + key["surface"], "state": "synthetic-test-only", "theme": "light", "language": "en", "viewport": [1, 1], "scale": 1}
        common = {"schemaVersion": 1, "key": key, "sourceCommit": candidate, "tuple": tuple_, "build": builds[key["product"]]}
        src = roles[key["product"]]["testSource"]
        output = write(root, f"{base}-output.json", {"syntheticTestOnly": True, "cases": [{"id": "test_feature", "result": "passed"}]})
        result = write(root, f"{base}-test.json", {**common, "testSource": src, "result": "passed", "cases": [{"id": "test_feature", "result": "passed"}], "output": output})
        interaction = write(root, f"{base}-interaction.json", {**common, "method": "lowlevel-headless", "launch": launches[key["product"]], "testResult": result, "steps": [{"before": "closed", "target": "synthetic target", "input": "click", "expected": "open", "after": "open", "capture": capture}]})
        privacy = write(root, f"{base}-privacy.json", {"syntheticTestOnly": True, "verdict": "passed", "key": key, "sourceCommit": candidate, "capture": capture})
        receipt = write(root, f"{base}-capture.json", {**common, "capturedAt": stamp, "region": [0, 0, 1, 1], "annotation": f"Synthetic feature {index} region", "capture": capture, "interaction": interaction, "dimensions": [1, 1], "privacy": {"verdict": "passed", "method": "isolated-profile-review", "review": privacy}})
        proof = {"key": key, "sourceCommit": candidate, **roles[key["product"]], "tuple": tuple_, "build": builds[key["product"]], "launch": launches[key["product"]], "testResult": result, "interaction": interaction, "capture": capture, "captureReceipt": receipt}
        write(root, row["evidence"], proof)
    return candidate, ledger


class EvidenceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory = tempfile.TemporaryDirectory(prefix="audacity-evidence-test-")
        cls.root = Path(cls.directory.name); cls.candidate, cls.ledger = fixture(cls.root)
        cls.descriptor = cls.root / cls.ledger["rows"][0]["evidence"]
        owned = [cls.root / name for name in (*evidence.CONSUMED_INPUTS, "src/completion_fixture.cpp")]
        owned.extend(p for p in (cls.root / "outputs").glob("*") if p.is_file() and not p.name.startswith("row-"))
        for row in (cls.ledger["rows"][0], cls.ledger["rows"][1], cls.ledger["rows"][39], next(r for r in cls.ledger["rows"] if r["product"] == "website")):
            descriptor = cls.root / row["evidence"]; owned.append(descriptor)
            proof = json.loads(descriptor.read_text())
            owned.extend(cls.root / proof[field]["path"] for field in ("testResult", "interaction", "captureReceipt", "capture"))
            receipt = json.loads((cls.root / proof["captureReceipt"]["path"]).read_text())
            owned.append(cls.root / receipt["privacy"]["review"]["path"])
            result = json.loads((cls.root / proof["testResult"]["path"]).read_text())
            owned.append(cls.root / result["output"]["path"])
        cls.baseline = {p: p.read_bytes() for p in owned}

    @classmethod
    def tearDownClass(cls): cls.directory.cleanup()

    def tearDown(self):
        for file, data in self.baseline.items():
            if not file.exists() or file.read_bytes() != data: file.write_bytes(data)
        git(self.root, "read-tree", self.candidate)

    def proof(self): return json.loads(self.descriptor.read_text())
    def save(self, proof): self.descriptor.write_text(json.dumps(proof), encoding="utf-8")
    def red(self, message, candidate=None):
        failures = evidence.validate(self.root, guard.CANONICAL, candidate or self.candidate, fail_fast=True)
        self.assertTrue(any(message in f for f in failures), failures[:4])

    def change(self, field, edit):
        proof = self.proof(); record = json.loads((self.root / proof[field]["path"]).read_text()); edit(record)
        proof[field] = write(self.root, proof[field]["path"], record); self.save(proof)

    def test_complete_structural_fixture(self):
        self.assertEqual([], evidence.validate(self.root, guard.CANONICAL, self.candidate))
        self.assertEqual(1170, len(self.ledger["rows"]))

    def test_candidate_required(self): self.assertIn("--candidate", evidence.validate(self.root, guard.CANONICAL, None)[0])
    def test_nonexistent_candidate(self): self.red("Git object unavailable", "a" * 40)
    def test_valid_wrong_source_sha(self):
        wrong = git(self.root, "commit-tree", f"{self.candidate}^{{tree}}", "-m", "Other real commit")
        self.change("testResult", lambda r: r.update(sourceCommit=wrong)); self.red("stale candidate/build mismatch")
    def test_descriptor_sha(self):
        proof = self.proof(); proof["sourceCommit"] = "a" * 40; self.save(proof); self.red("descriptor candidate/key mismatch")
    def test_arbitrary_prose(self):
        proof = self.proof(); proof["implementation"] = "verified"; self.save(proof); self.red("not prose")
    def test_nonimage_with_correct_hash(self):
        proof = self.proof(); (self.root / proof["capture"]["path"]).write_bytes(b"png fixture")
        proof["capture"] = reference(self.root, proof["capture"]["path"]); self.save(proof); self.red("not a PNG")
    def test_corrupt_png_with_correct_hash(self):
        proof = self.proof(); file = self.root / proof["capture"]["path"]; data = bytearray(file.read_bytes()); data[45] ^= 1; file.write_bytes(data)
        proof["capture"] = reference(self.root, proof["capture"]["path"]); self.save(proof); self.red("PNG CRC mismatch")
    def test_wrong_capture_hash(self):
        proof = self.proof(); proof["capture"]["sha256"] = "a" * 64; self.save(proof); self.red("hash mismatch")
    def test_wrong_built_hash(self):
        self.change("build", lambda r: r["artifact"].update(sha256="a" * 64)); self.red("hash mismatch")
    def test_missing_provenance(self):
        self.change("build", lambda r: r.pop("version")); self.red("version provenance missing")
    def test_wrong_metadata(self):
        self.change("build", lambda r: r.update(version="9.9.9")); self.red("version metadata mismatch")
    def test_stale_build(self):
        self.change("build", lambda r: r.update(sourceCommit="a" * 40, current=True)); self.red("build candidate/product mismatch")
    def test_wrong_tuple(self):
        self.change("interaction", lambda r: r["tuple"].update(theme="dark")); self.red("interaction tuple mismatch")
    def test_wrong_surface(self):
        self.change("captureReceipt", lambda r: r["key"].update(surface="preferences")); self.red("evidence key mismatch")
    def test_decoded_dimensions(self):
        proof = self.proof(); proof["tuple"]["viewport"] = [2, 1]
        for field in ("testResult", "interaction", "captureReceipt"):
            record = json.loads((self.root / proof[field]["path"]).read_text()); record["tuple"] = proof["tuple"]
            if field == "interaction": record["testResult"] = proof["testResult"]
            proof[field] = write(self.root, proof[field]["path"], record)
        self.save(proof); self.red("decoded capture dimensions/tuple mismatch")
    def test_receipt_reuse(self):
        proof = self.proof(); second = json.loads((self.root / self.ledger["rows"][1]["evidence"]).read_text()); second["testResult"] = proof["testResult"]
        write(self.root, self.ledger["rows"][1]["evidence"], second); self.red("reuse across evidence rows")
    def test_capture_content_reuse(self):
        proof = self.proof(); second = json.loads((self.root / self.ledger["rows"][39]["evidence"]).read_text())
        (self.root / second["capture"]["path"]).write_bytes((self.root / proof["capture"]["path"]).read_bytes())
        second["capture"] = reference(self.root, second["capture"]["path"])
        write(self.root, self.ledger["rows"][39]["evidence"], second); self.red("capture content reuse")
    def test_website_status_without_evidence(self):
        row = next(r for r in self.ledger["rows"] if r["product"] == "website"); (self.root / row["evidence"]).unlink(); self.red("missing file")
    def test_missing_nested_surface(self):
        registry = json.loads((self.root / evidence.REGISTRY).read_text()); registry["surfaces"].pop(); write(self.root, evidence.REGISTRY, registry)
        self.red("inventory binding mismatch")
    def committed_mutation(self, name, value, expected):
        write(self.root, name, value); git(self.root, "add", name); tree = git(self.root, "write-tree")
        candidate = git(self.root, "commit-tree", tree, "-m", "Incomplete inventory fixture"); self.red(expected, candidate)
    def test_committed_missing_nested_surface(self):
        registry = json.loads((self.root / evidence.REGISTRY).read_text()); registry["surfaces"].pop()
        self.committed_mutation(evidence.REGISTRY, registry, "concrete surface registry missing")
    def test_missing_feature_pair(self):
        ledger = copy.deepcopy(self.ledger); ledger["rows"].pop(); self.committed_mutation(evidence.LEDGER, ledger, "evidence coverage missing")
    def test_escaping_path(self):
        proof = self.proof(); proof["implementation"]["file"]["path"] = "../outside.txt"; self.save(proof); self.red("escaping path")
    def test_changed_source(self):
        (self.root / "src/completion_fixture.cpp").write_text("void ChangedAfterCandidate() {}\n"); proof = self.proof(); proof["implementation"]["file"] = reference(self.root, "src/completion_fixture.cpp")
        self.save(proof); self.red("source differs from audited candidate")
    def test_failed_test(self):
        self.change("testResult", lambda r: r.update(result="failed")); self.red("test result/source mismatch")
    def test_no_test_cases(self):
        self.change("testResult", lambda r: r.update(cases=[])); self.red("cases absent")
    def test_output_case_mismatch(self):
        self.change("testResult", lambda r: r.update(cases=[{"id": "invented", "result": "passed"}]))
        self.red("test output does not match")
    def test_result_declared_id_required(self):
        proof = self.proof(); result = json.loads((self.root / proof["testResult"]["path"]).read_text())
        result["cases"] = [{"id": "different_test", "result": "passed"}]
        result["output"] = write(self.root, result["output"]["path"], {"cases": result["cases"]})
        proof["testResult"] = write(self.root, proof["testResult"]["path"], result); self.save(proof)
        self.red("test result omits declared test ID")
    def test_no_interaction_steps(self):
        self.change("interaction", lambda r: r.update(steps=[])); self.red("steps absent")
    def test_missing_privacy(self):
        self.change("captureReceipt", lambda r: r.update(privacy=True)); self.red("privacy review absent")
    def test_real_tree_incomplete(self):
        self.assertTrue(any("completion is not proven" in f for f in guard.run(ROOT, True)[0]))
    def test_report_baseline(self): self.assertEqual([], guard.run(ROOT, False)[0])

    def test_all_consumed_inputs_candidate_bound(self):
        for name in evidence.CONSUMED_INPUTS[2:]:
            with self.subTest(name=name):
                file = self.root / name; original = file.read_bytes(); file.write_bytes(original + b"\nchanged after candidate\n")
                self.red("candidate inventory binding mismatch"); file.write_bytes(original)

    def test_arbitrary_text_rejected_for_every_source_role(self):
        original = self.proof()
        for role in ("implementation", "documentation", "localized", "persistence", "testSource"):
            with self.subTest(role=role):
                proof = copy.deepcopy(original); proof[role] = {"file": reference(self.root, "src/test-fixture.txt"), "id": "fixture::message"}
                self.save(proof); self.red(role + ":")

    def test_exact_role_identifiers(self):
        original = self.proof()
        for role in ("implementation", "documentation", "localized", "persistence", "testSource"):
            with self.subTest(role=role):
                proof = copy.deepcopy(original); proof[role]["id"] = "Absent::Message" if role == "localized" else "Absent"
                self.save(proof); self.red(role + ":")

    def artifact_change(self, data):
        proof = self.proof(); build = json.loads((self.root / proof["build"]["path"]).read_text())
        file = self.root / build["artifact"]["path"]; file.write_bytes(data); build["artifact"] = reference(self.root, build["artifact"]["path"])
        metadata = json.loads((self.root / build["versionMetadata"]["path"]).read_text()); metadata["artifact"] = build["artifact"]
        build["versionMetadata"] = write(self.root, build["versionMetadata"]["path"], metadata)
        proof["build"] = write(self.root, proof["build"]["path"], build); self.save(proof)

    def test_hashed_text_not_desktop_product(self):
        self.artifact_change(b"A synthetic file wearing an exe extension")
        self.red("desktop artifact is not PE")

    def test_embedded_version_mismatch(self):
        data = bytearray(pe_fixture()); struct.pack_into("<I", data, 1024 + 128 + 40 + 16, 0x90000)
        self.artifact_change(data); self.red("desktop embedded PE version mismatch")

    def test_launch_identity_mismatch(self):
        self.change("launch", lambda r: r["identity"].update(executable={"path": "other.exe", "sha256": "a" * 64}))
        self.red("desktop launched executable identity mismatch")

    def test_interaction_different_launch(self):
        self.change("interaction", lambda r: r.update(launch={"path": "other.json", "sha256": "a" * 64}))
        self.red("interaction launched/served identity mismatch")

    def test_future_capture(self):
        self.change("captureReceipt", lambda r: r.update(capturedAt=(datetime.now(timezone.utc) + timedelta(days=1)).strftime("%Y-%m-%dT%H:%M:%SZ")))
        self.red("capture chronology")

    def test_capture_before_build(self):
        self.change("captureReceipt", lambda r: r.update(capturedAt="2000-01-01T00:00:00Z")); self.red("capture chronology")

    def test_build_before_candidate(self):
        self.change("build", lambda r: r.update(producedAt="2000-01-01T00:00:00Z")); self.red("build chronology")

    def test_future_build(self):
        self.change("build", lambda r: r.update(producedAt="2999-01-01T00:00:00Z")); self.red("build chronology")

    def test_reproducible_epoch_mismatch(self):
        self.change("build", lambda r: r.update(timestampMode="reproducible", sourceDateEpoch=0)); self.red("SOURCE_DATE_EPOCH mismatch")

    def test_region_required(self):
        self.change("captureReceipt", lambda r: r.pop("region")); self.red("precise in-image feature region")

    def test_annotation_required(self):
        self.change("captureReceipt", lambda r: r.pop("annotation")); self.red("feature capture annotation")

    def test_shared_capture_same_surface_with_region(self):
        original = self.proof(); name = self.ledger["rows"][1]["evidence"]; second = json.loads((self.root / name).read_text())
        second["capture"] = original["capture"]
        interaction = json.loads((self.root / second["interaction"]["path"]).read_text()); interaction["steps"][-1]["capture"] = second["capture"]
        second["interaction"] = write(self.root, second["interaction"]["path"], interaction)
        receipt = json.loads((self.root / second["captureReceipt"]["path"]).read_text()); receipt["capture"] = second["capture"]; receipt["interaction"] = second["interaction"]
        privacy = json.loads((self.root / receipt["privacy"]["review"]["path"]).read_text()); privacy["capture"] = second["capture"]
        receipt["privacy"]["review"] = write(self.root, receipt["privacy"]["review"]["path"], privacy)
        second["captureReceipt"] = write(self.root, second["captureReceipt"]["path"], receipt); write(self.root, name, second)
        self.assertEqual([], evidence.validate(self.root, guard.CANONICAL, self.candidate))

    def test_website_staged_file_omission(self):
        name = next(r["evidence"] for r in self.ledger["rows"] if r["product"] == "website")
        proof = json.loads((self.root / name).read_text()); build = json.loads((self.root / proof["build"]["path"]).read_text())
        file = self.root / "outputs/site-manifest.json"; manifest = json.loads(file.read_text()); manifest["files"] = []
        build["artifact"] = write(self.root, "outputs/site-manifest.json", manifest)
        metadata = json.loads((self.root / build["versionMetadata"]["path"]).read_text()); metadata["artifact"] = build["artifact"]
        build["versionMetadata"] = write(self.root, build["versionMetadata"]["path"], metadata)
        proof["build"] = write(self.root, proof["build"]["path"], build); write(self.root, name, proof)
        self.red("website staged files absent")

    def test_website_served_hash_mismatch(self):
        name = next(r["evidence"] for r in self.ledger["rows"] if r["product"] == "website")
        proof = json.loads((self.root / name).read_text()); launch = json.loads((self.root / proof["launch"]["path"]).read_text()); launch["identity"]["responses"][0]["sha256"] = "a" * 64
        proof["launch"] = write(self.root, proof["launch"]["path"], launch); write(self.root, name, proof)
        self.red("website served response hashes mismatch")


class NarrativeTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory(prefix="audacity-narrative-test-")
        self.root = Path(self.directory.name)
        for name in (guard.INVENTORY, guard.SURFACES, guard.MATRIX, guard.TRANSLATION):
            target = self.root / name; target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / name, target)
        for name in (guard.INVENTORY, guard.SURFACES):
            for token in re.findall(r"`([^`]+)`", (ROOT / name).read_text(encoding="utf-8")):
                original, target = ROOT / token, self.root / token
                if not original.exists() or target.exists(): continue
                if original.is_dir(): target.mkdir(parents=True, exist_ok=True)
                else:
                    target.parent.mkdir(parents=True, exist_ok=True); target.write_text("Synthetic narrative reference.\n")

    def tearDown(self): self.directory.cleanup()
    def mutate(self, name, old, new):
        file = self.root / name; text = file.read_text(encoding="utf-8"); self.assertIn(old, text)
        file.write_text(text.replace(old, new, 1), encoding="utf-8")
    def red(self, expected): self.assertTrue(any(expected in f for f in guard.run(self.root, False)[0]))
    def test_baseline(self): self.assertEqual([], guard.run(self.root, False)[0])
    def test_removed_canonical_row(self):
        self.mutate(guard.INVENTORY, "| Language modes |", "| Removed |")
        self.red("canonical feature 'Language modes'")
    def test_removed_narrative_row(self):
        self.mutate(guard.SURFACES, "| Front screen | Language modes |", "| Front screen | Removed |")
        self.red("canonical feature 'Language modes' is absent")
    def test_removed_desktop_matrix_row(self):
        self.mutate(guard.MATRIX, "| Desktop application | Language modes |", "| Desktop application | Removed |")
        self.red("Desktop application' / 'Language modes")
    def test_removed_website_matrix_row(self):
        self.mutate(guard.MATRIX, "| Documentation site | Language modes |", "| Documentation site | Removed |")
        self.red("Documentation site' / 'Language modes")
    def test_missing_implementation(self):
        (self.root / "src/experience/internal/messagestyler.cpp").unlink(); self.red("does not exist")
    def test_missing_documentation(self):
        (self.root / "docs/features/emoji-switch.md").unlink(); self.red("does not exist")
    def test_missing_localized_context(self):
        self.mutate(guard.TRANSLATION, "<name>experience</name>", "<name>removed</name>")
        self.red("localized context 'experience'")
    def test_additional_narrative_surface_supported(self):
        file = self.root / guard.SURFACES; text = file.read_text(encoding="utf-8")
        row = next(line for line in text.splitlines() if line.startswith("| Front screen | Language modes |"))
        file.write_text(text.replace(row, row + "\n" + row.replace("Front screen", "Preferences")), encoding="utf-8")
        self.assertEqual([], guard.run(self.root, False)[0])


if __name__ == "__main__": unittest.main(verbosity=2)
