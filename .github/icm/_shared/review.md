---
verified_at: 2026-10-15
verified_commit: b28ff129d49b7a79cae81e69b205c1bf3821234b
---

# Review — the contract for automated reviewers and for answering them

The short form is the entry's "Review guidelines", where Codex's GitHub reviewer reads it. This file is why each rule exists and what enforces it. The history behind all of it is pull request #59 (the port to painlessMesh v2.0.0): 38 review rounds over 26 hours, 43 `@codex review` requests, the diff grown from 1,315 to 11,263 inserted lines, about 88 % of it scope reviewers asked for, including an 806-line solver no issue requested.

## Scope

The pull request's linked issue defines done; the issue stays the anchor as scope evolves, so this file does not restate scopes. Work the issue does not need is out of scope for findings: say it once, non-blocking, and a maintainer files a follow-up issue. A reviewer's suggestion that adds a capability gets its own issue; implementing it mid-pull-request is how a port became 11k lines.

Do flag generated or build output in the diff: 2,770 lines of generated Makefile went unflagged through 38 rounds on #59 (`.gitignore` now ignores `build*/`, `bin/`, `out/`; [conventions.md](conventions.md)).

## Severity floor

- **P0/P1**: a defect with a concrete failing scenario (wrong output, crash, data loss, security). Blocks the merge; deserves a thread.
- **P2 and below without a demonstrated failing scenario**: does not block. Stated once; the author files it as a follow-up issue, links it in the thread and resolves the thread. A P2 backlog on a pull request is not a merge gate.

## No fresh-evidence ratchet

No new finding against code that exists only because of your own previous round's finding. One round of review on a fix belongs to the round that asked for it; a genuinely new defect the fix spawned is filed as an issue. On #59, 24 of 72 findings were this ratchet.

## Review round budget

Three bot-review rounds per pull request, in total across agents and sessions. The `review-budget` job of `.github/workflows/ci.yml` counts the comments that start with `@codex review` and fails past three unless a human review has approved; `CI Success` fails with it. The comment above that job records that the number mirrors `MAX_REVIEW_ITERATIONS = 3` in the Command Center's review handler (a private repository): change both together or neither.

Branch protection on `main` (repository settings, read 2026-10-15) requires one approving review and lists no required status check, so `CI Success` is a red check a person sees, not a merge lock; the approving review is the lock.

## The terminal state

When CI is green and the pull request waits only on a required human review, or the budget is spent: stop. Post one summary comment (the open threads, what was fixed, what was deferred to which issue) and hand it to a person. Do not request another bot review, do not keep polling, do not merge. Once a person approves, re-running the check passes it (the job's own error text).

## Where findings go

As GitHub issues, linked from the thread. Not appended to `docs/V2_RELEASE_REVIEW.md` or any other in-repository ledger; that document is the record of the #58 review, closed.
