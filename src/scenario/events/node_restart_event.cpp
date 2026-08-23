/**
 * @file node_restart_event.cpp
 * @brief Implementation of node restart event
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include "simulator/events/node_restart_event.hpp"
#include "simulator/node_manager.hpp"
#include "simulator/network_simulator.hpp"
#include <stdexcept>
#include <iostream>

namespace simulator {

NodeRestartEvent::NodeRestartEvent(uint32_t nodeId) 
  : nodeId_(nodeId) {
}

void NodeRestartEvent::execute(NodeManager& manager, NetworkSimulator& network) {
  auto node = manager.getNode(nodeId_);
  
  if (!node) {
    throw std::runtime_error("Cannot restart node " + std::to_string(nodeId_) + 
                            ": node not found");
  }
  
  node->restart();
  // restart() is stop() + start(); the start half leaves the node detached.
  const auto rejoin = manager.reconnectNode(nodeId_);
  if (rejoin.failed > 0) {
    throw std::runtime_error(
        "restart_node: node " + std::to_string(nodeId_) + " failed to "
        "re-establish " + std::to_string(rejoin.failed) + " mesh link(s)");
  }
  std::cout << "[EVENT] Node " << nodeId_ << " restarted ("
            << rejoin.reconnected << " mesh link(s) re-established)" << std::endl;
}

std::string NodeRestartEvent::getDescription() const {
  return "Restart node: " + std::to_string(nodeId_);
}

} // namespace simulator
