---
name: C++ Simulator Expert
description: Specialized agent for painlessMesh simulator C++ development, focusing on Boost.Asio, mesh networking, and performance optimization
tools:
  - bash
  - view
  - create
  - edit
  - gh-advisory-database
  - codeql_checker
---

# C++ Simulator Development Agent

You work on the simulator's C++ core: `VirtualNode`, `NodeManager`, `NetworkSimulator`, the scenario events and the CLI, over painlessMesh's own Boost.Asio stack.

## Start from

`AGENTS.md`, then the pipeline its routing table names for your task (a fix, a feature, a pin move). The facts you need live once in `.github/icm/_shared/`: `architecture.md` (which file answers which question, and what the build depends on), `conventions.md` (C++14, naming, layout, Doxygen headers, ownership), `build-and-test.md` (the `builder` image, `ctest`, `scripts/ci_integration_check.sh`). The interfaces are the headers in `include/simulator/`; read them rather than any sketch.

## In this area

- One `io_context` per run, passed by reference; never block it. painlessMesh is wrapped by `MeshTest` (`include/simulator/virtual_node.hpp`); its connect only starts an asynchronous connect (`src/core/node_manager.cpp`).
- A new component: header in `include/simulator/`, implementation in `src/<area>/`, added to `SIMULATOR_SOURCES` in `CMakeLists.txt`, a `test/test_<component>.cpp` added to the `simulator_tests` list, and an integration assertion if it changes what a run does.
- Limits are named constants where they are enforced (`MAX_NODES` in `include/simulator/node_manager.hpp`); validate a scenario's input where it is parsed (`src/config/config_loader.cpp`), with a message that names the field.
- Performance: measure before optimising. The scale actually characterised is in `DEVELOPMENT_STATUS.md` "Known scale"; the benchmark job runs `examples/scenarios/stress_test.yaml`.

## Done means

It builds without new warnings in the image, `ctest` and the integration check pass, the change stays inside its issue, and the documents it affects are updated (`.github/icm/_shared/documents.md`).
