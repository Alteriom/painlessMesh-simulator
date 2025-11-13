# painlessMesh Device Simulator

[![CI](https://github.com/Alteriom/painlessMesh-simulator/workflows/CI/badge.svg)](https://github.com/Alteriom/painlessMesh-simulator/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

A standalone device simulator for testing and validating painlessMesh networks with 100+ virtual nodes.

## Overview

The painlessMesh Device Simulator enables:

- 🚀 **Large-Scale Testing**: Spawn 100+ virtual ESP32/ESP8266 mesh nodes
- 🔧 **Firmware Validation**: Test actual firmware code without hardware
- 📋 **Scenario-Based Testing**: Configure complex test scenarios via YAML
- 🌐 **Network Simulation**: Realistic conditions (latency, packet loss, partitions)
- 📊 **Performance Analysis**: Collect metrics and visualize topology
- 🔄 **CI/CD Integration**: Automated firmware testing

## Quick Start

### Installation

```bash
# Clone repository with submodules
git clone --recursive https://github.com/Alteriom/painlessMesh-simulator.git
cd painlessMesh-simulator

# Install dependencies (Ubuntu/Debian)
sudo apt-get install cmake ninja-build libboost-dev libyaml-cpp-dev

# Build
mkdir build && cd build
cmake -G Ninja ..
ninja

# Run example
./painlessmesh-simulator --config ../examples/scenarios/simple_mesh.yaml
```

### Basic Usage

Create a scenario file `my_test.yaml`:

```yaml
simulation:
  name: "My First Test"
  duration: 60

nodes:
  - template: "basic"
    count: 10
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"

topology:
  type: "random"

metrics:
  output: "results/my_test.csv"
```

Run the simulation:

```bash
./painlessmesh-simulator --config my_test.yaml
```

## Features

### Configuration-Driven

Define complex scenarios with simple YAML configuration:

```yaml
simulation:
  name: "Network Partition Test"
  duration: 300
  time_scale: 5.0  # 5x speed

nodes:
  - template: "sensor"
    count: 50
    firmware: "examples/firmware/sensor_node"

events:
  - time: 120
    action: "partition_network"
    groups: [[0-24], [25-49]]
  - time: 240
    action: "heal_partition"
```

### Firmware Integration

Test your actual ESP32/ESP8266 firmware:

```cpp
class MySensorFirmware : public FirmwareBase {
public:
  void setup() override {
    task_.set(30000, TASK_FOREVER, [this]() {
      sendSensorData();
    });
    scheduler_->addTask(task_);
    task_.enable();
  }
  
  void loop() override {
    // Your loop code
  }
  
  void onReceive(uint32_t from, String& msg) override {
    // Handle messages
  }

private:
  void sendSensorData() {
    String data = "{\"temp\": 25.5, \"humidity\": 60}";
    mesh_->sendBroadcast(data);
  }
  
  Task task_;
};
```

### Performance Testing

Validate performance with increasing node counts:

| Nodes | Real-time Factor | Memory |
|-------|------------------|---------|
| 10    | 1.0x (real-time) | 50 MB   |
| 50    | 0.5x (2x slower) | 200 MB  |
| 100   | 0.2x (5x slower) | 400 MB  |
| 200   | 0.1x (10x slower)| 800 MB  |

### CI/CD Integration

Integrate with GitHub Actions:

```yaml
name: Firmware Tests

on: [push, pull_request]

jobs:
  simulate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
        with:
          submodules: recursive
      
      - name: Install dependencies
        run: sudo apt-get install cmake ninja-build libboost-dev
      
      - name: Build simulator
        run: |
          cmake -G Ninja .
          ninja
      
      - name: Run simulation tests
        run: |
          ./bin/painlessmesh-simulator \
            --config scenarios/validation.yaml \
            --log-level WARN
```

## Documentation

### User Documentation

- **[Quick Start Guide](docs/SIMULATOR_QUICKSTART.md)**: Get started in 5 minutes
- **[Complete Plan](docs/SIMULATOR_PLAN.md)**: Full technical specification
- **[Executive Summary](docs/SIMULATOR_SUMMARY.md)**: Overview and benefits
- **[Documentation Index](docs/SIMULATOR_INDEX.md)**: Navigate all docs

### AI Agent Documentation

- **[MCP Configuration](.github/MCP_CONFIGURATION.md)**: GitHub MCP server setup and capabilities
- **[MCP Quick Reference](.github/MCP_QUICK_REFERENCE.md)**: Essential commands for AI agents
- **[Agent Session Template](.github/AGENT_SESSION_TEMPLATE.md)**: Workflow template for coding sessions
- **[Copilot Instructions](.github/copilot-instructions.md)**: Coding standards and best practices

### Architecture Documentation (Coming Soon)

- `docs/ARCHITECTURE.md`: System design and components
- `docs/CONFIGURATION_GUIDE.md`: Scenario configuration reference
- `docs/FIRMWARE_DEVELOPMENT.md`: Creating custom firmware
- `docs/API_REFERENCE.md`: API documentation

## Example Scenarios

### Simple Mesh Formation
```bash
# Run basic mesh with default settings
./painlessmesh-simulator --config examples/scenarios/simple_mesh.yaml
```

### Stress Test (100+ Nodes)
```bash
# Run stress test with debug logging and custom output directory
./painlessmesh-simulator --config examples/scenarios/stress_test.yaml \
  --log-level DEBUG \
  --output stress_test_results/
```

### Network Partition Recovery
```bash
# Run partition test at 5x speed to complete faster
./painlessmesh-simulator --config examples/scenarios/partition_recovery.yaml \
  --time-scale 5.0
```

### Fast Validation
```bash
# Validate configuration without running simulation
./painlessmesh-simulator --config my_scenario.yaml --validate-only
```

## Command-Line Options

### Required Options

| Option | Short | Description |
|--------|-------|-------------|
| `--config <file>` | `-c` | Path to YAML scenario configuration file |

### Optional Overrides

Override configuration file settings from the command line:

| Option | Short | Default | Description |
|--------|-------|---------|-------------|
| `--duration <seconds>` | `-d` | (from config) | Override simulation duration in seconds |
| `--time-scale <factor>` | `-t` | `1.0` | Time scale multiplier (e.g., `2.0` = 2x speed, `0.5` = half speed) |
| `--output <dir>` | `-o` | `results/` | Output directory for results and metrics |

### Logging and Display

Control logging verbosity and output format:

| Option | Short | Default | Description |
|--------|-------|---------|-------------|
| `--log-level <level>` | `-l` | `INFO` | Logging level: `DEBUG`, `INFO`, `WARN`, `ERROR` |
| `--ui <mode>` | `-u` | `none` | UI mode: `none`, `terminal` |

### Validation and Information

| Option | Short | Description |
|--------|-------|-------------|
| `--validate-only` | | Validate configuration file and exit without running simulation |
| `--help` | `-h` | Display help message and exit |
| `--version` | `-v` | Display version information and exit |

### Usage Examples

**Basic simulation:**
```bash
./painlessmesh-simulator --config scenario.yaml
```

**Fast-forward 5x speed with 2 minute duration:**
```bash
./painlessmesh-simulator --config scenario.yaml --time-scale 5.0 --duration 120
```

**Debug logging with custom output directory:**
```bash
./painlessmesh-simulator --config scenario.yaml \
  --log-level DEBUG \
  --output my_results/
```

**Validate configuration without running:**
```bash
./painlessmesh-simulator --config scenario.yaml --validate-only
```

**Terminal UI for real-time monitoring:**
```bash
./painlessmesh-simulator --config scenario.yaml --ui terminal
```

**Combined options for CI/CD:**
```bash
./painlessmesh-simulator --config test_scenario.yaml \
  --duration 60 \
  --time-scale 10.0 \
  --log-level WARN \
  --output ci_results/
```

### Exit Codes

The simulator uses standard exit codes to indicate the result of execution:

| Exit Code | Meaning | Example Scenario |
|-----------|---------|------------------|
| `0` | Success | Simulation completed successfully or help/version displayed |
| `1` | General error | Missing config file, invalid options, runtime error, file I/O error |
| `2` | Validation failure | Configuration file validation failed (with `--validate-only` or before simulation) |

**Example usage in scripts:**

```bash
# Validate before running
if ! ./painlessmesh-simulator --config scenario.yaml --validate-only; then
  echo "Configuration validation failed!"
  exit 1
fi

# Run simulation and check result
./painlessmesh-simulator --config scenario.yaml
if [ $? -eq 0 ]; then
  echo "Simulation completed successfully"
else
  echo "Simulation failed with exit code $?"
  exit 1
fi
```

### Common Tips

> **Tip**: Use `--validate-only` to quickly check configuration files for errors before committing to a long simulation run.

> **Tip**: Set `--time-scale` greater than 1.0 to speed up simulations for faster testing. For example, `--time-scale 10.0` runs a 10-minute simulation in 1 minute.

> **Tip**: Use `--log-level DEBUG` when troubleshooting issues, but keep it at `INFO` or higher for production runs to avoid performance overhead.

> **Warning**: Very high time-scale values (>10.0) may cause timing inaccuracies in the simulation. Use with caution for performance testing scenarios.

## Building from Source

### Prerequisites

**Required:**
- C++14 compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10+
- Boost 1.66+ (Boost.Asio)
- yaml-cpp

**Optional:**
- ncurses (for terminal UI)
- GraphViz (for topology export)
- Catch2 (for testing, included as submodule)

### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  cmake \
  ninja-build \
  libboost-dev \
  libboost-system-dev \
  libyaml-cpp-dev \
  libncurses-dev
```

### macOS

```bash
brew install cmake ninja boost yaml-cpp ncurses
```

### Windows

Use vcpkg:
```powershell
vcpkg install boost-asio yaml-cpp
```

### Build

```bash
# Clone with submodules
git clone --recursive https://github.com/Alteriom/painlessMesh-simulator.git
cd painlessMesh-simulator

# If you already cloned without --recursive, initialize submodules:
git submodule update --init --recursive

# Build
mkdir build && cd build
cmake -G Ninja ..
ninja

# Run tests
ninja test

# Install (optional)
sudo ninja install
```

## Development Status

### Current: Phase 1 - Foundation Setup ✅

- [x] Repository structure
- [x] Custom coding instructions
- [x] CI/CD infrastructure setup
- [x] Phase 1 implementation issue templates created
- [ ] Core VirtualNode implementation
- [ ] NodeManager implementation
- [ ] Basic configuration system

### Creating Phase 1 Issues

**⚡ Quick Start**: See [QUICK_ISSUE_CREATION.md](QUICK_ISSUE_CREATION.md) for one-command setup!

Detailed issue templates are ready in `.github/ISSUE_TEMPLATES_PHASE1/`. Create them using:

```bash
# Test GitHub CLI authentication first
./scripts/test-gh-auth.sh

# Create all Phase 1 issues
./scripts/create-phase1-issues.sh

# Or manually: see ISSUES_TO_CREATE.md for detailed instructions
```

**Note**: Requires GitHub CLI authentication. See [MCP Configuration](.github/MCP_CONFIGURATION.md) for setup.

### Roadmap

**Phase 1: Core Infrastructure** (Weeks 1-2) - [Issue Templates Ready]
- VirtualNode class (#1, #2)
- NodeManager (#3)
- Configuration system (#4)
- Basic CLI (#5)

**Phase 2: Scenario Engine** (Weeks 3-4)
- Event-based scenarios
- Network simulation
- Topology configuration

**Phase 3: Firmware Integration** (Weeks 5-6)
- FirmwareBase interface
- Mock Arduino/ESP APIs
- Example firmware modules

**Phase 4: Visualization & Metrics** (Weeks 7-8)
- Terminal UI
- Metrics collection
- Export capabilities

**Phase 5: Polish & Documentation** (Weeks 9-10)
- Complete testing
- Full documentation
- Release v1.0.0

## Contributing

We welcome contributions! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

### Development Setup

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/my-feature`)
3. Make your changes
4. Add tests for new functionality
5. Run tests (`ninja test`)
6. Commit your changes (`git commit -am 'Add new feature'`)
7. Push to the branch (`git push origin feature/my-feature`)
8. Open a Pull Request

### Code Style

- Follow C++14 standards
- Use 2-space indentation
- Follow naming conventions in `.github/copilot-instructions.md`
- Document all public APIs
- Write tests for new features

## Use Cases

### Library Developers
- Test painlessMesh with 100+ nodes locally
- Reproduce complex bugs easily
- Validate changes before release
- Performance benchmarking

### Firmware Developers
- Test firmware without hardware
- Rapid iteration cycles
- Edge case validation
- CI/CD integration

### QA Engineers
- Automated regression testing
- Performance monitoring
- Scenario coverage
- Consistent test environments

### Researchers
- Network topology experiments
- Protocol evaluation
- Algorithm validation
- Academic studies

## Architecture

```
┌─────────────────────────────────────────┐
│      Simulator Application               │
├─────────────────────────────────────────┤
│  Config   │  Scenario  │  Visualization │
│  Loader   │  Engine    │  Engine        │
├─────────────────────────────────────────┤
│         Node Manager                     │
│  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐  │
│  │Node 1│ │Node 2│ │Node 3│ │ ...  │  │
│  └──────┘ └──────┘ └──────┘ └──────┘  │
├─────────────────────────────────────────┤
│      Network Simulator (Boost.Asio)     │
├─────────────────────────────────────────┤
│      painlessMesh Library Core          │
└─────────────────────────────────────────┘
```

## Technology Stack

- **Language**: C++14
- **Networking**: Boost.Asio
- **Configuration**: yaml-cpp
- **Testing**: Catch2
- **Build**: CMake + Ninja
- **Mesh Library**: painlessMesh (submodule)

## License

MIT License - See [LICENSE](LICENSE) file for details.

## Support

- **Documentation**: [Full documentation index](docs/SIMULATOR_INDEX.md)
- **Issues**: [GitHub Issues](https://github.com/Alteriom/painlessMesh-simulator/issues)
- **Discussions**: [GitHub Discussions](https://github.com/Alteriom/painlessMesh-simulator/discussions)

## Related Projects

- [painlessMesh](https://github.com/Alteriom/painlessMesh): The mesh networking library
- [Alteriom Firmware](https://github.com/Alteriom): IoT firmware solutions

## Acknowledgments

Built on top of the excellent [painlessMesh](https://github.com/gmag11/painlessMesh) library by gmag11 and contributors.

---

**Status**: 🚧 Under Active Development  
**Version**: 0.1.0-alpha  
**Last Updated**: 2025-11-12

For detailed implementation plans, see [SIMULATOR_PLAN.md](docs/SIMULATOR_PLAN.md)
