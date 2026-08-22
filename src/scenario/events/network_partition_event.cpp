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

  // The loop above only touches the NetworkSimulator, which no delivery path
  // consults -- before this, a "partitioned" mesh carried exactly as much
  // traffic as an intact one. Cut the real links as well.
  const size_t cut = manager.partitionNetwork(partition_groups_);

  std::cout << "[EVENT] Network partitioned into " << partition_groups_.size()
            << " groups (" << cut << " mesh link(s) cut)" << std::endl;

  // A partition must produce exactly the requested groups. planTopology() keeps
  // each group internally connected where the topology allows it (mesh/random),
  // so a remaining mismatch means the declared topology genuinely cannot realise
  // this split -- a star whose group excludes the hub, say. That is a scenario
  // that would silently test something other than what it asks, so fail rather
  // than warn.
  // Only meaningful once a mesh was wired. Guard on that, not on whether this
  // partition happened to cut a cross edge: a disconnected topology (only A--B,
  // with C and D isolated) partitioned [[A,B],[C,D]] cuts nothing yet still has
  // three components, not two. An isolation test that never wired a mesh has no
  // recorded topology and is correctly skipped.
  const auto components = manager.getConnectedComponents();
  if (manager.hasWiredTopology() &&
      components.size() != partition_groups_.size()) {
    throw std::runtime_error(
        "network_partition requested " + std::to_string(partition_groups_.size()) +
        " groups but the topology fragmented into " +
        std::to_string(components.size()) +
        " components; a group is not internally connected in this topology");
  }
  
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
      // Partition cuts, not explicit drops: a later heal clears these while
      // leaving any coexisting connection_drop in force (mirrors NodeManager).
      network.partitionConnection(node1, node2);
      network.partitionConnection(node2, node1);
    }
  }
}

} // namespace simulator
