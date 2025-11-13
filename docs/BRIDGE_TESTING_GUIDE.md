# Bridge Testing Guide - painlessMesh Simulator

**Goal**: Comprehensive testing of bridge failover, election, and coordination without hardware.

**Audience**: Developers testing painlessMesh bridge functionality  
**Status**: Implementation Guide  
**Date**: November 13, 2025

---

## Table of Contents

1. [Overview](#overview)
2. [Bridge Concepts](#bridge-concepts)
3. [Test Scenarios](#test-scenarios)
4. [Implementation Guide](#implementation-guide)
5. [Example Scenarios](#example-scenarios)
6. [Validation Checklist](#validation-checklist)

---

## Overview

### What Can Be Tested

✅ **Bridge Failover**
- Primary bridge loses internet → secondary promotes
- Promotion time measurement
- Message continuity during transition
- Graceful handoff of MQTT connections

✅ **Bridge Election**
- Multiple bridges compete for primary role
- Priority-based selection
- Election triggered by various events
- Coordination message exchange

✅ **Bridge Health Monitoring**
- Heartbeat mechanism
- Timeout detection
- Automatic failover on timeout
- Recovery detection

✅ **Bridge Coordination**
- Primary/Secondary/Standby roles
- Role transitions
- Coordination messages
- State synchronization

✅ **Split-Brain Scenarios**
- Network partition with bridges in each segment
- Independent operation per partition
- Healing and re-coordination
- Preventing duplicate MQTT messages

✅ **Bridge Priority**
- Higher priority bridge preempts lower
- Configurable priority values
- Preemption timing
- Graceful handoff

### What Cannot Be Tested

❌ **Hardware-Specific**
- Actual WiFi strength/range
- Real MQTT broker latency
- ESP32/ESP8266 hardware behavior
- Physical power loss

❌ **External Dependencies**
- Actual internet connectivity
- Real router/gateway behavior
- DNS resolution
- SSL/TLS handshakes

---

## Bridge Concepts

### Bridge Roles

**Primary Bridge**:
- Actively forwards messages between mesh and MQTT
- Sends regular heartbeats
- Coordinates with secondary bridges
- Has internet connectivity

**Secondary Bridge**:
- Monitors primary bridge health
- Ready to promote if primary fails
- Maintains connection to mesh
- Has internet connectivity

**Standby Bridge**:
- No internet connectivity currently
- Monitors network
- Can become secondary when internet restored
- Can promote if primary/secondary both fail

### Bridge States

```
┌─────────────┐
│   Standby   │ (no internet)
└──────┬──────┘
       │ internet restored
       ▼
┌─────────────┐
│  Secondary  │ (has internet, monitoring)
└──────┬──────┘
       │ primary fails
       ▼
┌─────────────┐
│   Primary   │ (active forwarding)
└─────────────┘
```

### Election Algorithm

1. **Trigger**: Primary bridge timeout or internet loss
2. **Candidates**: All bridges with internet connectivity
3. **Selection**: Highest priority bridge wins
4. **Coordination**: New primary announces role
5. **Confirmation**: Others acknowledge and become secondary

### Key Metrics

- **Promotion Time**: Time from primary failure to secondary takeover
- **Message Loss**: Messages lost during transition
- **Downtime**: Period with no active bridge
- **False Positives**: Unnecessary elections
- **Split-Brain Duration**: Time with multiple primaries

---

## Test Scenarios

### Scenario 1: Basic Bridge Failover

**Goal**: Verify secondary bridge promotes when primary loses internet.

**Setup**:
- 2 bridges (primary with priority 100, secondary with priority 90)
- 8 regular sensor nodes
- MQTT traffic flowing

**Test Steps**:
1. Start simulation, verify bridge-1 is primary
2. At t=30s, trigger `bridge_internet_loss` on bridge-1
3. Observe bridge-2 promotion
4. Verify messages continue flowing
5. At t=60s, trigger `bridge_internet_restore` on bridge-1
6. Observe bridge-1 becomes secondary (bridge-2 stays primary)

**Expected Results**:
- Bridge-2 promotes within 10 seconds
- < 5 messages lost during transition
- Mesh remains stable
- Bridge-1 recognizes bridge-2 as primary after restore

**Success Criteria**:
```yaml
metrics:
  promotion_time: < 10s
  messages_lost: < 5
  total_downtime: < 15s
  bridge_changes: 2
```

**YAML Configuration**:
```yaml
simulation:
  name: "Basic Bridge Failover Test"
  duration: 90

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
    config:
      broadcast_interval: 5

events:
  - type: bridge_internet_loss
    time: 30
    target: bridge-1
  
  - type: bridge_internet_restore
    time: 60
    target: bridge-1

metrics:
  output: "results/bridge_failover_basic.csv"
  collect:
    - bridge_changes
    - promotion_time
    - message_loss
    - active_bridge
```

---

### Scenario 2: Bridge Election with Multiple Candidates

**Goal**: Verify correct bridge selected from multiple candidates.

**Setup**:
- 3 bridges with different priorities (100, 90, 80)
- 10 regular nodes
- All bridges start with internet

**Test Steps**:
1. Start simulation with all bridges online
2. Verify highest priority (bridge-1) becomes primary
3. At t=30s, crash bridge-1 (no cleanup)
4. Observe election between bridge-2 and bridge-3
5. Verify bridge-2 (priority 90) wins
6. At t=60s, restart bridge-1
7. Verify bridge-1 preempts and becomes primary

**Expected Results**:
- Bridge-1 initially primary
- Bridge-2 wins election after bridge-1 crash
- Bridge-1 preempts when rejoining
- Elections complete in < 15 seconds
- No split-brain periods

**YAML Configuration**:
```yaml
simulation:
  name: "Multi-Candidate Bridge Election"
  duration: 90

nodes:
  - id: bridge-1
    firmware: "bridge"
    config:
      bridge_priority: 100
      has_internet: true
  
  - id: bridge-2
    firmware: "bridge"
    config:
      bridge_priority: 90
      has_internet: true
  
  - id: bridge-3
    firmware: "bridge"
    config:
      bridge_priority: 80
      has_internet: true
  
  - template: "sensor"
    count: 10
    firmware: "simple_broadcast"

events:
  - type: node_crash
    time: 30
    target: bridge-1
  
  - type: node_restart
    time: 60
    target: bridge-1

metrics:
  collect:
    - election_events
    - active_bridge
    - bridge_roles
    - election_duration
```

---

### Scenario 3: Split-Brain Network Partition

**Goal**: Handle network partition with bridges in different segments.

**Setup**:
- 2 bridges (priority 100 and 90)
- 12 regular nodes
- Network partitions at t=30s

**Test Steps**:
1. Start simulation, verify bridge-1 is primary
2. At t=30s, partition network:
   - Group A: bridge-1 + 6 nodes
   - Group B: bridge-2 + 6 nodes
3. Verify each partition has working bridge
4. Verify no cross-partition traffic
5. At t=90s, heal partition
6. Verify bridges coordinate
7. Verify bridge-1 becomes primary (higher priority)

**Expected Results**:
- Each partition operates independently
- Both bridges functional in their partitions
- No message loss within partitions
- After heal, single primary elected within 20s
- No duplicate MQTT messages after heal

**YAML Configuration**:
```yaml
simulation:
  name: "Split-Brain Bridge Test"
  duration: 120

nodes:
  - id: bridge-1
    firmware: "bridge"
    config:
      bridge_priority: 100
      has_internet: true
  
  - id: bridge-2
    firmware: "bridge"
    config:
      bridge_priority: 90
      has_internet: true
  
  - template: "sensor"
    count: 12
    firmware: "simple_broadcast"
    id_prefix: "sensor-"

events:
  - type: network_partition
    time: 30
    groups:
      - [bridge-1, sensor-1, sensor-2, sensor-3, sensor-4, sensor-5, sensor-6]
      - [bridge-2, sensor-7, sensor-8, sensor-9, sensor-10, sensor-11, sensor-12]
  
  - type: network_heal
    time: 90

metrics:
  collect:
    - partition_state
    - bridge_per_partition
    - messages_per_partition
    - coordination_messages
    - heal_time
```

---

### Scenario 4: Bridge Health Monitoring

**Goal**: Verify bridge timeout detection and automatic failover.

**Setup**:
- 2 bridges (normal heartbeat interval: 10s, timeout: 30s)
- 10 regular nodes
- Primary bridge stops sending heartbeats (simulated hang)

**Test Steps**:
1. Start simulation, bridge-1 is primary
2. At t=30s, trigger `bridge_heartbeat_stop` on bridge-1
3. Monitor secondary bridge (bridge-2) timeout detection
4. Verify bridge-2 promotes after timeout (30s)
5. At t=90s, bridge-1 recovers heartbeat
6. Verify bridge-1 becomes secondary

**Expected Results**:
- Bridge-2 detects timeout after 30s
- Promotion occurs immediately after timeout
- Recovery detected within 1 heartbeat interval
- No false timeout detections

**YAML Configuration**:
```yaml
simulation:
  name: "Bridge Health Monitoring Test"
  duration: 120

bridge_config:
  heartbeat_interval: 10  # seconds
  timeout_threshold: 30   # seconds
  
nodes:
  - id: bridge-1
    firmware: "bridge"
    config:
      bridge_priority: 100
      has_internet: true
  
  - id: bridge-2
    firmware: "bridge"
    config:
      bridge_priority: 90
      has_internet: true
  
  - template: "sensor"
    count: 10
    firmware: "simple_broadcast"

events:
  - type: bridge_heartbeat_stop
    time: 30
    target: bridge-1
  
  - type: bridge_heartbeat_resume
    time: 90
    target: bridge-1

metrics:
  collect:
    - heartbeat_received
    - timeout_detected
    - time_to_timeout
    - false_timeouts
```

---

### Scenario 5: Bridge Priority and Preemption

**Goal**: Validate priority-based selection and preemption.

**Setup**:
- Lower priority bridge starts first as primary
- Higher priority bridge joins later
- Verify preemption occurs

**Test Steps**:
1. Start bridge-2 (priority 90) at t=0
2. Verify bridge-2 becomes primary
3. Start bridge-1 (priority 100) at t=30
4. Verify bridge-1 preempts bridge-2
5. Measure preemption time
6. Verify no messages lost

**Expected Results**:
- Bridge-2 initially primary
- Bridge-1 preempts within 15 seconds of joining
- Graceful handoff (bridge-2 queues messages, bridge-1 takes over)
- All queued messages delivered
- No duplicate messages

**YAML Configuration**:
```yaml
simulation:
  name: "Bridge Priority Preemption Test"
  duration: 90

nodes:
  - id: bridge-2
    firmware: "bridge"
    config:
      bridge_priority: 90
      has_internet: true
  
  - id: bridge-1
    firmware: "bridge"
    config:
      bridge_priority: 100
      has_internet: true
  
  - template: "sensor"
    count: 8
    firmware: "simple_broadcast"

events:
  - type: node_start
    time: 0
    target: bridge-2
  
  - type: node_start
    time: 30
    target: bridge-1

metrics:
  collect:
    - preemption_time
    - messages_queued
    - messages_lost
    - handoff_duration
```

---

### Scenario 6: Cascading Bridge Failures

**Goal**: Test multiple sequential bridge failures.

**Setup**:
- 3 bridges (priorities 100, 90, 80)
- 12 regular nodes
- Sequential failures

**Test Steps**:
1. All bridges online, bridge-1 is primary
2. At t=30s, bridge-1 loses internet
3. Bridge-2 promotes
4. At t=60s, bridge-2 loses internet
5. Bridge-3 promotes
6. At t=90s, all bridges regain internet
7. Bridge-1 preempts (highest priority)

**Expected Results**:
- Each promotion completes successfully
- Mesh remains operational throughout
- Final state: bridge-1 primary, others secondary
- Total downtime < 45 seconds (cumulative)

**YAML Configuration**:
```yaml
simulation:
  name: "Cascading Bridge Failures"
  duration: 120

nodes:
  - id: bridge-1
    firmware: "bridge"
    config:
      bridge_priority: 100
  
  - id: bridge-2
    firmware: "bridge"
    config:
      bridge_priority: 90
  
  - id: bridge-3
    firmware: "bridge"
    config:
      bridge_priority: 80
  
  - template: "sensor"
    count: 12
    firmware: "simple_broadcast"

events:
  - type: bridge_internet_loss
    time: 30
    target: bridge-1
  
  - type: bridge_internet_loss
    time: 60
    target: bridge-2
  
  - type: bridge_internet_restore
    time: 90
    target: bridge-1
  
  - type: bridge_internet_restore
    time: 95
    target: bridge-2

metrics:
  collect:
    - total_downtime
    - promotion_count
    - active_bridge_timeline
    - message_loss_per_event
```

---

## Implementation Guide

### Step 1: Implement Bridge Firmware

```cpp
// examples/firmware/bridge_node_firmware.cpp
class BridgeNodeFirmware : public FirmwareBase {
public:
  String getName() const override { return "BridgeNode"; }
  
  void setup() override {
    // Configure as bridge
    mesh_->setRoot(true);
    mesh_->setContainsRoot(true);
    
    // Set initial state
    hasInternet_ = config_.has_internet;
    priority_ = config_.bridge_priority;
    role_ = BridgeRole::STANDBY;
    
    // Register callbacks
    mesh_->onBridgeStatusChanged([this]() {
      handleBridgeStatusChange();
    });
    
    // Start heartbeat task
    heartbeatTask.set(TASK_SECOND * 10, TASK_FOREVER, [this]() {
      sendHeartbeat();
    });
    scheduler_->addTask(heartbeatTask);
    heartbeatTask.enable();
    
    // Start election monitoring
    monitorTask.set(TASK_SECOND * 5, TASK_FOREVER, [this]() {
      monitorBridgeHealth();
    });
    scheduler_->addTask(monitorTask);
    monitorTask.enable();
    
    // Participate in initial election
    participateInElection();
  }
  
  void loop() override {
    // Simulate MQTT broker interaction
    if (role_ == BridgeRole::PRIMARY && hasInternet_) {
      processMQTTMessages();
    }
    
    // Update metrics
    updateMetrics();
  }
  
  void onReceive(uint32_t from, String& msg) override {
    // Parse message type
    if (msg.startsWith("BRIDGE_HEARTBEAT:")) {
      handleHeartbeat(from, msg);
    }
    else if (msg.startsWith("BRIDGE_ELECTION:")) {
      handleElectionMessage(from, msg);
    }
    else if (msg.startsWith("BRIDGE_COORD:")) {
      handleCoordinationMessage(from, msg);
    }
    else {
      // Regular mesh message - forward to MQTT if primary
      if (role_ == BridgeRole::PRIMARY) {
        forwardToMQTT(from, msg);
      }
    }
  }
  
  void onNewConnection(uint32_t nodeId) override {
    // Send bridge status to new node
    announceBridgeStatus();
  }
  
  void onChangedConnections() override {
    // Topology changed, re-evaluate bridge status
    if (role_ == BridgeRole::PRIMARY) {
      verifyBridgeConnectivity();
    }
  }
  
  // Public methods for event control
  void setInternetStatus(bool status) {
    if (hasInternet_ != status) {
      hasInternet_ = status;
      handleInternetChange();
    }
  }
  
  void stopHeartbeat() {
    heartbeatTask.disable();
  }
  
  void resumeHeartbeat() {
    heartbeatTask.enable();
  }

private:
  enum class BridgeRole { STANDBY, SECONDARY, PRIMARY };
  
  BridgeRole role_;
  bool hasInternet_;
  uint8_t priority_;
  uint32_t lastHeartbeatTime_;
  std::map<uint32_t, uint32_t> bridgeHeartbeats_;  // nodeId -> last seen
  
  Task heartbeatTask;
  Task monitorTask;
  
  void participateInElection() {
    if (!hasInternet_) {
      role_ = BridgeRole::STANDBY;
      return;
    }
    
    // Broadcast election message with priority
    String msg = "BRIDGE_ELECTION:" + String(priority_) + ":" + String(nodeId_);
    sendBroadcast(msg);
    
    // Wait for responses and determine role
    scheduler_->addTask(new Task(TASK_SECOND * 5, TASK_ONCE, [this]() {
      determineRole();
    }));
  }
  
  void sendHeartbeat() {
    if (role_ == BridgeRole::PRIMARY) {
      String msg = "BRIDGE_HEARTBEAT:" + String(nodeId_) + ":" + String(priority_);
      sendBroadcast(msg);
    }
  }
  
  void handleHeartbeat(uint32_t from, const String& msg) {
    // Update last seen time for this bridge
    bridgeHeartbeats_[from] = getNodeTime();
    
    // If this is from a higher priority primary, become secondary
    // Parse priority from message
    // ... implementation ...
  }
  
  void monitorBridgeHealth() {
    if (role_ == BridgeRole::SECONDARY) {
      uint32_t now = getNodeTime();
      
      // Check if primary bridge timed out
      for (auto& [bridgeId, lastSeen] : bridgeHeartbeats_) {
        if ((now - lastSeen) > 30000) {  // 30 second timeout
          // Primary bridge timeout detected
          triggerElection();
          break;
        }
      }
    }
  }
  
  void handleInternetChange() {
    if (hasInternet_) {
      // Internet restored - participate in election
      participateInElection();
    } else {
      // Internet lost
      if (role_ == BridgeRole::PRIMARY) {
        // Announce stepping down
        String msg = "BRIDGE_STEPPING_DOWN:" + String(nodeId_);
        sendBroadcast(msg);
        
        role_ = BridgeRole::STANDBY;
        
        // Trigger election for remaining bridges
        triggerElection();
      } else {
        role_ = BridgeRole::STANDBY;
      }
    }
  }
  
  void processMQTTMessages() {
    // Simulate MQTT broker interaction
    // In real implementation, this would:
    // - Poll MQTT broker for messages
    // - Forward to mesh
    // - Send mesh messages to MQTT
  }
  
  void forwardToMQTT(uint32_t from, const String& msg) {
    // Simulate forwarding to MQTT
    metrics_.mqttMessagesSent++;
  }
};

REGISTER_FIRMWARE("bridge", BridgeNodeFirmware);
```

### Step 2: Add Bridge Events

```cpp
// src/events/bridge_events.cpp
class BridgeInternetLossEvent : public Event {
public:
  void execute(NodeManager& manager) override {
    auto node = manager.getNode(target_node_id);
    if (node && node->hasFirmware()) {
      auto firmware = dynamic_cast<BridgeNodeFirmware*>(node->getFirmware());
      if (firmware) {
        firmware->setInternetStatus(false);
      }
    }
  }

private:
  uint32_t target_node_id;
};

class BridgeHeartbeatStopEvent : public Event {
  // Similar implementation
};
```

### Step 3: Add Bridge Metrics

```cpp
// src/metrics/bridge_metrics.cpp
struct BridgeMetrics {
  uint32_t promotion_count = 0;
  std::vector<uint32_t> promotion_times;  // Milliseconds
  uint32_t election_count = 0;
  uint32_t active_bridge_id = 0;
  BridgeRole active_bridge_role;
  uint32_t total_downtime_ms = 0;
  uint32_t messages_lost_during_transition = 0;
  
  std::map<uint32_t, BridgeRole> bridge_roles;  // nodeId -> role
  
  void recordPromotion(uint32_t time_ms) {
    promotion_count++;
    promotion_times.push_back(time_ms);
  }
  
  void exportCSV(const std::string& filename) {
    // Export metrics to CSV
  }
};
```

---

## Validation Checklist

### Bridge Failover ✅

- [ ] Secondary promotes when primary loses internet
- [ ] Promotion time < 10 seconds
- [ ] Message loss < 5 messages
- [ ] Total downtime < 15 seconds
- [ ] Original primary becomes secondary after recovery
- [ ] No duplicate MQTT messages

### Bridge Election ✅

- [ ] Highest priority bridge selected
- [ ] Election completes < 15 seconds
- [ ] All bridges with internet participate
- [ ] Standby bridges (no internet) don't participate
- [ ] Election triggered by timeout
- [ ] Election triggered by internet loss

### Bridge Health ✅

- [ ] Heartbeats sent every 10 seconds
- [ ] Timeout detected after 30 seconds
- [ ] No false timeout detections
- [ ] Recovery detected within 1 heartbeat interval
- [ ] Health status exposed via API

### Bridge Coordination ✅

- [ ] Primary/Secondary/Standby roles tracked
- [ ] Role transitions logged
- [ ] Coordination messages sent
- [ ] State synchronized across bridges
- [ ] Split-brain prevented after network heal

### Bridge Priority ✅

- [ ] Higher priority bridge preempts lower
- [ ] Preemption time < 15 seconds
- [ ] Graceful handoff (no message loss)
- [ ] Priority configurable per bridge
- [ ] Priority respected in all elections

### Split-Brain Handling ✅

- [ ] Partitions operate independently
- [ ] Each partition has functional bridge
- [ ] Healing triggers coordination
- [ ] Single primary after heal
- [ ] No duplicate messages after heal

---

## Success Metrics

### Performance Targets

| Metric | Target | Measured |
|--------|--------|----------|
| Promotion Time | < 10s | ___s |
| Election Duration | < 15s | ___s |
| Preemption Time | < 15s | ___s |
| Message Loss (failover) | < 5 messages | ___ |
| Total Downtime | < 20s | ___s |
| False Timeouts | 0 | ___ |
| Heal Time (partition) | < 30s | ___s |

### Test Coverage

- [ ] 6 core scenarios implemented
- [ ] All scenarios pass acceptance criteria
- [ ] Metrics collected for all scenarios
- [ ] Results documented
- [ ] Regression tests created

---

## Next Steps

1. **Implement Phase 3** (Firmware Integration)
   - Complete Issues #8, #9, #10
   - Create BridgeNodeFirmware
   
2. **Implement Bridge Events**
   - bridge_internet_loss
   - bridge_internet_restore
   - bridge_heartbeat_stop

3. **Run Scenarios**
   - Start with Scenario 1 (Basic Failover)
   - Validate metrics
   - Fix issues
   - Proceed to remaining scenarios

4. **Document Results**
   - Record metrics for each scenario
   - Create performance baseline
   - Identify optimization opportunities

5. **Integrate with CI/CD**
   - Add bridge tests to CI pipeline
   - Block merges on test failures
   - Track performance trends

---

**This guide provides a complete roadmap for testing painlessMesh bridge functionality without hardware!**

