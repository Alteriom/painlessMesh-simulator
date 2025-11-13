/**
 * @file node_stop_event.cpp
 * @brief Implementation of node stop event
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include "simulator/events/node_stop_event.hpp"
#include "simulator/node_manager.hpp"
#include "simulator/network_simulator.hpp"
#include <stdexcept>
#include <iostream>

namespace simulator {

NodeStopEvent::NodeStopEvent(uint32_t nodeId, bool graceful) 
  : nodeId_(nodeId), graceful_(graceful) {
}

void NodeStopEvent::execute(NodeManager& manager, NetworkSimulator& network) {
  auto node = manager.getNode(nodeId_);
  
  if (!node) {
    throw std::runtime_error("Cannot stop node " + std::to_string(nodeId_) + 
                            ": node not found");
  }
  
  if (node->isRunning()) {
    node->stop();
    std::cout << "[EVENT] Node " << nodeId_ << " stopped " 
              << (graceful_ ? "(graceful)" : "(forced)") << std::endl;
  } else {
    std::cout << "[EVENT] Node " << nodeId_ << " is already stopped" << std::endl;
  }
}

std::string NodeStopEvent::getDescription() const {
  std::string desc = "Stop node: " + std::to_string(nodeId_);
  if (graceful_) {
    desc += " (graceful)";
  }
  return desc;
}

} // namespace simulator
