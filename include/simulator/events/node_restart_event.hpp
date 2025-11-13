/**
 * @file node_restart_event.hpp
 * @brief Event for restarting a node
 * 
 * This file contains the NodeRestartEvent class for restarting nodes
 * with optional delay.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef SIMULATOR_NODE_RESTART_EVENT_HPP
#define SIMULATOR_NODE_RESTART_EVENT_HPP

#include "simulator/event.hpp"
#include <cstdint>

namespace simulator {

/**
 * @brief Event that restarts a node
 * 
 * This event stops and then starts a node, simulating a restart.
 * The restart happens immediately (stop followed by start).
 * For delayed restarts, schedule a NodeStopEvent followed by
 * a NodeStartEvent at a later time.
 * 
 * Example usage:
 * @code
 * EventScheduler scheduler;
 * 
 * // Restart node 1001 at t=30 seconds
 * auto event = std::make_unique<NodeRestartEvent>(1001);
 * scheduler.scheduleEvent(std::move(event), 30);
 * @endcode
 */
class NodeRestartEvent : public Event {
public:
  /**
   * @brief Construct a node restart event
   * 
   * @param nodeId ID of node to restart
   */
  explicit NodeRestartEvent(uint32_t nodeId);
  
  /**
   * @brief Execute the node restart
   * 
   * Restarts the specified node if it exists.
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
  uint32_t nodeId_;  ///< ID of node to restart
};

} // namespace simulator

#endif // SIMULATOR_NODE_RESTART_EVENT_HPP
