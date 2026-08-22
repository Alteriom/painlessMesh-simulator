# AGENTS.md — painlessMesh-simulator

Desktop simulator for the painlessMesh ESP32/ESP8266 mesh library (C++17,
CMake, Boost.Asio). This file is the contract for **automated reviewers**
(Codex, Copilot, Claude) and for agents responding to their findings. Author
workflow rules live in `CLAUDE.md`; C++ style lives in
`.github/copilot-instructions.md`.

## Review scope

- Judge the PR **against its linked issue** — the issue number in the PR
  title/body defines what "done" means. Do not restate the scope here in
  prose; the issue is the anchor and stays authoritative as scope evolves.
- Work the PR does not need in order to close its issue is **out of scope
  for review findings**. If a missing capability genuinely matters, say so
  in a non-blocking comment and let a maintainer file a follow-up issue —
  do not request the feature as a review finding. (Context: PR #59 grew
  from 1,315 inserted lines to 11,263 — ~88% reviewer-generated scope —
  including an 806-line solver no issue asked for.)
- **Do flag** generated or build output in the diff. `build*/`, `bin/`,
  `out/` must never be tracked (2,770 lines of generated Makefile went
  unflagged through 38 review rounds on PR #59).

## Severity floor

- **P0/P1** — a defect with a concrete failing scenario (wrong output,
  crash, data loss, security). These block merge and deserve a thread.
- **P2 and below without a demonstrated failing scenario** — do not block
  the PR. State it once; the author converts it to a follow-up issue and
  resolves the thread. A P2 backlog on the PR is not a merge gate.

## No fresh-evidence ratchet

Do not raise a new finding against code that only exists because of your
own previous round's finding ("fresh evidence after the previous fix").
One round of review on a fix is part of the round that requested it; if
the fix itself spawns a genuinely new defect, file it as an issue rather
than extending the loop. On PR #59, 24 of 72 findings were this ratchet.

## Review round budget

**3 bot-review rounds per PR.** This mirrors `MAX_REVIEW_ITERATIONS = 3`
in the Alteriom Command Center (`backend/src/claude_workflows/
pr_review_handler.py`) — one number governs both review pathways; change
them together or not at all. The budget is enforced mechanically by the
`Review round budget` step of the required **CI Success** check, which
counts `@codex review` requests on the PR. Past the budget the check
fails until a human review approves the PR; the terminal state is a
human decision, not a fourth round.
