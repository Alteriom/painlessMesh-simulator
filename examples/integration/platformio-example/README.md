# PlatformIO Integration Example

This example demonstrates integrating the painlessMesh simulator into a PlatformIO project for testing ESP32 mesh firmware.

## Quick Start

**Build for ESP32:**
```bash
pio run -e esp32dev
pio run -e esp32dev --target upload
```

**Test with Simulator:**
```bash
cd test/simulator
mkdir build && cd build
cmake ..
cmake --build .
./painlessmesh-simulator --config ../scenarios/basic_test.yaml
```

## Project Structure

```
platformio-example/
├── lib/MyMeshApp/        # Platform-agnostic application code
├── src/main.cpp          # ESP32 firmware entry point
├── test/simulator/       # Simulator test harness
│   ├── CMakeLists.txt
│   ├── firmware/         # Simulator adapter
│   └── scenarios/        # Test scenarios (YAML)
└── platformio.ini
```

## Key Files

### lib/MyMeshApp/MyMeshApp.h

Platform-agnostic application logic that works on both ESP32 and simulator.

### src/main.cpp

ESP32 firmware that creates the mesh and instantiates your app.

### test/simulator/firmware/my_mesh_app_firmware.hpp

Simulator adapter that wraps your application for testing.

### test/simulator/scenarios/basic_test.yaml

Test scenario configuration.

## How It Works

1. **Shared Logic**: `MyMeshApp` contains your application code
2. **ESP32 Builds**: Uses `src/main.cpp` with PlatformIO
3. **Simulator Builds**: Uses `test/simulator/` with CMake
4. **Same Code**: Application logic is identical in both

## Benefits

- ✅ Test with 100+ nodes without hardware
- ✅ Fast iteration (no flashing required)
- ✅ Reproducible test scenarios
- ✅ CI/CD integration
- ✅ Same code on hardware and simulator

## Full Documentation

See [Integrating into Your Project Guide](../../../docs/INTEGRATING_INTO_YOUR_PROJECT.md) for complete details.
