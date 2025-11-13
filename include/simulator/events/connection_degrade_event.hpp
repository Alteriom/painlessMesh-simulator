/**
 * @file connection_degrade_event.hpp
 * @brief Event for degrading connection quality between nodes
 * 
 * This file contains the ConnectionDegradeEvent class for simulating
 * degraded network conditions (increased latency and packet loss).
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef SIMULATOR_CONNECTION_DEGRADE_EVENT_HPP
#define SIMULATOR_CONNECTION_DEGRADE_EVENT_HPP

#include "simulator/event.hpp"
#include <cstdint>
#include <string>

namespace simulator {

/**
 * @brief Event that degrades connection quality between two nodes
 * 
 * This event simulates poor network conditions by increasing latency
 * and packet loss on a specific connection. Useful for testing mesh
 * behavior under stress and route optimization.
 * 
 * Example usage:
 * @code
 * EventScheduler scheduler;
 * 
 * // Degrade connection at t=45s (500ms latency, 30% loss)
 * auto event = std::make_unique<ConnectionDegradeEvent>(1001, 1002, 500, 0.30f);
 * scheduler.scheduleEvent(std::move(event), 45);
 * @endcode
 */
class ConnectionDegradeEvent : public Event {
public:
  /**
   * @brief Construct a connection degrade event with default parameters
   * 
   * @param fromNode ID of first node
   * @param toNode ID of second node
   */
  ConnectionDegradeEvent(uint32_t fromNode, uint32_t toNode);
  
  /**
   * @brief Construct a connection degrade event with custom parameters
   * 
   * @param fromNode ID of first node
   * @param toNode ID of second node
   * @param latencyMs Degraded latency in milliseconds (default: 500ms)
   * @param packetLoss Packet loss probability 0.0-1.0 (default: 0.30 = 30%)
   */
  ConnectionDegradeEvent(uint32_t fromNode, uint32_t toNode, 
                         uint32_t latencyMs, float packetLoss);
  
  /**
   * @brief Execute the connection degradation
   * 
   * Increases latency and packet loss for the connection between the
   * specified nodes in both directions.
   * 
   * @param manager Node manager (not used)
   * @param network Network simulator for connection management
   */
  void execute(NodeManager& manager, NetworkSimulator& network) override;
  
  /**
   * @brief Get event description
   * 
   * @return Description string
   */
  std::string getDescription() const override;
  
  /**
   * @brief Get the first node ID
   * 
   * @return First node ID
   */
  uint32_t getFromNode() const { return fromNode_; }
  
  /**
   * @brief Get the second node ID
   * 
   * @return Second node ID
   */
  uint32_t getToNode() const { return toNode_; }
  
  /**
   * @brief Get the degraded latency value
   * 
   * @return Latency in milliseconds
   */
  uint32_t getLatency() const { return latencyMs_; }
  
  /**
   * @brief Get the packet loss probability
   * 
   * @return Packet loss probability (0.0-1.0)
   */
  float getPacketLoss() const { return packetLoss_; }

private:
  uint32_t fromNode_;      ///< First node ID
  uint32_t toNode_;        ///< Second node ID
  uint32_t latencyMs_;     ///< Degraded latency in milliseconds
  float packetLoss_;       ///< Packet loss probability (0.0-1.0)
};

} // namespace simulator

#endif // SIMULATOR_CONNECTION_DEGRADE_EVENT_HPP
