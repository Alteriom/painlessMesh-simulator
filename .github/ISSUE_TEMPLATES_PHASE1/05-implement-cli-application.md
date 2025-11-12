# [Phase 1.1] Implement CLI Application

**Labels**: `phase-1`, `cli`, `c++`  
**Milestone**: Phase 1 - Core Infrastructure  
**Estimated Time**: 4-6 hours

## Objective

Implement the command-line interface (CLI) application that ties together VirtualNode, NodeManager, and ConfigLoader to run simulations.

## Background

The CLI application is the main entry point for the simulator. It:
- Parses command-line arguments
- Loads configuration files
- Creates and manages nodes
- Runs the simulation loop
- Reports results

## Implementation Specification

### File Structure

- `src/main.cpp` - Main application entry point
- `include/simulator/cli_options.hpp` - CLI option parsing
- `src/cli/cli_parser.cpp` - Argument parser implementation

### Command-Line Interface

```bash
# Basic usage
./painlessmesh-simulator --config scenario.yaml

# With options
./painlessmesh-simulator \
  --config scenario.yaml \
  --duration 120 \
  --log-level DEBUG \
  --output results/ \
  --ui terminal

# Validation only
./painlessmesh-simulator --config scenario.yaml --validate-only

# Help
./painlessmesh-simulator --help
```

### CLI Options Structure

```cpp
// include/simulator/cli_options.hpp

#ifndef SIMULATOR_CLI_OPTIONS_HPP
#define SIMULATOR_CLI_OPTIONS_HPP

#include <string>
#include <optional>

namespace simulator {

enum class LogLevel {
  DEBUG,
  INFO,
  WARNING,
  ERROR
};

enum class UIMode {
  NONE,
  TERMINAL,
  // WEB (future)
};

struct CLIOptions {
  std::string config_file;
  std::optional<uint32_t> duration_override;
  std::optional<float> speed_override;
  LogLevel log_level{LogLevel::INFO};
  std::string log_file;
  UIMode ui_mode{UIMode::NONE};
  std::string output_dir{"results"};
  bool validate_only{false};
  bool headless{false};
  bool show_help{false};
  bool show_version{false};
};

class CLIParser {
public:
  static CLIOptions parse(int argc, char* argv[]);
  static void printHelp();
  static void printVersion();

private:
  static LogLevel parseLogLevel(const std::string& level);
  static UIMode parseUIMode(const std::string& mode);
};

} // namespace simulator

#endif // SIMULATOR_CLI_OPTIONS_HPP
```

### Main Application Flow

```cpp
// src/main.cpp

#include <iostream>
#include <exception>
#include "simulator/cli_options.hpp"
#include "simulator/config_loader.hpp"
#include "simulator/node_manager.hpp"

using namespace simulator;

int main(int argc, char* argv[]) {
  try {
    // Parse command-line options
    auto options = CLIParser::parse(argc, argv);
    
    if (options.show_help) {
      CLIParser::printHelp();
      return 0;
    }
    
    if (options.show_version) {
      CLIParser::printVersion();
      return 0;
    }
    
    // Load configuration
    auto config = ConfigLoader::loadFromFile(options.config_file);
    std::cout << "Loaded scenario: " << config.simulation.name << std::endl;
    
    if (options.validate_only) {
      std::cout << "Configuration is valid." << std::endl;
      return 0;
    }
    
    // Apply CLI overrides
    if (options.duration_override) {
      config.simulation.duration = *options.duration_override;
    }
    if (options.speed_override) {
      config.simulation.time_scale = *options.speed_override;
    }
    
    // Create IO context and node manager
    boost::asio::io_context io;
    NodeManager manager(io);
    
    // Create nodes from configuration
    std::cout << "Creating " << config.nodes.size() << " nodes..." << std::endl;
    for (const auto& node_config : config.nodes) {
      manager.createNode(node_config);
    }
    
    // Start all nodes
    std::cout << "Starting nodes..." << std::endl;
    manager.startAll();
    
    // Run simulation loop
    std::cout << "Running simulation for " << config.simulation.duration 
              << " seconds..." << std::endl;
    
    auto start_time = std::chrono::steady_clock::now();
    uint32_t elapsed = 0;
    
    while (elapsed < config.simulation.duration || config.simulation.duration == 0) {
      manager.updateAll();
      
      // Check elapsed time
      auto now = std::chrono::steady_clock::now();
      elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        now - start_time
      ).count();
      
      // Progress reporting (every 5 seconds)
      if (elapsed % 5 == 0) {
        std::cout << "Elapsed: " << elapsed << "s, Nodes: " 
                  << manager.getNodeCount() << std::endl;
      }
      
      // Sleep to control simulation speed
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    // Stop all nodes
    std::cout << "Stopping nodes..." << std::endl;
    manager.stopAll();
    
    // Report results
    std::cout << "Simulation complete!" << std::endl;
    std::cout << "Total nodes: " << manager.getNodeCount() << std::endl;
    
    return 0;
    
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }
}
```

## Tasks

### Implementation
- [ ] Create `src/main.cpp` with main() function
- [ ] Create `include/simulator/cli_options.hpp`
- [ ] Create `src/cli/cli_parser.cpp`
- [ ] Implement argument parsing (use boost::program_options or similar)
- [ ] Implement --help and --version flags
- [ ] Implement configuration override flags
- [ ] Implement validation-only mode
- [ ] Implement simulation loop
- [ ] Add progress reporting
- [ ] Add graceful shutdown (Ctrl+C handling)

### Testing
- [ ] Create `test/test_cli_parser.cpp`
- [ ] Test argument parsing
- [ ] Test help/version output
- [ ] Test configuration override
- [ ] Test error handling
- [ ] Manual integration test (run actual simulation)

### Documentation
- [ ] Add CLI usage to README.md
- [ ] Document all command-line options
- [ ] Add examples of common usage patterns
- [ ] Document exit codes

### Build System
- [ ] Add main.cpp to CMakeLists.txt
- [ ] Create `painlessmesh-simulator` executable target
- [ ] Link against simulator_lib
- [ ] Add program_options or CLI parsing library

## Acceptance Criteria

✅ **Implementation Complete**
- CLI application compiles and links
- All command-line options work
- Help text is clear and comprehensive
- Error handling is robust

✅ **Functionality**
- Can load and run simulation from YAML config
- Progress reporting works
- Graceful shutdown on Ctrl+C
- Exit codes reflect success/failure

✅ **Testing**
- CLI parser unit tests pass
- Integration test runs 10-node simulation successfully
- Error cases handled gracefully

✅ **Documentation**
- README has CLI usage section
- All options documented
- Examples provided

✅ **CI/CD Passes**
- Builds on all platforms
- Executable runs successfully

## Dependencies

**Depends on**:
- #2 - Implement VirtualNode
- #3 - Implement NodeManager
- #4 - Implement ConfigLoader

**Blocks**:
- Phase 1 completion

## References

- **Technical spec**: `docs/SIMULATOR_PLAN.md` - "Command-Line Interface"
- **Quick start**: `docs/SIMULATOR_QUICKSTART.md`
- **Coding standards**: `.github/copilot-instructions.md`

## Custom Agent Assistance

Use **@cpp-simulator-agent** for:
- CLI parsing library selection
- Simulation loop implementation
- Signal handling (Ctrl+C)
- Error handling patterns

## Implementation Notes

### Argument Parsing

Using boost::program_options:

```cpp
#include <boost/program_options.hpp>

namespace po = boost::program_options;

CLIOptions CLIParser::parse(int argc, char* argv[]) {
  CLIOptions options;
  
  po::options_description desc("painlessMesh Simulator");
  desc.add_options()
    ("help,h", "Show help message")
    ("version,v", "Show version")
    ("config,c", po::value<std::string>(&options.config_file)->required(),
     "Configuration file (YAML)")
    ("duration,d", po::value<uint32_t>(), "Override simulation duration (seconds)")
    ("speed,s", po::value<float>(), "Override time scale")
    ("log-level,l", po::value<std::string>()->default_value("INFO"),
     "Log level (DEBUG, INFO, WARNING, ERROR)")
    ("log-file", po::value<std::string>(&options.log_file),
     "Log file path")
    ("ui", po::value<std::string>()->default_value("none"),
     "UI mode (none, terminal)")
    ("output,o", po::value<std::string>(&options.output_dir)->default_value("results"),
     "Output directory for results")
    ("validate-only", po::bool_switch(&options.validate_only),
     "Validate configuration and exit")
    ("headless", po::bool_switch(&options.headless),
     "Run without UI (for CI)");
  
  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  
  if (vm.count("help")) {
    options.show_help = true;
    return options;
  }
  
  if (vm.count("version")) {
    options.show_version = true;
    return options;
  }
  
  po::notify(vm); // Throws if required options missing
  
  // Parse enums
  if (vm.count("log-level")) {
    options.log_level = parseLogLevel(vm["log-level"].as<std::string>());
  }
  
  if (vm.count("ui")) {
    options.ui_mode = parseUIMode(vm["ui"].as<std::string>());
  }
  
  // Handle overrides
  if (vm.count("duration")) {
    options.duration_override = vm["duration"].as<uint32_t>();
  }
  
  if (vm.count("speed")) {
    options.speed_override = vm["speed"].as<float>();
  }
  
  return options;
}
```

### Signal Handling

```cpp
#include <csignal>

volatile std::sig_atomic_t g_shutdown_requested = 0;

void signal_handler(int signal) {
  g_shutdown_requested = 1;
}

int main(int argc, char* argv[]) {
  // Register signal handler
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);
  
  try {
    // ... setup ...
    
    // Simulation loop
    while (!g_shutdown_requested && elapsed < duration) {
      manager.updateAll();
      // ...
    }
    
    if (g_shutdown_requested) {
      std::cout << "\nShutdown requested, stopping gracefully..." << std::endl;
    }
    
    // ... cleanup ...
  }
  // ...
}
```

## Testing Strategy

### Unit Tests

```cpp
TEST_CASE("CLI parser", "[cli]") {
  SECTION("parses basic options") {
    const char* argv[] = {
      "simulator",
      "--config", "test.yaml",
      "--log-level", "DEBUG"
    };
    int argc = sizeof(argv) / sizeof(argv[0]);
    
    auto options = CLIParser::parse(argc, const_cast<char**>(argv));
    REQUIRE(options.config_file == "test.yaml");
    REQUIRE(options.log_level == LogLevel::DEBUG);
  }
  
  SECTION("requires config file") {
    const char* argv[] = {"simulator"};
    int argc = 1;
    
    REQUIRE_THROWS(CLIParser::parse(argc, const_cast<char**>(argv)));
  }
}
```

### Integration Test

```bash
# Create test scenario
cat > test_scenario.yaml << EOF
simulation:
  name: "CLI Test"
  duration: 10

nodes:
  - template: "basic"
    count: 3
    config:
      mesh_prefix: "TestMesh"

topology:
  type: "random"
EOF

# Run simulator
./painlessmesh-simulator --config test_scenario.yaml --log-level DEBUG

# Verify exit code
echo $? # Should be 0
```

## Success Metrics

- ✅ CLI application runs successfully
- ✅ All options work as documented
- ✅ Can run 10-node simulation from YAML config
- ✅ Help and version flags work
- ✅ Error handling is robust
- ✅ Documentation complete
- ✅ CI/CD passes
- ✅ Code review approved

## Completion Criteria for Phase 1

Once this issue is complete, Phase 1 is done! The simulator should be able to:
- ✅ Load YAML configuration
- ✅ Create 10+ virtual nodes
- ✅ Form mesh network automatically
- ✅ Run simulation for specified duration
- ✅ Report basic results

**Next**: Phase 2 - Scenario Engine
