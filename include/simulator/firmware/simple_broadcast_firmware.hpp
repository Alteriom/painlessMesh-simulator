/**
 * @file simple_broadcast_firmware.hpp
 * @brief Simple broadcast firmware for testing
 * 
 * This firmware broadcasts messages periodically to test mesh functionality.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef SIMULATOR_SIMPLE_BROADCAST_FIRMWARE_HPP
#define SIMULATOR_SIMPLE_BROADCAST_FIRMWARE_HPP

#include "firmware_base.hpp"
#include <TaskSchedulerDeclarations.h>
#include "painlessmesh/mesh.hpp"
#include "Arduino.h"
#include <iostream>

namespace simulator {
namespace firmware {

/**
 * @brief Simple firmware that broadcasts messages periodically
 * 
 * SimpleBroadcastFirmware sends a broadcast message every broadcast_interval
 * milliseconds. This is useful for testing mesh connectivity and message
 * routing.
 * 
 * Configuration options:
 * - broadcast_interval: Interval between broadcasts in ms (default: 5000)
 * - broadcast_message: Message to broadcast (default: "Hello from node {id}")
 */
class SimpleBroadcastFirmware : public FirmwareBase {
public:
  /**
   * @brief Constructor
   */
  SimpleBroadcastFirmware() 
    : FirmwareBase("SimpleBroadcast"),
      broadcast_task_(TASK_SECOND * 5, TASK_FOREVER, 
                     std::bind(&SimpleBroadcastFirmware::broadcastMessage, this)) {
  }
  
  /**
   * @brief Setup firmware
   */
  void setup() override {
    // Get configuration
    String interval_str = getConfig("broadcast_interval", "5000");
    broadcast_interval_ = static_cast<uint32_t>(std::stoul(interval_str));
    
    broadcast_message_ = getConfig("broadcast_message", "Hello from node");
    
    // Configure broadcast task
    broadcast_task_.setInterval(broadcast_interval_);
    
    // Registered through the base so the task stops while the node is down
    if (registerTask(broadcast_task_)) {
      
      std::cout << "[INFO] Node " << node_id_ 
                << " SimpleBroadcast firmware initialized (interval: " 
                << broadcast_interval_ << "ms)" << std::endl;
    }
  }
  
  /**
   * @brief Main loop
   */
  void loop() override {
    // Nothing to do here - task scheduler handles broadcasts
  }
  
  /**
   * @brief Handle received messages
   */
  void onReceive(uint32_t from, String& msg) override {
    messages_received_++;
    std::cout << "[INFO] Node " << node_id_ << " received message from " 
              << from << ": " << msg << std::endl;
  }
  
  /**
   * @brief Handle new connections
   */
  void onNewConnection(uint32_t nodeId) override {
    std::cout << "[INFO] Node " << node_id_ << " connected to " 
              << nodeId << std::endl;
  }
  
  /**
   * @brief Handle connection topology changes
   */
  void onChangedConnections() override {
    std::cout << "[INFO] Node " << node_id_ 
              << " topology changed" << std::endl;
  }
  
  /**
   * @brief Gets number of messages sent
   */
  uint32_t getMessagesSent() const { return messages_sent_; }
  
  /**
   * @brief Gets number of messages received
   */
  uint32_t getMessagesReceived() const { return messages_received_; }

  /**
   * @brief Gets number of broadcasts the mesh refused to carry
   *
   * Non-zero means this node was isolated for part of the run.
   */
  uint32_t getBroadcastsRejected() const { return broadcasts_rejected_; }

private:
  /**
   * @brief Broadcasts a message to all nodes
   */
  void broadcastMessage() {
    if (!mesh_) {
      return;
    }
    
    // Create message
    String msg = broadcast_message_ + " " + std::to_string(node_id_);
    
    // Broadcast to all nodes. A false return means the mesh reached nobody --
    // an isolated node, or one cut off by a partition. Counting it anyway
    // would report undelivered traffic as sent.
    if (!sendBroadcast(msg)) {
      if (broadcasts_rejected_++ == 0) {
        std::cout << "[WARN] Node " << node_id_
                  << " broadcast reached no peers (further rejections are "
                  << "counted, not logged)" << std::endl;
      }
      return;
    }
    messages_sent_++;
    
    std::cout << "[INFO] Node " << node_id_ << " broadcasting: " 
              << msg << std::endl;
  }
  
  Task broadcast_task_;                    ///< Task for periodic broadcasts
  uint32_t broadcast_interval_{5000};      ///< Broadcast interval in ms
  String broadcast_message_;               ///< Message to broadcast
  uint32_t messages_sent_{0};              ///< Number of messages the mesh accepted
  uint32_t broadcasts_rejected_{0};        ///< Broadcasts the mesh reached nobody with
  uint32_t messages_received_{0};          ///< Number of messages received
};

} // namespace firmware
} // namespace simulator

#endif // SIMULATOR_SIMPLE_BROADCAST_FIRMWARE_HPP
