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
#include <stdexcept>

namespace simulator {

ConnectionRestoreEvent::ConnectionRestoreEvent(uint32_t fromNode, uint32_t toNode)
  : fromNode_(fromNode), toNode_(toNode) {
}

void ConnectionRestoreEvent::execute(NodeManager& manager, NetworkSimulator& network) {
  // Restore connection in both directions
  network.restoreConnection(fromNode_, toNode_);
  network.restoreConnection(toNode_, fromNode_);

  // Re-establish the actual mesh link the matching drop severed. A genuine
  // failure -- both endpoints up but the handshake did not settle -- must fail
  // the run, not be logged as a benign no-op; the several legitimate no-ops
  // (already live, deferred to a heal, a node down) are reported, not failed.
  const auto outcome = manager.restoreLink(fromNode_, toNode_);
  if (outcome == NodeManager::RestoreOutcome::Failed) {
    throw std::runtime_error(
        "connection_restore: link " + std::to_string(fromNode_) + " <-> " +
        std::to_string(toNode_) + " did not re-establish");
  }
  const bool relinked = outcome == NodeManager::RestoreOutcome::Reestablished;

  std::cout << "[EVENT] Connection restored: " << fromNode_
            << " <-> " << toNode_
            << (relinked ? " (mesh link re-established)"
                         : " (no mesh link to re-establish)") << std::endl;
}

std::string ConnectionRestoreEvent::getDescription() const {
  return "Restore connection: " + std::to_string(fromNode_) + 
         " <-> " + std::to_string(toNode_);
}

} // namespace simulator
