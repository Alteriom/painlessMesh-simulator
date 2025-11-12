# GitHub Copilot Custom Instructions - painlessMesh Simulator

## Project Overview

This is the **painlessMesh Device Simulator**, a standalone application for testing and validating mesh networks with 100+ virtual nodes. It enables firmware validation, scenario-based testing, and performance analysis without requiring physical hardware.

## Code Quality Standards

### Core Principles
1. **Clarity over Cleverness**: Write clear, maintainable code
2. **Test-Driven Development**: Write tests first, then implement
3. **Minimal Dependencies**: Use only necessary external libraries
4. **Performance Matters**: Optimize for handling 100+ concurrent nodes
5. **Documentation Required**: All public APIs must be documented

### C++ Standards

#### Style Guide
- **Standard**: C++14 (compatible with GCC 7+, Clang 5+, MSVC 2017+)
- **Formatting**: Follow existing codebase style
  - Indentation: 2 spaces
  - Braces: K&R style (opening brace on same line)
  - Naming:
    - Classes: `PascalCase` (e.g., `VirtualNode`)
    - Functions/Methods: `camelCase` (e.g., `getNodeId()`)
    - Variables: `snake_case` (e.g., `node_count`)
    - Constants: `UPPER_SNAKE_CASE` (e.g., `MAX_NODES`)
    - Private members: trailing underscore (e.g., `mesh_`)

#### Memory Management
- Prefer `std::unique_ptr` and `std::shared_ptr` over raw pointers
- Use RAII pattern for resource management
- Avoid manual `new`/`delete` when possible

#### Error Handling
- Use exceptions for exceptional conditions
- Return `std::optional` or `std::expected` for expected failures
- Provide meaningful error messages
- Log errors appropriately

### Architecture Guidelines

#### Component Structure
```
simulator/
├── core/           # VirtualNode, NodeManager
├── config/         # Configuration loading/parsing
├── scenario/       # Scenario engine
├── network/        # Network simulation
├── firmware/       # Firmware integration framework
├── metrics/        # Metrics collection
└── visualization/  # UI and export
```

#### Design Patterns
- **Dependency Injection**: Pass dependencies via constructors
- **Factory Pattern**: For creating nodes and firmware instances
- **Observer Pattern**: For event notifications
- **Strategy Pattern**: For different topologies and behaviors

#### Key Interfaces

**VirtualNode**: Represents a single simulated ESP32/ESP8266 device
```cpp
class VirtualNode {
public:
  VirtualNode(uint32_t nodeId, const NodeConfig& config, 
              Scheduler* scheduler, boost::asio::io_context& io);
  void start();
  void stop();
  void update();
  painlessMesh& getMesh();
  NodeMetrics getMetrics() const;
};
```

**FirmwareBase**: Base class for custom firmware implementations
```cpp
class FirmwareBase {
public:
  virtual ~FirmwareBase() = default;
  virtual void setup() = 0;
  virtual void loop() = 0;
  virtual void onReceive(uint32_t from, String& msg) = 0;
protected:
  painlessMesh* mesh_;
  Scheduler* scheduler_;
};
```

## Testing Requirements

### Test Categories

1. **Unit Tests**: Test individual components in isolation
   - Use Catch2 framework
   - Mock external dependencies
   - Aim for 80%+ code coverage

2. **Integration Tests**: Test component interactions
   - Test multi-node scenarios (2-10 nodes)
   - Verify mesh formation
   - Validate message routing

3. **System Tests**: Test complete scenarios
   - Run example scenarios
   - Validate 100+ node performance
   - Measure resource usage

4. **Performance Tests**: Benchmark critical paths
   - Node spawn time
   - Message throughput
   - Memory usage scaling

### Test Structure
```cpp
TEST_CASE("VirtualNode lifecycle") {
  SECTION("can be created and started") {
    // Arrange
    // Act
    // Assert
  }
  
  SECTION("can be stopped gracefully") {
    // Test code
  }
}
```

## CI/CD Requirements

### Build System
- **Build Tool**: CMake 3.10+
- **Generator**: Ninja (fast incremental builds)
- **Platforms**: Linux (primary), macOS, Windows

### CI Pipeline
1. **Build**: Compile on all platforms
2. **Lint**: Run clang-format, cppcheck
3. **Test**: Execute all test suites
4. **Coverage**: Generate code coverage report
5. **Benchmark**: Run performance tests
6. **Package**: Create release artifacts

### Automation
- **On Push**: Run full CI pipeline
- **On PR**: Run CI + additional validation
- **On Tag**: Build release artifacts
- **Scheduled**: Nightly stress tests

## Configuration Standards

### YAML Format
```yaml
simulation:
  name: "Test Name"
  duration: 60  # seconds
  time_scale: 1.0  # real-time multiplier

nodes:
  - template: "sensor"
    count: 10
    firmware: "path/to/firmware"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"

topology:
  type: "random"  # random, star, ring, mesh, custom

metrics:
  output: "results/metrics.csv"
  interval: 5
```

### Validation
- Schema validation on load
- Fail fast with clear error messages
- Provide helpful suggestions for common mistakes

## Documentation Standards

### Code Documentation
```cpp
/**
 * @brief Creates a virtual mesh node
 * 
 * @param nodeId Unique identifier for the node
 * @param config Node configuration parameters
 * @param scheduler Task scheduler instance
 * @param io Boost.Asio IO context
 * 
 * @throws std::invalid_argument if nodeId is 0
 * @throws std::runtime_error if initialization fails
 */
VirtualNode(uint32_t nodeId, const NodeConfig& config, 
            Scheduler* scheduler, boost::asio::io_context& io);
```

### File Headers
```cpp
/**
 * @file virtual_node.hpp
 * @brief Virtual node implementation for mesh simulation
 * 
 * This file contains the VirtualNode class which simulates a single
 * ESP32/ESP8266 device running painlessMesh.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */
```

### Markdown Documentation
- Clear headings and sections
- Code examples with syntax highlighting
- Visual diagrams where helpful
- Table of contents for long documents

## Performance Considerations

### Optimization Guidelines
1. **Avoid Premature Optimization**: Profile first
2. **Efficient Data Structures**: Choose appropriate containers
3. **Minimize Allocations**: Reuse objects when possible
4. **Asynchronous Operations**: Use Boost.Asio efficiently
5. **Lazy Initialization**: Defer expensive operations

### Performance Targets
- 10 nodes: 1.0x real-time
- 50 nodes: 0.5x real-time (2x slower)
- 100 nodes: 0.2x real-time (5x slower)
- 200 nodes: 0.1x real-time (10x slower)

### Monitoring
- Track CPU usage per component
- Monitor memory allocations
- Measure message throughput
- Log performance bottlenecks

## Common Patterns

### Node Creation
```cpp
auto config = NodeConfig::fromYAML(yaml_node);
auto node = std::make_shared<VirtualNode>(nodeId, config, scheduler, io);
node->start();
```

### Firmware Loading
```cpp
auto firmware = FirmwareFactory::create("sensor_node");
node->loadFirmware(firmware);
```

### Scenario Execution
```cpp
auto scenario = ScenarioLoader::load("scenario.yaml");
auto engine = ScenarioEngine(nodeManager);
engine.run(scenario);
```

### Metrics Collection
```cpp
auto metrics = metricsCollector.getMetrics();
metrics.exportToCSV("results.csv");
```

## Security Considerations

### Input Validation
- Validate all configuration inputs
- Sanitize file paths
- Limit resource usage (max nodes, memory)
- Prevent injection attacks

### Safe Coding
- Avoid buffer overflows
- Check array bounds
- Validate pointer access
- Handle integer overflow

## Integration with painlessMesh

### Submodule Management
```bash
# Initial setup
git submodule add https://github.com/Alteriom/painlessMesh.git external/painlessMesh
git submodule update --init --recursive

# Update to latest
cd external/painlessMesh
git pull origin master
cd ../..
git add external/painlessMesh
git commit -m "Update painlessMesh submodule"
```

### Build Integration
```cmake
# In CMakeLists.txt
add_subdirectory(external/painlessMesh)
target_link_libraries(simulator PRIVATE painlessmesh)
```

## Troubleshooting

### Common Issues

**Issue**: Nodes not forming mesh
- Check mesh prefix/password match
- Verify network simulator is running
- Enable debug logging

**Issue**: Performance degradation
- Profile with valgrind/perf
- Check for memory leaks
- Review message throughput

**Issue**: Build failures
- Verify dependencies installed
- Check CMake version
- Update submodules

## Getting Help

### Resources
- **SIMULATOR_PLAN.md**: Complete technical specification
- **SIMULATOR_QUICKSTART.md**: Usage examples
- **docs/ARCHITECTURE.md**: System architecture
- **docs/TROUBLESHOOTING.md**: Common problems

### Community
- GitHub Issues: Bug reports
- GitHub Discussions: Q&A
- Wiki: Community guides

## Coding Session Workflow

### When Starting Work
1. Pull latest changes
2. Update submodules
3. Review relevant documentation
4. Check existing tests
5. Plan minimal changes

### During Development
1. Write test first (TDD)
2. Implement minimal code
3. Run tests frequently
4. Check performance impact
5. Document as you go

### Before Committing
1. Run full test suite
2. Check code formatting
3. Update documentation
4. Review changes
5. Write clear commit message

### Commit Message Format
```
<type>(<scope>): <subject>

<body>

<footer>
```

Types: feat, fix, docs, style, refactor, test, chore

Example:
```
feat(virtual-node): Add firmware loading support

- Implement FirmwareBase interface
- Add firmware factory
- Update VirtualNode to accept firmware
- Add tests for firmware lifecycle

Closes #123
```

## AI Coding Assistant Guidelines

### When Using GitHub Copilot

**DO:**
- ✅ Review and understand suggested code
- ✅ Adapt suggestions to project style
- ✅ Add tests for suggested implementations
- ✅ Document suggested functions
- ✅ Verify security implications

**DON'T:**
- ❌ Accept suggestions blindly
- ❌ Skip testing suggested code
- ❌ Ignore style guidelines
- ❌ Commit untested code
- ❌ Bypass security checks

### Context to Provide
When asking for help, include:
- Relevant code snippets
- Error messages (full stack trace)
- What you've tried
- Expected vs actual behavior
- Platform/environment details

## Project-Specific Rules

### painlessMesh Integration
- Always use the latest stable submodule version
- Test compatibility when updating painlessMesh
- Maintain backward compatibility when possible
- Document any painlessMesh-specific quirks

### Alteriom Packages
- Support SensorPackage, CommandPackage, StatusPackage
- Follow Alteriom package format conventions
- Provide examples for each package type
- Test integration with Alteriom firmware

### Boost.Asio Usage
- Use async operations for network I/O
- Properly handle io_context lifecycle
- Avoid blocking operations
- Use strand for thread safety

## Version Compatibility

### Minimum Versions
- C++14
- CMake 3.10
- Boost 1.66
- GCC 7 / Clang 5 / MSVC 2017

### Recommended Versions
- C++17 (when available)
- CMake 3.20
- Boost 1.75
- GCC 10 / Clang 12 / MSVC 2019

## License and Copyright

### License
MIT License - See LICENSE file

### Copyright Notice
```cpp
/**
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */
```

### Third-Party Code
- Document all third-party code
- Include original license
- Credit original authors
- Check license compatibility

---

**Document Version**: 1.0
**Last Updated**: 2025-11-12
**Status**: Active

These instructions ensure consistent, high-quality development across the painlessMesh simulator project.
