/**
 * @file library_validation_firmware.cpp
 * @brief Implementation of library validation firmware
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include "simulator/firmware/library_validation_firmware.hpp"
#include "painlessmesh/layout.hpp"
#include <sstream>
#include <iomanip>

namespace simulator {
namespace firmware {

LibraryValidationFirmware::LibraryValidationFirmware()
  : FirmwareBase("LibraryValidation"),
    test_progress_task_(TASK_SECOND * 5, TASK_FOREVER, 
                       std::bind(&LibraryValidationFirmware::runValidationTests, this)),
    message_test_task_(TASK_SECOND * 2, TASK_FOREVER,
                      std::bind(&LibraryValidationFirmware::checkExpectedMessages, this)),
    status_report_task_(TASK_SECOND * 10, TASK_FOREVER, [this]() {
      if (is_coordinator_ && detailed_logging_) {
        std::cout << "[VAL] Node " << node_id_ 
                  << " Phase: " << static_cast<int>(phase_) 
                  << " Connections: " << connected_nodes_.size() << "\n";
      }
    }) {
  
  // Initialize API coverage tracking
  report_.coverage.total_apis = 25;  // From LIBRARY_VALIDATION_PLAN.md
}

void LibraryValidationFirmware::setup() {
  std::cout << "[VAL] Library Validation Firmware starting on node " << node_id_ << "\n";
  
  // Get configuration
  is_coordinator_ = (getConfig("role", "participant") == "coordinator");
  detailed_logging_ = (getConfig("enable_detailed_logging", "false") == "true");
  
  String duration_str = getConfig("test_duration", "30");
  test_duration_ms_ = static_cast<uint32_t>(std::stoul(duration_str)) * 1000;
  
  std::cout << "[VAL] Node " << node_id_ << " role: " 
            << (is_coordinator_ ? "COORDINATOR" : "PARTICIPANT") << "\n";
  
  // Initialize tracking
  initial_node_time_ = getNodeTime();
  phase_start_time_ = millis();
  
  // Schedule validation tasks
  if (scheduler_) {
    if (is_coordinator_) {
      scheduler_->addTask(test_progress_task_);
      test_progress_task_.enable();
      
      scheduler_->addTask(message_test_task_);
      message_test_task_.enable();
    }
    
    scheduler_->addTask(status_report_task_);
    status_report_task_.enable();
  }
  
  // Start with lifecycle tests
  phase_ = ValidationPhase::INITIALIZATION;
  testMeshLifecycle();
}

void LibraryValidationFirmware::loop() {
  // Main loop - most work done in scheduled tasks
}

void LibraryValidationFirmware::onReceive(uint32_t from, String& msg) {
  recordMessage(false, from);
  
  // Track messages from each node
  messages_from_node_[from]++;
  
  if (detailed_logging_) {
    std::cout << "[VAL] Node " << node_id_ << " received from " 
              << from << ": " << msg.substr(0, 50) << "\n";
  }
  
  // Parse test messages
  if (msg.find("PING:") == 0) {
    // Respond to ping
    String response = "PONG:" + std::to_string(node_id_);
    sendSingle(from, response);
    recordMessage(true);
  }
}

void LibraryValidationFirmware::onNewConnection(uint32_t nodeId) {
  connected_nodes_.insert(nodeId);
  
  if (detailed_logging_) {
    std::cout << "[VAL] Node " << node_id_ << " new connection: " 
              << nodeId << " (total: " << connected_nodes_.size() << ")\n";
  }
}

void LibraryValidationFirmware::onChangedConnections() {
  report_.topology_change_count++;
  
  if (detailed_logging_) {
    auto node_list = getNodeList();
    std::cout << "[VAL] Node " << node_id_ << " topology changed. " 
              << "Node count: " << node_list.size() << "\n";
  }
}

void LibraryValidationFirmware::onNodeTimeAdjusted(int32_t offset) {
  time_adjustments_.push_back(offset);
  
  if (detailed_logging_) {
    std::cout << "[VAL] Node " << node_id_ << " time adjusted by " 
              << offset << " us\n";
  }
}

void LibraryValidationFirmware::runValidationTests() {
  if (!is_coordinator_) return;
  
  uint32_t current_time = millis();
  uint32_t phase_duration = current_time - phase_start_time_;
  
  // Progress through test phases
  if (phase_duration >= test_duration_ms_) {
    progressToNextPhase();
  }
}

void LibraryValidationFirmware::progressToNextPhase() {
  // Finalize current phase
  switch (phase_) {
    case ValidationPhase::INITIALIZATION:
      std::cout << "[VAL] Completing INITIALIZATION phase\n";
      phase_ = ValidationPhase::MESH_FORMATION;
      break;
      
    case ValidationPhase::MESH_FORMATION:
      std::cout << "[VAL] Completing MESH_FORMATION phase\n";
      testConnectionManagement();
      phase_ = ValidationPhase::MESSAGE_TESTS;
      break;
      
    case ValidationPhase::MESSAGE_TESTS:
      std::cout << "[VAL] Completing MESSAGE_TESTS phase\n";
      testMessageSending();
      testMessageReception();
      phase_ = ValidationPhase::TIME_SYNC_TESTS;
      break;
      
    case ValidationPhase::TIME_SYNC_TESTS:
      std::cout << "[VAL] Completing TIME_SYNC_TESTS phase\n";
      testTimeSynchronization();
      phase_ = ValidationPhase::TOPOLOGY_TESTS;
      break;
      
    case ValidationPhase::TOPOLOGY_TESTS:
      std::cout << "[VAL] Completing TOPOLOGY_TESTS phase\n";
      testTopologyDiscovery();
      phase_ = ValidationPhase::RESILIENCE_TESTS;
      break;
      
    case ValidationPhase::RESILIENCE_TESTS:
      std::cout << "[VAL] Completing RESILIENCE_TESTS phase\n";
      testNetworkResilience();
      phase_ = ValidationPhase::COMPLETE;
      report_.finalize();
      report_.print();
      break;
      
    case ValidationPhase::COMPLETE:
      // All tests complete
      test_progress_task_.disable();
      message_test_task_.disable();
      status_report_task_.disable();
      break;
  }
  
  phase_start_time_ = millis();
}

void LibraryValidationFirmware::testMeshLifecycle() {
  std::cout << "[VAL] Testing Mesh Lifecycle APIs\n";
  
  // Test: mesh is initialized
  auto result = testAPI("init()", [this]() {
    return mesh_ != nullptr && initialized_;
  });
  report_.addResult(result);
  report_.coverage.recordTest("init()", result.passed);
  
  // Test: getNodeId works
  result = testAPI("getNodeId()", [this]() {
    return node_id_ != 0;
  });
  report_.addResult(result);
  
  // Test: getNodeTime works
  result = testAPI("getNodeTime()", [this]() {
    uint32_t time = getNodeTime();
    return time > 0;
  });
  report_.addResult(result);
  report_.coverage.recordTest("getNodeTime()", result.passed);
  
  // Test: getNodeList works
  result = testAPI("getNodeList()", [this]() {
    auto node_list = getNodeList();
    return true;  // Just checking it doesn't crash
  });
  report_.addResult(result);
  report_.coverage.recordTest("getNodeList()", result.passed);
}

void LibraryValidationFirmware::testMessageSending() {
  std::cout << "[VAL] Testing Message Sending APIs\n";
  
  auto node_list = getNodeList();
  if (node_list.empty()) {
    report_.addResult(TestResult("sendSingle()", false, "No nodes to send to"));
    report_.addResult(TestResult("sendBroadcast()", false, "No nodes to broadcast to"));
    return;
  }
  
  // Test: sendBroadcast without priority
  auto result = testAPI("sendBroadcast(String)", [this]() {
    String msg = "VALIDATION_BROADCAST:" + std::to_string(node_id_);
    sendBroadcast(msg);
    recordMessage(true);
    return true;  // If we got here without exception, it worked
  });
  report_.addResult(result);
  report_.coverage.recordTest("sendBroadcast()", result.passed);
  
  // Test: sendSingle
  uint32_t target = *node_list.begin();
  result = testAPI("sendSingle(uint32_t, String)", [this, target]() {
    String msg = "VALIDATION_SINGLE:" + std::to_string(node_id_);
    sendSingle(target, msg);
    recordMessage(true);
    return true;  // If we got here without exception, it worked
  });
  report_.addResult(result);
  report_.coverage.recordTest("sendSingle()", result.passed);
  report_.addResult(result);
}

void LibraryValidationFirmware::testMessageReception() {
  std::cout << "[VAL] Testing Message Reception\n";
  
  // Test: onReceive callback was registered and received messages
  auto result = testAPI("onReceive() callback", [this]() {
    return report_.total_messages_received > 0;
  });
  report_.addResult(result);
  report_.coverage.recordTest("onReceive()", result.passed);
  
  // Test: Messages received from multiple nodes
  result = testAPI("Multi-node reception", [this]() {
    return messages_from_node_.size() > 1;
  });
  report_.addResult(result);
}

void LibraryValidationFirmware::testConnectionManagement() {
  std::cout << "[VAL] Testing Connection Management APIs\n";
  
  // Test: onNewConnection callback fired
  auto result = testAPI("onNewConnection() callback", [this]() {
    return connected_nodes_.size() > 0;
  });
  report_.addResult(result);
  report_.coverage.recordTest("onNewConnection()", result.passed);
  
  // Test: onChangedConnections callback fired
  result = testAPI("onChangedConnections() callback", [this]() {
    return report_.topology_change_count > 0;
  });
  report_.addResult(result);
  report_.coverage.recordTest("onChangedConnections()", result.passed);
  
  // Test: getNodeList returns connected nodes
  result = testAPI("getNodeList() accuracy", [this]() {
    auto node_list = getNodeList();
    return node_list.size() >= connected_nodes_.size();
  });
  report_.addResult(result);
}

void LibraryValidationFirmware::testTimeSynchronization() {
  std::cout << "[VAL] Testing Time Synchronization APIs\n";
  
  // Test: onNodeTimeAdjusted callback fired
  auto result = testAPI("onNodeTimeAdjusted() callback", [this]() {
    return time_adjustments_.size() > 0;
  });
  report_.addResult(result);
  report_.coverage.recordTest("onNodeTimeAdjusted()", result.passed);
  
  // Test: Time has progressed
  result = testAPI("getNodeTime() progression", [this]() {
    uint32_t current_time = getNodeTime();
    return current_time > initial_node_time_;
  });
  report_.addResult(result);
  
  // Calculate time sync error
  if (!time_adjustments_.empty()) {
    uint32_t total_error = 0;
    uint32_t max_error = 0;
    for (auto offset : time_adjustments_) {
      uint32_t error = std::abs(offset);
      total_error += error;
      if (error > max_error) max_error = error;
    }
    report_.avg_time_sync_error_us = total_error / time_adjustments_.size();
    report_.max_time_sync_error_us = max_error;
  }
}

void LibraryValidationFirmware::testTopologyDiscovery() {
  std::cout << "[VAL] Testing Topology Discovery APIs\n";
  
  // Test: Topology changes detected
  auto result = testAPI("Topology change detection", [this]() {
    return report_.topology_change_count > 0;
  });
  report_.addResult(result);
  
  // Test: Node list is consistent
  result = testAPI("Node list consistency", [this]() {
    auto node_list = getNodeList();
    // Check that all nodes in node_list are valid (non-zero)
    for (auto node_id : node_list) {
      if (node_id == 0) return false;
    }
    return true;
  });
  report_.addResult(result);
}

void LibraryValidationFirmware::testNetworkResilience() {
  std::cout << "[VAL] Testing Network Resilience\n";
  
  // Test: Mesh remained stable
  auto result = testAPI("Mesh stability", [this]() {
    auto node_list = getNodeList();
    return node_list.size() > 0;
  });
  report_.addResult(result);
  
  // Test: Message delivery rate acceptable
  result = testAPI("Message delivery rate", [this]() {
    double loss_rate = report_.message_loss_rate;
    return loss_rate < 10.0;  // Less than 10% loss acceptable
  });
  report_.addResult(result);
}

TestResult LibraryValidationFirmware::testAPI(const std::string& api_name,
                                               std::function<bool()> test_func) {
  auto start = std::chrono::steady_clock::now();
  
  try {
    bool passed = test_func();
    
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    TestResult result(api_name, passed);
    result.duration_ms = static_cast<uint32_t>(duration.count());
    
    if (!passed) {
      result.details = "Test condition not met";
    }
    
    std::cout << "[VAL] Test: " << api_name << " - " 
              << (passed ? "PASSED" : "FAILED") << "\n";
    
    return result;
  } catch (const std::exception& e) {
    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    TestResult result(api_name, false, std::string("Exception: ") + e.what());
    result.duration_ms = static_cast<uint32_t>(duration.count());
    
    std::cout << "[VAL] Test: " << api_name << " - FAILED (exception: " 
              << e.what() << ")\n";
    
    return result;
  }
}

void LibraryValidationFirmware::recordMessage(bool sent, uint32_t from) {
  if (sent) {
    report_.total_messages_sent++;
  } else {
    report_.total_messages_received++;
  }
}

void LibraryValidationFirmware::checkExpectedMessages() {
  // Check for message loss
  // Expected: each connected node should have sent at least one message
  for (auto node_id : connected_nodes_) {
    if (messages_from_node_[node_id] == 0) {
      report_.message_loss_count++;
    }
  }
}

} // namespace firmware
} // namespace simulator
