/**
 * @file test_virtual_node.cpp
 * @brief Unit tests for VirtualNode class
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include <catch2/catch_test_macros.hpp>

#include "simulator/virtual_node.hpp"

#define ARDUINO_ARCH_ESP8266
#define PAINLESSMESH_BOOST

#include <TaskSchedulerDeclarations.h>
#include "painlessmesh/mesh.hpp"

using namespace simulator;

TEST_CASE("VirtualNode construction", "[virtual_node]") {
  Scheduler scheduler;
  boost::asio::io_context io;
  NodeConfig config;
  config.nodeId = 6001;
  config.meshPrefix = "TestMesh";
  config.meshPassword = "testpass";
  
  SECTION("can be created with valid nodeId") {
    REQUIRE_NOTHROW(VirtualNode(6001, config, &scheduler, io));
  }
  
  SECTION("nodeId is correctly stored") {
    VirtualNode node(6001, config, &scheduler, io);
    REQUIRE(node.getNodeId() == 6001);
  }
  
  SECTION("rejects nodeId of 0") {
    NodeConfig invalid_config = config;
    invalid_config.nodeId = 0;
    REQUIRE_THROWS_AS(
      VirtualNode(0, invalid_config, &scheduler, io),
      std::invalid_argument
    );
  }
  
  SECTION("rejects null scheduler") {
    REQUIRE_THROWS_AS(
      VirtualNode(6001, config, nullptr, io),
      std::invalid_argument
    );
  }
  
  SECTION("initializes with running state false") {
    VirtualNode node(6001, config, &scheduler, io);
    REQUIRE(node.isRunning() == false);
  }
}

TEST_CASE("VirtualNode lifecycle", "[virtual_node]") {
  Scheduler scheduler;
  boost::asio::io_context io;
  NodeConfig config;
  config.nodeId = 6002;
  config.meshPrefix = "TestMesh";
  config.meshPassword = "testpass";
  
  SECTION("can be started") {
    VirtualNode node(6002, config, &scheduler, io);
    REQUIRE_NOTHROW(node.start());
    REQUIRE(node.isRunning() == true);
  }
  
  SECTION("can be stopped") {
    VirtualNode node(6002, config, &scheduler, io);
    node.start();
    REQUIRE_NOTHROW(node.stop());
    REQUIRE(node.isRunning() == false);
  }
  
  SECTION("throws when starting already running node") {
    VirtualNode node(6002, config, &scheduler, io);
    node.start();
    REQUIRE_THROWS_AS(node.start(), std::runtime_error);
  }
  
  SECTION("can be stopped multiple times safely") {
    VirtualNode node(6002, config, &scheduler, io);
    node.start();
    REQUIRE_NOTHROW(node.stop());
    REQUIRE_NOTHROW(node.stop()); // Second stop should be safe
  }
  
  SECTION("can be restarted after stop") {
    VirtualNode node(6002, config, &scheduler, io);
    node.start();
    node.stop();
    REQUIRE_NOTHROW(node.start());
    REQUIRE(node.isRunning() == true);
  }
}

TEST_CASE("VirtualNode update", "[virtual_node]") {
  Scheduler scheduler;
  boost::asio::io_context io;
  NodeConfig config;
  config.nodeId = 6003;
  config.meshPrefix = "TestMesh";
  config.meshPassword = "testpass";
  
  SECTION("can be called when running") {
    VirtualNode node(6003, config, &scheduler, io);
    node.start();
    REQUIRE_NOTHROW(node.update());
  }
  
  SECTION("can be called when not running") {
    VirtualNode node(6003, config, &scheduler, io);
    REQUIRE_NOTHROW(node.update());
  }
  
  SECTION("multiple updates are safe") {
    VirtualNode node(6003, config, &scheduler, io);
    node.start();
    for (int i = 0; i < 10; ++i) {
      REQUIRE_NOTHROW(node.update());
    }
  }
}

TEST_CASE("VirtualNode mesh access", "[virtual_node]") {
  Scheduler scheduler;
  boost::asio::io_context io;
  NodeConfig config;
  config.nodeId = 6004;
  config.meshPrefix = "TestMesh";
  config.meshPassword = "testpass";
  
  SECTION("getMesh returns valid reference") {
    VirtualNode node(6004, config, &scheduler, io);
    REQUIRE_NOTHROW(node.getMesh());
  }
  
  SECTION("getMesh const returns valid reference") {
    const VirtualNode node(6004, config, &scheduler, io);
    REQUIRE_NOTHROW(node.getMesh());
  }
  
  SECTION("mesh nodeId matches VirtualNode nodeId") {
    VirtualNode node(6004, config, &scheduler, io);
    auto& mesh = node.getMesh();
    REQUIRE(mesh.getNodeId() == 6004);
  }
}

TEST_CASE("VirtualNode metrics", "[virtual_node]") {
  Scheduler scheduler;
  boost::asio::io_context io;
  NodeConfig config;
  config.nodeId = 6005;
  config.meshPrefix = "TestMesh";
  config.meshPassword = "testpass";
  
  SECTION("metrics are initialized to zero") {
    VirtualNode node(6005, config, &scheduler, io);
    auto metrics = node.getMetrics();
    REQUIRE(metrics.messages_sent == 0);
    REQUIRE(metrics.messages_received == 0);
    REQUIRE(metrics.bytes_sent == 0);
    REQUIRE(metrics.bytes_received == 0);
  }
  
  SECTION("metrics start_time is set") {
    VirtualNode node(6005, config, &scheduler, io);
    auto metrics = node.getMetrics();
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
      now - metrics.start_time
    );
    // Should be less than 1 second old
    REQUIRE(elapsed.count() < 1);
  }
  
  SECTION("getMetrics returns copy") {
    VirtualNode node(6005, config, &scheduler, io);
    auto metrics1 = node.getMetrics();
    auto metrics2 = node.getMetrics();
    // Both calls should return valid metrics
    REQUIRE(metrics1.messages_sent == metrics2.messages_sent);
  }
}

TEST_CASE("VirtualNode network quality", "[virtual_node]") {
  Scheduler scheduler;
  boost::asio::io_context io;
  NodeConfig config;
  config.nodeId = 6006;
  config.meshPrefix = "TestMesh";
  config.meshPassword = "testpass";
  
  SECTION("accepts valid quality values") {
    VirtualNode node(6006, config, &scheduler, io);
    REQUIRE_NOTHROW(node.setNetworkQuality(0.0f));
    REQUIRE_NOTHROW(node.setNetworkQuality(0.5f));
    REQUIRE_NOTHROW(node.setNetworkQuality(1.0f));
  }
  
  SECTION("rejects quality below 0.0") {
    VirtualNode node(6006, config, &scheduler, io);
    REQUIRE_THROWS_AS(
      node.setNetworkQuality(-0.1f),
      std::invalid_argument
    );
  }
  
  SECTION("rejects quality above 1.0") {
    VirtualNode node(6006, config, &scheduler, io);
    REQUIRE_THROWS_AS(
      node.setNetworkQuality(1.1f),
      std::invalid_argument
    );
  }
}

TEST_CASE("VirtualNode destructor cleanup", "[virtual_node]") {
  Scheduler scheduler;
  boost::asio::io_context io;
  NodeConfig config;
  config.nodeId = 6007;
  config.meshPrefix = "TestMesh";
  config.meshPassword = "testpass";
  
  SECTION("destructor stops running node") {
    {
      VirtualNode node(6007, config, &scheduler, io);
      node.start();
      REQUIRE(node.isRunning() == true);
      // Node goes out of scope and destructor is called
    }
    // If we get here, destructor succeeded
    REQUIRE(true);
  }
  
  SECTION("destructor handles stopped node") {
    {
      VirtualNode node(6007, config, &scheduler, io);
      node.start();
      node.stop();
      REQUIRE(node.isRunning() == false);
      // Node goes out of scope and destructor is called
    }
    // If we get here, destructor succeeded
    REQUIRE(true);
  }
}

TEST_CASE("VirtualNode multi-node scenario", "[virtual_node][integration]") {
  Scheduler scheduler;
  boost::asio::io_context io;
  
  SECTION("multiple nodes can coexist") {
    NodeConfig config1;
    config1.nodeId = 6008;
    config1.meshPrefix = "TestMesh";
    config1.meshPassword = "testpass";
    
    NodeConfig config2;
    config2.nodeId = 6009;
    config2.meshPrefix = "TestMesh";
    config2.meshPassword = "testpass";
    
    VirtualNode node1(6008, config1, &scheduler, io);
    VirtualNode node2(6009, config2, &scheduler, io);
    
    REQUIRE(node1.getNodeId() == 6008);
    REQUIRE(node2.getNodeId() == 6009);
    
    node1.start();
    node2.start();
    
    REQUIRE(node1.isRunning() == true);
    REQUIRE(node2.isRunning() == true);
    
    // Run simulation for a few iterations
    for (int i = 0; i < 10; ++i) {
      node1.update();
      node2.update();
      io.poll();
    }
    
    node1.stop();
    node2.stop();
  }
}
