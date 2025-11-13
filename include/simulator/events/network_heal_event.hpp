/**
 * @file network_heal_event.hpp
 * @brief Event for healing network partitions
 * 
 * This file contains the NetworkHealEvent class for restoring
 * network connectivity after a partition, rejoining all isolated groups.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef SIMULATOR_NETWORK_HEAL_EVENT_HPP
#define SIMULATOR_NETWORK_HEAL_EVENT_HPP

#include "simulator/event.hpp"
#include <string>

namespace simulator {

/**
 * @brief Event that heals network partitions
 * 
 * This event restores all connections that were dropped during network
 * partition events, effectively rejoining all isolated groups into a
 * single unified mesh network.
 * 
 * This is used in conjunction with NetworkPartitionEvent to test:
 * - Network recovery scenarios
 * - Mesh reformation after partition
 * - Bridge re-election and coordination
 * 
 * Example usage:
 * @code
 * EventScheduler scheduler;
 * 
 * // Create partition at t=30s
 * std::vector<std::vector<uint32_t>> groups = {
 *   {1001, 1002, 1003},
 *   {1004, 1005, 1006}
 * };
 * auto partition = std::make_unique<NetworkPartitionEvent>(groups);
 * scheduler.scheduleEvent(std::move(partition), 30);
 * 
 * // Heal partition at t=90s
 * auto heal = std::make_unique<NetworkHealEvent>();
 * scheduler.scheduleEvent(std::move(heal), 90);
 * @endcode
 */
class NetworkHealEvent : public Event {
public:
  /**
   * @brief Construct a network heal event
   */
  NetworkHealEvent() = default;
  
  /**
   * @brief Execute the network heal
   * 
   * Restores all previously dropped connections and resets partition
   * IDs to 0 (single partition).
   * 
   * @param manager Node manager for accessing nodes
   * @param network Network simulator for connection management
   */
  void execute(NodeManager& manager, NetworkSimulator& network) override;
  
  /**
   * @brief Get event description
   * 
   * @return Description string
   */
  std::string getDescription() const override;
};

} // namespace simulator

#endif // SIMULATOR_NETWORK_HEAL_EVENT_HPP
