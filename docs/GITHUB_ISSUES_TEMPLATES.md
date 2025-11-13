# Quick Issue Creation Guide

This file contains ready-to-use GitHub issue templates from the implementation roadmap. Copy/paste these directly into GitHub to start implementation.

---

## Phase 2: Network Simulation & Events (7 Issues)

### Issue #1: Network Latency Simulation

**Title**: Implement configurable network latency simulation

**Labels**: `enhancement`, `phase-2`, `network`, `P0`

**Description**:
Simulate realistic network conditions with configurable latency to enable testing mesh behavior under real-world network delays.

**Requirements**:
- Min/max latency bounds (e.g., 10-50ms)
- Distribution types: constant, normal, uniform
- Per-connection latency variation
- Apply latency to message delivery between nodes

**Implementation Approach**:
- Add latency parameter to VirtualNode connection configuration
- Queue messages with delayed delivery timestamps
- Use Boost.Asio deadline_timer for delayed processing
- Store pending messages in priority queue (sorted by delivery time)

**Acceptance Criteria**:
- [ ] Configure latency in YAML: `latency: {min: 10, max: 50, distribution: normal}`
- [ ] Measure actual message delivery times match configuration
- [ ] Distribution histogram matches specified type (normal/uniform/constant)
- [ ] Different connections can have different latencies
- [ ] Latency applied to all message types (single, broadcast, control)

**Test Scenario**:
```yaml
network:
  latency:
    min: 10
    max: 50
    distribution: "normal"
```

---

### Issue #2: Packet Loss Simulation

**Title**: Implement packet loss simulation

**Labels**: `enhancement`, `phase-2`, `network`, `P0`

**Description**:
Simulate unreliable networks with configurable packet loss rates to test mesh resilience and message retransmission.

**Requirements**:
- Configurable loss rate (0.0-1.0)
- Random packet drop based on probability
- Track dropped message statistics
- Apply to individual messages (not whole connections)

**Implementation Approach**:
- Generate random number for each message
- Drop message if random < loss_rate
- Update metrics (messages_dropped counter)
- Log dropped messages with debug level
- Ensure mesh protocol messages also affected (realistic)

**Acceptance Criteria**:
- [ ] Configure in YAML: `packet_loss: 0.05` (5% loss)
- [ ] Verify actual drop rate matches configuration (±1%)
- [ ] Dropped messages reflected in metrics
- [ ] Mesh adapts and retransmits when needed
- [ ] Statistics report: sent, delivered, dropped

**Test Scenario**:
```yaml
network:
  packet_loss: 0.10  # 10% loss rate
```

---

### Issue #3: Bandwidth Limiting

**Title**: Implement bandwidth limiting simulation

**Labels**: `enhancement`, `phase-2`, `network`, `P1`

**Description**:
Simulate bandwidth constraints to test mesh performance under limited throughput conditions.

**Requirements**:
- Configurable bandwidth per connection (bytes/sec)
- Message queuing when bandwidth exceeded
- Back-pressure handling
- Measure actual throughput

**Implementation Approach**:
- Implement token bucket algorithm
- Track bytes sent per time window (1 second)
- Queue messages when tokens exhausted
- Replenish tokens periodically
- Apply per-connection limits

**Acceptance Criteria**:
- [ ] Configure: `bandwidth: 100000` (100KB/s)
- [ ] Measure actual throughput ≈ configured limit
- [ ] Messages queue when limit hit
- [ ] Queue drains when bandwidth available
- [ ] No message loss due to queueing

**Test Scenario**:
```yaml
network:
  bandwidth: 50000  # 50KB/s
```

---

### Issue #4: Event Engine Foundation

**Title**: Create event-driven scenario engine

**Labels**: `enhancement`, `phase-2`, `events`, `architecture`, `P0`

**Description**:
Framework for triggering events during simulation to enable complex scenario testing.

**Event Types Needed**:
- Node lifecycle (start, stop, crash, restart)
- Connection control (drop, restore, degrade)
- Network topology (partition, heal)
- Bridge control (internet loss/restore)
- Custom events (extensible)

**Implementation Approach**:
```cpp
class Event {
public:
  virtual ~Event() = default;
  virtual void execute(NodeManager& manager) = 0;
  virtual std::string describe() const = 0;
  
  uint32_t time_seconds;  // When to execute
  std::string type;
};

class EventScheduler {
public:
  void schedule(std::unique_ptr<Event> event);
  void processEvents(uint32_t current_time);
  
private:
  std::priority_queue<EventPtr> events_;  // Min-heap by time
};
```

**YAML Configuration**:
```yaml
events:
  - type: node_stop
    time: 30
    target: sensor-5
    
  - type: bridge_internet_loss
    time: 45
    target: bridge-1
```

**Acceptance Criteria**:
- [ ] Event base class and scheduler implemented
- [ ] Events load from YAML configuration
- [ ] Events execute at correct simulation time
- [ ] Multiple events at same time handled correctly
- [ ] Event logging shows what happened and when
- [ ] Extensible for new event types

---

### Issue #5: Node Lifecycle Events

**Title**: Implement node lifecycle events (start, stop, crash, restart)

**Labels**: `enhancement`, `phase-2`, `events`, `P0`

**Description**:
Control individual node lifecycle during simulation to test mesh adaptation to node failures.

**Event Types**:
1. `node_start`: Start a previously stopped node
2. `node_stop`: Gracefully stop a node (cleanup, notify mesh)
3. `node_crash`: Abruptly terminate node (no cleanup, simulates power loss)
4. `node_restart`: Stop and start node (simulate reboot)

**Implementation**:
```cpp
class NodeLifecycleEvent : public Event {
public:
  enum Action { START, STOP, CRASH, RESTART };
  
  void execute(NodeManager& manager) override {
    auto node = manager.getNode(target_node_id);
    switch (action) {
      case START: node->start(); break;
      case STOP: node->stop(); break;
      case CRASH: node->crash(); break;  // No cleanup
      case RESTART: node->stop(); node->start(); break;
    }
  }
  
private:
  uint32_t target_node_id;
  Action action;
};
```

**YAML Examples**:
```yaml
events:
  - type: node_stop
    time: 30
    target: sensor-5
    
  - type: node_crash
    time: 45
    target: bridge-1
    
  - type: node_restart
    time: 60
    target: sensor-5
```

**Acceptance Criteria**:
- [ ] All 4 event types work correctly
- [ ] Mesh detects node departure and updates topology
- [ ] Messages reroute around failed nodes
- [ ] Node restart rejoins mesh successfully
- [ ] Metrics track node state changes
- [ ] Crash differs from stop (no cleanup)

---

### Issue #6: Connection Events

**Title**: Implement connection drop/restore events

**Labels**: `enhancement`, `phase-2`, `events`, `network`, `P0`

**Description**:
Simulate connection failures and recovery to test mesh routing adaptation.

**Event Types**:
1. `connection_drop`: Break connection between two specific nodes
2. `connection_restore`: Re-establish previously dropped connection
3. `connection_quality`: Temporarily degrade connection (high latency/loss)

**Implementation**:
```cpp
class ConnectionEvent : public Event {
public:
  enum Action { DROP, RESTORE, DEGRADE };
  
  void execute(NodeManager& manager) override {
    auto node1 = manager.getNode(node_id_1);
    auto node2 = manager.getNode(node_id_2);
    
    switch (action) {
      case DROP:
        node1->disconnectFrom(node2);
        break;
      case RESTORE:
        node1->connectTo(node2);
        break;
      case DEGRADE:
        node1->setConnectionQuality(node2, quality_factor);
        break;
    }
  }
  
private:
  uint32_t node_id_1, node_id_2;
  Action action;
  float quality_factor;  // For DEGRADE
};
```

**YAML Examples**:
```yaml
events:
  - type: connection_drop
    time: 30
    nodes: [sensor-1, sensor-2]
    
  - type: connection_restore
    time: 60
    nodes: [sensor-1, sensor-2]
    
  - type: connection_quality
    time: 45
    nodes: [bridge-1, sensor-5]
    latency: 200  # ms
    packet_loss: 0.2  # 20%
```

**Acceptance Criteria**:
- [ ] Connections drop on command
- [ ] Mesh routing adapts to dropped connections
- [ ] Messages reroute around broken link
- [ ] Connections restore correctly
- [ ] Quality degradation applies specified conditions
- [ ] Mesh remains stable during connection changes

---

### Issue #7: Network Partition Events

**Title**: Implement network partition and healing

**Labels**: `enhancement`, `phase-2`, `events`, `network`, `P1`

**Description**:
Split mesh into isolated groups and later rejoin to test partition handling and split-brain scenarios.

**Requirements**:
- Define node groups for partition
- Isolate groups (no cross-group traffic)
- Heal partition (restore connectivity)
- Test split-brain with multiple bridges

**Implementation**:
```cpp
class NetworkPartitionEvent : public Event {
public:
  void execute(NodeManager& manager) override {
    if (healing) {
      manager.healPartition();
    } else {
      manager.createPartition(groups);
    }
  }
  
private:
  std::vector<std::vector<uint32_t>> groups;  // Node IDs per group
  bool healing;
};
```

**YAML Example**:
```yaml
events:
  - type: network_partition
    time: 30
    groups:
      - [sensor-1, sensor-2, sensor-3, bridge-1]
      - [sensor-4, sensor-5, sensor-6, bridge-2]
  
  - type: network_heal
    time: 90
```

**Acceptance Criteria**:
- [ ] Partition splits mesh correctly
- [ ] Nodes in different groups cannot communicate
- [ ] Each partition operates independently
- [ ] Heal restores full mesh connectivity
- [ ] Bridge election works per partition
- [ ] After heal, bridges coordinate correctly

---

## Phase 3: Firmware Integration (3 Issues)

### Issue #8: FirmwareBase Interface Design

**Title**: Design and implement FirmwareBase interface

**Labels**: `enhancement`, `phase-3`, `firmware`, `architecture`, `P0`

**Description**:
Create base class for all firmware implementations that provides Arduino-like lifecycle and painlessMesh integration.

**Interface Design**:
```cpp
// include/simulator/firmware/firmware_base.hpp
namespace simulator {
namespace firmware {

class FirmwareBase {
public:
  virtual ~FirmwareBase() = default;
  
  // Arduino lifecycle hooks
  virtual void setup() = 0;
  virtual void loop() = 0;
  
  // painlessMesh callback hooks
  virtual void onReceive(uint32_t from, String& msg) = 0;
  virtual void onNewConnection(uint32_t nodeId) {}
  virtual void onChangedConnections() {}
  virtual void onDroppedConnection(uint32_t nodeId) {}
  virtual void onNodeTimeAdjusted(int32_t offset) {}
  
  // Firmware metadata
  virtual String getName() const = 0;
  virtual String getVersion() const { return "1.0.0"; }
  
protected:
  // Initialized by VirtualNode
  painlessmesh::Mesh<painlessmesh::Connection>* mesh_ = nullptr;
  Scheduler* scheduler_ = nullptr;
  uint32_t nodeId_ = 0;
  
  // Helper methods
  void sendBroadcast(const String& msg);
  void sendSingle(uint32_t dest, const String& msg);
  uint32_t getNodeTime();
  uint32_t getNodeId() const { return nodeId_; }
  
  friend class VirtualNode;
};

} // namespace firmware
} // namespace simulator
```

**Helper Method Implementations**:
```cpp
void FirmwareBase::sendBroadcast(const String& msg) {
  mesh_->sendBroadcast(msg);
}

void FirmwareBase::sendSingle(uint32_t dest, const String& msg) {
  mesh_->sendSingle(dest, msg);
}

uint32_t FirmwareBase::getNodeTime() {
  return mesh_->getNodeTime();
}
```

**Acceptance Criteria**:
- [ ] Interface compiles cleanly
- [ ] Helper methods implemented and working
- [ ] Documentation complete (Doxygen comments)
- [ ] Example firmware can use interface
- [ ] No memory leaks in base class
- [ ] Virtual destructor properly implemented

---

### Issue #9: Firmware Factory and Loading

**Title**: Implement firmware factory and dynamic loading

**Labels**: `enhancement`, `phase-3`, `firmware`, `P0`

**Description**:
Create factory pattern for loading different firmware implementations based on configuration.

**Factory Implementation**:
```cpp
// include/simulator/firmware/firmware_factory.hpp
namespace simulator {
namespace firmware {

class FirmwareFactory {
public:
  using Creator = std::function<std::unique_ptr<FirmwareBase>()>;
  
  static FirmwareFactory& instance();
  
  void registerFirmware(const std::string& name, Creator creator);
  std::unique_ptr<FirmwareBase> create(const std::string& name);
  
  std::vector<std::string> getRegisteredTypes() const;
  
private:
  std::map<std::string, Creator> creators_;
};

// Registration helper template
template<typename T>
class FirmwareRegistrar {
public:
  FirmwareRegistrar(const std::string& name) {
    FirmwareFactory::instance().registerFirmware(name, []() {
      return std::make_unique<T>();
    });
  }
};

// Convenient registration macro
#define REGISTER_FIRMWARE(name, class_name) \
  static FirmwareRegistrar<class_name> registrar_##class_name(name);

} // namespace firmware
} // namespace simulator
```

**Usage Example**:
```cpp
// sensor_firmware.cpp
class SensorFirmware : public FirmwareBase {
  // ... implementation ...
};

REGISTER_FIRMWARE("sensor", SensorFirmware);
```

**Acceptance Criteria**:
- [ ] Factory creates firmware instances correctly
- [ ] Registration macro works
- [ ] Multiple firmware types can be registered
- [ ] Unknown firmware type throws clear error
- [ ] Factory is thread-safe (if needed)
- [ ] List registered types for debugging

---

### Issue #10: VirtualNode Firmware Integration

**Title**: Integrate firmware execution into VirtualNode

**Labels**: `enhancement`, `phase-3`, `firmware`, `core`, `P0`

**Description**:
Modify VirtualNode to load and execute firmware, calling setup() and loop() appropriately.

**VirtualNode Changes**:
```cpp
// include/simulator/virtual_node.hpp
class VirtualNode {
public:
  // ... existing methods ...
  
  void loadFirmware(std::unique_ptr<FirmwareBase> firmware);
  bool hasFirmware() const { return firmware_ != nullptr; }
  
  void start() {
    // ... existing start logic ...
    
    // Initialize firmware
    if (firmware_) {
      firmware_->mesh_ = &getMesh();
      firmware_->scheduler_ = scheduler_;
      firmware_->nodeId_ = node_id_;
      firmware_->setup();
    }
    
    running_ = true;
  }
  
  void update() {
    // Update mesh
    mesh_->update();
    
    // Call firmware loop
    if (firmware_) {
      firmware_->loop();
    }
    
    // Poll IO
    io_.poll();
  }
  
  void stop() {
    running_ = false;
    if (mesh_) {
      mesh_->stop();
    }
    // Firmware cleanup automatic via destructor
  }
  
private:
  std::unique_ptr<FirmwareBase> firmware_;
};
```

**Configuration Loading**:
```cpp
// In NodeManager or ConfigLoader
auto firmware = FirmwareFactory::instance().create(config.firmware);
node->loadFirmware(std::move(firmware));
```

**Acceptance Criteria**:
- [ ] Firmware loads from config file path/name
- [ ] setup() called exactly once on node start
- [ ] loop() called every update cycle
- [ ] Mesh callbacks routed to firmware correctly
- [ ] Multiple nodes with different firmware work simultaneously
- [ ] Firmware nullptr handled gracefully (nodes without firmware)

---

*Continue with Issues #11-#24 in similar detail...*

---

## Priority Order for Implementation

### Must Have (Week 1-2)
1. Issue #1: Network Latency
2. Issue #4: Event Engine
3. Issue #8: FirmwareBase Interface
4. Issue #9: Firmware Factory
5. Issue #10: VirtualNode Integration

### Should Have (Week 3-4)
6. Issue #11: SimpleBroadcast Firmware
7. Issue #13: Bridge Node Firmware
8. Issue #5: Node Lifecycle Events
9. Issue #14: Bridge Election Scenario

### Nice to Have (Week 5+)
10. Issue #2: Packet Loss
11. Issue #6: Connection Events
12. Issue #15-21: Advanced bridge and mesh testing
13. Issue #22-24: Test suite integration

---

## Quick Start Checklist

- [ ] Review all issues above
- [ ] Customize priorities for your needs
- [ ] Create GitHub issues (copy/paste)
- [ ] Add to project board
- [ ] Assign milestones
- [ ] Start with Issue #1

