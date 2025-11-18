# Examples Directory

This directory contains examples demonstrating different aspects of the painlessMesh simulator.

## Quick Navigation

### For Firmware Developers (Start Here!)

**→ [Integration Examples](integration/)** - Complete examples of integrating simulator into your firmware project
- **[PlatformIO Example](integration/platformio-example/)** - Full working example with ESP32 firmware
- **[Project Structure Examples](PROJECT_STRUCTURE_EXAMPLES.md)** - Multiple integration patterns

### Test Scenarios

**→ [Scenarios](scenarios/)** - YAML configuration files for different test scenarios
- Simple mesh formation
- Stress tests with 100+ nodes
- Network partition recovery
- Firmware testing scenarios

### Firmware Examples

**→ [Firmware](firmware/)** - Example firmware implementations
- SimpleBroadcastFirmware
- EchoServerFirmware
- EchoClientFirmware
- LibraryValidationFirmware

### Visualizations

**→ [Visualizations](visualizations/)** - Tools for visualizing mesh topology and metrics

## Documentation

- [Integrating into Your Project](../docs/INTEGRATING_INTO_YOUR_PROJECT.md) - Step-by-step integration guide
- [Configuration Guide](../docs/CONFIGURATION_GUIDE.md) - YAML configuration reference
- [Firmware Integration Guide](../docs/FIRMWARE_INTEGRATION.md) - Firmware API reference

## What Should I Look At?

### I want to test my existing firmware
→ Start with [Integration Examples](integration/)

### I want to see how the simulator works
→ Look at [Scenarios](scenarios/) and run some examples

### I want to write custom firmware for the simulator
→ Check out [Firmware](firmware/) examples

### I want to understand different project structures
→ Read [Project Structure Examples](PROJECT_STRUCTURE_EXAMPLES.md)

---

**Need help?** See the [main documentation](../README.md#documentation) or open a [GitHub Discussion](https://github.com/Alteriom/painlessMesh-simulator/discussions).
