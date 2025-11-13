# Configuration Guide

This guide provides comprehensive documentation for creating and validating YAML configuration files for the painlessMesh simulator.

## Table of Contents

1. [Overview](#overview)
2. [Basic Structure](#basic-structure)
3. [Configuration Sections](#configuration-sections)
   - [Simulation](#simulation)
   - [Network](#network)
   - [Nodes](#nodes)
   - [Topology](#topology)
   - [Events](#events)
   - [Metrics](#metrics)
4. [Node Templates](#node-templates)
5. [Validation Rules](#validation-rules)
6. [Complete Examples](#complete-examples)
7. [Best Practices](#best-practices)
8. [Troubleshooting](#troubleshooting)

## Overview

The configuration system allows you to define complete simulation scenarios using YAML files. Each scenario specifies:

- **Simulation parameters**: Duration, time scale, random seed
- **Network conditions**: Latency, packet loss, bandwidth
- **Node configurations**: Mesh settings, firmware, custom parameters
- **Network topology**: Connection patterns between nodes
- **Events**: Scheduled actions during simulation
- **Metrics**: Data collection and export settings

### Purpose

Configuration files enable:
- **Reproducible Tests**: Same scenario runs identically with fixed seed
- **Scenario Library**: Build a collection of test scenarios
- **Batch Node Creation**: Use templates to generate many nodes
- **Automated Testing**: Run scenarios in CI/CD pipelines
- **Documentation**: Self-documenting test cases

## Basic Structure

A minimal configuration file:

```yaml
simulation:
  name: "My Simulation"
  duration: 60

nodes:
  - template: "basic"
    count: 5
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"

topology:
  type: "random"
```

## Configuration Sections

### Simulation

Defines the simulation parameters and runtime behavior.

#### Schema

```yaml
simulation:
  name: string              # Required - Simulation name
  description: string       # Optional - Detailed description
  duration: uint32          # Seconds (0 = infinite)
  time_scale: float         # Time multiplier (default: 1.0)
  seed: uint32              # Random seed (0 = random)
```

#### Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `name` | string | *required* | Human-readable simulation name |
| `description` | string | "" | Optional detailed description |
| `duration` | uint32 | 0 | Simulation duration in seconds (0 = run indefinitely) |
| `time_scale` | float | 1.0 | Time scale multiplier (1.0 = real-time, 5.0 = 5x faster) |
| `seed` | uint32 | 0 | Random seed for reproducibility (0 = use random seed) |

#### Example

```yaml
simulation:
  name: "Network Resilience Test"
  description: "Tests mesh recovery after multiple node failures"
  duration: 300      # 5 minutes
  time_scale: 2.0    # Run 2x faster
  seed: 12345        # Reproducible results
```

#### Notes

- **name** is required and must not be empty
- **time_scale** must be positive (> 0.0)
- **time_scale** > 1.0 makes simulation faster (good for stress tests)
- **time_scale** < 1.0 makes simulation slower (good for debugging)
- Setting **seed** ensures identical random behavior across runs

---

### Network

Configures network conditions and quality parameters.

#### Schema

```yaml
network:
  latency:
    min: uint32             # Minimum latency (ms)
    max: uint32             # Maximum latency (ms)
    distribution: string    # Distribution type
  packet_loss: float        # Packet loss rate (0.0-1.0)
  bandwidth: uint64         # Bandwidth (bits per second)
```

#### Parameters

**Latency Configuration:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `latency.min` | uint32 | 10 | Minimum network latency in milliseconds |
| `latency.max` | uint32 | 50 | Maximum network latency in milliseconds |
| `latency.distribution` | string | "normal" | Distribution type: "normal", "uniform", "exponential" |

**Network Quality:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `packet_loss` | float | 0.0 | Packet loss rate (0.0 = no loss, 1.0 = total loss) |
| `bandwidth` | uint64 | 1000000 | Network bandwidth in bits per second |

#### Example

```yaml
network:
  latency:
    min: 20           # 20ms minimum latency
    max: 100          # 100ms maximum latency (realistic WiFi)
    distribution: "normal"
  packet_loss: 0.05   # 5% packet loss
  bandwidth: 1000000  # 1 Mbps
```

#### Notes

- **min** must be less than or equal to **max**
- **packet_loss** must be between 0.0 and 1.0
- **bandwidth** must be greater than 0
- **distribution** types:
  - `normal`: Bell curve around average latency
  - `uniform`: Random latency within range
  - `exponential`: Occasional high latency spikes
- Use packet_loss: 0.0 for ideal network conditions
- Use packet_loss: 0.01-0.05 for realistic conditions
- Use packet_loss: 0.1+ for challenging conditions

---

### Nodes

Defines individual nodes or node templates for the mesh network.

#### Individual Node Schema

```yaml
nodes:
  - id: string              # Unique node identifier
    type: string            # Node type (sensor, bridge, etc.)
    firmware: string        # Path to firmware
    position: [int, int]    # [x, y] coordinates
    config:
      mesh_prefix: string   # Required - Mesh SSID prefix
      mesh_password: string # Required - Mesh password
      mesh_port: uint16     # Mesh port (default: 5555)
      # Extended configuration (optional)
      sensor_interval: uint32
      mqtt_broker: string
      mqtt_port: uint16
      mqtt_topic_prefix: string
```

#### Parameters

**Core Configuration:**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `id` | string | *required* | Unique node identifier |
| `type` | string | "" | Node type classification |
| `firmware` | string | "" | Path to firmware implementation |
| `position` | [int, int] | [] | X, Y coordinates for visualization |

**Mesh Settings (config block):**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `mesh_prefix` | string | *required* | Mesh network SSID prefix |
| `mesh_password` | string | *required* | Mesh network password |
| `mesh_port` | uint16 | 5555 | Mesh network port |

**Extended Configuration (optional):**

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `sensor_interval` | uint32 | - | Sensor reading interval in milliseconds |
| `mqtt_broker` | string | - | MQTT broker address |
| `mqtt_port` | uint16 | - | MQTT broker port |
| `mqtt_topic_prefix` | string | - | MQTT topic prefix |

#### Individual Node Example

```yaml
nodes:
  - id: "bridge-1"
    type: "mqtt_bridge"
    firmware: "examples/firmware/mqtt_bridge"
    position: [0, 0]
    config:
      mesh_prefix: "MyMesh"
      mesh_password: "secure_password"
      mesh_port: 5555
      mqtt_broker: "localhost"
      mqtt_port: 1883
      mqtt_topic_prefix: "mesh/"
```

#### Notes

- **id** must be unique across all nodes
- **mesh_prefix** and **mesh_password** are required for all nodes
- **position** is optional but useful for visualization
- Extended configuration fields are firmware-specific
- All nodes in the same mesh must use identical **mesh_prefix**, **mesh_password**, and **mesh_port**

---

### Node Templates

Templates allow batch creation of multiple similar nodes.

#### Template Schema

```yaml
nodes:
  - template: string        # Template name
    count: uint32           # Number of nodes to generate
    id_prefix: string       # ID prefix for generated nodes
    firmware: string        # Firmware path
    config:                 # Same as individual node config
      mesh_prefix: string
      mesh_password: string
      # ... other config options
```

#### Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `template` | string | *required* | Template identifier/name |
| `count` | uint32 | 1 | Number of nodes to generate |
| `id_prefix` | string | template + "-" | Prefix for generated node IDs |
| `firmware` | string | "" | Firmware path for all nodes |
| `config` | object | {} | Configuration applied to all nodes |

#### Template Example

```yaml
nodes:
  # Generate 50 sensor nodes
  - template: "sensor"
    count: 50
    id_prefix: "sensor-"
    firmware: "examples/firmware/sensor_node"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password123"
      mesh_port: 5555
      sensor_interval: 30000
```

This expands to:
- Node IDs: `sensor-0`, `sensor-1`, `sensor-2`, ... `sensor-49`
- All nodes share the same firmware and configuration
- Each node gets a unique numeric node ID (auto-generated from string ID)

#### Mixing Templates and Individual Nodes

You can combine templates and individual nodes:

```yaml
nodes:
  # Central hub
  - id: "hub-1"
    type: "bridge"
    firmware: "examples/firmware/bridge"
    config:
      mesh_prefix: "MyMesh"
      mesh_password: "password"
  
  # 20 sensor nodes
  - template: "sensor"
    count: 20
    id_prefix: "sensor-"
    firmware: "examples/firmware/sensor"
    config:
      mesh_prefix: "MyMesh"
      mesh_password: "password"
```

#### Template Expansion

Templates are automatically expanded during configuration loading:
1. Each template generates `count` individual nodes
2. Node IDs are generated as `id_prefix` + index (0-based)
3. All nodes inherit the template's firmware and configuration
4. Numeric node IDs are auto-generated from string IDs using hashing

---

### Topology

Defines the network connection structure between nodes.

#### Schema

```yaml
topology:
  type: string              # Topology type
  # Type-specific parameters
  hub: string               # Hub node ID (star only)
  density: float            # Connection density (random only)
  bidirectional: bool       # Bidirectional links (ring only)
  connections: [[string, string]]  # Custom connections
```

#### Topology Types

| Type | Description | Required Parameters |
|------|-------------|-------------------|
| `random` | Random connections based on density | `density` |
| `star` | Star topology with central hub | `hub` |
| `ring` | Ring topology with sequential connections | `bidirectional` (optional) |
| `mesh` | Full mesh (all nodes connected) | None |
| `custom` | Custom connections defined explicitly | `connections` |

#### Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `type` | string | "random" | Topology type |
| `hub` | string | - | Hub node ID (required for star) |
| `density` | float | 0.3 | Connection probability 0.0-1.0 (for random) |
| `bidirectional` | bool | true | Two-way links (for ring) |
| `connections` | array | [] | List of [from, to] pairs (for custom) |

#### Random Topology

```yaml
topology:
  type: "random"
  density: 0.3  # 30% connection probability
```

- Creates random connections between nodes
- **density** controls how connected the network is:
  - 0.0 = no connections (isolated nodes)
  - 0.3 = sparse network (30% of possible connections)
  - 0.7 = dense network (70% of possible connections)
  - 1.0 = full mesh (all nodes connected)

#### Star Topology

```yaml
topology:
  type: "star"
  hub: "bridge-1"  # Central hub node
```

- All nodes connect only to the central hub
- **hub** must reference an existing node ID
- Hub failure fragments the network

#### Ring Topology

```yaml
topology:
  type: "ring"
  bidirectional: true  # Two-way connections
```

- Nodes connected in a ring: N₀ ↔ N₁ ↔ N₂ ↔ ... ↔ Nₙ ↔ N₀
- **bidirectional**: true for ↔, false for → (unidirectional)

#### Full Mesh Topology

```yaml
topology:
  type: "mesh"
```

- Every node connected to every other node
- Highest redundancy but highest overhead
- Good for small networks (< 20 nodes)

#### Custom Topology

```yaml
topology:
  type: "custom"
  connections:
    - ["node-1", "node-2"]  # Connect node-1 to node-2
    - ["node-2", "node-3"]  # Connect node-2 to node-3
    - ["node-1", "node-3"]  # Connect node-1 to node-3
```

- Explicitly define each connection
- **connections**: array of [from, to] node ID pairs
- Both node IDs must exist in the configuration

#### Notes

- Topology only defines initial connections
- painlessMesh will discover and adjust connections dynamically
- Use custom topology for testing specific scenarios
- Validation ensures all referenced nodes exist

---

### Events

Scheduled events allow dynamic scenario changes during simulation.

#### Schema

```yaml
events:
  - time: uint32            # Event time (seconds)
    action: string          # Event action type
    target: string          # Target node ID
    description: string     # Optional description
    # Action-specific parameters
```

#### Event Actions

| Action | Description | Required Parameters |
|--------|-------------|-------------------|
| `stop_node` | Stop a node | `target` |
| `start_node` | Start a stopped node | `target` |
| `restart_node` | Restart a node | `target` |
| `remove_node` | Remove node from simulation | `target` |
| `add_nodes` | Add new nodes dynamically | `count`, `template` |
| `partition_network` | Split network into groups | `groups` |
| `heal_partition` | Restore network connectivity | None |
| `break_link` | Break connection between nodes | `from`, `to` |
| `restore_link` | Restore broken connection | `from`, `to` |
| `inject_message` | Inject a message | `from`, `to`, `payload` |
| `set_network_quality` | Change network quality | `quality`, `target` |

#### Common Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `time` | uint32 | 0 | Event time in seconds from simulation start |
| `action` | string | *required* | Event action type (see table above) |
| `target` | string | "" | Target node ID |
| `targets` | [string] | [] | Multiple target node IDs |
| `description` | string | "" | Human-readable description |

#### Node Control Events

**Stop Node:**
```yaml
- time: 60
  action: "stop_node"
  target: "sensor-25"
  description: "Simulate node failure"
```

**Start Node:**
```yaml
- time: 120
  action: "start_node"
  target: "sensor-25"
  description: "Recover failed node"
```

**Restart Node:**
```yaml
- time: 180
  action: "restart_node"
  target: "bridge-1"
  description: "Simulate node reboot"
```

#### Network Manipulation Events

**Partition Network:**
```yaml
- time: 60
  action: "partition_network"
  groups:
    - ["sensor-0", "sensor-1", "sensor-2"]
    - ["sensor-3", "sensor-4", "sensor-5"]
  description: "Split network into two groups"
```

**Heal Partition:**
```yaml
- time: 120
  action: "heal_partition"
  description: "Restore network connectivity"
```

**Break Link:**
```yaml
- time: 90
  action: "break_link"
  from: "hub-1"
  to: "sensor-10"
  description: "Simulate link failure"
```

**Restore Link:**
```yaml
- time: 150
  action: "restore_link"
  from: "hub-1"
  to: "sensor-10"
  description: "Restore broken link"
```

#### Message Injection

```yaml
- time: 30
  action: "inject_message"
  from: "bridge-1"
  to: "broadcast"
  payload: '{"type": 200, "command": "status_check"}'
  description: "Request status from all nodes"
```

Parameters:
- **from**: Source node ID
- **to**: Destination node ID (or "broadcast")
- **payload**: JSON message payload

#### Network Quality Changes

```yaml
- time: 60
  action: "set_network_quality"
  target: "sensor-10"
  quality: 0.5
  description: "Degrade connection quality to 50%"
```

Parameters:
- **quality**: 0.0-1.0 (0.0 = worst, 1.0 = best)
- **target**: Node ID (or omit for global)

#### Dynamic Node Addition

```yaml
- time: 120
  action: "add_nodes"
  count: 10
  template: "sensor"
  id_prefix: "late-sensor-"
  description: "Add 10 more nodes mid-simulation"
```

Parameters:
- **count**: Number of nodes to add
- **template**: Template name to use
- **id_prefix**: ID prefix for new nodes

#### Notes

- Events are sorted and executed in time order
- Event **time** must be within simulation **duration** (if set)
- Referenced nodes must exist at event time
- Multiple events can occur at the same time
- Use **description** to document test scenarios

---

### Metrics

Configures data collection and export during simulation.

#### Schema

```yaml
metrics:
  output: string            # Output file path
  interval: uint32          # Collection interval (seconds)
  collect: [string]         # Metrics to collect
  export: [string]          # Export formats
```

#### Parameters

| Parameter | Type | Default | Description |
|-----------|------|---------|-------------|
| `output` | string | "" | Output file path (without extension) |
| `interval` | uint32 | 5 | Collection interval in seconds |
| `collect` | [string] | [] | List of metrics to collect |
| `export` | [string] | [] | List of export formats |

#### Available Metrics

| Metric | Description |
|--------|-------------|
| `message_count` | Total messages sent/received |
| `delivery_rate` | Message delivery success rate |
| `latency_stats` | Latency statistics (min, max, avg) |
| `topology_changes` | Number of topology changes |
| `node_uptime` | Node uptime statistics |
| `connectivity_graph` | Connection graph structure |
| `packet_stats` | Packet transmission statistics |

#### Export Formats

| Format | Description | Extension |
|--------|-------------|-----------|
| `csv` | Comma-separated values | .csv |
| `json` | JSON format | .json |
| `graphviz` | GraphViz DOT format | .dot |

#### Example

```yaml
metrics:
  output: "results/my_test"
  interval: 10
  collect:
    - message_count
    - delivery_rate
    - latency_stats
    - topology_changes
  export:
    - csv
    - json
    - graphviz
```

This creates:
- `results/my_test.csv` - CSV metrics data
- `results/my_test.json` - JSON metrics data
- `results/my_test.dot` - GraphViz topology visualization

#### Notes

- **output** directory must exist or be created
- **interval** determines collection frequency
- Shorter **interval** = more detailed data but higher overhead
- **graphviz** export can be visualized with: `dot -Tpng output.dot -o output.png`
- Metrics are collected continuously and written at intervals

---

## Validation Rules

The configuration loader validates all settings before running the simulation.

### Simulation Validation

| Rule | Error Message | Suggestion |
|------|--------------|------------|
| Name required | "Simulation name is required" | Add a descriptive name |
| Positive time scale | "Time scale must be positive" | Use 1.0 for real-time |

### Network Validation

| Rule | Error Message | Suggestion |
|------|--------------|------------|
| Min ≤ Max latency | "Minimum latency cannot be greater than maximum" | Set min ≤ max |
| Packet loss range | "Packet loss must be between 0.0 and 1.0" | Use 0.01 for 1% loss |
| Non-zero bandwidth | "Bandwidth cannot be zero" | Specify bits per second |

### Node Validation

| Rule | Error Message | Suggestion |
|------|--------------|------------|
| Unique node IDs | "Duplicate node ID: {id}" | Ensure all node IDs are unique |
| Node ID required | "Node ID is required" | Provide unique identifier |
| Mesh prefix required | "Mesh prefix is required for node: {id}" | Set mesh_prefix in config |
| Mesh password required | "Mesh password is required for node: {id}" | Set mesh_password in config |
| Valid mesh port | "Invalid mesh port for node: {id}" | Use port 5555 or valid port |
| At least one node | "No nodes defined" | Add at least one node or template |

### Topology Validation

| Rule | Error Message | Suggestion |
|------|--------------|------------|
| Hub exists (star) | "Hub node not found: {hub}" | Ensure hub node ID matches |
| Hub required (star) | "Hub node required for star topology" | Specify hub node |
| Valid density | "Density must be between 0.0 and 1.0" | Use 0.3 for sparse, 0.7 for dense |
| Custom connections | "Custom topology requires connection definitions" | Add connections array |
| Valid node refs | "Connection references non-existent node: {id}" | Ensure all nodes exist |

### Event Validation

| Rule | Error Message | Suggestion |
|------|--------------|------------|
| Event time valid | "Event time {time}s exceeds simulation duration" | Ensure events within duration |
| Target exists | "Event references non-existent node: {target}" | Ensure target node exists |
| Valid quality | "Network quality must be between 0.0 and 1.0" | Use 0.0 for worst, 1.0 for best |

### Error Handling

When validation fails, the loader:
1. Collects all validation errors
2. Prints detailed error messages with suggestions
3. Returns `boost::none` (empty optional)

Example error output:
```
Configuration validation failed with 2 error(s):
  - simulation.time_scale: Time scale must be positive (Use 1.0 for real-time, >1.0 for faster simulation)
  - topology.hub: Hub node not found: invalid-hub (Ensure hub node ID matches an existing node)
```

---

## Complete Examples

### Example 1: Simple 10-Node Mesh

**File: `simple_mesh.yaml`**

```yaml
# Simple 10-Node Mesh Scenario
# This scenario demonstrates basic mesh formation with 10 nodes

simulation:
  name: "Simple 10-Node Mesh"
  description: "Basic mesh formation test with 10 sensor nodes"
  duration: 60  # Run for 60 seconds
  time_scale: 1.0  # Real-time simulation
  seed: 12345  # For reproducible results

network:
  latency:
    min: 10  # 10ms minimum latency
    max: 30  # 30ms maximum latency
    distribution: "normal"
  packet_loss: 0.0  # No packet loss
  bandwidth: 1000000  # 1 Mbps

nodes:
  # Generate 10 sensor nodes using template
  - template: "sensor"
    count: 10
    id_prefix: "sensor-"
    firmware: "examples/firmware/sensor_node"
    config:
      mesh_prefix: "SimpleMesh"
      mesh_password: "password123"
      mesh_port: 5555
      sensor_interval: 30000  # Read sensor every 30 seconds

topology:
  type: "random"
  density: 0.3  # Sparse network (30% connection probability)

metrics:
  output: "results/simple_mesh"
  interval: 5  # Collect metrics every 5 seconds
  collect:
    - message_count
    - delivery_rate
    - topology_changes
  export:
    - csv
    - json
```

**Key Features:**
- 10 nodes generated from template
- Ideal network conditions (no packet loss)
- Random sparse topology
- Reproducible with seed 12345

---

### Example 2: 100-Node Stress Test

**File: `stress_test.yaml`**

```yaml
# 100-Node Stress Test Scenario
# This scenario tests the simulator's ability to handle large mesh networks

simulation:
  name: "100-Node Stress Test"
  description: "Performance test with 100 nodes and realistic network conditions"
  duration: 300  # Run for 5 minutes
  time_scale: 5.0  # Run 5x faster than real-time
  seed: 54321

network:
  latency:
    min: 20  # 20ms minimum latency
    max: 100  # 100ms maximum latency (realistic WiFi)
    distribution: "normal"
  packet_loss: 0.05  # 5% packet loss (challenging conditions)
  bandwidth: 1000000  # 1 Mbps

nodes:
  # Generate 100 sensor nodes
  - template: "sensor"
    count: 100
    id_prefix: "sensor-"
    firmware: "examples/firmware/sensor_node"
    config:
      mesh_prefix: "StressMesh"
      mesh_password: "stress_test_password"
      mesh_port: 5555
      sensor_interval: 60000  # Read sensor every 60 seconds

topology:
  type: "random"
  density: 0.2  # Sparse network (20% connection probability)

events:
  # Test node failures at different points
  - time: 60
    action: "stop_node"
    target: "sensor-25"
    description: "Simulate node failure"
  
  - time: 120
    action: "stop_node"
    target: "sensor-50"
    description: "Another node failure"
  
  - time: 180
    action: "start_node"
    target: "sensor-25"
    description: "Recover first node"
  
  - time: 240
    action: "start_node"
    target: "sensor-50"
    description: "Recover second node"

metrics:
  output: "results/stress_test"
  interval: 10  # Collect metrics every 10 seconds
  collect:
    - message_count
    - delivery_rate
    - latency_stats
    - topology_changes
    - node_uptime
  export:
    - csv
    - json
    - graphviz  # Generate topology visualization
```

**Key Features:**
- 100 nodes for performance testing
- Realistic network conditions (5% packet loss, high latency)
- 5x time acceleration
- Node failure and recovery events
- Comprehensive metrics collection

---

### Example 3: Star Topology with MQTT Bridge

**File: `star_topology.yaml`**

```yaml
# Star Topology with MQTT Bridge
# This scenario demonstrates a star topology with a central MQTT bridge node

simulation:
  name: "Star Topology with MQTT Bridge"
  description: "20 sensor nodes connected through a central MQTT bridge"
  duration: 180  # Run for 3 minutes
  time_scale: 1.0  # Real-time simulation
  seed: 99999

network:
  latency:
    min: 10
    max: 50
    distribution: "normal"
  packet_loss: 0.01  # 1% packet loss
  bandwidth: 1000000  # 1 Mbps

nodes:
  # Central MQTT bridge node (hub)
  - id: "bridge-1"
    type: "mqtt_bridge"
    firmware: "examples/firmware/mqtt_bridge"
    position: [0, 0]  # Center position
    config:
      mesh_prefix: "StarMesh"
      mesh_password: "bridge_pass_123"
      mesh_port: 5555
      mqtt_broker: "localhost"
      mqtt_port: 1883
      mqtt_topic_prefix: "mesh/"
  
  # Generate 20 sensor nodes
  - template: "sensor"
    count: 20
    id_prefix: "sensor-"
    firmware: "examples/firmware/sensor_node"
    config:
      mesh_prefix: "StarMesh"
      mesh_password: "bridge_pass_123"
      mesh_port: 5555
      sensor_interval: 30000  # 30 second intervals

topology:
  type: "star"
  hub: "bridge-1"  # Central hub node

events:
  # Test hub failure and recovery
  - time: 60
    action: "stop_node"
    target: "bridge-1"
    description: "Simulate hub failure - mesh should fragment"
  
  - time: 120
    action: "start_node"
    target: "bridge-1"
    description: "Restore hub - mesh should reconnect"
  
  # Test message injection after recovery
  - time: 150
    action: "inject_message"
    from: "bridge-1"
    to: "broadcast"
    payload: '{"type": 200, "command": "status_check"}'
    description: "Request status from all nodes"

metrics:
  output: "results/star_topology"
  interval: 5  # Collect metrics every 5 seconds
  collect:
    - message_count
    - delivery_rate
    - latency_stats
    - topology_changes
    - connectivity_graph
  export:
    - csv
    - json
    - graphviz
```

**Key Features:**
- Star topology with central hub
- Mix of individual node and template
- Hub failure test scenario
- Message injection event
- MQTT bridge configuration

---

## Best Practices

### Scenario Design

**Start Simple, Add Complexity**
```yaml
# Start with minimal scenario
simulation:
  name: "Basic Test"
  duration: 60

nodes:
  - template: "sensor"
    count: 5
    config:
      mesh_prefix: "Test"
      mesh_password: "pass"

topology:
  type: "random"

# Then add network conditions, events, metrics as needed
```

**Use Descriptive Names**
```yaml
# Good
simulation:
  name: "Mesh Recovery After Hub Failure"
  description: "Tests how quickly the mesh recovers when central hub goes offline"

# Bad
simulation:
  name: "Test1"
```

**Set Reproducible Seeds**
```yaml
# Use fixed seed for reproducible tests
simulation:
  seed: 12345  # Same results every run

# Use 0 for random behavior
simulation:
  seed: 0  # Different results each run
```

### Performance Optimization

**Accelerate Long Tests**
```yaml
# For 100+ nodes, use time acceleration
simulation:
  duration: 300
  time_scale: 5.0  # 5 minutes runs in 1 minute
```

**Adjust Metrics Interval**
```yaml
# More frequent = more detail, more overhead
metrics:
  interval: 1  # Every second (high detail)

# Less frequent = less detail, less overhead
metrics:
  interval: 30  # Every 30 seconds (summary)
```

**Use Templates for Large Networks**
```yaml
# Don't define 100 nodes individually
# Use templates instead
nodes:
  - template: "sensor"
    count: 100
    id_prefix: "sensor-"
```

### Testing Strategies

**Gradual Complexity**
1. Test with 5 nodes, ideal network
2. Add network conditions (latency, packet loss)
3. Increase to 20-50 nodes
4. Add events (failures, partitions)
5. Scale to 100+ nodes

**Scenario Library**
```
scenarios/
├── basic/
│   ├── 5_node_mesh.yaml
│   ├── 10_node_mesh.yaml
│   └── 20_node_mesh.yaml
├── stress/
│   ├── 100_nodes.yaml
│   ├── 200_nodes.yaml
│   └── high_churn.yaml
├── topologies/
│   ├── star.yaml
│   ├── ring.yaml
│   └── mesh.yaml
└── failure/
    ├── hub_failure.yaml
    ├── multiple_failures.yaml
    └── network_partition.yaml
```

**Document Test Intent**
```yaml
# Use description field to explain test purpose
simulation:
  name: "Network Partition Recovery"
  description: |
    Tests mesh behavior when network splits into two partitions
    at 60s and heals at 120s. Measures recovery time and
    message delivery impact.
```

### Configuration Management

**Use Consistent Naming**
```yaml
# Good: Consistent prefixes
nodes:
  - id: "sensor-1"
  - id: "sensor-2"
  - id: "bridge-1"

# Bad: Inconsistent names
nodes:
  - id: "node1"
  - id: "MySensor"
  - id: "BR_001"
```

**Separate Concerns**
```yaml
# Use templates for common configuration
nodes:
  - template: "production_sensor"
    count: 50
    # All production sensor settings in template

  - template: "dev_sensor"
    count: 5
    # All dev sensor settings in template
```

**Version Your Scenarios**
```yaml
# Include version in name or description
simulation:
  name: "Stress Test v2.1"
  description: "Updated with new failure patterns (2025-01-15)"
```

---

## Troubleshooting

### Common Configuration Errors

#### Error: "Simulation name is required"

**Problem:**
```yaml
simulation:
  duration: 60
  # Missing name field
```

**Solution:**
```yaml
simulation:
  name: "My Simulation"  # Add name
  duration: 60
```

---

#### Error: "Time scale must be positive"

**Problem:**
```yaml
simulation:
  time_scale: 0.0  # Invalid
```

**Solution:**
```yaml
simulation:
  time_scale: 1.0  # Use positive value
```

---

#### Error: "Minimum latency cannot be greater than maximum"

**Problem:**
```yaml
network:
  latency:
    min: 100
    max: 50  # Min > Max
```

**Solution:**
```yaml
network:
  latency:
    min: 50   # Swap values
    max: 100
```

---

#### Error: "Packet loss must be between 0.0 and 1.0"

**Problem:**
```yaml
network:
  packet_loss: 5  # Should be 0.05 for 5%
```

**Solution:**
```yaml
network:
  packet_loss: 0.05  # 5% as decimal
```

---

#### Error: "Duplicate node ID: sensor-5"

**Problem:**
```yaml
nodes:
  - id: "sensor-5"
    # ...
  - id: "sensor-5"  # Duplicate!
    # ...
```

**Solution:**
```yaml
nodes:
  - id: "sensor-5"
    # ...
  - id: "sensor-6"  # Unique ID
    # ...
```

---

#### Error: "Mesh prefix is required for node: sensor-1"

**Problem:**
```yaml
nodes:
  - id: "sensor-1"
    config:
      mesh_password: "pass"
      # Missing mesh_prefix
```

**Solution:**
```yaml
nodes:
  - id: "sensor-1"
    config:
      mesh_prefix: "MyMesh"  # Add prefix
      mesh_password: "pass"
```

---

#### Error: "Hub node not found: invalid-hub"

**Problem:**
```yaml
topology:
  type: "star"
  hub: "invalid-hub"  # Node doesn't exist
```

**Solution:**
```yaml
nodes:
  - id: "bridge-1"  # Define hub node
    # ...

topology:
  type: "star"
  hub: "bridge-1"  # Reference existing node
```

---

#### Error: "Event time exceeds simulation duration"

**Problem:**
```yaml
simulation:
  duration: 60

events:
  - time: 120  # Beyond simulation duration
    action: "stop_node"
```

**Solution:**
```yaml
simulation:
  duration: 180  # Extend duration

events:
  - time: 120  # Now within duration
    action: "stop_node"
```

Or:
```yaml
simulation:
  duration: 60

events:
  - time: 30  # Within duration
    action: "stop_node"
```

---

### YAML Syntax Issues

#### Problem: Indentation Error

```yaml
simulation:
  name: "Test"
description: "Wrong indent"  # Should be indented
```

**Solution:**
```yaml
simulation:
  name: "Test"
  description: "Correct indent"  # Properly indented
```

---

#### Problem: Missing Quotes

```yaml
nodes:
  - id: sensor-1  # Might be interpreted as subtraction
```

**Solution:**
```yaml
nodes:
  - id: "sensor-1"  # Explicitly a string
```

---

#### Problem: Invalid List Syntax

```yaml
collect: message_count, delivery_rate  # Wrong
```

**Solution:**
```yaml
collect:
  - message_count
  - delivery_rate
```

---

### Validation Tips

**Use Schema Validation**

The ConfigLoader automatically validates your configuration:

```cpp
ConfigLoader loader;
auto config = loader.loadFromFile("scenario.yaml");
if (!config) {
  std::cerr << "Validation failed:\n";
  std::cerr << loader.getLastError() << std::endl;
}
```

**Check Before Running**

Create a validation-only mode:
```bash
./painlessmesh-simulator --validate scenario.yaml
```

**Review Error Messages**

Error messages include suggestions:
```
Configuration validation failed with 1 error(s):
  - network.packet_loss: Packet loss must be between 0.0 and 1.0
    (Use 0.01 for 1% packet loss)
```

---

### Debugging Scenarios

**Enable Verbose Logging**
```bash
./painlessmesh-simulator --config scenario.yaml --verbose
```

**Start with Minimal Configuration**
```yaml
# Minimal valid configuration
simulation:
  name: "Debug Test"
  duration: 10

nodes:
  - template: "basic"
    count: 2
    config:
      mesh_prefix: "Test"
      mesh_password: "pass"

topology:
  type: "mesh"
```

**Add Complexity Incrementally**
1. Verify basic configuration loads
2. Add network conditions
3. Add more nodes
4. Add events
5. Add metrics

---

## Reference

### Related Documentation

- **[SIMULATOR_PLAN.md](SIMULATOR_PLAN.md)**: Complete technical specification
- **[SIMULATOR_QUICKSTART.md](SIMULATOR_QUICKSTART.md)**: Quick start guide
- **[API_REFERENCE.md](API_REFERENCE.md)**: C++ API documentation

### Example Scenarios

Complete examples available in `examples/scenarios/`:
- `simple_mesh.yaml` - Basic 10-node mesh
- `stress_test.yaml` - 100-node performance test
- `star_topology.yaml` - Star topology with MQTT bridge

### Configuration Schema

The complete C++ API is documented in:
- `include/simulator/config_loader.hpp` - Header with schema definitions
- `src/config/config_loader.cpp` - Implementation with validation logic

---

**Document Version**: 1.0  
**Last Updated**: 2025-01-12  
**Status**: Complete
