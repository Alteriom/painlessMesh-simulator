# painlessMesh Library Validation Plan

## Executive Summary

This document outlines the comprehensive validation strategy for the painlessMesh library within the simulator. The goal is to ensure the simulator accurately validates **all non-hardware-related painlessMesh library functions** before extending support for custom application firmware.

## Problem Statement

The simulator should:
1. **Validate the painlessMesh library itself** - ensuring all core mesh functionality works correctly
2. **Then support custom firmware** - once library validation is proven at 100% coverage

Current state: We have application firmware examples (SimpleBroadcast, Echo Client/Server) but no systematic library validation firmware.

## Validation Philosophy

### Two-Tier Approach

**Tier 1: Library Validation Firmware (Baseline)**
- Systematically exercises ALL painlessMesh API functions
- Tests library behavior, not application logic
- Validates mesh protocol compliance
- Ensures simulation accuracy matches real hardware behavior
- Goal: 100% API coverage

**Tier 2: Application Firmware (Extension)**
- Built on validated library foundation
- Demonstrates real-world use cases
- Tests integration scenarios
- Examples: sensor nodes, bridges, custom protocols

## painlessMesh Library API Surface

### Core API Categories

Based on `external/painlessMesh/src/painlessmesh/mesh.hpp` and related files:

#### 1. Mesh Lifecycle & Configuration
- `init(Scheduler*, uint32_t nodeId)` - Initialize mesh
- `stop()` - Shutdown mesh
- `update()` - Process mesh operations
- `setRoot(bool)` - Set as root node
- `setContainsRoot(bool)` - Indicate mesh has root
- `isRoot()` - Check if node is root
- `setDebugMsgTypes(uint16_t)` - Configure logging

#### 2. Message Sending
- `sendSingle(uint32_t dest, String msg)` - Send to specific node
- `sendSingle(uint32_t dest, String msg, uint8_t priority)` - Send with priority
- `sendBroadcast(String msg, bool includeSelf=false)` - Broadcast to all
- `sendBroadcast(String msg, uint8_t priority, bool includeSelf=false)` - Broadcast with priority
- **Validation needed:**
  - Message delivery reliability
  - Routing through multi-hop mesh
  - Priority ordering
  - includeSelf flag behavior

#### 3. Message Reception
- `onReceive(receivedCallback_t)` - Register receive callback
- **Validation needed:**
  - Single messages received correctly
  - Broadcast messages received by all nodes
  - Message order preservation
  - Callback invocation timing

#### 4. Connection Management
- `onNewConnection(newConnectionCallback_t)` - Register connection callback
- `onDroppedConnection(droppedConnectionCallback_t)` - Register disconnect callback
- `onChangedConnections(changedConnectionsCallback_t)` - Register topology change callback
- **Validation needed:**
  - Callbacks fired on actual events
  - Connection establishment handshake
  - Graceful disconnect handling
  - Topology convergence after changes

#### 5. Time Synchronization
- `getNodeTime()` - Get synchronized mesh time (inherited from MeshTime)
- `onNodeTimeAdjusted(nodeTimeAdjustedCallback_t)` - Time adjustment callback
- `startDelayMeas(uint32_t nodeId)` - Measure network delay
- `onNodeDelayReceived(nodeDelayCallback_t)` - Delay measurement callback
- **Validation needed:**
  - Time synchronization across nodes
  - Time adjustment convergence
  - Delay measurement accuracy
  - Time drift handling

#### 6. Topology & Node Discovery
- `getNodeList()` - Get list of connected nodes (inherited from MeshTime)
- `asNodeTree()` - Get topology tree representation
- **Validation needed:**
  - Node list accuracy
  - Topology tree structure correctness
  - Multi-hop connectivity tracking
  - Partition detection

#### 7. Bridge Functionality
- `onBridgeStatusChanged(bridgeStatusChangedCallback_t)` - Bridge status callback
- `hasInternetConnection()` - Check Internet availability
- `getBridges()` - Get all known bridges
- `getPrimaryBridge()` - Get best bridge
- **Validation needed:**
  - Bridge status propagation
  - Bridge selection logic
  - Failover behavior
  - Status update timing

#### 8. Message Queue & Flow Control
- Internal: Message queue management
- Internal: Priority queue ordering
- Internal: Queue overflow handling
- **Validation needed:**
  - Queue size limits (MAX_MESSAGE_QUEUE)
  - Message dropping under load
  - Priority enforcement
  - Memory pressure handling (MIN_FREE_MEMORY)

#### 9. Protocol Handling
- Internal: Protocol package types (SINGLE, BROADCAST, TIME_SYNC, etc.)
- Internal: Package routing
- Internal: Loop detection
- **Validation needed:**
  - All protocol types handled
  - Routing algorithm correctness
  - Loop prevention
  - Split-brain recovery

#### 10. OTA Updates (Optional)
- `offerOTA()` - Offer OTA update
- `initOTASend()` - Initialize OTA send
- `initOTAReceive()` - Initialize OTA receive
- **Validation needed:**
  - OTA package distribution
  - Multi-part transfer
  - Update verification

## Library Validation Firmware Design

### Architecture

```cpp
/**
 * @file library_validation_firmware.hpp
 * @brief Comprehensive painlessMesh library validation firmware
 * 
 * This firmware systematically exercises all painlessMesh APIs to ensure
 * the simulator accurately represents real hardware behavior.
 */
class LibraryValidationFirmware : public FirmwareBase {
public:
  LibraryValidationFirmware();
  
  // Core lifecycle
  void setup() override;
  void loop() override;
  
  // Callbacks
  void onReceive(uint32_t from, String& msg) override;
  void onNewConnection(uint32_t nodeId) override;
  void onChangedConnections() override;
  void onNodeTimeAdjusted(int32_t offset) override;
  
  // Validation test execution
  void runAllTests();
  ValidationReport generateReport();
  
private:
  // Test suites for each API category
  void testMeshLifecycle();
  void testMessageSending();
  void testMessageReception();
  void testConnectionManagement();
  void testTimeSynchronization();
  void testTopologyDiscovery();
  void testBridgeFunctionality();
  void testMessageQueue();
  void testProtocolHandling();
  
  // Validation state tracking
  ValidationState state_;
  std::vector<TestResult> results_;
};
```

### Test Scenarios

#### Scenario 1: Basic Mesh Formation
- **Objective:** Validate mesh initialization and node discovery
- **Nodes:** 5 nodes
- **Tests:**
  - All nodes initialize successfully
  - All nodes discover each other
  - Topology converges to full mesh
  - Connection callbacks fire correctly

#### Scenario 2: Message Routing
- **Objective:** Validate single and broadcast message delivery
- **Nodes:** 10 nodes in chain topology (linear)
- **Tests:**
  - Node 1 sends to Node 10 (9 hops)
  - Message routes correctly through intermediaries
  - Broadcast reaches all nodes
  - No message duplication
  - Message order preserved

#### Scenario 3: Time Synchronization
- **Objective:** Validate mesh time convergence
- **Nodes:** 8 nodes with randomized initial time offsets
- **Tests:**
  - Times converge within tolerance (10ms)
  - Time adjustment callbacks fire
  - Delay measurements accurate
  - No time drift over 60 seconds

#### Scenario 4: Network Resilience
- **Objective:** Validate mesh recovery from node/link failures
- **Nodes:** 12 nodes
- **Tests:**
  - Remove central node - mesh reforms around gap
  - Remove edge node - minimal topology change
  - Partition network - detect split
  - Heal partition - mesh converges
  - Drop callbacks fire correctly

#### Scenario 5: Priority Messaging
- **Objective:** Validate priority queue ordering
- **Nodes:** 3 nodes
- **Tests:**
  - Send mixed priority messages
  - CRITICAL messages delivered first
  - LOW messages delivered last
  - Order within same priority preserved

#### Scenario 6: Root Node Behavior
- **Objective:** Validate root node functionality
- **Nodes:** 15 nodes, 1 designated root
- **Tests:**
  - Mesh forms faster with root
  - Topology stabilizes around root
  - Root node reports isRoot() == true
  - Other nodes report isRoot() == false
  - setContainsRoot() affects convergence speed

#### Scenario 7: Large-Scale Stability
- **Objective:** Validate library at scale
- **Nodes:** 100+ nodes
- **Tests:**
  - All nodes join mesh successfully
  - Topology remains stable
  - Messages route correctly
  - No memory leaks
  - No performance degradation

#### Scenario 8: Bridge Functionality
- **Objective:** Validate bridge status propagation
- **Nodes:** 10 nodes, 1 bridge node
- **Tests:**
  - Bridge status broadcasts received
  - hasInternetConnection() reports correctly
  - Primary bridge selected correctly
  - Status callback fires on changes
  - Bridge failover works

## Validation Metrics

### Coverage Metrics

Track API coverage:
```cpp
struct APICoverage {
  uint32_t total_apis = 0;        // Total API functions
  uint32_t tested_apis = 0;       // APIs with test coverage
  uint32_t passed_apis = 0;       // APIs passing validation
  uint32_t failed_apis = 0;       // APIs failing validation
  
  std::map<std::string, bool> api_status;  // Per-API pass/fail
};
```

### Validation Report

```cpp
struct ValidationReport {
  // Summary
  bool all_tests_passed = false;
  uint32_t total_tests = 0;
  uint32_t passed_tests = 0;
  uint32_t failed_tests = 0;
  
  // API Coverage
  APICoverage coverage;
  
  // Performance
  uint64_t total_messages_sent = 0;
  uint64_t total_messages_received = 0;
  uint64_t message_loss_count = 0;
  double message_loss_rate = 0.0;
  
  // Timing
  uint32_t avg_time_sync_error_us = 0;
  uint32_t max_time_sync_error_us = 0;
  uint32_t avg_message_latency_ms = 0;
  
  // Topology
  uint32_t topology_change_count = 0;
  uint32_t avg_convergence_time_ms = 0;
  
  // Detailed results
  std::vector<TestResult> test_results;
};
```

## Implementation Roadmap

### Phase 1: Foundation (Week 1)
- [x] Document all painlessMesh APIs
- [ ] Create LibraryValidationFirmware base structure
- [ ] Implement ValidationState tracking
- [ ] Create basic test scenarios (1-3)

### Phase 2: Core Validation (Week 2)
- [ ] Implement message routing tests
- [ ] Implement time synchronization tests
- [ ] Implement connection management tests
- [ ] Create validation reporting

### Phase 3: Advanced Validation (Week 3)
- [ ] Implement priority messaging tests
- [ ] Implement root node tests
- [ ] Implement resilience tests (failure scenarios)
- [ ] Add bridge functionality tests

### Phase 4: Scale & Performance (Week 4)
- [ ] Large-scale stability tests (100+ nodes)
- [ ] Performance benchmarking
- [ ] Memory leak detection
- [ ] Load testing

### Phase 5: Documentation & Examples (Week 5)
- [ ] Complete API validation documentation
- [ ] Document validated vs untested APIs
- [ ] Create application firmware guidelines
- [ ] Example scenarios for validated APIs

## Integration with Existing Tests

### Relationship to Unit Tests

**Unit tests** (test/*.cpp):
- Test individual simulator components
- Mock mesh interactions
- Fast, isolated testing

**Library validation firmware**:
- Integration testing of painlessMesh library
- Tests actual mesh behavior
- Multi-node scenarios
- Realistic timing

### Relationship to Application Firmware

**Library validation firmware**:
- Tests library functionality
- Systematic API coverage
- Pass/fail reporting
- Foundation for simulator confidence

**Application firmware** (examples/firmware/):
- Demonstrates use cases
- Tests specific scenarios
- User-facing examples
- Built on validated foundation

## Success Criteria

### Minimum Viable Validation

Before declaring simulator ready for application firmware:
- [ ] 100% of Core API Category functions tested
- [ ] 90%+ of Extended API functions tested
- [ ] All basic scenarios (1-6) passing
- [ ] Zero critical bugs in message routing
- [ ] Time synchronization within 10ms tolerance
- [ ] Mesh recovery from single node failure

### Full Validation

For production-ready simulator:
- [ ] 100% of all API functions tested
- [ ] All scenarios (1-8) passing
- [ ] Large-scale stability proven (100+ nodes)
- [ ] Performance benchmarks documented
- [ ] No known library bugs
- [ ] Validation report automation

## Continuous Validation

### CI/CD Integration

Run library validation on every commit:
```yaml
name: Library Validation

on: [push, pull_request]

jobs:
  validate:
    runs-on: ubuntu-latest
    steps:
      - name: Run Library Validation Firmware
        run: |
          ./painlessmesh-simulator \
            --config scenarios/library_validation_full.yaml \
            --log-level WARN
      
      - name: Check Validation Report
        run: |
          if [ ! -f results/validation_report.json ]; then
            echo "Validation report not generated!"
            exit 1
          fi
          
          passed=$(jq '.all_tests_passed' results/validation_report.json)
          if [ "$passed" != "true" ]; then
            echo "Library validation failed!"
            jq '.' results/validation_report.json
            exit 1
          fi
```

### Regression Detection

Track validation metrics over time:
- Message loss rate
- Time synchronization error
- Convergence time
- API coverage percentage

Alert on regressions.

## Appendix A: painlessMesh Test Reference

The painlessMesh library includes existing tests in `external/painlessMesh/test/boost/tcp_integration.cpp`:

```cpp
// Existing tests we should replicate in simulator:
SCENARIO("We can setup and connect two meshes over localport")
SCENARIO("The MeshTest class works correctly")
SCENARIO("We can send a message using our Nodes class")
SCENARIO("Time sync works")
SCENARIO("Rooting works")
SCENARIO("Network loops are detected")
SCENARIO("Disconnects are detected and forwarded")
SCENARIO("Disconnects don't lead to crashes")
```

These tests validate core functionality and should be our baseline.

## Appendix B: API Coverage Checklist

### Mesh Class API
- [ ] `init(Scheduler*, uint32_t)` - Initialization
- [ ] `stop()` - Shutdown
- [ ] `update()` - Update loop
- [ ] `setRoot(bool)` - Set root status
- [ ] `setContainsRoot(bool)` - Indicate root presence
- [ ] `isRoot()` - Query root status
- [ ] `setDebugMsgTypes(uint16_t)` - Configure logging
- [ ] `sendSingle(uint32_t, String)` - Send message
- [ ] `sendSingle(uint32_t, String, uint8_t)` - Send with priority
- [ ] `sendBroadcast(String, bool)` - Broadcast message
- [ ] `sendBroadcast(String, uint8_t, bool)` - Broadcast with priority
- [ ] `startDelayMeas(uint32_t)` - Measure delay
- [ ] `onReceive(receivedCallback_t)` - Register receive callback
- [ ] `onNewConnection(newConnectionCallback_t)` - Register connection callback
- [ ] `onDroppedConnection(droppedConnectionCallback_t)` - Register disconnect callback
- [ ] `onChangedConnections(changedConnectionsCallback_t)` - Register topology callback
- [ ] `onNodeTimeAdjusted(nodeTimeAdjustedCallback_t)` - Register time callback
- [ ] `onNodeDelayReceived(nodeDelayCallback_t)` - Register delay callback
- [ ] `onBridgeStatusChanged(bridgeStatusChangedCallback_t)` - Register bridge callback
- [ ] `hasInternetConnection()` - Check Internet
- [ ] `getBridges()` - Get bridge list
- [ ] `getPrimaryBridge()` - Get primary bridge

### MeshTime API (inherited)
- [ ] `getNodeTime()` - Get synchronized time
- [ ] `getNodeList()` - Get node list

### Layout API
- [ ] `asNodeTree()` - Get topology tree
- [ ] Size calculation
- [ ] Tree traversal

### Protocol API (internal)
- [ ] Single message protocol
- [ ] Broadcast message protocol
- [ ] Time sync protocol
- [ ] Bridge status protocol
- [ ] Routing protocol

---

**Document Version:** 1.0  
**Last Updated:** 2025-11-14  
**Status:** Draft - Awaiting Implementation
