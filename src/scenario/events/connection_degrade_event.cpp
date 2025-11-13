/**
 * @file connection_degrade_event.cpp
 * @brief Implementation of connection degrade event
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include "simulator/events/connection_degrade_event.hpp"
#include "simulator/node_manager.hpp"
#include "simulator/network_simulator.hpp"
#include <iostream>

namespace simulator {

ConnectionDegradeEvent::ConnectionDegradeEvent(uint32_t fromNode, uint32_t toNode)
  : fromNode_(fromNode), toNode_(toNode), latencyMs_(500), packetLoss_(0.30f) {
}

ConnectionDegradeEvent::ConnectionDegradeEvent(uint32_t fromNode, uint32_t toNode,
                                               uint32_t latencyMs, float packetLoss)
  : fromNode_(fromNode), toNode_(toNode), latencyMs_(latencyMs), packetLoss_(packetLoss) {
}

void ConnectionDegradeEvent::execute(NodeManager& manager, NetworkSimulator& network) {
  // Set increased latency
  LatencyConfig latency;
  latency.min_ms = latencyMs_;
  latency.max_ms = latencyMs_ * 2;
  latency.distribution = DistributionType::UNIFORM;
  
  network.setLatency(fromNode_, toNode_, latency);
  network.setLatency(toNode_, fromNode_, latency);
  
  // Set packet loss
  PacketLossConfig loss;
  loss.probability = packetLoss_;
  loss.burst_mode = false;
  
  network.setPacketLoss(fromNode_, toNode_, loss);
  network.setPacketLoss(toNode_, fromNode_, loss);
  
  std::cout << "[EVENT] Connection degraded: " << fromNode_ 
            << " <-> " << toNode_ 
            << " (latency: " << latencyMs_ << "ms, loss: " 
            << (packetLoss_ * 100.0f) << "%)" << std::endl;
}

std::string ConnectionDegradeEvent::getDescription() const {
  return "Degrade connection: " + std::to_string(fromNode_) + 
         " <-> " + std::to_string(toNode_) + 
         " (latency: " + std::to_string(latencyMs_) + "ms, loss: " + 
         std::to_string(packetLoss_ * 100.0f) + "%)";
}

} // namespace simulator
