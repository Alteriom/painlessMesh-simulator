# Connection Events Documentation

## Overview

Connection events provide fine-grained control over network connections between specific node pairs in the painlessMesh simulator. These events are essential for testing mesh routing, failover scenarios, and network resilience.

## Event Types

### ConnectionDropEvent

Drops a bidirectional connection between two nodes, blocking all message delivery.

**Use Cases:**
- Testing mesh routing around failed connections
- Simulating network partitions
- Validating bridge failover scenarios
- Testing route discovery and recovery

**C++ API:**
```cpp
#include "simulator/events/connection_drop_event.hpp"

// Create and schedule event
auto event = std::make_unique<ConnectionDropEvent>(1001, 1002);
scheduler.scheduleEvent(std::move(event), 30);  // Drop at t=30s
```

**YAML Configuration:**
```yaml
events:
  - time: 30
    action: connection_drop
    from: node-1
    to: node-2
    description: "Drop connection between node-1 and node-2"
```

**Behavior:**
- Drops connection in both directions (node-1 → node-2 and node-2 → node-1)
- Messages on dropped connections are silently dropped
- Dropped packets are recorded in network statistics
- Connection can be restored with ConnectionRestoreEvent

### ConnectionRestoreEvent

Restores a previously dropped connection between two nodes.

**Use Cases:**
- Testing network recovery after failures
- Validating route optimization after restoration
- Simulating intermittent connectivity
- Testing mesh self-healing

**C++ API:**
```cpp
#include "simulator/events/connection_restore_event.hpp"

// Create and schedule event
auto event = std::make_unique<ConnectionRestoreEvent>(1001, 1002);
scheduler.scheduleEvent(std::move(event), 60);  // Restore at t=60s
```

**YAML Configuration:**
```yaml
events:
  - time: 60
    action: connection_restore
    from: node-1
    to: node-2
    description: "Restore connection between node-1 and node-2"
```

**Behavior:**
- Restores connection in both directions
- Message delivery resumes immediately
- Safe to call on already-active connections (idempotent)
- Previous latency and packet loss settings are preserved

### ConnectionDegradeEvent

Degrades connection quality by increasing latency and packet loss.

**Use Cases:**
- Testing performance under poor network conditions
- Simulating wireless interference or congestion
- Validating adaptive routing algorithms
- Testing quality-of-service (QoS) mechanisms

**C++ API:**
```cpp
#include "simulator/events/connection_degrade_event.hpp"

// Create with default parameters (500ms latency, 30% loss)
auto event1 = std::make_unique<ConnectionDegradeEvent>(1001, 1002);

// Create with custom parameters
auto event2 = std::make_unique<ConnectionDegradeEvent>(
    1001, 1002,
    1000,   // 1000ms latency
    0.50f   // 50% packet loss
);

scheduler.scheduleEvent(std::move(event2), 45);
```

**YAML Configuration:**
```yaml
events:
  # Using default parameters
  - time: 45
    action: connection_degrade
    from: node-3
    to: node-4
  
  # Using custom parameters
  - time: 50
    action: connection_degrade
    from: node-1
    to: node-2
    latency: 1000        # milliseconds
    packet_loss: 0.50    # 0.0 to 1.0 (50%)
```

**Default Parameters:**
- Latency: 500ms (min) to 1000ms (max)
- Packet Loss: 30%

**Behavior:**
- Sets latency range (min to 2×min) with uniform distribution
- Sets packet loss probability
- Affects connection in both directions
- Can be applied to already-degraded connections (overwrites previous settings)

## Complete Example Scenario

Here's a comprehensive example demonstrating all connection events:

```yaml
simulation:
  name: "Connection Events Test"
  description: "Comprehensive connection control demonstration"
  duration: 120
  time_scale: 1.0

nodes:
  - id: "sensor-1"
    type: "sensor"
    mesh_prefix: "TestMesh"
    mesh_password: "test123"
  
  - id: "sensor-2"
    type: "sensor"
    mesh_prefix: "TestMesh"
    mesh_password: "test123"
  
  - id: "bridge-1"
    type: "bridge"
    mesh_prefix: "TestMesh"
    mesh_password: "test123"
  
  - id: "sensor-3"
    type: "sensor"
    mesh_prefix: "TestMesh"
    mesh_password: "test123"

topology:
  type: "mesh"  # All nodes connected

events:
  # Phase 1: Drop primary connection (0-40s)
  - time: 20
    action: connection_drop
    from: sensor-1
    to: sensor-2
    description: "Simulate link failure"
  
  # Phase 2: Degrade alternate route (40-60s)
  - time: 40
    action: connection_degrade
    from: sensor-2
    to: bridge-1
    latency: 800
    packet_loss: 0.40
    description: "Degrade alternate path quality"
  
  # Phase 3: Restore primary connection (60-80s)
  - time: 60
    action: connection_restore
    from: sensor-1
    to: sensor-2
    description: "Primary link restored"
  
  # Phase 4: Drop bridge connection (80-100s)
  - time: 80
    action: connection_drop
    from: bridge-1
    to: sensor-3
    description: "Isolate sensor-3"
  
  # Phase 5: Full recovery (100-120s)
  - time: 100
    action: connection_restore
    from: bridge-1
    to: sensor-3
    description: "Full mesh restored"

metrics:
  output: "results/connection_test.csv"
  interval: 5
  collect:
    - "connection_state"
    - "message_count"
    - "packet_loss"
    - "latency"
```

## Network Simulator API

The NetworkSimulator class provides the underlying connection control methods:

```cpp
#include "simulator/network_simulator.hpp"

NetworkSimulator network;

// Drop a connection
network.dropConnection(1001, 1002);

// Check connection status
bool active = network.isConnectionActive(1001, 1002);

// Restore a connection
network.restoreConnection(1001, 1002);

// Messages on dropped connections are automatically dropped
network.enqueueMessage(1001, 1002, "message", currentTime);
// If connection is dropped, message is silently dropped

// Get statistics including dropped packet count
auto stats = network.getStats(1001, 1002);
std::cout << "Dropped: " << stats.dropped_count << std::endl;
std::cout << "Delivered: " << stats.delivered_count << std::endl;
```

## Testing Connection Events

### Unit Tests

```cpp
#include <catch2/catch_test_macros.hpp>
#include "simulator/events/connection_drop_event.hpp"

TEST_CASE("Connection drop blocks messages") {
    NetworkSimulator network;
    boost::asio::io_context io;
    NodeManager manager(io);
    
    // Drop connection
    ConnectionDropEvent event(1001, 1002);
    event.execute(manager, network);
    
    // Verify connection is dropped
    REQUIRE_FALSE(network.isConnectionActive(1001, 1002));
    
    // Try to send message
    network.enqueueMessage(1001, 1002, "test", 1000);
    REQUIRE(network.getPendingMessageCount() == 0);  // Blocked
}
```

### Integration Tests

See `test/test_connection_events.cpp` for comprehensive test suite including:
- Connection drop and restore cycles
- Message blocking verification
- Statistics validation
- Quality degradation effects
- Multi-event sequences

## Metrics and Monitoring

Connection events affect the following metrics:

### Per-Connection Statistics
- `dropped_count` - Number of packets dropped (includes both packet loss and connection drops)
- `delivered_count` - Number of packets successfully delivered
- `drop_rate` - Packet drop rate (0.0 to 1.0)
- `min_latency_ms`, `max_latency_ms`, `avg_latency_ms` - Latency statistics

### Network-Wide Metrics
- `connection_state` - Active/dropped state of each connection
- `message_count` - Total messages sent/received
- `route_changes` - Number of route changes due to connection events

## Best Practices

### Testing Routing
```yaml
# Drop primary connection, verify alternate routing
- time: 30
  action: connection_drop
  from: node-1
  to: node-2

# Allow time for mesh to reroute (10-15s typical)
# Then verify messages still reach destination

- time: 50
  action: connection_restore
  from: node-1
  to: node-2
```

### Simulating Intermittent Connectivity
```yaml
# Pattern: drop, wait, restore, repeat
- time: 20
  action: connection_drop
  from: node-1
  to: node-2

- time: 40
  action: connection_restore
  from: node-1
  to: node-2

- time: 60
  action: connection_drop
  from: node-1
  to: node-2

- time: 80
  action: connection_restore
  from: node-1
  to: node-2
```

### Testing Failover
```yaml
# Drop primary bridge
- time: 30
  action: connection_drop
  from: primary-bridge
  to: sensor-group-1

# Verify traffic moves to backup bridge
# Monitor metrics for route_changes

# Restore primary bridge
- time: 90
  action: connection_restore
  from: primary-bridge
  to: sensor-group-1
```

## Common Pitfalls

1. **Forgetting Bidirectional Nature**: Connection events affect both directions automatically. You don't need to specify both directions.

2. **Connection State vs. Quality**: Connection drop completely blocks traffic. Connection degrade only affects quality (latency/loss) but messages still flow.

3. **Statistics Interpretation**: `dropped_count` includes both connection drops and packet loss drops. Check `isConnectionActive()` to distinguish.

4. **Restore Timing**: Allow sufficient time for mesh to detect and react to connection changes before restoring (typically 10-15 seconds).

## See Also

- [Event Scheduler Documentation](EVENT_SCHEDULER.md)
- [Network Simulator Documentation](NETWORK_SIMULATOR.md)
- [Example Scenarios](../examples/scenarios/)
- [API Reference](API_REFERENCE.md)
