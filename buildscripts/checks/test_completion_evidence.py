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
from pathlib import Path

import completeness_guard as guard
import completion_evidence as evidence

ROOT = Path(__file__).resolve().parents[2]


def reference(root, name):
    return {"path": name, "sha256": evidence.digest((root / name).read_bytes())}


def write(root, name, value):
    file = root / name
    file.parent.mkdir(parents=True, exist_ok=True)
    file.write_text(json.dumps(value, sort_keys=True) + "\n", encoding="utf-8")
    return reference(root, name)


def image(number):
    def chunk(kind, payload):
        return struct.pack(">I", len(payload)) + kind + payload + struct.pack(">I", zlib.crc32(kind + payload) & 0xffffffff)
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", 1, 1, 8, 2, 0, 0, 0)) + chunk(b"IDAT", zlib.compress(b"\0" + bytes((number & 255, number >> 8, 42)))) + chunk(b"IEND", b"")


def git(root, *args):
    return subprocess.run(["git", "-C", str(root), "-c", "user.name=Evidence test fixture", "-c", "user.email=fixture@example.invalid", "-c", "commit.gpgsign=false", *args], capture_output=True, check=True).stdout.decode().strip()


def fixture(root):
    for name in (evidence.REGISTRY, evidence.LEDGER):
        target = root / name; target.parent.mkdir(parents=True, exist_ok=True); shutil.copyfile(ROOT / name, target)
    ledger = json.loads((root / evidence.LEDGER).read_text())
    for row in ledger["rows"]: row["status"] = "implemented"
    write(root, evidence.LEDGER, ledger)
    source = root / "src/test-fixture.txt"; source.parent.mkdir(); source.write_text("Synthetic source for verifier tests only.\n")
    git(root, "init", "-q"); git(root, "add", "docs", "src"); git(root, "commit", "-qm", "Synthetic verifier source")
    candidate, builds = git(root, "rev-parse", "HEAD"), {}
    for product in evidence.SURFACES:
        name = f"outputs/{product}.bin"; (root / name).parent.mkdir(exist_ok=True)
        (root / name).write_bytes(b"Synthetic build fixture, not an executable.\n" + product.encode())
        metadata = {"sourceCommit": candidate, "version": "1.0.0", "updatedAt": "2026-09-06T00:00:00Z", "artifact": reference(root, name)}
        version = write(root, f"outputs/{product}-version.json", metadata)
        builds[product] = write(root, f"outputs/{product}-build.json", {**metadata, "schemaVersion": 1, "product": product, "versionMetadata": version})
    for index, row in enumerate(ledger["rows"]):
        key = {k: row[k] for k in ("product", "surface", "feature")}; base = f"outputs/row-{index:04d}"
        (root / f"{base}.png").write_bytes(image(index)); capture = reference(root, f"{base}.png")
        tuple_ = {"product": key["product"], "surface": key["surface"], "route": ("app://" if key["product"] == "desktop" else "/") + key["surface"], "state": "synthetic-test-only", "theme": "light", "language": "en", "viewport": [1, 1], "scale": 1}
        common = {"schemaVersion": 1, "key": key, "sourceCommit": candidate, "tuple": tuple_, "build": builds[key["product"]]}
        src = reference(root, "src/test-fixture.txt")
        output = write(root, f"{base}-output.json", {"syntheticTestOnly": True, "cases": [{"id": "synthetic", "result": "passed"}]})
        result = write(root, f"{base}-test.json", {**common, "testSource": src, "result": "passed", "cases": [{"id": "synthetic", "result": "passed"}], "output": output})
        interaction = write(root, f"{base}-interaction.json", {**common, "method": "lowlevel-headless", "testResult": result, "steps": [{"before": "closed", "target": "synthetic target", "input": "click", "expected": "open", "after": "open", "capture": capture}]})
        privacy = write(root, f"{base}-privacy.json", {"syntheticTestOnly": True, "verdict": "passed", "key": key, "sourceCommit": candidate, "capture": capture})
        receipt = write(root, f"{base}-capture.json", {**common, "capture": capture, "interaction": interaction, "dimensions": [1, 1], "privacy": {"verdict": "passed", "method": "isolated-profile-review", "review": privacy}})
        proof = {"key": key, "sourceCommit": candidate, **{f: src for f in ("implementation", "documentation", "localized", "persistence", "testSource")}, "tuple": tuple_, "build": builds[key["product"]], "testResult": result, "interaction": interaction, "capture": capture, "captureReceipt": receipt}
        write(root, row["evidence"], proof)
    return candidate, ledger


class EvidenceTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.directory = tempfile.TemporaryDirectory(prefix="audacity-evidence-test-")
        cls.root = Path(cls.directory.name); cls.candidate, cls.ledger = fixture(cls.root)
        cls.descriptor = cls.root / cls.ledger["rows"][0]["evidence"]
        owned = [cls.root / name for name in (evidence.REGISTRY, evidence.LEDGER, "src/test-fixture.txt", "outputs/desktop-build.json")]
        for row in (cls.ledger["rows"][0], cls.ledger["rows"][1], next(r for r in cls.ledger["rows"] if r["product"] == "website")):
            descriptor = cls.root / row["evidence"]; owned.append(descriptor)
            proof = json.loads(descriptor.read_text())
            owned.extend(cls.root / proof[field]["path"] for field in ("testResult", "interaction", "captureReceipt", "capture"))
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
        proof = self.proof(); second = json.loads((self.root / self.ledger["rows"][1]["evidence"]).read_text())
        (self.root / second["capture"]["path"]).write_bytes((self.root / proof["capture"]["path"]).read_bytes())
        second["capture"] = reference(self.root, second["capture"]["path"])
        write(self.root, self.ledger["rows"][1]["evidence"], second); self.red("capture content reuse")
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
        proof = self.proof(); proof["implementation"]["path"] = "../outside.txt"; self.save(proof); self.red("escaping path")
    def test_changed_source(self):
        (self.root / "src/test-fixture.txt").write_text("Changed after candidate\n"); proof = self.proof(); proof["implementation"] = reference(self.root, "src/test-fixture.txt")
        self.save(proof); self.red("source differs from audited candidate")
    def test_failed_test(self):
        self.change("testResult", lambda r: r.update(result="failed")); self.red("test result/source mismatch")
    def test_no_test_cases(self):
        self.change("testResult", lambda r: r.update(cases=[])); self.red("cases absent")
    def test_output_case_mismatch(self):
        self.change("testResult", lambda r: r.update(cases=[{"id": "invented", "result": "passed"}]))
        self.red("test output does not match")
    def test_no_interaction_steps(self):
        self.change("interaction", lambda r: r.update(steps=[])); self.red("steps absent")
    def test_missing_privacy(self):
        self.change("captureReceipt", lambda r: r.update(privacy=True)); self.red("privacy review absent")
    def test_real_tree_incomplete(self):
        self.assertTrue(any("completion is not proven" in f for f in guard.run(ROOT, True)[0]))
    def test_report_baseline(self): self.assertEqual([], guard.run(ROOT, False)[0])


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
