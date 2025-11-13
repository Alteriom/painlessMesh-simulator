/**
 * @file test_partition_events.cpp
 * @brief Tests for network partition and heal events
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "simulator/events/network_partition_event.hpp"
#include "simulator/events/network_heal_event.hpp"
#include "simulator/network_simulator.hpp"
#include "simulator/node_manager.hpp"

using namespace simulator;

TEST_CASE("VirtualNode partition tracking", "[node][partition]") {
  boost::asio::io_context io;
  NodeManager manager(io);
  
  NodeConfig config{1001, "TestMesh", "password", 16101};
  auto node = manager.createNode(config);
  
  SECTION("default partition ID is 0") {
    REQUIRE(node->getPartitionId() == 0);
  }
  
  SECTION("can set partition ID") {
    node->setPartitionId(1);
    REQUIRE(node->getPartitionId() == 1);
  }
  
  SECTION("can change partition ID") {
    node->setPartitionId(1);
    REQUIRE(node->getPartitionId() == 1);
    
    node->setPartitionId(2);
    REQUIRE(node->getPartitionId() == 2);
  }
  
  SECTION("can reset partition ID to 0") {
    node->setPartitionId(3);
    REQUIRE(node->getPartitionId() == 3);
    
    node->setPartitionId(0);
    REQUIRE(node->getPartitionId() == 0);
  }
}

TEST_CASE("NetworkSimulator restoreAllConnections", "[network][partition]") {
  NetworkSimulator network(12345);  // Deterministic seed
  
  SECTION("restores all dropped connections") {
    // Drop multiple connections
    network.dropConnection(1001, 1002);
    network.dropConnection(1002, 1001);
    network.dropConnection(1003, 1004);
    network.dropConnection(1004, 1003);
    network.dropConnection(1005, 1006);
    network.dropConnection(1006, 1005);
    
    // Verify they're dropped
    REQUIRE_FALSE(network.isConnectionActive(1001, 1002));
    REQUIRE_FALSE(network.isConnectionActive(1002, 1001));
    REQUIRE_FALSE(network.isConnectionActive(1003, 1004));
    REQUIRE_FALSE(network.isConnectionActive(1004, 1003));
    REQUIRE_FALSE(network.isConnectionActive(1005, 1006));
    REQUIRE_FALSE(network.isConnectionActive(1006, 1005));
    
    // Restore all
    network.restoreAllConnections();
    
    // Verify they're restored
    REQUIRE(network.isConnectionActive(1001, 1002));
    REQUIRE(network.isConnectionActive(1002, 1001));
    REQUIRE(network.isConnectionActive(1003, 1004));
    REQUIRE(network.isConnectionActive(1004, 1003));
    REQUIRE(network.isConnectionActive(1005, 1006));
    REQUIRE(network.isConnectionActive(1006, 1005));
  }
  
  SECTION("works when no connections are dropped") {
    REQUIRE(network.isConnectionActive(1001, 1002));
    
    network.restoreAllConnections();
    
    REQUIRE(network.isConnectionActive(1001, 1002));
  }
  
  SECTION("independent connections remain unaffected") {
    // Drop some connections
    network.dropConnection(1001, 1002);
    network.dropConnection(1002, 1001);
    
    // These were never dropped
    REQUIRE(network.isConnectionActive(2001, 2002));
    REQUIRE(network.isConnectionActive(3001, 3002));
    
    // Restore all
    network.restoreAllConnections();
    
    // All should be active
    REQUIRE(network.isConnectionActive(1001, 1002));
    REQUIRE(network.isConnectionActive(1002, 1001));
    REQUIRE(network.isConnectionActive(2001, 2002));
    REQUIRE(network.isConnectionActive(3001, 3002));
  }
}

TEST_CASE("NetworkPartitionEvent construction", "[event][partition]") {
  SECTION("can be created with 2 partition groups") {
    std::vector<std::vector<uint32_t>> groups = {
      {1001, 1002},
      {1003, 1004}
    };
    
    NetworkPartitionEvent event(groups);
    REQUIRE(event.getPartitionCount() == 2);
    REQUIRE(event.getPartitionGroups() == groups);
  }
  
  SECTION("can be created with 3 partition groups") {
    std::vector<std::vector<uint32_t>> groups = {
      {1001, 1002, 1003},
      {1004, 1005, 1006},
      {1007, 1008, 1009}
    };
    
    NetworkPartitionEvent event(groups);
    REQUIRE(event.getPartitionCount() == 3);
  }
  
  SECTION("throws when less than 2 groups") {
    std::vector<std::vector<uint32_t>> groups = {
      {1001, 1002, 1003}
    };
    
    REQUIRE_THROWS_AS(NetworkPartitionEvent(groups), std::invalid_argument);
  }
  
  SECTION("throws when empty groups vector") {
    std::vector<std::vector<uint32_t>> groups;
    
    REQUIRE_THROWS_AS(NetworkPartitionEvent(groups), std::invalid_argument);
  }
  
  SECTION("throws when a group is empty") {
    std::vector<std::vector<uint32_t>> groups = {
      {1001, 1002},
      {}  // Empty group
    };
    
    REQUIRE_THROWS_AS(NetworkPartitionEvent(groups), std::invalid_argument);
  }
  
  SECTION("has descriptive string") {
    std::vector<std::vector<uint32_t>> groups = {
      {1001, 1002},
      {1003, 1004}
    };
    
    NetworkPartitionEvent event(groups);
    std::string desc = event.getDescription();
    REQUIRE_THAT(desc, Catch::Matchers::ContainsSubstring("Partition network"));
    REQUIRE_THAT(desc, Catch::Matchers::ContainsSubstring("2 groups"));
  }
}

TEST_CASE("NetworkPartitionEvent execution with 2 partitions", "[event][partition]") {
  NetworkSimulator network;
  boost::asio::io_context io;
  NodeManager manager(io);
  
  // Create nodes with unique ports
  NodeConfig config1{1001, "TestMesh", "password", 16201};
  NodeConfig config2{1002, "TestMesh", "password", 16202};
  NodeConfig config3{1003, "TestMesh", "password", 16203};
  NodeConfig config4{1004, "TestMesh", "password", 16204};
  
  auto node1 = manager.createNode(config1);
  auto node2 = manager.createNode(config2);
  auto node3 = manager.createNode(config3);
  auto node4 = manager.createNode(config4);
  
  SECTION("drops connections between partition groups") {
    std::vector<std::vector<uint32_t>> groups = {
      {1001, 1002},
      {1003, 1004}
    };
    
    NetworkPartitionEvent event(groups);
    
    // Verify all connections are active before
    REQUIRE(network.isConnectionActive(1001, 1003));
    REQUIRE(network.isConnectionActive(1001, 1004));
    REQUIRE(network.isConnectionActive(1002, 1003));
    REQUIRE(network.isConnectionActive(1002, 1004));
    
    // Execute partition
    event.execute(manager, network);
    
    // Connections within groups should remain active
    REQUIRE(network.isConnectionActive(1001, 1002));
    REQUIRE(network.isConnectionActive(1002, 1001));
    REQUIRE(network.isConnectionActive(1003, 1004));
    REQUIRE(network.isConnectionActive(1004, 1003));
    
    // Connections between groups should be dropped
    REQUIRE_FALSE(network.isConnectionActive(1001, 1003));
    REQUIRE_FALSE(network.isConnectionActive(1001, 1004));
    REQUIRE_FALSE(network.isConnectionActive(1002, 1003));
    REQUIRE_FALSE(network.isConnectionActive(1002, 1004));
    REQUIRE_FALSE(network.isConnectionActive(1003, 1001));
    REQUIRE_FALSE(network.isConnectionActive(1003, 1002));
    REQUIRE_FALSE(network.isConnectionActive(1004, 1001));
    REQUIRE_FALSE(network.isConnectionActive(1004, 1002));
  }
  
  SECTION("assigns partition IDs to nodes") {
    std::vector<std::vector<uint32_t>> groups = {
      {1001, 1002},
      {1003, 1004}
    };
    
    NetworkPartitionEvent event(groups);
    event.execute(manager, network);
    
    // Check partition IDs (1-based)
    REQUIRE(node1->getPartitionId() == 1);
    REQUIRE(node2->getPartitionId() == 1);
    REQUIRE(node3->getPartitionId() == 2);
    REQUIRE(node4->getPartitionId() == 2);
  }
  
  SECTION("blocks messages between partitions") {
    std::vector<std::vector<uint32_t>> groups = {
      {1001, 1002},
      {1003, 1004}
    };
    
    NetworkPartitionEvent event(groups);
    event.execute(manager, network);
    
    uint64_t currentTime = 1000000;
    
    // Messages within partition should work
    network.enqueueMessage(1001, 1002, "within partition 1", currentTime);
    network.enqueueMessage(1003, 1004, "within partition 2", currentTime);
    REQUIRE(network.getPendingMessageCount() == 2);
    
    // Messages between partitions should be dropped
    network.enqueueMessage(1001, 1003, "across partitions", currentTime);
    network.enqueueMessage(1002, 1004, "across partitions", currentTime);
    REQUIRE(network.getPendingMessageCount() == 2);  // Still only 2
    
    // Verify stats show drops
    auto stats1 = network.getStats(1001, 1003);
    REQUIRE(stats1.dropped_count == 1);
    
    auto stats2 = network.getStats(1002, 1004);
    REQUIRE(stats2.dropped_count == 1);
  }
}

TEST_CASE("NetworkPartitionEvent execution with 3 partitions", "[event][partition]") {
  NetworkSimulator network;
  boost::asio::io_context io;
  NodeManager manager(io);
  
  // Create 9 nodes (3 per partition) with unique ports
  std::vector<std::shared_ptr<VirtualNode>> nodes;
  for (uint32_t i = 1001; i <= 1009; ++i) {
    NodeConfig config{i, "TestMesh", "password", static_cast<uint16_t>(16300 + (i - 1001))};
    nodes.push_back(manager.createNode(config));
  }
  
  SECTION("drops connections between all partition pairs") {
    std::vector<std::vector<uint32_t>> groups = {
      {1001, 1002, 1003},
      {1004, 1005, 1006},
      {1007, 1008, 1009}
    };
    
    NetworkPartitionEvent event(groups);
    event.execute(manager, network);
    
    // Connections within each partition should be active
    REQUIRE(network.isConnectionActive(1001, 1002));
    REQUIRE(network.isConnectionActive(1001, 1003));
    REQUIRE(network.isConnectionActive(1004, 1005));
    REQUIRE(network.isConnectionActive(1004, 1006));
    REQUIRE(network.isConnectionActive(1007, 1008));
    REQUIRE(network.isConnectionActive(1007, 1009));
    
    // Connections between partitions should be dropped
    // Partition 1 <-> Partition 2
    REQUIRE_FALSE(network.isConnectionActive(1001, 1004));
    REQUIRE_FALSE(network.isConnectionActive(1002, 1005));
    REQUIRE_FALSE(network.isConnectionActive(1003, 1006));
    
    // Partition 1 <-> Partition 3
    REQUIRE_FALSE(network.isConnectionActive(1001, 1007));
    REQUIRE_FALSE(network.isConnectionActive(1002, 1008));
    REQUIRE_FALSE(network.isConnectionActive(1003, 1009));
    
    // Partition 2 <-> Partition 3
    REQUIRE_FALSE(network.isConnectionActive(1004, 1007));
    REQUIRE_FALSE(network.isConnectionActive(1005, 1008));
    REQUIRE_FALSE(network.isConnectionActive(1006, 1009));
  }
  
  SECTION("assigns correct partition IDs") {
    std::vector<std::vector<uint32_t>> groups = {
      {1001, 1002, 1003},
      {1004, 1005, 1006},
      {1007, 1008, 1009}
    };
    
    NetworkPartitionEvent event(groups);
    event.execute(manager, network);
    
    // Partition 1
    REQUIRE(nodes[0]->getPartitionId() == 1);
    REQUIRE(nodes[1]->getPartitionId() == 1);
    REQUIRE(nodes[2]->getPartitionId() == 1);
    
    // Partition 2
    REQUIRE(nodes[3]->getPartitionId() == 2);
    REQUIRE(nodes[4]->getPartitionId() == 2);
    REQUIRE(nodes[5]->getPartitionId() == 2);
    
    // Partition 3
    REQUIRE(nodes[6]->getPartitionId() == 3);
    REQUIRE(nodes[7]->getPartitionId() == 3);
    REQUIRE(nodes[8]->getPartitionId() == 3);
  }
}

TEST_CASE("NetworkHealEvent", "[event][partition]") {
  NetworkSimulator network;
  boost::asio::io_context io;
  NodeManager manager(io);
  
  SECTION("has descriptive string") {
    NetworkHealEvent event;
    std::string desc = event.getDescription();
    REQUIRE_THAT(desc, Catch::Matchers::ContainsSubstring("Heal"));
  }
  
  SECTION("restores all dropped connections") {
    // Drop some connections manually
    network.dropConnection(1001, 1002);
    network.dropConnection(1002, 1001);
    network.dropConnection(1003, 1004);
    network.dropConnection(1004, 1003);
    
    REQUIRE_FALSE(network.isConnectionActive(1001, 1002));
    REQUIRE_FALSE(network.isConnectionActive(1003, 1004));
    
    // Execute heal
    NetworkHealEvent event;
    event.execute(manager, network);
    
    // All connections should be restored
    REQUIRE(network.isConnectionActive(1001, 1002));
    REQUIRE(network.isConnectionActive(1002, 1001));
    REQUIRE(network.isConnectionActive(1003, 1004));
    REQUIRE(network.isConnectionActive(1004, 1003));
  }
  
  SECTION("resets partition IDs to 0") {
    // Create nodes with partition IDs and unique ports
    NodeConfig config1{1001, "TestMesh", "password", 16401};
    NodeConfig config2{1002, "TestMesh", "password", 16402};
    NodeConfig config3{1003, "TestMesh", "password", 16403};
    
    auto node1 = manager.createNode(config1);
    auto node2 = manager.createNode(config2);
    auto node3 = manager.createNode(config3);
    
    node1->setPartitionId(1);
    node2->setPartitionId(2);
    node3->setPartitionId(3);
    
    REQUIRE(node1->getPartitionId() == 1);
    REQUIRE(node2->getPartitionId() == 2);
    REQUIRE(node3->getPartitionId() == 3);
    
    // Execute heal
    NetworkHealEvent event;
    event.execute(manager, network);
    
    // All partition IDs should be reset to 0
    REQUIRE(node1->getPartitionId() == 0);
    REQUIRE(node2->getPartitionId() == 0);
    REQUIRE(node3->getPartitionId() == 0);
  }
  
  SECTION("works when network is not partitioned") {
    // Create nodes with unique port
    NodeConfig config1{1001, "TestMesh", "password", 16501};
    auto node1 = manager.createNode(config1);
    
    REQUIRE(network.isConnectionActive(1001, 1002));
    REQUIRE(node1->getPartitionId() == 0);
    
    // Execute heal (should be safe)
    NetworkHealEvent event;
    event.execute(manager, network);
    
    REQUIRE(network.isConnectionActive(1001, 1002));
    REQUIRE(node1->getPartitionId() == 0);
  }
}

TEST_CASE("Partition and heal integration", "[event][partition][integration]") {
  NetworkSimulator network(12345);
  boost::asio::io_context io;
  NodeManager manager(io);
  
  // Create 6 nodes with unique ports
  std::vector<std::shared_ptr<VirtualNode>> nodes;
  for (uint32_t i = 1001; i <= 1006; ++i) {
    NodeConfig config{i, "TestMesh", "password", static_cast<uint16_t>(16600 + (i - 1001))};
    nodes.push_back(manager.createNode(config));
  }
  
  SECTION("partition and heal sequence") {
    // Initial state: all connections active, no partitions
    REQUIRE(network.isConnectionActive(1001, 1004));
    REQUIRE(nodes[0]->getPartitionId() == 0);
    
    // Create partition
    std::vector<std::vector<uint32_t>> groups = {
      {1001, 1002, 1003},
      {1004, 1005, 1006}
    };
    NetworkPartitionEvent partition(groups);
    partition.execute(manager, network);
    
    // Verify partition state
    REQUIRE_FALSE(network.isConnectionActive(1001, 1004));
    REQUIRE(nodes[0]->getPartitionId() == 1);
    REQUIRE(nodes[3]->getPartitionId() == 2);
    
    // Heal partition
    NetworkHealEvent heal;
    heal.execute(manager, network);
    
    // Verify healed state
    REQUIRE(network.isConnectionActive(1001, 1004));
    REQUIRE(nodes[0]->getPartitionId() == 0);
    REQUIRE(nodes[3]->getPartitionId() == 0);
  }
  
  SECTION("messages work after heal") {
    uint64_t currentTime = 1000000;
    
    // Create partition
    std::vector<std::vector<uint32_t>> groups = {
      {1001, 1002, 1003},
      {1004, 1005, 1006}
    };
    NetworkPartitionEvent partition(groups);
    partition.execute(manager, network);
    
    // Messages between partitions should be blocked
    network.enqueueMessage(1001, 1004, "blocked", currentTime);
    REQUIRE(network.getPendingMessageCount() == 0);
    
    // Heal partition
    NetworkHealEvent heal;
    heal.execute(manager, network);
    
    // Messages should now work
    network.enqueueMessage(1001, 1004, "works now", currentTime);
    REQUIRE(network.getPendingMessageCount() == 1);
  }
  
  SECTION("multiple partition-heal cycles") {
    uint64_t currentTime = 1000000;
    
    std::vector<std::vector<uint32_t>> groups = {
      {1001, 1002, 1003},
      {1004, 1005, 1006}
    };
    
    // Cycle 1: Partition
    NetworkPartitionEvent partition1(groups);
    partition1.execute(manager, network);
    REQUIRE_FALSE(network.isConnectionActive(1001, 1004));
    REQUIRE(nodes[0]->getPartitionId() == 1);
    
    // Cycle 1: Heal
    NetworkHealEvent heal1;
    heal1.execute(manager, network);
    REQUIRE(network.isConnectionActive(1001, 1004));
    REQUIRE(nodes[0]->getPartitionId() == 0);
    
    // Cycle 2: Partition again
    NetworkPartitionEvent partition2(groups);
    partition2.execute(manager, network);
    REQUIRE_FALSE(network.isConnectionActive(1001, 1004));
    REQUIRE(nodes[0]->getPartitionId() == 1);
    
    // Cycle 2: Heal again
    NetworkHealEvent heal2;
    heal2.execute(manager, network);
    REQUIRE(network.isConnectionActive(1001, 1004));
    REQUIRE(nodes[0]->getPartitionId() == 0);
  }
}

TEST_CASE("Partition event with uneven groups", "[event][partition]") {
  NetworkSimulator network;
  boost::asio::io_context io;
  NodeManager manager(io);
  
  // Create nodes with unique ports
  std::vector<std::shared_ptr<VirtualNode>> nodes;
  for (uint32_t i = 1001; i <= 1010; ++i) {
    NodeConfig config{i, "TestMesh", "password", static_cast<uint16_t>(16700 + (i - 1001))};
    nodes.push_back(manager.createNode(config));
  }
  
  SECTION("handles uneven partition sizes") {
    std::vector<std::vector<uint32_t>> groups = {
      {1001, 1002},                          // 2 nodes
      {1003, 1004, 1005, 1006},              // 4 nodes
      {1007, 1008, 1009, 1010}               // 4 nodes
    };
    
    NetworkPartitionEvent event(groups);
    event.execute(manager, network);
    
    // Verify partition IDs
    REQUIRE(nodes[0]->getPartitionId() == 1);
    REQUIRE(nodes[1]->getPartitionId() == 1);
    REQUIRE(nodes[2]->getPartitionId() == 2);
    REQUIRE(nodes[5]->getPartitionId() == 2);
    REQUIRE(nodes[6]->getPartitionId() == 3);
    REQUIRE(nodes[9]->getPartitionId() == 3);
    
    // Verify connections within groups work
    REQUIRE(network.isConnectionActive(1001, 1002));
    REQUIRE(network.isConnectionActive(1003, 1006));
    REQUIRE(network.isConnectionActive(1007, 1010));
    
    // Verify connections between groups are dropped
    REQUIRE_FALSE(network.isConnectionActive(1001, 1003));
    REQUIRE_FALSE(network.isConnectionActive(1002, 1007));
    REQUIRE_FALSE(network.isConnectionActive(1006, 1010));
  }
  
  SECTION("handles single node partition") {
    std::vector<std::vector<uint32_t>> groups = {
      {1001},                                // 1 node (isolated)
      {1002, 1003, 1004, 1005, 1006, 1007, 1008, 1009, 1010}  // 9 nodes
    };
    
    NetworkPartitionEvent event(groups);
    event.execute(manager, network);
    
    // Single node is partition 1
    REQUIRE(nodes[0]->getPartitionId() == 1);
    
    // All others are partition 2
    for (size_t i = 1; i < nodes.size(); ++i) {
      REQUIRE(nodes[i]->getPartitionId() == 2);
    }
    
    // Isolated node has no connections to others
    REQUIRE_FALSE(network.isConnectionActive(1001, 1002));
    REQUIRE_FALSE(network.isConnectionActive(1001, 1010));
    
    // Others can still communicate
    REQUIRE(network.isConnectionActive(1002, 1010));
  }
}
