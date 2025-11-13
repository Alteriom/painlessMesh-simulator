/**
 * @file connection_drop_event.cpp
 * @brief Implementation of connection drop event
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include "simulator/events/connection_drop_event.hpp"
#include "simulator/node_manager.hpp"
#include "simulator/network_simulator.hpp"
#include <iostream>

namespace simulator {

ConnectionDropEvent::ConnectionDropEvent(uint32_t fromNode, uint32_t toNode)
  : fromNode_(fromNode), toNode_(toNode) {
}

void ConnectionDropEvent::execute(NodeManager& manager, NetworkSimulator& network) {
  // Drop connection in both directions
  network.dropConnection(fromNode_, toNode_);
  network.dropConnection(toNode_, fromNode_);
  
  std::cout << "[EVENT] Connection dropped: " << fromNode_ 
            << " <-> " << toNode_ << std::endl;
}

std::string ConnectionDropEvent::getDescription() const {
  return "Drop connection: " + std::to_string(fromNode_) + 
         " <-> " + std::to_string(toNode_);
}

} // namespace simulator
