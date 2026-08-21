#!/usr/bin/env bash
#
# Behavioural gate for the simulator.
#
# The previous integration step ran one scenario and checked that the process
# exited 0. It always did -- including when every node failed to load its
# firmware and no message was ever sent. This script asserts on what the
# simulator actually did, so a green run means something.
#
# Usage: scripts/ci_integration_check.sh [path-to-simulator-binary]

set -uo pipefail

SIM="${1:-build/bin/painlessmesh-simulator}"
SCENARIO_DIR="examples/scenarios"

# Scenarios whose event actions have no runtime implementation yet. Listed
# explicitly so the gap stays visible instead of being rounded off to "passing".
KNOWN_UNSUPPORTED=(
  "issue_138_cascade_healing.yaml"     # needs partial_heal
  "network_partition_test.yaml"        # needs start_all_nodes
  "split_brain_partition_test.yaml"    # needs start_all_nodes
)

failures=0
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

fail() { echo "FAIL: $*"; failures=$((failures + 1)); }
pass() { echo "  ok: $*"; }

if [ ! -x "$SIM" ]; then
  echo "FAIL: simulator binary not found at $SIM"
  exit 1
fi

is_known_unsupported() {
  local name; name="$(basename "$1")"
  for known in "${KNOWN_UNSUPPORTED[@]}"; do
    [ "$name" = "$known" ] && return 0
  done
  return 1
}

echo "== 1. every shipped scenario must parse and validate =="
for scenario in "$SCENARIO_DIR"/*.yaml; do
  if out=$("$SIM" --config "$scenario" --validate-only 2>&1); then
    pass "$(basename "$scenario")"
  elif is_known_unsupported "$scenario" && \
       echo "$out" | grep -q 'Unknown event action'; then
    # Skip only when the failure is the expected unsupported-action diagnostic.
    # A KNOWN_UNSUPPORTED scenario that breaks for any other reason (malformed
    # YAML, an unknown node, a bad parameter) must still fail the gate.
    echo "  SKIP: $(basename "$scenario") (known unsupported event action)"
  elif is_known_unsupported "$scenario"; then
    fail "$(basename "$scenario") is allowlisted but failed for another reason"
    echo "$out" | grep -E 'ERROR|  - ' | head -3
  else
    fail "$(basename "$scenario") does not validate"
    echo "$out" | grep -E 'ERROR|  - ' | head -3
  fi
done

echo
echo
echo "== 1b. --validate-only must reject an infeasible timeline, not just bad YAML =="
# --validate-only used to return before the topology/event feasibility checks,
# so a scenario a normal run rejects (cyclic event links, a link event on a
# non-edge of an explicit topology) passed the validation sweep. Build one and
# confirm it is caught.
cat > "$tmp/infeasible.yaml" <<'YAML'
simulation:
  name: "cyclic drops"
  duration: 30
  seed: 1
nodes:
  - id: "n1"
    firmware: "SimpleBroadcast"
    config: {mesh_prefix: "IF", mesh_password: "if123456", mesh_port: 5995}
  - id: "n2"
    firmware: "SimpleBroadcast"
    config: {mesh_prefix: "IF", mesh_password: "if123456", mesh_port: 5995}
  - id: "n3"
    firmware: "SimpleBroadcast"
    config: {mesh_prefix: "IF", mesh_password: "if123456", mesh_port: 5995}
topology:
  type: "mesh"
events:
  - {time: 5,  action: connection_drop, from: "n1", to: "n2"}
  - {time: 10, action: connection_drop, from: "n2", to: "n3"}
  - {time: 15, action: connection_drop, from: "n1", to: "n3"}
YAML
if "$SIM" --config "$tmp/infeasible.yaml" --validate-only >/dev/null 2>&1; then
  fail "--validate-only accepted an infeasible timeline (cyclic event links)"
else
  pass "--validate-only rejects an infeasible timeline"
fi

echo "== 2. broadcast scenarios must actually move messages, and be seen doing it =="
# ino_firmware_test covers the .ino wrapper, which sent through mesh-> directly
# and bypassed the accounting hook: it reported 0 sent while its peers received
# 44. The unit suite cannot reach that firmware (REGISTER_FIRMWARE's static
# registration does not reach the test binary), so it is asserted here.
for broadcast_scenario in firmware_broadcast ino_firmware_test; do
  out=$("$SIM" --config "$SCENARIO_DIR/$broadcast_scenario.yaml" --duration 20 2>&1)
  rc=$?
  sent=$(echo "$out" | sed -n 's/^Total messages sent: //p')
  received=$(echo "$out" | sed -n 's/^Total messages received: //p')
  if [ $rc -ne 0 ]; then
    fail "$broadcast_scenario exited $rc"
  elif [ -z "${sent:-}" ] || [ "$sent" -lt 10 ]; then
    fail "$broadcast_scenario sent ${sent:-0} messages, expected >= 10"
  elif [ -z "${received:-}" ] || [ "$received" -lt "$sent" ]; then
    fail "$broadcast_scenario received ${received:-0} for $sent sent -- broadcast is not reaching peers"
  else
    pass "$broadcast_scenario: sent=$sent received=$received"
  fi
done

echo
echo "== 3. a lifecycle scenario must actually fire its events =="
out=$("$SIM" --config "$SCENARIO_DIR/node_lifecycle_test.yaml" 2>&1)
rc=$?
fired=$(echo "$out" | grep -c '^\[EVENT\] t=')
if [ $rc -ne 0 ]; then
  fail "node_lifecycle_test exited $rc"
elif [ "$fired" -lt 5 ]; then
  fail "node_lifecycle_test fired $fired scheduled events, expected >= 5"
elif ! echo "$out" | grep -q 'crashed (ungraceful)'; then
  fail "node_lifecycle_test never crashed a node"
elif ! echo "$out" | grep -qE '^\[[0-9]+s\] [0-9]+/[0-9]+ nodes running'; then
  fail "node_lifecycle_test never reported a running-node count"
else
  # After the crash, at least one progress line must report fewer nodes running
  # than the mesh holds. Derived rather than hardcoded so the check survives a
  # scenario with a different node count.
  running_dropped=$(echo "$out" \
    | sed -n 's/^\[[0-9]*s\] \([0-9]*\)\/\([0-9]*\) nodes running.*/\1 \2/p' \
    | awk '$1 < $2' | wc -l)
  if [ "$running_dropped" -lt 1 ]; then
    fail "a node was crashed but the running count never dropped"
  else
    pass "$fired events fired; running count reflected the crash"
  fi
fi

echo
echo "== 4. a scenario naming unknown firmware must fail, not pass quietly =="
sed 's|firmware: "SimpleBroadcast"|firmware: "NoSuchFirmware"|' \
  "$SCENARIO_DIR/simple_mesh.yaml" > "$tmp/bad_firmware.yaml"
if "$SIM" --config "$tmp/bad_firmware.yaml" --duration 5 >/dev/null 2>&1; then
  fail "a scenario with unloadable firmware exited 0"
else
  pass "unloadable firmware fails the run"
fi

echo
echo "== 5. a partition must actually cost deliveries, and a heal must repay them =="
# The regression this catches: link events used to mutate only the standalone
# NetworkSimulator, which nothing on the delivery path consults, so a
# "partitioned" mesh carried exactly as much traffic as an intact one.
part_scenario="$SCENARIO_DIR/partition_delivery_test.yaml"
sed '/^events:/,$d' "$part_scenario" > "$tmp/partition_control.yaml"
sed '/^  - time: 26$/,$d' "$part_scenario" > "$tmp/partition_only.yaml"

received_of() { echo "$1" | sed -n 's/^Total messages received: //p'; }

# Capture each exit code: the simulator prints its totals before returning
# nonzero for a failed scheduled event, so a run that exits 1 could otherwise
# still supply counts that satisfy the assertions below.
control_out=$("$SIM" --config "$tmp/partition_control.yaml" 2>&1); control_rc=$?
split_out=$("$SIM" --config "$tmp/partition_only.yaml" 2>&1); split_rc=$?
heal_out=$("$SIM" --config "$part_scenario" 2>&1); heal_rc=$?

control_rx=$(received_of "$control_out")
split_rx=$(received_of "$split_out")
cut_links=$(echo "$heal_out" | sed -n 's/.*(\([0-9]*\) mesh link(s) cut).*/\1/p' | head -1)
restored_links=$(echo "$heal_out" | sed -n 's/.*(\([0-9]*\) mesh link(s) restored).*/\1/p' | head -1)

if [ "$control_rc" -ne 0 ] || [ "$split_rc" -ne 0 ] || [ "$heal_rc" -ne 0 ]; then
  fail "a partition/heal probe exited nonzero (control=$control_rc split=$split_rc heal=$heal_rc)"
elif [ -z "${control_rx:-}" ] || [ "$control_rx" -lt 20 ]; then
  fail "partition control run received ${control_rx:-0} messages, too few to compare against"
elif [ -z "${split_rx:-}" ]; then
  fail "partitioned run reported no receive total"
elif [ "$((split_rx * 100 / control_rx))" -ge 80 ]; then
  fail "partitioned run received $split_rx vs $control_rx intact -- the split cost nothing"
elif [ -z "${cut_links:-}" ] || [ "$cut_links" -lt 1 ]; then
  fail "the partition event cut ${cut_links:-0} live mesh links"
elif [ -z "${restored_links:-}" ] || [ "$restored_links" -lt 1 ]; then
  fail "the heal event restored ${restored_links:-0} mesh links"
else
  pass "split received $split_rx vs $control_rx intact; $cut_links link(s) cut, $restored_links restored"
fi

echo
echo "== 6. a restarted node must rejoin the mesh, not just report running =="
# Before start() rebuilt the mesh, a stopped-then-started node came back with
# its painlessMesh routing torn down: full marks on the running count, a third
# of its peers' receive count, and not one of its own sends leaving the node.
out=$("$SIM" --config "$SCENARIO_DIR/restart_rejoin_test.yaml" --log-level DEBUG 2>&1)
rc=$?
# Step 7 asserts on the same 40s run rather than paying for a second one.
printf '%s\n' "$out" > "$tmp/restart_rejoin.log"
relinked=$(echo "$out" | sed -n 's/.*started (\([0-9]*\) mesh link(s) re-established).*/\1/p' | head -1)
# The scenario stops exactly one node, so the worst receive count in the run is
# that node's. Comparing it against the best avoids hardcoding a message count
# that would drift with machine speed.
restarted_rx=$(echo "$out" | sed -n 's/^  Node [0-9]*: sent=[0-9]*, received=\([0-9]*\)$/\1/p' | sort -n | head -1)
best_rx=$(echo "$out" | sed -n 's/^  Node [0-9]*: sent=[0-9]*, received=\([0-9]*\)$/\1/p' | sort -n | tail -1)

if [ $rc -ne 0 ]; then
  fail "restart_rejoin_test exited $rc"
elif [ -z "${relinked:-}" ] || [ "$relinked" -lt 1 ]; then
  fail "the restarted node re-established ${relinked:-0} mesh links"
elif [ -z "${best_rx:-}" ] || [ "$best_rx" -lt 10 ]; then
  fail "restart_rejoin_test moved too little traffic to judge (best node received ${best_rx:-0})"
elif [ "$((restarted_rx * 100 / best_rx))" -lt 60 ]; then
  fail "restarted node received $restarted_rx against a peer best of $best_rx -- it never rejoined"
else
  pass "restarted node received $restarted_rx vs peer best $best_rx; $relinked link(s) re-established"
fi

echo
echo "== 7. a stopped node must stop transmitting, and resume when it starts =="
# Every node's firmware tasks live on NodeManager's one shared Scheduler, which
# updateAll() executes for the whole fleet. A stopped node therefore went on
# broadcasting through its torn-down mesh for the whole downtime -- 5 phantom
# sends across the 10s outage here -- and each one was booked as a real
# transmission. The second assertion keeps the cure honest: suspending the
# firmware forever would silence the ghosts too.
rr_log="$tmp/restart_rejoin.log"
if [ ! -s "$rr_log" ]; then
  fail "step 6 produced no restart_rejoin log to assert on"
else
  stopped_id=$(sed -n 's/^\[EVENT\] Node \([0-9]*\) stopped.*/\1/p' "$rr_log" | head -1)
  if [ -z "${stopped_id:-}" ]; then
    fail "restart_rejoin_test stopped no node"
  else
    ghost_sends=$(awk -v id="$stopped_id" '
      $0 ~ "\\[EVENT\\] Node " id " stopped"  { down = 1; next }
      $0 ~ "\\[EVENT\\] Node " id " started"  { down = 0; next }
      down && $0 ~ "Node " id " broadcasting"   { n++ }
      END { print n + 0 }' "$rr_log")
    sends_after_restart=$(awk -v id="$stopped_id" '
      $0 ~ "\\[EVENT\\] Node " id " started" { up = 1; next }
      up && $0 ~ "Node " id " broadcasting"    { n++ }
      END { print n + 0 }' "$rr_log")
    if [ "$ghost_sends" -gt 0 ]; then
      fail "node $stopped_id broadcast $ghost_sends time(s) while stopped"
    elif [ "$sends_after_restart" -lt 1 ]; then
      fail "node $stopped_id never broadcast again after restarting"
    else
      pass "no sends during downtime; $sends_after_restart send(s) after restart"
    fi
  fi
fi

echo
echo "== 8. a scenario's declared topology must be the graph that gets wired =="
# `topology:` was parsed and validated from the first release and never
# applied: every run got a random spanning tree instead. 21 of the 23 shipped
# scenarios declare a topology, so the newly live link events were addressing
# edges that did not exist -- connection_events_test declares a full mesh and
# its t=20 drop of a named pair reported "0 live endpoint(s) closed".
#
# painlessMesh holds a spanning tree whatever it is handed (wiring all 6 links
# of a 4-node mesh at once measured 0 live links and no traffic at all), so the
# planner reduces the declared graph and keeps event-named pairs first. That is
# what this asserts: the declared type is wired, and a declared drop cuts.
# Trimmed to the first drop: the validator rejects --duration shorter than the
# scenario's last event, and the later events add 65s to the gate for nothing.
sed -e 's/^  duration: 90.*/  duration: 25/' -e '/^  - time: 35$/,$d' \
  "$SCENARIO_DIR/connection_events_test.yaml" > "$tmp/topology_probe.yaml"
topo_out=$("$SIM" --config "$tmp/topology_probe.yaml" 2>&1)
topo_rc=$?
declared_drop=$(printf '%s\n' "$topo_out" \
  | sed -n 's/.*Connection dropped:.*(\([0-9]*\) live endpoint(s) closed).*/\1/p' | head -1)
live_before_drop=$(printf '%s\n' "$topo_out" \
  | sed -n 's/^\[15s\] [0-9]*\/[0-9]* nodes running, \([0-9]*\) live link(s).*/\1/p' | head -1)

if [ $topo_rc -ne 0 ]; then
  fail "the trimmed connection_events_test exited $topo_rc"
elif ! printf '%s\n' "$topo_out" | grep -q 'topology=mesh'; then
  fail "connection_events_test declares a mesh but did not wire one"
elif [ -z "${live_before_drop:-}" ] || [ "$live_before_drop" -lt 1 ]; then
  fail "the wired topology carried ${live_before_drop:-0} live link(s) before the drop"
elif [ -z "${declared_drop:-}" ] || [ "$declared_drop" -lt 1 ]; then
  fail "a declared connection_drop closed ${declared_drop:-0} live endpoint(s) -- the named pair was never wired"
else
  pass "mesh wired, $live_before_drop live link(s), declared drop closed $declared_drop endpoint(s)"
fi

echo
echo "== 9. an event must find the links the event before it restored =="
# connectNodes() only starts an asynchronous TCP connect, and EventScheduler
# runs same-time events back to back with no pump between them. So a heal
# reported "1 mesh link(s) restored" and the injection declared for the same
# second was REFUSED -- the route did not exist yet, and the link only showed
# live at the next progress tick. Ordering the events correctly (step: finding
# 16) is not enough on its own; the link has to be carrying traffic.
heal_out=$("$SIM" --config "$SCENARIO_DIR/heal_then_inject_test.yaml" 2>&1)
heal_rc=$?
restored=$(printf '%s\n' "$heal_out" \
  | sed -n 's/.*Network partitions healed (\([0-9]*\) mesh link(s) restored).*/\1/p' | head -1)
refused=$(printf '%s\n' "$heal_out" | grep -c 'Message injected.*REFUSED')
delivered=$(printf '%s\n' "$heal_out" | grep -c '^\[EVENT\] Message injected from')

if [ $heal_rc -ne 0 ]; then
  fail "heal_then_inject_test exited $heal_rc"
elif [ -z "${restored:-}" ] || [ "$restored" -lt 1 ]; then
  fail "the heal restored ${restored:-0} mesh link(s), so the injection proves nothing"
elif [ "$delivered" -lt 1 ]; then
  fail "the same-second injection never ran"
elif [ "$refused" -gt 0 ]; then
  fail "the injection declared in the same second as the heal was refused -- the restored link was not live yet"
else
  pass "heal restored $restored link(s) and the same-second injection was delivered"
fi

echo
if [ "$failures" -gt 0 ]; then
  echo "integration check FAILED ($failures problem(s))"
  exit 1
fi
echo "integration check passed"
