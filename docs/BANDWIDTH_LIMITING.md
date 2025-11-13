# Bandwidth Limiting in painlessMesh Simulator

## Overview

The painlessMesh simulator now supports bandwidth limiting to simulate realistic network conditions with constrained throughput. This feature uses a token bucket algorithm to enforce both byte-based and message-based rate limits on network connections.

## Features

- **Configurable bandwidth limits**: Set maximum bytes/sec and messages/sec per connection
- **Token bucket algorithm**: Fair rate limiting with burst handling
- **Per-connection and default settings**: Override bandwidth for specific node pairs
- **Bandwidth utilization metrics**: Track actual usage vs configured limits
- **YAML configuration support**: Easy scenario configuration

## Use Cases

Bandwidth limiting is essential for:
- **Testing low-bandwidth scenarios**: Simulate IoT devices on limited connections (2G, NB-IoT, LoRaWAN)
- **Mesh performance analysis**: Understand how the mesh handles congestion
- **Protocol optimization**: Identify opportunities to reduce bandwidth usage
- **Real-world simulation**: Match actual hardware constraints

## Configuration

### YAML Configuration Format

```yaml
network:
  bandwidth:
    default:
      max_bytes_per_sec: 10000      # 10 KB/s maximum throughput
      max_messages_per_sec: 100     # 100 messages/sec limit
      bucket_size: 2000             # Token bucket size for bursts
    
    specific_connections:
      # Very slow connection (1 KB/s)
      - from: "sensor-0"
        to: "sensor-1"
        max_bytes_per_sec: 1000
        max_messages_per_sec: 10
        bucket_size: 500
      
      # Fast connection (50 KB/s)
      - from: "sensor-1"
        to: "sensor-2"
        max_bytes_per_sec: 50000
        max_messages_per_sec: 500
        bucket_size: 5000
      
      # Message-limited only (no byte limit)
      - from: "sensor-2"
        to: "sensor-3"
        max_messages_per_sec: 20
        bucket_size: 40
```

### Configuration Parameters

#### BandwidthConfig

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `max_bytes_per_sec` | uint32_t | 0 (unlimited) | Maximum bytes per second |
| `max_messages_per_sec` | uint32_t | 0 (unlimited) | Maximum messages per second |
| `bucket_size` | uint32_t | 1000 | Token bucket capacity for bursts |

**Note**: Setting both limits to 0 disables bandwidth limiting for that connection.

### Programmatic Configuration

```cpp
#include "simulator/network_simulator.hpp"

using namespace simulator;

// Create simulator
NetworkSimulator sim;

// Set default bandwidth
BandwidthConfig default_bw;
default_bw.max_bytes_per_sec = 10000;    // 10 KB/s
default_bw.max_messages_per_sec = 100;   // 100 msg/s
default_bw.bucket_size = 2000;           // 2000 tokens
sim.setDefaultBandwidth(default_bw);

// Set connection-specific bandwidth
BandwidthConfig slow_link;
slow_link.max_bytes_per_sec = 1000;      // 1 KB/s
slow_link.bucket_size = 500;
sim.setBandwidth(nodeId1, nodeId2, slow_link);
```

## Token Bucket Algorithm

### How It Works

The token bucket algorithm provides smooth rate limiting with burst support:

1. **Bucket Initialization**: Each connection gets a bucket with `bucket_size` tokens
2. **Token Consumption**: 
   - Sending a message consumes tokens (bytes for byte limit, 1 for message limit)
   - If insufficient tokens available, message is dropped (throttled)
3. **Token Refill**: 
   - Tokens refill over time at the configured rate
   - Refill rate: `max_bytes_per_sec` or `max_messages_per_sec`
   - Bucket never exceeds `bucket_size` (prevents unbounded accumulation)

### Example Timeline

With configuration:
- `max_bytes_per_sec = 1000` (1 KB/s)
- `bucket_size = 1000`

| Time (ms) | Action | Available Tokens | Result |
|-----------|--------|------------------|--------|
| 0 | Initialize | 1000 | - |
| 0 | Send 500 bytes | 500 | ✓ Accepted |
| 0 | Send 500 bytes | 0 | ✓ Accepted |
| 0 | Send 500 bytes | 0 | ✗ Throttled (insufficient tokens) |
| 1000 | Refill (+1000) | 1000 | - |
| 1000 | Send 900 bytes | 100 | ✓ Accepted |

### Burst Handling

The `bucket_size` parameter allows controlled bursts:
- **Small bucket**: Smooth, consistent rate (e.g., `bucket_size = max_bytes_per_sec`)
- **Large bucket**: Allow larger bursts (e.g., `bucket_size = 2 * max_bytes_per_sec`)

## Metrics and Statistics

### Available Metrics

The simulator tracks bandwidth-related metrics for each connection:

```cpp
auto stats = sim.getStats(fromNode, toNode);

// Bandwidth metrics
uint64_t bytes_sent = stats.bytes_sent;                  // Total bytes sent
uint64_t throttled = stats.bandwidth_throttled;          // Messages dropped due to bandwidth
float utilization = stats.bandwidth_utilization;         // Utilization ratio (0.0-1.0)
```

### Metric Descriptions

| Metric | Type | Description |
|--------|------|-------------|
| `bytes_sent` | uint64_t | Total bytes successfully sent on this connection |
| `bandwidth_throttled` | uint64_t | Number of messages dropped due to bandwidth limits |
| `bandwidth_utilization` | float | Bandwidth usage as fraction of configured limit (0.0 = unused, 1.0 = fully utilized) |

### Exporting Metrics

Configure metric collection in your YAML scenario:

```yaml
metrics:
  output: "results/bandwidth_test.csv"
  interval: 5  # Collect every 5 seconds
  collect:
    - "bytes_sent"
    - "bandwidth_throttled"
    - "bandwidth_utilization"
    - "messages_sent"
    - "messages_received"
```

## Example Scenarios

### Scenario 1: IoT Sensor Network (Low Bandwidth)

Simulate battery-powered sensors on constrained connections:

```yaml
network:
  bandwidth:
    default:
      max_bytes_per_sec: 2000       # 2 KB/s (typical for NB-IoT)
      max_messages_per_sec: 20      # 20 messages/sec
      bucket_size: 1000
```

**Use case**: Test mesh routing efficiency under severe bandwidth constraints

### Scenario 2: Mixed Bandwidth Network

Different link qualities in the same network:

```yaml
network:
  bandwidth:
    default:
      max_bytes_per_sec: 10000      # 10 KB/s default
      bucket_size: 2000
    
    specific_connections:
      # Gateway with high bandwidth
      - from: "gateway"
        to: "*"  # All connections from gateway
        max_bytes_per_sec: 100000   # 100 KB/s
      
      # Edge sensor with very low bandwidth
      - from: "edge-sensor-1"
        to: "*"
        max_bytes_per_sec: 500      # 500 bytes/sec
```

**Use case**: Realistic heterogeneous network simulation

### Scenario 3: Message-Rate Limited

Limit by message count, not bytes (useful for testing message overhead):

```yaml
network:
  bandwidth:
    default:
      max_messages_per_sec: 10      # Only 10 messages/sec
      bucket_size: 20               # Allow small bursts
```

**Use case**: Test protocol behavior when message frequency is limited

## Best Practices

### Configuration Guidelines

1. **Bucket Size Selection**:
   - Set to `max_bytes_per_sec` for smooth rate limiting
   - Set to `2-3x max_bytes_per_sec` to allow reasonable bursts
   - Too large: Defeats rate limiting purpose
   - Too small: May drop legitimate burst traffic

2. **Combining Limits**:
   - Use both byte and message limits when both matter
   - Use only byte limit for protocols with variable message sizes
   - Use only message limit to test message frequency impact

3. **Realistic Values**:
   - **2G/EDGE**: 50-250 KB/s
   - **NB-IoT**: 1-100 KB/s
   - **LoRaWAN**: 0.3-50 KB/s (depends on spreading factor)
   - **WiFi**: 500 KB/s - 10 MB/s
   - **Ethernet**: 1-100 MB/s

### Testing Recommendations

1. **Baseline First**: Test without bandwidth limits to establish baseline
2. **Gradual Reduction**: Progressively reduce bandwidth to find breaking points
3. **Monitor Throttling**: Track `bandwidth_throttled` to identify congestion
4. **Combine with Latency**: Real networks have both latency and bandwidth limits
5. **Test Recovery**: Verify mesh can recover when bandwidth constraints are relaxed

## Integration with Other Features

### With Latency Simulation

Bandwidth and latency work together:

```yaml
network:
  latency:
    default:
      min: 50
      max: 200
      distribution: "normal"
  
  bandwidth:
    default:
      max_bytes_per_sec: 5000
      bucket_size: 1000
```

**Effect**: Messages are both delayed (latency) and rate-limited (bandwidth)

### With Packet Loss

Realistic simulation combines all three:

```yaml
network:
  latency:
    default: { min: 50, max: 200 }
  packet_loss:
    default: { probability: 0.05 }
  bandwidth:
    default: { max_bytes_per_sec: 5000 }
```

**Use case**: Simulate challenging real-world conditions

## Troubleshooting

### High Throttling Rate

**Symptom**: Large `bandwidth_throttled` count

**Possible causes**:
- Bandwidth limit too low for application
- Message sizes too large
- Burst of messages exceeding bucket size
- Insufficient bucket size for bursty traffic

**Solutions**:
- Increase `max_bytes_per_sec` or `max_messages_per_sec`
- Increase `bucket_size` to handle bursts
- Optimize application to use smaller messages
- Implement message queuing in application layer

### Zero Bandwidth Utilization

**Symptom**: `bandwidth_utilization = 0` but messages are being sent

**Possible causes**:
- Bandwidth limits not configured (both limits = 0)
- Insufficient traffic to reach limits
- Metrics collected too early

**Solutions**:
- Verify bandwidth configuration is loaded
- Increase traffic generation rate
- Run simulation longer before collecting metrics

### Unexpected Message Drops

**Symptom**: Messages dropped even with low utilization

**Possible causes**:
- Very small bucket size
- Message size exceeds bucket capacity
- Concurrent messages consuming tokens

**Solutions**:
- Increase `bucket_size` to at least match largest message
- Spread message transmission over time
- Monitor token bucket state during debugging

## Performance Considerations

### Computational Overhead

Bandwidth limiting adds minimal overhead:
- **Per-message cost**: O(1) token bucket operations
- **Memory overhead**: One TokenBucket per configured connection
- **Refill cost**: O(1) calculation on each access

### Scalability

The implementation scales well:
- ✓ Handles 100+ nodes with bandwidth limiting
- ✓ Efficient map-based token bucket storage
- ✓ Lazy initialization (buckets created on first use)

### Optimization Tips

1. **Limit configured connections**: Only set bandwidth for connections that need it
2. **Use default settings**: Apply global limits when most connections have similar bandwidth
3. **Coarse time resolution**: Time is in milliseconds; sub-millisecond precision not needed

## API Reference

### NetworkSimulator Methods

#### setDefaultBandwidth
```cpp
void setDefaultBandwidth(const BandwidthConfig& config);
```
Sets the default bandwidth configuration for all connections.

**Throws**: `std::invalid_argument` if config is invalid

#### setBandwidth
```cpp
void setBandwidth(uint32_t fromNode, uint32_t toNode, const BandwidthConfig& config);
```
Sets bandwidth for a specific connection.

**Parameters**:
- `fromNode`: Source node ID
- `toNode`: Destination node ID
- `config`: Bandwidth configuration

**Throws**: `std::invalid_argument` if config is invalid

#### getBandwidth
```cpp
BandwidthConfig getBandwidth(uint32_t fromNode, uint32_t toNode) const;
```
Gets bandwidth configuration for a connection (specific or default).

**Returns**: BandwidthConfig for the connection

#### canSendMessage
```cpp
bool canSendMessage(uint32_t from, uint32_t to, size_t messageSize, uint64_t currentTime);
```
Checks if a message can be sent based on bandwidth limits.

**Parameters**:
- `from`: Source node ID
- `to`: Destination node ID
- `messageSize`: Message size in bytes
- `currentTime`: Current simulation time in milliseconds

**Returns**: `true` if message can be sent, `false` if bandwidth limit exceeded

#### consumeBandwidth
```cpp
void consumeBandwidth(uint32_t from, uint32_t to, size_t messageSize, uint64_t currentTime);
```
Consumes bandwidth tokens for a message.

**Parameters**:
- `from`: Source node ID
- `to`: Destination node ID
- `messageSize`: Message size in bytes
- `currentTime`: Current simulation time in milliseconds

## See Also

- [Network Latency Simulation](NETWORK_LATENCY.md)
- [Configuration Guide](CONFIGURATION_GUIDE.md)
- [Simulator Quickstart](SIMULATOR_QUICKSTART.md)

## References

- [Token Bucket Algorithm](https://en.wikipedia.org/wiki/Token_bucket)
- [Network Bandwidth Management](https://en.wikipedia.org/wiki/Bandwidth_management)
- [QoS Traffic Shaping](https://en.wikipedia.org/wiki/Traffic_shaping)
