---
verified_at: 2026-10-15
verified_commit: b28ff129d49b7a79cae81e69b205c1bf3821234b
---

# Documents — what a person reads here, and who owns its facts

The agent-facing files are `AGENTS.md` (the entry), this folder, `.github/copilot-instructions.md` and the Copilot custom agents in `.github/agents/`. Everything below is a document a person reads: each restates facts that code, a scenario or a workflow owns, and the owner is right when they disagree. Each linked document is pinned in the workspace, so an agent can be given one by path.

To correct a document: edit it, check every changed claim against its owner (run the command, read the line), and open a pull request that names the owning file for each. No build publishes these pages; `README.md` and `docs/` are read on GitHub.

## Start here

| Document | What it answers | Owner of the facts |
|---|---|---|
| [README.md](../../../README.md) | what the simulator is, quick start, the command line and exit codes, building | `src/cli/cli_parser.cpp`, `CMakeLists.txt`, `Dockerfile` |
| [GETTING_STARTED.md](../../../GETTING_STARTED.md) | the first run on each platform | `docker-quickstart.sh`, `Dockerfile` |
| [DEVELOPMENT_STATUS.md](../../../DEVELOPMENT_STATUS.md) | what works and what does not, written against the binary | `src/scenario/event_factory.cpp`, `scripts/ci_integration_check.sh` |
| [CONTRIBUTING.md](../../../CONTRIBUTING.md) | setup, submodules, coding and test standards, the pull request | `.gitmodules`, `CMakeLists.txt` |

## Using the simulator

| Document | What it answers | Owner of the facts |
|---|---|---|
| [docs/CONFIGURATION_GUIDE.md](../../../docs/CONFIGURATION_GUIDE.md) | every scenario key, topology and event | `src/config/config_loader.cpp`, `src/config/topology_planner.cpp` |
| [docs/SIMULATOR_QUICKSTART.md](../../../docs/SIMULATOR_QUICKSTART.md) | usage examples, the command line | `src/cli/cli_parser.cpp` |
| [docs/NETWORK_LATENCY.md](../../../docs/NETWORK_LATENCY.md), [docs/PACKET_LOSS.md](../../../docs/PACKET_LOSS.md), [docs/BANDWIDTH_LIMITING.md](../../../docs/BANDWIDTH_LIMITING.md) | the three network conditions | `src/network/network_simulator.cpp` |
| [docs/CONNECTION_EVENTS.md](../../../docs/CONNECTION_EVENTS.md), [docs/PARTITION_EVENTS.md](../../../docs/PARTITION_EVENTS.md), [docs/CUSTOM_EVENTS.md](../../../docs/CUSTOM_EVENTS.md) | link and partition events; writing a new event class | `src/scenario/events/`, `src/scenario/event_factory.cpp` |
| [docs/BRIDGE_TESTING_GUIDE.md](../../../docs/BRIDGE_TESTING_GUIDE.md) | bridge failover scenarios | `src/firmware/bridge_ino_firmware.cpp`, `examples/scenarios/bridge_failover_test.yaml` |
| [docs/DOCKER_GUIDE.md](../../../docs/DOCKER_GUIDE.md), [docs/WSL2_SETUP_GUIDE.md](../../../docs/WSL2_SETUP_GUIDE.md) | building on Windows through the image or WSL2 | `Dockerfile`, `docker-quickstart.ps1` |

## Testing firmware

| Document | What it answers | Owner of the facts |
|---|---|---|
| [docs/INTEGRATING_INTO_YOUR_PROJECT.md](../../../docs/INTEGRATING_INTO_YOUR_PROJECT.md) | running an existing ESP32/ESP8266 project's logic in the simulator | `examples/integration/platformio-example/` |
| [docs/FIRMWARE_DEVELOPMENT_GUIDE.md](../../../docs/FIRMWARE_DEVELOPMENT_GUIDE.md), [docs/FIRMWARE_INTEGRATION.md](../../../docs/FIRMWARE_INTEGRATION.md) | the two firmware tiers, `FirmwareBase`, registration | `include/simulator/firmware/firmware_base.hpp`, `include/simulator/firmware/firmware_factory.hpp` |
| [docs/FIRMWARE_TESTING_GUIDE.md](../../../docs/FIRMWARE_TESTING_GUIDE.md) | running painlessMesh's `.ino` examples as firmware | `include/simulator/firmware/ino_firmware_wrapper.hpp`, `examples/scenarios/ino_firmware_test.yaml` |
| [docs/LIBRARY_VALIDATION_SUMMARY.md](../../../docs/LIBRARY_VALIDATION_SUMMARY.md) | the library-validation firmware and what it checks | `src/firmware/library_validation_firmware.cpp`, `examples/scenarios/library_validation.yaml` |
| [examples/README.md](../../../examples/README.md), [examples/firmware/README.md](../../../examples/firmware/README.md), [examples/integration/README.md](../../../examples/integration/README.md) | the examples, folder by folder | the files beside them |

## Records

| Document | What it is |
|---|---|
| [docs/V2_RELEASE_REVIEW.md](../../../docs/V2_RELEASE_REVIEW.md) | the review of the simulator against painlessMesh v2.0.0 (issue #58, closed) and the port it led to. Closed: findings go to issues now ([review.md](review.md)) |
| [scripts/README.md](../../../scripts/README.md) | the Windows build scripts and the issue-creation helpers |

## Known drift

- **`README.md`** "Development Status" and "Roadmap" describe Phase 1 of November 2025; `DEVELOPMENT_STATUS.md` supersedes them. Its "Architecture Documentation (Coming Soon)" lists `docs/CONFIGURATION_GUIDE.md`, which exists; the other two pages it names do not. Its quick start runs `./painlessmesh-simulator` from the build folder; the binary is in `build/bin/` (`CMAKE_RUNTIME_OUTPUT_DIRECTORY` in `CMakeLists.txt`). It points at GitHub Discussions, which are disabled, and calls Catch2 a submodule; `CMakeLists.txt` fetches it with `FetchContent`.
- **`CONTRIBUTING.md`** names a `develop` branch as the target of pull requests and `git pull upstream develop`; there is no `develop` branch, pull requests go to `main`. It lists ARCHITECTURE, API_REFERENCE and USER_GUIDE pages under `docs/` that were never written, and a `ninja coverage` target `CMakeLists.txt` does not define (coverage is the `ENABLE_COVERAGE` option plus lcov, as in `ci.yml`). Its "Error Handling" example returns `std::optional`, which C++14 does not have; the code uses `boost::optional` ([conventions.md](conventions.md)).
- **`SECURITY.md`** is a template from another project: it names a different package, a placeholder reporting address and a 1.0.x line this repository never released (`CMakeLists.txt` says 0.1.0, `DEVELOPMENT_STATUS.md` 0.2.0-alpha). Not pinned until a person rewrites it.
- **`DEVELOPMENT_STATUS.md`** calls the upstream drift run nightly; `upstream-drift.yml` runs it weekly (Sundays). It lists macOS and Windows builds; CI builds Linux only, in the image.
- **`CHANGELOG.md`** has only an "Unreleased" stub; nothing is released or tagged here.
- **Planning-era documents**, superseded by the code, `DEVELOPMENT_STATUS.md` and this folder, and not pinned: under `docs/`, SIMULATOR_PLAN, SIMULATOR_SUMMARY, SIMULATOR_INDEX, IMPLEMENTATION_ROADMAP, IMPLEMENTATION_SUMMARY, PHASE1_STATUS, SETUP_COMPLETE, SETUP_SUMMARY, GITHUB_ISSUES_TEMPLATES, ISSUE_138_ANALYSIS, LIBRARY_VALIDATION_PLAN, LIBRARY_VALIDATION_QUICK_START, FIRMWARE_INTEGRATION_SUMMARY, PLATFORM_COMPARISON and the three WINDOWS_BUILD pages; at the root, ACTION_REQUIRED, QUICK_ISSUE_CREATION, BUILD_WINDOWS_STATUS, WINDOWS_FIXES_NEEDED, ISSUE_138_SUMMARY, ISSUE_138_VALIDATION and FIRMWARE_TESTING_SOLUTION; under `.github/`, the Phase 1 issue templates, AGENT_SESSION_TEMPLATE and the three MCP pages (a GitHub MCP setup of November 2025, superseded for agents by `AGENTS.md`). `.github/copilot-mcp.json` configures a server whose `app.js` is not in this repository.
