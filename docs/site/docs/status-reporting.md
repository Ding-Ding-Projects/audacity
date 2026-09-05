# Status reporting

## Behaviour

This is a public, open source repository. It does not integrate with any
private status dashboard, and it never will while it stays public: a status
integration that named a private service, or that shipped credentials or
routing information for one, would be exactly the kind of leak the project's
public/private boundary exists to prevent.

Instead, status is reported through three surfaces that already exist for
every open source project of this shape, and that anyone can read without
an account:

- **The release workflow.** Every push to the default branch runs the
  project's continuous integration workflow, which builds and, where the
  build succeeds, packages and publishes a release. The workflow's own run
  page is the live, real time record of what is currently building, what
  passed, and what failed. There is nothing to duplicate here: the workflow
  run page already does exactly this.
- **`CHANGELOG.md`.** The repository root changelog is the durable, dated
  record of what shipped, in what release, with a commit link for every
  entry once it is released. See `docs/features/changelog.md` for the exact
  format and how the in-application "What's new" dialog renders it.
- **GitHub Issues and pull requests.** Day to day progress, blockers, and
  decisions live in the repository's own issue tracker and pull request
  discussions, which are the ordinary public record for an open source
  project.

## Why there is no dedicated in-application status page

A private Status Hub integration would need a shared secret or a fixed
endpoint address to reach a specific operator's server. Both of those are
things that must never enter a public repository. Building the surface
without the routing information would produce a screen that always reports
"unavailable", which is decoration, not a feature: it would satisfy nothing
except an inventory row, at the cost of dead code that nobody could ever
turn on.

## What is recorded in the completeness inventory

`docs/inventory/completeness-inventory.md` carries a `Status Hub row` entry
marked `not applicable`, pointing back to this document as the reason. That
row is intentionally honest rather than silently absent, so that a reader
scanning the inventory sees the decision that was made and why, instead of
wondering whether the row was simply forgotten.

## Failure modes

There is no failure mode to describe for a feature that is not implemented.
If the release workflow itself fails, that failure is visible on the
workflow's own run page; if `CHANGELOG.md` is stale, that is a documentation
defect, tracked the same way any other documentation defect is.

## Security considerations

None specific to this document. No credential, token, or private endpoint
address is stored, referenced, or built into this repository for status
reporting purposes.

## Verification

- `CHANGELOG.md` exists at the repository root and every entry's commit hash
  is checked against the repository at configure time
  (`buildscripts/cmake/ValidateChangelog.cmake`).
- The release workflow's own run history is the verification surface for
  build and packaging status; there is nothing in this repository to test,
  because there is no code path here to exercise.
