/**
 * @file network_partition_event.hpp
 * @brief Event for creating network partitions (split-brain scenarios)
 * 
 * This file contains the NetworkPartitionEvent class for simulating
 * network partitions where the mesh is split into multiple isolated groups.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef SIMULATOR_NETWORK_PARTITION_EVENT_HPP
#define SIMULATOR_NETWORK_PARTITION_EVENT_HPP

#include "simulator/event.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace simulator {

/**
 * @brief Event that partitions the network into isolated groups
 * 
 * This event simulates a network partition (split-brain scenario) by
 * dropping connections between different groups of nodes. Each group
 * becomes an isolated partition that can operate independently.
 * 
 * This is critical for testing:
 * - Bridge coordination during network splits
 * - Mesh behavior in partition scenarios
 * - Network healing and recovery
 * 
 * Example usage:
 * @code
 * EventScheduler scheduler;
 * 
 * // Create 3 partitions at t=30 seconds
 * std::vector<std::vector<uint32_t>> groups = {
 *   {1001, 1002, 1003},  // Partition 1
 *   {1004, 1005, 1006},  // Partition 2
 *   {1007, 1008, 1009}   // Partition 3
 * };
 * auto event = std::make_unique<NetworkPartitionEvent>(groups);
 * scheduler.scheduleEvent(std::move(event), 30);
 * @endcode
 */
class NetworkPartitionEvent : public Event {
public:
  /**
   * @brief Construct a network partition event
   * 
   * @param partitionGroups Vector of node ID groups, one per partition
   * 
   * @throws std::invalid_argument if less than 2 groups provided
   * @throws std::invalid_argument if any group is empty
   */
  explicit NetworkPartitionEvent(const std::vector<std::vector<uint32_t>>& partitionGroups);
  
  /**
   * @brief Execute the network partition
   * 
   * Drops connections between different partition groups and assigns
   * partition IDs to nodes for tracking.
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
  
  /**
   * @brief Get the partition groups
   * 
   * @return Vector of node ID groups
   */
  const std::vector<std::vector<uint32_t>>& getPartitionGroups() const { 
    return partition_groups_; 
  }
  
  /**
   * @brief Get the number of partitions
   * 
   * @return Number of partition groups
   */
  size_t getPartitionCount() const { return partition_groups_.size(); }

private:
  std::vector<std::vector<uint32_t>> partition_groups_;  ///< Node groups per partition
  
  /**
   * @brief Drops connections between two groups
   * 
   * @param network Network simulator
   * @param group1 First group of node IDs
   * @param group2 Second group of node IDs
   */
  void dropConnectionsBetweenGroups(NetworkSimulator& network,
                                     const std::vector<uint32_t>& group1,
                                     const std::vector<uint32_t>& group2);
};

} // namespace simulator

#endif // SIMULATOR_NETWORK_PARTITION_EVENT_HPP
