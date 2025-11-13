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
    node->stop();
  }
}

std::string NodeCrashEvent::getDescription() const {
  return "Node crash: node " + std::to_string(nodeId_);
}

} // namespace simulator
