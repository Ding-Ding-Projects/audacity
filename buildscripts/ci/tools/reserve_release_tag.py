#!/usr/bin/env python3
"""Reserve a new immutable release tag through gh, or verify its target.

This reserves a name, not a build or publication lock. A failed build can leave
its reserved tag behind. Never delete or reuse it; the next attempt advances.
"""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from typing import Any

MAX_SEQUENCE = 2_147_483_647  # Matches the packaging script's Int32 conversion.
SHA_PATTERN = re.compile(r"[0-9a-fA-F]{40}")
VERSION_PATTERN = re.compile(r"(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)\.(?:0|[1-9][0-9]*)")


class ReservationError(RuntimeError):
    pass


def attempted_identity(receipt: dict) -> str:
    return (f"repository={receipt['repository']} tag={receipt['tag']} ref={receipt['ref']} "
            f"sha={receipt['sha']} attempt={receipt['attempts']}")


class UncertainReservation(ReservationError):
    def __init__(self, receipt: dict, reason: str):
        self.receipt = dict(receipt, state="uncertain", reserved=None, verified=False, reason=reason)
        super().__init__(f"Uncertain reservation: {attempted_identity(self.receipt)}. "
                         "Do not retry the POST, delete or reuse the tag. An absent immediate read does not settle its outcome.")


def write_receipt_atomic(path: Path, receipt: dict) -> None:
    # Stage beside the destination, flush, then link it into place exclusively.
    # A hard link publishes the complete file atomically without overwriting an
    # existing receipt on Windows or POSIX. Retain staging on a write/link error.
    staging = None
    try:
        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", dir=path.parent,
                                         prefix=f".{path.name}.", suffix=".tmp", delete=False) as stream:
            staging = Path(stream.name)
            json.dump(receipt, stream, indent=2)
            stream.write("\n")
            stream.flush()
            os.fsync(stream.fileno())
        os.link(staging, path)
    except OSError as error:
        retained = f" Staging retained for inspection: {staging}." if staging else ""
        raise OSError("Atomic receipt persistence failed." + retained) from error
    # The destination already contains complete flushed bytes if staging removal
    # cannot finish. Do not turn a saved receipt into an apparent write failure.
    try:
        staging.unlink()
    except OSError:
        pass


class ApiError(ReservationError):
    def __init__(self, status: int | None, body: Any):
        self.status = status
        self.body = body
        super().__init__(f"GitHub API request failed (HTTP {status if status else 'unknown'}).")

    def is_reference_collision(self) -> bool:
        if self.status != 422 or not isinstance(self.body, dict):
            return False
        if self.body.get("message") == "Reference already exists":
            return True
        errors = self.body.get("errors")
        return isinstance(errors, list) and any(
            isinstance(error, dict)
            and error.get("resource") == "Reference"
            and error.get("code") == "already_exists"
            for error in errors
        )


class GhApi:
    """Use the existing gh credential store; never read credentials ourselves."""

    def __init__(self, runner=subprocess.run):
        self.runner = runner

    def request(self, method: str, endpoint: str, *, data=None, paginate=False):
        args = ["gh", "api", "--method", method, endpoint,
                "-H", "Accept: application/vnd.github+json",
                "-H", "X-GitHub-Api-Version: 2022-11-28"]
        if paginate:
            args += ["--paginate", "--slurp"]
        if data is not None:
            args += ["--input", "-"]
        try:
            result = self.runner(args, input=json.dumps(data) if data is not None else None,
                                 text=True, capture_output=True, check=False, timeout=60)
        except (OSError, subprocess.TimeoutExpired) as error:
            # An interrupted POST might already have reserved a tag. Do not retry
            # an unknown outcome or expose CLI diagnostics containing account data.
            raise ReservationError("gh request could not complete; outcome may be unknown. Do not reuse a possibly reserved tag.") from error
        try:
            body = json.loads(result.stdout)
        except (TypeError, json.JSONDecodeError):
            body = None
        if result.returncode:
            match = re.search(r"\(HTTP ([0-9]{3})\)", result.stderr or "")
            status = int(match.group(1)) if match else None
            if status is None and isinstance(body, dict) and re.fullmatch(r"[0-9]{3}", str(body.get("status", ""))):
                status = int(body["status"])
            raise ApiError(status, body)
        if body is None:
            raise ReservationError("gh returned malformed JSON; no reservation retry is safe.")
        return body


def validate_inputs(repo: str, sha: str) -> str:
    if not re.fullmatch(r"[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+", repo) or any(
        part in (".", "..") for part in repo.split("/")
    ):
        raise ReservationError("An explicit owner/repository is required.")
    if not isinstance(sha, str) or not SHA_PATTERN.fullmatch(sha) or sha == "0" * 40:
        raise ReservationError("The candidate must be an exact nonzero 40-digit commit SHA.")
    return sha.lower()


def validate_version(version: str) -> None:
    if not isinstance(version, str) or not VERSION_PATTERN.fullmatch(version) or any(
        len(part) > 10 or int(part) > MAX_SEQUENCE for part in version.split(".")
    ):
        raise ReservationError("The product version must be canonical major.minor.patch within Int32 bounds.")


def validate_ref(record: Any) -> str:
    if not isinstance(record, dict):
        raise ReservationError("Malformed Git reference record.")
    ref = record.get("ref")
    obj = record.get("object")
    if (not isinstance(ref, str) or not ref.startswith("refs/tags/")
            or not re.fullmatch(r"refs/tags/[^\s~^:?*\\\[\]\x00-\x20\x7f]+", ref)
            or any(part.startswith(".") or part.endswith((".", ".lock")) for part in ref.split("/"))
            or ".." in ref or "//" in ref or "@{" in ref or ref.endswith("/")
            or not isinstance(obj, dict) or obj.get("type") not in ("commit", "tag")
            or not isinstance(obj.get("sha"), str) or not SHA_PATTERN.fullmatch(obj["sha"])
            or obj["sha"] == "0" * 40):
        raise ReservationError("Malformed Git tag reference or object identity.")
    return ref


def choose_tag(pages: Any, version: str, minimum: int) -> str:
    validate_version(version)
    if isinstance(minimum, bool) or not isinstance(minimum, int) or not 1 <= minimum <= MAX_SEQUENCE:
        raise ReservationError("Minimum release sequence must be between 1 and 2147483647.")
    if not isinstance(pages, list) or not pages or any(not isinstance(page, list) for page in pages):
        raise ReservationError("Malformed paginated tag inventory.")
    prefix = f"refs/tags/v{version}-m3."
    highest = 0
    seen = set()
    for page in pages:
        for record in page:
            ref = validate_ref(record)
            if ref in seen:
                raise ReservationError("Duplicate tag reference in paginated inventory.")
            seen.add(ref)
            if not ref.startswith(prefix):
                continue  # Foreign tags and other product versions do not own this series.
            suffix = ref[len(prefix):]
            if not re.fullmatch(r"0|[1-9][0-9]*", suffix) or len(suffix) > 10 or int(suffix) > MAX_SEQUENCE:
                raise ReservationError("Malformed release sequence in matching tag namespace.")
            highest = max(highest, int(suffix))
    sequence = max(minimum, highest + 1)
    if sequence > MAX_SEQUENCE:
        raise ReservationError("Release sequence exhausted; do not wrap or reuse a tag.")
    return f"v{version}-m3.{sequence}"


def verify_tag(api, repo: str, sha: str, tag: str) -> dict:
    sha = validate_inputs(repo, sha)
    match = re.fullmatch(r"v([0-9]+\.[0-9]+\.[0-9]+)-m3\.([1-9][0-9]*)", tag)
    if not match:
        raise ReservationError("A canonical reserved release tag is required.")
    validate_version(match.group(1))
    if len(match.group(2)) > 10 or int(match.group(2)) > MAX_SEQUENCE:
        raise ReservationError("Reserved sequence is outside packaging bounds.")
    record = api.request("GET", f"repos/{repo}/git/ref/tags/{tag}")
    if (validate_ref(record) != f"refs/tags/{tag}" or record["object"]["type"] != "commit"
            or record["object"]["sha"].lower() != sha):
        raise ReservationError("Reserved tag does not point directly to the exact candidate commit.")
    return {"tag": tag, "sha": sha, "verified": True}


def reserve_tag(api, repo: str, sha: str, version: str, minimum: int = 1) -> dict:
    sha = validate_inputs(repo, sha)
    validate_version(version)
    # Validate bounds before making any API request.
    choose_tag([[]], version, minimum)
    commit = api.request("GET", f"repos/{repo}/git/commits/{sha}")
    if not isinstance(commit, dict) or commit.get("sha") != sha:
        raise ReservationError("Candidate commit lookup did not confirm the exact SHA.")
    for attempt in range(1, 4):
        pages = api.request("GET", f"repos/{repo}/git/matching-refs/tags/v{version}-m3.?per_page=100", paginate=True)
        tag = choose_tag(pages, version, minimum)
        ref = f"refs/tags/{tag}"
        sequence = int(tag.rsplit(".", 1)[1])
        receipt = {"schemaVersion": 1, "repository": repo, "tag": tag, "ref": ref, "sha": sha,
                   "productVersion": version, "sequence": sequence,
                   "packageVersion": f"{version}-m3{sequence:03d}", "attempts": attempt}
        try:
            created = api.request("POST", f"repos/{repo}/git/refs", data={"ref": ref, "sha": sha})
        except ApiError as error:
            if error.is_reference_collision() and attempt < 3:
                continue  # Re-read the complete inventory before the next create-only attempt.
            if error.is_reference_collision():
                raise ReservationError("Tag reservation collided three times; stop and coordinate the publishers.") from error
            if error.status is None or error.status >= 500:
                raise UncertainReservation(receipt, "post-http-outcome-unconfirmed") from error
            raise
        except ReservationError as error:
            # GhApi can raise before it has decoded the POST response. Attach
            # identity here, where the exact attempted reference is still known.
            raise UncertainReservation(receipt, "post-response-unconfirmed") from error
        # POST is create-only. No PATCH, update, force, reuse or deletion path exists.
        try:
            if (validate_ref(created) != ref or created["object"]["type"] != "commit"
                    or created["object"]["sha"].lower() != sha):
                raise ReservationError("Reservation response is inconsistent.")
            verify_tag(api, repo, sha, tag)
        except ReservationError as error:
            raise UncertainReservation(receipt, "post-target-verification-unconfirmed") from error
        return dict(receipt, state="verified", reserved=True, verified=True)
    raise AssertionError("Unreachable reservation boundary")


def main(argv=None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    sub = parser.add_subparsers(dest="command", required=True)
    reserve = sub.add_parser("reserve", help="Create and verify a new tag at an explicitly selected candidate")
    verify = sub.add_parser("verify", help="Verify a reserved tag immediately before publication")
    for command in (reserve, verify):
        command.add_argument("--repo", required=True)
        command.add_argument("--sha", required=True)
    reserve.add_argument("--version", required=True)
    reserve.add_argument("--minimum", type=int, default=1)
    reserve.add_argument("--output", required=True, type=Path, help="New atomic receipt file, including uncertain outcomes; existing files are never overwritten")
    verify.add_argument("--tag", required=True)
    args = parser.parse_args(argv)
    try:
        # Refuse an existing receipt before reserving anything.
        if args.command == "reserve" and args.output and args.output.exists():
            raise ReservationError("Reservation receipt already exists; verify it instead of reserving another tag.")
        if args.command == "reserve" and args.output and not args.output.parent.is_dir():
            raise ReservationError("Reservation receipt parent directory must already exist.")
        api = GhApi()
        if args.command == "reserve":
            result = reserve_tag(api, args.repo, args.sha, args.version, args.minimum)
            if args.output:
                # Output failure never rolls back the already-created immutable tag.
                try:
                    write_receipt_atomic(args.output, result)
                except OSError as error:
                    raise ReservationError(f"Tag {result['tag']} was reserved and verified, but its receipt could not be saved; "
                                           f"{attempted_identity(result)}. {error} Retain the tag and investigate.") from error
        else:
            result = verify_tag(api, args.repo, args.sha, args.tag)
        print(json.dumps(result, sort_keys=True))
        return 0
    except UncertainReservation as error:
        # Always report exact identity even if receipt persistence itself fails.
        print(f"Release reservation stopped: {error}", file=sys.stderr)
        try:
            write_receipt_atomic(args.output, error.receipt)
            print(f"Uncertain receipt saved: {args.output}", file=sys.stderr)
        except OSError as persistence_error:
            print(f"Receipt persistence failed for {attempted_identity(error.receipt)}. {persistence_error}", file=sys.stderr)
            print(json.dumps(error.receipt, sort_keys=True), file=sys.stderr)
        return 1
    except (ReservationError, OSError) as error:
        print(f"Release reservation stopped: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
