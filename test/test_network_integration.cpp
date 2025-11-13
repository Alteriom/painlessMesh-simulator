/**
 * @file test_network_integration.cpp
 * @brief Integration tests for NetworkSimulator with multiple nodes
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include <catch2/catch_test_macros.hpp>

#include "simulator/network_simulator.hpp"
#include "simulator/node_manager.hpp"
#include "simulator/config_loader.hpp"

#define ARDUINO_ARCH_ESP8266
#define PAINLESSMESH_BOOST

#include <TaskSchedulerDeclarations.h>
#include "painlessmesh/mesh.hpp"

using namespace simulator;

TEST_CASE("NetworkSimulator integration with 10 nodes", "[network_integration]") {
  // Create network simulator with deterministic seed
  NetworkSimulator net_sim(42);
  
  // Configure network latency
  LatencyConfig config;
  config.min_ms = 10;
  config.max_ms = 50;
  config.distribution = DistributionType::NORMAL;
  net_sim.setDefaultLatency(config);
  
  // Simulate message passing between 10 nodes
  SECTION("messages are delayed correctly") {
    uint64_t current_time = 0;
    
    // Node 1 sends to nodes 2-10
    for (uint32_t to = 2; to <= 10; ++to) {
      net_sim.enqueueMessage(1, to, "Hello from node 1", current_time);
    }
    
    REQUIRE(net_sim.getPendingMessageCount() == 9);
    
    // No messages ready immediately
    auto ready = net_sim.getReadyMessages(current_time);
    REQUIRE(ready.empty());
    
    // After minimum latency, some messages may be ready
    current_time += 10;
    ready = net_sim.getReadyMessages(current_time);
    // May or may not have messages at exactly min latency - distribution dependent
    
    // After maximum latency, all messages should be ready
    current_time = 100;  // Well past max latency
    ready = net_sim.getReadyMessages(current_time);
    size_t total_ready = ready.size();
    
    // All 9 messages should have been delivered by now
    REQUIRE(total_ready == 9);
    REQUIRE(net_sim.getPendingMessageCount() == 0);
  }
  
  SECTION("statistics track latency correctly") {
    uint64_t current_time = 0;
    
    // Send 100 messages from node 1 to node 2
    for (int i = 0; i < 100; ++i) {
      net_sim.enqueueMessage(1, 2, "test", current_time);
      current_time += 5;  // Stagger messages
    }
    
    // Get all messages
    auto ready = net_sim.getReadyMessages(10000);
    REQUIRE(ready.size() == 100);
    
    // Check statistics
    auto stats = net_sim.getStats(1, 2);
    REQUIRE(stats.message_count == 100);
    REQUIRE(stats.min_latency_ms >= 10);
    REQUIRE(stats.max_latency_ms <= 50);
    REQUIRE(stats.avg_latency_ms >= 10);
    REQUIRE(stats.avg_latency_ms <= 50);
  }
  
  SECTION("different connections have independent latencies") {
    // Set different latencies for different connections
    LatencyConfig fast_config;
    fast_config.min_ms = 5;
    fast_config.max_ms = 10;
    fast_config.distribution = DistributionType::UNIFORM;
    
    LatencyConfig slow_config;
    slow_config.min_ms = 100;
    slow_config.max_ms = 150;
    slow_config.distribution = DistributionType::UNIFORM;
    
    net_sim.setLatency(1, 2, fast_config);
    net_sim.setLatency(1, 3, slow_config);
    
    uint64_t current_time = 0;
    
    // Send messages on both connections
    net_sim.enqueueMessage(1, 2, "fast", current_time);
    net_sim.enqueueMessage(1, 3, "slow", current_time);
    
    // Fast message should arrive first
    current_time = 15;
    auto ready = net_sim.getReadyMessages(current_time);
    REQUIRE(ready.size() == 1);
    REQUIRE(ready[0].to == 2);
    REQUIRE(ready[0].message == "fast");
    
    // Slow message arrives later
    current_time = 160;
    ready = net_sim.getReadyMessages(current_time);
    REQUIRE(ready.size() == 1);
    REQUIRE(ready[0].to == 3);
    REQUIRE(ready[0].message == "slow");
  }
}

TEST_CASE("NetworkSimulator with mesh topology", "[network_integration]") {
  // Simulate a 10-node mesh with random topology
  NetworkSimulator net_sim(123);
  
  // Configure normal distribution latency
  LatencyConfig config;
  config.min_ms = 20;
  config.max_ms = 80;
  config.distribution = DistributionType::NORMAL;
  net_sim.setDefaultLatency(config);
  
  SECTION("broadcast simulation") {
    uint64_t current_time = 0;
    
    // Node 1 broadcasts to all other nodes
    for (uint32_t to = 2; to <= 10; ++to) {
      net_sim.enqueueMessage(1, to, "broadcast message", current_time);
    }
    
    // Process messages in time steps
    std::vector<std::pair<uint64_t, size_t>> delivery_timeline;
    
    for (uint64_t t = 0; t <= 100; t += 5) {
      auto ready = net_sim.getReadyMessages(t);
      if (!ready.empty()) {
        delivery_timeline.push_back({t, ready.size()});
      }
    }
    
    // Verify messages were delivered over time
    REQUIRE(!delivery_timeline.empty());
    
    // All 9 messages should be delivered
    size_t total_delivered = 0;
    for (const auto& entry : delivery_timeline) {
      total_delivered += entry.second;
    }
    REQUIRE(total_delivered == 9);
  }
  
  SECTION("ring topology simulation") {
    uint64_t current_time = 0;
    
    // Create ring: 1->2->3->4->5->6->7->8->9->10->1
    for (uint32_t i = 1; i <= 10; ++i) {
      uint32_t next = (i % 10) + 1;
      net_sim.enqueueMessage(i, next, "ring message", current_time);
      current_time += 10;  // Stagger sends
    }
    
    // Process all messages
    auto ready = net_sim.getReadyMessages(1000);
    REQUIRE(ready.size() == 10);
  }
}

TEST_CASE("NetworkSimulator performance with many messages", "[network_integration]") {
  NetworkSimulator net_sim;
  
  LatencyConfig config;
  config.min_ms = 10;
  config.max_ms = 50;
  config.distribution = DistributionType::UNIFORM;
  net_sim.setDefaultLatency(config);
  
  SECTION("handles 1000 messages efficiently") {
    uint64_t current_time = 0;
    
    // Enqueue 1000 messages
    for (int i = 0; i < 1000; ++i) {
      uint32_t from = (i % 10) + 1;
      uint32_t to = ((i + 1) % 10) + 1;
      net_sim.enqueueMessage(from, to, "msg", current_time);
      current_time += 1;  // 1ms apart
    }
    
    REQUIRE(net_sim.getPendingMessageCount() == 1000);
    
    // Process all messages
    auto ready = net_sim.getReadyMessages(10000);
    REQUIRE(ready.size() == 1000);
    REQUIRE(net_sim.getPendingMessageCount() == 0);
  }
}

TEST_CASE("NetworkSimulator with ConfigLoader", "[network_integration]") {
  std::string yaml = R"(
simulation:
  name: "Network Test"
  duration: 60

nodes:
  - template: "sensor"
    count: 5
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"

network:
  latency:
    default:
      min: 15
      max: 45
      distribution: "normal"
    
    specific_connections:
      - from: "sensor-0"
        to: "sensor-1"
        min: 100
        max: 150
        distribution: "exponential"

topology:
  type: "random"
  )";
  
  ConfigLoader loader;
  auto config = loader.loadFromString(yaml);
  
  REQUIRE(config.has_value());
  
  // Create network simulator from config
  NetworkSimulator net_sim(42);
  net_sim.setDefaultLatency(config->network.default_latency);
  
  // Expand templates to get actual node IDs
  loader.expandTemplates(*config);
  
  // Apply specific connection latencies
  for (const auto& conn : config->network.specific_latencies) {
    // In real integration, we'd need to map node string IDs to numeric IDs
    // For this test, we'll use the hash-based ID generation
    uint32_t from_id = loader.generateNodeId(conn.from);
    uint32_t to_id = loader.generateNodeId(conn.to);
    net_sim.setLatency(from_id, to_id, conn.config);
  }
  
  SECTION("configuration is applied correctly") {
    auto default_config = net_sim.getLatency(999, 1000);
    REQUIRE(default_config.min_ms == 15);
    REQUIRE(default_config.max_ms == 45);
    REQUIRE(default_config.distribution == DistributionType::NORMAL);
    
    // Check specific connection
    uint32_t from_id = loader.generateNodeId("sensor-0");
    uint32_t to_id = loader.generateNodeId("sensor-1");
    auto specific_config = net_sim.getLatency(from_id, to_id);
    REQUIRE(specific_config.min_ms == 100);
    REQUIRE(specific_config.max_ms == 150);
    REQUIRE(specific_config.distribution == DistributionType::EXPONENTIAL);
  }
  
  SECTION("messages respect configured latencies") {
    uint64_t current_time = 0;
    
    // Send on default connection
    net_sim.enqueueMessage(999, 1000, "default", current_time);
    
    // Send on specific connection
    uint32_t from_id = loader.generateNodeId("sensor-0");
    uint32_t to_id = loader.generateNodeId("sensor-1");
    net_sim.enqueueMessage(from_id, to_id, "specific", current_time);
    
    // Default connection message should arrive first (15-45ms)
    auto ready = net_sim.getReadyMessages(50);
    bool found_default = false;
    for (const auto& msg : ready) {
      if (msg.message == "default") {
        found_default = true;
      }
    }
    REQUIRE(found_default);
    
    // Specific connection message arrives later (100-150ms)
    ready = net_sim.getReadyMessages(160);
    bool found_specific = false;
    for (const auto& msg : ready) {
      if (msg.message == "specific") {
        found_specific = true;
      }
    }
    REQUIRE(found_specific);
  }
}
