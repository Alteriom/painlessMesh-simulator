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
  elif is_known_unsupported "$scenario"; then
    echo "  SKIP: $(basename "$scenario") (known unsupported event action)"
  else
    fail "$(basename "$scenario") does not validate"
    echo "$out" | grep -E 'ERROR|  - ' | head -3
  fi
done

echo
echo "== 2. a broadcast scenario must actually move messages =="
out=$("$SIM" --config "$SCENARIO_DIR/firmware_broadcast.yaml" --duration 20 2>&1)
rc=$?
sent=$(echo "$out" | sed -n 's/^Total messages sent: //p')
received=$(echo "$out" | sed -n 's/^Total messages received: //p')
if [ $rc -ne 0 ]; then
  fail "firmware_broadcast exited $rc"
elif [ -z "${sent:-}" ] || [ "$sent" -lt 10 ]; then
  fail "firmware_broadcast sent ${sent:-0} messages, expected >= 10"
elif [ -z "${received:-}" ] || [ "$received" -lt "$sent" ]; then
  fail "firmware_broadcast received ${received:-0} for $sent sent -- broadcast is not reaching peers"
else
  pass "sent=$sent received=$received"
fi

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

control_out=$("$SIM" --config "$tmp/partition_control.yaml" 2>&1)
split_out=$("$SIM" --config "$tmp/partition_only.yaml" 2>&1)
heal_out=$("$SIM" --config "$part_scenario" 2>&1)

control_rx=$(received_of "$control_out")
split_rx=$(received_of "$split_out")
cut_links=$(echo "$heal_out" | sed -n 's/.*(\([0-9]*\) mesh link(s) cut).*/\1/p' | head -1)
restored_links=$(echo "$heal_out" | sed -n 's/.*(\([0-9]*\) mesh link(s) restored).*/\1/p' | head -1)

if [ -z "${control_rx:-}" ] || [ "$control_rx" -lt 20 ]; then
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
if [ "$failures" -gt 0 ]; then
  echo "integration check FAILED ($failures problem(s))"
  exit 1
fi
echo "integration check passed"
