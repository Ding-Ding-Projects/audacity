"""Offline release coordination fixtures. No real tag or release is mutated."""
import importlib.util
import contextlib
import io
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

SPEC = importlib.util.spec_from_file_location("reserve_release_tag", Path(__file__).with_name("reserve_release_tag.py"))
tags = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(tags)

SHA = "a" * 40
OTHER_SHA = "b" * 40
REPO = "example/project"


def record(tag, sha=SHA, kind="commit"):
    return {"ref": "refs/tags/" + tag, "object": {"type": kind, "sha": sha}}


class FakeApi:
    def __init__(self, records=(), collisions=0, failure=None):
        self.records = {item["ref"]: item for item in records}
        self.collisions = collisions
        self.failure = failure
        self.calls = []

    def request(self, method, endpoint, *, data=None, paginate=False):
        self.calls.append((method, endpoint, data, paginate))
        if "/git/commits/" in endpoint:
            return {"sha": SHA}
        if "/git/matching-refs/" in endpoint:
            # More than one page tests that the reader cannot stop at page one.
            rows = list(self.records.values())
            return [rows[:1], rows[1:]]
        if method == "POST":
            if self.failure:
                raise self.failure
            ref = data["ref"]
            if self.collisions:
                self.collisions -= 1
                self.records[ref] = {"ref": ref, "object": {"type": "commit", "sha": OTHER_SHA}}
                raise tags.ApiError(422, {"message": "Reference already exists"})
            if ref in self.records:
                raise AssertionError("Helper attempted to reuse an existing tag")
            self.records[ref] = {"ref": ref, "object": {"type": "commit", "sha": data["sha"]}}
            return self.records[ref]
        if "/git/ref/tags/" in endpoint:
            return self.records["refs/tags/" + endpoint.split("/git/ref/tags/", 1)[1]]
        raise AssertionError((method, endpoint))


class ReleaseReservationTests(unittest.TestCase):
    def test_manual_fourteen_then_workflow_chooses_fifteen(self):
        api = FakeApi([record("v4.0.0-m3.13")])
        manual = tags.reserve_tag(api, REPO, SHA, "4.0.0")
        workflow = tags.reserve_tag(api, REPO, SHA, "4.0.0", minimum=12)
        self.assertEqual(manual["tag"], "v4.0.0-m3.14")
        self.assertEqual(workflow["tag"], "v4.0.0-m3.15")
        self.assertEqual(workflow["packageVersion"], "4.0.0-m3015")
        self.assertEqual(set(api.records), {"refs/tags/v4.0.0-m3.13", "refs/tags/v4.0.0-m3.14", "refs/tags/v4.0.0-m3.15"})

    def test_minimum_is_floor_not_tag_identity(self):
        self.assertEqual(tags.choose_tag([[]], "4.0.0", 50), "v4.0.0-m3.50")
        self.assertEqual(tags.choose_tag([[record("v4.0.0-m3.75")]], "4.0.0", 50), "v4.0.0-m3.76")

    def test_collision_rereads_and_reserves_next_name(self):
        api = FakeApi([record("v4.0.0-m3.13")], collisions=1)
        result = tags.reserve_tag(api, REPO, SHA, "4.0.0")
        self.assertEqual(result["tag"], "v4.0.0-m3.15")
        self.assertEqual(result["attempts"], 2)
        self.assertEqual(api.records["refs/tags/v4.0.0-m3.14"]["object"]["sha"], OTHER_SHA)
        self.assertEqual(len([call for call in api.calls if "/matching-refs/" in call[1]]), 2)

    def test_three_collisions_stop_without_force_or_deletion(self):
        api = FakeApi(collisions=3)
        with self.assertRaisesRegex(tags.ReservationError, "three times"):
            tags.reserve_tag(api, REPO, SHA, "4.0.0")
        self.assertEqual(len([call for call in api.calls if call[0] == "POST"]), 3)
        self.assertEqual({call[0] for call in api.calls}, {"GET", "POST"})

    def test_foreign_tags_other_versions_and_pagination(self):
        pages = [[record("v4.0.0-m3.9"), record("nightly/other")],
                 [record("v4.0.0-m3.14", kind="tag"), record("v5.0.0-m3.900"), record("v4.0.01-m3.80")]]
        self.assertEqual(tags.choose_tag(pages, "4.0.0", 1), "v4.0.0-m3.15")

    def test_invalid_sha_stops_before_any_api_request(self):
        for sha in ("main", "a" * 39, "0" * 40, "g" * 40, SHA + "\n"):
            with self.subTest(sha=sha):
                api = FakeApi()
                with self.assertRaises(tags.ReservationError):
                    tags.reserve_tag(api, REPO, sha, "4.0.0")
                self.assertEqual(api.calls, [])

    def test_candidate_lookup_must_confirm_exact_commit(self):
        api = FakeApi()
        with self.assertRaisesRegex(tags.ReservationError, "exact SHA"):
            tags.reserve_tag(api, REPO, OTHER_SHA, "4.0.0")
        self.assertEqual(len(api.calls), 1)

    def test_auth_failure_is_not_retried(self):
        for status in (401, 403):
            api = FakeApi(failure=tags.ApiError(status, {"message": "Reference already exists"}))
            with self.assertRaises(tags.ApiError):
                tags.reserve_tag(api, REPO, SHA, "4.0.0")
            self.assertEqual(len([call for call in api.calls if call[0] == "POST"]), 1)
            self.assertEqual(api.records, {})

    def test_generic_validation_or_server_error_is_not_collision(self):
        for status in (422, 500, None):
            api = FakeApi(failure=tags.ApiError(status, {"message": "Validation Failed"}))
            expected = tags.ApiError if status == 422 else tags.UncertainReservation
            with self.assertRaises(expected):
                tags.reserve_tag(api, REPO, SHA, "4.0.0")
            self.assertEqual(len([call for call in api.calls if call[0] == "POST"]), 1)

    def test_structured_already_exists_collision_is_recognized(self):
        self.assertTrue(tags.ApiError(422, {"errors": [{"resource": "Reference", "code": "already_exists"}]}).is_reference_collision())
        self.assertFalse(tags.ApiError(422, {"errors": [{"resource": "Release", "code": "already_exists"}]}).is_reference_collision())

    def test_malformed_ref_records_fail_closed(self):
        malformed = [None, {}, {"ref": "refs/tags/v4.0.0-m3.1"},
                     record("v4.0.0-m3.bad"), record("v4.0.0-m3.01"), record("v4.0.0-m3.1\n"),
                     record("v4.0.0-m3.1", sha="123"), record("v4.0.0-m3.1", kind="blob"),
                     record("folder/../tag"), record("folder/tag.lock")]
        for item in malformed:
            with self.subTest(item=item), self.assertRaises(tags.ReservationError):
                tags.choose_tag([[item]], "4.0.0", 1)

    def test_malformed_pages_and_duplicate_refs_fail_closed(self):
        for pages in ([], {}, [None], [record("v4.0.0-m3.1")], [[record("v4.0.0-m3.1")], [record("v4.0.0-m3.1")]]):
            with self.subTest(pages=pages), self.assertRaises(tags.ReservationError):
                tags.choose_tag(pages, "4.0.0", 1)

    def test_invalid_version_sequence_and_exhaustion(self):
        for version in ("4.0", "04.0.0", "4.0.0-beta", "2147483648.0.0"):
            with self.subTest(version=version), self.assertRaises(tags.ReservationError):
                tags.choose_tag([[]], version, 1)
        for minimum in (0, -1, True, tags.MAX_SEQUENCE + 1):
            with self.subTest(minimum=minimum), self.assertRaises(tags.ReservationError):
                tags.choose_tag([[]], "4.0.0", minimum)
        with self.assertRaises(tags.ReservationError):
            tags.choose_tag([[record(f"v4.0.0-m3.{tags.MAX_SEQUENCE}")]], "4.0.0", 1)

    def test_verify_rejects_wrong_commit_annotated_or_wrong_ref(self):
        for item in (record("v4.0.0-m3.14", sha=OTHER_SHA), record("v4.0.0-m3.14", kind="tag")):
            api = FakeApi([item])
            with self.assertRaisesRegex(tags.ReservationError, "exact candidate"):
                tags.verify_tag(api, REPO, SHA, "v4.0.0-m3.14")
        self.assertTrue(tags.verify_tag(FakeApi([record("v4.0.0-m3.14")]), REPO, SHA, "v4.0.0-m3.14")["verified"])

    def test_readback_failure_retains_reservation_and_does_not_retry(self):
        class UnreadableApi(FakeApi):
            def request(self, method, endpoint, **kwargs):
                if "/git/ref/tags/" in endpoint:
                    raise tags.ApiError(403, {})
                return super().request(method, endpoint, **kwargs)
        api = UnreadableApi()
        with self.assertRaisesRegex(tags.UncertainReservation, "Uncertain reservation"):
            tags.reserve_tag(api, REPO, SHA, "4.0.0")
        self.assertEqual(len(api.records), 1)
        self.assertEqual(len([call for call in api.calls if call[0] == "POST"]), 1)

    def test_malformed_create_response_reports_possible_reservation_without_retry(self):
        class MalformedApi(FakeApi):
            def request(self, method, endpoint, **kwargs):
                result = super().request(method, endpoint, **kwargs)
                return {} if method == "POST" else result
        api = MalformedApi()
        with self.assertRaisesRegex(tags.UncertainReservation, "Uncertain reservation"):
            tags.reserve_tag(api, REPO, SHA, "4.0.0")
        self.assertEqual(len(api.records), 1)

    def test_receipt_preflight_refuses_existing_file_or_absent_parent_before_api(self):
        with tempfile.TemporaryDirectory() as directory:
            existing = Path(directory) / "existing.json"
            existing.write_text("keep these bytes", encoding="utf-8")
            missing = Path(directory) / "absent" / "receipt.json"
            for output in (existing, missing):
                api = FakeApi()
                with patch.object(tags, "GhApi", return_value=api), contextlib.redirect_stderr(io.StringIO()):
                    code = tags.main(["reserve", "--repo", REPO, "--sha", SHA, "--version", "4.0.0", "--output", str(output)])
                self.assertEqual(code, 1)
                self.assertEqual(api.calls, [])
            self.assertEqual(existing.read_text(encoding="utf-8"), "keep these bytes")

    def test_receipt_write_failure_reports_retained_verified_tag(self):
        with tempfile.TemporaryDirectory() as directory:
            api = FakeApi()
            errors = io.StringIO()
            with patch.object(tags, "GhApi", return_value=api), patch.object(tags.os, "link", side_effect=PermissionError()), contextlib.redirect_stderr(errors):
                code = tags.main(["reserve", "--repo", REPO, "--sha", SHA, "--version", "4.0.0", "--output", str(Path(directory) / "receipt.json")])
            self.assertEqual(code, 1)
            self.assertIn("v4.0.0-m3.1 was reserved and verified", errors.getvalue())
            self.assertEqual(len(api.records), 1)

    def test_main_preserves_timeout_and_malformed_post_identity_in_atomic_receipt(self):
        real_api_type = tags.GhApi
        for failure in ("timeout", "malformed"):
            with self.subTest(failure=failure), tempfile.TemporaryDirectory() as directory:
                calls = []
                def runner(args, **kwargs):
                    method, endpoint = args[3:5]
                    calls.append((method, endpoint))
                    if method == "POST":
                        if failure == "timeout":
                            raise subprocess.TimeoutExpired(args, 60)
                        return subprocess.CompletedProcess(args, 0, "{", "")
                    body = {"sha": SHA} if "/git/commits/" in endpoint else [[record("v4.0.0-m3.13")]]
                    return subprocess.CompletedProcess(args, 0, json.dumps(body), "")
                api = real_api_type(runner)
                output = Path(directory) / "uncertain.json"
                errors = io.StringIO()
                with patch.object(tags, "GhApi", return_value=api), contextlib.redirect_stderr(errors):
                    code = tags.main(["reserve", "--repo", REPO, "--sha", SHA, "--version", "4.0.0", "--output", str(output)])
                self.assertEqual(code, 1)
                receipt = json.loads(output.read_text(encoding="utf-8"))
                self.assertEqual(receipt["state"], "uncertain")
                self.assertIsNone(receipt["reserved"])
                self.assertFalse(receipt["verified"])
                self.assertEqual(receipt["repository"], REPO)
                self.assertEqual(receipt["tag"], "v4.0.0-m3.14")
                self.assertEqual(receipt["ref"], "refs/tags/v4.0.0-m3.14")
                self.assertEqual(receipt["sha"], SHA)
                self.assertEqual(receipt["attempts"], 1)
                for identity in (REPO, receipt["tag"], receipt["ref"], SHA, "attempt=1"):
                    self.assertIn(identity, errors.getvalue())
                self.assertEqual(len([call for call in calls if call[0] == "POST"]), 1)
                self.assertEqual(len(calls), 3)
                self.assertEqual(list(Path(directory).iterdir()), [output])

    def test_main_uncertain_receipt_link_failure_keeps_identity_and_staged_record(self):
        class TimeoutApi(FakeApi):
            def request(self, method, endpoint, **kwargs):
                if method == "POST":
                    self.calls.append((method, endpoint, kwargs.get("data"), False))
                    raise tags.ReservationError("Transport timeout")
                return super().request(method, endpoint, **kwargs)
        with tempfile.TemporaryDirectory() as directory:
            api = TimeoutApi([record("v4.0.0-m3.13")])
            output = Path(directory) / "uncertain.json"
            errors = io.StringIO()
            with patch.object(tags, "GhApi", return_value=api), patch.object(tags.os, "link", side_effect=PermissionError()), contextlib.redirect_stderr(errors):
                code = tags.main(["reserve", "--repo", REPO, "--sha", SHA, "--version", "4.0.0", "--output", str(output)])
            self.assertEqual(code, 1)
            self.assertFalse(output.exists())
            staged = list(Path(directory).glob(".uncertain.json.*.tmp"))
            self.assertEqual(len(staged), 1)
            receipt = json.loads(staged[0].read_text(encoding="utf-8"))
            self.assertEqual(receipt["state"], "uncertain")
            for identity in (REPO, "v4.0.0-m3.14", "refs/tags/v4.0.0-m3.14", SHA, "attempt=1"):
                self.assertIn(identity, errors.getvalue())
            self.assertIn(str(staged[0]), errors.getvalue())
            self.assertEqual(len([call for call in api.calls if call[0] == "POST"]), 1)

    def test_atomic_receipt_never_overwrites_existing_output(self):
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "existing.json"
            output.write_text("preserve original", encoding="utf-8")
            with self.assertRaises(OSError):
                tags.write_receipt_atomic(output, {"state": "uncertain"})
            self.assertEqual(output.read_text(encoding="utf-8"), "preserve original")

    def test_main_success_writes_matching_verified_receipt_and_stdout(self):
        with tempfile.TemporaryDirectory() as directory:
            api = FakeApi([record("v4.0.0-m3.13")])
            output = Path(directory) / "verified.json"
            stdout = io.StringIO()
            with patch.object(tags, "GhApi", return_value=api), contextlib.redirect_stdout(stdout):
                code = tags.main(["reserve", "--repo", REPO, "--sha", SHA, "--version", "4.0.0", "--output", str(output)])
            self.assertEqual(code, 0)
            receipt = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(receipt, json.loads(stdout.getvalue()))
            self.assertEqual(receipt["state"], "verified")
            self.assertTrue(receipt["verified"])
            self.assertEqual(receipt["ref"], "refs/tags/v4.0.0-m3.14")

    def test_main_immediate_readback_absence_remains_uncertain(self):
        class MissingReadbackApi(FakeApi):
            def request(self, method, endpoint, **kwargs):
                if "/git/ref/tags/" in endpoint:
                    raise tags.ApiError(404, {"message": "Not Found"})
                return super().request(method, endpoint, **kwargs)
        with tempfile.TemporaryDirectory() as directory:
            api = MissingReadbackApi()
            output = Path(directory) / "uncertain.json"
            with patch.object(tags, "GhApi", return_value=api), contextlib.redirect_stderr(io.StringIO()):
                code = tags.main(["reserve", "--repo", REPO, "--sha", SHA, "--version", "4.0.0", "--output", str(output)])
            self.assertEqual(code, 1)
            receipt = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(receipt["state"], "uncertain")
            self.assertIsNone(receipt["reserved"])
            self.assertEqual(receipt["reason"], "post-target-verification-unconfirmed")
            self.assertEqual(len([call for call in api.calls if call[0] == "POST"]), 1)

    def test_gh_transport_posts_json_on_stdin_and_uses_paginated_slurp(self):
        calls = []
        def runner(args, **kwargs):
            calls.append((args, kwargs))
            return subprocess.CompletedProcess(args, 0, "[]", "")
        api = tags.GhApi(runner)
        payload = {"ref": "refs/tags/v4.0.0-m3.14", "sha": SHA}
        api.request("POST", f"repos/{REPO}/git/refs", data=payload)
        api.request("GET", f"repos/{REPO}/git/matching-refs/tags/v4.0.0-m3.", paginate=True)
        self.assertEqual(json.loads(calls[0][1]["input"]), payload)
        self.assertEqual(calls[0][0][-2:], ["--input", "-"])
        self.assertNotIn(SHA, calls[0][0])
        self.assertNotIn("env", calls[0][1])
        self.assertEqual(calls[1][0][-2:], ["--paginate", "--slurp"])

    def test_gh_auth_and_malformed_json_are_fail_closed(self):
        def denied(args, **kwargs):
            return subprocess.CompletedProcess(args, 1, '{"message":"Bad credentials"}', 'gh: Bad credentials (HTTP 401)')
        with self.assertRaises(tags.ApiError) as result:
            tags.GhApi(denied).request("GET", "repos/example/project/git/refs")
        self.assertEqual(result.exception.status, 401)
        def malformed(args, **kwargs):
            return subprocess.CompletedProcess(args, 0, '{', '')
        with self.assertRaisesRegex(tags.ReservationError, "malformed JSON"):
            tags.GhApi(malformed).request("GET", "repos/example/project/git/refs")

    def test_timeout_is_not_retried_when_post_outcome_is_unknown(self):
        calls = []
        def timeout(args, **kwargs):
            calls.append(args)
            raise subprocess.TimeoutExpired(args, 60)
        with self.assertRaisesRegex(tags.ReservationError, "outcome may be unknown"):
            tags.GhApi(timeout).request("POST", "repos/example/project/git/refs", data={})
        self.assertEqual(len(calls), 1)


if __name__ == "__main__":
    unittest.main()
