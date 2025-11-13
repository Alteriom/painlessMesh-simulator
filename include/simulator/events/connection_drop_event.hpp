/**
 * @file connection_drop_event.hpp
 * @brief Event for dropping connections between nodes
 * 
 * This file contains the ConnectionDropEvent class for simulating
 * connection failures between specific node pairs.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef SIMULATOR_CONNECTION_DROP_EVENT_HPP
#define SIMULATOR_CONNECTION_DROP_EVENT_HPP

#include "simulator/event.hpp"
#include <cstdint>
#include <string>

namespace simulator {

/**
 * @brief Event that drops a connection between two nodes
 * 
 * This event simulates a connection failure between two specific nodes,
 * preventing message delivery in both directions. Useful for testing
 * mesh routing, bridge failover, and network partition scenarios.
 * 
 * Example usage:
 * @code
 * EventScheduler scheduler;
 * 
 * // Drop connection between nodes 1001 and 1002 at t=30 seconds
 * auto event = std::make_unique<ConnectionDropEvent>(1001, 1002);
 * scheduler.scheduleEvent(std::move(event), 30);
 * @endcode
 */
class ConnectionDropEvent : public Event {
public:
  /**
   * @brief Construct a connection drop event
   * 
   * @param fromNode ID of first node
   * @param toNode ID of second node
   */
  ConnectionDropEvent(uint32_t fromNode, uint32_t toNode);
  
  /**
   * @brief Execute the connection drop
   * 
   * Drops the connection between the specified nodes in both directions.
   * 
   * @param manager Node manager (not used)
   * @param network Network simulator for connection management
   */
  void execute(NodeManager& manager, NetworkSimulator& network) override;
  
  /**
   * @brief Get event description
   * 
   * @return Description string
   */
  std::string getDescription() const override;
  
  /**
   * @brief Get the first node ID
   * 
   * @return First node ID
   */
  uint32_t getFromNode() const { return fromNode_; }
  
  /**
   * @brief Get the second node ID
   * 
   * @return Second node ID
   */
  uint32_t getToNode() const { return toNode_; }

private:
  uint32_t fromNode_;  ///< First node ID
  uint32_t toNode_;    ///< Second node ID
};

} // namespace simulator

#endif // SIMULATOR_CONNECTION_DROP_EVENT_HPP
