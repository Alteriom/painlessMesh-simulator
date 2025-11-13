/**
 * @file connection_restore_event.hpp
 * @brief Event for restoring dropped connections between nodes
 * 
 * This file contains the ConnectionRestoreEvent class for restoring
 * previously dropped connections between node pairs.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef SIMULATOR_CONNECTION_RESTORE_EVENT_HPP
#define SIMULATOR_CONNECTION_RESTORE_EVENT_HPP

#include "simulator/event.hpp"
#include <cstdint>
#include <string>

namespace simulator {

/**
 * @brief Event that restores a connection between two nodes
 * 
 * This event restores a previously dropped connection between two nodes,
 * re-enabling message delivery in both directions. Used in conjunction
 * with ConnectionDropEvent to test network healing and recovery scenarios.
 * 
 * Example usage:
 * @code
 * EventScheduler scheduler;
 * 
 * // Drop connection at t=30s
 * auto drop = std::make_unique<ConnectionDropEvent>(1001, 1002);
 * scheduler.scheduleEvent(std::move(drop), 30);
 * 
 * // Restore connection at t=60s
 * auto restore = std::make_unique<ConnectionRestoreEvent>(1001, 1002);
 * scheduler.scheduleEvent(std::move(restore), 60);
 * @endcode
 */
class ConnectionRestoreEvent : public Event {
public:
  /**
   * @brief Construct a connection restore event
   * 
   * @param fromNode ID of first node
   * @param toNode ID of second node
   */
  ConnectionRestoreEvent(uint32_t fromNode, uint32_t toNode);
  
  /**
   * @brief Execute the connection restore
   * 
   * Restores the connection between the specified nodes in both directions.
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

#endif // SIMULATOR_CONNECTION_RESTORE_EVENT_HPP
