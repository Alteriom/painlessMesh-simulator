# Network Latency Simulation

## Overview

The painlessMesh simulator includes realistic network latency simulation to accurately model real-world mesh network behavior. This feature allows you to configure latency characteristics for the entire network or for specific connections between nodes.

## Features

- **Configurable latency ranges**: Set minimum and maximum latency in milliseconds
- **Multiple distribution types**: Uniform, Normal (Gaussian), and Exponential distributions
- **Per-connection overrides**: Configure specific latency for individual node pairs
- **Statistics tracking**: Collect min/max/avg latency metrics per connection
- **YAML configuration**: Easy-to-use configuration format

## Configuration

### Basic Configuration

Add a `network` section to your YAML configuration file with latency settings:

```yaml
network:
  latency:
    default:
      min: 10       # Minimum latency in milliseconds
      max: 50       # Maximum latency in milliseconds
      distribution: "normal"  # Distribution type
```

### Distribution Types

#### Uniform Distribution

Equal probability for all values in the range [min, max]:

```yaml
latency:
  default:
    min: 10
    max: 50
    distribution: "uniform"
```

**Use case**: Consistent network conditions with equal likelihood of any latency value.

#### Normal Distribution

Bell curve distribution centered around the mean (min + max) / 2:

```yaml
latency:
  default:
    min: 10
    max: 90
    distribution: "normal"
```

**Use case**: Most realistic for typical network conditions where latency clusters around an average value with occasional spikes.

**Note**: Approximately 99.7% of values fall within the [min, max] range (3-sigma rule).

#### Exponential Distribution

Skewed distribution favoring lower latency values:

```yaml
latency:
  default:
    min: 10
    max: 100
    distribution: "exponential"
```

**Use case**: Networks where low latency is common but occasional high-latency spikes occur.

### Per-Connection Configuration

Override default latency for specific node pairs:

```yaml
network:
  latency:
    default:
      min: 10
      max: 50
      distribution: "normal"
    
    specific_connections:
      - from: "sensor-0"
        to: "sensor-1"
        min: 100
        max: 200
        distribution: "uniform"
      
      - from: "node-A"
        to: "node-B"
        min: 5
        max: 15
        distribution: "exponential"
```

**Use cases**:
- Simulating poor wireless connections between distant nodes
- Modeling nodes behind obstacles or interference
- Testing mesh routing around slow links
- Simulating edge nodes with satellite or cellular backhaul

## Implementation Details

### NetworkSimulator Class

The `NetworkSimulator` class manages latency simulation:

```cpp
#include "simulator/network_simulator.hpp"

// Create simulator
NetworkSimulator sim;

// Configure default latency
LatencyConfig config;
config.min_ms = 10;
config.max_ms = 50;
config.distribution = DistributionType::NORMAL;
sim.setDefaultLatency(config);

// Set specific connection latency
LatencyConfig slow_config;
slow_config.min_ms = 100;
slow_config.max_ms = 200;
slow_config.distribution = DistributionType::UNIFORM;
sim.setLatency(nodeId1, nodeId2, slow_config);

// Enqueue a message
uint64_t currentTime = getCurrentTimeMs();
sim.enqueueMessage(fromNode, toNode, message, currentTime);

// Process ready messages
auto ready = sim.getReadyMessages(currentTime);
for (const auto& msg : ready) {
  deliverMessage(msg.from, msg.to, msg.message);
}
```

### Message Queue

Messages are stored in a priority queue ordered by delivery time. When you call `getReadyMessages()`, all messages with `deliveryTime <= currentTime` are returned and removed from the queue.

### Statistics Collection

The simulator tracks statistics for each connection:

```cpp
auto stats = sim.getStats(fromNode, toNode);
std::cout << "Messages: " << stats.message_count << std::endl;
std::cout << "Min latency: " << stats.min_latency_ms << " ms" << std::endl;
std::cout << "Max latency: " << stats.max_latency_ms << " ms" << std::endl;
std::cout << "Avg latency: " << stats.avg_latency_ms << " ms" << std::endl;
```

## Example Scenarios

### Scenario 1: Testing Mesh Stability Under Variable Latency

```yaml
simulation:
  name: "Variable Latency Test"
  duration: 300  # 5 minutes

network:
  latency:
    default:
      min: 20
      max: 80
      distribution: "normal"

nodes:
  - template: "sensor"
    count: 20
    config:
      mesh_prefix: "VariableLatency"
      mesh_password: "testpass"

topology:
  type: "random"
  density: 0.4
```

### Scenario 2: Edge Node with Poor Connectivity

```yaml
network:
  latency:
    default:
      min: 10
      max: 30
      distribution: "normal"
    
    specific_connections:
      # Edge node has poor connection to all others
      - from: "edge-node"
        to: "*"  # All nodes
        min: 200
        max: 500
        distribution: "exponential"
```

Note: Wildcard connections are not yet implemented but planned for future versions.

### Scenario 3: Testing Route Selection

Create a scenario where the mesh must choose between a slow direct route and a faster multi-hop route:

```yaml
network:
  latency:
    default:
      min: 5
      max: 10
      distribution: "uniform"
    
    specific_connections:
      # Direct path is slow
      - from: "node-A"
        to: "node-B"
        min: 500
        max: 1000
        distribution: "uniform"
      
      # Multi-hop through node-C is faster
      # (A -> C and C -> B both use default fast latency)

topology:
  type: "custom"
  connections:
    - ["node-A", "node-B"]  # Direct slow path
    - ["node-A", "node-C"]  # Fast hop 1
    - ["node-C", "node-B"]  # Fast hop 2
```

## Best Practices

### Latency Range Selection

- **LAN/Local**: min=1, max=5 (milliseconds)
- **Local WiFi**: min=5, max=20
- **WiFi Mesh**: min=10, max=50
- **Poor WiFi**: min=50, max=200
- **Cellular**: min=50, max=300
- **Satellite**: min=500, max=1500

### Distribution Selection

- Use **normal** for most realistic simulations
- Use **uniform** for controlled testing with predictable ranges
- Use **exponential** when modeling occasional network congestion

### Testing Strategy

1. **Baseline**: Start with low, consistent latency (uniform 5-10ms)
2. **Realistic**: Use normal distribution with moderate range (10-50ms)
3. **Stress**: Increase range and use exponential distribution (20-200ms)
4. **Edge Cases**: Test with extreme latencies (500-2000ms)

### Performance Considerations

- Latency simulation adds minimal CPU overhead
- Memory usage scales with number of pending messages
- For 100+ nodes, consider limiting message rate
- Statistics collection is optional (disable if not needed)

## Validation

To verify latency is working correctly:

1. Check statistics match configuration:
```cpp
auto stats = sim.getStats(node1, node2);
assert(stats.min_latency_ms >= config.min_ms);
assert(stats.max_latency_ms <= config.max_ms);
```

2. Measure actual delivery times in tests
3. Use deterministic seed for reproducible results:
```cpp
NetworkSimulator sim(12345);  // Fixed seed
```

## Troubleshooting

### Messages not being delivered

- Ensure `getReadyMessages()` is called with increasing time values
- Check that simulation time is advancing (call `updateAll()` regularly)
- Verify latency configuration is valid (min <= max)

### Unexpected latency values

- Remember: distributions can produce edge values at min/max
- Check for configuration errors (ms vs seconds)
- Use fixed seed for debugging: `NetworkSimulator sim(42)`

### Performance issues

- Reduce message rate if queue grows too large
- Consider batch processing messages
- Disable statistics collection if not needed

## Future Enhancements

Planned features:

- **Packet loss simulation**: Drop messages based on configured rate
- **Bandwidth limiting**: Delay messages based on size and bandwidth
- **Network partitions**: Simulate temporary disconnections
- **Latency jitter**: Add variance to latency over time
- **Wildcard connections**: Configure latency for patterns like "edge-* -> *"

## API Reference

### LatencyConfig

```cpp
struct LatencyConfig {
  uint32_t min_ms;              // Minimum latency in milliseconds
  uint32_t max_ms;              // Maximum latency in milliseconds
  DistributionType distribution; // Distribution type
  
  bool isValid() const;         // Returns true if min <= max
};
```

### DistributionType

```cpp
enum class DistributionType {
  UNIFORM,      // Equal probability for all values
  NORMAL,       // Bell curve (Gaussian) distribution
  EXPONENTIAL   // Exponential distribution (skewed to lower values)
};
```

### NetworkSimulator

```cpp
class NetworkSimulator {
public:
  // Construction
  NetworkSimulator();
  NetworkSimulator(uint32_t seed);
  
  // Configuration
  void setDefaultLatency(const LatencyConfig& config);
  void setLatency(uint32_t from, uint32_t to, const LatencyConfig& config);
  LatencyConfig getLatency(uint32_t from, uint32_t to) const;
  
  // Message queueing
  void enqueueMessage(uint32_t from, uint32_t to, 
                     const std::string& message, uint64_t currentTime);
  std::vector<DelayedMessage> getReadyMessages(uint64_t currentTime);
  
  // Queue management
  size_t getPendingMessageCount() const;
  void clear();
  
  // Statistics
  LatencyStats getStats(uint32_t from, uint32_t to) const;
  void resetStats();
};
```

## See Also

- [Configuration Guide](CONFIGURATION_GUIDE.md) - Complete YAML reference
- [Examples](../examples/scenarios/) - Example scenario files
- [API Reference](API_REFERENCE.md) - Complete API documentation
- [Architecture](ARCHITECTURE.md) - System design details
