---
verified_at: 2026-10-15
verified_commit: b28ff129d49b7a79cae81e69b205c1bf3821234b
---

# Build, test, and what counts as evidence

What CI runs is `.github/workflows/ci.yml`; read its steps rather than copying them anywhere. Every build and test there runs inside the `builder` stage of `Dockerfile`, and so should yours.

## Build and run, as CI does

```bash
git submodule update --init --recursive            # external/painlessMesh at the pinned commit
docker build --target builder -t painlessmesh-simulator:builder .
docker run --rm -v "$PWD:/build" -w /build painlessmesh-simulator:builder bash -c   "mkdir -p build && cd build && cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_TESTING=ON .. && ninja && ctest --output-on-failure"
docker run --rm -v "$PWD:/build" -w /build painlessmesh-simulator:builder   ./scripts/ci_integration_check.sh ./build/bin/painlessmesh-simulator
```

Outside Docker, the same with the packages `Dockerfile` installs (`README.md` "Building from Source"); `-DPAINLESSMESH_PATH=<path>` builds against another painlessMesh checkout instead of the submodule (`CMakeLists.txt`). The binaries land in `build/bin/`: `painlessmesh-simulator` and `simulator_tests` (one Catch2 executable; run one case with `./build/bin/simulator_tests "<name or [tag]>"`). The CMake options (`ENABLE_TESTING`, `ENABLE_COVERAGE`, `ENABLE_BENCHMARKS`, `BUILD_EXAMPLES`, `BUILD_DOCS`) are at the top of `CMakeLists.txt`; benchmarks and examples are placeholders there.

On Windows, use the image (`docker-quickstart.ps1`, `docs/DOCKER_GUIDE.md`) or WSL2 (`docs/WSL2_SETUP_GUIDE.md`); the native MSVC build is experimental and not what CI runs (`docs/WINDOWS_BUILD_DECISION.md`).

## Linux in the image is authoritative

A bare Ubuntu runner fails six partition tests with "bind: Permission denied" from Boost.Asio, an environment difference and not a defect (the comment above "Set up Docker Buildx" in `upstream-drift.yml`). A reproduction or a green run counts when it ran in the `builder` image.

## What CI runs, and when

| Workflow, job | Runs | On |
|---|---|---|
| `ci.yml` `lint` | clang-format and cppcheck, both `continue-on-error`: advisory, never red | push to `main`, `master`, `develop`, `feature/**`; pull requests; Sundays 03:00 UTC |
| `ci.yml` `build-docker` | Debug and Release builds in the image | same |
| `ci.yml` `test` | `ctest` with coverage on, then lcov and Codecov (both allowed to fail) | same |
| `ci.yml` `integration-test` | `scripts/ci_integration_check.sh`: every shipped scenario must validate, and the runs it checks must do what they claim | same |
| `ci.yml` `benchmark` | `examples/scenarios/stress_test.yaml` at `--time-scale 10.0` | the schedule, or `[benchmark]` in the head commit message |
| `ci.yml` `review-budget` | counts `@codex review` comments ([review.md](review.md)) | pull requests |
| `ci.yml` `ci-success` | `CI Success`: red if lint, build, test or integration failed, or the review budget failed | every run |
| `upstream-drift.yml` | builds, tests and runs the integration check against painlessMesh `main` and `Feat/next-release`, ignoring the pin; on the schedule a failure opens or comments the drift issue | Sundays 03:00 UTC; pull requests that move `external/painlessMesh` |
| `icm-check.yml` | the workspace files compile to the ICM standard | changes to the entry files or `.github/icm/`; weekly with `--strict` |

The `docs` job of `ci.yml` generates nothing yet (no `Doxyfile`); CodeQL runs as GitHub's default setup, not from a workflow here (the comment above the disabled `security` job).

## Evidence

- A test in `test/` (a `test_*.cpp`, added to the `simulator_tests` list in `CMakeLists.txt`) that fails before the change and passes after, or for run-level behaviour a scenario plus an assertion in `scripts/ci_integration_check.sh`. Exit code 0 alone proves nothing: the script's header says why.
- A scenario that cannot run yet because its action has no runtime class is listed in `expected_unsupported_action` of `scripts/ci_integration_check.sh` with that one action; implementing the action removes the line.
- The simulator is not hardware: say so when a change's claim depends on radio or timing behaviour (`DEVELOPMENT_STATUS.md` "Current status").
