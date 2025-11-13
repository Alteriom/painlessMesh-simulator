/**
 * @file echo_server_firmware.hpp
 * @brief Echo server firmware for testing
 * 
 * This firmware responds to all received messages by echoing them back
 * to the sender.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef SIMULATOR_ECHO_SERVER_FIRMWARE_HPP
#define SIMULATOR_ECHO_SERVER_FIRMWARE_HPP

#include "firmware_base.hpp"
#include <TaskSchedulerDeclarations.h>
#include "painlessmesh/mesh.hpp"
#include "Arduino.h"
#include <iostream>

namespace simulator {
namespace firmware {

/**
 * @brief Firmware that echoes received messages back to the sender
 * 
 * EchoServerFirmware responds to any received message by sending back
 * an echo response prefixed with "ECHO: ". This is useful for testing
 * request/response patterns in mesh networks.
 * 
 * Configuration options: None required
 */
class EchoServerFirmware : public FirmwareBase {
public:
  /**
   * @brief Constructor
   */
  EchoServerFirmware() : FirmwareBase("EchoServer") {}
  
  /**
   * @brief Setup firmware
   */
  void setup() override {
    std::cout << "[INFO] Node " << node_id_ 
              << " EchoServer firmware started" << std::endl;
  }
  
  /**
   * @brief Main loop
   */
  void loop() override {
    // Server waits for requests - no periodic tasks needed
  }
  
  /**
   * @brief Handle received messages - echo them back
   */
  void onReceive(uint32_t from, String& msg) override {
    // Create echo response
    String response = "ECHO: " + msg;
    
    // Send response back to sender
    if (mesh_) {
      mesh_->sendSingle(from, response);
      echo_count_++;
      
      std::cout << "[INFO] Node " << node_id_ << " echoed to " << from 
                << ": " << response << std::endl;
    }
  }
  
  /**
   * @brief Handle new connections
   */
  void onNewConnection(uint32_t nodeId) override {
    std::cout << "[INFO] Node " << node_id_ << " EchoServer: New client " 
              << nodeId << std::endl;
    connection_count_++;
  }
  
  /**
   * @brief Gets number of messages echoed
   */
  uint32_t getEchoCount() const { return echo_count_; }
  
  /**
   * @brief Gets number of connections
   */
  uint32_t getConnectionCount() const { return connection_count_; }

private:
  uint32_t echo_count_{0};        ///< Number of messages echoed
  uint32_t connection_count_{0};  ///< Number of connections received
};

} // namespace firmware
} // namespace simulator

#endif // SIMULATOR_ECHO_SERVER_FIRMWARE_HPP
