/**
 * @file connection_restore_event.cpp
 * @brief Implementation of connection restore event
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include "simulator/events/connection_restore_event.hpp"
#include "simulator/node_manager.hpp"
#include "simulator/network_simulator.hpp"
#include <iostream>

namespace simulator {

ConnectionRestoreEvent::ConnectionRestoreEvent(uint32_t fromNode, uint32_t toNode)
  : fromNode_(fromNode), toNode_(toNode) {
}

void ConnectionRestoreEvent::execute(NodeManager& manager, NetworkSimulator& network) {
  // Restore connection in both directions
  network.restoreConnection(fromNode_, toNode_);
  network.restoreConnection(toNode_, fromNode_);
  
  std::cout << "[EVENT] Connection restored: " << fromNode_ 
            << " <-> " << toNode_ << std::endl;
}

std::string ConnectionRestoreEvent::getDescription() const {
  return "Restore connection: " + std::to_string(fromNode_) + 
         " <-> " + std::to_string(toNode_);
}

} // namespace simulator
