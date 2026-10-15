---
verified_at: 2026-10-15
verified_commit: b28ff129d49b7a79cae81e69b205c1bf3821234b
---

# Architecture — where to look for each question

One binary (`painlessmesh-simulator`, from `src/main.cpp` and `src/cli/cli_parser.cpp`) over one static library (`simulator_lib`, every other file under `src/`), declared in `CMakeLists.txt`. Headers are under `include/simulator/`, one per class; the `.cpp` of the same name is under `src/<area>/`.

| Question | Owner |
|---|---|
| what a run does, in order: parse, load, create nodes, wire the topology, schedule events, run, report | `src/main.cpp` |
| one virtual ESP32/ESP8266: its painlessMesh instance, firmware, metrics | `include/simulator/virtual_node.hpp`, `src/core/virtual_node.cpp` |
| many nodes: create, start, stop, crash, restart, connectivity | `include/simulator/node_manager.hpp`, `src/core/node_manager.cpp` |
| the painlessMesh globals and Arduino shims the library needs on a desktop | `src/core/painlessmesh_support.cpp`, `include/simulator/platform_compat.hpp` (included first on Windows) |
| latency, packet loss, bandwidth (token bucket) | `include/simulator/network_simulator.hpp`, `src/network/network_simulator.cpp` |
| the scenario file: every key, its default, its validation, the event action names | `include/simulator/config_loader.hpp`, `src/config/config_loader.cpp` (action names near line 1013) |
| `topology:` turned into the links to wire, as a pure function | `include/simulator/topology_planner.hpp`, `src/config/topology_planner.cpp` |
| the event timeline: which actions have a runtime class, which are reported as unsupported | `src/scenario/event_factory.cpp`, `src/scenario/event_scheduler.cpp`, `src/scenario/events/` |
| firmware: the base class, the factory, self-registration | `include/simulator/firmware/firmware_base.hpp`, `include/simulator/firmware/firmware_factory.hpp` (`REGISTER_FIRMWARE(name, type)`), `src/firmware/` |
| the command line and its exit codes | `include/simulator/cli_options.hpp`, `src/cli/cli_parser.cpp`, `README.md` "Command-Line Options" |

## Things the build depends on that are easy to break

- **Firmware self-registers at static-init time**, so `simulator_lib` is linked whole into the binary; without that the linker drops every self-registered firmware and only the ones `src/main.cpp` registers by hand are reachable (the comment above `target_link_libraries(painlessmesh-simulator` in `CMakeLists.txt`).
- **painlessMesh is consumed from source**, not as a CMake subproject: `CMakeLists.txt` puts `${PAINLESSMESH_PATH}/src` and its test shims on the include path and compiles the few translation units v2.0.0 moved out of headers (the `_pm_src` loop). A pin move that adds a `.cpp` upstream shows up as an undefined reference at link time (`docs/V2_RELEASE_REVIEW.md` "Answer").
- **Firmware sends go through `FirmwareBase`**, not straight through `mesh_`, or the node's send metrics stay zero (`include/simulator/firmware/firmware_base.hpp`, the comment on sending; `DEVELOPMENT_STATUS.md` "Metrics").

## What works and what does not, today

`DEVELOPMENT_STATUS.md` is written against what the binary does: the event actions that have no runtime class yet (and the scenarios that need them), time scaling that does not fast-forward, metrics export that writes no file, the terminal UI. `scripts/ci_integration_check.sh` lists the same gaps as scenarios allowed to fail on one named action, so a gap closed there is a line removed from that list.
