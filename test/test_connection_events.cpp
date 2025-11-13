/**
 * @file test_connection_events.cpp
 * @brief Tests for connection control events
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "simulator/events/connection_drop_event.hpp"
#include "simulator/events/connection_restore_event.hpp"
#include "simulator/events/connection_degrade_event.hpp"
#include "simulator/network_simulator.hpp"
#include "simulator/node_manager.hpp"

using namespace simulator;

TEST_CASE("NetworkSimulator connection control", "[network][connection]") {
  NetworkSimulator network(12345);  // Deterministic seed
  
  SECTION("connections are active by default") {
    REQUIRE(network.isConnectionActive(1001, 1002));
    REQUIRE(network.isConnectionActive(1002, 1001));
    REQUIRE(network.isConnectionActive(2001, 2002));
  }
  
  SECTION("can drop a connection") {
    network.dropConnection(1001, 1002);
    
    REQUIRE_FALSE(network.isConnectionActive(1001, 1002));
    // Other direction should still be active until explicitly dropped
    REQUIRE(network.isConnectionActive(1002, 1001));
  }
  
  SECTION("can drop connection in both directions") {
    network.dropConnection(1001, 1002);
    network.dropConnection(1002, 1001);
    
    REQUIRE_FALSE(network.isConnectionActive(1001, 1002));
    REQUIRE_FALSE(network.isConnectionActive(1002, 1001));
  }
  
  SECTION("can restore a dropped connection") {
    network.dropConnection(1001, 1002);
    REQUIRE_FALSE(network.isConnectionActive(1001, 1002));
    
    network.restoreConnection(1001, 1002);
    REQUIRE(network.isConnectionActive(1001, 1002));
  }
  
  SECTION("restoring active connection is safe") {
    REQUIRE(network.isConnectionActive(1001, 1002));
    network.restoreConnection(1001, 1002);
    REQUIRE(network.isConnectionActive(1001, 1002));
  }
  
  SECTION("dropped connections affect different node pairs independently") {
    network.dropConnection(1001, 1002);
    
    REQUIRE_FALSE(network.isConnectionActive(1001, 1002));
    REQUIRE(network.isConnectionActive(2001, 2002));
    REQUIRE(network.isConnectionActive(1001, 2001));
  }
}

TEST_CASE("NetworkSimulator dropped connection blocks messages", "[network][connection]") {
  NetworkSimulator network(12345);
  uint64_t currentTime = 1000000;  // 1 second
  
  SECTION("messages are queued on active connection") {
    network.enqueueMessage(1001, 1002, "test message", currentTime);
    REQUIRE(network.getPendingMessageCount() == 1);
  }
  
  SECTION("messages are dropped on inactive connection") {
    network.dropConnection(1001, 1002);
    network.enqueueMessage(1001, 1002, "test message", currentTime);
    
    REQUIRE(network.getPendingMessageCount() == 0);
    
    // Verify stats show dropped packet
    auto stats = network.getStats(1001, 1002);
    REQUIRE(stats.dropped_count == 1);
    REQUIRE(stats.delivered_count == 0);
  }
  
  SECTION("messages resume after connection restore") {
    // Drop connection
    network.dropConnection(1001, 1002);
    network.enqueueMessage(1001, 1002, "blocked message", currentTime);
    REQUIRE(network.getPendingMessageCount() == 0);
    
    // Restore connection
    network.restoreConnection(1001, 1002);
    network.enqueueMessage(1001, 1002, "resumed message", currentTime);
    REQUIRE(network.getPendingMessageCount() == 1);
    
    // Verify stats
    auto stats = network.getStats(1001, 1002);
    REQUIRE(stats.dropped_count == 1);
    REQUIRE(stats.delivered_count == 1);
  }
}

TEST_CASE("ConnectionDropEvent", "[event][connection]") {
  NetworkSimulator network;
  boost::asio::io_context io;
  NodeManager manager(io);
  
  SECTION("can be created with node IDs") {
    ConnectionDropEvent event(1001, 1002);
    REQUIRE(event.getFromNode() == 1001);
    REQUIRE(event.getToNode() == 1002);
  }
  
  SECTION("has descriptive string") {
    ConnectionDropEvent event(1001, 1002);
    std::string desc = event.getDescription();
    REQUIRE_THAT(desc, Catch::Matchers::ContainsSubstring("Drop connection"));
    REQUIRE_THAT(desc, Catch::Matchers::ContainsSubstring("1001"));
    REQUIRE_THAT(desc, Catch::Matchers::ContainsSubstring("1002"));
  }
  
  SECTION("drops connection in both directions") {
    ConnectionDropEvent event(1001, 1002);
    
    REQUIRE(network.isConnectionActive(1001, 1002));
    REQUIRE(network.isConnectionActive(1002, 1001));
    
    event.execute(manager, network);
    
    REQUIRE_FALSE(network.isConnectionActive(1001, 1002));
    REQUIRE_FALSE(network.isConnectionActive(1002, 1001));
  }
}

TEST_CASE("ConnectionRestoreEvent", "[event][connection]") {
  NetworkSimulator network;
  boost::asio::io_context io;
  NodeManager manager(io);
  
  SECTION("can be created with node IDs") {
    ConnectionRestoreEvent event(1001, 1002);
    REQUIRE(event.getFromNode() == 1001);
    REQUIRE(event.getToNode() == 1002);
  }
  
  SECTION("has descriptive string") {
    ConnectionRestoreEvent event(1001, 1002);
    std::string desc = event.getDescription();
    REQUIRE_THAT(desc, Catch::Matchers::ContainsSubstring("Restore connection"));
    REQUIRE_THAT(desc, Catch::Matchers::ContainsSubstring("1001"));
    REQUIRE_THAT(desc, Catch::Matchers::ContainsSubstring("1002"));
  }
  
  SECTION("restores dropped connection in both directions") {
    // First drop the connection
    network.dropConnection(1001, 1002);
    network.dropConnection(1002, 1001);
    
    REQUIRE_FALSE(network.isConnectionActive(1001, 1002));
    REQUIRE_FALSE(network.isConnectionActive(1002, 1001));
    
    // Restore it
    ConnectionRestoreEvent event(1001, 1002);
    event.execute(manager, network);
    
    REQUIRE(network.isConnectionActive(1001, 1002));
    REQUIRE(network.isConnectionActive(1002, 1001));
  }
  
  SECTION("safely restores already active connection") {
    ConnectionRestoreEvent event(1001, 1002);
    event.execute(manager, network);
    
    REQUIRE(network.isConnectionActive(1001, 1002));
    REQUIRE(network.isConnectionActive(1002, 1001));
  }
}

TEST_CASE("ConnectionDegradeEvent", "[event][connection]") {
  NetworkSimulator network;
  boost::asio::io_context io;
  NodeManager manager(io);
  
  SECTION("can be created with default parameters") {
    ConnectionDegradeEvent event(1001, 1002);
    REQUIRE(event.getFromNode() == 1001);
    REQUIRE(event.getToNode() == 1002);
    REQUIRE(event.getLatency() == 500);
    REQUIRE(event.getPacketLoss() == 0.30f);
  }
  
  SECTION("can be created with custom parameters") {
    ConnectionDegradeEvent event(1001, 1002, 1000, 0.50f);
    REQUIRE(event.getLatency() == 1000);
    REQUIRE(event.getPacketLoss() == 0.50f);
  }
  
  SECTION("has descriptive string") {
    ConnectionDegradeEvent event(1001, 1002, 500, 0.30f);
    std::string desc = event.getDescription();
    REQUIRE_THAT(desc, Catch::Matchers::ContainsSubstring("Degrade connection"));
    REQUIRE_THAT(desc, Catch::Matchers::ContainsSubstring("1001"));
    REQUIRE_THAT(desc, Catch::Matchers::ContainsSubstring("1002"));
    REQUIRE_THAT(desc, Catch::Matchers::ContainsSubstring("500"));
  }
  
  SECTION("increases latency for connection") {
    // Get baseline latency
    auto baseline = network.getLatency(1001, 1002);
    
    // Degrade connection
    ConnectionDegradeEvent event(1001, 1002, 500, 0.10f);
    event.execute(manager, network);
    
    // Check latency increased
    auto degraded = network.getLatency(1001, 1002);
    REQUIRE(degraded.min_ms >= 500);
  }
  
  SECTION("increases packet loss for connection") {
    // Degrade connection
    ConnectionDegradeEvent event(1001, 1002, 500, 0.40f);
    event.execute(manager, network);
    
    // Check packet loss increased
    auto loss = network.getPacketLoss(1001, 1002);
    REQUIRE(loss.probability == 0.40f);
  }
  
  SECTION("degrades connection in both directions") {
    ConnectionDegradeEvent event(1001, 1002, 500, 0.30f);
    event.execute(manager, network);
    
    // Check both directions
    auto latency1 = network.getLatency(1001, 1002);
    auto latency2 = network.getLatency(1002, 1001);
    REQUIRE(latency1.min_ms == 500);
    REQUIRE(latency2.min_ms == 500);
    
    auto loss1 = network.getPacketLoss(1001, 1002);
    auto loss2 = network.getPacketLoss(1002, 1001);
    REQUIRE(loss1.probability == 0.30f);
    REQUIRE(loss2.probability == 0.30f);
  }
}

TEST_CASE("Connection events integration", "[event][connection][integration]") {
  NetworkSimulator network(12345);
  boost::asio::io_context io;
  NodeManager manager(io);
  uint64_t currentTime = 1000000;
  
  SECTION("drop blocks messages, restore allows messages") {
    // Send initial message (should work)
    network.enqueueMessage(1001, 1002, "before drop", currentTime);
    REQUIRE(network.getPendingMessageCount() == 1);
    
    // Drop connection
    ConnectionDropEvent dropEvent(1001, 1002);
    dropEvent.execute(manager, network);
    
    // Try to send message (should be blocked)
    network.enqueueMessage(1001, 1002, "during drop", currentTime);
    REQUIRE(network.getPendingMessageCount() == 1);  // Still only first message
    
    // Restore connection
    ConnectionRestoreEvent restoreEvent(1001, 1002);
    restoreEvent.execute(manager, network);
    
    // Send message (should work again)
    network.enqueueMessage(1001, 1002, "after restore", currentTime);
    REQUIRE(network.getPendingMessageCount() == 2);
    
    // Check stats
    auto stats = network.getStats(1001, 1002);
    REQUIRE(stats.delivered_count == 2);
    REQUIRE(stats.dropped_count == 1);
  }
  
  SECTION("degrade increases latency and packet loss") {
    // Degrade connection
    ConnectionDegradeEvent degradeEvent(1001, 1002, 800, 0.50f);
    degradeEvent.execute(manager, network);
    
    // Send messages and verify some are dropped due to packet loss
    int sent = 0;
    int queued = 0;
    for (int i = 0; i < 100; i++) {
      size_t before = network.getPendingMessageCount();
      network.enqueueMessage(1001, 1002, "test", currentTime);
      size_t after = network.getPendingMessageCount();
      sent++;
      if (after > before) {
        queued++;
      }
    }
    
    // With 50% packet loss, expect roughly 50% of messages queued
    // Allow some variance due to randomness
    REQUIRE(queued > 30);
    REQUIRE(queued < 70);
    
    auto stats = network.getStats(1001, 1002);
    REQUIRE(stats.dropped_count > 30);
  }
  
  SECTION("multiple connection events in sequence") {
    // Drop connection
    ConnectionDropEvent drop(1001, 1002);
    drop.execute(manager, network);
    REQUIRE_FALSE(network.isConnectionActive(1001, 1002));
    
    // Restore connection
    ConnectionRestoreEvent restore(1001, 1002);
    restore.execute(manager, network);
    REQUIRE(network.isConnectionActive(1001, 1002));
    
    // Degrade connection
    ConnectionDegradeEvent degrade(1001, 1002, 1000, 0.20f);
    degrade.execute(manager, network);
    
    // Connection should still be active
    REQUIRE(network.isConnectionActive(1001, 1002));
    
    // But with degraded quality
    auto latency = network.getLatency(1001, 1002);
    REQUIRE(latency.min_ms == 1000);
    
    auto loss = network.getPacketLoss(1001, 1002);
    REQUIRE(loss.probability == 0.20f);
  }
}
