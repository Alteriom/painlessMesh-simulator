# Issue #138 Test Scenarios

This directory contains comprehensive test scenarios designed to validate issue #138 fixes in painlessMesh. These scenarios specifically test network partition behavior, bridge re-election, and mesh recovery.

## Overview

Issue #138 likely relates to split-brain scenarios in mesh networks where:
- Multiple bridges remain active after partition healing
- Nodes fail to properly reconnect after network partition
- Routing tables become corrupted or stale
- Bridge election fails or produces inconsistent results

## Test Scenarios

### 1. Rapid Partition Cycles (`issue_138_rapid_partition_cycles.yaml`)

**Purpose**: Tests mesh behavior under multiple rapid partition/heal events

**Key Features**:
- 4 complete partition/heal cycles in 3 minutes
- 6 nodes split into 2 groups repeatedly
- Validates state doesn't accumulate corruption
- Tests bridge re-election resilience

**Critical Test**: After each heal, exactly 1 bridge should be active

**Duration**: 180 seconds (3 minutes)

**Run**:
```bash
./bin/painlessmesh-simulator --config examples/scenarios/issue_138_rapid_partition_cycles.yaml
```

**Expected Results**:
- ✓ active_bridge_count = 1 after each healing (4 times)
- ✓ No state corruption after multiple cycles
- ✓ Stable mesh in final 45 seconds

**Failure Indicators**:
- ✗ Multiple bridges remain after healing
- ✗ Mesh fails to rejoin after 2nd or 3rd cycle
- ✗ Performance degrades over time

---

### 2. Message Routing Validation (`issue_138_message_routing.yaml`)

**Purpose**: Explicitly tests message delivery before, during, and after partition

**Key Features**:
- Injects test messages at each phase
- Validates intra-partition vs cross-partition routing
- Tests message delivery across previously partitioned groups
- Comprehensive routing validation

**Critical Test**: Messages between previously partitioned nodes must work after healing

**Duration**: 150 seconds (2.5 minutes)

**Run**:
```bash
./bin/painlessmesh-simulator --config examples/scenarios/issue_138_message_routing.yaml
```

**Expected Results**:
- ✓ 100% delivery rate in unified mesh (phase 1 and 6)
- ✓ Intra-partition messages succeed during partition
- ✓ Cross-partition messages properly blocked during partition
- ✓ All cross-partition messages work after healing

**Failure Indicators**:
- ✗ Messages fail between previously partitioned nodes after healing
- ✗ Routing tables not updated after healing
- ✗ Some nodes remain "unreachable"

---

### 3. Uneven Partitions (`issue_138_uneven_partitions.yaml`)

**Purpose**: Tests edge cases with very uneven partition distribution

**Key Features**:
- 10 nodes split unevenly: 7 + 2 + 1 nodes
- Tests single-node isolation (worst case)
- Validates minority partition behavior
- Tests complete mesh reformation from uneven split

**Critical Test**: Single isolated node must successfully rejoin

**Duration**: 120 seconds (2 minutes)

**Run**:
```bash
./bin/painlessmesh-simulator --config examples/scenarios/issue_138_uneven_partitions.yaml
```

**Expected Results**:
- ✓ All partitions operate correctly (even single-node)
- ✓ Isolated node successfully rejoins
- ✓ Small partition (2 nodes) rejoins correctly
- ✓ Single bridge after healing

**Failure Indicators**:
- ✗ Isolated node fails to rejoin
- ✗ Small partition remains separate
- ✗ Multiple bridges after healing

---

### 4. Stress Test with Partitioning (`issue_138_stress_partition.yaml`)

**Purpose**: Tests partition behavior with large number of nodes (50)

**Key Features**:
- 50 nodes split into 3 partitions
- Tests scalability of bridge election
- Validates performance under partition load
- Runs at 2x speed for efficiency

**Critical Test**: Bridge election must scale to 50 nodes and complete quickly

**Duration**: 180 seconds (3 minutes at 2x speed = 1.5 minutes real time)

**Run**:
```bash
./bin/painlessmesh-simulator --config examples/scenarios/issue_138_stress_partition.yaml
```

**Expected Results**:
- ✓ 50 nodes form unified mesh
- ✓ Clean 3-way partition (17+17+16)
- ✓ Each partition elects bridge
- ✓ All 50 nodes rejoin after healing
- ✓ Single bridge in final mesh
- ✓ Convergence within 30 seconds

**Failure Indicators**:
- ✗ Bridge election timeout with many nodes
- ✗ Incomplete mesh reformation
- ✗ Multiple bridges remain active
- ✗ Performance degradation

---

### 5. Cascade Healing (`issue_138_cascade_healing.yaml`)

**Purpose**: Tests progressive partition healing (one by one)

**Key Features**:
- 3 partitions created, then healed progressively
- First heal: merge A+B (C remains isolated)
- Second heal: merge (A+B)+C
- Tests stepwise bridge re-election

**Critical Test**: Bridge re-election at each merge step

**Duration**: 180 seconds (3 minutes)

**Run**:
```bash
./bin/painlessmesh-simulator --config examples/scenarios/issue_138_cascade_healing.yaml
```

**Expected Results**:
- ✓ 3-way partition clean (4+4+4 nodes)
- ✓ A+B merge produces 1 bridge for 8-node mesh
- ✓ partition_count = 2 after first heal
- ✓ Final merge produces 1 bridge for 12-node mesh
- ✓ partition_count = 1 after final heal
- ✓ All nodes reachable at each stage

**Failure Indicators**:
- ✗ Multiple bridges after A+B merge
- ✗ Multiple bridges after final merge
- ✗ Routing fails between progressively merged groups
- ✗ Bridge election conflicts

---

## Running All Scenarios

Run all issue #138 scenarios in sequence:

```bash
#!/bin/bash
# Run all issue #138 validation scenarios

SCENARIOS=(
  "issue_138_rapid_partition_cycles.yaml"
  "issue_138_message_routing.yaml"
  "issue_138_uneven_partitions.yaml"
  "issue_138_stress_partition.yaml"
  "issue_138_cascade_healing.yaml"
)

for scenario in "${SCENARIOS[@]}"; do
  echo "========================================="
  echo "Running: $scenario"
  echo "========================================="
  ./bin/painlessmesh-simulator \
    --config "examples/scenarios/$scenario" \
    --log-level INFO \
    --output "results/issue_138/"
  
  if [ $? -eq 0 ]; then
    echo "✓ $scenario PASSED"
  else
    echo "✗ $scenario FAILED"
  fi
  echo ""
done

echo "All scenarios complete. Check results/ for output."
```

Save this as `scripts/run_issue_138_tests.sh` and make executable:
```bash
chmod +x scripts/run_issue_138_tests.sh
./scripts/run_issue_138_tests.sh
```

## Analyzing Results

### Critical Metrics to Check

For each scenario, verify:

1. **`active_bridge_count`**: Should be exactly 1 after each healing event
2. **`partition_count`**: Should match expected state at each phase
3. **`message_delivery_rate`**: Should be 100% in unified mesh
4. **`connection_count`**: Should recover after healing
5. **`unreachable_nodes`**: Should be 0 in unified mesh

### Using Metrics CSV

```bash
# Check for multiple bridges (should output "1" for all healed phases)
grep "healed" results/issue_138_rapid_cycles.csv | cut -d',' -f5

# Check message delivery rate (should be 100% or 1.0)
grep "healed" results/issue_138_message_routing.csv | cut -d',' -f6

# Check isolated node count (should be 0 after healing)
grep "healed" results/issue_138_uneven_partitions.csv | cut -d',' -f7
```

### Python Analysis Script

```python
#!/usr/bin/env python3
import pandas as pd
import sys

def analyze_scenario(csv_file):
    df = pd.read_csv(csv_file)
    
    # Check for multiple bridges after healing
    healed_phases = df[df['phase'] == 'healed']
    multi_bridge = healed_phases[healed_phases['active_bridge_count'] > 1]
    
    if len(multi_bridge) > 0:
        print(f"✗ FAIL: Multiple bridges detected after healing")
        print(multi_bridge[['time', 'active_bridge_count', 'bridge_identity']])
        return False
    
    # Check message delivery in unified mesh
    unified_phases = df[df['partition_count'] == 1]
    low_delivery = unified_phases[unified_phases['message_delivery_rate'] < 0.95]
    
    if len(low_delivery) > 0:
        print(f"✗ FAIL: Low message delivery rate in unified mesh")
        print(low_delivery[['time', 'message_delivery_rate']])
        return False
    
    print(f"✓ PASS: All checks passed")
    return True

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python analyze_issue_138.py <results.csv>")
        sys.exit(1)
    
    success = analyze_scenario(sys.argv[1])
    sys.exit(0 if success else 1)
```

## Expected Failures (If Issue #138 Exists)

If issue #138 is present in painlessMesh, one or more scenarios will exhibit:

### Symptom 1: Multiple Bridges After Healing
```
Time: 120s, Phase: healed, active_bridge_count: 2
bridge-1 claims leadership
bridge-2 also claims leadership
```

### Symptom 2: Failed Message Routing
```
Time: 105s, Phase: healed
Message from node-1 to node-6: FAILED (unreachable)
Routing table shows node-6 as unreachable
```

### Symptom 3: Isolated Nodes After Healing
```
Time: 85s, Phase: healed, isolated_node_count: 1
sensor-isolated unable to rejoin mesh
Connection attempts timeout
```

### Symptom 4: Bridge Election Timeout
```
Time: 125s, Phase: healed
Bridge election timeout after 30 seconds
Multiple candidates, no winner elected
Mesh unstable
```

## Success Criteria Summary

All scenarios should show:

- ✓ Exactly 1 active bridge after each healing event
- ✓ partition_count matches expected state
- ✓ 100% message delivery in unified mesh
- ✓ All nodes reachable (unreachable_nodes = 0)
- ✓ Stable topology (no flapping)
- ✓ Bridge election completes within 15 seconds
- ✓ No orphaned or isolated nodes after healing
- ✓ Consistent topology view across all nodes

If any scenario fails these criteria, issue #138 is likely present.

## CI/CD Integration

Add to `.github/workflows/ci.yml`:

```yaml
- name: Run Issue #138 Validation Tests
  run: |
    ./scripts/run_issue_138_tests.sh
    if [ $? -ne 0 ]; then
      echo "Issue #138 validation failed"
      exit 1
    fi
```

## Documentation References

- [Issue #138 Analysis](../../docs/ISSUE_138_ANALYSIS.md) - Detailed analysis
- [Partition Events](../../docs/PARTITION_EVENTS.md) - Event API documentation
- [Split-Brain Test](split_brain_partition_test.yaml) - Original scenario
- [Network Partition Test](network_partition_test.yaml) - Simple scenario

## Version History

- **v1.0** (2025-11-14): Initial scenario set created
  - 5 comprehensive scenarios
  - Coverage for rapid cycles, message routing, uneven partitions, stress test, cascade healing

---

**Status**: ✅ Ready for Testing  
**Coverage**: Comprehensive split-brain and partition recovery validation  
**Test Count**: 5 scenarios covering all major edge cases
