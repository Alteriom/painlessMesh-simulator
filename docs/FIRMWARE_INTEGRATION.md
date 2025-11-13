# Firmware Integration Guide

## Overview

The painlessMesh simulator now supports custom firmware loading, enabling you to test mesh-aware applications without physical hardware. This document explains how to use and develop firmware for the simulator.

## Architecture

### Components

1. **FirmwareBase** - Abstract base class that all firmware must extend
2. **FirmwareFactory** - Singleton factory for registering and creating firmware
3. **VirtualNode** - Manages firmware lifecycle and routes mesh callbacks
4. **SimpleBroadcastFirmware** - Example firmware implementation

### Firmware Lifecycle

```
1. Factory Creation:    FirmwareFactory::create("FirmwareName")
2. Loading:             node->loadFirmware(firmware)
3. Node Start:          node->start()
4. Initialization:      firmware->initialize(mesh, scheduler, nodeId, config)
5. Setup:               firmware->setup()
6. Running:             firmware->loop() called every update cycle
7. Events:              firmware->onReceive(), onNewConnection(), etc.
```

## Using Existing Firmware

### SimpleBroadcast Firmware

SimpleBroadcast is a built-in firmware that periodically broadcasts messages to all nodes in the mesh.

**Configuration Options**:
- `broadcast_interval` - Interval between broadcasts in milliseconds (default: 5000)
- `broadcast_message` - Message prefix to broadcast (default: "Hello from node")

**Example YAML Configuration**:

```yaml
nodes:
  - id: "node1"
    firmware: "SimpleBroadcast"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password123"
      mesh_port: 5555
      broadcast_interval: "3000"
      broadcast_message: "Hello from"
```

### Running with Firmware

```bash
# Validate configuration
./painlessmesh-simulator --config examples/scenarios/simple_firmware_test.yaml --validate-only

# Run simulation
./painlessmesh-simulator --config examples/scenarios/simple_firmware_test.yaml
```

## Developing Custom Firmware

### Basic Template

```cpp
#include "simulator/firmware/firmware_base.hpp"

class MyCustomFirmware : public simulator::firmware::FirmwareBase {
public:
  MyCustomFirmware() : FirmwareBase("MyCustomFirmware") {}
  
  void setup() override {
    // Initialize your firmware
    // Access mesh via mesh_ pointer
    // Access scheduler via scheduler_ pointer
    // Access config via getConfig()
    
    // Example: Get custom config value
    auto interval = getConfig("custom_interval", "1000");
    
    // Example: Create periodic task
    task_.set(std::stoul(interval), TASK_FOREVER, [this]() {
      doPeriodicWork();
    });
    scheduler_->addTask(task_);
    task_.enable();
  }
  
  void loop() override {
    // Called every update cycle
    // Keep this fast - use scheduler for periodic tasks
  }
  
  void onReceive(uint32_t from, String& msg) override {
    // Handle received messages
    std::cout << "Received from " << from << ": " << msg << std::endl;
  }
  
  void onNewConnection(uint32_t nodeId) override {
    // Handle new mesh connections
    std::cout << "Connected to node " << nodeId << std::endl;
  }
  
  void onChangedConnections() override {
    // Handle topology changes
  }
  
private:
  void doPeriodicWork() {
    // Send a message
    String msg = "Status update from node " + std::to_string(node_id_);
    mesh_->sendBroadcast(msg);
  }
  
  Task task_;
};
```

### Registering Firmware

**Option 1: Manual Registration in main.cpp**

```cpp
#include "my_custom_firmware.hpp"

int main(int argc, char* argv[]) {
  // Register your firmware
  firmware::FirmwareFactory::instance().registerFirmware("MyCustom",
    []() { return std::make_unique<MyCustomFirmware>(); });
  
  // ... rest of main
}
```

**Option 2: Auto-registration with Macro**

```cpp
// In my_custom_firmware.cpp
#include "my_custom_firmware.hpp"
#include "simulator/firmware/firmware_factory.hpp"

REGISTER_FIRMWARE(MyCustom, MyCustomFirmware)
```

Note: Auto-registration requires the .cpp file to be linked into the binary.

### Checking Available Firmware

You can check which firmware types are available:

```cpp
// Check if a specific firmware is registered
if (FirmwareFactory::instance().hasFirmware("MyCustom")) {
  std::cout << "MyCustom firmware is available" << std::endl;
}

// List all registered firmware
auto available = FirmwareFactory::instance().listFirmware();
std::cout << "Available firmware: ";
for (const auto& name : available) {
  std::cout << name << " ";
}
std::cout << std::endl;
```

This is useful for validating YAML configurations or debugging firmware loading issues.

### Accessing Configuration

Firmware can access configuration values passed from YAML:

```cpp
// In setup()
if (hasConfig("my_setting")) {
  auto value = getConfig("my_setting", "default_value");
  // Use value...
}
```

YAML example:
```yaml
nodes:
  - id: "node1"
    firmware: "MyCustom"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"
      my_setting: "custom_value"  # Your custom config
```

### Using the Task Scheduler

The painlessMesh library includes a task scheduler for periodic operations:

```cpp
// Create a task that runs every 5 seconds
Task myTask(5 * TASK_SECOND, TASK_FOREVER, [this]() {
  // Do something periodically
  sendStatusUpdate();
});

// Add to scheduler in setup()
void setup() override {
  scheduler_->addTask(myTask);
  myTask.enable();
}
```

### Sending Messages

You can use the protected helper methods for common operations:

```cpp
// Using helper methods (recommended - handles null checks)
void loop() override {
  sendBroadcast("Hello everyone!");  // Broadcast to all nodes
  sendSingle(targetNodeId, "Hello node!");  // Send to specific node
}

// Or access mesh directly for advanced features
void advancedSend() {
  // Send with callback
  mesh_->sendBroadcast("Message", [](uint32_t from, bool success) {
    if (success) {
      std::cout << "Message sent successfully" << std::endl;
    }
  });
}
```

### Using Helper Methods

The FirmwareBase class provides convenient helper methods:

```cpp
void setup() override {
  // Get mesh information
  uint32_t myId = getNodeId();
  uint32_t currentTime = getNodeTime();
  auto neighbors = getNodeList();
  
  std::cout << "Node " << myId << " started at time " << currentTime << std::endl;
  std::cout << "Connected to " << neighbors.size() << " nodes" << std::endl;
  
  // Send initial broadcast
  sendBroadcast("Node " + std::to_string(myId) + " is online");
}

void onReceive(uint32_t from, String& msg) override {
  // Echo messages back to sender
  String response = "Echo: " + msg;
  sendSingle(from, response);
}
```

## Testing Firmware

### Unit Testing

```cpp
#include <catch2/catch_test_macros.hpp>
#include "my_custom_firmware.hpp"

TEST_CASE("MyCustomFirmware functionality") {
  boost::asio::io_context io;
  Scheduler scheduler;
  
  NodeConfig config;
  config.nodeId = 1001;
  config.meshPrefix = "Test";
  config.meshPassword = "password";
  config.firmware = "MyCustom";
  
  VirtualNode node(1001, config, &scheduler, io);
  
  // Register and load firmware
  FirmwareFactory::instance().registerFirmware("MyCustom",
    []() { return std::make_unique<MyCustomFirmware>(); });
  
  node.loadFirmware("MyCustom");
  node.start();
  
  // Test firmware behavior
  auto* firmware = dynamic_cast<MyCustomFirmware*>(node.getFirmware());
  REQUIRE(firmware != nullptr);
  
  // Run updates and verify behavior
  for (int i = 0; i < 10; ++i) {
    scheduler.execute();
    node.update();
    io.poll();
  }
  
  node.stop();
}
```

### Integration Testing

Create a YAML scenario with your firmware:

```yaml
simulation:
  name: "MyCustom Firmware Test"
  duration: 30
  
nodes:
  - id: "node1"
    firmware: "MyCustom"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"
      my_setting: "value"
```

Run the simulator:
```bash
./painlessmesh-simulator --config my_test.yaml
```

## API Reference

### FirmwareBase Methods

#### Lifecycle
- `void setup()` - Called once after initialization (pure virtual)
- `void loop()` - Called every update cycle (pure virtual)

#### Callbacks
- `void onReceive(uint32_t from, String& msg)` - Message received
- `void onNewConnection(uint32_t nodeId)` - New mesh connection
- `void onChangedConnections()` - Topology changed
- `void onNodeTimeAdjusted(int32_t offset)` - Time synchronized

#### Accessors
- `std::string getName() const` - Get firmware name
- `String getVersion() const` - Get firmware version (default: "1.0.0", override to customize)
- `uint32_t getNodeId() const` - Get node ID
- `String getConfig(const String& key, const String& default)` - Get config value
- `bool hasConfig(const String& key) const` - Check if config key exists
- `bool isInitialized() const` - Check if firmware has been initialized

#### Protected Helper Methods
- `void sendBroadcast(const String& msg)` - Send broadcast message to all nodes
- `void sendSingle(uint32_t dest, const String& msg)` - Send message to specific node
- `uint32_t getNodeTime() const` - Get current mesh time in microseconds
- `std::list<uint32_t> getNodeList() const` - Get list of connected node IDs

#### Protected Members
- `painlessmesh::Mesh* mesh_` - Mesh instance
- `Scheduler* scheduler_` - Task scheduler
- `uint32_t node_id_` - Node ID
- `std::map<String, String> config_` - Configuration map
- `bool initialized_` - Initialization flag

## Examples

### Sensor Node Firmware

```cpp
class SensorFirmware : public FirmwareBase {
public:
  SensorFirmware() : FirmwareBase("Sensor") {}
  
  void setup() override {
    // Read sensor every 30 seconds
    sensorTask_.set(30 * TASK_SECOND, TASK_FOREVER, [this]() {
      readAndSendSensor();
    });
    scheduler_->addTask(sensorTask_);
    sensorTask_.enable();
  }
  
  void loop() override {}
  
  void onReceive(uint32_t from, String& msg) override {
    // Handle commands
    if (msg == "READ_SENSOR") {
      readAndSendSensor();
    }
  }
  
private:
  void readAndSendSensor() {
    // Simulate sensor reading
    float value = 20.0 + (rand() % 100) / 10.0;
    
    String data = "{\"node\":" + std::to_string(node_id_) + 
                  ",\"temp\":" + std::to_string(value) + "}";
    
    mesh_->sendBroadcast(data);
  }
  
  Task sensorTask_;
};
```

### Bridge Node Firmware

```cpp
class BridgeFirmware : public FirmwareBase {
public:
  BridgeFirmware() : FirmwareBase("Bridge") {}
  
  void setup() override {
    // Bridge nodes forward messages to external systems
    auto mqtt_broker = getConfig("mqtt_broker", "localhost");
    connectToMQTT(mqtt_broker);
  }
  
  void onReceive(uint32_t from, String& msg) override {
    // Forward mesh messages to MQTT
    publishToMQTT("mesh/data", msg);
  }
  
private:
  void connectToMQTT(const String& broker) {
    // MQTT connection logic (simulated)
    std::cout << "Connected to MQTT broker: " << broker << std::endl;
  }
  
  void publishToMQTT(const String& topic, const String& payload) {
    std::cout << "MQTT Publish [" << topic << "]: " << payload << std::endl;
  }
};
```

## Troubleshooting

### Firmware Not Found
```
[ERROR] Unknown firmware: MyFirmware
[INFO] Available firmware:
```

**Solution**: Ensure firmware is registered before use. Add registration in main.cpp:
```cpp
FirmwareFactory::instance().registerFirmware("MyFirmware",
  []() { return std::make_unique<MyFirmware>(); });
```

Or check available firmware to debug:
```cpp
// List all available firmware
auto available = FirmwareFactory::instance().listFirmware();
std::cout << "Available firmware: ";
for (const auto& name : available) {
  std::cout << name << " ";
}
std::cout << std::endl;
```

### Firmware Not Executing
Check that:
1. Firmware is loaded: `node->loadFirmware("FirmwareName")`
2. Node is started: `node->start()`
3. Update is called: `node->update()` in simulation loop

### Configuration Not Available
Ensure config keys match YAML:
```yaml
config:
  my_key: "value"  # Use exact key name in getConfig("my_key")
```

### Messages Not Received
Verify:
1. Nodes are connected (topology setup)
2. onReceive() callback is implemented
3. Mesh is initialized and running

## Best Practices

1. **Keep loop() Fast**: Use scheduler for time-based operations
2. **Handle Callbacks**: Implement onReceive() even if just for logging
3. **Error Handling**: Check for null pointers (mesh_, scheduler_)
4. **Resource Management**: Clean up tasks in destructor if needed
5. **Testing**: Test firmware in isolation before integration
6. **Logging**: Use consistent logging for debugging
7. **Configuration**: Provide sensible defaults for config values

## Future Enhancements

- Dynamic firmware loading from shared libraries
- Firmware state persistence
- Performance profiling per firmware
- Inter-firmware communication APIs
- Visual debugging tools

## See Also

- [SimpleBroadcastFirmware](../include/simulator/firmware/simple_broadcast_firmware.hpp) - Example implementation
- [FirmwareBase API](../include/simulator/firmware/firmware_base.hpp) - Base class documentation
- [Example Scenarios](../examples/scenarios/) - YAML configuration examples
