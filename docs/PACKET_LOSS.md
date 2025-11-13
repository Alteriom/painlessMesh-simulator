# Packet Loss Simulation

The painlessMesh simulator supports configurable packet loss simulation to model unreliable network conditions and test mesh resilience.

## Overview

Packet loss simulation allows you to:
- Test mesh network resilience under poor network conditions
- Validate message retry and acknowledgment mechanisms
- Simulate real-world RF interference and congestion
- Test different packet loss patterns (random vs. burst)

## Features

### Random Packet Loss
Each packet is independently dropped based on a configured probability.

### Burst Mode Packet Loss
Packets are dropped in consecutive bursts, simulating RF interference or temporary connection loss.

### Per-Connection Configuration
Different packet loss rates can be configured for specific node pairs, allowing fine-grained control over network quality.

## Configuration

### YAML Configuration Format

#### Basic Configuration (Legacy)
```yaml
network:
  packet_loss: 0.10  # 10% packet loss for all connections
```

#### Advanced Configuration
```yaml
network:
  packet_loss:
    default:
      probability: 0.05   # 5% packet loss by default
      burst_mode: false   # Random independent drops
    
    specific_connections:
      # Poor connection with random drops
      - from: "sensor-0"
        to: "sensor-1"
        probability: 0.20   # 20% packet loss
        burst_mode: false
      
      # Very poor connection with burst losses
      - from: "sensor-2"
        to: "sensor-3"
        probability: 0.30   # 30% packet loss
        burst_mode: true
        burst_length: 5     # Drop 5 consecutive packets per burst
```

### Configuration Parameters

#### `probability`
- **Type**: Float (0.0 to 1.0)
- **Description**: Probability of packet loss
- **Examples**:
  - `0.0` = No packet loss
  - `0.10` = 10% packet loss
  - `0.50` = 50% packet loss
  - `1.0` = 100% packet loss (all packets dropped)

#### `burst_mode`
- **Type**: Boolean
- **Default**: `false`
- **Description**: Enable burst mode packet loss
- **When `false`**: Each packet is independently dropped based on probability
- **When `true`**: Packets are dropped in bursts of length `burst_length`

#### `burst_length`
- **Type**: Integer (> 0)
- **Default**: `3`
- **Description**: Number of consecutive packets to drop per burst
- **Only applies when**: `burst_mode` is `true`

## Programmatic API

### C++ API

#### Setting Default Packet Loss
```cpp
#include "simulator/network_simulator.hpp"

NetworkSimulator sim;

// Configure default packet loss
PacketLossConfig config;
config.probability = 0.10f;  // 10% loss
config.burst_mode = false;
sim.setDefaultPacketLoss(config);
```

#### Setting Per-Connection Packet Loss
```cpp
// Configure specific connection
PacketLossConfig poor_connection;
poor_connection.probability = 0.30f;  // 30% loss
poor_connection.burst_mode = true;
poor_connection.burst_length = 5;

sim.setPacketLoss(node1_id, node2_id, poor_connection);
```

#### Retrieving Configuration
```cpp
PacketLossConfig config = sim.getPacketLoss(node1_id, node2_id);
std::cout << "Packet loss probability: " << config.probability << std::endl;
```

#### Checking Statistics
```cpp
auto stats = sim.getStats(node1_id, node2_id);
std::cout << "Dropped packets: " << stats.dropped_count << std::endl;
std::cout << "Delivered packets: " << stats.delivered_count << std::endl;
std::cout << "Drop rate: " << stats.drop_rate << std::endl;
```

## Examples

### Example 1: Testing Mesh Formation with Packet Loss

Test that a mesh network can still form with 20% packet loss:

```yaml
simulation:
  name: "Mesh Formation with Packet Loss"
  duration: 60

network:
  packet_loss:
    default:
      probability: 0.20
      burst_mode: false
  
  latency:
    default:
      min: 10
      max: 50
      distribution: "normal"

nodes:
  - template: "sensor"
    count: 10
    id_prefix: "sensor-"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"

topology:
  type: "random"
  density: 0.4
```

### Example 2: Simulating Poor RF Environment

Simulate a challenging RF environment with burst packet loss:

```yaml
network:
  packet_loss:
    default:
      probability: 0.10
      burst_mode: true
      burst_length: 3
    
    specific_connections:
      # Simulate a node far from the mesh with very poor connection
      - from: "sensor-9"
        to: "sensor-0"
        probability: 0.40
        burst_mode: true
        burst_length: 10
      
      - from: "sensor-0"
        to: "sensor-9"
        probability: 0.40
        burst_mode: true
        burst_length: 10
```

### Example 3: Testing Message Retry Logic

Test application-level message retry with moderate packet loss:

```yaml
network:
  packet_loss:
    default:
      probability: 0.15   # 15% loss
      burst_mode: false
  
  latency:
    default:
      min: 50
      max: 150
      distribution: "exponential"

# Your application should implement retry logic to handle lost packets
```

## Statistics

The simulator tracks packet loss statistics per connection:

- **`dropped_count`**: Number of packets dropped
- **`delivered_count`**: Number of packets successfully delivered
- **`drop_rate`**: Calculated drop rate (dropped / (dropped + delivered))

### Accessing Statistics

Statistics are available through the `NetworkSimulator::getStats()` method:

```cpp
auto stats = sim.getStats(from_node, to_node);
std::cout << "Connection " << from_node << " -> " << to_node << ":" << std::endl;
std::cout << "  Dropped: " << stats.dropped_count << std::endl;
std::cout << "  Delivered: " << stats.delivered_count << std::endl;
std::cout << "  Drop rate: " << (stats.drop_rate * 100.0f) << "%" << std::endl;
```

## Best Practices

### Testing Strategy

1. **Start with low packet loss (5-10%)** to verify basic resilience
2. **Gradually increase** to test worst-case scenarios (20-30%)
3. **Test burst mode** to simulate real-world RF interference
4. **Combine with latency** for realistic network conditions
5. **Validate mesh convergence** even with high packet loss

### Realistic Packet Loss Rates

- **Excellent RF environment**: 0-2% packet loss
- **Good RF environment**: 2-5% packet loss
- **Fair RF environment**: 5-10% packet loss
- **Poor RF environment**: 10-20% packet loss
- **Very poor RF environment**: 20-40% packet loss
- **Extreme conditions**: 40%+ packet loss

### Burst Length Guidelines

- **Short bursts (3-5 packets)**: Momentary interference
- **Medium bursts (5-10 packets)**: Brief signal loss
- **Long bursts (10+ packets)**: Extended connection issues

## Implementation Details

### How It Works

1. When a message is enqueued, the simulator calls `shouldDropPacket(from, to)`
2. For random mode, a random number is generated and compared to the probability
3. For burst mode, the simulator tracks burst state per connection:
   - When not in a burst, probability determines if a burst starts
   - When in a burst, all packets are dropped until the burst completes
4. Dropped packets are not added to the message queue
5. Statistics are updated for both dropped and delivered packets

### Performance Considerations

Packet loss simulation adds minimal overhead:
- Random number generation per packet (when loss configured)
- Burst state tracking per connection (only in burst mode)
- Statistics updates per packet attempt

## Troubleshooting

### Mesh Not Forming with High Packet Loss

**Symptom**: Nodes fail to discover each other with >30% packet loss

**Solution**: This is expected behavior. Mesh protocols require some minimum reliability. Try:
- Reducing packet loss rate
- Implementing application-level retry logic
- Increasing message send frequency

### Inconsistent Drop Rates

**Symptom**: Observed drop rate doesn't match configured probability

**Cause**: With small sample sizes, random variation is normal

**Solution**: 
- Send more messages (1000+) for accurate statistics
- Allow ±5-10% margin of error in tests
- Use fixed seed for reproducible testing

### Burst Mode More Aggressive Than Expected

**Symptom**: Burst mode causes higher than expected drop rate

**Cause**: Back-to-back bursts can occur when probability triggers again immediately

**Explanation**: This is correct behavior - burst mode with 50% probability means:
- 50% chance to start a burst
- Once burst starts, all packets in burst are dropped
- After burst ends, immediately check probability again

**Solution**: Use lower probabilities with burst mode (20-30% instead of 50%)

## See Also

- [Network Latency Simulation](LATENCY.md)
- [Scenario Configuration](SCENARIO_CONFIG.md)
- [Metrics Collection](METRICS.md)
- [Integration Testing](TESTING.md)
