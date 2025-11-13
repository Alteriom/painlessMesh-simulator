/**
 * @file echo_client_firmware.hpp
 * @brief Echo client firmware for testing
 * 
 * This firmware periodically sends requests and expects echo responses.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef SIMULATOR_ECHO_CLIENT_FIRMWARE_HPP
#define SIMULATOR_ECHO_CLIENT_FIRMWARE_HPP

#include "firmware_base.hpp"
#include <TaskSchedulerDeclarations.h>
#include "painlessmesh/mesh.hpp"
#include "Arduino.h"
#include <iostream>

namespace simulator {
namespace firmware {

/**
 * @brief Firmware that sends periodic requests and processes echo responses
 * 
 * EchoClientFirmware sends periodic requests to either a specific server node
 * or broadcasts them to all nodes. It tracks responses to validate
 * request/response patterns.
 * 
 * Configuration options:
 * - server_node_id: Target server node ID (0 = broadcast mode)
 * - request_interval: Interval between requests in seconds (default: 5)
 */
class EchoClientFirmware : public FirmwareBase {
public:
  /**
   * @brief Constructor
   */
  EchoClientFirmware() 
    : FirmwareBase("EchoClient"),
      request_task_(TASK_SECOND * 5, TASK_FOREVER,
                   std::bind(&EchoClientFirmware::sendRequest, this)) {
  }
  
  /**
   * @brief Setup firmware
   */
  void setup() override {
    // Get server node ID from config (0 = broadcast mode)
    String server_id_str = getConfig("server_node_id", "0");
    server_node_id_ = static_cast<uint32_t>(std::stoul(server_id_str));
    
    // Get request interval from config
    String interval_str = getConfig("request_interval", "5");
    uint32_t interval_seconds = static_cast<uint32_t>(std::stoul(interval_str));
    request_interval_ = interval_seconds * TASK_SECOND;
    
    // Configure request task
    request_task_.setInterval(request_interval_);
    
    // Add task to scheduler
    if (scheduler_) {
      scheduler_->addTask(request_task_);
      request_task_.enable();
      
      std::cout << "[INFO] Node " << node_id_ 
                << " EchoClient firmware started, "
                << "server=" << server_node_id_ 
                << ", interval=" << interval_seconds << "s" 
                << std::endl;
    }
  }
  
  /**
   * @brief Main loop
   */
  void loop() override {
    // Loop processes responses - no additional work needed
  }
  
  /**
   * @brief Handle received messages - process echo responses
   */
  void onReceive(uint32_t from, String& msg) override {
    if (msg.find("ECHO: ") == 0) {
      // This is an echo response
      String original = msg.substr(6);  // Skip "ECHO: " prefix
      responses_received_++;
      
      std::cout << "[INFO] Node " << node_id_ << " received response from " 
                << from << ": " << msg << std::endl;
    }
  }
  
  /**
   * @brief Gets number of requests sent
   */
  uint32_t getRequestsSent() const { return requests_sent_; }
  
  /**
   * @brief Gets number of responses received
   */
  uint32_t getResponsesReceived() const { return responses_received_; }

private:
  /**
   * @brief Sends a request message
   */
  void sendRequest() {
    if (!mesh_) {
      return;
    }
    
    // Create request message
    String msg = "Request #" + std::to_string(requests_sent_);
    
    if (server_node_id_ == 0) {
      // Broadcast mode - send to all nodes
      mesh_->sendBroadcast(msg);
      std::cout << "[INFO] Node " << node_id_ << " broadcasting request: " 
                << msg << std::endl;
    } else {
      // Send to specific server
      mesh_->sendSingle(server_node_id_, msg);
      std::cout << "[INFO] Node " << node_id_ << " sending request to " 
                << server_node_id_ << ": " << msg << std::endl;
    }
    
    requests_sent_++;
  }
  
  Task request_task_;                     ///< Task for periodic requests
  uint32_t server_node_id_{0};            ///< Target server (0 = broadcast)
  uint32_t request_interval_{TASK_SECOND * 5};  ///< Request interval
  uint32_t requests_sent_{0};             ///< Number of requests sent
  uint32_t responses_received_{0};        ///< Number of responses received
};

} // namespace firmware
} // namespace simulator

#endif // SIMULATOR_ECHO_CLIENT_FIRMWARE_HPP
