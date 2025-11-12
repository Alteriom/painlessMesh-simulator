# [Phase 1.1] Implement VirtualNode Class

**Labels**: `phase-1`, `core`, `c++`  
**Milestone**: Phase 1 - Core Infrastructure  
**Estimated Time**: 4-6 hours

## Objective

Implement the VirtualNode class that represents a single simulated ESP32/ESP8266 device running painlessMesh.

## Background

VirtualNode is the core abstraction for a simulated mesh node. It wraps a painlessMesh instance, manages node lifecycle, and provides interfaces for firmware integration and metrics collection.

## Implementation Specification

### File Structure

Create these files:
- `include/simulator/virtual_node.hpp` - Public header
- `src/core/virtual_node.cpp` - Implementation
- `test/test_virtual_node.cpp` - Unit tests

### Class Interface

```cpp
// include/simulator/virtual_node.hpp

#ifndef SIMULATOR_VIRTUAL_NODE_HPP
#define SIMULATOR_VIRTUAL_NODE_HPP

#include <memory>
#include <cstdint>
#include <boost/asio.hpp>

// Forward declarations
class Scheduler;
class painlessMesh;
class MeshTest; // From painlessMesh test/boost/

namespace simulator {

struct NodeConfig {
  uint32_t nodeId;
  std::string meshPrefix;
  std::string meshPassword;
  uint16_t meshPort = 5555;
  // Add more config as needed
};

struct NodeMetrics {
  uint32_t messages_sent = 0;
  uint32_t messages_received = 0;
  uint32_t bytes_sent = 0;
  uint32_t bytes_received = 0;
  std::chrono::steady_clock::time_point start_time;
};

class VirtualNode {
public:
  /**
   * @brief Create a virtual mesh node
   * 
   * @param nodeId Unique node identifier (must be non-zero)
   * @param config Node configuration
   * @param scheduler Task scheduler for async operations
   * @param io Boost.Asio IO context
   * 
   * @throws std::invalid_argument if nodeId is 0
   */
  VirtualNode(uint32_t nodeId, 
              const NodeConfig& config,
              Scheduler* scheduler,
              boost::asio::io_context& io);
  
  ~VirtualNode();
  
  // Lifecycle management
  void start();
  void stop();
  void update();
  
  // Accessors
  uint32_t getNodeId() const { return node_id_; }
  painlessMesh& getMesh();
  const painlessMesh& getMesh() const;
  NodeMetrics getMetrics() const { return metrics_; }
  bool isRunning() const { return running_; }
  
  // Configuration
  void setNetworkQuality(float quality); // 0.0-1.0

private:
  uint32_t node_id_;
  std::unique_ptr<MeshTest> mesh_;
  Scheduler* scheduler_;
  boost::asio::io_context& io_;
  NodeMetrics metrics_;
  bool running_{false};
  
  // Callbacks
  void onReceive(uint32_t from, std::string& msg);
  void onNewConnection(uint32_t nodeId);
  void onChangedConnections();
};

} // namespace simulator

#endif // SIMULATOR_VIRTUAL_NODE_HPP
```

## Tasks

### Implementation
- [ ] Create `include/simulator/virtual_node.hpp` with class interface
- [ ] Create `src/core/virtual_node.cpp` with implementation
- [ ] Implement constructor with validation
- [ ] Implement `start()` method to initialize mesh
- [ ] Implement `stop()` method for graceful shutdown
- [ ] Implement `update()` method for periodic updates
- [ ] Implement mesh callbacks (onReceive, onNewConnection, onChangedConnections)
- [ ] Implement metrics collection
- [ ] Add proper error handling and logging

### Testing
- [ ] Create `test/test_virtual_node.cpp`
- [ ] Test node creation and destruction
- [ ] Test start/stop lifecycle
- [ ] Test invalid node ID rejection
- [ ] Test mesh callbacks
- [ ] Test metrics collection
- [ ] Achieve 80%+ code coverage

### Documentation
- [ ] Add Doxygen comments to all public methods
- [ ] Document exceptions that can be thrown
- [ ] Add usage examples in header comments
- [ ] Update README with VirtualNode description

### Build System
- [ ] Add virtual_node.cpp to CMakeLists.txt
- [ ] Add test_virtual_node to test suite
- [ ] Verify builds on Linux, macOS, Windows (via CI)

## Acceptance Criteria

✅ **Implementation Complete**
- VirtualNode class compiles without warnings
- All methods implemented according to spec
- Proper RAII resource management
- Thread-safe where applicable

✅ **Testing Complete**
- All unit tests pass
- Code coverage ≥ 80%
- Tests cover normal and error cases
- Integration with MeshTest verified

✅ **Documentation Complete**
- All public APIs documented
- Code examples provided
- Doxygen generates clean docs

✅ **CI/CD Passes**
- Builds on all platforms
- All tests pass
- No compiler warnings

## Dependencies

**Depends on**:
- #1 - Add painlessMesh submodule

**Blocks**:
- #3 - Implement NodeManager (uses VirtualNode)

## References

- **Technical spec**: `docs/SIMULATOR_PLAN.md` - "VirtualNode Class API" section
- **Coding standards**: `.github/copilot-instructions.md` - "VirtualNode Implementation"
- **Test infrastructure**: `external/painlessMesh/test/boost/` - MeshTest examples
- **Agent**: `.github/agents/cpp-simulator-agent.md`

## Custom Agent Assistance

Use **@cpp-simulator-agent** for:
- Boost.Asio integration patterns
- Memory management strategies
- Testing approaches
- Performance optimization

## Implementation Notes

### Using MeshTest Wrapper

```cpp
// VirtualNode constructor
VirtualNode::VirtualNode(uint32_t nodeId, const NodeConfig& config,
                         Scheduler* scheduler, boost::asio::io_context& io)
  : node_id_(nodeId), scheduler_(scheduler), io_(io) {
  
  if (nodeId == 0) {
    throw std::invalid_argument("Node ID must be non-zero");
  }
  
  // Create MeshTest wrapper (from painlessMesh test infrastructure)
  mesh_ = std::make_unique<MeshTest>(scheduler, nodeId, io);
  
  // Configure mesh
  mesh_->init(config.meshPrefix, config.meshPassword, config.meshPort);
  
  // Set up callbacks
  mesh_->setReceiveCallback([this](uint32_t from, String& msg) {
    onReceive(from, msg);
  });
}
```

### Lifecycle Management

```cpp
void VirtualNode::start() {
  if (running_) {
    throw std::logic_error("Node already running");
  }
  
  mesh_->init(); // Start mesh networking
  running_ = true;
  metrics_.start_time = std::chrono::steady_clock::now();
}

void VirtualNode::stop() {
  if (!running_) return;
  
  mesh_->stop();
  running_ = false;
}

void VirtualNode::update() {
  if (!running_) return;
  
  scheduler_->execute(); // Process scheduled tasks
  // Update metrics
}
```

### Metrics Collection

Track:
- Messages sent/received
- Bytes sent/received
- Connection count
- Uptime
- Mesh topology changes

## Testing Strategy

### Unit Tests

```cpp
// test/test_virtual_node.cpp
#include <catch2/catch_test_macros.hpp>
#include "simulator/virtual_node.hpp"

TEST_CASE("VirtualNode lifecycle", "[virtual_node]") {
  Scheduler scheduler;
  boost::asio::io_context io;
  
  NodeConfig config;
  config.nodeId = 1001;
  config.meshPrefix = "TestMesh";
  config.meshPassword = "test123";
  
  SECTION("can be created") {
    REQUIRE_NOTHROW(VirtualNode(1001, config, &scheduler, io));
  }
  
  SECTION("rejects zero node ID") {
    config.nodeId = 0;
    REQUIRE_THROWS_AS(VirtualNode(0, config, &scheduler, io),
                      std::invalid_argument);
  }
  
  SECTION("can be started and stopped") {
    VirtualNode node(1001, config, &scheduler, io);
    
    REQUIRE_FALSE(node.isRunning());
    
    node.start();
    REQUIRE(node.isRunning());
    
    node.stop();
    REQUIRE_FALSE(node.isRunning());
  }
}
```

## Success Metrics

- ✅ VirtualNode class implemented per specification
- ✅ All unit tests pass (80%+ coverage)
- ✅ Documentation complete
- ✅ CI/CD pipeline passes
- ✅ No memory leaks (valgrind clean)
- ✅ Code review approved
