# [Phase 1.2] Implement Configuration Loader

**Labels**: `phase-1`, `configuration`, `yaml`  
**Milestone**: Phase 1 - Core Infrastructure  
**Estimated Time**: 6-8 hours

## Objective

Implement YAML configuration loading system to define simulation scenarios, node configurations, and network topologies.

## Background

ConfigLoader parses YAML files to create simulation scenarios with:
- Node definitions and templates
- Network topology specifications
- Simulation parameters
- Validation and error handling

## Implementation Specification

### File Structure

- `include/simulator/config_loader.hpp` - Public header
- `src/config/config_loader.cpp` - Implementation
- `test/test_config_loader.cpp` - Unit tests
- `examples/scenarios/simple_mesh.yaml` - Example config

### Class Interface

```cpp
// include/simulator/config_loader.hpp

#ifndef SIMULATOR_CONFIG_LOADER_HPP
#define SIMULATOR_CONFIG_LOADER_HPP

#include <string>
#include <vector>
#include <optional>
#include "simulator/virtual_node.hpp"

namespace simulator {

struct SimulationConfig {
  std::string name;
  uint32_t duration{0}; // 0 = infinite
  float time_scale{1.0};
  uint32_t seed{0};
};

struct TopologyConfig {
  enum class Type {
    RANDOM,
    STAR,
    RING,
    MESH,
    CUSTOM
  };
  
  Type type{Type::RANDOM};
  std::string hub; // For star topology
  float density{0.3}; // For random topology
  std::vector<std::pair<std::string, std::string>> connections; // For custom
};

struct ScenarioConfig {
  SimulationConfig simulation;
  std::vector<NodeConfig> nodes;
  TopologyConfig topology;
};

class ConfigLoader {
public:
  /**
   * @brief Load scenario configuration from YAML file
   * 
   * @param filepath Path to YAML configuration file
   * @return Parsed scenario configuration
   * 
   * @throws std::runtime_error if file not found
   * @throws std::runtime_error if YAML parsing fails
   * @throws std::invalid_argument if validation fails
   */
  static ScenarioConfig loadFromFile(const std::string& filepath);
  
  /**
   * @brief Load scenario configuration from YAML string
   * 
   * @param yaml_content YAML content as string
   * @return Parsed scenario configuration
   */
  static ScenarioConfig loadFromString(const std::string& yaml_content);
  
  /**
   * @brief Validate scenario configuration
   * 
   * @param config Configuration to validate
   * @return true if valid, false otherwise
   */
  static bool validate(const ScenarioConfig& config);
  
  /**
   * @brief Get validation errors
   * 
   * @param config Configuration to validate
   * @return Vector of error messages (empty if valid)
   */
  static std::vector<std::string> getValidationErrors(const ScenarioConfig& config);

private:
  static SimulationConfig parseSimulation(const YAML::Node& yaml);
  static std::vector<NodeConfig> parseNodes(const YAML::Node& yaml);
  static TopologyConfig parseTopology(const YAML::Node& yaml);
  static void expandNodeTemplates(std::vector<NodeConfig>& nodes);
};

} // namespace simulator

#endif // SIMULATOR_CONFIG_LOADER_HPP
```

## Tasks

### Implementation
- [ ] Create `include/simulator/config_loader.hpp`
- [ ] Create `src/config/config_loader.cpp`
- [ ] Implement `loadFromFile()` with file I/O
- [ ] Implement `loadFromString()` for testing
- [ ] Implement YAML parsing for simulation settings
- [ ] Implement YAML parsing for node definitions
- [ ] Implement node template expansion (count, id_prefix)
- [ ] Implement topology configuration parsing
- [ ] Implement validation logic
- [ ] Add detailed error messages for validation failures

### Testing
- [ ] Create `test/test_config_loader.cpp`
- [ ] Test valid configuration loading
- [ ] Test invalid YAML syntax handling
- [ ] Test missing required fields
- [ ] Test node template expansion
- [ ] Test validation error messages
- [ ] Test file not found handling
- [ ] Achieve 80%+ code coverage

### Example Scenarios
- [ ] Create `examples/scenarios/simple_mesh.yaml`
- [ ] Create `examples/scenarios/stress_test.yaml`
- [ ] Create `examples/scenarios/star_topology.yaml`
- [ ] Add comments explaining each configuration option

### Documentation
- [ ] Add Doxygen comments to all public methods
- [ ] Document YAML schema in `docs/CONFIGURATION_GUIDE.md`
- [ ] Add examples to README
- [ ] Document error codes and messages

### Build System
- [ ] Add config_loader.cpp to CMakeLists.txt
- [ ] Add yaml-cpp dependency
- [ ] Add test_config_loader to test suite

## Acceptance Criteria

✅ **Implementation Complete**
- ConfigLoader compiles without warnings
- YAML parsing works correctly
- Node template expansion implemented
- Validation logic comprehensive

✅ **Testing Complete**
- All unit tests pass
- Code coverage ≥ 80%
- Tests cover normal and error cases
- Example scenarios load successfully

✅ **Documentation Complete**
- YAML schema documented
- Examples provided
- Error messages clear and actionable

✅ **CI/CD Passes**
- Builds on all platforms
- yaml-cpp dependency resolved
- All tests pass

## Dependencies

**Depends on**:
- #1 - Add painlessMesh submodule (for types)
- #2 - Implement VirtualNode (for NodeConfig type)

**Blocks**:
- #5 - Implement CLI (uses ConfigLoader)

## References

- **Technical spec**: `docs/SIMULATOR_PLAN.md` - "Configuration File Format"
- **YAML schema**: `.github/agents/scenario-config-agent.md`
- **Agent**: `.github/agents/scenario-config-agent.md`

## Custom Agent Assistance

Use **@scenario-config-agent** for:
- YAML schema design
- Validation rules
- Error message writing
- Example scenario creation

## YAML Schema

### Simple Example

```yaml
# examples/scenarios/simple_mesh.yaml
simulation:
  name: "Simple 10-Node Mesh"
  duration: 60  # seconds
  time_scale: 1.0

nodes:
  - template: "basic"
    count: 10
    id_prefix: "node-"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "test123"
      mesh_port: 5555

topology:
  type: "random"
  density: 0.3
```

### With Individual Nodes

```yaml
simulation:
  name: "Star Topology"
  duration: 120

nodes:
  # Hub node
  - id: "hub-1"
    config:
      mesh_prefix: "StarMesh"
      mesh_password: "pass123"
  
  # Template nodes
  - template: "sensor"
    count: 5
    id_prefix: "sensor-"
    config:
      mesh_prefix: "StarMesh"
      mesh_password: "pass123"

topology:
  type: "star"
  hub: "hub-1"
```

## Implementation Notes

### YAML Parsing

```cpp
ScenarioConfig ConfigLoader::loadFromFile(const std::string& filepath) {
  // Check file exists
  if (!std::filesystem::exists(filepath)) {
    throw std::runtime_error("Config file not found: " + filepath);
  }
  
  // Load YAML
  YAML::Node yaml = YAML::LoadFile(filepath);
  
  // Parse sections
  ScenarioConfig config;
  config.simulation = parseSimulation(yaml["simulation"]);
  config.nodes = parseNodes(yaml["nodes"]);
  config.topology = parseTopology(yaml["topology"]);
  
  // Expand templates
  expandNodeTemplates(config.nodes);
  
  // Validate
  auto errors = getValidationErrors(config);
  if (!errors.empty()) {
    std::string msg = "Configuration validation failed:\n";
    for (const auto& err : errors) {
      msg += "  - " + err + "\n";
    }
    throw std::invalid_argument(msg);
  }
  
  return config;
}
```

### Node Template Expansion

```cpp
void ConfigLoader::expandNodeTemplates(std::vector<NodeConfig>& nodes) {
  std::vector<NodeConfig> expanded;
  
  for (const auto& node : nodes) {
    if (node.count > 1) {
      // Expand template
      for (uint32_t i = 0; i < node.count; ++i) {
        NodeConfig instance = node;
        instance.nodeId = 1000 + expanded.size();
        if (!node.id_prefix.empty()) {
          instance.id = node.id_prefix + std::to_string(i);
        }
        expanded.push_back(instance);
      }
    } else {
      expanded.push_back(node);
    }
  }
  
  nodes = std::move(expanded);
}
```

### Validation

```cpp
std::vector<std::string> ConfigLoader::getValidationErrors(
    const ScenarioConfig& config) {
  std::vector<std::string> errors;
  
  // Validate simulation
  if (config.simulation.name.empty()) {
    errors.push_back("Simulation name is required");
  }
  
  if (config.simulation.time_scale <= 0) {
    errors.push_back("Time scale must be positive");
  }
  
  // Validate nodes
  if (config.nodes.empty()) {
    errors.push_back("At least one node is required");
  }
  
  // Check for duplicate node IDs
  std::set<uint32_t> node_ids;
  for (const auto& node : config.nodes) {
    if (node.nodeId == 0) {
      errors.push_back("Node ID must be non-zero");
    }
    if (!node_ids.insert(node.nodeId).second) {
      errors.push_back("Duplicate node ID: " + std::to_string(node.nodeId));
    }
    if (node.meshPrefix.empty()) {
      errors.push_back("mesh_prefix is required for all nodes");
    }
  }
  
  // Validate topology
  if (config.topology.type == TopologyConfig::Type::STAR && 
      config.topology.hub.empty()) {
    errors.push_back("Star topology requires 'hub' to be specified");
  }
  
  return errors;
}
```

## Testing Strategy

```cpp
TEST_CASE("ConfigLoader operations", "[config_loader]") {
  SECTION("loads valid configuration") {
    const char* yaml = R"(
      simulation:
        name: "Test"
        duration: 60
      nodes:
        - template: "basic"
          count: 3
          config:
            mesh_prefix: "TestMesh"
      topology:
        type: "random"
    )";
    
    auto config = ConfigLoader::loadFromString(yaml);
    REQUIRE(config.simulation.name == "Test");
    REQUIRE(config.nodes.size() == 3);
  }
  
  SECTION("validates required fields") {
    const char* yaml = R"(
      simulation:
        duration: 60
      nodes: []
    )";
    
    REQUIRE_THROWS_AS(ConfigLoader::loadFromString(yaml), std::invalid_argument);
  }
}
```

## Success Metrics

- ✅ Configuration loading works correctly
- ✅ All validation rules enforced
- ✅ Example scenarios load successfully
- ✅ All tests pass (80%+ coverage)
- ✅ Documentation complete
- ✅ CI/CD passes
- ✅ Code review approved
