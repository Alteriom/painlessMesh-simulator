# Integrating the Simulator into Your Firmware Project

## Overview

This guide explains how to integrate the painlessMesh simulator into your **existing ESP32/ESP8266 firmware project** so you can test your mesh application at scale without physical hardware.

**Use Case**: You have an existing firmware project (PlatformIO, Arduino IDE, or custom build) that uses painlessMesh, and you want to validate it with 100+ virtual nodes before deploying to hardware.

## Quick Start

**Goal**: Test your existing firmware with the simulator in 3 steps:

1. **Adapt your firmware** - Make it simulator-compatible
2. **Add test harness** - Create a simulator integration point
3. **Run tests** - Validate with 100+ nodes locally

## Prerequisites

- Existing ESP32/ESP8266 firmware using painlessMesh
- Basic understanding of C++ and CMake (or willingness to learn)
- Development environment setup (see [Getting Started](GETTING_STARTED.md))

## Understanding the Integration

### Two Integration Approaches

#### Approach 1: External Project (Recommended)
Keep simulator as separate testing project:
```
your-firmware-project/
├── src/
│   └── main.cpp (ESP32 firmware)
├── lib/
│   └── MyMeshApp/ (your application code)
├── test/
│   └── simulator/ (simulator test harness)
│       ├── CMakeLists.txt
│       ├── test_firmware.cpp
│       └── scenarios/
└── platformio.ini
```

**Pros**: Clean separation, no impact on firmware build
**Best for**: Existing projects, team development, CI/CD

#### Approach 2: Embedded Integration
Add simulator support directly into firmware:
```
your-firmware-project/
├── src/
│   └── main.cpp (ESP32 firmware)
├── lib/
│   └── MyMeshApp/ (simulator-aware)
├── simulator/ (simulator project)
│   ├── CMakeLists.txt
│   └── main.cpp
└── platformio.ini
```

**Pros**: Single repository, shared code
**Best for**: New projects, simulator-first development

## Approach 1: External Project Integration (Recommended)

### Step 1: Organize Your Firmware Code

Extract your application logic from `main.cpp` into reusable classes:

**Before** - Typical Arduino-style firmware:
```cpp
// src/main.cpp - TIGHTLY COUPLED TO ESP32
#include <Arduino.h>
#include <painlessMesh.h>

painlessMesh mesh;
Scheduler userScheduler;

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received from %u: %s\n", from, msg.c_str());
}

void setup() {
  Serial.begin(115200);
  mesh.init("MyMesh", "password", &userScheduler, 5555);
  mesh.onReceive(&receivedCallback);
}

void loop() {
  mesh.update();
}
```

**After** - Organized for testing:
```cpp
// lib/MyMeshApp/MyMeshApp.hpp - REUSABLE APPLICATION LOGIC
#pragma once
#include <painlessMesh.h>

class MyMeshApp {
public:
  MyMeshApp(painlessMesh* mesh, Scheduler* scheduler);
  
  void setup();
  void loop();
  
  // Message handlers
  void onReceive(uint32_t from, String& msg);
  void onNewConnection(uint32_t nodeId);
  void onChangedConnections();

private:
  painlessMesh* mesh_;
  Scheduler* scheduler_;
  uint32_t nodeId_;
  
  // Your application logic
  Task sendTask_;
  void sendMessage();
};

// lib/MyMeshApp/MyMeshApp.cpp
#include "MyMeshApp.hpp"

MyMeshApp::MyMeshApp(painlessMesh* mesh, Scheduler* scheduler)
  : mesh_(mesh), scheduler_(scheduler) {
  nodeId_ = mesh_->getNodeId();
}

void MyMeshApp::setup() {
  // Setup periodic tasks
  sendTask_.set(30000, TASK_FOREVER, [this]() { sendMessage(); });
  scheduler_->addTask(sendTask_);
  sendTask_.enable();
}

void MyMeshApp::loop() {
  // Fast loop logic (if any)
}

void MyMeshApp::onReceive(uint32_t from, String& msg) {
  // Handle messages
}

void MyMeshApp::onNewConnection(uint32_t nodeId) {
  // Handle new connections
}

void MyMeshApp::onChangedConnections() {
  // Handle topology changes
}

void MyMeshApp::sendMessage() {
  String msg = "Hello from node " + String(nodeId_);
  mesh_->sendBroadcast(msg);
}

// src/main.cpp - ESP32 ENTRY POINT
#include <Arduino.h>
#include <MyMeshApp.hpp>

painlessMesh mesh;
Scheduler userScheduler;
MyMeshApp* app;

void setup() {
  Serial.begin(115200);
  
  mesh.init("MyMesh", "password", &userScheduler, 5555);
  
  app = new MyMeshApp(&mesh, &userScheduler);
  app->setup();
  
  mesh.onReceive([](uint32_t from, String& msg) {
    app->onReceive(from, msg);
  });
  mesh.onNewConnection([](uint32_t nodeId) {
    app->onNewConnection(nodeId);
  });
  mesh.onChangedConnections([]() {
    app->onChangedConnections();
  });
}

void loop() {
  mesh.update();
  app->loop();
}
```

### Step 2: Create Simulator Test Harness

Create `test/simulator/` directory with simulator-specific code:

```cpp
// test/simulator/my_mesh_app_firmware.hpp
#pragma once
#include "simulator/firmware/firmware_base.hpp"
#include <MyMeshApp.hpp>  // Your application code

namespace simulator {
namespace firmware {

class MyMeshAppFirmware : public FirmwareBase {
public:
  MyMeshAppFirmware() : FirmwareBase("MyMeshApp") {}
  
  void setup() override {
    // Create your app instance with simulator mesh
    app_ = std::make_unique<MyMeshApp>(mesh_, scheduler_);
    app_->setup();
  }
  
  void loop() override {
    app_->loop();
  }
  
  void onReceive(uint32_t from, String& msg) override {
    app_->onReceive(from, msg);
  }
  
  void onNewConnection(uint32_t nodeId) override {
    app_->onNewConnection(nodeId);
  }
  
  void onChangedConnections() override {
    app_->onChangedConnections();
  }

private:
  std::unique_ptr<MyMeshApp> app_;
};

} // namespace firmware
} // namespace simulator
```

### Step 3: Create Test Firmware Registration

```cpp
// test/simulator/my_mesh_app_firmware.cpp
#include "my_mesh_app_firmware.hpp"
#include "simulator/firmware/firmware_factory.hpp"

using namespace simulator::firmware;

// Auto-register firmware with factory
REGISTER_FIRMWARE(MyMeshApp, MyMeshAppFirmware)
```

### Step 4: Create CMakeLists.txt for Tests

```cmake
# test/simulator/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)

# Add simulator as subdirectory (or use find_package if installed)
add_subdirectory(../../external/painlessMesh-simulator simulator)

# Your firmware library (shared between ESP32 and simulator)
add_library(my_mesh_app
  ../../lib/MyMeshApp/MyMeshApp.cpp
)

target_include_directories(my_mesh_app PUBLIC
  ../../lib
  ${PAINLESSMESH_PATH}/src
)

# Test firmware adapter
add_library(my_mesh_app_firmware
  my_mesh_app_firmware.cpp
)

target_link_libraries(my_mesh_app_firmware
  PUBLIC
    my_mesh_app
    simulator_firmware
)

# Link test firmware into simulator executable
target_link_libraries(painlessmesh-simulator
  PRIVATE
    my_mesh_app_firmware
)
```

### Step 5: Create Test Scenarios

Create scenario files to test your firmware:

```yaml
# test/simulator/scenarios/basic_test.yaml
simulation:
  name: "MyMeshApp Basic Test"
  duration: 60
  time_scale: 5.0  # Run 5x faster

nodes:
  - id: "node1"
    firmware: "MyMeshApp"
    config:
      mesh_prefix: "MyMesh"
      mesh_password: "password"
      mesh_port: 5555

  - id: "node2"
    firmware: "MyMeshApp"
    config:
      mesh_prefix: "MyMesh"
      mesh_password: "password"
      mesh_port: 5555

  - id: "node3"
    firmware: "MyMeshApp"
    config:
      mesh_prefix: "MyMesh"
      mesh_password: "password"
      mesh_port: 5555

topology:
  type: "random"

metrics:
  output: "basic_test_metrics.csv"
  interval: 5
```

### Step 6: Build and Run Tests

```bash
# Navigate to test directory
cd test/simulator

# Build
mkdir build && cd build
cmake ..
cmake --build .

# Run tests
./painlessmesh-simulator --config ../scenarios/basic_test.yaml
```

### Step 7: Add to CI/CD

```yaml
# .github/workflows/firmware-test.yml
name: Firmware Tests

on: [push, pull_request]

jobs:
  simulator-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
        with:
          submodules: recursive
      
      - name: Install dependencies
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake ninja-build libboost-dev libyaml-cpp-dev
      
      - name: Build simulator tests
        run: |
          cd test/simulator
          mkdir build && cd build
          cmake -G Ninja ..
          ninja
      
      - name: Run basic test
        run: |
          cd test/simulator/build
          ./painlessmesh-simulator --config ../scenarios/basic_test.yaml
      
      - name: Upload metrics
        uses: actions/upload-artifact@v3
        with:
          name: test-metrics
          path: test/simulator/build/*.csv
```

## Approach 2: Embedded Integration

### Project Structure

```
your-firmware-project/
├── lib/
│   └── MyMeshApp/
│       ├── MyMeshApp.hpp
│       └── MyMeshApp.cpp
├── src/
│   └── main.cpp (ESP32 firmware)
├── simulator/
│   ├── CMakeLists.txt
│   ├── main.cpp (simulator entry point)
│   ├── firmware/
│   │   └── my_mesh_app_firmware.hpp
│   └── scenarios/
│       └── test.yaml
├── platformio.ini
└── CMakeLists.txt (for simulator builds)
```

### CMakeLists.txt

```cmake
# CMakeLists.txt (root)
cmake_minimum_required(VERSION 3.10)
project(MyFirmwareProject)

# Only build simulator, not PlatformIO code
if(NOT PLATFORMIO_BUILD)
  add_subdirectory(simulator)
endif()
```

```cmake
# simulator/CMakeLists.txt
# Add simulator as external project or submodule
add_subdirectory(../external/painlessMesh-simulator simulator)

# Your firmware library
add_library(my_firmware
  ../lib/MyMeshApp/MyMeshApp.cpp
)

target_include_directories(my_firmware PUBLIC
  ../lib
)

# Simulator firmware adapter
add_library(my_firmware_adapter
  firmware/my_mesh_app_firmware.cpp
)

target_link_libraries(my_firmware_adapter
  PUBLIC
    my_firmware
    simulator_firmware
)

# Link into simulator
target_link_libraries(painlessmesh-simulator
  PRIVATE
    my_firmware_adapter
)
```

## Common Integration Patterns

### Pattern 1: Sensor Node

```cpp
// lib/SensorNode/SensorNode.hpp
class SensorNode {
public:
  SensorNode(painlessMesh* mesh, Scheduler* scheduler);
  void setup();
  void loop();
  void onReceive(uint32_t from, String& msg);

private:
  void readSensor();
  void sendData();
  
  painlessMesh* mesh_;
  Scheduler* scheduler_;
  Task readTask_;
  float lastReading_;
};

// test/simulator/sensor_node_firmware.hpp
class SensorNodeFirmware : public FirmwareBase {
public:
  void setup() override {
    node_ = std::make_unique<SensorNode>(mesh_, scheduler_);
    node_->setup();
  }
  
  void loop() override { node_->loop(); }
  void onReceive(uint32_t from, String& msg) override {
    node_->onReceive(from, msg);
  }

private:
  std::unique_ptr<SensorNode> node_;
};
```

### Pattern 2: Gateway/Bridge Node

```cpp
// lib/GatewayNode/GatewayNode.hpp
class GatewayNode {
public:
  GatewayNode(painlessMesh* mesh, Scheduler* scheduler);
  void setup();
  void loop();
  void onReceive(uint32_t from, String& msg);
  
  // Gateway-specific
  void handleExternalData(const String& data);
  void forwardToMesh(const String& data);

private:
  painlessMesh* mesh_;
  Scheduler* scheduler_;
  // Gateway logic...
};

// test/simulator/gateway_firmware.hpp
class GatewayFirmware : public FirmwareBase {
public:
  void setup() override {
    gateway_ = std::make_unique<GatewayNode>(mesh_, scheduler_);
    gateway_->setup();
    
    // Simulate external data source
    injectTask_.set(10000, TASK_FOREVER, [this]() {
      simulateExternalData();
    });
    scheduler_->addTask(injectTask_);
    injectTask_.enable();
  }
  
  void loop() override { gateway_->loop(); }
  void onReceive(uint32_t from, String& msg) override {
    gateway_->onReceive(from, msg);
  }

private:
  void simulateExternalData() {
    // Inject test data into gateway
    String data = "{\"external\":\"test data\"}";
    gateway_->handleExternalData(data);
  }
  
  std::unique_ptr<GatewayNode> gateway_;
  Task injectTask_;
};
```

### Pattern 3: Multi-Node Roles

```cpp
// Create different node types with same codebase
// test/simulator/scenarios/multi_role.yaml
nodes:
  # Sensor nodes
  - id: "sensor1"
    firmware: "SensorNode"
    config:
      mesh_prefix: "MyMesh"
      mesh_password: "password"
      node_type: "temperature"
      read_interval: "30000"
  
  - id: "sensor2"
    firmware: "SensorNode"
    config:
      mesh_prefix: "MyMesh"
      mesh_password: "password"
      node_type: "humidity"
      read_interval: "30000"
  
  # Gateway node
  - id: "gateway1"
    firmware: "GatewayNode"
    config:
      mesh_prefix: "MyMesh"
      mesh_password: "password"
      external_port: "8080"
  
  # Relay nodes (no sensors, just forward)
  - id: "relay1"
    firmware: "RelayNode"
    config:
      mesh_prefix: "MyMesh"
      mesh_password: "password"
```

## Testing Strategies

### Unit Testing Your Application Code

Test your application logic independently:

```cpp
// test/test_my_mesh_app.cpp (Catch2)
#include <catch2/catch_test_macros.hpp>
#include <MyMeshApp.hpp>

TEST_CASE("MyMeshApp message handling") {
  // Create mock mesh and scheduler
  painlessMesh mesh;
  Scheduler scheduler;
  
  MyMeshApp app(&mesh, &scheduler);
  app.setup();
  
  SECTION("Handles incoming messages") {
    String msg = "TEST_MESSAGE";
    uint32_t from = 12345;
    
    app.onReceive(from, msg);
    
    // Verify behavior
    REQUIRE(/* check expected state */);
  }
}
```

### Integration Testing with Simulator

Test complete mesh scenarios:

```yaml
# test/simulator/scenarios/integration_test.yaml
simulation:
  name: "Full Integration Test"
  duration: 300
  time_scale: 10.0  # Run 10x faster

nodes:
  # Create 20 sensor nodes
  - template: "sensor"
    count: 20
    firmware: "SensorNode"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "test123"
      read_interval: "10000"
  
  # Create 2 gateway nodes
  - template: "gateway"
    count: 2
    firmware: "GatewayNode"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "test123"

topology:
  type: "mesh"  # Full mesh connectivity

events:
  # Test node failure at 60 seconds
  - time: 60
    action: "stop_node"
    nodes: ["sensor1", "sensor2"]
  
  # Test recovery at 120 seconds
  - time: 120
    action: "start_node"
    nodes: ["sensor1", "sensor2"]

metrics:
  output: "integration_test_metrics.csv"
  interval: 5
  collect:
    - "messages_sent"
    - "messages_received"
    - "message_delivery_rate"
    - "average_latency"
```

### Performance Testing

Test at scale:

```yaml
# test/simulator/scenarios/stress_test.yaml
simulation:
  name: "100 Node Stress Test"
  duration: 600
  time_scale: 5.0

nodes:
  - template: "sensor"
    count: 100  # Test with 100 nodes!
    firmware: "SensorNode"
    config:
      mesh_prefix: "StressTest"
      mesh_password: "stress123"

topology:
  type: "random"
  density: 0.3  # 30% connectivity

metrics:
  output: "stress_test_metrics.csv"
  interval: 10
```

## Best Practices

### 1. Separate Business Logic from Platform Code

**Good** - Platform-agnostic business logic:
```cpp
class MessageProcessor {
public:
  String processCommand(const String& cmd);
  bool validateMessage(const String& msg);
};
```

**Bad** - Tightly coupled to ESP32:
```cpp
void processCommand(String cmd) {
  Serial.println("Processing: " + cmd);
  digitalWrite(LED_PIN, HIGH);  // ESP32-specific
}
```

### 2. Use Dependency Injection

**Good** - Testable dependencies:
```cpp
class MyApp {
public:
  MyApp(painlessMesh* mesh, Scheduler* scheduler, ILogger* logger)
    : mesh_(mesh), scheduler_(scheduler), logger_(logger) {}
};
```

**Bad** - Global dependencies:
```cpp
extern painlessMesh mesh;
extern Scheduler scheduler;

class MyApp {
  void doSomething() {
    mesh.sendBroadcast("msg");  // Hard to test
  }
};
```

### 3. Abstract Hardware Dependencies

Create interfaces for hardware:

```cpp
// ITemperatureSensor.hpp
class ITemperatureSensor {
public:
  virtual ~ITemperatureSensor() = default;
  virtual float readTemperature() = 0;
};

// Real implementation for ESP32
class DHT22Sensor : public ITemperatureSensor {
public:
  float readTemperature() override {
    return dht.readTemperature();
  }
};

// Mock implementation for simulator
class MockTemperatureSensor : public ITemperatureSensor {
public:
  float readTemperature() override {
    return 20.0 + (rand() % 100) / 10.0;  // Simulated reading
  }
};

// Usage in firmware
class SensorNode {
public:
  SensorNode(painlessMesh* mesh, ITemperatureSensor* sensor)
    : mesh_(mesh), sensor_(sensor) {}
  
  void readAndSend() {
    float temp = sensor_->readTemperature();
    String msg = "{\"temp\":" + String(temp) + "}";
    mesh_->sendBroadcast(msg);
  }

private:
  painlessMesh* mesh_;
  ITemperatureSensor* sensor_;
};
```

### 4. Use Configuration Over Hard-Coding

```cpp
// Good - Configurable
class MyApp {
public:
  void setup() {
    String interval = getConfig("send_interval", "30000");
    sendTask_.setInterval(std::stoul(interval));
  }
};

// Bad - Hard-coded
class MyApp {
  void setup() {
    sendTask_.setInterval(30000);  // Can't change in tests
  }
};
```

### 5. Write Self-Contained Tests

Each scenario should be independent:

```yaml
# Good - Self-contained
simulation:
  name: "Partition Recovery Test"
  duration: 120
  seed: 12345  # Reproducible

nodes:
  - template: "sensor"
    count: 10
    firmware: "SensorNode"
    config:
      mesh_prefix: "PartitionTest"
      # ... complete config

# Bad - Depends on external state
simulation:
  name: "Test 2"
  duration: 60
  # Assumes Test 1 already ran
```

## Troubleshooting

### Issue: Firmware doesn't compile for simulator

**Symptom**: Build errors when compiling for simulator

**Cause**: ESP32-specific code (Arduino.h, ESP32 APIs)

**Solution**: Abstract platform dependencies:

```cpp
// platform.hpp
#ifdef SIMULATOR_BUILD
  #include <iostream>
  #define LOG(x) std::cout << x << std::endl
#else
  #include <Arduino.h>
  #define LOG(x) Serial.println(x)
#endif
```

### Issue: Can't find my firmware in simulator

**Symptom**: "Unknown firmware: MyApp" error

**Cause**: Firmware not registered with factory

**Solution**: Ensure registration code is linked:

```cpp
// In your firmware .cpp file
REGISTER_FIRMWARE(MyApp, MyAppFirmware)

// And in CMakeLists.txt
target_link_libraries(painlessmesh-simulator
  PRIVATE
    my_app_firmware  # Must link the .cpp file
)
```

### Issue: Simulator crashes on firmware load

**Symptom**: Segmentation fault when loading firmware

**Cause**: Null pointer access in setup()

**Solution**: Check for null pointers:

```cpp
void MyFirmware::setup() {
  if (!mesh_ || !scheduler_) {
    throw std::runtime_error("Firmware not initialized");
  }
  // ... rest of setup
}
```

### Issue: Different behavior on hardware vs simulator

**Symptom**: Works in simulator but fails on ESP32

**Cause**: Platform differences (timing, threading, etc.)

**Solution**: 
1. Add debug logging to compare execution paths
2. Test timing assumptions
3. Check for race conditions
4. Validate buffer sizes

```cpp
void MyFirmware::onReceive(uint32_t from, String& msg) {
  LOG("Received from " + String(from) + ": " + msg);
  // Process message...
}
```

### Issue: CMake can't find painlessMesh

**Symptom**: CMake error: "Could not find painlessMesh"

**Cause**: PAINLESSMESH_PATH not set or incorrect

**Solution**: Set the path explicitly:

```bash
cmake -DPAINLESSMESH_PATH=/path/to/painlessMesh ..
```

Or in CMakeLists.txt:
```cmake
set(PAINLESSMESH_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../external/painlessMesh" CACHE PATH "Path to painlessMesh")
```

### Issue: Multiple definition errors

**Symptom**: Linker errors about multiple definitions

**Cause**: Including .cpp files or missing include guards

**Solution**:
1. Only include .h files, not .cpp
2. Ensure all headers have include guards
3. Check for duplicate REGISTER_FIRMWARE calls

### Issue: Task scheduler doesn't work

**Symptom**: Periodic tasks don't execute

**Cause**: Not calling scheduler->execute() or mesh->update()

**Solution**: In simulator, the framework handles this automatically, but ensure your loop() doesn't block:

```cpp
void loop() override {
  // Don't do this:
  // delay(1000);
  
  // Use scheduler instead:
  // task_.set(1000, TASK_ONCE, []() { /* do work */ });
}
```

## Migration Checklist

- [ ] Organize firmware into reusable library classes
- [ ] Extract application logic from `main.cpp`
- [ ] Create firmware adapter class extending `FirmwareBase`
- [ ] Register firmware with `REGISTER_FIRMWARE` macro
- [ ] Create test scenarios (YAML files)
- [ ] Set up CMake build for simulator tests
- [ ] Run basic simulation test locally
- [ ] Add CI/CD workflow for automated testing
- [ ] Document firmware configuration options
- [ ] Create performance benchmark scenarios

## Next Steps

1. **Start Simple**: Test with 3-5 nodes first
2. **Increase Scale**: Gradually test with 10, 50, 100+ nodes
3. **Add Scenarios**: Create edge case scenarios (partitions, failures)
4. **Automate**: Add to CI/CD pipeline
5. **Benchmark**: Measure performance and optimize

## Additional Resources

- [Firmware Development Guide](FIRMWARE_DEVELOPMENT_GUIDE.md) - Detailed firmware API
- [Configuration Guide](CONFIGURATION_GUIDE.md) - Complete YAML reference
- [Example Scenarios](../examples/scenarios/) - Pre-built test scenarios
- [Getting Started](GETTING_STARTED.md) - Platform setup
- [Troubleshooting](TROUBLESHOOTING.md) - Common issues

## Example Projects

### PlatformIO Integration Example

**→ [Complete working example](../examples/integration/platformio-example/)**

This example includes:
- Platform-agnostic application code (`MyMeshApp`)
- ESP32 firmware (`src/main.cpp`)
- Simulator test harness
- Multiple test scenarios
- Ready to copy and adapt

See the example's README for build instructions.

### Project Structure Examples

**→ [More examples and patterns](../examples/PROJECT_STRUCTURE_EXAMPLES.md)**

Includes examples for:
- PlatformIO projects
- Arduino IDE projects
- Multi-node type systems
- Minimal integration

## Community Examples

See [Community Examples](https://github.com/Alteriom/painlessMesh-simulator/wiki/Community-Examples) for real-world integration examples from other users.

## Support

- **GitHub Issues**: [Report bugs or ask questions](https://github.com/Alteriom/painlessMesh-simulator/issues)
- **GitHub Discussions**: [Community Q&A](https://github.com/Alteriom/painlessMesh-simulator/discussions)
- **Documentation**: [Full documentation index](SIMULATOR_INDEX.md)

---

**Last Updated**: 2025-11-18
**Version**: 1.0.0
