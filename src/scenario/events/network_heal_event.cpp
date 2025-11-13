/**
 * @file network_heal_event.cpp
 * @brief Implementation of network heal event
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include "simulator/events/network_heal_event.hpp"
#include "simulator/node_manager.hpp"
#include "simulator/network_simulator.hpp"
#include <iostream>

namespace simulator {

void NetworkHealEvent::execute(NodeManager& manager, NetworkSimulator& network) {
  // Restore all previously dropped connections
  network.restoreAllConnections();
  
  // Clear partition IDs for all nodes
  for (auto& node : manager.getAllNodes()) {
    node->setPartitionId(0);  // Single partition
  }
  
  std::cout << "[EVENT] Network partitions healed" << std::endl;
}

std::string NetworkHealEvent::getDescription() const {
  return "Heal network partitions";
}

} // namespace simulator
