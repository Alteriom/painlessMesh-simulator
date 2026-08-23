# CLAUDE.md — painlessMesh-simulator

Desktop simulator for painlessMesh (Alteriom fork, v2.0.0). C++17 + CMake +
Ninja, Boost.Asio networking, yaml-cpp scenario configs. Reviewer contract
(scope, severity floor, round budget) is in `AGENTS.md` — read it before
requesting or answering a code review.

## Key directories

- `src/core/` — simulation engine (node lifecycle, event scheduler)
- `src/network/` — virtual network layer over Boost.Asio
- `src/config/` — YAML scenario/topology loading and validation
- `src/firmware/` — firmware-behaviour emulation hooks
- `src/cli/`, `src/main.cpp` — entry point
- `test/` — Catch2 unit + integration tests (`test_*.cpp`)
- `external/` — painlessMesh checkout (path via `-DPAINLESSMESH_PATH`)

## Build

```bash
mkdir -p build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DPAINLESSMESH_PATH=<path> ..
ninja && ctest
```

Never commit build output — `build*/`, `bin/`, `out/` are ignored; if
`git status` shows a Makefile or object files, the build dir is
misnamed, not an exception.

## Review loop discipline

Hard-learned on PR #59 (38 review rounds, 26 hours, 43 `@codex review`
comments, diff grew 1,315 → 11,263 lines). The rules:

- **Round budget: 3 `@codex review` requests per PR, total** — across all
  agents and sessions, not per-agent. The count is enforced by the
  `Review round budget` step in the required CI Success check and mirrors
  the Command Center's `MAX_REVIEW_ITERATIONS = 3`.
- **REVIEW_REQUIRED is a terminal state for agents.** When CI is green and
  the PR is blocked only on a required human review: stop. Post one
  summary comment (open threads, what was fixed, what was deferred) and
  hand to a human approver. Do not request another bot review, do not
  keep polling, do not merge.
- **P2 findings without a failing scenario become follow-up issues**, not
  fix commits on the PR. File the issue, link it in the thread, resolve
  the thread.
- **Scope stays anchored to the PR's linked issue.** A reviewer suggestion
  that adds capability gets its own issue — implementing it mid-PR is how
  a port becomes 11k lines.
- **Review findings are recorded as GitHub issues**, not appended to
  `docs/V2_RELEASE_REVIEW.md` or any other in-repo ledger.
