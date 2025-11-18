# Firmware Integration Summary

## Quick Links for Firmware Developers

### 🚀 Just Getting Started?
**→ [Integrating into Your Project](INTEGRATING_INTO_YOUR_PROJECT.md)** - Complete step-by-step guide

### 💻 Want a Working Example?
**→ [PlatformIO Example](../examples/integration/platformio-example/)** - Copy-paste-ready example

### 📖 Need Reference?
**→ [Project Structure Examples](../examples/PROJECT_STRUCTURE_EXAMPLES.md)** - Multiple patterns

## What's Available

### Documentation (1,800+ lines)

1. **[Integration Guide](INTEGRATING_INTO_YOUR_PROJECT.md)** (1,021 lines)
   - Two integration approaches
   - Complete code examples
   - Testing strategies
   - Best practices
   - Troubleshooting guide

2. **[Project Structure Examples](../examples/PROJECT_STRUCTURE_EXAMPLES.md)** (724 lines)
   - PlatformIO structure
   - Arduino IDE structure
   - Multi-node systems
   - Minimal integration

3. **[Firmware Integration API](FIRMWARE_INTEGRATION.md)**
   - Firmware API reference
   - Factory patterns
   - Configuration access

4. **[Firmware Development Guide](FIRMWARE_DEVELOPMENT_GUIDE.md)**
   - Two-tier architecture
   - Library validation vs application
   - Development patterns

### Working Example

**[PlatformIO Example](../examples/integration/platformio-example/)**

Complete, buildable example with:
- Platform-agnostic application code (`MyMeshApp`)
- ESP32 firmware entry point
- Simulator test harness
- Basic and stress test scenarios
- CMake build configuration

**Build for ESP32:**
```bash
cd examples/integration/platformio-example
pio run -e esp32dev --target upload
```

**Test with Simulator:**
```bash
cd examples/integration/platformio-example/test/simulator
mkdir build && cd build
cmake .. && cmake --build .
./painlessmesh-simulator --config ../scenarios/basic_test.yaml
```

## Integration Approaches

### Approach 1: External Test Project (Recommended)

Keep simulator as separate testing project:
```
your-firmware/
├── lib/MyApp/         # Your application code
├── src/main.cpp       # ESP32 firmware
└── test/simulator/    # Simulator tests (separate)
```

**Pros**: Clean separation, no firmware impact

### Approach 2: Embedded Integration

Add simulator to firmware repository:
```
your-firmware/
├── lib/MyApp/         # Your application code
├── src/main.cpp       # ESP32 firmware
└── simulator/         # Simulator project
```

**Pros**: Single repository, shared code

## Key Principles

### 1. Platform-Agnostic Application Logic

```cpp
// lib/MyApp/MyApp.h - Works on ESP32 AND simulator
class MyApp {
public:
  MyApp(painlessMesh* mesh, Scheduler* scheduler);
  void setup();
  void loop();
  void onReceive(uint32_t from, String& msg);
};
```

### 2. Dependency Injection

```cpp
// Pass dependencies, don't use globals
MyApp::MyApp(painlessMesh* mesh, Scheduler* scheduler)
  : mesh_(mesh), scheduler_(scheduler) {}
```

### 3. Minimal Simulator Adapter

```cpp
// test/simulator/firmware/my_app_firmware.hpp
class MyAppFirmware : public FirmwareBase {
  void setup() override {
    app_ = std::make_unique<MyApp>(mesh_, scheduler_);
    app_->setup();
  }
private:
  std::unique_ptr<MyApp> app_;
};
REGISTER_FIRMWARE(MyApp, MyAppFirmware)
```

### 4. Test Scenarios in YAML

```yaml
# test/simulator/scenarios/test.yaml
simulation:
  name: "My Test"
  duration: 60

nodes:
  - firmware: "MyApp"
    count: 10
    config:
      mesh_prefix: "MyMesh"
```

## Testing Strategy

### Unit Tests
Test your application logic independently with mock mesh.

### Integration Tests
Test complete mesh scenarios with simulator:
- 5-10 nodes for basic functionality
- 20-50 nodes for integration testing
- 100+ nodes for stress testing

### Performance Tests
Benchmark with increasing node counts:
```yaml
# stress_test.yaml
nodes:
  - firmware: "MyApp"
    count: 100
    
events:
  - time: 60
    action: "stop_node"  # Test failure handling
```

## Common Patterns

### Sensor Node
```cpp
class SensorNode {
  void readAndSend() {
    float value = readSensor();
    String msg = "{\"sensor\":\"temp\",\"value\":" + String(value) + "}";
    mesh_->sendBroadcast(msg);
  }
};
```

### Gateway Node
```cpp
class GatewayNode {
  void onReceive(uint32_t from, String& msg) {
    // Forward to external system
    forwardToMQTT(msg);
  }
};
```

### Multi-Role System
```yaml
nodes:
  - firmware: "SensorNode"
    count: 20
  - firmware: "ActuatorNode"
    count: 5
  - firmware: "GatewayNode"
    count: 1
```

## Troubleshooting

### Can't find firmware
**Error**: "Unknown firmware: MyApp"  
**Fix**: Ensure `REGISTER_FIRMWARE(MyApp, MyAppFirmware)` is linked

### Platform-specific code
**Error**: Build fails with Arduino.h errors  
**Fix**: Abstract hardware dependencies or use `#ifdef`

### CMake issues
**Error**: Can't find painlessMesh  
**Fix**: Set `PAINLESSMESH_PATH` in CMake

See [full troubleshooting guide](INTEGRATING_INTO_YOUR_PROJECT.md#troubleshooting) for more.

## Migration Checklist

- [ ] Extract application logic from `main.cpp` into reusable class
- [ ] Remove Arduino/ESP32-specific code from application class
- [ ] Use dependency injection (pass mesh and scheduler)
- [ ] Create simulator firmware adapter extending `FirmwareBase`
- [ ] Register firmware with `REGISTER_FIRMWARE` macro
- [ ] Create test scenarios (YAML files)
- [ ] Set up CMake build for simulator tests
- [ ] Run basic simulation test locally
- [ ] Add CI/CD workflow for automated testing
- [ ] Document firmware configuration options

## Next Steps

1. **Read Integration Guide**: [Integrating into Your Project](INTEGRATING_INTO_YOUR_PROJECT.md)
2. **Study Example**: [PlatformIO Example](../examples/integration/platformio-example/)
3. **Adapt Your Code**: Follow the patterns
4. **Create Scenarios**: Write YAML test files
5. **Run Tests**: Validate with 100+ nodes
6. **Automate**: Add to CI/CD pipeline

## Additional Resources

- [Configuration Guide](CONFIGURATION_GUIDE.md) - Complete YAML reference
- [Getting Started](../GETTING_STARTED.md) - Platform setup
- [Simulator Plan](SIMULATOR_PLAN.md) - Technical specification
- [Example Scenarios](../examples/scenarios/) - Pre-built tests

## Support

- **GitHub Issues**: [Bug reports and questions](https://github.com/Alteriom/painlessMesh-simulator/issues)
- **GitHub Discussions**: [Community Q&A](https://github.com/Alteriom/painlessMesh-simulator/discussions)
- **Documentation Index**: [All docs](SIMULATOR_INDEX.md)

---

**Quick Start**: Copy the [PlatformIO example](../examples/integration/platformio-example/), adapt it to your application, and start testing!

**Last Updated**: 2025-11-18  
**Version**: 1.0.0
