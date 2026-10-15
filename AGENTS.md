# AGENTS.md — painlessMesh-simulator

A desktop simulator for the painlessMesh ESP32/ESP8266 mesh library (Alteriom fork, pinned as a submodule): virtual nodes run painlessMesh's own Boost.Asio stack, YAML scenarios declare the topology, the network conditions and an event timeline, and CI asserts on what the simulator did. C++14, CMake, Ninja, Catch2; public, MIT. Humans start at `README.md` and `GETTING_STARTED.md`. This file is the entry for every agent: Claude Code reads it through `CLAUDE.md`, Codex and its GitHub reviewer read it directly, and Copilot is pointed here by `.github/copilot-instructions.md`.

## Where things live

| Path | What it holds |
|---|---|
| `src/`, `include/simulator/` | the simulator: core (nodes), network, config, scenario events, firmware, CLI — mapped in [architecture.md](.github/icm/_shared/architecture.md) |
| `external/painlessMesh` | the library under test, a submodule on the branch `.gitmodules` names |
| `test/`, `scripts/ci_integration_check.sh` | the Catch2 suite (`simulator_tests`) and the behavioural gate CI runs on every shipped scenario |
| `examples/` | scenarios (`examples/scenarios/*.yaml`), firmware examples, a PlatformIO integration example |
| `docs/` | user guides and the v2.0.0 review — indexed in [documents.md](.github/icm/_shared/documents.md) |
| `Dockerfile`, `docker/` | the `builder` image every CI build and test runs in |
| `.github/workflows/` | `ci.yml` (lint, build, tests, integration, review round budget, `CI Success`), `upstream-drift.yml`, `icm-check.yml` |
| [`.github/icm/_shared/`](.github/icm/_shared/CONTEXT.md) | what every job below draws on, one file per question |
| [`.github/icm/01-fix/`](.github/icm/01-fix/CLAUDE.md) | fix a defect |
| [`.github/icm/02-feature/`](.github/icm/02-feature/CLAUDE.md) | add a capability: an event action, a network condition, a firmware, a CLI option, a scenario |
| [`.github/icm/03-deps/`](.github/icm/03-deps/CLAUDE.md) | review a Dependabot bump or move the painlessMesh pin |

## Route by what just happened

| Route | If | Go to | Then stop at |
|---|---|---|---|
| triage | an issue or report arrives and its kind is unclear | read it whole (`gh issue view <n> --comments`), then take the row below that fits | the row chosen, said to the person |
| fix | a defect is reported (an issue, a failing test, a scenario that does the wrong thing) | [01_reproduce](.github/icm/01-fix/stages/01_reproduce/CONTEXT.md) | a person has seen the reproduction fail |
| ci-red | `CI` is red on `main`, or `Upstream Drift` opened its issue | [01_reproduce](.github/icm/01-fix/stages/01_reproduce/CONTEXT.md), with the run log as the report | a person has seen the reproduction fail |
| feature | a new event action, network condition, firmware, CLI option or scenario is asked for | [01_spec](.github/icm/02-feature/stages/01_spec/CONTEXT.md) | a person approves the spec |
| fix-to-feature | a fix turns out to need an unimplemented event action or a new scenario field | [01_spec](.github/icm/02-feature/stages/01_spec/CONTEXT.md), with the fix run's `reproduction.md` as the request | a person approves the spec |
| deps | Dependabot opened a pull request, or the painlessMesh pin is to move | [01_review](.github/icm/03-deps/stages/01_review/CONTEXT.md) | a person decides to merge it or not |
| review | a review is requested of you, or a reviewer's finding arrives on a pull request | [review.md](.github/icm/_shared/review.md) | the finding answered inside the budget, or the pull request handed to a person |
| docs | a document is wrong or missing and no code has to change | [documents.md](.github/icm/_shared/documents.md) | a person reads the edit beside the owning file |
| next | a stage's output was approved | the next numbered stage of the same job | a person reads that stage's output |
| status | asked for status | scan `.github/icm/*/stages/*/output/` | report what exists |

## Review guidelines

For Codex, Copilot and Claude reviewing a pull request here, and for agents answering their findings ([review.md](.github/icm/_shared/review.md) has the evidence):

- Judge the pull request against its linked issue; work the issue does not need is not a finding (say it once, non-blocking, and a maintainer files it). Do flag build output in the diff (`build*/`, `bin/`, `out/`).
- P0/P1 needs a concrete failing scenario and blocks; P2 and below without one does not block: the author turns it into a follow-up issue and resolves the thread.
- No fresh-evidence ratchet: no new finding against code that exists only because of your previous round's finding.
- Three `@codex review` rounds per pull request, in total; the `Review round budget` job of `ci.yml` enforces it. Past it, or when only a human review is missing with CI green, stop: one summary comment, hand it to a person, never merge. Findings become GitHub issues, not lines in an in-repository ledger.

## Agents connecting remotely

This repository becomes the ICM workspace `painlessmesh-simulator` on MemoryRelay, created by a person from this repository's `main`; the Alteriom hub `portfolio` then delegates to it. Until the workspace exists, an agent on a clone reads these files directly: this file is the entry and the table above is the route. Once it exists, an agent that is not on a clone reaches the same files over MCP at `https://api.memoryrelay.net/mcp` with a key in `MEMORYRELAY_API_KEY`; none of it holds a secret.

- Start with `icm_context_build` for the route that fits (`{kind: "route", id: "fix"}`), not with the whole repository.
- Report what you read with `icm_report_reads` when a stage ends, and what the task came to with `icm_report_outcome`.
- Never write through MCP: a change is a branch and a pull request here, read and merged by a person.

## Rules

- Nothing moves to the next stage until a person has read the output of the last one.
- A stage writes to its own `output/`, which is not committed: one run per worktree or branch; clear that job's `stages/*/output/` when its pull request merges or is abandoned.
- `.github/icm/` points at the files that own each fact. When they disagree, the owning file is right: correct `.github/icm/` in the same pull request and bump its `verified_commit`.
- Build and test in the `builder` image, as CI does ([build-and-test.md](.github/icm/_shared/build-and-test.md)); never commit build output.
- Public repository: no host name, address, secret or private repository path of ours in code or in these files.
