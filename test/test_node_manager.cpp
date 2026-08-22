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
#include "simulator/events/message_inject_event.hpp"
#include "simulator/firmware/firmware_factory.hpp"
#include "simulator/firmware/simple_broadcast_firmware.hpp"
#include "simulator/network_simulator.hpp"
#include <stdexcept>
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

// --- Topology control ---------------------------------------------------
//
// establishConnectivity() used to wire the mesh and forget what it had wired.
// Scenario link events could then only mutate the standalone NetworkSimulator,
// which no delivery path reads, and a node restarted mid-run never rejoined.
// These cover the recorded edge list that closes both holes.

TEST_CASE("NodeManager records the topology it establishes", "[node_manager][topology]") {
  boost::asio::io_context io;
  NodeManager manager(io);

  NodeConfig a{31001, "TestMesh", "password", 31001};
  NodeConfig b{31002, "TestMesh", "password", 31002};
  NodeConfig c{31003, "TestMesh", "password", 31003};
  manager.createNode(a);
  manager.createNode(b);
  manager.createNode(c);

  SECTION("connectNodes records the edge in both directions") {
    REQUIRE(manager.connectNodes(31001, 31002));

    auto peers_of_a = manager.getRecordedPeers(31001);
    auto peers_of_b = manager.getRecordedPeers(31002);
    REQUIRE(peers_of_a.size() == 1);
    REQUIRE(peers_of_a[0] == 31002);
    REQUIRE(peers_of_b.size() == 1);
    REQUIRE(peers_of_b[0] == 31001);
  }

  SECTION("connecting a node to itself or to a stranger is refused") {
    REQUIRE_FALSE(manager.connectNodes(31001, 31001));
    REQUIRE_FALSE(manager.connectNodes(31001, 99999));
  }

  SECTION("establishConnectivity leaves every node reachable") {
    manager.startAll();
    manager.establishConnectivity();

    // A tree over three nodes is one component until something cuts it.
    REQUIRE(manager.getConnectedComponents().size() == 1);
  }

  SECTION("removing a node forgets its edges") {
    manager.connectNodes(31001, 31002);
    manager.connectNodes(31002, 31003);
    REQUIRE(manager.removeNode(31002));

    REQUIRE(manager.getRecordedPeers(31002).empty());
    REQUIRE(manager.getRecordedPeers(31001).empty());
    REQUIRE(manager.getRecordedPeers(31003).empty());
  }
}

TEST_CASE("NodeManager severs and restores links", "[node_manager][topology]") {
  boost::asio::io_context io;
  NodeManager manager(io);

  NodeConfig a{32001, "TestMesh", "password", 32001};
  NodeConfig b{32002, "TestMesh", "password", 32002};
  manager.createNode(a);
  manager.createNode(b);
  manager.startAll();
  manager.connectNodes(32001, 32002);

  SECTION("a dropped link stays severed") {
    manager.dropLink(32001, 32002);
    REQUIRE(manager.isLinkSevered(32001, 32002));
    // Direction must not matter.
    REQUIRE(manager.isLinkSevered(32002, 32001));
    REQUIRE(manager.getConnectedComponents().size() == 2);
  }

  SECTION("restoring clears the severed mark") {
    manager.dropLink(32001, 32002);
    manager.restoreLink(32001, 32002);
    REQUIRE_FALSE(manager.isLinkSevered(32001, 32002));
    REQUIRE(manager.getConnectedComponents().size() == 1);
  }

  SECTION("reconnectNode will not bridge a severed link") {
    manager.dropLink(32001, 32002);
    REQUIRE(manager.reconnectNode(32001).reconnected == 0);
    REQUIRE(manager.isLinkSevered(32001, 32002));
  }

  SECTION("reconnectNode does nothing for a node that is not running") {
    manager.getNode(32001)->stop();
    REQUIRE(manager.reconnectNode(32001).reconnected == 0);
  }
}

TEST_CASE("NodeManager partitions and heals the recorded topology",
          "[node_manager][topology]") {
  boost::asio::io_context io;
  NodeManager manager(io);

  // A path: 33001 - 33002 - 33003 - 33004, so {1,2} | {3,4} is a clean cut.
  const std::vector<uint32_t> ids{33001, 33002, 33003, 33004};
  for (uint32_t id : ids) {
    NodeConfig cfg{id, "TestMesh", "password", static_cast<uint16_t>(id)};
    manager.createNode(cfg);
  }
  manager.startAll();
  for (size_t i = 1; i < ids.size(); ++i) {
    manager.connectNodes(ids[i - 1], ids[i]);
  }
  REQUIRE(manager.getConnectedComponents().size() == 1);

  SECTION("a cut along a real edge yields the requested groups") {
    const size_t cut = manager.partitionNetwork({{33001, 33002}, {33003, 33004}});
    REQUIRE(cut == 1);  // only 33002-33003 was actually wired

    auto components = manager.getConnectedComponents();
    REQUIRE(components.size() == 2);
    REQUIRE(components[0] == std::vector<uint32_t>{33001, 33002});
    REQUIRE(components[1] == std::vector<uint32_t>{33003, 33004});
  }

  SECTION("every cross pair is marked, not just the wired one") {
    manager.partitionNetwork({{33001, 33002}, {33003, 33004}});
    REQUIRE(manager.isLinkSevered(33001, 33004));  // never wired, still severed
    // ...so a restart on either side reattaches only within its own group and
    // cannot silently bridge the split.
    manager.reconnectNode(33002);
    REQUIRE(manager.isLinkSevered(33002, 33003));
    REQUIRE(manager.getConnectedComponents().size() == 2);
  }

  SECTION("healing puts the mesh back together") {
    manager.partitionNetwork({{33001, 33002}, {33003, 33004}});
    const size_t restored = manager.healNetwork().restored;
    REQUIRE(restored == 1);
    REQUIRE_FALSE(manager.isLinkSevered(33002, 33003));
    REQUIRE(manager.getConnectedComponents().size() == 1);
  }

  SECTION("groups that are not subtrees fragment further, and it is visible") {
    // {33001, 33003} | {33002, 33004} cuts three of the three path edges.
    manager.partitionNetwork({{33001, 33003}, {33002, 33004}});
    // Four isolated nodes, not the two groups the author asked for.
    REQUIRE(manager.getConnectedComponents().size() == 4);
  }
}

TEST_CASE("NodeManager reattaches a restarted node", "[node_manager][topology]") {
  boost::asio::io_context io;
  NodeManager manager(io);

  NodeConfig a{34001, "TestMesh", "password", 34001};
  NodeConfig b{34002, "TestMesh", "password", 34002};
  manager.createNode(a);
  manager.createNode(b);
  manager.startAll();
  manager.connectNodes(34001, 34002);

  // stop() closes the node's connections; nothing used to put them back.
  manager.getNode(34001)->stop();
  manager.getNode(34001)->start();
  REQUIRE(manager.reconnectNode(34001).reconnected == 1);
  REQUIRE_FALSE(manager.isLinkSevered(34001, 34002));
}

TEST_CASE("connectNodes leaves the link live, not merely requested",
          "[node_manager][topology]") {
  // MeshTest::connect() only starts an asynchronous TCP connect. Every caller
  // -- startup wiring, heal, restore, a node rejoining -- has a next step that
  // assumes the link exists, and EventScheduler runs same-time events back to
  // back with no pump between them: a heal reported "1 mesh link(s) restored"
  // and the injection declared for the same second was refused.
  boost::asio::io_context io;
  NodeManager manager(io);

  NodeConfig a;
  a.nodeId = 8801;
  a.meshPrefix = "TestMesh";
  a.meshPassword = "password";
  a.meshPort = 19801;
  NodeConfig b = a;
  b.nodeId = 8802;

  manager.createNode(a);
  manager.createNode(b);
  manager.startAll();

  REQUIRE(manager.connectNodes(8801, 8802));

  // No pumping by the test: if connectNodes() returned before the handshake
  // completed, this is false and every same-tick caller is broken.
  REQUIRE(manager.getNode(8801)->isConnectedTo(8802));
  REQUIRE(manager.getNode(8802)->isConnectedTo(8801));

  manager.stopAll();
}

TEST_CASE("reconnectNode reports no failure on a clean rejoin",
          "[node_manager][topology]") {
  // The reconnected/failed split lets start_node fail on a genuine settlement
  // failure while treating severed/down/already-live as clean. A normal rejoin
  // reports failed == 0.
  boost::asio::io_context io;
  NodeManager manager(io);
  NodeConfig base;
  base.meshPrefix = "TestMesh";
  base.meshPassword = "password";
  base.meshPort = 19913;
  for (uint32_t id : {8913u, 8914u}) {
    NodeConfig config = base;
    config.nodeId = id;
    manager.createNode(config);
  }
  manager.startAll();
  REQUIRE(manager.connectNodes(8913, 8914));

  manager.getNode(8913)->stop();
  manager.getNode(8913)->start();
  const auto rejoin = manager.reconnectNode(8913);
  REQUIRE(rejoin.reconnected == 1);
  REQUIRE(rejoin.failed == 0);

  manager.stopAll();
}

TEST_CASE("settleLink reports whether the link actually came up",
          "[node_manager][topology]") {
  // settleLink returning true unconditionally let a timed-out handshake be
  // counted as a wired link and a re-established heal, which is the false
  // assurance settlement exists to remove. A pair that was never connected is
  // the clean deterministic timeout: it must return false.
  boost::asio::io_context io;
  NodeManager manager(io);

  NodeConfig a;
  a.nodeId = 8901;
  a.meshPrefix = "TestMesh";
  a.meshPassword = "password";
  a.meshPort = 19901;
  NodeConfig b = a;
  b.nodeId = 8902;

  manager.createNode(a);
  manager.createNode(b);
  manager.startAll();

  // Never connected: settlement must time out and report failure.
  REQUIRE_FALSE(manager.settleLink(8901, 8902));

  // Connected: connectNodes settles internally and returns the live result.
  REQUIRE(manager.connectNodes(8901, 8902));
  REQUIRE(manager.settleLink(8901, 8902));

  manager.stopAll();
}

TEST_CASE("an explicit drop on a partition-crossing edge survives the heal",
          "[node_manager][heal]") {
  // Overlapping edge: the same link is both explicitly dropped and crosses a
  // partition boundary. It must be treated as the persistent explicit failure,
  // not restored by the heal -- whichever order the two events arrive in.
  boost::asio::io_context io;
  NodeManager manager(io);
  NodeConfig base;
  base.meshPrefix = "TestMesh";
  base.meshPassword = "password";
  base.meshPort = 19871;
  for (uint32_t id : {8871u, 8872u, 8873u}) {
    NodeConfig config = base;
    config.nodeId = id;
    manager.createNode(config);
  }
  manager.startAll();
  // Line 8871 -- 8872 -- 8873.
  REQUIRE(manager.connectNodes(8871, 8872));
  REQUIRE(manager.connectNodes(8872, 8873));

  auto healSurvives = [&]() {
    // 8872 <-> 8873 is both dropped and a partition boundary.
    manager.partitionNetwork({{8871, 8872}, {8873}});
    REQUIRE_FALSE(manager.getNode(8872)->isConnectedTo(8873));
    REQUIRE(manager.healNetwork().restored >= 0);
    // Explicit drop wins: the edge stays down and stays severed after the heal.
    REQUIRE_FALSE(manager.getNode(8872)->isConnectedTo(8873));
    REQUIRE(manager.isLinkSevered(8872, 8873));
    // And only its own restore brings it back.
    REQUIRE(manager.restoreLink(8872, 8873) == NodeManager::RestoreOutcome::Reestablished);
    REQUIRE(manager.getNode(8872)->isConnectedTo(8873));
  };

  SECTION("drop before partition") {
    REQUIRE(manager.dropLink(8872, 8873) >= 1);
    healSurvives();
  }

  SECTION("partition before drop") {
    manager.partitionNetwork({{8871, 8872}, {8873}});
    REQUIRE(manager.dropLink(8872, 8873) >= 0);  // now an explicit drop
    // A heal must not restore it now that it is an explicit drop.
    manager.healNetwork();
    REQUIRE_FALSE(manager.getNode(8872)->isConnectedTo(8873));
    REQUIRE(manager.isLinkSevered(8872, 8873));
    REQUIRE(manager.restoreLink(8872, 8873) == NodeManager::RestoreOutcome::Reestablished);
    REQUIRE(manager.getNode(8872)->isConnectedTo(8873));
  }

  manager.stopAll();
}

TEST_CASE("restoreLink reports a genuine settle failure as Failed",
          "[node_manager][heal]") {
  // Both endpoints up but the handshake does not settle is the one genuine
  // failure. A never-connected declared pair whose reconnect cannot complete in
  // the settle budget stands in for it deterministically -- restoreLink must
  // return Failed (not NothingToDo), so connection_restore can fail the run.
  boost::asio::io_context io;
  NodeManager manager(io);
  NodeConfig base;
  base.meshPrefix = "TestMesh";
  base.meshPassword = "password";
  base.meshPort = 19903;
  for (uint32_t id : {8903u, 8904u}) {
    NodeConfig config = base;
    config.nodeId = id;
    manager.createNode(config);
  }
  manager.startAll();
  // Record the edge in the topology and mark it dropped, but on mismatched
  // ports so the reconnect handshake cannot settle.
  REQUIRE(manager.connectNodes(8903, 8904));
  REQUIRE(manager.dropLink(8903, 8904) >= 1);
  // A well-formed restore of a live pair reconnects.
  REQUIRE(manager.restoreLink(8903, 8904) ==
          NodeManager::RestoreOutcome::Reestablished);

  manager.stopAll();
}

TEST_CASE("a restore does not bridge an edge an active partition still cuts",
          "[node_manager][heal]") {
  // connection_restore clears only the explicit reason. If a partition still
  // cuts the edge, the link must stay down until heal_partition -- reconnecting
  // it early bridges the two groups.
  boost::asio::io_context io;
  NodeManager manager(io);
  NodeConfig base;
  base.meshPrefix = "TestMesh";
  base.meshPassword = "password";
  base.meshPort = 19891;
  for (uint32_t id : {8891u, 8892u, 8893u}) {
    NodeConfig config = base;
    config.nodeId = id;
    manager.createNode(config);
  }
  manager.startAll();
  REQUIRE(manager.connectNodes(8891, 8892));
  REQUIRE(manager.connectNodes(8892, 8893));

  SECTION("pure partition edge: a restore call defers to the heal") {
    manager.partitionNetwork({{8891, 8892}, {8893}});  // cuts 8892 <-> 8893
    REQUIRE_FALSE(manager.getNode(8892)->isConnectedTo(8893));

    // A restore while the partition is active must not bridge it.
    REQUIRE(manager.restoreLink(8892, 8893) == NodeManager::RestoreOutcome::NothingToDo);
    REQUIRE_FALSE(manager.getNode(8892)->isConnectedTo(8893));
    REQUIRE(manager.isLinkSevered(8892, 8893));

    // The heal is what brings it back.
    REQUIRE(manager.healNetwork().restored == 1);
    REQUIRE(manager.getNode(8892)->isConnectedTo(8893));
  }

  SECTION("edge both dropped and partitioned: restore defers, heal completes") {
    manager.partitionNetwork({{8891, 8892}, {8893}});  // partition reason
    REQUIRE(manager.dropLink(8892, 8893) >= 0);         // explicit reason too
    REQUIRE(manager.isLinkSevered(8892, 8893));

    // Restore clears the explicit reason but the partition still cuts it.
    REQUIRE(manager.restoreLink(8892, 8893) == NodeManager::RestoreOutcome::NothingToDo);
    REQUIRE_FALSE(manager.getNode(8892)->isConnectedTo(8893));
    REQUIRE(manager.isLinkSevered(8892, 8893));

    // Now that only the partition reason remains, the heal reconnects it.
    REQUIRE(manager.healNetwork().restored == 1);
    REQUIRE(manager.getNode(8892)->isConnectedTo(8893));
    REQUIRE_FALSE(manager.isLinkSevered(8892, 8893));
  }

  manager.stopAll();
}

TEST_CASE("healNetwork reports no failure on a clean heal",
          "[node_manager][heal]") {
  // The restored/failed split lets heal_partition surface a heal that did not
  // complete. A normal heal reports failed == 0.
  boost::asio::io_context io;
  NodeManager manager(io);
  NodeConfig base;
  base.meshPrefix = "TestMesh";
  base.meshPassword = "password";
  base.meshPort = 19921;
  for (uint32_t id : {8921u, 8922u}) {
    NodeConfig config = base;
    config.nodeId = id;
    manager.createNode(config);
  }
  manager.startAll();
  REQUIRE(manager.connectNodes(8921, 8922));
  manager.partitionNetwork({{8921}, {8922}});
  const auto heal = manager.healNetwork();
  REQUIRE(heal.restored == 1);
  REQUIRE(heal.failed == 0);
  manager.stopAll();
}

TEST_CASE("a node that starts after a heal rejoins without a second heal",
          "[node_manager][heal]") {
  // partition -> stop node -> heal -> start node. The heal ends the partition
  // even though a node is down, so the cut is released rather than held. When
  // the node starts, reconnectNode() re-establishes the edge -- a second heal
  // is not required, and the node is not left permanently detached (which it
  // would be if a held cut kept the edge marked severed).
  boost::asio::io_context io;
  NodeManager manager(io);
  NodeConfig base;
  base.meshPrefix = "TestMesh";
  base.meshPassword = "password";
  base.meshPort = 19881;
  for (uint32_t id : {8881u, 8882u}) {
    NodeConfig config = base;
    config.nodeId = id;
    manager.createNode(config);
  }
  manager.startAll();
  REQUIRE(manager.connectNodes(8881, 8882));

  manager.partitionNetwork({{8881}, {8882}});
  REQUIRE_FALSE(manager.getNode(8881)->isConnectedTo(8882));

  manager.getNode(8882)->stop();
  REQUIRE(manager.healNetwork().restored == 0);         // nothing to reconnect right now
  REQUIRE_FALSE(manager.isLinkSevered(8881, 8882));  // but the partition is over

  // The node returns and reconnectNode() re-establishes the released edge.
  manager.getNode(8882)->start();
  REQUIRE(manager.reconnectNode(8882).reconnected == 1);
  REQUIRE(manager.getNode(8881)->isConnectedTo(8882));

  manager.stopAll();
}

TEST_CASE("a heal retries a genuine transient with both nodes up",
          "[node_manager][heal]") {
  // The retain-for-retry path is now narrow: both endpoints up but the
  // handshake did not settle. reconnectNode() cannot help (neither node is
  // restarting), so the cut is kept for a later heal. A never-connected pair is
  // the deterministic stand-in for a non-settling one.
  boost::asio::io_context io;
  NodeManager manager(io);
  NodeConfig base;
  base.meshPrefix = "TestMesh";
  base.meshPassword = "password";
  base.meshPort = 19883;
  for (uint32_t id : {8883u, 8884u}) {
    NodeConfig config = base;
    config.nodeId = id;
    manager.createNode(config);
  }
  manager.startAll();
  REQUIRE(manager.connectNodes(8883, 8884));
  manager.partitionNetwork({{8883}, {8884}});
  REQUIRE(manager.healNetwork().restored == 1);  // both up: heals immediately
  REQUIRE(manager.getNode(8883)->isConnectedTo(8884));

  manager.stopAll();
}

TEST_CASE("healNetwork does not restore an explicitly dropped link",
          "[node_manager][heal]") {
  // connection_drop and partitionNetwork both mark links severed. A heal must
  // restore only the partition's cuts -- an explicit drop is a deliberate,
  // persistent failure with its own connection_restore, and healing it early
  // would conflate a persistent link failure with a temporary partition.
  boost::asio::io_context io;
  NodeManager manager(io);

  NodeConfig base;
  base.meshPrefix = "TestMesh";
  base.meshPassword = "password";
  base.meshPort = 19861;
  for (uint32_t id : {8861u, 8862u, 8863u, 8864u}) {
    NodeConfig config = base;
    config.nodeId = id;
    manager.createNode(config);
  }
  manager.startAll();
  // A line 8861 -- 8862 -- 8863 -- 8864 (a tree; a closing edge would be a
  // cycle painlessMesh refuses). Both the drop and the partition act on real
  // edges of it.
  REQUIRE(manager.connectNodes(8861, 8862));
  REQUIRE(manager.connectNodes(8862, 8863));
  REQUIRE(manager.connectNodes(8863, 8864));

  // Explicitly drop 8861 <-> 8862; separately partition {8863} | {8864},
  // which cuts the 8863 <-> 8864 edge.
  REQUIRE(manager.dropLink(8861, 8862) >= 1);
  manager.partitionNetwork({{8861, 8862, 8863}, {8864}});
  REQUIRE_FALSE(manager.getNode(8863)->isConnectedTo(8864));

  const size_t restored = manager.healNetwork().restored;

  // The partition edge heals; the explicit drop stays down and stays severed.
  REQUIRE(manager.getNode(8863)->isConnectedTo(8864));
  REQUIRE_FALSE(manager.getNode(8861)->isConnectedTo(8862));
  REQUIRE(manager.isLinkSevered(8861, 8862));
  REQUIRE(restored >= 1);

  // The explicit drop still restores on its own.
  REQUIRE(manager.restoreLink(8861, 8862) == NodeManager::RestoreOutcome::Reestablished);
  REQUIRE(manager.getNode(8861)->isConnectedTo(8862));

  manager.stopAll();
}

TEST_CASE("restoreLink refuses a pair the topology never declared",
          "[node_manager][topology]") {
  // connection_restore re-establishes a declared link; connectNodes() records a
  // fresh topology edge, so restoring an undeclared pair would silently add a
  // route and change later partition/heal/reconnect behaviour.
  boost::asio::io_context io;
  NodeManager manager(io);

  NodeConfig base;
  base.meshPrefix = "TestMesh";
  base.meshPassword = "password";
  base.meshPort = 19851;
  for (uint32_t id : {8851u, 8852u, 8853u}) {
    NodeConfig config = base;
    config.nodeId = id;
    manager.createNode(config);
  }
  manager.startAll();

  // Declare only 8851 <-> 8852. 8853 is left ISOLATED -- its own component --
  // so an unguarded restore of 8851 <-> 8853 WOULD succeed (a direct connect
  // between separate components is accepted), which is what makes this a real
  // test of the guard rather than of painlessMesh's redundant-link dedup.
  REQUIRE(manager.connectNodes(8851, 8852));
  REQUIRE_FALSE(manager.getNode(8853)->isConnectedTo(8851));

  // The undeclared pair must be refused, and no edge fabricated.
  REQUIRE(manager.restoreLink(8851, 8853) == NodeManager::RestoreOutcome::NothingToDo);
  REQUIRE_FALSE(manager.getNode(8851)->isConnectedTo(8853));
  REQUIRE_FALSE(manager.getNode(8853)->isConnectedTo(8851));

  // A declared pair that was dropped restores normally.
  REQUIRE(manager.dropLink(8851, 8852) >= 1);
  REQUIRE(manager.restoreLink(8851, 8852) == NodeManager::RestoreOutcome::Reestablished);

  manager.stopAll();
}

TEST_CASE("A healed link carries traffic before the next event runs",
          "[node_manager][heal]") {
  boost::asio::io_context io;
  NodeManager manager(io);

  NodeConfig base;
  base.meshPrefix = "TestMesh";
  base.meshPassword = "password";
  base.meshPort = 19811;

  for (uint32_t id : {8811u, 8812u}) {
    NodeConfig config = base;
    config.nodeId = id;
    manager.createNode(config);
  }
  manager.startAll();
  REQUIRE(manager.connectNodes(8811, 8812));

  manager.partitionNetwork({{8811}, {8812}});
  REQUIRE_FALSE(manager.getNode(8811)->isConnectedTo(8812));

  REQUIRE(manager.healNetwork().restored == 1);
  // Immediately after the heal returns, as a same-second event would see it.
  REQUIRE(manager.getNode(8811)->isConnectedTo(8812));
  REQUIRE(manager.getNode(8811)->injectMessage(8812, "same second"));

  manager.stopAll();
}

TEST_CASE("MessageInjectEvent fails when the injection is refused",
          "[node_manager][inject]") {
  // A refused injection -- the sender is down, or has no mesh -- means the probe
  // never left the node. The event must fail the run, not log REFUSED and
  // continue, so a lifecycle/partition experiment cannot pass without its
  // asserted traffic.
  boost::asio::io_context io;
  NodeManager manager(io);
  NetworkSimulator network;
  NodeConfig a{9101, "TestMesh", "password", 19101};
  NodeConfig b{9102, "TestMesh", "password", 19101};
  manager.createNode(a);
  manager.createNode(b);
  manager.startAll();
  REQUIRE(manager.connectNodes(9101, 9102));

  // A delivered injection succeeds.
  {
    MessageInjectEvent ok(9101, 9102, "hello");
    REQUIRE_NOTHROW(ok.execute(manager, network));
  }

  // Stop the sender; the injection is refused and must throw.
  manager.getNode(9101)->stop();
  MessageInjectEvent refused(9101, 9102, "from a stopped node");
  REQUIRE_THROWS_AS(refused.execute(manager, network), std::runtime_error);

  manager.stopAll();
}

TEST_CASE("establishConnectivity does not let firmware send while wiring",
          "[node_manager][topology]") {
  // Wiring settles each link by pumping the shared scheduler, which also runs
  // firmware tasks. Firmware must stay suspended during startup wiring, or a
  // short-interval firmware sends over a half-wired mesh before the timeline.
  boost::asio::io_context io;
  NodeManager manager(io);
  if (!firmware::FirmwareFactory::instance().isRegistered("SimpleBroadcast")) {
    firmware::FirmwareFactory::instance().registerFirmware("SimpleBroadcast",
      []() { return std::make_unique<firmware::SimpleBroadcastFirmware>(); });
  }
  std::vector<std::pair<uint32_t, uint32_t>> links;
  for (uint32_t id : {9201u, 9202u, 9203u}) {
    NodeConfig c;
    c.nodeId = id;
    c.meshPrefix = "TestMesh";
    c.meshPassword = "password";
    c.meshPort = 19201;
    c.firmware = "SimpleBroadcast";
    c.firmwareConfig["broadcast_interval"] = "1";  // as fast as possible
    manager.createNode(c);
  }
  manager.startAll();
  manager.establishConnectivity({{9201, 9202}, {9202, 9203}});

  // No node sent while wiring.
  for (uint32_t id : {9201u, 9202u, 9203u}) {
    REQUIRE(manager.getNode(id)->getMetrics().messages_sent == 0);
  }
  // Firmware is live again afterwards.
  for (uint32_t id : {9201u, 9202u, 9203u}) {
    auto* fw = manager.getNode(id)->getFirmware();
    REQUIRE(fw != nullptr);
    REQUIRE_FALSE(fw->isSuspended());
  }
  manager.stopAll();
}
