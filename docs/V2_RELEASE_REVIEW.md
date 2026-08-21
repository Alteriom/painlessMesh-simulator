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
- Unit tests: 136 test cases, 1558 assertions, all passing. Findings 14-24 and
  26-34 each added coverage (30 is gate-script only); finding 25 is a
  seed-resolution change in the entry point, verified by running two seedless
  scenarios and observing different drawn seeds, with the gate scenarios pinned
  so CI stays deterministic.
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
