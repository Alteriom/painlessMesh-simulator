# Development Status - painlessMesh Simulator

**Last Updated:** 2026-08-21
**Version:** 0.2.0-alpha
**painlessMesh pin:** `Feat/next-release` (v2.0.0 line)

This file was nine months stale and wrong in both directions -- it listed shipped
features as missing, and listed a metric as working that was structurally always
zero. It is now written against what the binary actually does, verified by
`scripts/ci_integration_check.sh`.

## Current status

The simulator builds against the painlessMesh v2.0.0 release branch, runs
scenario event timelines, and reports metrics that are real. It is usable as a
CI gate; it is not yet a substitute for hardware.

## What works

### Core
- **Configuration**: YAML scenario parsing and validation, with `--validate-only`.
- **Node management**: create, start, stop, crash and restart virtual nodes.
- **Mesh connectivity**: nodes connect through painlessMesh's own boost TCP stack.
- **Simulation loop**: configurable duration, progress reporting with a live
  running/total node count.
- **CLI**: duration, time-scale, log level, output dir, UI mode.

### Scenario events
All ten event classes are wired into the run loop through `EventFactory`:
`start_node`, `stop_node`, `crash_node`, `restart_node`, `connection_drop`,
`connection_restore`, `connection_degrade`, `partition_network` (alias
`network_partition`), `heal_partition` (alias `network_heal`) and
`inject_message`.

Until this release the event classes existed but nothing constructed them, so
every scenario ran as a static mesh and its `events:` timeline was silently
discarded.

### Firmware layer
- `FirmwareBase` interface with lifecycle hooks, plus `.ino` wrappers.
- Built-in firmware: `SimpleBroadcast`, `EchoServer`, `EchoClient`,
  `BasicInoFirmware`, `BridgeInoFirmware`, `library_validation`.
- Firmware self-registration via `REGISTER_FIRMWARE` now reaches the binary
  (the static archive is linked whole; before, only the two hand-registered in
  `main.cpp` were reachable and scenarios naming the others degraded silently).
- A scenario naming firmware that cannot load now **fails the run** instead of
  exiting 0 having done nothing.

### Metrics
- `messages_received` and `bytes_received` per node.
- `messages_sent` and `bytes_sent` per node. These were previously always zero:
  firmware sent straight through `mesh_`, bypassing the node, so nothing counted
  the send. Sends now route through `FirmwareBase` and are accounted.

### Network simulation
- Latency, packet loss and bandwidth limiting are implemented in
  `NetworkSimulator` and exercised by `test_network_simulator` /
  `test_network_integration`.

### Build and test
- Linux (GCC/Clang), macOS, Windows (vcpkg).
- 112 test cases / 1351 assertions, passing against painlessMesh
  `Feat/next-release`.
- CI gates lint, build, unit tests **and** behavioural integration checks.
- A nightly `upstream-drift` workflow builds against painlessMesh `main` and
  `Feat/next-release` independently of the submodule pin, and opens an issue when
  that breaks.

## What does not work yet

### Unimplemented event actions
Parsed and validated by the config loader, but with no runtime event class.
`EventFactory` reports them rather than dropping them, and a scenario using one
fails rather than running a quietly different test.

- `start_all_nodes` -- needed by `network_partition_test.yaml` and
  `split_brain_partition_test.yaml`
- `partial_heal` (heal a subset of partitions) -- needed by
  `issue_138_cascade_healing.yaml`; `NetworkHealEvent` currently clears every
  partition unconditionally
- `add_nodes`, `remove_node`, `break_link`, `restore_link`,
  `set_network_quality`

### Not modelled
- **painlessMesh v2.0.0 surfaces**: `ack.hpp`, `message_tracker.hpp` and
  `gateway.hpp` have no simulator coverage. The ACK API is the v2.0.0 headline
  and the most-churned area upstream.
- **Time scaling**: `--time-scale` only shortens the inter-update sleep, so it
  raises update frequency. The duration check still compares wall-clock elapsed
  time, so a 120s scenario still takes 120s no matter the scale -- scenarios
  cannot be fast-forwarded, which caps how much scenario coverage fits in CI.
- **Metrics export**: scenarios declare `metrics.output` / `export: [csv, json]`;
  no file is written.
- **Delivery-rate and reconvergence assertions** inside scenarios. The CI gate
  asserts these externally; scenarios cannot yet declare their own pass criteria.
- **Terminal UI**: `--ui terminal` is accepted; ncurses rendering is not built out.

## Known scale

`stress_test.yaml` runs 100 nodes. 500-node runs have not been characterised on
CI hardware -- treat the 50-500 node regression target as unproven until the
benchmark job reports numbers.
