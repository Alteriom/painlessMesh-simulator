/**
 * @file node_start_event.hpp
 * @brief Event for starting a stopped node
 * 
 * This file contains the NodeStartEvent class for starting nodes
 * that have been stopped or crashed.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef SIMULATOR_NODE_START_EVENT_HPP
#define SIMULATOR_NODE_START_EVENT_HPP

#include "simulator/event.hpp"
#include <cstdint>

namespace simulator {

/**
 * @brief Event that starts a stopped node
 * 
 * This event starts a node that has been stopped, allowing it to
 * rejoin the mesh network.
 * 
 * Example usage:
 * @code
 * EventScheduler scheduler;
 * 
 * // Start node 1001 at t=30 seconds
 * auto event = std::make_unique<NodeStartEvent>(1001);
 * scheduler.scheduleEvent(std::move(event), 30);
 * @endcode
 */
class NodeStartEvent : public Event {
public:
  /**
   * @brief Construct a node start event
   * 
   * @param nodeId ID of node to start
   */
  explicit NodeStartEvent(uint32_t nodeId);
  
  /**
   * @brief Execute the node start
   * 
   * Starts the specified node if it exists and is not running.
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
  uint32_t nodeId_;  ///< ID of node to start
};

} // namespace simulator

#endif // SIMULATOR_NODE_START_EVENT_HPP
