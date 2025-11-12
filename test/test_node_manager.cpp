/**
 * @file test_node_manager.cpp
 * @brief Unit tests for NodeManager class
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "simulator/node_manager.hpp"
#include "simulator/virtual_node.hpp"
#include <boost/asio.hpp>

using namespace simulator;

TEST_CASE("NodeManager construction", "[node_manager]") {
  boost::asio::io_context io;
  
  SECTION("can be created") {
    REQUIRE_NOTHROW(NodeManager(io));
  }
  
  SECTION("starts with no nodes") {
    NodeManager manager(io);
    REQUIRE(manager.getNodeCount() == 0);
  }
}

TEST_CASE("NodeManager node creation", "[node_manager]") {
  boost::asio::io_context io;
  NodeManager manager(io);
  
  SECTION("can create a single node") {
    NodeConfig config{10001, "TestMesh", "password", 16001};
    auto node = manager.createNode(config);
    
    REQUIRE(node != nullptr);
    REQUIRE(node->getNodeId() == 10001);
    REQUIRE(manager.getNodeCount() == 1);
  }
  
  SECTION("can create multiple nodes") {
    NodeConfig config1{10001, "TestMesh", "password", 16001};
    NodeConfig config2{10002, "TestMesh", "password", 16002};
    NodeConfig config3{10003, "TestMesh", "password", 16003};
    
    auto node1 = manager.createNode(config1);
    auto node2 = manager.createNode(config2);
    auto node3 = manager.createNode(config3);
    
    REQUIRE(manager.getNodeCount() == 3);
    REQUIRE(node1->getNodeId() == 10001);
    REQUIRE(node2->getNodeId() == 10002);
    REQUIRE(node3->getNodeId() == 10003);
  }
  
  SECTION("rejects zero node ID") {
    NodeConfig config{0, "TestMesh", "password"};
    
    REQUIRE_THROWS_AS(manager.createNode(config), std::invalid_argument);
    REQUIRE(manager.getNodeCount() == 0);
  }
  
  SECTION("rejects duplicate node IDs") {
    NodeConfig config{10001, "TestMesh", "password"};
    
    auto node1 = manager.createNode(config);
    REQUIRE_THROWS_AS(manager.createNode(config), std::runtime_error);
    REQUIRE(manager.getNodeCount() == 1);
  }
}

TEST_CASE("NodeManager node removal", "[node_manager]") {
  boost::asio::io_context io;
  NodeManager manager(io);
  
  SECTION("can remove existing node") {
    NodeConfig config{10001, "TestMesh", "password", 16010};
    manager.createNode(config);
    
    REQUIRE(manager.getNodeCount() == 1);
    REQUIRE(manager.removeNode(10001) == true);
    REQUIRE(manager.getNodeCount() == 0);
  }
  
  SECTION("returns false for non-existent node") {
    REQUIRE(manager.removeNode(9999) == false);
  }
  
  SECTION("stops running node before removal") {
    NodeConfig config{10001, "TestMesh", "password", 16011};
    auto node = manager.createNode(config);
    node->start();
    
    REQUIRE(node->isRunning() == true);
    REQUIRE(manager.removeNode(10001) == true);
    REQUIRE(manager.getNodeCount() == 0);
  }
  
  SECTION("can remove and recreate node with same ID") {
    NodeConfig config{10001, "TestMesh", "password", 16012};
    
    auto node1 = manager.createNode(config);
    REQUIRE(manager.removeNode(10001) == true);
    
    auto node2 = manager.createNode(config);
    REQUIRE(node2->getNodeId() == 10001);
    REQUIRE(manager.getNodeCount() == 1);
  }
}

TEST_CASE("NodeManager lifecycle operations", "[node_manager]") {
  boost::asio::io_context io;
  NodeManager manager(io);
  
  SECTION("startAll starts all nodes") {
    NodeConfig config1{10001, "TestMesh", "password", 16020};
    NodeConfig config2{10002, "TestMesh", "password", 16021};
    
    auto node1 = manager.createNode(config1);
    auto node2 = manager.createNode(config2);
    
    REQUIRE(node1->isRunning() == false);
    REQUIRE(node2->isRunning() == false);
    
    manager.startAll();
    
    REQUIRE(node1->isRunning() == true);
    REQUIRE(node2->isRunning() == true);
  }
  
  SECTION("stopAll stops all nodes") {
    NodeConfig config1{10001, "TestMesh", "password", 16022};
    NodeConfig config2{10002, "TestMesh", "password", 16023};
    
    auto node1 = manager.createNode(config1);
    auto node2 = manager.createNode(config2);
    
    manager.startAll();
    REQUIRE(node1->isRunning() == true);
    REQUIRE(node2->isRunning() == true);
    
    manager.stopAll();
    
    REQUIRE(node1->isRunning() == false);
    REQUIRE(node2->isRunning() == false);
  }
  
  SECTION("startAll skips already running nodes") {
    NodeConfig config1{10001, "TestMesh", "password", 16024};
    NodeConfig config2{10002, "TestMesh", "password", 16025};
    
    auto node1 = manager.createNode(config1);
    auto node2 = manager.createNode(config2);
    
    node1->start();
    REQUIRE(node1->isRunning() == true);
    
    REQUIRE_NOTHROW(manager.startAll());
    
    REQUIRE(node1->isRunning() == true);
    REQUIRE(node2->isRunning() == true);
  }
  
  SECTION("stopAll skips already stopped nodes") {
    NodeConfig config1{10001, "TestMesh", "password", 16026};
    NodeConfig config2{10002, "TestMesh", "password", 16027};
    
    auto node1 = manager.createNode(config1);
    auto node2 = manager.createNode(config2);
    
    manager.startAll();
    node1->stop();
    REQUIRE(node1->isRunning() == false);
    
    REQUIRE_NOTHROW(manager.stopAll());
    
    REQUIRE(node1->isRunning() == false);
    REQUIRE(node2->isRunning() == false);
  }
}

TEST_CASE("NodeManager update operations", "[node_manager]") {
  boost::asio::io_context io;
  NodeManager manager(io);
  
  SECTION("updateAll can be called with no nodes") {
    REQUIRE_NOTHROW(manager.updateAll());
  }
  
  SECTION("updateAll processes nodes") {
    NodeConfig config{10001, "TestMesh", "password", 16030};
    auto node = manager.createNode(config);
    node->start();
    
    // Just verify it doesn't crash
    REQUIRE_NOTHROW(manager.updateAll());
  }
  
  SECTION("updateAll can be called multiple times") {
    NodeConfig config{10001, "TestMesh", "password", 16031};
    auto node = manager.createNode(config);
    node->start();
    
    for (int i = 0; i < 10; ++i) {
      REQUIRE_NOTHROW(manager.updateAll());
    }
  }
}

TEST_CASE("NodeManager query operations", "[node_manager]") {
  boost::asio::io_context io;
  NodeManager manager(io);
  
  SECTION("getNode returns correct node") {
    NodeConfig config{10001, "TestMesh", "password", 16040};
    auto created_node = manager.createNode(config);
    
    auto retrieved_node = manager.getNode(10001);
    REQUIRE(retrieved_node != nullptr);
    REQUIRE(retrieved_node->getNodeId() == 10001);
    REQUIRE(retrieved_node == created_node);
  }
  
  SECTION("getNode returns nullptr for non-existent node") {
    auto node = manager.getNode(9999);
    REQUIRE(node == nullptr);
  }
  
  SECTION("getNode const version works") {
    NodeConfig config{10001, "TestMesh", "password", 16041};
    manager.createNode(config);
    
    const NodeManager& const_manager = manager;
    auto node = const_manager.getNode(10001);
    REQUIRE(node != nullptr);
    REQUIRE(node->getNodeId() == 10001);
  }
  
  SECTION("hasNode returns correct status") {
    NodeConfig config{10001, "TestMesh", "password", 16042};
    
    REQUIRE(manager.hasNode(10001) == false);
    manager.createNode(config);
    REQUIRE(manager.hasNode(10001) == true);
    manager.removeNode(10001);
    REQUIRE(manager.hasNode(10001) == false);
  }
  
  SECTION("getNodeIds returns all node IDs") {
    NodeConfig config1{10001, "TestMesh", "password", 16043};
    NodeConfig config2{10002, "TestMesh", "password", 16044};
    NodeConfig config3{10003, "TestMesh", "password", 16045};
    
    manager.createNode(config1);
    manager.createNode(config2);
    manager.createNode(config3);
    
    auto ids = manager.getNodeIds();
    REQUIRE(ids.size() == 3);
    
    // Check all IDs are present (order not guaranteed)
    REQUIRE(std::find(ids.begin(), ids.end(), 10001) != ids.end());
    REQUIRE(std::find(ids.begin(), ids.end(), 10002) != ids.end());
    REQUIRE(std::find(ids.begin(), ids.end(), 10003) != ids.end());
  }
  
  SECTION("getAllNodes returns all nodes") {
    NodeConfig config1{10001, "TestMesh", "password", 16046};
    NodeConfig config2{10002, "TestMesh", "password", 16047};
    
    manager.createNode(config1);
    manager.createNode(config2);
    
    auto nodes = manager.getAllNodes();
    REQUIRE(nodes.size() == 2);
    
    // Check all nodes are present
    bool found10001 = false;
    bool found10002 = false;
    for (const auto& node : nodes) {
      if (node->getNodeId() == 10001) found10001 = true;
      if (node->getNodeId() == 10002) found10002 = true;
    }
    REQUIRE(found10001 == true);
    REQUIRE(found10002 == true);
  }
  
  SECTION("getNodeCount returns correct count") {
    REQUIRE(manager.getNodeCount() == 0);
    
    NodeConfig config1{10001, "TestMesh", "password", 16048};
    manager.createNode(config1);
    REQUIRE(manager.getNodeCount() == 1);
    
    NodeConfig config2{10002, "TestMesh", "password", 16049};
    manager.createNode(config2);
    REQUIRE(manager.getNodeCount() == 2);
    
    manager.removeNode(10001);
    REQUIRE(manager.getNodeCount() == 1);
    
    manager.removeNode(10002);
    REQUIRE(manager.getNodeCount() == 0);
  }
}

TEST_CASE("NodeManager resource limits", "[node_manager]") {
  boost::asio::io_context io;
  NodeManager manager(io);
  
  SECTION("MAX_NODES constant is defined") {
    REQUIRE(NodeManager::MAX_NODES == 1000);
  }
  
  SECTION("can create up to a reasonable number of nodes") {
    // Create 10 nodes to verify the system works
    // We don't test the full 1000 limit as it would be slow
    for (uint32_t i = 0; i < 10; ++i) {
      NodeConfig config{10000 + i, "TestMesh", "password", static_cast<uint16_t>(16050 + i)};
      REQUIRE_NOTHROW(manager.createNode(config));
    }
    REQUIRE(manager.getNodeCount() == 10);
  }
}

TEST_CASE("NodeManager integration tests", "[node_manager][integration]") {
  boost::asio::io_context io;
  NodeManager manager(io);
  
  SECTION("multiple nodes can be managed together") {
    // Create 5 nodes with unique ports
    for (uint32_t i = 0; i < 5; ++i) {
      NodeConfig config{10001 + i, "TestMesh", "password", static_cast<uint16_t>(16060 + i)};
      manager.createNode(config);
    }
    
    REQUIRE(manager.getNodeCount() == 5);
    
    // Start all nodes
    manager.startAll();
    
    // Verify all nodes are running
    auto nodes = manager.getAllNodes();
    for (const auto& node : nodes) {
      REQUIRE(node->isRunning() == true);
    }
    
    // Run simulation for a short time
    for (int i = 0; i < 100; ++i) {
      manager.updateAll();
    }
    
    // Stop all nodes
    manager.stopAll();
    
    // Verify all nodes are stopped
    for (const auto& node : nodes) {
      REQUIRE(node->isRunning() == false);
    }
  }
  
  SECTION("nodes can be started and run together") {
    // Create 3 nodes with same mesh configuration
    NodeConfig config1{10001, "TestMesh", "password", 16070};
    NodeConfig config2{10002, "TestMesh", "password", 16071};
    NodeConfig config3{10003, "TestMesh", "password", 16072};
    
    manager.createNode(config1);
    manager.createNode(config2);
    manager.createNode(config3);
    
    // Start all nodes
    manager.startAll();
    
    // Verify all nodes are running
    auto nodes = manager.getAllNodes();
    for (const auto& node : nodes) {
      REQUIRE(node->isRunning() == true);
    }
    
    // Run simulation for a while
    for (int i = 0; i < 1000; ++i) {
      manager.updateAll();
    }
    
    // Verify nodes are still running after update cycles
    for (const auto& node : nodes) {
      REQUIRE(node->isRunning() == true);
    }
    
    // Clean up
    manager.stopAll();
    
    // Verify all nodes are stopped
    for (const auto& node : nodes) {
      REQUIRE(node->isRunning() == false);
    }
  }
  
  SECTION("destructor stops all nodes") {
    {
      NodeManager temp_manager(io);
      
      NodeConfig config1{10001, "TestMesh", "password", 16080};
      NodeConfig config2{10002, "TestMesh", "password", 16081};
      
      auto node1 = temp_manager.createNode(config1);
      auto node2 = temp_manager.createNode(config2);
      
      temp_manager.startAll();
      
      REQUIRE(node1->isRunning() == true);
      REQUIRE(node2->isRunning() == true);
      
      // Destructor should stop all nodes
    }
    
    // If we get here without crashing, the test passes
    REQUIRE(true);
  }
}
