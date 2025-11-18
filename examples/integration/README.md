# Integration Examples

This directory contains complete, working examples of how to integrate the painlessMesh simulator into different types of firmware projects.

## Available Examples

### 1. PlatformIO Example

**Path**: `platformio-example/`

Shows how to integrate simulator testing into a PlatformIO project.

**What it demonstrates**:
- Organizing code for both ESP32 and simulator
- Creating a simulator test harness
- Building and running tests
- Example test scenarios

**Best for**: PlatformIO users, ESP32 developers

### 2. Arduino IDE Example

**Path**: `arduino-example/` (Coming soon)

Shows how to test Arduino sketches with the simulator.

**What it demonstrates**:
- Adapting Arduino .ino files for testing
- Minimal integration approach
- Building with CMake

**Best for**: Arduino IDE users, hobbyists

### 3. Advanced Multi-Node Example

**Path**: `multi-node-example/` (Coming soon)

Shows a complete IoT system with different node types.

**What it demonstrates**:
- Sensor nodes
- Actuator nodes
- Gateway nodes
- Complex test scenarios
- Performance testing

**Best for**: Production systems, complex applications

## Quick Start

Each example contains:
- Complete source code
- Build instructions
- Test scenarios
- Detailed README

Pick the example that matches your development environment and follow its README.

## Documentation

- [Integrating into Your Project Guide](../../docs/INTEGRATING_INTO_YOUR_PROJECT.md) - Step-by-step integration guide
- [Project Structure Examples](../PROJECT_STRUCTURE_EXAMPLES.md) - More structure patterns
- [Firmware Integration Guide](../../docs/FIRMWARE_INTEGRATION.md) - Firmware API reference

## Contributing Examples

Have a working integration example? We'd love to include it! Please:

1. Follow the structure of existing examples
2. Include a comprehensive README
3. Add working test scenarios
4. Submit a pull request

---

**Note**: These examples are meant to be copied and adapted for your own projects. They are fully functional starting points, not libraries to depend on.
