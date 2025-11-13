/**
 * @file node_stop_event.hpp
 * @brief Event for stopping a running node
 * 
 * This file contains the NodeStopEvent class for gracefully stopping nodes.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef SIMULATOR_NODE_STOP_EVENT_HPP
#define SIMULATOR_NODE_STOP_EVENT_HPP

#include "simulator/event.hpp"
#include <cstdint>

namespace simulator {

/**
 * @brief Event that stops a running node
 * 
 * This event gracefully stops a node, allowing it to disconnect
 * properly from the mesh network.
 * 
 * Example usage:
 * @code
 * EventScheduler scheduler;
 * 
 * // Stop node 1001 at t=30 seconds
 * auto event = std::make_unique<NodeStopEvent>(1001, true);
 * scheduler.scheduleEvent(std::move(event), 30);
 * @endcode
 */
class NodeStopEvent : public Event {
public:
  /**
   * @brief Construct a node stop event
   * 
   * @param nodeId ID of node to stop
   * @param graceful Whether to stop gracefully (always true for NodeStopEvent)
   */
  explicit NodeStopEvent(uint32_t nodeId, bool graceful = true);
  
  /**
   * @brief Execute the node stop
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
  
  /**
   * @brief Check if stop is graceful
   * 
   * @return true if graceful stop, false otherwise
   */
  bool isGraceful() const { return graceful_; }

private:
  uint32_t nodeId_;   ///< ID of node to stop
  bool graceful_;     ///< Whether to stop gracefully
};

} // namespace simulator

#endif // SIMULATOR_NODE_STOP_EVENT_HPP
