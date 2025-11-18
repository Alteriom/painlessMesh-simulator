# Firmware Examples

This directory contains example firmware implementations for the painlessMesh simulator.

## For Existing Firmware Projects

**Have your own ESP32/ESP8266 firmware?** See the [Integrating into Your Project Guide](../../docs/INTEGRATING_INTO_YOUR_PROJECT.md) for step-by-step instructions on testing your firmware with the simulator.

## Available Examples

### SimpleBroadcastFirmware

Located in `src/firmware/simple_broadcast_firmware.cpp`, this is a built-in firmware that demonstrates basic mesh functionality by broadcasting messages periodically.

**Features:**
- Periodic broadcast messages
- Configurable interval
- Message reception handling
- Auto-registration using `REGISTER_FIRMWARE` macro

**Configuration:**
- `broadcast_interval`: Interval between broadcasts in milliseconds (default: 5000)
- `broadcast_message`: Message prefix to broadcast (default: "Hello from node")

**Usage in YAML:**
```yaml
nodes:
  - id: "node1"
    firmware: "SimpleBroadcast"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"
      broadcast_interval: "3000"
      broadcast_message: "Hello from"
```

**Example Scenario:** See `examples/scenarios/firmware_broadcast.yaml`

### EchoServerFirmware

Located in `src/firmware/echo_server_firmware.cpp`, this firmware responds to any received message by echoing it back to the sender with an "ECHO: " prefix.

**Features:**
- Echoes all received messages back to sender
- Tracks number of messages echoed
- Tracks number of client connections
- Useful for testing request/response patterns

**Configuration:** None required

**Usage in YAML:**
```yaml
nodes:
  - id: "server1"
    firmware: "EchoServer"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"
```

**Example Scenario:** See `examples/scenarios/firmware_echo.yaml`

### EchoClientFirmware

Located in `src/firmware/echo_client_firmware.cpp`, this firmware periodically sends requests and processes echo responses from echo servers.

**Features:**
- Sends periodic requests
- Supports broadcast or targeted sending
- Processes echo responses
- Tracks requests sent and responses received

**Configuration:**
- `server_node_id`: Target server node ID, or "0" for broadcast mode (default: "0")
- `request_interval`: Interval between requests in seconds (default: 5)

**Usage in YAML:**
```yaml
nodes:
  - id: "client1"
    firmware: "EchoClient"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"
      server_node_id: "0"  # Broadcast mode
      request_interval: "3"
```

**Example Scenario:** See `examples/scenarios/firmware_echo.yaml`

## Creating Your Own Firmware

### Step 1: Create Firmware Class

Create a new header file (e.g., `my_firmware.hpp`):

```cpp
#include "simulator/firmware/firmware_base.hpp"
#include <TaskSchedulerDeclarations.h>

namespace simulator {
namespace firmware {

class MyCustomFirmware : public FirmwareBase {
public:
  MyCustomFirmware() : FirmwareBase("MyCustom") {}
  
  void setup() override {
    // Initialize your firmware
    std::cout << "[INFO] MyCustom firmware initialized" << std::endl;
  }
  
  void loop() override {
    // Called every update cycle
  }
  
  void onReceive(uint32_t from, String& msg) override {
    std::cout << "[INFO] Received from " << from << ": " << msg << std::endl;
  }
};

} // namespace firmware
} // namespace simulator
```

### Step 2: Register Firmware

Create implementation file (e.g., `my_firmware.cpp`):

```cpp
#include "my_firmware.hpp"
#include "simulator/firmware/firmware_factory.hpp"

using namespace simulator::firmware;

// Auto-register the firmware
REGISTER_FIRMWARE(MyCustom, MyCustomFirmware)
```

### Step 3: Use in Configuration

Add to your YAML scenario:

```yaml
nodes:
  - id: "custom_node"
    firmware: "MyCustom"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"
```

## Firmware Factory API

### Checking Available Firmware

```cpp
#include "simulator/firmware/firmware_factory.hpp"

using namespace simulator::firmware;

// Check if firmware exists
if (FirmwareFactory::instance().hasFirmware("SimpleBroadcast")) {
  std::cout << "SimpleBroadcast is available" << std::endl;
}

// List all registered firmware
auto available = FirmwareFactory::instance().listFirmware();
std::cout << "Available firmware: ";
for (const auto& name : available) {
  std::cout << name << " ";
}
std::cout << std::endl;
```

### Creating Firmware Instances

```cpp
// Create by name (from factory)
auto firmware = FirmwareFactory::instance().create("SimpleBroadcast");
if (firmware) {
  // Load into node
  node.loadFirmware(std::move(firmware));
}

// Check for errors
auto unknown = FirmwareFactory::instance().create("UnknownFirmware");
if (!unknown) {
  std::cerr << "Failed to create firmware" << std::endl;
}
```

### Manual Registration

If you don't want to use the `REGISTER_FIRMWARE` macro:

```cpp
FirmwareFactory::instance().registerFirmware("MyCustom",
  []() { return std::make_unique<MyCustomFirmware>(); });
```

## Advanced Examples

### Sensor Node with Periodic Reading

```cpp
class SensorFirmware : public FirmwareBase {
public:
  SensorFirmware() : FirmwareBase("Sensor"),
    sensorTask_(30 * TASK_SECOND, TASK_FOREVER,
                std::bind(&SensorFirmware::readSensor, this)) {}
  
  void setup() override {
    scheduler_->addTask(sensorTask_);
    sensorTask_.enable();
  }
  
  void loop() override {}
  
  void onReceive(uint32_t from, String& msg) override {
    if (msg == "READ") {
      readSensor();
    }
  }
  
private:
  void readSensor() {
    float temp = 20.0 + (rand() % 100) / 10.0;
    String data = "{\"node\":" + std::to_string(node_id_) +
                  ",\"temp\":" + std::to_string(temp) + "}";
    mesh_->sendBroadcast(data);
  }
  
  Task sensorTask_;
};
```

### Message Relay Node

```cpp
class RelayFirmware : public FirmwareBase {
public:
  RelayFirmware() : FirmwareBase("Relay") {}
  
  void setup() override {
    auto relay_to_str = getConfig("relay_to", "");
    if (!relay_to_str.empty()) {
      relay_to_ = std::stoul(relay_to_str);
    }
  }
  
  void loop() override {}
  
  void onReceive(uint32_t from, String& msg) override {
    if (relay_to_ != 0) {
      // Forward message to specific node
      mesh_->sendSingle(relay_to_, msg);
      std::cout << "[INFO] Relayed message from " << from
                << " to " << relay_to_ << std::endl;
    }
  }
  
private:
  uint32_t relay_to_{0};
};
```

## Testing Firmware

### Unit Test Example

```cpp
TEST_CASE("MyCustomFirmware test") {
  boost::asio::io_context io;
  Scheduler scheduler;
  
  NodeConfig config;
  config.nodeId = 1001;
  config.meshPrefix = "Test";
  config.firmware = "MyCustom";
  
  // Register firmware
  FirmwareFactory::instance().registerFirmware("MyCustom",
    []() { return std::make_unique<MyCustomFirmware>(); });
  
  VirtualNode node(1001, config, &scheduler, io);
  node.loadFirmware("MyCustom");
  node.start();
  
  // Test behavior
  node.update();
  
  node.stop();
}
```

## See Also

- [Firmware Integration Guide](../../docs/FIRMWARE_INTEGRATION.md) - Complete documentation
- [FirmwareBase API](../../include/simulator/firmware/firmware_base.hpp) - Base class reference
- [FirmwareFactory API](../../include/simulator/firmware/firmware_factory.hpp) - Factory class reference
- [SimpleBroadcastFirmware](../../include/simulator/firmware/simple_broadcast_firmware.hpp) - Example implementation
