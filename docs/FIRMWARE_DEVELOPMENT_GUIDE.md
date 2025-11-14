# Firmware Development Guide

## Overview

This guide explains the two-tier firmware architecture in the painlessMesh simulator and how to develop firmware for each tier.

## Two-Tier Architecture

### Tier 1: Library Validation Firmware (Foundation)

**Purpose**: Validate that the painlessMesh library itself works correctly in the simulator.

**Goal**: Ensure 100% coverage of all non-hardware-related painlessMesh APIs before any application firmware is trusted.

**Key Characteristics**:
- Tests library functionality, not application logic
- Systematic API coverage
- Pass/fail validation reporting
- Foundation for simulator confidence

**Example**: `LibraryValidationFirmware`

### Tier 2: Application Firmware (Extension)

**Purpose**: Demonstrate real-world use cases and integration scenarios.

**Goal**: Show how to build practical mesh applications on top of validated library functions.

**Key Characteristics**:
- Built on validated library foundation
- Application-specific logic
- Real-world patterns and practices
- User-facing examples

**Examples**: `SimpleBroadcastFirmware`, `EchoServerFirmware`, sensor nodes, bridges

## When to Use Each Tier

### Use Library Validation Firmware When:

1. **Testing Library Functionality**
   - Verifying painlessMesh API behavior
   - Ensuring simulator accuracy
   - Finding library bugs
   - Regression testing library changes

2. **Baseline Confidence**
   - Before trusting simulator for application development
   - Before releasing simulator versions
   - After painlessMesh library updates

3. **API Coverage**
   - Documenting which APIs are validated
   - Identifying untested library features
   - Planning library support roadmap

### Use Application Firmware When:

1. **Building Applications**
   - Creating actual IoT solutions
   - Testing firmware logic
   - Validating integration patterns

2. **Examples & Tutorials**
   - Teaching mesh programming
   - Demonstrating best practices
   - Showing real-world scenarios

3. **Custom Testing**
   - Application-specific edge cases
   - Integration testing
   - Performance benchmarking

## Library Validation Firmware Development

### Structure

```cpp
class MyLibraryValidationFirmware : public FirmwareBase {
public:
  MyLibraryValidationFirmware();
  
  void setup() override;
  void loop() override;
  
  // Test execution
  void runValidationTests();
  ValidationReport generateReport();
  
private:
  // Test suites for API categories
  void testMeshLifecycle();
  void testMessageSending();
  void testConnectionManagement();
  // ... more test suites
  
  ValidationState state_;
  std::vector<TestResult> results_;
};
```

### Best Practices

1. **Systematic Testing**
   - Test each API in isolation
   - Document what is being tested
   - Use consistent test patterns

2. **Clear Reporting**
   - Report pass/fail for each API
   - Include error details
   - Track coverage metrics

3. **Reproducible Results**
   - Use deterministic scenarios
   - Log all test conditions
   - Make tests repeatable

4. **Progressive Validation**
   - Start with basic APIs (init, getNodeId)
   - Build up to complex scenarios (multi-hop routing)
   - Test failure modes last

### Example Test Pattern

```cpp
void MyValidationFirmware::testMessageSending() {
  // Test: sendBroadcast
  auto result = testAPI("sendBroadcast()", [this]() {
    String msg = "TEST_MESSAGE";
    sendBroadcast(msg);
    recordMessage(true);
    return true;  // Test passed if no exception
  });
  
  report_.addResult(result);
  report_.coverage.recordTest("sendBroadcast()", result.passed);
}
```

## Application Firmware Development

### Structure

```cpp
class MySensorFirmware : public FirmwareBase {
public:
  MySensorFirmware();
  
  void setup() override {
    // Application initialization
    setupSensor();
    scheduleSensorReading();
  }
  
  void loop() override {
    // Application logic
  }
  
  void onReceive(uint32_t from, String& msg) override {
    // Handle application messages
  }

private:
  void readSensor();
  void sendSensorData();
  
  // Application state
  Task sensor_task_;
  float last_reading_;
};
```

### Best Practices

1. **Use Validated APIs**
   - Only use APIs validated by library validation firmware
   - Check validation report for API status
   - Document dependencies on library features

2. **Application Logic**
   - Focus on your application, not library testing
   - Implement business logic clearly
   - Use well-defined message formats

3. **Error Handling**
   - Handle network failures gracefully
   - Implement retry logic
   - Use timeouts appropriately

4. **Performance**
   - Don't block in loop()
   - Use scheduler for periodic tasks
   - Minimize memory allocations

### Example Application Pattern

```cpp
void MySensorFirmware::setup() {
  // Initialize sensor hardware (mocked in simulator)
  pinMode(SENSOR_PIN, INPUT);
  
  // Schedule periodic sensor reading
  sensor_task_.set(30000, TASK_FOREVER, [this]() {
    readAndSendSensorData();
  });
  
  scheduler_->addTask(sensor_task_);
  sensor_task_.enable();
}

void MySensorFirmware::readAndSendSensorData() {
  // Read sensor
  float temp = analogRead(SENSOR_PIN) * 0.1;
  
  // Create message (e.g., JSON)
  String msg = "{\"temp\":" + String(temp) + ",\"node\":" + 
               String(node_id_) + "}";
  
  // Send to mesh
  sendBroadcast(msg);
}
```

## API Validation Status

### Core APIs (Validated ✅)

Based on `LibraryValidationFirmware`:

- ✅ `init()` - Mesh initialization
- ✅ `getNodeId()` - Node ID query
- ✅ `getNodeTime()` - Time query
- ✅ `getNodeList()` - Node list query
- ✅ `sendBroadcast(String)` - Broadcast messaging
- ✅ `sendSingle(uint32_t, String)` - Single-target messaging
- ✅ `onReceive()` - Message reception callback
- ✅ `onNewConnection()` - Connection callback
- ✅ `onChangedConnections()` - Topology callback
- ✅ `onNodeTimeAdjusted()` - Time sync callback

### Extended APIs (To Be Validated ⏳)

Not yet covered by validation firmware:

- ⏳ `sendSingle(uint32_t, String, uint8_t)` - Priority messaging
- ⏳ `sendBroadcast(String, uint8_t, bool)` - Priority broadcast
- ⏳ `setRoot(bool)` - Root node configuration
- ⏳ `setContainsRoot(bool)` - Root presence indication
- ⏳ `isRoot()` - Root status query
- ⏳ `startDelayMeas(uint32_t)` - Delay measurement
- ⏳ `onNodeDelayReceived()` - Delay callback
- ⏳ Bridge APIs (hasInternetConnection, getBridges, getPrimaryBridge)
- ⏳ OTA APIs (offerOTA, initOTASend, initOTAReceive)

### Hardware-Specific APIs (Not Validated ❌)

Cannot be validated in simulator:

- ❌ WiFi channel scanning
- ❌ Physical signal strength (RSSI)
- ❌ Hardware cryptography
- ❌ Flash storage
- ❌ Real-time constraints

## Development Workflow

### For Library Validation

1. **Identify Untested API**
   - Check `LIBRARY_VALIDATION_PLAN.md` for coverage
   - Find API in painlessMesh headers

2. **Add Test**
   - Create test method in `LibraryValidationFirmware`
   - Implement test logic
   - Add to appropriate test phase

3. **Verify Test**
   - Run library_validation.yaml scenario
   - Check validation report
   - Fix any issues

4. **Update Documentation**
   - Mark API as validated in docs
   - Document any limitations
   - Update coverage metrics

### For Application Firmware

1. **Check API Availability**
   - Review validated APIs list
   - Ensure needed APIs are validated
   - Request validation if needed

2. **Implement Firmware**
   - Extend `FirmwareBase`
   - Implement required methods
   - Add application logic

3. **Create Scenario**
   - Write YAML scenario file
   - Configure nodes with firmware
   - Define test topology

4. **Test Firmware**
   - Run scenario in simulator
   - Verify behavior
   - Check metrics/logs

5. **Document & Share**
   - Add README for firmware
   - Document configuration options
   - Provide usage examples

## Configuration

### Library Validation Firmware Config

```yaml
nodes:
  - firmware: "library_validation"
    firmware_config:
      role: "coordinator"  # or "participant"
      test_duration: "30"  # seconds per phase
      enable_detailed_logging: "true"
```

### Application Firmware Config

```yaml
nodes:
  - firmware: "my_sensor_firmware"
    firmware_config:
      sensor_interval: "30000"  # milliseconds
      sensor_pin: "34"
      transmit_power: "high"
```

## Testing Guidelines

### Library Validation Tests

- **Isolation**: Test one API at a time
- **Coverage**: Test all code paths
- **Edge Cases**: Test boundary conditions
- **Failure Modes**: Test error handling
- **Repeatability**: Tests must be deterministic

### Application Firmware Tests

- **Integration**: Test with other nodes
- **Scenarios**: Test realistic conditions
- **Performance**: Test at scale
- **Resilience**: Test failure recovery
- **Usability**: Test user workflows

## Metrics & Reporting

### Library Validation Metrics

```cpp
struct ValidationReport {
  bool all_tests_passed;
  uint32_t total_tests;
  uint32_t passed_tests;
  uint32_t failed_tests;
  
  APICoverage coverage;  // API-level tracking
  
  // Message statistics
  uint64_t total_messages_sent;
  uint64_t total_messages_received;
  double message_loss_rate;
  
  // Timing metrics
  uint32_t avg_time_sync_error_us;
  uint32_t max_time_sync_error_us;
};
```

### Application Firmware Metrics

Use standard node metrics:
- Message counts
- Connection counts
- Uptime
- Custom application metrics

## Common Patterns

### Coordinator/Participant Pattern

**Use Case**: One node orchestrates tests, others respond.

**Library Validation Example**:
```yaml
nodes:
  - id: "coordinator"
    firmware_config:
      role: "coordinator"
  
  - template: "participant"
    count: 9
    firmware_config:
      role: "participant"
```

**Application Example**:
```yaml
nodes:
  - id: "gateway"
    firmware: "mqtt_bridge"
    firmware_config:
      role: "bridge"
  
  - template: "sensor"
    count: 20
    firmware: "sensor_node"
```

### Progressive Testing Pattern

**Use Case**: Tests build on previous results.

**Implementation**:
```cpp
enum class TestPhase {
  INITIALIZATION,   // Basic setup
  CONNECTIVITY,     // Connection establishment
  MESSAGING,        // Message passing
  ADVANCED          // Complex scenarios
};

void progressToNextPhase() {
  // Complete current phase
  // Start next phase
  // Update test state
}
```

## FAQ

### Q: When should I write library validation firmware vs application firmware?

**A**: Write library validation firmware when testing painlessMesh APIs themselves. Write application firmware when building a specific IoT solution.

### Q: Can I use untested APIs in my application firmware?

**A**: You can, but understand that simulator behavior may not match real hardware for those APIs. Request validation if needed.

### Q: How do I know which APIs are validated?

**A**: Check the "API Validation Status" section in this document and review the latest `ValidationReport` from running `library_validation.yaml`.

### Q: Should I add tests to LibraryValidationFirmware?

**A**: Yes, if you discover untested painlessMesh APIs or want to improve coverage. Follow the contribution guidelines.

### Q: Can application firmware also do validation?

**A**: Application firmware can include sanity checks, but comprehensive library validation belongs in dedicated validation firmware.

### Q: What if I find a library bug?

**A**: Report it! If found via validation firmware, we can add a test. If found via application firmware, consider adding a validation test to prevent regression.

## Examples

### Library Validation Firmware

See: `src/firmware/library_validation_firmware.cpp`
Scenario: `examples/scenarios/library_validation.yaml`

### Application Firmware

- **SimpleBroadcastFirmware**: Basic broadcast example
- **EchoServerFirmware**: Request/response pattern
- **EchoClientFirmware**: Client implementation

More examples in `examples/firmware/` directory.

## References

- [LIBRARY_VALIDATION_PLAN.md](LIBRARY_VALIDATION_PLAN.md) - Complete validation strategy
- [painlessMesh Documentation](https://gitlab.com/painlessMesh/painlessMesh)
- [painlessMesh API Reference](https://gitlab.com/painlessMesh/painlessMesh/-/wikis/home)

---

**Document Version:** 1.0  
**Last Updated:** 2025-11-14  
**Status:** Active
