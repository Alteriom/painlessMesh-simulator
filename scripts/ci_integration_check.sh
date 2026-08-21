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
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
sed 's|firmware: "SimpleBroadcast"|firmware: "NoSuchFirmware"|' \
  "$SCENARIO_DIR/simple_mesh.yaml" > "$tmp/bad_firmware.yaml"
if "$SIM" --config "$tmp/bad_firmware.yaml" --duration 5 >/dev/null 2>&1; then
  fail "a scenario with unloadable firmware exited 0"
else
  pass "unloadable firmware fails the run"
fi

echo
if [ "$failures" -gt 0 ]; then
  echo "integration check FAILED ($failures problem(s))"
  exit 1
fi
echo "integration check passed"
