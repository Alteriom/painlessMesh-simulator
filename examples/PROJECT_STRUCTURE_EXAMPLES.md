# Project Structure Examples

This document provides concrete examples of how to structure your firmware project to integrate with the painlessMesh simulator.

## Example 1: PlatformIO Project with Simulator Tests

### Project Structure

```
my-iot-project/
├── lib/
│   └── MyMeshApp/
│       ├── MyMeshApp.h
│       └── MyMeshApp.cpp
├── src/
│   └── main.cpp
├── test/
│   ├── unit/
│   │   └── test_app.cpp
│   └── simulator/
│       ├── CMakeLists.txt
│       ├── firmware/
│       │   ├── my_mesh_app_firmware.hpp
│       │   └── my_mesh_app_firmware.cpp
│       └── scenarios/
│           ├── basic_test.yaml
│           ├── stress_test.yaml
│           └── partition_test.yaml
├── platformio.ini
└── README.md
```

### File: lib/MyMeshApp/MyMeshApp.h

```cpp
#pragma once
#include <painlessMesh.h>
#include <TaskSchedulerDeclarations.h>

/**
 * @brief Core application logic for mesh node
 * 
 * This class is platform-agnostic and can run on both
 * ESP32 hardware and in the simulator.
 */
class MyMeshApp {
public:
  /**
   * @brief Construct a new mesh application
   * @param mesh Pointer to painlessMesh instance
   * @param scheduler Pointer to task scheduler
   */
  MyMeshApp(painlessMesh* mesh, Scheduler* scheduler);
  
  /**
   * @brief Initialize the application
   * 
   * Called once after mesh initialization
   */
  void setup();
  
  /**
   * @brief Main application loop
   * 
   * Called frequently, keep fast
   */
  void loop();
  
  /**
   * @brief Handle received mesh messages
   * @param from Sender node ID
   * @param msg Message content
   */
  void onReceive(uint32_t from, String& msg);
  
  /**
   * @brief Handle new mesh connection
   * @param nodeId Newly connected node ID
   */
  void onNewConnection(uint32_t nodeId);
  
  /**
   * @brief Handle mesh topology changes
   */
  void onChangedConnections();

private:
  painlessMesh* mesh_;
  Scheduler* scheduler_;
  uint32_t nodeId_;
  
  // Application tasks
  Task statusTask_;
  void sendStatusUpdate();
  
  // Message handlers
  void handleCommandMessage(uint32_t from, const String& cmd);
  void handleDataMessage(uint32_t from, const String& data);
};
```

### File: lib/MyMeshApp/MyMeshApp.cpp

```cpp
#include "MyMeshApp.h"

MyMeshApp::MyMeshApp(painlessMesh* mesh, Scheduler* scheduler)
  : mesh_(mesh), scheduler_(scheduler) {
  nodeId_ = mesh_->getNodeId();
}

void MyMeshApp::setup() {
  // Setup periodic status updates
  statusTask_.set(30 * TASK_SECOND, TASK_FOREVER, [this]() {
    sendStatusUpdate();
  });
  
  scheduler_->addTask(statusTask_);
  statusTask_.enable();
}

void MyMeshApp::loop() {
  // Fast loop logic (if any)
}

void MyMeshApp::onReceive(uint32_t from, String& msg) {
  // Route messages based on type
  if (msg.startsWith("CMD:")) {
    handleCommandMessage(from, msg.substring(4));
  } else if (msg.startsWith("DATA:")) {
    handleDataMessage(from, msg.substring(5));
  }
}

void MyMeshApp::onNewConnection(uint32_t nodeId) {
  // Log new connection
  String msg = "NODE:" + String(nodeId_) + " connected to " + String(nodeId);
  mesh_->sendBroadcast(msg);
}

void MyMeshApp::onChangedConnections() {
  // Handle topology changes
  auto nodes = mesh_->getNodeList();
  // Update routing, sync data, etc.
}

void MyMeshApp::sendStatusUpdate() {
  String status = "STATUS:" + String(nodeId_) + 
                  ":OK:TIME=" + String(mesh_->getNodeTime());
  mesh_->sendBroadcast(status);
}

void MyMeshApp::handleCommandMessage(uint32_t from, const String& cmd) {
  // Process command
  if (cmd == "PING") {
    String reply = "PONG:" + String(nodeId_);
    mesh_->sendSingle(from, reply);
  }
}

void MyMeshApp::handleDataMessage(uint32_t from, const String& data) {
  // Process data message
}
```

### File: src/main.cpp (ESP32 Entry Point)

```cpp
#include <Arduino.h>
#include <MyMeshApp.h>

painlessMesh mesh;
Scheduler userScheduler;
MyMeshApp* app;

void setup() {
  Serial.begin(115200);
  
  // Initialize mesh
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init("MyMeshNetwork", "password123", &userScheduler, 5555);
  
  // Create application
  app = new MyMeshApp(&mesh, &userScheduler);
  app->setup();
  
  // Register callbacks
  mesh.onReceive([](uint32_t from, String& msg) {
    app->onReceive(from, msg);
  });
  
  mesh.onNewConnection([](uint32_t nodeId) {
    app->onNewConnection(nodeId);
  });
  
  mesh.onChangedConnections([]() {
    app->onChangedConnections();
  });
  
  Serial.println("Mesh node started");
}

void loop() {
  mesh.update();
  app->loop();
}
```

### File: test/simulator/firmware/my_mesh_app_firmware.hpp

```cpp
#pragma once
#include "simulator/firmware/firmware_base.hpp"
#include <MyMeshApp.h>

namespace simulator {
namespace firmware {

/**
 * @brief Simulator adapter for MyMeshApp
 * 
 * This class wraps your application code for use in the simulator.
 */
class MyMeshAppFirmware : public FirmwareBase {
public:
  MyMeshAppFirmware() : FirmwareBase("MyMeshApp") {}
  
  void setup() override {
    // Create application instance with simulator mesh
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

### File: test/simulator/firmware/my_mesh_app_firmware.cpp

```cpp
#include "my_mesh_app_firmware.hpp"
#include "simulator/firmware/firmware_factory.hpp"

using namespace simulator::firmware;

// Auto-register firmware with factory
REGISTER_FIRMWARE(MyMeshApp, MyMeshAppFirmware)
```

### File: test/simulator/CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.10)
project(MyMeshAppSimulatorTests)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find or add painlessMesh-simulator
# Option 1: If installed
# find_package(painlessMesh-simulator REQUIRED)

# Option 2: As git submodule
add_subdirectory(../../external/painlessMesh-simulator simulator)

# Your application library
add_library(my_mesh_app
  ../../lib/MyMeshApp/MyMeshApp.cpp
)

target_include_directories(my_mesh_app PUBLIC
  ../../lib
  ${PAINLESSMESH_PATH}/src
)

target_link_libraries(my_mesh_app
  PUBLIC
    painlessmesh_lib
)

# Simulator firmware adapter
add_library(my_mesh_app_firmware
  firmware/my_mesh_app_firmware.cpp
)

target_include_directories(my_mesh_app_firmware PUBLIC
  firmware
)

target_link_libraries(my_mesh_app_firmware
  PUBLIC
    my_mesh_app
    simulator_firmware
)

# Link firmware into simulator executable
target_link_libraries(painlessmesh-simulator
  PRIVATE
    my_mesh_app_firmware
)
```

### File: test/simulator/scenarios/basic_test.yaml

```yaml
simulation:
  name: "MyMeshApp Basic Functionality Test"
  description: "Test basic message sending and receiving with 5 nodes"
  duration: 60
  time_scale: 5.0
  seed: 12345

network:
  default_latency:
    min_ms: 10
    max_ms: 30

nodes:
  - id: "node1"
    firmware: "MyMeshApp"
    config:
      mesh_prefix: "MyMeshNetwork"
      mesh_password: "password123"
      mesh_port: 5555

  - id: "node2"
    firmware: "MyMeshApp"
    config:
      mesh_prefix: "MyMeshNetwork"
      mesh_password: "password123"
      mesh_port: 5555

  - id: "node3"
    firmware: "MyMeshApp"
    config:
      mesh_prefix: "MyMeshNetwork"
      mesh_password: "password123"
      mesh_port: 5555

  - id: "node4"
    firmware: "MyMeshApp"
    config:
      mesh_prefix: "MyMeshNetwork"
      mesh_password: "password123"
      mesh_port: 5555

  - id: "node5"
    firmware: "MyMeshApp"
    config:
      mesh_prefix: "MyMeshNetwork"
      mesh_password: "password123"
      mesh_port: 5555

topology:
  type: "random"

metrics:
  output: "basic_test_metrics.csv"
  interval: 5
  collect:
    - "messages_sent"
    - "messages_received"
    - "bytes_sent"
    - "bytes_received"
```

### File: platformio.ini

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

lib_deps =
    https://github.com/Alteriom/painlessMesh.git

monitor_speed = 115200
upload_speed = 921600

build_flags =
    -DCORE_DEBUG_LEVEL=3

[env:simulator]
; This is just a placeholder - actual simulator builds use CMake
; Run: cd test/simulator && mkdir build && cd build && cmake .. && make
```

### Building and Running

**Build for ESP32:**
```bash
pio run -e esp32dev
pio run -e esp32dev --target upload
pio device monitor
```

**Build and run simulator:**
```bash
cd test/simulator
mkdir build && cd build
cmake ..
cmake --build .
./painlessmesh-simulator --config ../scenarios/basic_test.yaml
```

## Example 2: Arduino IDE Project with Simulator Tests

### Project Structure

```
MyMeshProject/
├── MyMeshProject.ino
├── MeshApp.h
├── MeshApp.cpp
├── simulator_tests/
│   ├── CMakeLists.txt
│   ├── firmware/
│   │   ├── mesh_app_firmware.hpp
│   │   └── mesh_app_firmware.cpp
│   └── scenarios/
│       └── test.yaml
└── README.md
```

### File: MyMeshProject.ino

```cpp
#include "MeshApp.h"

painlessMesh mesh;
Scheduler userScheduler;
MeshApp* app;

void setup() {
  Serial.begin(115200);
  
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init("MyMesh", "password", &userScheduler, 5555);
  
  app = new MeshApp(&mesh, &userScheduler);
  app->begin();
  
  mesh.onReceive([](uint32_t from, String& msg) {
    app->handleMessage(from, msg);
  });
}

void loop() {
  mesh.update();
  app->update();
}
```

### File: MeshApp.h

```cpp
#pragma once

#ifdef ARDUINO
  #include <Arduino.h>
  #include <painlessMesh.h>
#else
  // Simulator includes
  #include <painlessMesh.h>
  #include <TaskSchedulerDeclarations.h>
#endif

class MeshApp {
public:
  MeshApp(painlessMesh* mesh, Scheduler* scheduler);
  
  void begin();
  void update();
  void handleMessage(uint32_t from, String& msg);

private:
  painlessMesh* mesh_;
  Scheduler* scheduler_;
  Task sendTask_;
  
  void sendData();
};
```

### File: MeshApp.cpp

```cpp
#include "MeshApp.h"

MeshApp::MeshApp(painlessMesh* mesh, Scheduler* scheduler)
  : mesh_(mesh), scheduler_(scheduler) {
}

void MeshApp::begin() {
  sendTask_.set(10000, TASK_FOREVER, [this]() {
    sendData();
  });
  
  scheduler_->addTask(sendTask_);
  sendTask_.enable();
}

void MeshApp::update() {
  // Fast update logic
}

void MeshApp::handleMessage(uint32_t from, String& msg) {
  #ifdef ARDUINO
    Serial.printf("Received from %u: %s\n", from, msg.c_str());
  #else
    std::cout << "Received from " << from << ": " << msg.c_str() << std::endl;
  #endif
}

void MeshApp::sendData() {
  String msg = "Data from " + String(mesh_->getNodeId());
  mesh_->sendBroadcast(msg);
}
```

## Example 3: Multi-Node Type Project

### Project Structure

```
smart-home-mesh/
├── lib/
│   ├── Common/
│   │   ├── MeshNode.h
│   │   └── MeshNode.cpp
│   ├── SensorNode/
│   │   ├── SensorNode.h
│   │   └── SensorNode.cpp
│   ├── ActuatorNode/
│   │   ├── ActuatorNode.h
│   │   └── ActuatorNode.cpp
│   └── GatewayNode/
│       ├── GatewayNode.h
│       └── GatewayNode.cpp
├── src/
│   ├── sensor_firmware/
│   │   └── main.cpp
│   ├── actuator_firmware/
│   │   └── main.cpp
│   └── gateway_firmware/
│       └── main.cpp
├── test/
│   └── simulator/
│       ├── firmware/
│       │   ├── sensor_firmware.hpp
│       │   ├── actuator_firmware.hpp
│       │   └── gateway_firmware.hpp
│       └── scenarios/
│           ├── full_system.yaml
│           └── sensor_only.yaml
└── platformio.ini
```

### File: test/simulator/scenarios/full_system.yaml

```yaml
simulation:
  name: "Smart Home Full System Test"
  duration: 300
  time_scale: 5.0

nodes:
  # 10 temperature sensors
  - template: "temp_sensor"
    count: 10
    firmware: "SensorNode"
    config:
      mesh_prefix: "SmartHome"
      mesh_password: "secure123"
      sensor_type: "temperature"
      read_interval: "30000"
  
  # 5 humidity sensors
  - template: "humidity_sensor"
    count: 5
    firmware: "SensorNode"
    config:
      mesh_prefix: "SmartHome"
      mesh_password: "secure123"
      sensor_type: "humidity"
      read_interval: "30000"
  
  # 3 actuators (lights, HVAC)
  - template: "actuator"
    count: 3
    firmware: "ActuatorNode"
    config:
      mesh_prefix: "SmartHome"
      mesh_password: "secure123"
      actuator_type: "light"
  
  # 1 gateway
  - id: "gateway1"
    firmware: "GatewayNode"
    config:
      mesh_prefix: "SmartHome"
      mesh_password: "secure123"
      mqtt_broker: "mqtt.example.com"

topology:
  type: "mesh"

events:
  # Simulate sensor reading at 60s
  - time: 60
    action: "inject_message"
    node: "gateway1"
    message: "CMD:READ_ALL"
  
  # Simulate actuator command at 120s
  - time: 120
    action: "inject_message"
    node: "gateway1"
    message: "CMD:ACTUATOR:light:ON"

metrics:
  output: "full_system_metrics.csv"
  interval: 10
```

## Example 4: Minimal Integration

For simple projects that just want basic testing:

### Project Structure

```
simple-mesh/
├── src/
│   └── main.cpp
├── test_simulator.cpp
└── CMakeLists.txt
```

### File: test_simulator.cpp

```cpp
#include "simulator/firmware/firmware_base.hpp"
#include "simulator/firmware/firmware_factory.hpp"

// Inline firmware for testing
class SimpleFirmware : public simulator::firmware::FirmwareBase {
public:
  SimpleFirmware() : FirmwareBase("Simple") {}
  
  void setup() override {
    task_.set(5000, TASK_FOREVER, [this]() {
      String msg = "Hello from " + std::to_string(node_id_);
      sendBroadcast(msg);
    });
    scheduler_->addTask(task_);
    task_.enable();
  }
  
  void loop() override {}
  
  void onReceive(uint32_t from, String& msg) override {
    std::cout << "Node " << node_id_ << " received: " << msg.c_str() << std::endl;
  }

private:
  Task task_;
};

REGISTER_FIRMWARE(Simple, SimpleFirmware)
```

### File: CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.10)
project(SimpleMeshTest)

add_subdirectory(external/painlessMesh-simulator)

add_library(simple_firmware test_simulator.cpp)
target_link_libraries(simple_firmware PUBLIC simulator_firmware)
target_link_libraries(painlessmesh-simulator PRIVATE simple_firmware)
```

## Best Practices Summary

1. **Separate Application Logic**: Keep business logic in reusable classes
2. **Use Dependency Injection**: Pass mesh and scheduler as parameters
3. **Abstract Hardware**: Create interfaces for hardware dependencies
4. **One Firmware File**: Keep simulator adapter minimal
5. **Test Scenarios**: Create comprehensive YAML test files
6. **CI/CD Integration**: Automate simulator tests in your pipeline

## Additional Resources

- [Integrating into Your Project Guide](../docs/INTEGRATING_INTO_YOUR_PROJECT.md) - Complete integration walkthrough
- [Firmware Integration Guide](../docs/FIRMWARE_INTEGRATION.md) - Firmware API reference
- [Configuration Guide](../docs/CONFIGURATION_GUIDE.md) - YAML configuration reference

---

**Last Updated**: 2025-11-18
