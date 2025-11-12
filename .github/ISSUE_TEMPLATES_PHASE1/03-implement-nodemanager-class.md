# [Phase 1.1] Implement NodeManager Class

**Labels**: `phase-1`, `core`, `c++`  
**Milestone**: Phase 1 - Core Infrastructure  
**Estimated Time**: 4-6 hours

## Objective

Implement the NodeManager class that creates, manages, and coordinates multiple VirtualNode instances.

## Background

NodeManager is responsible for:
- Creating and destroying virtual nodes
- Managing node lifecycle (start/stop all nodes)
- Coordinating updates across all nodes
- Providing query interface for node information

## Implementation Specification

### File Structure

- `include/simulator/node_manager.hpp` - Public header
- `src/core/node_manager.cpp` - Implementation
- `test/test_node_manager.cpp` - Unit tests

### Class Interface

```cpp
// include/simulator/node_manager.hpp

#ifndef SIMULATOR_NODE_MANAGER_HPP
#define SIMULATOR_NODE_MANAGER_HPP

#include <memory>
#include <map>
#include <vector>
#include "simulator/virtual_node.hpp"

namespace simulator {

class NodeManager {
public:
  /**
   * @brief Create a node manager
   * 
   * @param io Boost.Asio IO context for networking
   */
  explicit NodeManager(boost::asio::io_context& io);
  
  ~NodeManager();
  
  // Node lifecycle
  /**
   * @brief Create a new virtual node
   * 
   * @param config Node configuration
   * @return Shared pointer to created node
   * 
   * @throws std::runtime_error if node with same ID exists
   * @throws std::runtime_error if max nodes reached
   */
  std::shared_ptr<VirtualNode> createNode(const NodeConfig& config);
  
  /**
   * @brief Remove a node by ID
   * 
   * @param nodeId ID of node to remove
   * @return true if node was removed, false if not found
   */
  bool removeNode(uint32_t nodeId);
  
  /**
   * @brief Start all nodes
   */
  void startAll();
  
  /**
   * @brief Stop all nodes
   */
  void stopAll();
  
  /**
   * @brief Update all nodes (process scheduler, etc.)
   */
  void updateAll();
  
  // Queries
  size_t getNodeCount() const { return nodes_.size(); }
  std::shared_ptr<VirtualNode> getNode(uint32_t nodeId);
  std::vector<uint32_t> getNodeIds() const;
  std::vector<std::shared_ptr<VirtualNode>> getAllNodes() const;
  
  // Resource limits
  static constexpr size_t MAX_NODES = 1000;

private:
  boost::asio::io_context& io_;
  Scheduler scheduler_;
  std::map<uint32_t, std::shared_ptr<VirtualNode>> nodes_;
  uint32_t next_node_id_{1000};
};

} // namespace simulator

#endif // SIMULATOR_NODE_MANAGER_HPP
```

## Tasks

### Implementation
- [ ] Create `include/simulator/node_manager.hpp`
- [ ] Create `src/core/node_manager.cpp`
- [ ] Implement constructor and destructor
- [ ] Implement `createNode()` with validation
- [ ] Implement `removeNode()` with cleanup
- [ ] Implement `startAll()` / `stopAll()`
- [ ] Implement `updateAll()` for coordinated updates
- [ ] Implement query methods
- [ ] Add resource limit enforcement (MAX_NODES)
- [ ] Add proper error handling and logging

### Testing
- [ ] Create `test/test_node_manager.cpp`
- [ ] Test node creation and removal
- [ ] Test duplicate node ID rejection
- [ ] Test max nodes limit enforcement
- [ ] Test start/stop all functionality
- [ ] Test query methods
- [ ] Test multi-node mesh formation (integration test)
- [ ] Achieve 80%+ code coverage

### Documentation
- [ ] Add Doxygen comments to all public methods
- [ ] Document thread safety considerations
- [ ] Add usage examples
- [ ] Update README with NodeManager description

### Build System
- [ ] Add node_manager.cpp to CMakeLists.txt
- [ ] Add test_node_manager to test suite
- [ ] Verify builds on all platforms

## Acceptance Criteria

✅ **Implementation Complete**
- NodeManager class compiles without warnings
- All methods implemented per specification
- Proper resource management (RAII)
- Thread-safe where applicable

✅ **Testing Complete**
- All unit tests pass
- Integration tests verify multi-node mesh formation
- Code coverage ≥ 80%
- Tests cover normal and error cases

✅ **Documentation Complete**
- All public APIs documented
- Examples provided
- Thread safety documented

✅ **CI/CD Passes**
- Builds on all platforms
- All tests pass
- No warnings or errors

## Dependencies

**Depends on**:
- #2 - Implement VirtualNode class

**Blocks**:
- #5 - Implement CLI application (uses NodeManager)

## References

- **Technical spec**: `docs/SIMULATOR_PLAN.md` - "NodeManager Implementation"
- **Coding standards**: `.github/copilot-instructions.md`
- **Agent**: `.github/agents/cpp-simulator-agent.md`

## Custom Agent Assistance

Use **@cpp-simulator-agent** for:
- Multi-node coordination patterns
- Resource management strategies
- Testing approaches
- Performance optimization

## Implementation Notes

### Node Creation

```cpp
std::shared_ptr<VirtualNode> NodeManager::createNode(const NodeConfig& config) {
  // Validate
  if (config.nodeId == 0) {
    throw std::invalid_argument("Node ID must be non-zero");
  }
  
  if (nodes_.count(config.nodeId) > 0) {
    throw std::runtime_error("Node ID already exists");
  }
  
  if (nodes_.size() >= MAX_NODES) {
    throw std::runtime_error("Maximum node count reached");
  }
  
  // Create node
  auto node = std::make_shared<VirtualNode>(
    config.nodeId, config, &scheduler_, io_
  );
  
  // Store in map
  nodes_[config.nodeId] = node;
  
  return node;
}
```

### Coordinated Updates

```cpp
void NodeManager::updateAll() {
  // Process scheduler tasks
  scheduler_.execute();
  
  // Update each node
  for (auto& [id, node] : nodes_) {
    node->update();
  }
  
  // Poll IO context
  io_.poll();
}
```

### Lifecycle Management

```cpp
void NodeManager::startAll() {
  for (auto& [id, node] : nodes_) {
    if (!node->isRunning()) {
      node->start();
    }
  }
}

void NodeManager::stopAll() {
  for (auto& [id, node] : nodes_) {
    if (node->isRunning()) {
      node->stop();
    }
  }
}
```

## Testing Strategy

### Unit Tests

```cpp
TEST_CASE("NodeManager operations", "[node_manager]") {
  boost::asio::io_context io;
  NodeManager manager(io);
  
  SECTION("can create nodes") {
    NodeConfig config{1001, "TestMesh", "pass123"};
    auto node = manager.createNode(config);
    
    REQUIRE(node != nullptr);
    REQUIRE(node->getNodeId() == 1001);
    REQUIRE(manager.getNodeCount() == 1);
  }
  
  SECTION("rejects duplicate node IDs") {
    NodeConfig config{1001, "TestMesh", "pass123"};
    manager.createNode(config);
    
    REQUIRE_THROWS_AS(manager.createNode(config), std::runtime_error);
  }
  
  SECTION("enforces MAX_NODES limit") {
    // Test is commented out for speed, but should be implemented
    // for (size_t i = 0; i < NodeManager::MAX_NODES; ++i) {
    //   NodeConfig config{1000 + i, "TestMesh", "pass123"};
    //   REQUIRE_NOTHROW(manager.createNode(config));
    // }
    // 
    // NodeConfig one_too_many{2000, "TestMesh", "pass123"};
    // REQUIRE_THROWS_AS(manager.createNode(one_too_many), std::runtime_error);
  }
}
```

### Integration Tests

```cpp
TEST_CASE("Multi-node mesh formation", "[integration]") {
  boost::asio::io_context io;
  NodeManager manager(io);
  
  // Create 5 nodes
  for (uint32_t i = 0; i < 5; ++i) {
    NodeConfig config{1001 + i, "TestMesh", "pass123"};
    manager.createNode(config);
  }
  
  // Start all nodes
  manager.startAll();
  
  // Run simulation for a while
  for (int i = 0; i < 1000; ++i) {
    manager.updateAll();
  }
  
  // Verify all nodes see each other
  auto nodes = manager.getAllNodes();
  for (auto& node : nodes) {
    auto mesh_nodes = node->getMesh().getNodeList();
    REQUIRE(mesh_nodes.size() == 5);
  }
  
  // Stop all
  manager.stopAll();
}
```

## Success Metrics

- ✅ NodeManager implemented per specification
- ✅ Can manage 10+ nodes simultaneously
- ✅ All tests pass (80%+ coverage)
- ✅ Integration test verifies mesh formation
- ✅ Documentation complete
- ✅ CI/CD passes
- ✅ Code review approved
