/**
 * @file network_partition_event.cpp
 * @brief Implementation of network partition event
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include "simulator/events/network_partition_event.hpp"
#include "simulator/node_manager.hpp"
#include "simulator/network_simulator.hpp"
#include <iostream>
#include <stdexcept>

namespace simulator {

NetworkPartitionEvent::NetworkPartitionEvent(
    const std::vector<std::vector<uint32_t>>& partitionGroups)
  : partition_groups_(partitionGroups) {
  
  // Validate input
  if (partition_groups_.size() < 2) {
    throw std::invalid_argument(
      "NetworkPartitionEvent requires at least 2 partition groups");
  }
  
  for (size_t i = 0; i < partition_groups_.size(); ++i) {
    if (partition_groups_[i].empty()) {
      throw std::invalid_argument(
        "NetworkPartitionEvent: partition group " + std::to_string(i) + " is empty");
    }
  }
}

void NetworkPartitionEvent::execute(NodeManager& manager, NetworkSimulator& network) {
  // Drop all connections between different groups
  for (size_t i = 0; i < partition_groups_.size(); ++i) {
    for (size_t j = i + 1; j < partition_groups_.size(); ++j) {
      dropConnectionsBetweenGroups(network, partition_groups_[i], partition_groups_[j]);
    }
  }
  
  std::cout << "[EVENT] Network partitioned into " << partition_groups_.size() 
            << " groups" << std::endl;
  
  // Mark partition state for metrics
  for (size_t i = 0; i < partition_groups_.size(); ++i) {
    for (const auto& nodeId : partition_groups_[i]) {
      auto node = manager.getNode(nodeId);
      if (node) {
        node->setPartitionId(static_cast<uint32_t>(i + 1));  // 1-based partition IDs
      }
    }
  }
}

std::string NetworkPartitionEvent::getDescription() const {
  return "Partition network into " + std::to_string(partition_groups_.size()) + " groups";
}

void NetworkPartitionEvent::dropConnectionsBetweenGroups(
    NetworkSimulator& network,
    const std::vector<uint32_t>& group1,
    const std::vector<uint32_t>& group2) {
  
  for (const auto& node1 : group1) {
    for (const auto& node2 : group2) {
      network.dropConnection(node1, node2);
      network.dropConnection(node2, node1);
    }
  }
}

} // namespace simulator
