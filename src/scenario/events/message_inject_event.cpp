/**
 * @file message_inject_event.cpp
 * @brief Implementation of MessageInjectEvent
 *
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include "simulator/events/message_inject_event.hpp"
#include "simulator/node_manager.hpp"
#include "simulator/network_simulator.hpp"

#include <iostream>
#include <stdexcept>

namespace simulator {

MessageInjectEvent::MessageInjectEvent(uint32_t fromNode, uint32_t toNode,
                                       const std::string& payload)
  : fromNode_(fromNode), toNode_(toNode), payload_(payload) {
  if (fromNode_ == 0) {
    throw std::invalid_argument("MessageInjectEvent requires a sending node");
  }
}

void MessageInjectEvent::execute(NodeManager& manager, NetworkSimulator& network) {
  auto node = manager.getNode(fromNode_);

  if (!node) {
    throw std::runtime_error("Cannot inject message from node " +
                             std::to_string(fromNode_) + ": node not found");
  }

  const bool sent = node->injectMessage(toNode_, payload_);

  std::cout << "[EVENT] Message injected from " << fromNode_ << " to "
            << (toNode_ == 0 ? std::string("<broadcast>")
                             : std::to_string(toNode_))
            << (sent ? "" : " -- REFUSED (node down or mesh rejected it)")
            << std::endl;

  // A refused injection means the probe never left the node -- the sender is
  // down, or it has no mesh to send into. That is not a delivery outcome to
  // note and move on from; the requested traffic was never exercised, so fail
  // the run rather than let a lifecycle or partition experiment pass without it.
  if (!sent) {
    throw std::runtime_error(
        "inject_message from " + std::to_string(fromNode_) + " to " +
        (toNode_ == 0 ? std::string("<broadcast>") : std::to_string(toNode_)) +
        " was refused; the probe was never injected");
  }
}

std::string MessageInjectEvent::getDescription() const {
  return "Inject message: " + std::to_string(fromNode_) + " -> " +
         (toNode_ == 0 ? std::string("broadcast") : std::to_string(toNode_));
}

} // namespace simulator
