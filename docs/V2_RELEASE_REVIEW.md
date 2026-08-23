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
itself; 9-16 came out of review of the resulting PR and are the deeper half --
a fired event is not the same thing as an event that did something, a node the
simulator calls stopped is not necessarily a node that stopped, a topology the
scenario declares was not the topology it ran on, a link the simulator says it
restored was not yet carrying traffic when the next event needed it, a probe
meant to measure the mesh was quietly reshaping it, a run that dropped an event
still called itself a success, the same link event meant two different things
depending on how it was spelled, a declared-but-empty topology quietly ran a
random mesh instead, a "random" seed produced the same graph every time, a
partition heal undid a deliberately dropped link, and a heal that could not
reach a node threw the repair away.

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

### 13. Stopped nodes went on transmitting (high)

Raised by `chatgpt-codex-connector` on the second review pass, and confirmed
before it was fixed.

`NodeManager` owns exactly one `Scheduler` and `updateAll()` executes it for
the whole fleet. `VirtualNode::update()` does return early for a node that is
not running -- but that only skips `mesh_->update()` and the firmware's
`loop()`. A firmware's *scheduler task* is not on that path. So from `stop()`
to `start()` the task kept firing: `SimpleBroadcastFirmware::broadcastMessage()`
sent through the stopped mesh's torn-down routing state, and
`FirmwareBase::sendBroadcast()` ran the send-accounting hook added in finding 4,
booking transmissions that never left the node.

Measured on `restart_rejoin_test.yaml`, one node down from t=10 to t=20 with a
2000 ms broadcast interval:

| | Broadcasts logged during downtime | Total sends reported |
|---|---|---|
| Before | 5 -- one per interval, exactly | 63 |
| After | 0 | 58 |

The five-send difference is the phantom traffic, removed from the metric.

`FirmwareBase` now owns task registration. `registerTask()` adds the task to
the scheduler, enables it and records it; `suspend()` disables every registered
task and blocks the `sendBroadcast()`/`sendSingle()` helpers; `resume()`
restores each task to the enabled state it held at suspend time rather than
enabling all of them, so `LibraryValidationFirmware` -- which disables its own
tasks once its tests finish -- does not find them running again after a
restart. `VirtualNode::stop()` and `crash()` suspend; `start()` resumes after
`setupFirmware()`. The four firmwares that registered tasks directly were
converted, and the authoring guides now document `registerTask()` as the way
in: a task added straight to the scheduler is still untracked, and would
reintroduce this.

Gate step 7 covers both halves -- no sends during downtime, and at least one
after the restart, so silencing the firmware permanently would not pass.

### 14. The declared topology was never applied (critical)

Raised by `chatgpt-codex-connector` on the third review pass. Confirmed, and
the fix it proposed would have made things worse.

`ConfigLoader` parsed and validated `topology:` from the first release --
type, hub, density, custom connections, the lot -- and nothing ever read it.
`NodeManager::establishConnectivity()` took no arguments and always built a
random spanning tree. 21 of the 23 shipped scenarios declare a topology, so
nearly every run was against a graph nobody asked for. Star scenarios had no
hub; custom scenarios had none of their drawn links.

Harmless while events were inert. Once findings 1 and 9 made link events live,
the events addressed edges that did not exist. `connection_events_test.yaml`
declares a full mesh and drops `node-1 <-> node-2` at t=20:

```
[EVENT] Connection dropped: 1934219892 <-> 572338315 (0 live endpoint(s) closed)
```

The obvious fix -- wire what the YAML declares -- was measured before it was
trusted, and it is catastrophic. Four nodes running SimpleBroadcast, 20s:

| Topology | Links wired | Live links | Sent | Received |
|---|---|---|---|---|
| star (3 links, a tree) | 3 | 3 | 40 | 119 |
| custom line (3 links, a tree) | 3 | 3 | 40 | 118 |
| random d=0.5 (3 links, a tree) | 3 | 3 | 40 | 118 |
| ring (4 links, one cycle) | 4 | **0** | **0** | **0** |
| mesh (6 links) | 6 | **0** | **0** | **0** |
| random d=1.0 (6 links) | 6 | **0** | **0** | **0** |

Wiring a cyclic graph in one pass does not give painlessMesh a richer mesh; it
gives it none at all. The cause is the burst, not the cycle: closing a ring
*at runtime* is handled gracefully -- the extra connection is rejected and the
tree survives (3 live links, 60 sent / 179 received) -- and with a settling
pass between connects, the same 6-link mesh converges to 3 live links and
healthy traffic. Overlapping handshakes are what the library cannot take.

So the simulator now plans the declared graph, reduces it to a spanning forest
-- which is what painlessMesh converges to anyway -- and says so:

```
[WARN] topology: mesh declares 6 link(s); painlessMesh holds a spanning tree,
       so 3 surplus link(s) were not wired
[INFO] Mesh connectivity established (topology=mesh, 3 of 3 planned link(s)
       wired, 6 declared)
```

Reduction order is deterministic and prefers the pairs the scenario's own
events name, so a declared drop has a live link to cut. The same drop now
reports:

```
[EVENT] Connection dropped: 1934219892 <-> 572338315 (2 live endpoint(s) closed)
```

`NodeManager::settleLink()` pumps the mesh between connects so the burst
cannot recur if a caller passes a cyclic list anyway. Planning lives in
`planTopology()`, a pure function with its own tests, and gate step 8 asserts
the declared type is wired and the declared drop cuts.

What a topology declaration now controls is *which* tree a run uses -- a real
fidelity gain over a random one -- not how densely connected it is. Density
above a tree, a closed ring and a full mesh are declared, reported and
reduced; see the gaps table.

### 15. Failed sends were counted as delivered (high)

`Mesh::sendBroadcast()` returns false when `router::broadcast` reached nobody,
and `sendSingle()` false when there is no route. `FirmwareBase`'s helpers
ignored both and ran the send-accounting hook regardless, so an isolated node
-- or one on the wrong side of a partition -- reported traffic it never got
rid of. `VirtualNode::injectMessage()` already had this right; the firmware
path did not.

Both helpers now return the mesh's verdict and only account for a true.
`SimpleBroadcastFirmware` counts and logs only accepted broadcasts and tracks
rejections separately (warning once per node rather than per message);
`EchoClient` does not count a request the mesh refused; `EchoServer` does not
count an echo with no route back.

Two existing tests asserted the old behaviour -- an EchoServer with no peers
"echoing" to node 9999, and an echo counted before the link it needed had come
up -- and were corrected rather than pinned.

### 16. Equal-time events ran in arbitrary order (medium)

`EventScheduler`'s comparator documented FIFO for events sharing a timestamp
and did not implement it: `std::priority_queue` is a binary heap, which is not
stable, and the comparator looked only at `scheduledTime`. Six events queued
at the same second came out `a c f e b d`. A same-second `heal` could run
after the `inject` that needed it, or a `start` before the `stop` above it.

Events are now stamped with a monotonic sequence at schedule time and the
comparator breaks ties on it. Two tests cover it, and both fail on the old
comparator with the scramble above.

### 17. A restored link was not live when the next event ran (medium)

Raised by `chatgpt-codex-connector` on the fourth review pass. Confirmed.

`NodeManager::connectNodes()` calls `MeshTest::connect()`, which only *starts*
an asynchronous TCP connect. `EventScheduler::processEvents()` runs every event
due at a timestamp back to back with no pump between them. So a `heal_partition`
followed by an `inject_message` at the same second -- now that finding 16
guarantees they run in that order -- healed the link and then refused the
injection, because the handshake had not completed. On `heal_then_inject_test`
(a 4-node line, split in half, healed at t=20, `n1 -> n4` injected at t=20):

```
[EVENT] t=20s: Heal network partitions
[EVENT] Network partitions healed (1 mesh link(s) restored)
[EVENT] t=20s: Inject message: 158454472 -> 1772092626
[EVENT] Message injected from 158454472 to 1772092626 -- REFUSED
```

The link only showed live at the next progress tick, five seconds later.

The fix is one line, in the right place. `connectNodes()` now calls
`settleLink()` before returning, so *every* caller -- startup wiring, heal,
restore, a node rejoining -- hands back a link that is actually carrying
traffic, and no caller has to remember to pump. (The startup path called
`settleLink()` itself before; that call moved down into `connectNodes()` rather
than being duplicated.) The injection above is now delivered, and gate step 9
asserts it: a same-second injection after a heal must not be refused.

Fixing this surfaced a second-order bug in `reconnectNode()`. It skipped any
peer that "already" reported connected -- but a peer can still be holding a
`Connection` object for the socket the restarted node closed on its way down,
learning otherwise only on its next poll. Judging on that stale peer view left
the node permanently detached. It now judges on the restarted node's own view
alone, which is authoritative.

### 18. `.ino` wrapper firmware bypassed send accounting (medium)

`BasicInoFirmware::sendMessage()` calls `mesh->sendBroadcast()` directly -- a
faithful transcription of `basic.ino` -- and so never reached the accounting
hook `VirtualNode` installs on the base helper. A three-node run of it reported
`Total messages sent: 0` while its peers received 44: traffic moving, invisible
to every metric and to the gate. This is the same class as finding 15, in the
one firmware the earlier fix did not touch.

`sendMessage()` now sends through `FirmwareBase::sendBroadcast()` and honours
the result, so the send is counted (and a refused broadcast is not). The two
`LibraryValidationFirmware` sites that called `recordMessage(true)`
unconditionally after a send were fixed the same way -- with care, because
`recordMessage`'s argument is a *direction*, not a verdict, so a false there
would have incremented the receive counter.

The unit suite cannot reach `BasicInoFirmware`: its `REGISTER_FIRMWARE` static
registration does not link into the test binary and the class has no header to
re-register from. So the coverage is a scenario, `ino_firmware_test.yaml`,
folded into gate step 2 -- which now runs both a SimpleBroadcast and an .ino
scenario and requires each to report a non-zero, peer-confirmed send count.

### 19. Message injections biased the topology they were meant to probe (critical)

Raised by `chatgpt-codex-connector` on the fifth review pass, against finding
14's own fix. Correct, and a sharp catch.

The topology planner keeps pairs a scenario's events name when it reduces a
declared graph to a spanning tree, so a `connection_drop` has a live link to
cut. The first cut of that logic treated *every* event with `from`/`to` fields
as a preferred link -- including `inject_message`. An injection's whole purpose
in a routing scenario is to traverse the mesh, often multi-hop.
`issue_138_message_routing.yaml` declares a mesh and injects `node-1 -> node-6`
across a future partition boundary precisely to exercise multi-hop delivery;
preferring that pair as a direct edge wired the probe's endpoints as neighbours
and hid the routing failure it exists to catch. The experiment changed based on
its own measurement.

`eventPairs()` now filters on the action: only the events that act on a
specific physical link -- `connection_drop`, `connection_restore`,
`connection_degrade`, `break_link`, `restore_link` -- bias the tree.
`inject_message` and everything else do not. Two unit tests pin it: an
injection leaves the plan byte-identical to the unbiased baseline, and a drop on
the same pair forces it into the tree while the injection does not.

(The obvious negative assertion -- that the injection's pair is *absent* from
the plan -- would have been seed-dependent, since a random tree may include it
anyway. The test asserts "no influence" instead, which holds for every seed.)

### 20. `settleLink()` reported success even on timeout (medium)

Raised by `chatgpt-codex-connector` on the sixth review pass, against finding
17's fix. Correct.

`settleLink()` (finding 17) pumped the mesh until both endpoints reported the
connection, then returned -- but it returned `void`, and `connectNodes()`
returned `true` unconditionally after calling it. So a handshake that did not
complete within the 100 ms budget was still counted: startup reported the link
"wired", and heal/restore/reconnect reported it "re-established", while
dependent traffic could still be refused -- the exact false assurance
settlement exists to remove.

`settleLink()` now returns whether both endpoints actually came up, and
`connectNodes()` returns that. On loopback a real link settles in a millisecond
or two, so every genuine connection still returns true; only a link that never
comes live returns false, and its caller's count reflects that. The topology
edge stays recorded either way -- the scenario still intends it, and a later
heal may bring it up -- so the intent map is unchanged; only the success count
stops lying about the present. A unit test pins it: a never-connected pair
settles false, a connected pair settles true.

### 21. A thrown scheduled event did not fail the run (medium)

`EventScheduler::processEvents()` caught an exception from an event, logged
`[ERROR] Event execution failed`, dropped the event and carried on -- correct
for keeping the rest of the timeline moving, but it returned only the
*executed* count, and the run's main loop ignored even that. So a timeline with
a failed event still printed `Simulation completed successfully` and exited 0.
A gate or experiment would accept a run whose timeline did not execute.

`processEvents()` now counts throws in `getFailedCount()` (accumulated across
calls, cleared by `clear()`), and the run's exit path fails with a non-zero
status and a clear message when that count is non-zero. Unit tests cover the
counting: a `FailingEvent` yields `getFailedCount() == 1`, a clean run yields
0, and the count accumulates and clears.

No *shipped* scenario can currently reach this: every event that could throw
does so on an unknown node, and those are caught at schedule time -- by config
validation for `target`, and by the event factory for `from`/`to` -- both of
which already fail the run. So this hardens the execution path against future
events (and the "cannot rebuild its transport" class the reviewer named) rather
than fixing an observable failure today, which is why its coverage is the
scheduler unit tests, not a gate scenario that cannot legitimately be built.

### 22. The `targets:` link syntax lost its preferred edge (critical)

Raised by `chatgpt-codex-connector` on the seventh review pass, against finding
14/19's logic. Correct.

`EventFactory::resolveLink()` accepts a connection event's endpoints in two
spellings: `targets: [a, b]` or `from`/`to`. Finding 19's `eventPairs()` read
only `from`/`to`, so a `connection_drop` declared with the `targets` syntax was
not treated as a preferred edge. For a cyclic declaration like `mesh`, the
spanning-tree reduction could then discard that exact pair, and the scheduled
drop would later act on no live link -- the "0 live endpoint(s) closed" silence
finding 14 exists to remove, reintroduced through the other spelling.

`eventPairs()` now mirrors `resolveLink()`: it reads `targets[0..1]` when
present, else `from`/`to`. A unit test pins the equivalence -- a drop declared
via `targets` plans the identical graph to the same drop declared via
`from`/`to`. (Asserting the pair is merely present would be seed-dependent; the
equivalence is not, and it fails the moment the `targets` spelling is ignored.)

### 23. A restore could fabricate an undeclared route (medium)

`connection_restore` calls `NodeManager::restoreLink()`, which called
`connectNodes()` -- and `connectNodes()` records a fresh `topology_` edge. So a
restore naming a pair the declared topology never had (a drop/restore pair
omitted from a `custom` topology, say) did not restore anything; it *added* a
route, changing later partition, heal and reconnect behaviour. A restore is
meant to bring back a link that existed, not invent one.

`restoreLink()` now refuses a pair absent from `topology_`, the same guard
`healNetwork()` already applies to its edges. A unit test pins it, and is
careful to be a real test of the guard rather than of painlessMesh's own
redundant-link dedup: the refused pair's far node is left *isolated* in its own
component, so an unguarded restore genuinely would connect it -- the guard is
what stops it, and removing the guard fails the test.

### 24. An empty declared topology fell back to a random mesh (medium)

Raised by `chatgpt-codex-connector` on the eighth review pass. Correct.

`planTopology()` skips invalid custom links (self-links, unknown nodes) and can
return an empty plan with a warning. The run's connectivity branch then read
"empty plan" as "no topology given" and built the historical random tree. So a
custom topology of only self-links -- `connections: [["n1", "n1"]]`, which
validation accepted -- ran silently with random links the author never wrote,
instead of being rejected.

Fixed on both sides. Validation now rejects a self-link (`"Connection links a
node to itself"`), so the common case fails at load with a clear message rather
than running anything. And the random-tree fallback is now gated on whether a
`topology:` block was *declared*, not on whether the plan came out empty: a
declared topology that plans no links is an error (or, for a single node,
simply nothing to wire), never a licence to substitute a random mesh. The two
guards are independent -- validation catches the concrete case early, the
`declared` gate catches any other way a declared topology could plan empty.

A unit test pins the validation rejection; it fails when the self-link check is
removed.

### 25. A zero seed wired the same graph every run (medium)

Raised by `chatgpt-codex-connector` on the ninth review pass. Correct -- a
contract violation the topology work introduced.

`SimulationConfig` documents `seed = 0` as *random*. Finding 14's planner
substituted a fixed `kDefaultSeed` for a zero seed, and `NetworkSimulator` was
constructed with the literal zero. Now that the plan wires the actual mesh, a
scenario that omits `simulation.seed` got the identical spanning tree on every
run instead of the documented variation.

The run's entry point now resolves the seed once: a zero `simulation.seed`
draws a real seed from `std::random_device`, and that resolved value feeds both
`planTopology()` and `NetworkSimulator`. The drawn seed is logged with the exact
line needed to pin it -- *"drew random seed N (set simulation.seed: N to
reproduce)"* -- so a run that surfaces something interesting is reproducible
after the fact. `planTopology()` stays a pure, total function: it still maps a
zero argument to `kDefaultSeed` deterministically, but the entry point no longer
passes zero, so that fallback now only serves a unit test that calls it
directly.

The two shipped scenarios the behavioural gate reads that had no seed --
`node_lifecycle_test` and `connection_events_test` -- were pinned to an explicit
seed, so CI stays reproducible while seedless *user* scenarios get the random
behaviour the contract promises.

### 26. A partition heal restored explicitly dropped links (medium)

Raised by `chatgpt-codex-connector` on the tenth review pass. Correct.

`connection_drop` and `partitionNetwork()` both mark links in `severed_` (both
go through `dropLink()`). `healNetwork()` copied and cleared *all* of `severed_`
and reconnected the lot -- so a heal restored a link a `connection_drop` had
deliberately taken down, ahead of its own `connection_restore`, conflating a
persistent link failure with a temporary partition.

`partitionNetwork()` now records its cuts in a separate `partition_cuts_` set,
and `healNetwork()` restores only those, erasing each from `severed_` as it
goes. Explicit-drop links stay in `severed_` -- still blocking `reconnectNode()`
from bridging them, still waiting for their own restore. `restoreLink()` and
`removeNode()` keep `partition_cuts_` consistent alongside `severed_`.

A unit test on a 4-node line drops one edge explicitly and partitions another:
the heal reconnects the partitioned edge and leaves the dropped one down and
severed, and the drop still restores on its own afterward. Removing the
separation restores the dropped link in the heal and fails the test.

### 27. An explicit drop overlapping a partition still healed (medium)

Raised by `chatgpt-codex-connector` on the eleventh review pass, against finding
26's fix. Correct -- the fix handled two *different* edges but not the same edge
cut two ways.

Finding 26 recorded every partition cross-pair in `partition_cuts_`
unconditionally. So when an edge was both explicitly dropped *and* crossed a
partition boundary, it landed in `partition_cuts_` anyway, and the heal restored
it -- the persistent explicit failure undone after all.

The severance model now records *why* a link is down: `explicit_drops_` (from
`connection_drop`, persistent until `connection_restore`) and `partition_cuts_`
(from `partitionNetwork()`, healed by `healNetwork()`). An explicit drop clears
any partition marker on its edge; a partition marks an edge cut only when it is
not already an explicit drop. Both orders resolve to "explicit wins": the edge
stays down through a heal and comes back only on its own restore.
`isLinkSevered()` is the union of the two. A unit test covers both orderings.

### 28. A heal discarded a cut it could not restore (medium)

Finding 26's `healNetwork()` cleared `partition_cuts_` up front and erased each
link from the severed set before trying to reconnect. If the reconnect could not
happen -- a node still down, or (after finding 20) a handshake that did not
settle -- the cut's state was already gone: no retry, and the run still exited 0
with the mesh partitioned.

`healNetwork()` no longer discards a cut it could not immediately rebuild. The
exact retention rule was refined by finding 31: a cut whose endpoint is *down*
is released (the partition has ended; `reconnectNode()` restores it when the
node returns), while a cut where both endpoints are *up* but the handshake did
not settle is retained for a later heal to retry.

### 29. A restore bridged an edge an active partition still cut (medium)

Raised by `chatgpt-codex-connector` on the twelfth review pass, against finding
27's fix -- and it showed that "explicit wins" was the wrong model.

Finding 27 made an explicit drop *replace* a partition marker on the same edge.
So `connection_restore` on that edge, while the partition was still unhealed,
cleared the only remaining reason and reconnected -- bridging the two partition
groups ahead of `heal_partition`. The same happened for an edge cut only by a
partition if a `connection_restore` named it.

The model is now: **a link can be down for several reasons at once, and comes
back only when every reason clears.** `explicit_drops_` and `partition_cuts_`
coexist per edge. `connection_drop` adds the explicit reason and leaves any
partition reason; `partitionNetwork()` adds the partition reason and leaves any
explicit one. `connection_restore` clears only the explicit reason and defers if
a partition still cuts the edge; `healNetwork()` clears only the partition
reason and leaves an edge down if it is also explicitly dropped. `isLinkSevered`
is the union. A reconnect happens only at the moment the last reason is removed.

This subsumes findings 26-28 into one coherent rule instead of a special case
per pair of events. Unit tests cover a restore deferring to a heal, both for a
purely partitioned edge and for one that is both dropped and partitioned; the
overlapping-edge and pending-cut tests from 27/28 still hold.

### 30. The partition gate step ignored the simulator's exit code (medium)

Raised by `chatgpt-codex-connector` on the thirteenth review pass. Correct.

Gate step 5 ran three simulator invocations (control, split, heal) and discarded
their exit statuses; the script does not use `set -e`. Since finding 21 made the
simulator print its totals and *then* exit non-zero on a failed scheduled event,
a partition or heal run that exited 1 could still supply receive and link counts
that satisfied the step's assertions -- a green gate over a broken run. The other
steps already capture and reject a non-zero code; step 5 was the one that did
not.

Step 5 now captures each probe's exit code and fails if any is non-zero, before
reading their counts. Verified the whole gate still passes on the fixed build.

### 31. A node starting after a heal stayed detached (medium)

Raised by `chatgpt-codex-connector` on the fourteenth review pass, against
finding 28's retention. Correct -- and it exposed that "retain every unhealed
cut" was too broad.

Finding 28 held a partition cut in `partition_cuts_` when a node was down at
heal time. But `reconnectNode()` -- called when that node later `start`s --
skips any edge still marked severed. So `partition -> stop node -> heal -> start
node` left the node permanently detached: the heal could not reconnect it (down),
and the start could not either (still severed), short of a second heal the
scenario had no reason to include.

The rule is now split by *why* the heal could not act. `heal_partition` ends the
partition regardless, so a cut whose endpoint is down is *released* -- the edge
is no longer severed, and `reconnectNode()` re-establishes it when the node
returns. Only a genuine transient -- both endpoints up but the handshake did not
settle -- is retained for a later heal, where `reconnectNode()` cannot help
because nothing is restarting. Unit tests cover both: a node that starts after a
heal rejoins with no second heal, and a both-up heal still completes in place.

### 32. A partition that omitted a node reported a split it did not make (medium)

`partitionNetwork()` cuts only pairs that cross a group boundary. A node in no
group is invisible to that loop, so it keeps bridging: `A--X--B` with
`groups: [[A], [B]]` cuts the (unwired) A-B pair, reports a split, and X carries
the traffic across regardless. The event claimed a partition it did not make.

Validation now requires a `network_partition`'s groups to cover every scenario
node exactly once -- each node in some group, none named twice, none unknown.
The omitted-bridge case fails at load with `"Partition omits node 'X', which
would keep bridging the split"`. All nine shipped partition scenarios already
satisfy this; a unit test covers omission, double-listing, and a valid full
partition.

### 33. A random topology dropped an edge it never wired (medium)

Raised by `chatgpt-codex-connector` on the fifteenth review pass, against the
preferred-edge logic. Correct.

`spanningSubset()` prefers event-named pairs, but it can only *reorder* edges
already in the candidate set -- it cannot add one. For a `random` topology the
candidate set is drawn at random, so a `connection_drop` pair may simply be
absent, and the drop then ran against no live link ("0 live endpoint(s)
closed") despite the planner's stated preferred-edge guarantee.

`planTopology()` now merges the event-named pairs (valid node pairs) into the
candidate graph *before* reduction, so `spanningSubset()` has them to prefer.
The guarantee is now real for every topology type, not only the ones whose
declared set happens to contain the pair. A unit test drops `n1<->n5` in a
zero-density random topology across six seeds -- the pair is in the plan every
time, and the graph stays connected. `plan.declared` still counts only the
topology's own links, so the surplus-links warning is unchanged.

### 34. An empty partition group reported a split it did not make (medium)

Finding 32's coverage check accepts `groups: [[n1, n2], []]` -- every node
appears exactly once, and an empty group adds nobody. But `partitionNetwork()`
cuts only cross-group pairs, and an empty side has no members to cross to, so no
edge is cut while the event still reports a two-way partition.

Validation now rejects any empty partition group. A unit test covers it.

### 35. The candidate merge silently reshaped explicit topologies (critical)

Raised by `chatgpt-codex-connector` on the sixteenth review pass, against
finding 33. Correct -- finding 33's merge over-reached.

Finding 33 added an event-named pair to the candidate graph for *every* topology
type. For `random` that is right (the draw is arbitrary). For `star`, `ring` and
`custom` the declared graph is explicit and intentional: inserting an event's
pair there changed it -- a leaf-to-leaf `connection_drop` in a star was added
and preferred, displacing a hub edge -- and it slipped an undeclared edge into
`topology_` at startup, so `restoreLink()`'s undeclared-edge guard (finding 23)
never saw it.

The merge is now restricted to `random`. For the explicit modes the declared
graph stands as written; an event naming a non-edge of one is a configuration
error, left to fail at runtime rather than papered over by reshaping the graph.
A unit test asserts a star's plan is byte-identical with and without a
leaf-to-leaf drop event, and that the leaf-leaf pair is not wired.

### 36. A cyclic set of event links ran an event as a silent no-op (medium)

When event-named links form a cycle -- three `connection_drop`s naming all
three edges of a three-node mesh -- `spanningSubset()` can keep only two (a
tree) and dropped the third with just a warning. The run then scheduled all
three events, and the third executed against a link that was never wired: zero
endpoints closed, a timeline the author did not ask for.

The planner now records every event-named link it could not wire in
`unwireable_preferred`, and the entry point fails the run when that list is
non-empty rather than executing a hollow event. A unit test confirms the third
drop is flagged, and an end-to-end run of three drops on a triangle exits
non-zero with *"1 event-named link(s) cannot be wired..."*.

### 37. Link events on undeclared explicit edges still no-opped (medium)

Raised by `chatgpt-codex-connector` on the seventeenth review pass, against
finding 35. Correct -- finding 35's comment promised a runtime failure that only
`connection_restore` actually enforced.

Finding 35 stopped *adding* an event's pair to an explicit topology, but a
`connection_drop` or `connection_degrade` on a pair that is not one of that
topology's edges was still scheduled: `dropLink()` recorded the phantom pair,
closed zero endpoints, and the run exited 0. Only `connection_restore` had the
membership guard (finding 23).

`planTopology()` now, for the explicit modes, flags any event-named pair that is
not a declared edge as `unwireable_preferred` -- the same fatal list finding 36
uses for cyclic links. The entry point fails the run with a message naming
either cause. `mesh` declares every pair, so this only bites `star`, `ring` and
`custom`; none of the shipped scenarios with link events use those. A unit test
asserts a star leaf-to-leaf drop is flagged, and end-to-end that scenario exits
non-zero.

### 38. `graceful: false` did a graceful stop (medium)

`NodeStopEvent::execute()` called `VirtualNode::stop()` unconditionally and only
printed `(forced)`. So a `stop_node` with `graceful: false` performed the same
clean teardown as `graceful: true` -- it never exercised the crash path, never
incremented `crash_count`, and reported a forced outage that did not happen.

A forced stop now calls `crash()` (the ungraceful path), which drops the node
without a clean shutdown and counts the crash; a graceful stop still calls
`stop()`. Unit tests assert `crash_count` moves for a forced stop and stays flat
for a graceful one.

### 39. A failed restore was logged, not propagated (medium)

Raised by `chatgpt-codex-connector` on the eighteenth review pass, against the
settlement-return work. Correct.

`ConnectionRestoreEvent` treated `restoreLink()` returning false as a benign
"no mesh link to re-establish" and completed successfully -- so a genuine
failure (both endpoints up, handshake past the settle deadline) left the link
down while `getFailedCount()` stayed zero and the run exited 0.

`restoreLink()` now returns a `RestoreOutcome` -- `Reestablished`, `NothingToDo`
(already live, deferred to a heal, a node down, or an undeclared edge), or
`Failed` (both up but the handshake did not settle). `ConnectionRestoreEvent`
throws on `Failed` -- which `processEvents()` counts and the exit path turns
into a non-zero status (finding 21) -- and logs the `NothingToDo` reasons
without failing, so a legitimate deferral is not mistaken for an error. Like
findings 20-21 the `Failed` branch cannot be produced on loopback, so it is
covered by the enum plumbing and a success-path unit test rather than a gate
scenario.

### 40. `--validate-only` skipped the feasibility checks (medium)

`--validate-only` returned right after config validation, before the topology
plan and event scheduling ran. So a scenario an ordinary run rejects -- cyclic
event links, a link event on a non-edge of an explicit topology (findings
36/37), or an event whose action has no runtime class -- reported *"Validation
successful"*, and the CI validation sweep (gate step 1) could not catch it.

The feasibility checks -- `planTopology()`'s `unwireable_preferred`, and
`EventFactory::scheduleAll()`'s skipped list -- now run as part of validation,
before the `--validate-only` return. Both are pure over the config and need no
nodes. A new gate step (1b) builds a cyclic-drop scenario and asserts
`--validate-only` rejects it. Verified that both a cyclic-links and a star
leaf-drop scenario now exit non-zero under `--validate-only`.

### 41. A self-referential link event ran as a no-op (medium)

Raised by `chatgpt-codex-connector` on the nineteenth review pass. Correct.

`eventPairs()` silently skips a link event whose endpoints resolve to the same
node (`from == to`), but `EventFactory::resolveLink()` still schedules it. So a
`connection_drop` from `n1` to `n1` passed `--validate-only`, recorded a phantom
self-drop, closed zero endpoints, and exited 0.

Validation now rejects a link event (`connection_drop`/`restore`/`degrade`,
`break_link`/`restore_link`) whose two endpoints are the same node -- reading
the pair from `targets` or `from`/`to`, as `resolveLink()` does. A unit test and
an end-to-end `--validate-only` run confirm the rejection.

### 42. A failed node reconnection was reported as a clean rejoin (medium)

`reconnectNode()` returned a single count, collapsing a genuine settlement
failure (an eligible peer whose handshake did not settle) into the same zero it
uses for the legitimate skips -- severed, peer down, already live. So
`start_node`/`restart_node` reported a clean rejoin and the run exited 0 with the
node still detached.

`reconnectNode()` now returns `{reconnected, failed}`, counting a `connectNodes()`
false for an eligible peer as a failure. `NodeStartEvent` and `NodeRestartEvent`
throw when `failed > 0` -- counted by `processEvents()`, non-zero exit via
finding 21 -- exactly as `ConnectionRestoreEvent` now does for a failed restore.
Like findings 20-21 and 39 the failure branch cannot occur on loopback, so it is
covered by the struct plumbing and a clean-rejoin unit test (`failed == 0`).

### 43. A restart's configured delay was ignored (medium)

Raised by `chatgpt-codex-connector` on the twentieth review pass. Correct.

`restart_node` accepts a `delay`, but the factory built an atomic
`NodeRestartEvent` (stop immediately followed by start) and discarded it. A
scenario asking for an 8-second outage got none, and the run reported success
having simulated no downtime.

`scheduleAll()` now models a `restart_node` with `delay > 0` as two lifecycle
events -- a `NodeStopEvent` at the event's time and a `NodeStartEvent` at
`time + delay` -- so the outage actually happens. An undelayed restart stays
atomic. Verified end-to-end: a restart at t=10 with delay 8 stops the node at
t=10 and starts it at t=18. Unit tests cover both the split and the atomic case.

### 44. Out-of-range degrade packet loss failed mid-run (medium)

`connection_degrade`'s `packet_loss` reaches `NetworkSimulator::setPacketLoss()`,
which throws on a value outside `[0, 1]`. Neither config validation nor the
factory checked it, so `--validate-only` passed and an ordinary run failed only
once the event fired, part way through the timeline.

Validation now rejects a `connection_degrade` whose `packet_loss` is outside
`[0, 1]`, so it fails at load with a clear message. A unit test and a
`--validate-only` run confirm the rejection.

### 45. A delayed restart could finish after the run ended (medium)

Raised by `chatgpt-codex-connector` on the twenty-first review pass, against
finding 43's fix. Correct.

Finding 43 schedules a delayed restart's start at `time + delay`. Validation
bounded only the original event time, so a `restart_node` at t=15 with `delay:
10` in a `duration: 20` run scheduled the stop within the run and the start at
t=25 -- which never fired. The node stayed stopped while the run reported
success, because the exit path checks failed events, not pending ones.

Validation now bounds the computed start time -- `time + delay` -- against the
duration, with overflow-safe addition (both are `uint32`). The example fails at
load: *"Restart start (time 15 + delay 10 = 25s) exceeds simulation duration
20s"*. A unit test and a `--validate-only` run confirm it.

### 46. Feasibility failures used the wrong exit code (medium)

Raised by `chatgpt-codex-connector` on the twenty-second review pass. Correct.

`README.md` reserves exit `2` for configuration-validation failures and `1` for
runtime/I/O errors. The feasibility checks (findings 36/37/40) and the
declared-but-empty topology check (finding 24) returned `1`, so a script that
distinguishes an invalid scenario from a runtime error got the wrong status for
exactly these config problems.

The five config-validation exit paths -- unwireable event links (pre-validate
and run), unschedulable events (pre-validate and run), and a declared topology
that plans no links -- now return `2`. The genuine runtime failure (`wired <
planned`, a settle failure) stays `1`. Verified: a cyclic-drop and a star
leaf-drop scenario each exit `2` under `--validate-only`.

### 47. The gate skipped allowlisted scenarios on any failure (medium)

Gate step 1's validation sweep treated *any* failure from a `KNOWN_UNSUPPORTED`
scenario as an expected skip. So if one gained malformed YAML, an unknown node,
or any unrelated regression, the "comprehensive" gate still passed it.

The skip is now conditional on the diagnostic being the expected one -- the
scenario's failure must mention *"Unknown event action"* (the unimplemented
action it is allowlisted for). An allowlisted scenario that fails for any other
reason now calls `fail`. Verified by appending malformed YAML to an allowlisted
scenario and confirming the gate flags it instead of skipping.

### 48. An allowlisted scenario's other errors hid behind the unknown action (medium)

Raised by `chatgpt-codex-connector` on the twenty-third review pass, against
finding 47. Correct -- and it exposed a fail-fast in the loader.

Finding 47 skipped an allowlisted scenario when its diagnostic was *"Unknown
event action"*. But `stringToEventAction()` *threw* on an unknown action during
parse, so `loadFromString()` aborted before `getValidationErrors()` could
inspect the rest of the config. An allowlisted scenario that gained an unrelated
error (an unknown node, a bad parameter) still surfaced only the unknown-action
message, and the gate skipped it.

The loader no longer fail-fasts: an unknown action becomes
`EventAction::UNKNOWN` (its string kept in `action_raw`), the config loads, and
`validateEvent()` reports the unknown action *alongside* every other validation
error. `EventFactory::create()` already maps `UNKNOWN` to a skipped event. The
gate now skips an allowlisted scenario only when *every* reported validation
error is the unknown-action one; any other error -- or a YAML failure that
leaves no validation lines at all -- fails it.

Verified: an allowlisted `start_all_nodes` scenario with a bad partition group
reports both the unknown action and the group error, and the gate fails it; a
clean allowlisted scenario reports only the unknown action and is skipped. A
unit test confirms the loader records the unknown action without aborting and
still flags an unrelated self-link error.

### 49. The drift-report job could not reach the repo (critical)

Raised by `chatgpt-codex-connector` on the twenty-fourth review pass. Correct.

`upstream-drift.yml`'s `report` job runs on the scheduled sweep in a fresh
workspace with no checkout, and set only `GH_TOKEN`. `gh issue list/comment/
create` infer the repository from a local git repo or `GH_REPO`; with neither,
they fail with *"not a git repository"* -- so the drift issue is never opened,
exactly when a nightly upstream break needs to notify.

The job now sets `GH_REPO: ${{ github.repository }}`, which all three `gh`
commands read. A workflow-only fix, on the job whose entire purpose -- surfacing
drift no one is watching for -- was silently defeated by the missing context.

### 50. A partition could fragment a group into extra components (medium)

Raised by `chatgpt-codex-connector` on the twenty-fifth review pass. Correct,
and it needed the harder of the two fixes.

`network_partition` cuts only cross-group links. But `planTopology()`'s
spanning-tree reduction did not know about partition groups, so a group that was
not already a connected subtree fragmented: a 4-node mesh reduced to a star
around n1, partitioned `[[n1,n2],[n3,n4]]`, left n3 and n4 with no edge between
them -- three components, not two -- and the event only *warned* while the run
exited 0. Four of the shipped partition scenarios already tripped this.

Both halves of the reviewer's suggestion, because one alone was not enough.
`planTopology()` now feeds each partition group's internal edges to
`spanningSubset()` as *soft-preferred* links -- prioritised into the tree where
the topology allows (kept for `mesh`; merged into candidates for `random`), but
not flagged unwireable when they cannot fit, since a group need not be a
subtree. All six shipped partition scenarios now realise their groups exactly,
with zero mismatch. And where a topology genuinely cannot -- a `star` whose
group excludes the hub -- `NetworkPartitionEvent` now *fails* rather than warns,
guarded on having actually cut a wired mesh so it never fires on a
NetworkSimulator-only unit test.

A unit test asserts both intra-group edges survive the mesh reduction and the
plan stays a connected tree; an infeasible star partition exits non-zero
end-to-end.

### 51. Infeasible partitions failed only at runtime (medium)

Raised by `chatgpt-codex-connector` on the twenty-sixth review pass, against
finding 50. Correct.

Finding 50's soft-preferred group edges are silently skipped when the topology
cannot honour them (a `star` group excluding the hub, or incompatible groups
across events). So `--validate-only` succeeded while an ordinary run threw when
the partition's timestamp arrived.

`planTopology()` now checks, per partition group, whether the group is connected
using only the plan's intra-group edges. A group that is not is recorded in a
new `plan.infeasible_partitions`, and the entry point fails validation on it
(exit 2) -- the runtime throw from finding 50 stays as a backstop. Verified: a
star partition excluding the hub, and a custom line partitioned into
non-contiguous groups, both fail `--validate-only`.

### 52. The fragmentation check skipped when no cross edge was cut (medium)

Finding 50 guarded its runtime check on `cut > 0`. But a disconnected topology
can fragment with no cross edge to cut: only `A--B` wired, `C` and `D` isolated,
partitioned `[[A,B],[C,D]]` cuts nothing yet has three components, and the guard
suppressed the check.

The check is now guarded on `NodeManager::hasWiredTopology()` -- whether a mesh
was ever wired -- not on the cut count. A real mesh is always checked regardless
of how many cross edges this particular partition severed; only a
NetworkSimulator-level unit test that never wired a mesh is skipped. A unit test
partitions a wired-but-disconnected mesh and asserts the event throws.

### 53. A refused injection was swallowed (medium)

Raised by `chatgpt-codex-connector` on the twenty-seventh review pass, and
consistent with findings 39 and 42. Correct.

`MessageInjectEvent` logged `REFUSED` and returned normally when
`injectMessage()` returned false -- the sender down, or with no mesh to send
into. `EventScheduler` recorded no failure and the run exited 0, so a lifecycle
or partition experiment could pass without ever exercising its asserted probe.

The event now throws when the injection is refused, which `processEvents()`
counts and the exit path turns non-zero (finding 21). A refused injection means
the probe never left the node -- not a delivery outcome to note and move on
from. All four shipped inject scenarios were checked: none produces a refusal
(a partitioned mesh's `sendSingle` is best-effort and returns true), so this
fails only a genuinely broken timeline. A unit test asserts a delivered
injection does not throw and an injection from a stopped node does; an
end-to-end run of an inject-from-stopped-node scenario exits non-zero.

### 54. Overlapping partition groups were planned greedily (medium)

Raised by `chatgpt-codex-connector` on the twenty-eighth review pass, against
finding 50/51. Correct.

Finding 50 fed each partition group a single intra-group *path* (consecutive
pairs) as soft-preferred edges. With overlapping partition events the greedy
reduction could drop a needed edge as cyclic and then report a group infeasible,
even when a valid spanning tree existed. On a 4-node mesh, groups `[n1,n2,n3]`
and `[n1,n3,n4]` gave preferences `1-2, 2-3, 1-3, 3-4`; the reduction kept
`1-2, 2-3`, dropped `1-3` as cyclic, and called the second group infeasible --
though the tree `1-2, 1-3, 3-4` keeps both groups connected.

The planner now offers the whole intra-group *clique* (every pair within each
group) as soft-preferred, not a fixed path. `spanningSubset()`'s union-find then
picks a consistent set: for any group not yet connected, some intra-group pair
bridges two of its components and is kept, so every group that *can* be connected
is. The greedy suboptimality is gone; a group is flagged infeasible only when the
declared topology genuinely has no intra-group edges to keep (a `star` group
excluding the hub). A unit test asserts the reviewer's overlapping-groups case
plans a connected tree with no infeasibility.

### 55. Overlapping partition groups needed joint solving, not a flat clique (medium)

Raised by `chatgpt-codex-connector` on the twenty-ninth review pass, against
finding 54. Correct -- offering the clique was still consumed by the same greedy
union-find. Groups `[[z,x,y],[w]]` then `[[x,y],[z,w]]` gave `z-x, z-y, x-y,
z-w`; the reduction kept `z-x, z-y`, dropped `x-y` as cyclic, and reported
`[x,y]` infeasible -- though `z-x, x-y, z-w` satisfies both events.

Group connectivity is now solved as a constraint set, not by flattening edges
into the reduction. `solveGroupConnectivity()` processes groups
most-constrained-first (smallest) and adds one bridging intra-group edge per
group per round, round-robin, so no group monopolises the shared forest; it
tries several seeded group orderings and takes the first that connects every
group. Its chosen edges become the soft preferences the spanning tree keeps.
This is the connected-subtree-constraints problem (NP-hard for arbitrary
subsets), so it is a strong multi-restart heuristic rather than a complete
solver, and the authoritative per-group feasibility check still runs on the
final plan -- a group the heuristic cannot connect is flagged, never silently
mis-wired.

The reviewer's example now validates: with the flat clique it fails
`--validate-only` (*"partition group [x, y] is not internally connected"*); with
the solver it succeeds. A unit test covers the overlapping case.

### 56. `--validate-only` did not check firmware names (medium)

Raised by `chatgpt-codex-connector` on the thirtieth review pass. Correct.

The firmware-registration check ran during node creation, after `--validate-only`
returns, so a scenario naming an unregistered firmware passed validation while an
ordinary run rejected it -- and with the general-error status `1`, not the
documented validation status `2`. The registry is populated before the config
loads, so the name is resolvable during validation.

The validation phase now checks every non-empty firmware name against the
registry and returns `2` on an unregistered one, before the `--validate-only`
return. Verified: a node naming `NoSuchFirmware` exits `2` under `--validate-only`.

### 57. Degrade latency overflowed when doubled (medium)

`ConnectionDegradeEvent` sets the max latency to `latency * 2`. A latency above
`UINT32_MAX / 2` (e.g. `3000000000`) wrapped below the min, and
`NetworkSimulator::setLatency()` threw only when the event fired, mid-run --
`--validate-only` approved it.

Validation now rejects a `connection_degrade` latency above `UINT32_MAX / 2`
(consistent with the packet-loss bound from finding 44), and the doubling in the
event saturates rather than wraps, so the arithmetic is sound even if the event
is constructed directly. A unit test and a `--validate-only` run confirm the
rejection.

### 58. Group connectivity was measured globally, not induced (medium)

Raised by `chatgpt-codex-connector` on the thirty-first review pass, against
finding 55's solver. Correct.

`solveGroupConnectivity()` checked group connectivity on the *global* union-find,
so a path through a node *outside* the group made the group look connected and
stopped it receiving the internal edges it needed. On `[[n0,n1,n3],[n2]]` and
`[[n1,n2,n3],[n0]]` the solver could pick `n0-n1, n1-n2, n0-n3` and regard the
second group connected via the external `n0`; the final induced check then
rejected it, though `n0-n1, n1-n2, n1-n3` satisfies both.

The solver now measures *induced* connectivity -- a group is connected only via
picked edges internal to it -- and adds an edge only when it bridges two induced
components of the group (and closes no global cycle), matching the authoritative
final check. The reviewer's example now validates; with the global check it fails
`--validate-only`.

### 59. A NaN packet loss passed validation (medium)

`connection_degrade`'s `packet_loss` range check (finding 44) used `< 0 || > 1`,
both false for a NaN, so YAML `.nan` passed validation while
`PacketLossConfig::isValid()` rejected it and `setPacketLoss()` threw at runtime.

Validation now requires `std::isfinite` as well as the range, for `packet_loss`
and for `set_network_quality`'s `quality`. A `.nan` degrade exits `2` under
`--validate-only`.

### 60. Partition solving needed backtracking over edge choices (medium)

Raised by `chatgpt-codex-connector` on the thirty-second review pass, against
finding 55/58. Correct.

The solver varied only group *order*; within a group it still greedily took the
first eligible edge. `[[n0,n2,n3],[n1]]` and `[[n1,n2,n3],[n0]]` fail in either
order -- picking `0-2, 1-2` then `0-3` or `1-3` -- though `0-2, 1-2, 2-3`
satisfies both.

`solveGroupConnectivity()` is now a bounded **backtracking** search: it tries
each eligible intra-group edge (one that bridges two induced components of the
group and closes no global cycle) and recurses, undoing on failure, so the
*choice* of edge varies, not just the order. It is complete within a step budget
(200k); on overflow it returns the best partial, which the final feasibility
check treats as infeasible -- safe. The reviewer's example now validates; capping
the budget to one step reproduces the rejection.

### 61. Allowlisted-scenario errors hid behind the short-circuit (medium)

Raised against finding 48. `main()` returned on the first configuration problem
(the unknown-action validation error), so its firmware and topology feasibility
checks never ran -- an allowlisted scenario that also had an unregistered
firmware or a cyclic link surfaced only the unknown-action diagnostic, and the
gate skipped it.

The validation phase now **collects every problem** -- validation errors,
unwireable links, infeasible partitions, unregistered firmware, unschedulable
events -- into one list and reports them together, rather than returning on the
first. The gate skips an allowlisted scenario only when every reported line is
the expected unsupported-action diagnostic (`Unknown event action` or `action
not implemented`); any other now surfaces alongside it and fails the gate.
Verified: an allowlisted scenario with an unregistered firmware fails, a clean
allowlisted one is skipped.

### 62. A failed partition heal was logged as success (medium)

Consistent with findings 39/42/53. `healNetwork()` retained a cut that both
endpoints were up for but whose handshake did not settle, and returned only the
restored count, so `NetworkHealEvent` logged success and the run exited 0 with
the partition unresolved.

`healNetwork()` now returns `{restored, failed}`; `NetworkHealEvent` throws when
`failed > 0` (a cut pending only because a node is down is not counted -- that
reconnects on start, finding 31). Like the sibling findings the failure branch
cannot occur on loopback, so it is covered by the struct plumbing and a
clean-heal unit test (`failed == 0`).

### 63. Partition solving ignored hard-preferred links (medium)

Raised by `chatgpt-codex-connector` on the thirty-third review pass, against
finding 60. Correct.

The backtracking solver received only the groups and candidate edges, while
`spanningSubset()` then inserts the hard-preferred event links *ahead* of the
solver's group edges. A link event on `n2-n3` with partitions `[[n0],[n1,n2,n3]]`
and `[[n0,n1,n3],[n2]]`: the solver picks `n1-n2, n1-n3, n0-n1`, the link keeps
`n2-n3` and displaces `n1-n3`, and the final check rejects the second partition
-- though `n2-n3, n1-n3, n0-n1` satisfies the link and both partitions.

`solveGroupConnectivity()` now **seeds** its search with the hard-preferred
forest (the declared, non-cyclic event-link edges), so it builds its group edges
on top of the links `spanningSubset()` will keep. The reviewer's example now
validates; passing an empty hard-preferred set to the solver reproduces the
rejection, and the plan keeps the link-event pair. A unit test covers it.

### 64. The gate matched the diagnostic class, not the specific action (medium)

Raised by `chatgpt-codex-connector` on the thirty-fourth review pass, against
finding 61's gate filter. Correct.

The filter skipped an allowlisted scenario when every error line was an "Unknown
event action" or "action not implemented" diagnostic -- the *class*, not the
specific action each file is allowlisted for. A misspelling
(`partial_heal` -> `partial_hel`) or an unrelated unknown action still matched
the class, so the scenario skipped and CI stayed green over a typo.

Each allowlisted filename is now paired with its expected action
(`expected_unsupported_action()`): `issue_138_cascade_healing` -> `partial_heal`,
the two partition scenarios -> `start_all_nodes`. The gate skips only when a line
names *exactly* that action and every reported line is either that named action
or the generic "action not implemented". A misspelled or unrelated action falls
to `other` and fails. Verified: all three shipped scenarios skip against their
named action, and `partial_heal` misspelled as `partial_hel` fails the gate.

### 65. A delayed-restart timestamp could overflow the event clock (medium)

Raised by `chatgpt-codex-connector` on the thirty-fifth review pass, against
finding 45. Correct.

Finding 45 bounded `time + delay` against the duration -- but only for a finite
duration. For `duration: 0` (infinite) the guard was skipped, while
`scheduleAll()` computes the start time as `uint32`, so a sum past `UINT32_MAX`
wrapped to an earlier timestamp and the start fired before the stop.

Validation now rejects `time + delay > UINT32_MAX` (computed in `uint64`)
regardless of the duration, in addition to the finite-duration bound; and
`scheduleAll()` saturates the addition as defense. Verified: a `duration: 0`
restart at `4e9` with delay `1e9` exits `2` under `--validate-only`.

### 66. A NaN random density reached a size_t cast (medium)

Consistent with findings 59 and 66. `topology.density: .nan` passed the
`< 0 || > 1` range check (both false for NaN), and `planTopology()` -- now that
it wires the mesh -- cast `NaN` to `size_t` for the edge target, which is
undefined behaviour.

Validation now requires `std::isfinite(density)` for a `random` topology. A
`.nan` density exits `2` under `--validate-only`.

### 67. Firmware ran while startup connectivity was still settling (medium)

Raised by `chatgpt-codex-connector` on the thirty-sixth review pass. Correct.

`main()` starts every node -- which resumes its firmware tasks (finding 13) --
and *then* wires the mesh. `establishConnectivity()` settles each link by
pumping the shared scheduler, which also runs those firmware tasks. So a
short-interval firmware, or a scenario with many/slow links, sent messages and
moved metrics during topology construction, over a partially-wired mesh, before
the timeline began.

`establishConnectivity()` (both overloads) now suspends every node's firmware
before wiring and resumes it after, using the finding-13 suspend/resume. Runtime
rewiring (heal/restore/reconnect) goes through `connectNodes()` directly and is
deliberately unaffected -- the mesh is live then. Verified: a 6-node mesh with a
100 ms firmware interval sends zero broadcasts before "connectivity established"
(it sent several before). A unit test asserts `messages_sent == 0` after wiring
and that firmware is live again.

### 68. Events took effect one tick after their timestamp (medium)

Raised by `chatgpt-codex-connector` on the thirty-seventh review pass. Correct.

The run loop called `updateAll()` -- advancing every node and firmware once for
the timestamp -- *before* `processEvents()`. So a `stop_node` or
`partition_network` at `t`, or a send coinciding with a later outage, emitted
traffic and moved metrics for that tick before the outage took effect.

The loop now computes the elapsed time and processes the due events *before*
advancing the nodes, so the declared timestamp is the actual state boundary.
Verified: a `stop_node` at `t=0` leaves that node with `sent=0` (it sent once
before the reorder); a running peer still sends.

### 69. Suspension did not gate the firmware loop (medium)

Raised against finding 67. `FirmwareBase::suspend()` disables the firmware's
scheduler tasks and blocks its send helpers, but `VirtualNode::update()` called
`firmware_->loop()` unconditionally. A firmware doing work in `loop()` -- or
sending directly through its `mesh_` pointer, bypassing the helper guard --
still ran several times over a partially-wired mesh during startup.

`update()` now skips `loop()` while the firmware is suspended, so the suspension
from findings 13 and 67 is complete. A unit test with a firmware that counts its
`loop()` calls asserts zero during wiring; removing the gate makes it non-zero.

### 70. Suspension did not gate the firmware connection callbacks (medium)

Raised against finding 69. The `loop()` gate closed one path, but the mesh's
`onNewConnection`/`onChangedConnections` callbacks routed through `VirtualNode`
still fired unconditionally. Every `settleLink()` pumps the loopback handshake,
which drives those callbacks -- so firmware such as `LibraryValidationFirmware`
mutated connection/topology state (and arbitrary custom callbacks did arbitrary
work) over a half-wired mesh before `start_time`.

These callbacks are edge-triggered, so gating alone would have dropped the
firmware's view of the topology it booted into. They are now *deferred* while
suspended and *replayed* once wiring settles: `onNewConnection` is queued per
peer and `onChangedConnections` collapsed to a single flag, replayed by the new
`VirtualNode::resumeFirmware()` -- through which both the node's own `start()`
and `NodeManager`'s startup-wiring resume now go. `onReceive` is gated but not
queued: a message arriving mid-wiring is pre-boot handshake traffic, never a
timeline inject (those run after connectivity settles), so replaying it would be
wrong. A unit test records, for every callback, whether the firmware was
suspended when it fired, and asserts it never was while still confirming the
neighbours were replayed; removing the gate makes a callback fire while
suspended and the test fails.

### 71. Heal restored explicitly dropped connections in the network model (medium)

Raised against the coexisting-severance-reasons fix. `NetworkHealEvent` called
`NetworkSimulator::restoreAllConnections()`, which cleared *every* modelled
drop. After the mesh-side `NodeManager` learned to keep an explicit
`connection_drop` severed across a partition heal, the model no longer matched:
a pair the mesh still held down was marked live in the `NetworkSimulator`, so
`isConnectionActive()` and messages enqueued through the model treated it as
restored.

The model now tracks the two severance reasons in separate sets, exactly as the
`NodeManager` does: `dropConnection()` records an explicit drop (cleared by
`restoreConnection()`), and a new `partitionConnection()` records a partition
cut (cleared by a new `healPartitions()`). `isConnectionActive()` is true only
when a pair is in neither set. `NetworkPartitionEvent` now severs through
`partitionConnection()` and `NetworkHealEvent` heals through `healPartitions()`,
so an explicit drop outlives a heal in the model just as it does in the mesh.
A unit test drops one pair explicitly and partitions another, heals, and asserts
the partitioned pair is live again while the explicitly dropped pair stays down
-- including the pair-severed-for-both-reasons case, which stays down until both
reasons clear.

### 72. Status doc listed an implemented action as unimplemented (low)

`DEVELOPMENT_STATUS.md` still listed `inject_message` among the actions that are
"parsed and validated but with no runtime event class", and counted "nine event
classes" wired into the run loop. This PR added `MessageInjectEvent`, wired it in
`EventFactory`, and exercises it in the integration gate. The doc now lists
`inject_message` among the ten wired event classes and drops it from the
unimplemented list, so it no longer warns users off a supported action.

### 73. An interrupted run still reported success (medium)

Raised against the delayed-restart / event-failure exit-code work. On
`SIGINT`/`SIGTERM` the run loop's `while (running)` exits, but the completion
check considered only events that *threw* (`getFailedCount()`). A signal
arriving before the timeline finished left arbitrary events unrun, yet the
process printed *"Simulation completed successfully"* and exited 0 -- a gate or
experiment would accept a run that stopped halfway. The finite-duration
delayed-restart case was already bounded; signal-driven shutdown was not.

The signal handler now records the signal number, and after the run the entry
point treats an interrupted run (`received_signal != 0`, equivalently `!running`
-- the duration-reached path leaves `running` set) that still
`hasPendingEvents()` as incomplete: it names the unrun timeline and returns
`128 + signal` (130 for SIGINT, 143 for SIGTERM), distinct from the validation
(2) and event-failure (1) codes. An interrupt that arrives after the last event
already ran is a clean stop and still exits 0. A new behavioural gate step
(step 10) starts a scenario with an event at `t=20`, sends `SIGTERM` at `t≈2`,
and asserts the process exits non-zero and names the pending timeline; removing
the check makes it exit 0 and the step fails.

### 74. The validation sweep could reach the planner with a non-finite density (medium)

Raised against the finite-density validation fix. Validation records a
non-finite/out-of-range `density` and the run returns 2 -- but the same entry
point runs `planTopology()` as a feasibility check *before* acting on those
collected problems, so the planner is reached with the bad value still in hand.
The random planner computes `static_cast<size_t>(density * possible + 0.5)`, and
`static_cast<size_t>` of a NaN, an infinity, or a negative product is undefined
behaviour -- so `--validate-only` on an already-invalid config could crash or
behave unpredictably.

The planner now clamps `density` to the `[0, 1]` fraction it is defined to be
(non-finite or negative → 0, above one → 1) before the conversion, making
`planTopology()` total for any input while validation stays the authoritative
rejecter. The unit test is a totality/success-path guard: the RANDOM plan
reduces to a spanning tree regardless of density and this platform's
`float → size_t` for a non-finite value is benign, so the UB is not a
deterministic failure to assert against -- the test pins that the planner never
crashes or returns a wild size, which is the guarantee the clamp provides.

### 75. Runtime link settlement ran firmware between same-second events (medium)

Raised against the run-loop reorder (finding 68). A runtime `heal_partition`,
`connection_restore`, or node rejoin settles its link through `settleLink()`,
which pumps `updateAll()` -- and `updateAll()` runs every node's firmware tasks
and `loop()`. Because those events execute inside `EventScheduler::processEvents()`
for a timestamp, a periodic send due at that instant was emitted *between* two
same-second events, breaking finding 68's promise to process all due events
before advancing firmware. (Startup wiring was already covered by findings
67/69/70; this is the runtime counterpart.)

`settleLink()` now suspends every firmware for the duration of the settle,
preserving each node's prior state so a settle nested inside startup wiring (all
firmware already suspended) stays suspended and is resumed once by its caller.
The mesh's own handshake tasks live on the shared scheduler and are not firmware
tasks, so the handshake still completes. A unit test wires a new link *after*
startup with a firmware that counts its `loop()` calls and asserts the count
stays zero across the settle; removing the suspension makes it non-zero.

### 76. Partition component counting used recorded edges, not live connections (medium)

Raised against the severance model. `getConnectedComponents()` -- which
`NetworkPartitionEvent` uses to assert a partition produced the requested number
of fragments -- traversed the recorded `topology_` graph filtered only by
`isLinkSevered()`. But a stopped or crashed node closes its mesh connections
*without* adding a severance marker, so a path through a downed node still read
as connected. In a line `A-B-C-D`, stopping articulation node `B` and then
requesting `[[A,B,C],[D]]` passed the two-component check even though the live
mesh had `A` and `C` disconnected -- so a combined lifecycle-and-partition
experiment could exit successfully with more fragments than requested.

The traversal now walks live connections (`VirtualNode::isConnectedTo()`, which
reads the actual mesh sockets) instead of recorded edges, so a stopped node
splits its group here exactly as it does in the running mesh. A unit test stops
an articulation node on a wired line and asserts the two nodes it bridged no
longer share a component; the old recorded-topology traversal put them together.

### 77. Callback replay after a settle still ran mid-batch (medium)

Raised against finding 75. Suspending firmware for the *settle* was not enough:
`settleLink()`'s restore resumed firmware -- and `resumeFirmware()` synchronously
replays the deferred `onNewConnection`/`onChangedConnections` callbacks and
re-enables sends -- at the *end* of the settle, which is still inside
`processEvents()` for the timestamp. So a heal/restore/rejoin callback could send
or mutate state between two same-second events (before a following stop or
partition), despite finding 68's ordering guarantee.

The callback replay is now deferred to the settling node's next `update()`,
which runs after the whole event batch, rather than at the end of the settle. A
runtime settle resumes the firmware's scheduler *tasks* directly (so its cadence
is untouched) but leaves the deferred `onNewConnection`/`onChangedConnections`
queued; `VirtualNode::update()` replays them just before `loop()`, the first time
the firmware runs unsuspended -- which the run loop reaches only after every due
event has executed. Startup wiring still replays eagerly on resume
(`resumeFirmware()`), preserving finding 70. A unit test joins an isolated node
at runtime and asserts the callback is *not* replayed when the settle restores,
only on the next `update()`; replaying synchronously at settle end fails it.

(An earlier attempt suspended firmware around the whole `processEvents()` batch
instead. That regressed the partition gate: `FirmwareBase::resume()` re-enables
tasks via `Task::enable()`, which resets their timing, so suspending and resuming
every timestamp re-fired every periodic task and inflated deliveries ~20x. The
run loop's firmware cadence must not be perturbed -- deferring only the replay
does not touch it.)

### 78. A stale connection to a stopped node still merged components (medium)

Raised against finding 76. Checking only `cur->isConnectedTo(peer)` was
one-sided: a running peer can hold a stale connection object to a stopped node
until the next IO poll (the `reconnectNode()` path documents this), so stopping
`B` and then requesting `[[A,B],[C],[D]]` could still count `A-B` as connected
and pass three components even though `A-B` is already down and the first group
is not connected.

The traversal now requires both endpoints to be **running** and the link live
from **both** views (`cur->isConnectedTo(peer)` and `peer->isConnectedTo(cur)`),
so a stale one-sided socket no longer merges a stopped node into a live
component. The finding-76 unit test gains an assertion that the stopped node does
not share a component with its still-running neighbour; the one-sided check put
them together.

### 79. Validation missed lifecycle-dependent event failures (medium)

Raised against the validation sweep. `planTopology()` checks a scenario's
*static* feasibility, but validation otherwise only constructs and queues the
events -- so a timeline guaranteed to fail because of an *earlier* event passed
`--validate-only` and only threw once the run was underway. Two concrete cases:
a `stop_node B` followed by `partition_network [[A,B],[C]]` on a line `A-B-C`
(the static plan keeps `A-B`, but at runtime the stopped `B` splits the group
and the partition throws), and an `inject_message` scheduled after its sender was
stopped (refused at runtime).

A new `validateEventTimeline()` walks the events in scheduled order, tracking
each node's up/down state (and connection drops / partition cuts), and reports
the deterministic mismatches: a `network_partition` whose groups no longer form
exactly that many live components, and an `inject_message` from a by-then-stopped
sender. It models the same live-component count the runtime measures, so it does
not reject a valid scenario -- a partition after stopping a *leaf* still passes,
and a `restart_node` that brings a node back before the partition passes. A
timeline containing an action it cannot model (an `UNKNOWN`/unimplemented action,
or `add_nodes`/`remove_node`, which change the node set) skips the check rather
than guess, so it never fires a false positive on a scenario that already fails
validation for another reason. Wired into the entry point's validation sweep so
`--validate-only` and the CI gate reject these scenarios up front; unit tests
cover both rejections, all three accept cases, and the skip.

## Remaining gaps

These are real work, not oversights, and are deliberately left for follow-up
rather than faked with a misleading alias.

| Gap | Blocks | Notes |
|---|---|---|
| `start_all_nodes` event action | `network_partition_test.yaml`, `split_brain_partition_test.yaml` | No enum value or event class; mechanically similar to `NodeStartEvent` over all node ids |
| `partial_heal` event action | `issue_138_cascade_healing.yaml` | `NetworkHealEvent` clears every partition unconditionally; needs a subset argument |
| `add_nodes`, `remove_node`, `break_link`, `restore_link`, `set_network_quality` | -- | Parsed and validated, no runtime class. `EventFactory` reports them |
| Links beyond a spanning tree | `mesh`, `ring`, dense `random` | painlessMesh holds a tree. A declared full mesh is reduced and reported (finding 14); modelling a genuinely multi-path mesh would need a transport the library does not have |
| `bidirectional: false` on a ring | Directional-link scenarios | A simulated link is one TCP connection carrying both ways. The plan warns rather than pretending |
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
- Unit tests: 168 test cases, 1697 assertions, all passing. Findings 14-24 and
  26-79 each added coverage (30 and 73 are gate-script/entry-point; the failure
  branches of 39 and 42 are loopback-undefined and 74 is UB-hardening, so those
  are unit-covered on the success path; 72 is a documentation correction);
  finding 25 is a seed-resolution change in the entry point, verified by
  running two seedless scenarios and observing different drawn seeds, with the
  gate scenarios pinned so CI stays deterministic.
- Scenarios: 20 of 23 validate; 3 skipped for unimplemented event actions.
- Behavioural gate: passes on the fixed build, fails with 7 problems on the
  build that preceded findings 1-8.
- Findings 9 and 10 were each measured before and after, on the numbers
  tabulated above, and each has a gate step of its own. Both steps were run
  against a binary built from the commit that preceded them: step 5 reports
  *"partitioned run received 598 vs 598 intact -- the split cost nothing"* and
  step 6 *"the restarted node re-established 0 mesh links"*, exit 1. They are
  not vacuous.
- Finding 13 the same way: with the `suspend()` calls removed, gate step 7
  reports *"node 1226381676 broadcast 5 time(s) while stopped"*, exit 1, and
  the new unit test fails on `messages_sent 10 == 5`. Steps 1-6 still pass on
  the fixed build, so the change is not a regression trade.
- Findings 14 and 16 likewise. Forced back onto the random-tree path, gate
  step 8 reports *"connection_events_test declares a mesh but did not wire
  one"*, exit 1; the naive alternative -- wiring the declared mesh as-is -- was
  measured at 0 live links and 0 messages before it was rejected. With the
  sequence tie-break removed, the ordering tests fail on
  `{a, c, f, e, b, d} == {a, b, c, d, e, f}`.
- Findings 17 and 18 the same way. With `settleLink()` removed from
  `connectNodes()`, gate step 9's injection is refused --
  *"Message injected ... -- REFUSED"*; with `BasicInoFirmware` sending through
  `mesh->` directly again, its scenario reports `Total messages sent: 0` while
  peers receive 44. All 9 gate steps pass on the fixed build.
- Finding 19 at the unit level: with the action filter removed from
  `eventPairs()`, both injection-bias tests fail -- an injection once again
  changes the planned tree. All 9 gate steps still pass, since no shipped
  scenario's assertions depended on the bias.
- Findings 20 and 21 at the unit level. With `settleLink()` forced to return
  true, the never-connected pair test fails; with the failure counter removed,
  the scheduler's `getFailedCount()` assertions fail. Both are unit-covered
  rather than gated: a settlement timeout does not occur on loopback, and no
  shipped event throws at execution time.
- Findings 22 and 23 the same way. With the `targets` spelling ignored, the
  two-spellings-plan-the-same-graph test fails; with the `restoreLink()` guard
  removed, the isolated-node restore test connects the undeclared pair and
  fails. Both reworked from a first cut that passed regardless -- the topology
  assertion was seed-dependent, and the restore pair was reachable through a
  third node, so painlessMesh's own dedup, not the guard, was refusing it.
- Finding 24: with the self-link validation check removed, the self-link
  rejection test fails, and `connections: [["n1","n1"]]` runs a random tree
  again.

CI on PR #59 confirmed the Docker-based jobs: lint, both Docker builds, unit
tests and the new behavioural integration gate all pass on GitHub runners.

The `upstream-drift` job's first run failed for a reason unrelated to drift --
it built on a bare runner, where 6 pre-existing partition tests fail on
`boost::asio` bind. It now builds in the project's Docker image like every other
job here. Its issue-filing step only runs on the nightly schedule and remains
unexercised.
