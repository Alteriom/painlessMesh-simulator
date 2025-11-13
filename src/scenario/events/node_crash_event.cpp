/**
 * @file node_crash_event.cpp
 * @brief Implementation of node crash event
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include "simulator/events/node_crash_event.hpp"
#include "simulator/node_manager.hpp"
#include "simulator/network_simulator.hpp"
#include <stdexcept>
#include <iostream>

namespace simulator {

NodeCrashEvent::NodeCrashEvent(uint32_t nodeId) 
  : nodeId_(nodeId) {
}

void NodeCrashEvent::execute(NodeManager& manager, NetworkSimulator& network) {
  auto node = manager.getNode(nodeId_);
  
  if (!node) {
    throw std::runtime_error("Cannot crash node " + std::to_string(nodeId_) + 
                            ": node not found");
  }
  
  if (node->isRunning()) {
    node->crash();
    std::cout << "[EVENT] Node " << nodeId_ << " crashed (ungraceful)" << std::endl;
  } else {
    std::cout << "[EVENT] Node " << nodeId_ << " is already stopped" << std::endl;
  }
}

std::string NodeCrashEvent::getDescription() const {
  return "Node crash: node " + std::to_string(nodeId_);
}

} // namespace simulator
