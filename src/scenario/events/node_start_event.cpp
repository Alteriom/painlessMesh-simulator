/**
 * @file node_start_event.cpp
 * @brief Implementation of node start event
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include "simulator/events/node_start_event.hpp"
#include "simulator/node_manager.hpp"
#include "simulator/network_simulator.hpp"
#include <stdexcept>
#include <iostream>

namespace simulator {

NodeStartEvent::NodeStartEvent(uint32_t nodeId) 
  : nodeId_(nodeId) {
}

void NodeStartEvent::execute(NodeManager& manager, NetworkSimulator& network) {
  auto node = manager.getNode(nodeId_);
  
  if (!node) {
    throw std::runtime_error("Cannot start node " + std::to_string(nodeId_) + 
                            ": node not found");
  }
  
  if (!node->isRunning()) {
    node->start();
    // start() brings the node's mesh object back up but wires nothing:
    // stop()/crash() closed its connections, and establishConnectivity() ran
    // once before the timeline. Without this the node reports isRunning() while
    // sending into a mesh it is no longer attached to.
    const auto rejoin = manager.reconnectNode(nodeId_);
    if (rejoin.failed > 0) {
      throw std::runtime_error(
          "start_node: node " + std::to_string(nodeId_) + " failed to "
          "re-establish " + std::to_string(rejoin.failed) + " mesh link(s)");
    }
    std::cout << "[EVENT] Node " << nodeId_ << " started ("
              << rejoin.reconnected << " mesh link(s) re-established)"
              << std::endl;
  } else {
    std::cout << "[EVENT] Node " << nodeId_ << " is already running" << std::endl;
  }
}

std::string NodeStartEvent::getDescription() const {
  return "Start node: " + std::to_string(nodeId_);
}

} // namespace simulator
