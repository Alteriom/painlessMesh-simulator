/**
 * @file node_crash_event.hpp
 * @brief Event for simulating node crashes/failures
 * 
 * This file contains the NodeCrashEvent class which demonstrates how to
 * create a custom event type for node failure simulation.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef SIMULATOR_NODE_CRASH_EVENT_HPP
#define SIMULATOR_NODE_CRASH_EVENT_HPP

#include "simulator/event.hpp"
#include <cstdint>

namespace simulator {

/**
 * @brief Event that simulates a node crash/failure
 * 
 * This event stops a specific node, simulating a crash or power failure.
 * The node will no longer participate in the mesh network until restarted.
 * 
 * Example usage:
 * @code
 * EventScheduler scheduler;
 * 
 * // Crash node 1001 at t=30 seconds
 * auto event = std::make_unique<NodeCrashEvent>(1001);
 * scheduler.scheduleEvent(std::move(event), 30);
 * @endcode
 */
class NodeCrashEvent : public Event {
public:
  /**
   * @brief Construct a node crash event
   * 
   * @param nodeId ID of node to crash
   */
  explicit NodeCrashEvent(uint32_t nodeId);
  
  /**
   * @brief Execute the node crash
   * 
   * Stops the specified node if it exists and is running.
   * 
   * @param manager Node manager for accessing nodes
   * @param network Network simulator (not used)
   * 
   * @throws std::runtime_error if node doesn't exist
   */
  void execute(NodeManager& manager, NetworkSimulator& network) override;
  
  /**
   * @brief Get event description
   * 
   * @return Description string
   */
  std::string getDescription() const override;

private:
  uint32_t nodeId_;  ///< ID of node to crash
};

} // namespace simulator

#endif // SIMULATOR_NODE_CRASH_EVENT_HPP
