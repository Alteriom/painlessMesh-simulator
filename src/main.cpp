/**
 * @file main.cpp
 * @brief Main entry point for painlessMesh simulator application
 * 
 * This file contains the main() function that orchestrates the simulation
 * lifecycle: parsing arguments, loading configuration, creating nodes,
 * running the simulation, and reporting results.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include "simulator/cli_options.hpp"
#include "simulator/config_loader.hpp"
#include "simulator/node_manager.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <map>
#include <boost/asio.hpp>
#include <csignal>

using namespace simulator;

// Global flag for graceful shutdown
static volatile bool running = true;

/**
 * @brief Signal handler for SIGINT/SIGTERM
 * 
 * Sets the running flag to false to trigger graceful shutdown.
 */
void signalHandler(int signal) {
  std::cout << "\n[INFO] Received signal " << signal << ", shutting down gracefully...\n";
  running = false;
}

/**
 * @brief Apply CLI overrides to configuration
 * 
 * @param config Configuration to modify
 * @param options CLI options with overrides
 */
void applyCliOverrides(ScenarioConfig& config, const CLIOptions& options) {
  if (options.duration) {
    std::cout << "[INFO] Overriding duration: " << *options.duration << " seconds\n";
    config.simulation.duration = *options.duration;
  }
  
  if (options.time_scale) {
    std::cout << "[INFO] Overriding time scale: " << *options.time_scale << "x\n";
    config.simulation.time_scale = *options.time_scale;
  }
  
  if (!options.output_dir.empty()) {
    config.metrics.output = options.output_dir + "/metrics.csv";
  }
}

/**
 * @brief Main entry point
 * 
 * @param argc Argument count
 * @param argv Argument vector
 * @return Exit code (0=success, 1=error, 2=validation failure)
 */
int main(int argc, char* argv[]) {
  try {
    // Parse command-line arguments
    CLIOptions options;
    try {
      options = parseCommandLine(argc, argv);
    } catch (const std::exception& e) {
      std::cerr << "[ERROR] " << e.what() << std::endl;
      return 1;
    }
    
    // Handle --help and --version
    if (options.help || options.version) {
      return 0;
    }
    
    // Load configuration
    std::cout << "[INFO] Loading configuration from: " << options.config_file << std::endl;
    ConfigLoader loader;
    auto config_opt = loader.loadFromFile(options.config_file);
    
    if (!config_opt) {
      std::cerr << "[ERROR] Failed to load configuration: " << loader.getLastError() << std::endl;
      return 1;
    }
    
    auto config = *config_opt;
    
    // Expand templates if present
    if (!config.templates.empty()) {
      std::cout << "[INFO] Expanding " << config.templates.size() << " node templates..." << std::endl;
      loader.expandTemplates(config);
    }
    
    // Apply CLI overrides
    applyCliOverrides(config, options);
    
    // Validate configuration
    std::cout << "[INFO] Validating configuration..." << std::endl;
    auto errors = loader.getValidationErrors(config);
    
    if (!errors.empty()) {
      std::cerr << "[ERROR] Configuration validation failed:\n";
      for (const auto& error : errors) {
        std::cerr << "  - " << error.field << ": " << error.message;
        if (!error.suggestion.empty()) {
          std::cerr << " (Suggestion: " << error.suggestion << ")";
        }
        std::cerr << std::endl;
      }
      return 2;
    }
    
    std::cout << "[INFO] Configuration valid" << std::endl;
    
    // Handle --validate-only mode
    if (options.validate_only) {
      std::cout << "[INFO] Validation successful. Exiting (--validate-only mode)" << std::endl;
      return 0;
    }
    
    // Print simulation info
    std::cout << "\n";
    std::cout << "=== Simulation Configuration ===" << std::endl;
    std::cout << "Name: " << config.simulation.name << std::endl;
    if (!config.simulation.description.empty()) {
      std::cout << "Description: " << config.simulation.description << std::endl;
    }
    std::cout << "Duration: " << (config.simulation.duration > 0 ? 
                                  std::to_string(config.simulation.duration) + " seconds" : 
                                  "infinite") << std::endl;
    std::cout << "Time scale: " << config.simulation.time_scale << "x" << std::endl;
    std::cout << "Node count: " << config.nodes.size() << std::endl;
    std::cout << "Log level: " << options.log_level << std::endl;
    std::cout << "================================\n" << std::endl;
    
    // Create IO context and node manager
    boost::asio::io_context io;
    NodeManager manager(io);
    
    // Install signal handler (using traditional signal handling)
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
    
    // Create nodes from configuration
    std::cout << "[INFO] Creating " << config.nodes.size() << " virtual nodes..." << std::endl;
    
    for (const auto& node_config : config.nodes) {
      try {
        NodeConfig nc;
        nc.nodeId = node_config.nodeId;
        nc.meshPrefix = node_config.mesh_prefix;
        nc.meshPassword = node_config.mesh_password;
        nc.meshPort = node_config.mesh_port;
        nc.firmware = node_config.firmware;
        nc.firmwareConfig = node_config.firmwareConfig;
        
        auto node = manager.createNode(nc);
        
        if (options.log_level == "DEBUG") {
          std::cout << "[DEBUG] Created node " << node_config.nodeId 
                    << " (" << node_config.id << ")" << std::endl;
        }
      } catch (const std::exception& e) {
        std::cerr << "[ERROR] Failed to create node " << node_config.id 
                  << ": " << e.what() << std::endl;
        return 1;
      }
    }
    
    std::cout << "[INFO] Successfully created " << manager.getNodeCount() << " nodes" << std::endl;
    
    // Start all nodes
    std::cout << "[INFO] Starting all nodes..." << std::endl;
    manager.startAll();
    std::cout << "[INFO] All nodes started" << std::endl;
    
    // Establish connectivity between nodes
    std::cout << "[INFO] Establishing mesh connectivity..." << std::endl;
    manager.establishConnectivity();
    std::cout << "[INFO] Mesh connectivity established" << std::endl;
    
    // Run simulation
    std::cout << "\n[INFO] Starting simulation...\n" << std::endl;
    
    auto start_time = std::chrono::steady_clock::now();
    int64_t last_report = -1;
    uint32_t update_count = 0;
    
    while (running) {
      // Update all nodes
      manager.updateAll();
      update_count++;
      
      // Calculate elapsed time
      auto now = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
      
      // Progress reporting every 5 seconds
      if (elapsed > 0 && elapsed % 5 == 0 && elapsed != last_report) {
        std::cout << "[" << elapsed << "s] " 
                  << manager.getNodeCount() << " nodes running, "
                  << update_count << " updates performed" << std::endl;
        last_report = elapsed;
      }
      
      // Check timeout
      if (config.simulation.duration > 0) {
        if (elapsed >= static_cast<int64_t>(config.simulation.duration)) {
          std::cout << "\n[INFO] Simulation duration reached (" 
                    << config.simulation.duration << " seconds)" << std::endl;
          break;
        }
      }
      
      // Small sleep to avoid busy waiting
      // Adjust based on time scale for more accurate simulation
      int sleep_ms = static_cast<int>(10.0f / config.simulation.time_scale);
      if (sleep_ms < 1) sleep_ms = 1;
      std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }
    
    // Stop all nodes
    std::cout << "\n[INFO] Stopping all nodes..." << std::endl;
    manager.stopAll();
    
    // Calculate final statistics
    auto end_time = std::chrono::steady_clock::now();
    auto total_duration = std::chrono::duration_cast<std::chrono::seconds>(
      end_time - start_time).count();
    
    // Report final results
    std::cout << "\n";
    std::cout << "=== Simulation Results ===" << std::endl;
    std::cout << "Total duration: " << total_duration << " seconds" << std::endl;
    std::cout << "Nodes: " << manager.getNodeCount() << std::endl;
    std::cout << "Updates: " << update_count << std::endl;
    std::cout << "Average update rate: " 
              << (total_duration > 0 ? update_count / total_duration : 0) 
              << " updates/sec" << std::endl;
    
    // Report metrics for each node
    std::cout << "\nNode Metrics:" << std::endl;
    uint64_t total_sent = 0, total_received = 0;
    for (const auto& node_id : manager.getNodeIds()) {
      auto node = manager.getNode(node_id);
      if (node) {
        auto metrics = node->getMetrics();
        total_sent += metrics.messages_sent;
        total_received += metrics.messages_received;
        if (options.log_level == "DEBUG") {
          std::cout << "  Node " << node_id 
                    << ": sent=" << metrics.messages_sent
                    << ", received=" << metrics.messages_received << std::endl;
        }
      }
    }
    std::cout << "Total messages sent: " << total_sent << std::endl;
    std::cout << "Total messages received: " << total_received << std::endl;
    std::cout << "==========================" << std::endl;
    
    std::cout << "\n[INFO] Simulation completed successfully" << std::endl;
    return 0;
    
  } catch (const std::exception& e) {
    std::cerr << "\n[ERROR] Unhandled exception: " << e.what() << std::endl;
    return 1;
  } catch (...) {
    std::cerr << "\n[ERROR] Unknown exception occurred" << std::endl;
    return 1;
  }
}
