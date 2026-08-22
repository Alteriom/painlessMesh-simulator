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
#include <stdexcept>

namespace simulator {

void NetworkHealEvent::execute(NodeManager& manager, NetworkSimulator& network) {
  // Restore all previously dropped connections
  network.restoreAllConnections();

  // Rebuild the mesh links the partition severed.
  const auto heal = manager.healNetwork();

  // Clear partition IDs for all nodes
  for (auto& node : manager.getAllNodes()) {
    node->setPartitionId(0);  // Single partition
  }

  std::cout << "[EVENT] Network partitions healed (" << heal.restored
            << " mesh link(s) restored)" << std::endl;

  // A cut that both endpoints were up for but the handshake did not settle is a
  // genuine failure -- the partition the scenario asked to heal is still in
  // place. Surface it rather than logging a clean heal. (A cut pending only
  // because a node is down is not counted here; reconnectNode() restores it
  // when the node returns.)
  if (heal.failed > 0) {
    throw std::runtime_error(
        "heal_partition left " + std::to_string(heal.failed) +
        " cut(s) unrestored; the partition did not fully heal");
  }
}

std::string NetworkHealEvent::getDescription() const {
  return "Heal network partitions";
}

} // namespace simulator
