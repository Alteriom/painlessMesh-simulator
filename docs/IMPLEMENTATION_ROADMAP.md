# painlessMesh Simulator - Implementation Roadmap

**Goal**: Enable comprehensive firmware validation for painlessMesh library features, particularly mesh networking and bridge functionality, without hardware dependencies.

**Status**: Phase 1 Complete ✅ | Planning Phase 2-4

**Date**: November 13, 2025

---

## Table of Contents

1. [Overview](#overview)
2. [Current State](#current-state)
3. [Phase 2: Network Simulation & Events](#phase-2-network-simulation--events)
4. [Phase 3: Firmware Integration Framework](#phase-3-firmware-integration-framework)
5. [Phase 4: Bridge & Advanced Features Testing](#phase-4-bridge--advanced-features-testing)
6. [Phase 5: Test Suite Expansion](#phase-5-test-suite-expansion)
7. [Implementation Timeline](#implementation-timeline)
8. [Success Criteria](#success-criteria)

---

## Overview

### Primary Objectives

1. **Firmware Validation**: Test painlessMesh library features without hardware
2. **Bridge Testing**: Validate bridge failover, promotion, and coordination
3. **Mesh Testing**: Verify routing, topology changes, message delivery
4. **Scenario-Based Testing**: Simulate network failures, partitions, recovery
5. **Integration Testing**: Extend existing test suite with simulator-based tests

### Non-Hardware Features to Test

✅ **Can Test**:
- Mesh formation and topology management
- Message routing (single, broadcast)
- Bridge election and failover
- Bridge coordination (primary/secondary/standby)
- Network partition detection and recovery
- Connection loss and reconnection
- Message queuing and priority
- Time synchronization (NTP simulation)
- MQTT bridge integration (simulated broker)
- Custom package handling (Alteriom packages)

❌ **Cannot Test** (Hardware-specific):
- Physical WiFi signal strength
- Actual ESP32/ESP8266 hardware peripherals
- Real sensor readings
- Actual GPIO operations
- Power consumption

---

## Current State

### Phase 1: Core Infrastructure ✅ COMPLETE

**Delivered**:
- VirtualNode class (wraps painlessMesh with Boost.Asio)
- NodeManager (creates/manages multiple nodes)
- Configuration loader (YAML scenarios)
- Mesh formation (automatic connectivity)
- Simulation loop (updates all nodes)
- Basic metrics collection

**Evidence**:
```
[60s] 10 nodes running, 5939 updates performed
Average update rate: 98 updates/sec
```

**What's Missing**: No firmware loaded, so nodes don't send application messages.

---

## Phase 2: Network Simulation & Events

**Duration**: 2 weeks  
**Goal**: Realistic network conditions and event-driven scenarios  
**Priority**: HIGH (enables realistic testing)

### 2.1 Network Condition Simulation

#### Issues to Create:

**Issue #1: Network Latency Simulation**
```yaml
Title: Implement configurable network latency simulation
Labels: enhancement, phase-2, network
Priority: P0 (blocking Phase 3)

Description:
Simulate realistic network conditions with configurable latency.

Requirements:
- Min/max latency bounds (e.g., 10-50ms)
- Distribution types: constant, normal, uniform
- Per-connection latency variation
- Apply latency to message delivery

Implementation:
- Add latency parameter to VirtualNode connections
- Queue messages with delayed delivery
- Use Boost.Asio timers for delayed processing

Acceptance Criteria:
- Configure latency in YAML: latency: {min: 10, max: 50, distribution: normal}
- Measure actual message delivery times
- Verify distribution matches configuration
```

**Issue #2: Packet Loss Simulation**
```yaml
Title: Implement packet loss simulation
Labels: enhancement, phase-2, network
Priority: P0

Description:
Simulate unreliable network with configurable packet loss.

Requirements:
- Configurable loss rate (0.0-1.0)
- Apply to individual messages
- Track dropped message statistics

Implementation:
- Random packet drop based on loss rate
- Update metrics (messages_dropped counter)
- Log dropped messages for debugging

Acceptance Criteria:
- Configure in YAML: packet_loss: 0.05  # 5% loss
- Verify actual drop rate matches configuration
- Dropped messages reflected in metrics
```

**Issue #3: Bandwidth Limiting**
```yaml
Title: Implement bandwidth limiting
Labels: enhancement, phase-2, network
Priority: P1

Description:
Simulate bandwidth constraints for realistic mesh performance.

Requirements:
- Configurable bandwidth per connection (bytes/sec)
- Message queuing when bandwidth exceeded
- Back-pressure handling

Implementation:
- Track bytes sent per time window
- Queue messages when limit reached
- Implement token bucket algorithm

Acceptance Criteria:
- Configure: bandwidth: 100000  # 100KB/s
- Measure actual throughput
- Messages queue when limit hit
```

### 2.2 Event System

#### Issues to Create:

**Issue #4: Event Engine Foundation**
```yaml
Title: Create event-driven scenario engine
Labels: enhancement, phase-2, events
Priority: P0

Description:
Framework for triggering events during simulation.

Requirements:
- Time-based event scheduling
- Condition-based event triggers
- Event handlers for common scenarios

Event Types:
- Node start/stop
- Connection drop/restore
- Bridge internet loss/restore
- Network partition/heal
- Node crash/recovery

Implementation:
- Event class hierarchy
- EventScheduler (priority queue by time)
- Event handlers per event type
- YAML event configuration

Acceptance Criteria:
- Schedule events: events: [{type: node_stop, time: 30, target: node-5}]
- Events execute at correct time
- Events trigger handlers correctly

Example YAML:
events:
  - type: node_stop
    time: 30
    target: sensor-5
    
  - type: bridge_internet_loss
    time: 45
    target: bridge-1
    
  - type: network_partition
    time: 60
    groups: [[node-1, node-2], [node-3, node-4]]
```

**Issue #5: Node Lifecycle Events**
```yaml
Title: Implement node lifecycle events (start, stop, crash, restart)
Labels: enhancement, phase-2, events
Priority: P0

Description:
Control individual node lifecycle during simulation.

Event Types:
1. node_start: Start a stopped node
2. node_stop: Gracefully stop a node
3. node_crash: Abruptly terminate node (no cleanup)
4. node_restart: Stop and start node

Implementation:
- NodeLifecycleEvent handler
- Call appropriate NodeManager methods
- Update node state tracking
- Trigger mesh topology recalculation

Acceptance Criteria:
- Events work as expected
- Mesh adapts to node changes
- Metrics reflect state changes
```

**Issue #6: Connection Events**
```yaml
Title: Implement connection drop/restore events
Labels: enhancement, phase-2, events, network
Priority: P0

Description:
Simulate connection failures and recovery.

Event Types:
1. connection_drop: Break connection between two nodes
2. connection_restore: Re-establish connection
3. connection_quality: Degrade connection (high latency/loss)

Implementation:
- Track connection state
- Disconnect nodes
- Reconnect nodes
- Apply quality degradation

Acceptance Criteria:
- Connections drop on command
- Mesh routing adapts
- Connections restore correctly
```

**Issue #7: Network Partition Events**
```yaml
Title: Implement network partition and healing
Labels: enhancement, phase-2, events, network
Priority: P1

Description:
Split mesh into isolated groups and later rejoin.

Requirements:
- Define node groups for partition
- Isolate groups (no cross-group traffic)
- Heal partition (restore connectivity)
- Test split-brain scenarios

Implementation:
- NetworkPartitionEvent
- Track partition groups
- Filter messages by group
- Merge groups on heal

Acceptance Criteria:
- Partition splits mesh correctly
- Nodes in different groups cannot communicate
- Heal restores full mesh connectivity
- Bridge election works per partition

Example:
events:
  - type: network_partition
    time: 30
    groups:
      - [sensor-1, sensor-2, sensor-3]
      - [sensor-4, sensor-5, bridge-1]
  
  - type: network_heal
    time: 60
```

---

## Phase 3: Firmware Integration Framework

**Duration**: 2-3 weeks  
**Goal**: Load and execute user firmware on virtual nodes  
**Priority**: CRITICAL (enables actual testing)

### 3.1 Firmware Framework

#### Issues to Create:

**Issue #8: FirmwareBase Interface Design**
```yaml
Title: Design and implement FirmwareBase interface
Labels: enhancement, phase-3, firmware, architecture
Priority: P0 (blocking all firmware work)

Description:
Create base class for all firmware implementations.

Interface:
class FirmwareBase {
public:
  virtual ~FirmwareBase() = default;
  
  // Arduino lifecycle
  virtual void setup() = 0;
  virtual void loop() = 0;
  
  // painlessMesh callbacks
  virtual void onReceive(uint32_t from, String& msg) = 0;
  virtual void onNewConnection(uint32_t nodeId) {}
  virtual void onChangedConnections() {}
  virtual void onNodeTimeAdjusted(int32_t offset) {}
  
protected:
  painlessMesh* mesh_ = nullptr;
  Scheduler* scheduler_ = nullptr;
  uint32_t nodeId_ = 0;
  
  // Helper methods
  void sendBroadcast(const String& msg);
  void sendSingle(uint32_t dest, const String& msg);
  uint32_t getNodeTime();
};

Implementation:
- Define interface in include/simulator/firmware/firmware_base.hpp
- Implement helper methods
- Provide access to mesh instance
- Document usage

Acceptance Criteria:
- Interface compiles
- Helper methods work
- Documentation complete
- Example firmware uses interface
```

**Issue #9: Firmware Factory and Loading**
```yaml
Title: Implement firmware factory and dynamic loading
Labels: enhancement, phase-3, firmware
Priority: P0

Description:
Load firmware implementations and instantiate on nodes.

Requirements:
- FirmwareFactory class
- Registry of available firmware types
- Create firmware instances per node type
- Inject mesh and scheduler

Implementation:
class FirmwareFactory {
public:
  using Creator = std::function<std::unique_ptr<FirmwareBase>()>;
  
  static void registerFirmware(const std::string& name, Creator creator);
  static std::unique_ptr<FirmwareBase> create(const std::string& name);
};

// Registration macro
#define REGISTER_FIRMWARE(name, class) \
  static FirmwareRegistrar<class> registrar_##class(name);

Usage:
// In sensor_firmware.cpp
REGISTER_FIRMWARE("sensor", SensorFirmware);

Acceptance Criteria:
- Factory creates firmware instances
- Registration works via macro
- Firmware properly initialized
- Multiple firmware types supported
```

**Issue #10: VirtualNode Firmware Integration**
```yaml
Title: Integrate firmware execution into VirtualNode
Labels: enhancement, phase-3, firmware, core
Priority: P0

Description:
Load and run firmware on virtual nodes.

Requirements:
- Load firmware based on config
- Call setup() on node start
- Call loop() in update cycle
- Route mesh callbacks to firmware

Implementation:
class VirtualNode {
  // ...existing...
  
  void loadFirmware(std::unique_ptr<FirmwareBase> firmware);
  
  void start() {
    // ...existing start logic...
    if (firmware_) {
      firmware_->setup();
    }
  }
  
  void update() {
    mesh_->update();
    if (firmware_) {
      firmware_->loop();
    }
    io_.poll();
  }
  
private:
  std::unique_ptr<FirmwareBase> firmware_;
};

Acceptance Criteria:
- Firmware loads from config
- setup() called once on start
- loop() called every update
- Callbacks routed correctly
- Multiple nodes with different firmware work
```

### 3.2 Example Firmware Implementations

#### Issues to Create:

**Issue #11: Simple Broadcast Firmware**
```yaml
Title: Create SimpleBroadcast example firmware
Labels: enhancement, phase-3, firmware, example
Priority: P0 (needed for testing)

Description:
Basic firmware that periodically broadcasts messages.

Requirements:
- Broadcast message every N seconds
- Count messages sent/received
- Log activity

Implementation:
class SimpleBroadcastFirmware : public FirmwareBase {
public:
  void setup() override {
    broadcastTask.set(TASK_SECOND * 5, TASK_FOREVER, [this]() {
      String msg = "Hello from node " + String(nodeId_);
      sendBroadcast(msg);
      messagesSent++;
    });
    scheduler_->addTask(broadcastTask);
    broadcastTask.enable();
  }
  
  void loop() override {
    // Optional periodic work
  }
  
  void onReceive(uint32_t from, String& msg) override {
    messagesReceived++;
    Serial.printf("Node %u received from %u: %s\n", 
                  nodeId_, from, msg.c_str());
  }

private:
  Task broadcastTask;
  uint32_t messagesSent = 0;
  uint32_t messagesReceived = 0;
};

REGISTER_FIRMWARE("simple_broadcast", SimpleBroadcastFirmware);

Acceptance Criteria:
- Broadcasts every 5 seconds
- All nodes receive broadcasts
- Metrics show messages sent/received
- Works with 10+ nodes

Test Scenario:
simulation:
  name: "Broadcast Test"
  duration: 60

nodes:
  - template: "broadcaster"
    count: 10
    firmware: "simple_broadcast"
```

**Issue #12: Echo Server/Client Firmware**
```yaml
Title: Create EchoServer and EchoClient firmware
Labels: enhancement, phase-3, firmware, example
Priority: P1

Description:
Request-response pattern firmware for testing directed messages.

EchoServer:
- Listens for messages
- Responds with echo

EchoClient:
- Sends periodic requests to server
- Validates responses

Implementation:
class EchoServerFirmware : public FirmwareBase {
  void onReceive(uint32_t from, String& msg) override {
    if (msg.startsWith("ECHO:")) {
      String response = "REPLY:" + msg.substring(5);
      sendSingle(from, response);
    }
  }
};

class EchoClientFirmware : public FirmwareBase {
  void setup() override {
    // Find server node
    // Send periodic requests
  }
  
  void onReceive(uint32_t from, String& msg) override {
    if (msg.startsWith("REPLY:")) {
      // Validate response
      responsesReceived++;
    }
  }
};

Acceptance Criteria:
- Client sends requests
- Server responds
- Client validates responses
- Works with network latency
```

**Issue #13: Bridge Node Firmware**
```yaml
Title: Create BridgeNode firmware with MQTT simulation
Labels: enhancement, phase-3, firmware, bridge, example
Priority: P0 (critical for bridge testing)

Description:
Firmware that acts as mesh-to-MQTT bridge.

Requirements:
- Configure as bridge node
- Simulate MQTT broker connection
- Handle bridge coordination messages
- Support internet loss/restore
- Participate in bridge election

Implementation:
class BridgeNodeFirmware : public FirmwareBase {
public:
  void setup() override {
    // Configure as bridge
    mesh_->setRoot(true);
    mesh_->setContainsRoot(true);
    
    // Simulate MQTT connection
    hasInternet_ = true;
    
    // Register bridge status callbacks
    mesh_->onBridgeStatusChanged([this]() {
      handleBridgeChange();
    });
  }
  
  void loop() override {
    // Simulate MQTT broker interaction
    if (hasInternet_) {
      // Process MQTT messages
    }
  }
  
  void setInternetStatus(bool status) {
    hasInternet_ = status;
    // Trigger bridge failover if needed
  }

private:
  bool hasInternet_;
  bool isPrimaryBridge_;
};

REGISTER_FIRMWARE("bridge", BridgeNodeFirmware);

Acceptance Criteria:
- Bridge advertises bridge status
- Handles internet loss
- Participates in election
- Secondary bridges standby
- MQTT messages simulated
```

---

## Phase 4: Bridge & Advanced Features Testing

**Duration**: 2-3 weeks  
**Goal**: Comprehensive bridge functionality validation  
**Priority**: HIGH (your primary requirement)

### 4.1 Bridge Failover Testing

#### Issues to Create:

**Issue #14: Bridge Election Scenario**
```yaml
Title: Implement bridge election testing scenario
Labels: enhancement, phase-4, bridge, testing
Priority: P0

Description:
Test bridge election when multiple bridges available.

Test Cases:
1. Multiple bridges start, one becomes primary
2. Primary bridge loses internet
3. Secondary bridge promotes to primary
4. Original primary regains internet (becomes secondary)
5. Both bridges lose internet (regular nodes take over)

Scenario YAML:
simulation:
  name: "Bridge Election Test"
  duration: 120

nodes:
  - id: bridge-1
    firmware: "bridge"
    config:
      mesh_prefix: "TestMesh"
      bridge_priority: 100
      has_internet: true
  
  - id: bridge-2
    firmware: "bridge"
    config:
      bridge_priority: 90
      has_internet: true
  
  - template: "sensor"
    count: 8
    firmware: "simple_broadcast"

events:
  - type: bridge_internet_loss
    time: 30
    target: bridge-1
    
  - type: bridge_internet_restore
    time: 60
    target: bridge-1
  
  - type: bridge_internet_loss
    time: 90
    target: bridge-2

metrics:
  collect:
    - bridge_changes
    - election_events
    - promotion_time
    - message_delivery_during_transition

Acceptance Criteria:
- Bridge-2 promotes when bridge-1 loses internet
- Promotion completes within 10 seconds
- Messages continue flowing during transition
- Bridge-1 becomes secondary after recovery
- No messages lost during failover
```

**Issue #15: Bridge Health Monitoring Test**
```yaml
Title: Test bridge health monitoring and heartbeat
Labels: enhancement, phase-4, bridge, testing
Priority: P1

Description:
Validate bridge health check mechanism.

Test Cases:
1. Healthy bridge sends regular heartbeats
2. Bridge timeout triggers election
3. Bridge recovery restores heartbeat
4. Multiple bridge timeouts handled correctly

Requirements:
- Monitor bridge heartbeat messages
- Track time since last heartbeat
- Detect bridge failure
- Trigger election on timeout

Acceptance Criteria:
- Heartbeats sent every 10 seconds
- Timeout detected after 30 seconds
- Election triggered automatically
- Recovery detected within 15 seconds
```

**Issue #16: Split-Brain Bridge Scenario**
```yaml
Title: Test split-brain scenario with multiple bridges
Labels: enhancement, phase-4, bridge, testing
Priority: P1

Description:
Handle network partition with bridges in different segments.

Test Case:
1. Network has 2 bridges and 10 nodes
2. Partition splits network: [bridge-1, 5 nodes] vs [bridge-2, 5 nodes]
3. Each partition elects its own primary bridge
4. Network heals
5. Bridges coordinate, one becomes primary

Scenario:
events:
  - type: network_partition
    time: 30
    groups:
      - [bridge-1, sensor-1, sensor-2, sensor-3, sensor-4, sensor-5]
      - [bridge-2, sensor-6, sensor-7, sensor-8, sensor-9, sensor-10]
  
  - type: network_heal
    time: 90

Expected Behavior:
- Each partition has working bridge
- Messages route within partition
- After heal, bridges coordinate
- Higher priority bridge becomes primary
- Lower priority becomes secondary

Acceptance Criteria:
- Partitions operate independently
- Both bridges functional in their partition
- Heal triggers coordination
- Final state has single primary
- No duplicate MQTT messages
```

### 4.2 Bridge Coordination Testing

#### Issues to Create:

**Issue #17: Primary/Secondary/Standby Roles**
```yaml
Title: Test bridge role transitions (primary, secondary, standby)
Labels: enhancement, phase-4, bridge, testing
Priority: P0

Description:
Validate all bridge role transitions.

Test Matrix:
- Primary → Secondary (new higher-priority bridge)
- Primary → Standby (internet loss)
- Secondary → Primary (primary fails)
- Secondary → Standby (internet loss)
- Standby → Secondary (internet restore)
- Standby → Primary (primary and secondary fail)

Requirements:
- Track role transitions
- Validate role-specific behavior
- Ensure proper message routing
- Test coordination messages

Acceptance Criteria:
- All transitions work correctly
- Role changes logged
- Coordination messages sent
- MQTT traffic handled appropriately per role
```

**Issue #18: Bridge Priority Testing**
```yaml
Title: Test bridge priority and preemption
Labels: enhancement, phase-4, bridge, testing
Priority: P1

Description:
Validate priority-based bridge selection.

Test Cases:
1. Lower priority bridge is primary
2. Higher priority bridge joins mesh
3. Higher priority preempts primary role
4. Graceful handoff of connections

Scenario:
nodes:
  - id: bridge-low
    firmware: "bridge"
    bridge_priority: 50
  
  - id: bridge-high
    firmware: "bridge"
    bridge_priority: 100

events:
  - type: node_start
    time: 0
    target: bridge-low
  
  - type: node_start
    time: 30
    target: bridge-high

Expected:
- Bridge-low becomes primary at t=0
- Bridge-high joins at t=30
- Bridge-high preempts within 15 seconds
- Bridge-low becomes secondary
- No message loss during handoff

Acceptance Criteria:
- Preemption works as expected
- Handoff time < 15 seconds
- Messages queued during transition
- All messages delivered
```

### 4.3 Advanced Mesh Features

#### Issues to Create:

**Issue #19: Message Routing Validation**
```yaml
Title: Test message routing across complex topologies
Labels: enhancement, phase-4, mesh, testing
Priority: P1

Description:
Validate routing works correctly.

Test Cases:
1. Single-hop delivery
2. Multi-hop delivery (3+ hops)
3. Routing around failed nodes
4. Routing after topology change
5. Broadcast delivery to all nodes

Topologies to Test:
- Linear chain (node1 → node2 → node3 → node4)
- Star (hub with spokes)
- Ring (circular)
- Random mesh

Acceptance Criteria:
- Messages reach destination
- Optimal path used
- Route updates after topology change
- Broadcast reaches all reachable nodes
```

**Issue #20: Time Synchronization Testing**
```yaml
Title: Test NTP-like time synchronization
Labels: enhancement, phase-4, mesh, testing
Priority: P2

Description:
Validate mesh time synchronization.

Requirements:
- Root node provides time reference
- Nodes synchronize to root
- Time offset tracked
- Drift compensation

Test Cases:
1. Nodes sync after joining mesh
2. Time accuracy within 100ms
3. Root change triggers re-sync
4. Network latency compensated

Acceptance Criteria:
- All nodes sync within 30 seconds
- Time delta < 100ms
- onNodeTimeAdjusted callbacks fired
- Sync persists over time
```

**Issue #21: Connection Quality Impact Testing**
```yaml
Title: Test mesh behavior under poor network conditions
Labels: enhancement, phase-4, mesh, network, testing
Priority: P1

Description:
Validate mesh adapts to network quality changes.

Test Scenarios:
1. High latency (100-500ms)
2. Packet loss (5-20%)
3. Bandwidth constraints (10KB/s)
4. Combinations of above

Expected Behavior:
- Messages still delivered (may be delayed)
- Routing adapts to avoid poor links
- Timeouts adjusted appropriately
- No false connection drops

Acceptance Criteria:
- 95% message delivery with 10% packet loss
- Messages delivered with high latency
- Mesh remains stable
- Metrics reflect network conditions
```

---

## Phase 5: Test Suite Expansion

**Duration**: 1-2 weeks  
**Goal**: Integrate simulator into existing test suite  
**Priority**: MEDIUM (improves quality assurance)

### 5.1 Unit Test Integration

#### Issues to Create:

**Issue #22: Simulator-Based Unit Tests**
```yaml
Title: Create unit tests using simulator
Labels: enhancement, phase-5, testing
Priority: P1

Description:
Add simulator-based tests to existing test suite.

Test Categories:
1. Basic mesh formation
2. Message delivery
3. Node lifecycle
4. Connection management
5. Bridge operations

Example Test:
TEST_CASE("Mesh formation with 10 nodes") {
  // Create scenario
  ScenarioConfig config;
  config.simulation.duration = 30;
  config.nodes.resize(10);
  
  // Run simulation
  boost::asio::io_context io;
  NodeManager manager(io);
  // ... create nodes ...
  
  // Verify
  REQUIRE(manager.getNodeCount() == 10);
  // ... check all nodes connected ...
}

Acceptance Criteria:
- 20+ simulator-based unit tests
- Tests run in CI/CD
- Fast execution (< 30s total)
- Good code coverage
```

**Issue #23: Regression Test Suite**
```yaml
Title: Build regression test suite with simulator
Labels: enhancement, phase-5, testing
Priority: P1

Description:
Prevent regressions in painlessMesh functionality.

Tests to Create:
1. Bridge failover (issue #192)
2. Message priority (issue #156)
3. Connection stability
4. Memory leaks
5. Performance degradation

Format:
- Each known bug → regression test
- Test reproduces original issue
- Verify fix still works

Acceptance Criteria:
- Regression tests for top 10 historical bugs
- Tests fail without fixes
- Tests pass with fixes
- Documented in test/regression/
```

### 5.2 Continuous Integration

#### Issues to Create:

**Issue #24: CI/CD Simulator Integration**
```yaml
Title: Add simulator tests to CI/CD pipeline
Labels: enhancement, phase-5, ci-cd
Priority: P1

Description:
Run simulator tests automatically on every commit.

Requirements:
- Add simulator build to CI
- Run unit tests
- Run scenario tests
- Generate coverage reports
- Fail on regressions

Implementation:
# .github/workflows/simulator-tests.yml
name: Simulator Tests
on: [push, pull_request]
jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      
      - name: Build Simulator
        run: |
          mkdir build && cd build
          cmake -G Ninja ..
          ninja
      
      - name: Run Unit Tests
        run: ./build/bin/simulator_tests
      
      - name: Run Scenario Tests
        run: |
          ./build/bin/painlessmesh-simulator \
            --config examples/scenarios/bridge_failover.yaml
      
      - name: Generate Coverage
        run: |
          # lcov/gcov coverage
          # Upload to Codecov

Acceptance Criteria:
- Tests run on every PR
- Results visible in GitHub
- Coverage tracked
- Failures block merge
```

---

## Implementation Timeline

### Month 1: Foundation (Weeks 1-4)

**Week 1: Phase 2 Part 1 - Network Simulation**
- Issue #1: Network latency ✓
- Issue #2: Packet loss ✓
- Issue #3: Bandwidth limiting ✓

**Week 2: Phase 2 Part 2 - Event System**
- Issue #4: Event engine ✓
- Issue #5: Node lifecycle events ✓
- Issue #6: Connection events ✓
- Issue #7: Network partition ✓

**Week 3: Phase 3 Part 1 - Firmware Framework**
- Issue #8: FirmwareBase interface ✓
- Issue #9: Firmware factory ✓
- Issue #10: VirtualNode integration ✓

**Week 4: Phase 3 Part 2 - Example Firmware**
- Issue #11: SimpleBroadcast ✓
- Issue #12: Echo server/client ✓
- Issue #13: Bridge node ✓

### Month 2: Bridge Testing (Weeks 5-8)

**Week 5: Phase 4 Part 1 - Basic Bridge**
- Issue #14: Bridge election scenario ✓
- Issue #15: Health monitoring ✓

**Week 6: Phase 4 Part 2 - Advanced Bridge**
- Issue #16: Split-brain scenario ✓
- Issue #17: Role transitions ✓
- Issue #18: Priority testing ✓

**Week 7: Phase 4 Part 3 - Mesh Features**
- Issue #19: Routing validation ✓
- Issue #20: Time sync ✓
- Issue #21: Network quality ✓

**Week 8: Phase 5 - Test Integration**
- Issue #22: Unit test integration ✓
- Issue #23: Regression suite ✓
- Issue #24: CI/CD integration ✓

---

## Success Criteria

### Phase 2 Success ✅

- [ ] Realistic network simulation (latency, loss, bandwidth)
- [ ] Event system triggers scenarios
- [ ] Network partitions work correctly
- [ ] Node lifecycle controlled via events

### Phase 3 Success ✅

- [ ] Firmware loads and executes on nodes
- [ ] Multiple firmware types supported
- [ ] Example firmware demonstrates features
- [ ] Messages sent/received correctly
- [ ] Bridge firmware functional

### Phase 4 Success ✅

- [ ] Bridge failover tested comprehensively
- [ ] Election algorithm validated
- [ ] Split-brain scenarios handled
- [ ] Role transitions work correctly
- [ ] Priority-based selection works
- [ ] All mesh features tested

### Phase 5 Success ✅

- [ ] 50+ simulator-based tests
- [ ] CI/CD runs all tests
- [ ] Coverage > 80%
- [ ] Regression tests for known issues
- [ ] Documentation complete

### Overall Success ✅

**You can confidently validate painlessMesh firmware without hardware:**

✅ **Bridge Testing**:
- Validate bridge failover in < 5 minutes
- Test election with any number of bridges
- Simulate internet loss/restore
- Test split-brain scenarios
- Verify coordination messages

✅ **Mesh Testing**:
- Test routing with any topology
- Validate message delivery
- Test network failures
- Verify recovery behavior
- Measure performance

✅ **Development Workflow**:
- Write firmware → Test in simulator → Deploy to hardware
- Catch bugs early (before flashing devices)
- Iterate quickly (no hardware setup)
- Automated testing (CI/CD integration)
- Reproducible scenarios

---

## Next Steps

### Immediate Actions (This Week)

1. **Review this plan** - Confirm priorities and scope
2. **Create GitHub issues** - Use templates above
3. **Start Phase 2.1** - Begin with network latency (Issue #1)
4. **Set up project board** - Track progress

### Development Approach

**For Each Issue:**
1. Create issue in GitHub
2. Assign to milestone (Phase 2/3/4/5)
3. Implement feature
4. Write tests
5. Document
6. Review and merge

**Testing Strategy:**
- TDD approach (write test first)
- Each feature has unit tests
- Integration tests for scenarios
- Document test cases

### Documentation Needs

Create/update these docs:
- [ ] `FIRMWARE_DEVELOPMENT_GUIDE.md` - How to write firmware for simulator
- [ ] `SCENARIO_GUIDE.md` - How to create test scenarios
- [ ] `BRIDGE_TESTING_GUIDE.md` - Comprehensive bridge testing guide
- [ ] `API_REFERENCE.md` - Complete API documentation

---

## Questions for Clarification

### 1. Bridge Testing Priorities

What bridge scenarios are MOST critical to test?
- [ ] Failover (primary → secondary)
- [ ] Election (multiple candidates)
- [ ] Split-brain handling
- [ ] Internet loss/restore
- [ ] Priority-based selection
- [ ] Coordination messages

### 2. Firmware Integration

Do you want to:
- [ ] Test existing Alteriom firmware directly in simulator?
- [ ] Create simplified test versions of firmware?
- [ ] Both?

### 3. MQTT Simulation

For MQTT bridge testing:
- [ ] Simulate MQTT broker in simulator
- [ ] Connect to real MQTT broker
- [ ] Mock MQTT broker (test mode)

### 4. Performance Requirements

What scale do you need?
- Typical: 10-20 nodes
- Large: 50-100 nodes
- Stress: 100+ nodes

### 5. Hardware Integration

Future consideration:
- [ ] Hardware-in-the-loop testing (simulator + real devices)
- [ ] Hybrid mode (some virtual, some real nodes)

---

## Conclusion

This roadmap provides a **comprehensive path to firmware validation** using the simulator. By implementing Phases 2-4, you'll be able to:

1. **Test bridge functionality** thoroughly without hardware
2. **Validate mesh behavior** under various conditions
3. **Catch bugs early** in development cycle
4. **Automate testing** via CI/CD
5. **Document behavior** through scenarios

**Estimated Timeline**: 8 weeks for full implementation  
**Team Size**: 1-2 developers  
**Risk**: Low (building on working Phase 1 foundation)

**Ready to proceed?** Let's start by creating the GitHub issues for Phase 2!

---

**Document Version**: 1.0  
**Last Updated**: November 13, 2025  
**Status**: Planning / Ready for Implementation
