# Network Partition Events Documentation

## Overview

Network partition events enable simulation of split-brain scenarios where the mesh network is divided into multiple isolated groups. These events are critical for testing bridge coordination, mesh behavior during network failures, and recovery mechanisms when partitions are healed.

## Event Types

### NetworkPartitionEvent

Partitions the network into multiple isolated groups, creating a split-brain scenario.

**Use Cases:**
- Testing bridge coordination during network splits
- Validating independent partition operation
- Simulating major network failures or geographical isolation
- Testing per-partition bridge election
- Validating mesh behavior under partition conditions

**C++ API:**
```cpp
#include "simulator/events/network_partition_event.hpp"

// Create 2-partition split
std::vector<std::vector<uint32_t>> groups = {
    {1001, 1002, 1003},  // Partition 1
    {1004, 1005, 1006}   // Partition 2
};
auto event = std::make_unique<NetworkPartitionEvent>(groups);
scheduler.scheduleEvent(std::move(event), 30);  // Partition at t=30s
```

**YAML Configuration:**
```yaml
events:
  - time: 30
    action: network_partition
    groups:
      - [bridge-1, sensor-1, sensor-2, sensor-3]
      - [bridge-2, sensor-4, sensor-5, sensor-6]
    description: "Split mesh into 2 isolated partitions"
```

**Behavior:**
- Drops all connections between different partition groups
- Assigns partition IDs to nodes (1-based, 0 = no partition)
- Connections within each partition remain active
- Each partition operates independently
- Messages between partitions are blocked
- Can create 2 or more partitions simultaneously

**Validation:**
- Requires at least 2 partition groups
- Each partition group must contain at least 1 node
- Throws `std::invalid_argument` if validation fails

### NetworkHealEvent

Heals all network partitions, rejoining isolated groups into a unified mesh.

**Use Cases:**
- Testing network recovery after split
- Validating mesh reformation
- Testing global bridge re-election after partition heal
- Simulating network repair or reconnection

**C++ API:**
```cpp
#include "simulator/events/network_heal_event.hpp"

// Heal all partitions
auto event = std::make_unique<NetworkHealEvent>();
scheduler.scheduleEvent(std::move(event), 90);  // Heal at t=90s
```

**YAML Configuration:**
```yaml
events:
  - time: 90
    action: network_heal
    description: "Heal network partitions and reform unified mesh"
```

**Behavior:**
- Restores all previously dropped connections
- Resets partition IDs to 0 (unified network)
- Message delivery resumes between all nodes
- Triggers mesh reformation and route recalculation
- Safe to call when network is not partitioned (idempotent)

## Partition ID Tracking

Nodes track their partition membership via partition IDs:

**C++ API:**
```cpp
#include "simulator/virtual_node.hpp"

// Get node's partition ID
uint32_t partitionId = node->getPartitionId();

// Set partition ID (typically done by partition events)
node->setPartitionId(1);  // Assign to partition 1

// Check if node is partitioned
bool isPartitioned = (node->getPartitionId() != 0);
```

**Partition ID Values:**
- `0`: Node is in unified network (no partition)
- `1, 2, 3, ...`: Node belongs to specific partition

## Complete Example Scenarios

### Two-Partition Split-Brain Test

Simple scenario demonstrating basic partition functionality:

```yaml
simulation:
  name: "Network Partition Test"
  description: "Test 2-partition network split and healing"
  duration: 90
  time_scale: 1.0

nodes:
  # Partition 1 (after split)
  - id: "node-1"
    type: "sensor"
  - id: "node-2"
    type: "sensor"
  - id: "node-3"
    type: "bridge"
  
  # Partition 2 (after split)
  - id: "node-4"
    type: "sensor"
  - id: "node-5"
    type: "sensor"
  - id: "node-6"
    type: "bridge"

topology:
  type: "mesh"

events:
  - time: 0
    action: start_all_nodes
  
  # Create partition at 30s
  - time: 30
    action: network_partition
    groups:
      - [node-1, node-2, node-3]
      - [node-4, node-5, node-6]
    description: "Split network into 2 partitions"
  
  # Heal at 60s
  - time: 60
    action: network_heal
    description: "Rejoin partitions"

metrics:
  output: "results/partition_metrics.csv"
  interval: 3
  collect:
    - "partition_state"
    - "message_count"
    - "connection_state"
```

**Expected Behavior:**
- **0-30s**: Unified mesh, all nodes communicate
- **30-60s**: Two partitions, no inter-partition communication, independent bridge elections
- **60-90s**: Unified mesh restored, global bridge election, normal operation

### Three-Partition Split-Brain Test

Advanced scenario with multiple partitions:

```yaml
simulation:
  name: "Split-Brain Partition Test"
  description: "Test 3-partition split with bridge coordination"
  duration: 120
  time_scale: 1.0

nodes:
  # Group 1
  - id: "bridge-1"
    type: "bridge"
  - id: "sensor-1"
    type: "sensor"
  - id: "sensor-2"
    type: "sensor"
  
  # Group 2
  - id: "bridge-2"
    type: "bridge"
  - id: "sensor-3"
    type: "sensor"
  - id: "sensor-4"
    type: "sensor"
  
  # Group 3
  - id: "bridge-3"
    type: "bridge"
  - id: "sensor-5"
    type: "sensor"
  - id: "sensor-6"
    type: "sensor"

topology:
  type: "mesh"

events:
  - time: 0
    action: start_all_nodes
  
  # Split into 3 partitions at 30s
  - time: 30
    action: network_partition
    groups:
      - [bridge-1, sensor-1, sensor-2]
      - [bridge-2, sensor-3, sensor-4]
      - [bridge-3, sensor-5, sensor-6]
    description: "Create 3-way split"
  
  # Heal at 90s
  - time: 90
    action: network_heal
    description: "Rejoin all partitions"

metrics:
  output: "results/split_brain_metrics.csv"
  interval: 5
  collect:
    - "partition_state"
    - "bridge_elections"
    - "message_delivery"
    - "partition_count"
```

**Expected Behavior:**
- **Phase 1 (0-30s)**: Single unified mesh, one global bridge
- **Phase 2 (30-90s)**: Three independent partitions, three independent bridges
- **Phase 3 (90-120s)**: Unified mesh restored, single global bridge elected

## Network Simulator API

The NetworkSimulator provides partition-related methods:

```cpp
#include "simulator/network_simulator.hpp"

NetworkSimulator network;

// Restore all dropped connections (used by heal event)
network.restoreAllConnections();

// Check connection status
bool active = network.isConnectionActive(1001, 1002);

// Manual partition control (typically via events)
network.dropConnection(1001, 1002);  // Drop specific connection
network.restoreConnection(1001, 1002);  // Restore specific connection
```

## Testing Partition Events

### Unit Tests

```cpp
#include <catch2/catch_test_macros.hpp>
#include "simulator/events/network_partition_event.hpp"
#include "simulator/events/network_heal_event.hpp"

TEST_CASE("Network partition blocks inter-partition messages") {
    NetworkSimulator network;
    boost::asio::io_context io;
    NodeManager manager(io);
    
    // Create nodes
    NodeConfig config1{1001, "TestMesh", "password", 16001};
    NodeConfig config2{1002, "TestMesh", "password", 16002};
    auto node1 = manager.createNode(config1);
    auto node2 = manager.createNode(config2);
    
    // Create partition
    std::vector<std::vector<uint32_t>> groups = {
        {1001},
        {1002}
    };
    NetworkPartitionEvent partition(groups);
    partition.execute(manager, network);
    
    // Verify connection dropped
    REQUIRE_FALSE(network.isConnectionActive(1001, 1002));
    
    // Verify partition IDs assigned
    REQUIRE(node1->getPartitionId() == 1);
    REQUIRE(node2->getPartitionId() == 2);
    
    // Heal partition
    NetworkHealEvent heal;
    heal.execute(manager, network);
    
    // Verify healed
    REQUIRE(network.isConnectionActive(1001, 1002));
    REQUIRE(node1->getPartitionId() == 0);
    REQUIRE(node2->getPartitionId() == 0);
}
```

### Integration Tests

See `test/test_partition_events.cpp` for comprehensive test suite including:
- VirtualNode partition ID tracking
- NetworkSimulator restoreAllConnections
- NetworkPartitionEvent construction and validation
- Multi-partition scenarios (2 and 3 partitions)
- NetworkHealEvent execution
- Partition-heal cycles
- Uneven partition sizes
- Message blocking/delivery verification

## Metrics and Monitoring

Partition events affect the following metrics:

### Per-Node Metrics
- `partition_id` - Current partition membership (0 = unified)
- `partition_count` - Number of active partitions
- `bridge_state` - Whether node is acting as bridge for its partition

### Network-Wide Metrics
- `partition_state` - Map of partition ID to node list
- `partition_count` - Total number of partitions
- `nodes_per_partition` - Distribution of nodes across partitions
- `bridge_elections` - Bridge election events per partition
- `connection_state` - Inter/intra-partition connection states

### Message Metrics
- `message_delivery` - Success/failure by partition
- `dropped_count` - Messages blocked by partition boundaries
- `delivered_count` - Messages within partitions

## Best Practices

### Testing Bridge Coordination

```yaml
# Ensure each partition has a potential bridge node
events:
  - time: 30
    action: network_partition
    groups:
      - [bridge-1, sensor-1, sensor-2]  # Has bridge
      - [bridge-2, sensor-3, sensor-4]  # Has bridge
    description: "Partition with bridges in each group"

# Allow time for bridge elections (10-15s typical)
# Monitor bridge_elections metric

  - time: 90
    action: network_heal
    
# After heal, verify single bridge elected globally
```

### Simulating Geographical Isolation

```yaml
# Split by location
events:
  - time: 20
    action: network_partition
    groups:
      - [site-a-bridge, site-a-sensor-1, site-a-sensor-2]
      - [site-b-bridge, site-b-sensor-1, site-b-sensor-2]
      - [site-c-bridge, site-c-sensor-1, site-c-sensor-2]
    description: "Simulate inter-site network failure"
```

### Testing Partition Resilience

```yaml
# Multiple partition-heal cycles
events:
  # Cycle 1
  - time: 30
    action: network_partition
    groups: [[group1], [group2]]
  - time: 60
    action: network_heal
  
  # Cycle 2
  - time: 90
    action: network_partition
    groups: [[group1], [group2]]
  - time: 120
    action: network_heal
```

### Uneven Partition Sizes

```yaml
# Test with different partition sizes
events:
  - time: 30
    action: network_partition
    groups:
      - [node-1, node-2]                    # 2 nodes
      - [node-3, node-4, node-5, node-6]    # 4 nodes
      - [node-7]                            # 1 node (isolated)
```

## Common Pitfalls

1. **Insufficient Stabilization Time**: Allow 10-15 seconds after partition for each group to detect isolation and elect bridges before testing partition-specific behavior.

2. **Missing Bridge Nodes**: Ensure each partition has at least one potential bridge node. Partitions without bridges may not function correctly.

3. **Expecting Cross-Partition Communication**: Messages between partitions are completely blocked. Use partition IDs to verify isolation.

4. **Forgetting Partition IDs**: After heal, partition IDs are reset to 0. Check this in tests and metrics.

5. **Timing Issues**: Don't immediately heal after partition. Allow time for partition-specific behaviors to be observed (minimum 10-15s recommended).

## Advanced Usage

### Cascading Partitions

```yaml
# Progressive network degradation
events:
  # Initial 2-way split
  - time: 30
    action: network_partition
    groups:
      - [bridge-1, sensor-1, sensor-2, sensor-3]
      - [bridge-2, sensor-4, sensor-5, sensor-6]
  
  # Heal first split
  - time: 60
    action: network_heal
  
  # Then 3-way split
  - time: 90
    action: network_partition
    groups:
      - [bridge-1, sensor-1]
      - [sensor-2, sensor-3]
      - [bridge-2, sensor-4, sensor-5, sensor-6]
```

### Partition with Connection Degradation

```yaml
# Combine partition with quality degradation
events:
  # Create partition
  - time: 30
    action: network_partition
    groups: [[group1], [group2]]
  
  # Degrade connections within partition 1
  - time: 45
    action: connection_degrade
    from: node-1
    to: node-2
    latency: 800
    packet_loss: 0.40
  
  # Heal partition
  - time: 90
    action: network_heal
```

## Differences from Connection Drop

| Feature | NetworkPartitionEvent | ConnectionDropEvent |
|---------|----------------------|---------------------|
| Scope | Drops connections between groups | Drops specific connection |
| Scale | Affects multiple node pairs | Affects one node pair |
| Partition IDs | Assigns partition IDs | No partition tracking |
| Use Case | Split-brain scenarios | Specific link failures |
| Healing | Requires NetworkHealEvent | Use ConnectionRestoreEvent |
| API | `restoreAllConnections()` | `restoreConnection()` |

## Performance Considerations

- Partition events with many nodes may take a few milliseconds to execute
- O(N²) complexity where N is number of nodes in partition groups
- For 100-node meshes, partition creation takes ~10-20ms
- Healing is O(1) - simply clears dropped connection set

## See Also

- [Connection Events Documentation](CONNECTION_EVENTS.md)
- [Event Scheduler Documentation](EVENT_SCHEDULER.md)
- [Network Simulator Documentation](NETWORK_SIMULATOR.md)
- [Example Scenarios](../examples/scenarios/split_brain_partition_test.yaml)
- [API Reference](API_REFERENCE.md)
- [Bridge Coordination Guide](BRIDGE_COORDINATION.md)
