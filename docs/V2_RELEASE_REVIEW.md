# Simulator review against painlessMesh v2.0.0

**Issue:** [#58](https://github.com/Alteriom/painlessMesh-simulator/issues/58)
**Reviewed against:** painlessMesh `Feat/next-release` @ `9a9ecab` (2026-08-21)
**Previous pin:** `4aca017` (2025-11-12) -- 680 commits behind

The goal set in #58 was to decide whether this simulator can carry automated
mesh regression testing for v2.0.0 alongside the ESP32 build farm: dedicated
hardware for HIL, simulation for meshes too large to own.

## Answer

Yes, and it is cheaper than the review expected -- but the reason it looked
risky was hiding a much larger problem than drift.

The 680-commit gap was **not** the obstacle. Bumping the submodule to
`Feat/next-release` produced exactly one failure, at link time:

```
undefined reference to `painlessmesh::tcp::shared_buffer'
undefined reference to `painlessmesh::tcp::lastScheduledDeletionTime'
```

v2.0.0 moved those two globals out of `connection.hpp` into a new
`src/connection.cpp`. The simulator consumes painlessMesh as a header-only CMake
`INTERFACE` target, so that translation unit was never compiled. Every other
file compiled clean: `mesh.hpp` (+1721 lines), `tcp.hpp` (a 5x rewrite),
`connection.hpp` and `callback.hpp` all changed without breaking a single
simulator call site. `VirtualNode` needed no rewrite. The whole port is ten
lines of CMake.

The real finding is what the port exposed. Findings 1-8 came out of the port
itself; 9-12 came out of review of the resulting PR and are the deeper half --
a fired event is not the same thing as an event that did something.

## Findings

### 1. The scenario event system was dead code (critical)

Nine event classes ship -- crash, start, stop, restart, connection
drop/restore/degrade, network partition and heal. `ConfigLoader` parsed and
validated every scenario's `events:` timeline into `EventConfig` records.

Nothing ever joined the two. `main.cpp` contained no reference to
`config.events` or `EventScheduler`, and nothing outside `src/scenario/events/`
ever constructed an event object. **Every scenario ran as a static mesh for its
duration and its timeline was silently discarded.** No partition was ever
simulated; no node ever crashed on schedule.

This is the single reason the simulator could not gate anything: the tests it
claimed to run were not running.

*Fixed* -- `EventFactory` (`src/scenario/event_factory.cpp`) translates
`EventConfig` into `Event` objects, `main.cpp` schedules them and drives
`processEvents()` from the run loop. Events that cannot be built are reported
and fail the run rather than being dropped.

Making the events fire turned out to be only half of it: findings 9 and 10
below cover the events that fired and still changed nothing.

### 2. The CI gate asserted nothing (critical)

The integration job ran `simple_mesh.yaml` and checked `$?` -- but *after* an
intervening `echo`, so the check was dead code and could never fail. It did not
matter: the scenario it ran referenced `examples/firmware/sensor_node`, a path
that is not a registered firmware name, so **all ten nodes failed to load
firmware, the run did nothing, and it exited 0.**

`integration-test` was also absent from the `ci-success` gate, so even an
honest failure there would not have failed CI. Every lint step is
`continue-on-error: true`, making the gate's dependency on `lint` vacuous. The
benchmark job requires `event_name == 'schedule'` while the schedule block was
commented out in `12ab2d7`, so it could never run.

*Fixed* -- `scripts/ci_integration_check.sh` asserts on behaviour: every
scenario validates, a broadcast scenario moves messages, a lifecycle scenario
fires its events and the running-node count drops when a node crashes, and a
scenario naming unknown firmware fails. `integration-test` is now in the
`ci-success` gate and the schedule trigger is restored.

The gate is not vacuous: run against the pre-fix binary it reports 7 failures
and exits 1.

### 3. Firmware self-registration never reached the binary (high)

`REGISTER_FIRMWARE()` declares a file-scope static in each firmware `.cpp`. A
linker pulls a static-archive member in only when something already references
it, and nothing referenced these -- so `EchoServer`, `EchoClient`,
`BasicInoFirmware` and `BridgeInoFirmware` were compiled and then discarded.
Only the two firmwares hand-registered in `main.cpp` were reachable, and the six
scenario entries naming `EchoClient`/`EchoServer` degraded silently.

*Fixed* -- `simulator_lib` is linked whole (`--whole-archive`, `-force_load`,
`/WHOLEARCHIVE`), so the macro does what it claims. All six firmwares now
register.

### 4. `messages_sent` was structurally always zero (high)

`VirtualNode::metrics_.messages_sent` was initialised to 0 and never
incremented. Firmware called `mesh_->sendBroadcast()` directly, bypassing the
node, so nothing observed a send. Every run reported `Total messages sent: 0`
even while reporting hundreds received -- and `DEVELOPMENT_STATUS.md` listed
metrics tracking as working.

Without a send count there is no delivery rate, and a delivery-rate assertion is
precisely what a mesh regression gate needs.

*Fixed* -- sends route through `FirmwareBase`, which notifies the owning node.
`firmware_broadcast.yaml` now reports sent=50 received=356.

### 5. Half the scenario library did not load (high)

10 of 21 shipped scenarios failed to parse or validate:

- 7 declared `mesh_prefix`/`mesh_password` flat on the node instead of under
  `config:`, which is what the loader reads.
- 5 used `network_partition` / `network_heal`, which the loader did not know
  (it knew `partition_network` / `heal_partition`).
- `firmware_echo.yaml` declared a star topology with no hub.
- 3 referenced `examples/firmware/{sensor_node,mqtt_bridge}`, which do not exist.

All five `issue_138_*` partition scenarios -- the exact large-mesh partition
coverage v2.0.0 needs -- were in that set.

*Fixed for 18 of 21.* The remaining three need event actions that have no
runtime implementation (below).

### 6. Failures were swallowed by design (high)

`NodeManager::createNode` carried the comment *"Continue without firmware rather
than failing node creation"*. Combined with #2, a completely failed run reported
success. That default is defensible for an interactive tool and fatal for a CI
gate.

*Fixed* -- failures are counted and `main` exits 1 when a scenario's configured
firmware does not load, naming the firmware that is available.

### 7. Progress output overstated the mesh (low)

The per-5s line printed `getNodeCount()` as "nodes running", so a scenario that
crashed half the mesh still printed the full count. *Fixed* -- reports
`running/total`.

### 8. Documentation rot (low)

`DEVELOPMENT_STATUS.md` was nine months stale and wrong in both directions: it
listed shipped features (firmware layer, latency, packet loss, partitions) as
not working, and listed metrics tracking as working when it was always zero.
*Rewritten.* `patches/painlessmesh_windows_fixes.patch` was referenced by no
CMake, Dockerfile, CI or script and its content is upstream at both pins.
*Deleted.*

### 9. Link events never reached the live mesh (critical)

Found on review of this PR by `chatgpt-codex-connector`, and confirmed by
measurement before it was fixed.

`ConnectionDropEvent`, `ConnectionDegradeEvent` and `NetworkPartitionEvent`
all wrote to `NetworkSimulator` -- a standalone latency/loss/queue model that
nothing on the delivery path consults. Firmware traffic goes over each
`VirtualNode`'s real painlessMesh connections. So the events logged success and
the mesh carried on unchanged.

Measured on six broadcasting nodes over 40 seconds, splitting 3/3 at t=15:

| Run | Total messages received |
|---|---|
| No partition | 596 |
| `network_partition` at t=15 | 596 |

Not "close to" -- identical. Every `issue_138_*` partition scenario was
therefore asserting nothing about partitions.

`NodeManager` now owns the edge list `establishConnectivity()` builds, and
`dropLink`/`restoreLink`/`partitionNetwork`/`healNetwork` close and re-open the
actual painlessMesh connections. Same scenario after the fix: **204 received
against 598 intact.** `connection_degrade` is the exception -- there is no
interposer on the loopback socket path, so it still configures the model only,
and now says so in its own output rather than reading as a delivered effect.

A partition also reports when the groups an author named are not subtrees of
the mesh, because cutting the cross edges then fragments the mesh further than
requested. That is a property of the topology, not a bug, but it silently
changed what the scenario tested.

### 10. A restarted node reported running without rejoining (critical)

Also raised by `chatgpt-codex-connector`; the underlying cause is deeper than
the report.

`NodeStartEvent` called `VirtualNode::start()`, which neither reconnected the
node nor could have: `establishConnectivity()` ran once, before the timeline,
and nothing recorded what it wired. Worse, `Mesh::stop()` closes every
connection, disables and nulls the ack tasks, clears the ack trackers and tears
down the `PackageHandler`'s scheduler tasks -- and nothing in `Mesh` re-arms
any of it. `start()` reused that instance.

The result was a node at full marks on the running count with its routing
machinery dead. Three broadcasting nodes, one stopped at t=10 and started at
t=20, 40-second run:

| Run | Restarted node received | Peers received | Total |
|---|---|---|---|
| Before | 9 | 25, 25 | 59 |
| Reconnect only | 20 | 25, 25 | 70 |
| Reconnect + mesh rebuild | 30 | 35, 34 | 99 |
| Control (never stopped) | 39 | 40, 40 | 119 |

`start()` now rebuilds the mesh when a previous `stop()`/`crash()` tore it
down, re-points the firmware at the new instance without re-running `setup()`
(re-adding a live `Task` corrupts the scheduler's list), and
`NodeManager::reconnectNode()` re-attaches the node to its recorded peers --
skipping any link a partition marked severed, so recovery cannot silently
bridge a split.

### 11. `inject_message` had no runtime class (high)

Parsed and validated since the config loader shipped, implemented nowhere, so
`issue_138_message_routing.yaml` and `star_topology.yaml` could validate but
not run. Implemented as `MessageInjectEvent` (`from`, optional `to`, `payload`;
`target` accepted as a sender alias, `to: "broadcast"`/`"all"` or an absent
`to` means a mesh-wide send). It is what lets a partition scenario ask its own
question -- does a message cross the split -- without depending on a firmware's
send cadence.

Both scenarios now run to completion. `issue_138_message_routing.yaml` schedules
18 of 18 events where it previously refused to start, and reports its partition
cutting 4 live links and its heal restoring 4. It also draws the new warning
that its two named groups leave five connected components rather than two --
worth knowing before reading anything into its results.

### 12. `time_scale` could never fast-forward anything (medium)

Raised by `chatgpt-codex-connector`, which proposed scaling the event clock.
That would make the run incoherent rather than faster: `millis()` in the Boost
build is `gettimeofday()`, so painlessMesh's TaskScheduler, ack timeouts and
connection timers are all on the wall clock. Scaling only the scenario timeline
would fire events against a mesh that had had proportionally less time to
converge, and scaling `duration` too would cut every run short.

There is no virtual clock to advance, so the honest fix is to stop implying
there is: the simulator now warns when `time_scale != 1.0` that only the poll
rate changes, and the four docs that advertised "run 5x faster" have been
corrected. A real fast-forward means virtualising `millis()` across painlessMesh
and the TaskScheduler, which is a different piece of work.

## Remaining gaps

These are real work, not oversights, and are deliberately left for follow-up
rather than faked with a misleading alias.

| Gap | Blocks | Notes |
|---|---|---|
| `start_all_nodes` event action | `network_partition_test.yaml`, `split_brain_partition_test.yaml` | No enum value or event class; mechanically similar to `NodeStartEvent` over all node ids |
| `partial_heal` event action | `issue_138_cascade_healing.yaml` | `NetworkHealEvent` clears every partition unconditionally; needs a subset argument |
| `add_nodes`, `remove_node`, `break_link`, `restore_link`, `set_network_quality` | -- | Parsed and validated, no runtime class. `EventFactory` reports them |
| Live-transport degradation | `connection_degrade` | Nodes talk over real loopback sockets with nothing interposed, so latency and loss cannot be applied to live traffic. The event configures the `NetworkSimulator` model and says so |
| v2.0.0 ACK surfaces | The release headline | `ack.hpp`, `message_tracker.hpp`, `gateway.hpp` have no coverage, and are the most-churned upstream area |
| Metrics export | Trend analysis | `metrics.output` and `export: [csv, json]` are inert; no file is written |
| Scenario-declared pass criteria | Scenario-level gating | Assertions live in the CI script, not in the scenario |
| Fast-forward | CI throughput | `--time-scale` only raises the poll rate. A real fast-forward needs a virtual `millis()` across painlessMesh and the TaskScheduler. Now warned about at startup and corrected in the docs rather than left implied |
| Scale above 100 nodes | The 50-500 node target | `stress_test.yaml` tops out at 100; 500 nodes is uncharacterised |
| Test suite needs the Docker image | Running tests anywhere else | 6 partition tests fail on a bare `ubuntu-24.04` runner with `bind: Permission denied` out of `boost::asio`, on an unprivileged port (16101). They pass in the project's Docker builder. Surfaced by the drift job's first non-Docker run; the job now uses the Docker image like every other build here, but the underlying portability limit is real |

## On the two overlapping "test without hardware" surfaces

`alteriom-esp32-farm`'s `hal/alteriom_hil/sim.py` is a hand-written Python fake
whose own docstring says it "does NOT exercise painlessMesh itself" and must be
manually kept aligned with the firmware. This simulator does exercise the real
library, on the real boost TCP stack, against the real v2.0.0 headers.

That is the argument for making this simulator the canonical no-hardware
backend and retiring `sim.py`'s duplicated model -- but it should be made on
evidence, not architecture. The falsification test proposed in #58 is still the
right next step and is now actually runnable: revert individual v2.0.0 ACK
fixes (`4b67625`, `76f2289`, `00546c8`) and measure how many the simulator
catches that painlessMesh's in-tree `test/catch` and `test/boost` suites miss.
That number is the business case. It was not measurable before, because the
event system that would have caught them never ran.

## Verification

Everything above was verified locally against `Feat/next-release` @ `9a9ecab`:

- Build: clean, GCC 12.2, C++14, Boost 1.74.
- Unit tests: 117 test cases, 1402 assertions, all passing (was 107/1333 before
  this work; 112/1351 before the topology and injection coverage).
- Scenarios: 20 of 23 validate; 3 skipped for unimplemented event actions.
- Behavioural gate: passes on the fixed build, fails with 7 problems on the
  build that preceded findings 1-8.
- Findings 9 and 10 were each measured before and after, on the numbers
  tabulated above, and each has a gate step of its own. Both steps were run
  against a binary built from the commit that preceded them: step 5 reports
  *"partitioned run received 598 vs 598 intact -- the split cost nothing"* and
  step 6 *"the restarted node re-established 0 mesh links"*, exit 1. They are
  not vacuous.

CI on PR #59 confirmed the Docker-based jobs: lint, both Docker builds, unit
tests and the new behavioural integration gate all pass on GitHub runners.

The `upstream-drift` job's first run failed for a reason unrelated to drift --
it built on a bare runner, where 6 pre-existing partition tests fail on
`boost::asio` bind. It now builds in the project's Docker image like every other
job here. Its issue-filing step only runs on the nightly schedule and remains
unexercised.
