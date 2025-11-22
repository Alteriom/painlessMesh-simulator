# Firmware-Based Testing Guide

## Overview

This guide explains how to use the painlessMesh simulator to test actual .ino firmware examples, enabling comprehensive validation of the painlessMesh library with real-world code patterns.

## Problem Statement

Recent painlessMesh bugs (like issue #160) were not caught before release because:
1. Tests didn't use actual .ino firmware patterns that users write
2. Edge cases only appeared in specific firmware configurations
3. No regression tests existed for reported issues
4. Testing required physical ESP32/ESP8266 hardware

## Solution

The simulator now supports loading Arduino .ino examples as firmware, allowing:
- **Regression Testing**: Tests fail with buggy library versions, pass when fixed
- **Real Firmware Validation**: Test actual code patterns users write
- **Issue-Specific Tests**: Each painlessMesh issue gets a dedicated test
- **Automated CI/CD**: No hardware required for comprehensive testing

## Architecture

### Components

```
simulator/firmware/
├── ino_firmware_wrapper.hpp  # Base class for .ino wrappers
├── bridge_ino_firmware.cpp    # Wrapper for bridge.ino
├── basic_ino_firmware.cpp     # Wrapper for basic.ino
└── [other .ino wrappers]

test/
└── test_bridge_internet_detection.cpp  # Tests for issue #160
```

### How It Works

1. **InoFirmwareWrapper**: Base class that wraps .ino functions
2. **Concrete Wrappers**: Each .ino example gets a C++ wrapper class
3. **Test Cases**: Catch2 tests that load firmware and verify behavior
4. **Regression Tests**: Tests target specific painlessMesh issues

## Creating a Firmware Wrapper

### Step 1: Create the Wrapper Class

```cpp
// src/firmware/my_example_ino_firmware.cpp
#include "simulator/firmware/ino_firmware_wrapper.hpp"
#include "simulator/firmware/firmware_factory.hpp"

namespace simulator {
namespace firmware {

class MyExampleInoFirmware : public InoFirmwareWrapper {
public:
  MyExampleInoFirmware() : InoFirmwareWrapper("my_example.ino") {}

protected:
  InoFirmwareInterface createInoInterface() override {
    InoFirmwareInterface iface;
    iface.setup = std::bind(&MyExampleInoFirmware::ino_setup, this);
    iface.loop = std::bind(&MyExampleInoFirmware::ino_loop, this);
    iface.receivedCallback = std::bind(&MyExampleInoFirmware::ino_receivedCallback,
                                       this, std::placeholders::_1, std::placeholders::_2);
    return iface;
  }

private:
  // Implement .ino functions
  void ino_setup() {
    auto* mesh = getMesh();
    auto* scheduler = getScheduler();
    
    // Your setup code from the .ino file
    std::cout << "[INO] my_example: setup on node " << getNodeId() << "\n";
    
    // Add tasks, register callbacks, etc.
  }
  
  void ino_loop() {
    // Your loop code from the .ino file
  }
  
  void ino_receivedCallback(uint32_t from, String& msg) {
    // Your message handling from the .ino file
  }
};

// Register firmware
REGISTER_FIRMWARE(MyExampleInoFirmware, MyExampleInoFirmware)

} // namespace firmware
} // namespace simulator
```

### Step 2: Add to Build System

Edit `CMakeLists.txt`:

```cmake
# Add to SIMULATOR_SOURCES
src/firmware/my_example_ino_firmware.cpp
```

### Step 3: Create Tests

```cpp
// test/test_my_example.cpp
#include <catch2/catch_test_macros.hpp>
#include "simulator/virtual_node.hpp"
#include "simulator/node_manager.hpp"

TEST_CASE("My example firmware test", "[firmware][my-example]") {
  boost::asio::io_context io;
  NodeManager manager(io);
  
  NodeConfig config;
  config.nodeId = 1001;
  config.meshPrefix = "TestMesh";
  config.meshPassword = "password";
  config.firmware = "MyExampleInoFirmware";
  
  auto node = manager.createNode(config);
  REQUIRE(node != nullptr);
  
  manager.startAll();
  
  // Run simulation
  for (int i = 0; i < 100; ++i) {
    manager.updateAll();
    io.poll();
  }
  
  // Verify behavior
  // ...
  
  manager.stopAll();
}
```

## Testing painlessMesh Issues

### Example: Issue #160 - Bridge Internet Detection

**The Bug**: `hasInternetConnection()` returns false on bridge nodes immediately after init, even when the bridge has internet connectivity.

**The Fix**: Override `hasInternetConnection()` to check the node's own WiFi status before checking knownBridges.

**The Test**:

```cpp
TEST_CASE("Bridge internet detection - Issue #160") {
  // Create bridge node with test firmware
  NodeConfig bridge_config;
  bridge_config.nodeId = 10001;
  bridge_config.firmware = "BridgeInoFirmware";
  
  auto bridge_node = manager.createNode(bridge_config);
  bridge_node->start();
  
  // Run one update cycle
  bridge_node->update();
  
  // CRITICAL: Bridge should report internet immediately after init
  // With bug (<1.8.14): would return false
  // With fix (>=1.8.14): returns true
  auto firmware = dynamic_cast<BridgeInoFirmware*>(bridge_node->getFirmware());
  REQUIRE(firmware->internet_check_immediately_after_init == true);
}
```

### Test Lifecycle

1. **Before Fix**: Test fails - demonstrates the bug exists
2. **After Fix**: Test passes - validates the fix works
3. **Regression Prevention**: Test continues to run in CI/CD

## Best Practices

### 1. Keep Wrappers Simple

Focus on wrapping the .ino functions directly. Don't add complex logic.

```cpp
// Good
void ino_setup() {
  auto* mesh = getMesh();
  mesh->setDebugMsgTypes(ERROR | STARTUP);
  mesh->init(MESH_PREFIX, MESH_PASSWORD, getScheduler(), MESH_PORT);
}

// Avoid - too much simulator-specific logic
void ino_setup() {
  if (simulation_mode == FAST) {
    speedup_time_scale();
  }
  // ...
}
```

### 2. Add Test Tracking Variables

Make firmware state observable for tests:

```cpp
class MyFirmware : public InoFirmwareWrapper {
public:
  // Test tracking
  bool setup_completed = false;
  uint32_t messages_sent = 0;
  uint32_t messages_received = 0;
  
private:
  void ino_setup() {
    // ... setup code ...
    setup_completed = true;
  }
  
  void ino_receivedCallback(uint32_t from, String& msg) {
    messages_received++;
    // ... handle message ...
  }
};
```

### 3. Use Meaningful Test Names

```cpp
// Good
TEST_CASE("Bridge detects internet immediately after init - Issue #160")

// Less clear
TEST_CASE("Test bridge")
```

### 4. Document Expected vs Actual Behavior

```cpp
// CRITICAL TEST for Issue #160:
// Expected (with fix): hasInternetConnection() returns true
// Actual (with bug): hasInternetConnection() returns false
// 
// The bug only checked knownBridges list, missing the node's own status
REQUIRE(firmware->has_internet_after_init == true);
```

## Running Tests

### Build and Test

```bash
# Build
mkdir -p build && cd build
cmake -G Ninja ..
ninja

# Run all tests
./bin/simulator_tests

# Run specific test suite
./bin/simulator_tests "[bridge][internet]"

# Run specific test case
./bin/simulator_tests "Bridge internet detection - Issue #160"
```

### Test Output

```
===============================================================================
test cases:  5 |  5 passed
assertions: 23 | 23 passed

===============================================================================
All tests passed
```

## CI/CD Integration

### GitHub Actions

```yaml
name: Firmware Tests

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
        with:
          submodules: recursive
      
      - name: Install Dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y libboost-all-dev libyaml-cpp-dev
      
      - name: Build
        run: |
          mkdir build && cd build
          cmake -G Ninja ..
          ninja
      
      - name: Run Firmware Tests
        run: |
          cd build
          ./bin/simulator_tests "[firmware]"
```

## Examples

### Available Firmware Wrappers

1. **BridgeInoFirmware** - Tests bridge functionality (issue #160)
2. **BasicInoFirmware** - Tests basic mesh operations

### Planned Additions

- EchoServerInoFirmware - Request/response patterns
- SensorNodeInoFirmware - Periodic data transmission
- BridgeFailoverInoFirmware - Multi-bridge scenarios

## Troubleshooting

### Build Errors

**Problem**: `'getMesh' was not declared in this scope`

**Solution**: Use the public `getMesh()` method from `InoFirmwareWrapper`:

```cpp
auto* mesh = getMesh();  // Correct
auto* mesh = mesh_;      // Wrong - private member
```

**Problem**: `cannot 'dynamic_cast' ... (target is not pointer or reference)`

**Solution**: Add pointer type:

```cpp
auto firmware = dynamic_cast<MyFirmware*>(node->getFirmware());  // Correct
auto firmware = dynamic_cast<MyFirmware>(node->getFirmware());   // Wrong
```

### Test Failures

**Problem**: Test passes when it should fail

**Check**:
1. Are you testing the right condition?
2. Is the firmware properly loaded?
3. Is the simulation running long enough?

**Problem**: Test fails unexpectedly

**Debug**:
1. Add logging to firmware wrapper
2. Check simulation timing
3. Verify mesh formation occurred

## Future Enhancements

1. **Automatic .ino Parsing**: Generate wrappers from .ino files
2. **WiFi Simulation**: Full WiFi connection/disconnection simulation
3. **Hardware Emulation**: More complete ESP32/ESP8266 API emulation
4. **Performance Testing**: Benchmark firmware under load
5. **Coverage Analysis**: Track which .ino lines are executed

## Contributing

When adding new firmware wrappers:

1. Create wrapper class following the pattern above
2. Add comprehensive tests for the firmware behavior
3. Document any painlessMesh issues the tests validate
4. Update this guide with your example

## References

- [painlessMesh Library](https://github.com/Alteriom/painlessMesh)
- [Issue #160](https://github.com/Alteriom/painlessMesh/issues/160)
- [Simulator Architecture](SIMULATOR_PLAN.md)
- [Contributing Guide](../CONTRIBUTING.md)

---

**Last Updated**: 2025-11-22
**Status**: Active Development
