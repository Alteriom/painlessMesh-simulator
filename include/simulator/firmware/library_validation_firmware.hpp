/**
 * @file library_validation_firmware.hpp
 * @brief Comprehensive painlessMesh library validation firmware
 * 
 * This firmware systematically exercises all painlessMesh APIs to ensure
 * the simulator accurately represents real hardware behavior. It serves as
 * the baseline validation that must pass before custom application firmware
 * can be trusted to run correctly in the simulator.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef SIMULATOR_LIBRARY_VALIDATION_FIRMWARE_HPP
#define SIMULATOR_LIBRARY_VALIDATION_FIRMWARE_HPP

#include "firmware_base.hpp"
#include <TaskSchedulerDeclarations.h>
#include "painlessmesh/mesh.hpp"
#include "Arduino.h"
#include <iostream>
#include <vector>
#include <map>
#include <chrono>

namespace simulator {
namespace firmware {

/**
 * @brief Test result for individual validation test
 */
struct TestResult {
  std::string test_name;
  bool passed;
  std::string details;
  uint32_t duration_ms;
  
  TestResult(const std::string& name, bool pass, const std::string& detail = "")
    : test_name(name), passed(pass), details(detail), duration_ms(0) {}
};

/**
 * @brief API coverage tracking
 */
struct APICoverage {
  uint32_t total_apis = 0;
  uint32_t tested_apis = 0;
  uint32_t passed_apis = 0;
  uint32_t failed_apis = 0;
  
  std::map<std::string, bool> api_status;
  
  void recordTest(const std::string& api_name, bool passed) {
    tested_apis++;
    if (passed) {
      passed_apis++;
    } else {
      failed_apis++;
    }
    api_status[api_name] = passed;
  }
  
  double getPassRate() const {
    if (tested_apis == 0) return 0.0;
    return (double)passed_apis / tested_apis * 100.0;
  }
};

/**
 * @brief Comprehensive validation report
 */
struct ValidationReport {
  bool all_tests_passed = false;
  uint32_t total_tests = 0;
  uint32_t passed_tests = 0;
  uint32_t failed_tests = 0;
  
  APICoverage coverage;
  
  uint64_t total_messages_sent = 0;
  uint64_t total_messages_received = 0;
  uint64_t message_loss_count = 0;
  double message_loss_rate = 0.0;
  
  uint32_t avg_time_sync_error_us = 0;
  uint32_t max_time_sync_error_us = 0;
  
  uint32_t topology_change_count = 0;
  
  std::vector<TestResult> test_results;
  
  void addResult(const TestResult& result) {
    test_results.push_back(result);
    total_tests++;
    if (result.passed) {
      passed_tests++;
    } else {
      failed_tests++;
    }
  }
  
  void finalize() {
    all_tests_passed = (failed_tests == 0 && total_tests > 0);
    if (total_messages_sent > 0) {
      message_loss_rate = (double)message_loss_count / total_messages_sent * 100.0;
    }
  }
  
  void print() const {
    std::cout << "\n========== Library Validation Report ==========\n";
    std::cout << "Overall: " << (all_tests_passed ? "PASSED" : "FAILED") << "\n";
    std::cout << "Tests: " << passed_tests << "/" << total_tests << " passed";
    std::cout << " (" << failed_tests << " failed)\n";
    std::cout << "\nAPI Coverage:\n";
    std::cout << "  Tested: " << coverage.tested_apis << "/" << coverage.total_apis;
    std::cout << " (" << coverage.getPassRate() << "% pass rate)\n";
    std::cout << "\nMessage Statistics:\n";
    std::cout << "  Sent: " << total_messages_sent << "\n";
    std::cout << "  Received: " << total_messages_received << "\n";
    std::cout << "  Loss Rate: " << message_loss_rate << "%\n";
    std::cout << "\nFailed Tests:\n";
    for (const auto& result : test_results) {
      if (!result.passed) {
        std::cout << "  - " << result.test_name << ": " << result.details << "\n";
      }
    }
    std::cout << "===============================================\n\n";
  }
};

/**
 * @brief Validation state tracking
 */
enum class ValidationPhase {
  INITIALIZATION,
  MESH_FORMATION,
  MESSAGE_TESTS,
  TIME_SYNC_TESTS,
  TOPOLOGY_TESTS,
  RESILIENCE_TESTS,
  COMPLETE
};

/**
 * @brief Library validation firmware
 * 
 * This firmware systematically tests all painlessMesh library functionality:
 * 1. Mesh initialization and lifecycle
 * 2. Message sending (single, broadcast, priority)
 * 3. Message reception and callbacks
 * 4. Connection management
 * 5. Time synchronization
 * 6. Topology discovery
 * 7. Bridge functionality
 * 8. Network resilience
 * 
 * Configuration options:
 * - role: "coordinator" | "participant" (coordinator runs tests, participants respond)
 * - test_duration: Duration in seconds for each test phase
 * - enable_detailed_logging: Enable verbose test output
 */
class LibraryValidationFirmware : public FirmwareBase {
public:
  LibraryValidationFirmware();
  
  void setup() override;
  void loop() override;
  void onReceive(uint32_t from, String& msg) override;
  void onNewConnection(uint32_t nodeId) override;
  void onChangedConnections() override;
  void onNodeTimeAdjusted(int32_t offset) override;
  
  /**
   * @brief Get validation report
   * @return Current validation report
   */
  ValidationReport getReport() const { return report_; }
  
  /**
   * @brief Check if all tests completed
   * @return true if validation finished
   */
  bool isComplete() const { return phase_ == ValidationPhase::COMPLETE; }

private:
  // Test execution
  void runValidationTests();
  void progressToNextPhase();
  
  // Individual test suites
  void testMeshLifecycle();
  void testMessageSending();
  void testMessageReception();
  void testConnectionManagement();
  void testTimeSynchronization();
  void testTopologyDiscovery();
  void testNetworkResilience();
  
  // Test helpers
  TestResult testAPI(const std::string& api_name, 
                     std::function<bool()> test_func);
  void recordMessage(bool sent, uint32_t from = 0);
  void checkExpectedMessages();
  
  // Configuration
  bool is_coordinator_{false};
  bool detailed_logging_{false};
  uint32_t test_duration_ms_{30000};  // 30 seconds per phase
  
  // State
  ValidationPhase phase_{ValidationPhase::INITIALIZATION};
  ValidationReport report_;
  
  // Tracking
  std::set<uint32_t> expected_nodes_;
  std::set<uint32_t> connected_nodes_;
  std::map<uint32_t, uint32_t> messages_from_node_;
  std::vector<int32_t> time_adjustments_;
  uint32_t initial_node_time_{0};
  uint32_t phase_start_time_{0};
  
  // Test tasks
  Task test_progress_task_;
  Task message_test_task_;
  Task status_report_task_;
};

} // namespace firmware
} // namespace simulator

#endif // SIMULATOR_LIBRARY_VALIDATION_FIRMWARE_HPP
