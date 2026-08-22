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
#include "simulator/topology_planner.hpp"
#include "simulator/event_factory.hpp"
#include "simulator/event_scheduler.hpp"
#include "simulator/network_simulator.hpp"
#include "simulator/firmware/firmware_factory.hpp"
#include "simulator/firmware/simple_broadcast_firmware.hpp"
#include "simulator/firmware/library_validation_firmware.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <map>
#include <boost/asio.hpp>
#include <csignal>
#include <random>

using namespace simulator;

// Global flag for graceful shutdown
static volatile bool running = true;

// The signal that stopped the run, or 0 if it ended on its own. The run loop's
// duration-reached path leaves `running` set, so `received_signal` (equivalently
// !running after the loop) is what distinguishes an interrupted run from a
// completed one -- an interrupt can leave arbitrary events still pending.
static volatile std::sig_atomic_t received_signal = 0;

/**
 * @brief Signal handler for SIGINT/SIGTERM
 *
 * Sets the running flag to false to trigger graceful shutdown.
 */
void signalHandler(int signal) {
  std::cout << "\n[INFO] Received signal " << signal << ", shutting down gracefully...\n";
  received_signal = signal;
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
    // Most built-in firmware self-registers via REGISTER_FIRMWARE() at static-init time
    // (see the whole-archive link in CMakeLists.txt, without which none of it arrives).
    // LibraryValidationFirmware does not use the macro, so register it by hand.
    firmware::FirmwareFactory::instance().registerFirmware("library_validation",
      []() { return std::make_unique<firmware::LibraryValidationFirmware>(); });
    
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
    // Collect EVERY configuration problem before deciding, so a single
    // --validate-only pass surfaces all of them together. Short-circuiting on
    // the first (e.g. an unknown event action) hid firmware and topology errors
    // behind it, which let a CI allowlist that recognised the first diagnostic
    // skip a scenario that was also broken in another way.
    std::vector<std::string> problems;
    for (const auto& error : loader.getValidationErrors(config)) {
      problems.push_back(error.field + ": " + error.message +
                         (error.suggestion.empty()
                              ? std::string()
                              : " (Suggestion: " + error.suggestion + ")"));
    }

    // Feasibility checks a normal run performs -- run them as part of validation
    // too, and append rather than return, so --validate-only (and the CI sweep)
    // reject a scenario an ordinary invocation would. All are pure over the
    // config and need no nodes; a fixed seed is fine since none depends on the
    // random draw.
    {
      const auto feas = planTopology(config.topology, config.nodes,
                                     config.events, config.simulation.seed);
      if (!feas.unwireable_preferred.empty()) {
        problems.push_back(std::to_string(feas.unwireable_preferred.size()) +
                           " event-named link(s) cannot be wired -- not an edge "
                           "of the declared topology, or would close a cycle "
                           "painlessMesh will not hold");
      }
      for (const auto& bad : feas.infeasible_partitions) {
        problems.push_back(bad);
      }
      for (const auto& node : config.nodes) {
        if (!node.firmware.empty() &&
            !firmware::FirmwareFactory::instance().isRegistered(node.firmware)) {
          problems.push_back("node '" + node.id +
                             "' names unregistered firmware '" + node.firmware +
                             "'");
        }
      }
      std::map<std::string, uint32_t> id_to_node_id;
      for (const auto& node_config : config.nodes) {
        id_to_node_id[node_config.id] = node_config.nodeId;
      }
      EventScheduler probe;
      std::vector<std::string> skipped;
      EventFactory::scheduleAll(config.events, id_to_node_id, probe, skipped);
      for (const auto& sk : skipped) {
        problems.push_back("event cannot be scheduled: " + sk);
      }
    }

    if (!problems.empty()) {
      std::cerr << "[ERROR] Configuration validation failed:\n";
      for (const auto& p : problems) {
        std::cerr << "  - " << p << std::endl;
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
    if (config.simulation.time_scale != 1.0f) {
      // time_scale only divides the loop's sleep, i.e. how often nodes are
      // polled. It cannot compress the run: painlessMesh's TaskScheduler, its
      // ack timeouts and its connection timers all read millis(), which the
      // boost build wires to gettimeofday(). There is no virtual clock to
      // advance, so `duration`, the scenario timeline and every mesh timer stay
      // on the wall clock. Said out loud, because the docs read as if a 5x
      // scenario finished five times sooner.
      std::cout << "[WARN] time_scale raises the poll rate only. Mesh timers "
                   "and the event timeline run on the wall clock, so a "
                << config.simulation.duration << "s scenario still takes "
                << config.simulation.duration << "s." << std::endl;
    }
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

    // A scenario that names firmware which never loads produced a run in which no node
    // did anything -- and, before this check, still exited 0. That made the simulator
    // useless as a CI gate, so fail the run instead of reporting a hollow success.
    if (manager.getFirmwareLoadFailureCount() > 0) {
      std::cerr << "[ERROR] " << manager.getFirmwareLoadFailureCount()
                << " of " << manager.getNodeCount()
                << " nodes could not load their configured firmware." << std::endl;
      std::cerr << "[ERROR] Available firmware: ";
      const auto available = firmware::FirmwareFactory::instance().getRegisteredNames();
      for (size_t i = 0; i < available.size(); ++i) {
        std::cerr << (i ? ", " : "") << available[i];
      }
      std::cerr << std::endl;
      return 1;
    }
    
    // Start all nodes
    std::cout << "[INFO] Starting all nodes..." << std::endl;
    manager.startAll();
    std::cout << "[INFO] All nodes started" << std::endl;
    
    // Resolve the run seed once. SimulationConfig documents seed 0 as "random",
    // so draw a real one when it is unset -- otherwise every seedless scenario
    // would wire the identical spanning tree run to run, now that the plan
    // drives the actual mesh. Log the drawn value so a run that surfaces
    // something interesting can be reproduced by pinning it. CI scenarios that
    // need determinism set an explicit seed.
    uint32_t run_seed = config.simulation.seed;
    if (run_seed == 0) {
      run_seed = std::random_device{}();
      std::cout << "[INFO] No simulation.seed set; drew random seed " << run_seed
                << " (set simulation.seed: " << run_seed << " to reproduce)"
                << std::endl;
    }

    // Establish connectivity between nodes. The scenario's topology block was
    // parsed and validated from the first release but never applied: every run
    // got a random spanning tree, so a full-mesh scenario dropping a named pair
    // reported "0 live endpoint(s) closed" because that pair was never wired.
    std::cout << "[INFO] Establishing mesh connectivity..." << std::endl;
    const auto plan = planTopology(config.topology, config.nodes, config.events,
                                   run_seed);
    for (const auto& warning : plan.warnings) {
      std::cout << "[WARN] topology: " << warning << std::endl;
    }
    // An event-named link that cannot be wired (it would close a cycle
    // painlessMesh will not hold) would make its scheduled drop/restore/degrade
    // a silent no-op, running the timeline the author did not ask for. Fail
    // instead of executing a hollow event.
    if (!plan.unwireable_preferred.empty()) {
      std::cerr << "[ERROR] " << plan.unwireable_preferred.size()
                << " event-named link(s) cannot be wired -- not an edge of the "
                << "declared topology, or would close a cycle painlessMesh will "
                << "not hold; those link events would run against no live link"
                << std::endl;
      return 2;  // configuration validation failure
    }
    for (const auto& bad : plan.infeasible_partitions) {
      std::cerr << "[ERROR] " << bad << std::endl;
    }
    if (!plan.infeasible_partitions.empty()) {
      return 2;  // configuration validation failure
    }
    size_t wired = 0;
    if (plan.links.empty() && !config.topology.declared) {
      // No topology block at all: keep the historical random tree so a scenario
      // without one behaves as before. A *declared* topology that plans no
      // links is a different thing -- an error, handled below -- not a licence
      // to substitute unrelated random links for what the author wrote.
      manager.establishConnectivity();
      std::cout << "[INFO] Mesh connectivity established (random tree)"
                << std::endl;
    } else if (plan.links.empty()) {
      // Declared, but nothing to wire: e.g. a custom topology whose only links
      // were invalid, or a single-node scenario. Do not paper over it.
      if (config.nodes.size() < 2) {
        std::cout << "[INFO] Single node; no connectivity to establish"
                  << std::endl;
      } else {
        std::cerr << "[ERROR] Topology '"
                  << topologyTypeName(config.topology.type)
                  << "' was declared but planned no links; refusing to fall "
                  << "back to a random mesh" << std::endl;
        return 2;  // configuration validation failure
      }
    } else {
      wired = manager.establishConnectivity(plan.links);
      std::cout << "[INFO] Mesh connectivity established (topology="
                << topologyTypeName(config.topology.type) << ", " << wired
                << " of " << plan.links.size() << " planned link(s) wired, "
                << plan.declared << " declared)" << std::endl;
      if (wired < plan.links.size()) {
        std::cerr << "[ERROR] " << (plan.links.size() - wired)
                  << " declared link(s) could not be wired" << std::endl;
        return 1;
      }
    }
    
    // Build the scenario timeline. The config loader has parsed and validated
    // config.events all along; until now nothing turned those records into Event
    // objects, so every scenario ran as a static mesh for its duration.
    EventScheduler event_scheduler;
    NetworkSimulator network(run_seed);
    {
      std::map<std::string, uint32_t> id_to_node_id;
      for (const auto& node_config : config.nodes) {
        id_to_node_id[node_config.id] = node_config.nodeId;
      }
      std::vector<std::string> skipped;
      const size_t scheduled = EventFactory::scheduleAll(
          config.events, id_to_node_id, event_scheduler, skipped);
      if (!config.events.empty()) {
        std::cout << "[INFO] Scheduled " << scheduled << " of "
                  << config.events.size() << " scenario events" << std::endl;
      }
      for (const auto& s : skipped) {
        std::cerr << "[WARN] Skipped event: " << s << std::endl;
      }
      // A scenario whose timeline cannot be built is not the scenario the author
      // wrote. Fail rather than run a silently different test.
      if (!skipped.empty()) {
        std::cerr << "[ERROR] " << skipped.size()
                  << " scenario event(s) could not be scheduled." << std::endl;
        return 2;  // configuration validation failure
      }
    }

    // Run simulation
    std::cout << "\n[INFO] Starting simulation...\n" << std::endl;
    
    auto start_time = std::chrono::steady_clock::now();
    int64_t last_report = -1;
    uint32_t update_count = 0;
    
    while (running) {
      // Calculate elapsed time
      auto now = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();

      // Fire any scenario events that have come due BEFORE advancing the nodes,
      // so the declared timestamp is the actual state boundary: a stop_node or
      // partition at t takes effect before any firmware traffic at t, rather
      // than one update() tick after it.
      if (event_scheduler.hasPendingEvents()) {
        event_scheduler.processEvents(static_cast<uint32_t>(elapsed), manager, network);
      }

      // Update all nodes
      manager.updateAll();
      update_count++;
      
      // Progress reporting every 5 seconds
      if (elapsed > 0 && elapsed % 5 == 0 && elapsed != last_report) {
        // Live links, not just running nodes: a mesh can report every node up
        // while carrying nothing, which is how findings 9, 10 and 14 all hid.
        size_t live_links = 0;
        for (const auto& node : manager.getAllNodes()) {
          live_links += node->getConnectionCount();
        }
        std::cout << "[" << elapsed << "s] " 
                  << manager.getRunningCount() << "/" << manager.getNodeCount()
                  << " nodes running, "
                  << (live_links / 2) << " live link(s), "
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

    // A scheduled event that threw was logged and skipped, not fatal in the
    // moment -- but the timeline did not fully execute, so the run did not
    // succeed. Without this a start/restart event that cannot rebuild its
    // transport still exits 0 under "completed successfully", and a gate or
    // experiment accepts a timeline that never ran.
    const size_t event_failures = event_scheduler.getFailedCount();
    if (event_failures > 0) {
      std::cerr << "\n[ERROR] " << event_failures
                << " scheduled event(s) failed to execute; the timeline did not "
                << "run to completion" << std::endl;
      return 1;
    }

    // A SIGINT/SIGTERM clears `running`; the duration-reached path leaves it
    // set. If we were interrupted with events still queued, the requested
    // timeline never finished -- report it rather than exiting 0 as "completed
    // successfully", which a gate or experiment would accept as a full run. An
    // interrupt that arrives after the last event already ran is a clean stop.
    if (received_signal != 0 && event_scheduler.hasPendingEvents()) {
      std::cerr << "\n[ERROR] Interrupted by signal " << received_signal
                << " with " << event_scheduler.getPendingEventCount()
                << " scheduled event(s) still pending; the timeline did not run "
                << "to completion" << std::endl;
      return 128 + static_cast<int>(received_signal);
    }

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
