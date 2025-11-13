/**
 * @file test_node_lifecycle.cpp
 * @brief Unit tests for node lifecycle events and methods
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include <catch2/catch_test_macros.hpp>
#include "simulator/virtual_node.hpp"
#include "simulator/node_manager.hpp"
#include "simulator/event_scheduler.hpp"
#include "simulator/events/node_start_event.hpp"
#include "simulator/events/node_stop_event.hpp"
#include "simulator/events/node_crash_event.hpp"
#include "simulator/events/node_restart_event.hpp"
#include "simulator/network_simulator.hpp"
#include <boost/asio.hpp>
#include <thread>
#include <chrono>

using namespace simulator;

TEST_CASE("VirtualNode lifecycle methods", "[virtual_node][lifecycle]") {
  boost::asio::io_context io;
  NodeManager manager(io);
  
  NodeConfig config;
  config.nodeId = 7001;
  config.meshPrefix = "TestMesh";
  config.meshPassword = "password";
  config.meshPort = 5555;
  
  auto node = manager.createNode(config);
  
  SECTION("crash() increments crash count") {
    node->start();
    REQUIRE(node->isRunning());
    REQUIRE(node->getCrashCount() == 0);
    
    node->crash();
    REQUIRE_FALSE(node->isRunning());
    REQUIRE(node->getCrashCount() == 1);
    
    // Start and crash again
    node->start();
    node->crash();
    REQUIRE(node->getCrashCount() == 2);
  }
  
  SECTION("crash() does nothing if node not running") {
    REQUIRE_FALSE(node->isRunning());
    uint32_t initial_count = node->getCrashCount();
    
    node->crash();
    REQUIRE_FALSE(node->isRunning());
    REQUIRE(node->getCrashCount() == initial_count);
  }
  
  SECTION("restart() stops and starts node") {
    node->start();
    REQUIRE(node->isRunning());
    
    node->restart();
    REQUIRE(node->isRunning());
  }
  
  SECTION("getUptime() returns 0 when not running") {
    REQUIRE_FALSE(node->isRunning());
    REQUIRE(node->getUptime() == 0);
  }
  
  SECTION("getUptime() returns time since start") {
    node->start();
    
    // Sleep for a small amount of time
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    uint64_t uptime = node->getUptime();
    REQUIRE(uptime >= 10);  // At least 10ms
    REQUIRE(uptime < 1000); // But not too long
  }
  
  SECTION("uptime is tracked across stop/start") {
    node->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    auto metrics1 = node->getMetrics();
    node->stop();
    
    // Total uptime should be stored
    auto metrics2 = node->getMetrics();
    REQUIRE(metrics2.total_uptime_ms >= 10);
    
    // Start again
    node->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    node->stop();
    
    // Total uptime should accumulate
    auto metrics3 = node->getMetrics();
    REQUIRE(metrics3.total_uptime_ms >= metrics2.total_uptime_ms + 10);
  }
  
  SECTION("crash updates total uptime") {
    node->start();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    node->crash();
    auto metrics = node->getMetrics();
    REQUIRE(metrics.total_uptime_ms >= 10);
    REQUIRE(metrics.crash_count == 1);
  }
}

TEST_CASE("NodeStartEvent", "[event][lifecycle]") {
  boost::asio::io_context io;
  NodeManager manager(io);
  NetworkSimulator network;
  
  NodeConfig config;
  config.nodeId = 2001;
  config.meshPrefix = "TestMesh";
  config.meshPassword = "password";
  
  auto node = manager.createNode(config);
  
  SECTION("starts a stopped node") {
    REQUIRE_FALSE(node->isRunning());
    
    NodeStartEvent event(2001);
    event.execute(manager, network);
    
    REQUIRE(node->isRunning());
  }
  
  SECTION("does nothing if node already running") {
    node->start();
    REQUIRE(node->isRunning());
    
    NodeStartEvent event(2001);
    REQUIRE_NOTHROW(event.execute(manager, network));
    REQUIRE(node->isRunning());
  }
  
  SECTION("throws if node doesn't exist") {
    NodeStartEvent event(9999);
    REQUIRE_THROWS_AS(event.execute(manager, network), std::runtime_error);
  }
  
  SECTION("provides descriptive message") {
    NodeStartEvent event(2001);
    REQUIRE(event.getDescription() == "Start node: 2001");
  }
  
  SECTION("can be scheduled") {
    EventScheduler scheduler;
    scheduler.scheduleEvent(std::make_unique<NodeStartEvent>(2001), 30);
    
    REQUIRE(scheduler.hasPendingEvents());
    REQUIRE(scheduler.getPendingEventCount() == 1);
    REQUIRE(scheduler.getNextEventTime() == 30);
  }
}

TEST_CASE("NodeStopEvent", "[event][lifecycle]") {
  boost::asio::io_context io;
  NodeManager manager(io);
  NetworkSimulator network;
  
  NodeConfig config;
  config.nodeId = 3001;
  config.meshPrefix = "TestMesh";
  config.meshPassword = "password";
  
  auto node = manager.createNode(config);
  node->start();
  
  SECTION("stops a running node") {
    REQUIRE(node->isRunning());
    
    NodeStopEvent event(3001);
    event.execute(manager, network);
    
    REQUIRE_FALSE(node->isRunning());
  }
  
  SECTION("does nothing if node already stopped") {
    node->stop();
    REQUIRE_FALSE(node->isRunning());
    
    NodeStopEvent event(3001);
    REQUIRE_NOTHROW(event.execute(manager, network));
    REQUIRE_FALSE(node->isRunning());
  }
  
  SECTION("throws if node doesn't exist") {
    NodeStopEvent event(9999);
    REQUIRE_THROWS_AS(event.execute(manager, network), std::runtime_error);
  }
  
  SECTION("provides descriptive message with graceful flag") {
    NodeStopEvent event1(3001, true);
    REQUIRE(event1.getDescription() == "Stop node: 3001 (graceful)");
    
    NodeStopEvent event2(3001, false);
    REQUIRE(event2.getDescription() == "Stop node: 3001");
  }
  
  SECTION("graceful flag is accessible") {
    NodeStopEvent event1(3001, true);
    REQUIRE(event1.isGraceful());
    
    NodeStopEvent event2(3001, false);
    REQUIRE_FALSE(event2.isGraceful());
  }
  
  SECTION("can be scheduled") {
    EventScheduler scheduler;
    scheduler.scheduleEvent(std::make_unique<NodeStopEvent>(3001, true), 30);
    
    REQUIRE(scheduler.hasPendingEvents());
    REQUIRE(scheduler.getPendingEventCount() == 1);
    REQUIRE(scheduler.getNextEventTime() == 30);
  }
}

TEST_CASE("NodeCrashEvent", "[event][lifecycle]") {
  boost::asio::io_context io;
  NodeManager manager(io);
  NetworkSimulator network;
  
  NodeConfig config;
  config.nodeId = 4001;
  config.meshPrefix = "TestMesh";
  config.meshPassword = "password";
  
  auto node = manager.createNode(config);
  node->start();
  
  SECTION("crashes a running node") {
    REQUIRE(node->isRunning());
    REQUIRE(node->getCrashCount() == 0);
    
    NodeCrashEvent event(4001);
    event.execute(manager, network);
    
    REQUIRE_FALSE(node->isRunning());
    REQUIRE(node->getCrashCount() == 1);
  }
  
  SECTION("increments crash count each time") {
    NodeCrashEvent event(4001);
    
    event.execute(manager, network);
    REQUIRE(node->getCrashCount() == 1);
    
    node->start();
    event.execute(manager, network);
    REQUIRE(node->getCrashCount() == 2);
  }
  
  SECTION("does nothing if node already stopped") {
    node->crash();
    uint32_t crash_count = node->getCrashCount();
    
    NodeCrashEvent event(4001);
    REQUIRE_NOTHROW(event.execute(manager, network));
    
    // Crash count should not increase if already stopped
    REQUIRE(node->getCrashCount() == crash_count);
  }
  
  SECTION("throws if node doesn't exist") {
    NodeCrashEvent event(9999);
    REQUIRE_THROWS_AS(event.execute(manager, network), std::runtime_error);
  }
}

TEST_CASE("NodeRestartEvent", "[event][lifecycle]") {
  boost::asio::io_context io;
  NodeManager manager(io);
  NetworkSimulator network;
  
  NodeConfig config;
  config.nodeId = 5001;
  config.meshPrefix = "TestMesh";
  config.meshPassword = "password";
  
  auto node = manager.createNode(config);
  node->start();
  
  SECTION("restarts a running node") {
    REQUIRE(node->isRunning());
    
    NodeRestartEvent event(5001);
    event.execute(manager, network);
    
    REQUIRE(node->isRunning());
  }
  
  SECTION("starts a stopped node") {
    node->stop();
    REQUIRE_FALSE(node->isRunning());
    
    NodeRestartEvent event(5001);
    event.execute(manager, network);
    
    REQUIRE(node->isRunning());
  }
  
  SECTION("throws if node doesn't exist") {
    NodeRestartEvent event(9999);
    REQUIRE_THROWS_AS(event.execute(manager, network), std::runtime_error);
  }
  
  SECTION("provides descriptive message") {
    NodeRestartEvent event(5001);
    REQUIRE(event.getDescription() == "Restart node: 5001");
  }
  
  SECTION("can be scheduled") {
    EventScheduler scheduler;
    scheduler.scheduleEvent(std::make_unique<NodeRestartEvent>(5001), 30);
    
    REQUIRE(scheduler.hasPendingEvents());
    REQUIRE(scheduler.getPendingEventCount() == 1);
    REQUIRE(scheduler.getNextEventTime() == 30);
  }
}

TEST_CASE("Lifecycle event integration", "[event][lifecycle][integration]") {
  boost::asio::io_context io;
  NodeManager manager(io);
  NetworkSimulator network;
  EventScheduler scheduler;
  
  // Create a node
  NodeConfig config;
  config.nodeId = 6001;
  config.meshPrefix = "TestMesh";
  config.meshPassword = "password";
  
  auto node = manager.createNode(config);
  
  SECTION("simulate node crash and restart scenario") {
    // Schedule events
    scheduler.scheduleEvent(std::make_unique<NodeStartEvent>(6001), 0);
    scheduler.scheduleEvent(std::make_unique<NodeCrashEvent>(6001), 30);
    scheduler.scheduleEvent(std::make_unique<NodeStartEvent>(6001), 45);
    scheduler.scheduleEvent(std::make_unique<NodeStopEvent>(6001, true), 60);
    
    // Simulate time progression
    uint32_t executed = scheduler.processEvents(0, manager, network);
    REQUIRE(executed == 1);
    REQUIRE(node->isRunning());
    
    executed = scheduler.processEvents(30, manager, network);
    REQUIRE(executed == 1);
    REQUIRE_FALSE(node->isRunning());
    REQUIRE(node->getCrashCount() == 1);
    
    executed = scheduler.processEvents(45, manager, network);
    REQUIRE(executed == 1);
    REQUIRE(node->isRunning());
    
    executed = scheduler.processEvents(60, manager, network);
    REQUIRE(executed == 1);
    REQUIRE_FALSE(node->isRunning());
    REQUIRE(node->getCrashCount() == 1); // Graceful stop doesn't increment
  }
  
  SECTION("multiple crashes increment counter") {
    node->start();
    
    scheduler.scheduleEvent(std::make_unique<NodeCrashEvent>(6001), 10);
    scheduler.scheduleEvent(std::make_unique<NodeStartEvent>(6001), 20);
    scheduler.scheduleEvent(std::make_unique<NodeCrashEvent>(6001), 30);
    scheduler.scheduleEvent(std::make_unique<NodeStartEvent>(6001), 40);
    scheduler.scheduleEvent(std::make_unique<NodeCrashEvent>(6001), 50);
    
    scheduler.processEvents(50, manager, network);
    
    REQUIRE(node->getCrashCount() == 3);
  }
  
  SECTION("restart event works in sequence") {
    scheduler.scheduleEvent(std::make_unique<NodeStartEvent>(6001), 0);
    scheduler.scheduleEvent(std::make_unique<NodeRestartEvent>(6001), 30);
    scheduler.scheduleEvent(std::make_unique<NodeRestartEvent>(6001), 60);
    
    scheduler.processEvents(60, manager, network);
    
    REQUIRE(node->isRunning());
    REQUIRE(node->getCrashCount() == 0); // Restarts don't count as crashes
  }
}
