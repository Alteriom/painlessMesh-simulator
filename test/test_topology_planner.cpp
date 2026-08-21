/**
 * @file test_topology_planner.cpp
 * @brief Unit tests for the scenario topology planner
 *
 * `topology:` was parsed and validated but never applied -- every run got a
 * random spanning tree instead. These tests pin the mapping from declaration
 * to wired links, including the reduction to what painlessMesh will actually
 * hold.
 *
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include <catch2/catch_test_macros.hpp>
#include "simulator/topology_planner.hpp"

#include <algorithm>

using namespace simulator;

namespace {

std::vector<NodeConfigExtended> makeNodes(size_t count) {
  std::vector<NodeConfigExtended> nodes;
  for (size_t i = 1; i <= count; ++i) {
    NodeConfigExtended node;
    node.id = "n" + std::to_string(i);
    node.nodeId = static_cast<uint32_t>(1000 + i);
    nodes.push_back(node);
  }
  return nodes;
}

bool hasLink(const std::vector<PlannedLink>& links, uint32_t a, uint32_t b) {
  return std::any_of(links.begin(), links.end(), [&](const PlannedLink& l) {
    return (l.first == a && l.second == b) || (l.first == b && l.second == a);
  });
}

/// Every node reachable from the first, over the planned links
bool isConnected(const std::vector<PlannedLink>& links, size_t nodeCount) {
  if (nodeCount < 2) return true;
  std::set<uint32_t> seen{1001};
  bool grew = true;
  while (grew) {
    grew = false;
    for (const auto& link : links) {
      if (seen.count(link.first) && !seen.count(link.second)) {
        seen.insert(link.second);
        grew = true;
      } else if (seen.count(link.second) && !seen.count(link.first)) {
        seen.insert(link.first);
        grew = true;
      }
    }
  }
  return seen.size() == nodeCount;
}

}  // namespace

TEST_CASE("Topology planner reduces a declared graph to what the mesh holds",
          "[topology]") {
  const auto nodes = makeNodes(4);
  const std::vector<EventConfig> noEvents;

  SECTION("mesh declares every pair but plans a spanning tree") {
    TopologyConfig topology;
    topology.type = TopologyType::MESH;

    const auto plan = planTopology(topology, nodes, noEvents, 42);

    // Wiring all 6 at once was measured to leave 0 live links and no traffic:
    // painlessMesh tears the mesh down rather than pruning under a burst.
    REQUIRE(plan.declared == 6);
    REQUIRE(plan.links.size() == 3);
    REQUIRE(isConnected(plan.links, nodes.size()));
    REQUIRE_FALSE(plan.warnings.empty());
  }

  SECTION("star wires the declared hub, not an arbitrary node") {
    TopologyConfig topology;
    topology.type = TopologyType::STAR;
    topology.hub = std::string("n3");

    const auto plan = planTopology(topology, nodes, noEvents, 42);

    REQUIRE(plan.links.size() == 3);
    REQUIRE(hasLink(plan.links, 1003, 1001));
    REQUIRE(hasLink(plan.links, 1003, 1002));
    REQUIRE(hasLink(plan.links, 1003, 1004));
    REQUIRE(plan.warnings.empty());
  }

  SECTION("a missing hub is reported, not silently substituted") {
    TopologyConfig topology;
    topology.type = TopologyType::STAR;
    topology.hub = std::string("nowhere");

    const auto plan = planTopology(topology, nodes, noEvents, 42);

    REQUIRE(plan.links.size() == 3);
    REQUIRE(plan.warnings.size() == 1);
  }

  SECTION("custom keeps exactly the declared shape when it is a tree") {
    TopologyConfig topology;
    topology.type = TopologyType::CUSTOM;
    topology.connections = {{"n1", "n2"}, {"n2", "n3"}, {"n3", "n4"}};

    const auto plan = planTopology(topology, nodes, noEvents, 42);

    REQUIRE(plan.declared == 3);
    REQUIRE(plan.links.size() == 3);
    REQUIRE(hasLink(plan.links, 1001, 1002));
    REQUIRE(hasLink(plan.links, 1002, 1003));
    REQUIRE(hasLink(plan.links, 1003, 1004));
  }

  SECTION("custom naming an unknown node warns and skips that link") {
    TopologyConfig topology;
    topology.type = TopologyType::CUSTOM;
    topology.connections = {{"n1", "n2"}, {"n2", "ghost"}};

    const auto plan = planTopology(topology, nodes, noEvents, 42);

    REQUIRE(plan.links.size() == 1);
    REQUIRE(plan.warnings.size() == 1);
  }

  SECTION("ring drops the closing link that would make a cycle") {
    TopologyConfig topology;
    topology.type = TopologyType::RING;

    const auto plan = planTopology(topology, nodes, noEvents, 42);

    REQUIRE(plan.declared == 4);
    REQUIRE(plan.links.size() == 3);
    REQUIRE(isConnected(plan.links, nodes.size()));
  }

  SECTION("random stays connected regardless of density") {
    TopologyConfig topology;
    topology.type = TopologyType::RANDOM;
    topology.density = 0.0f;

    const auto plan = planTopology(topology, nodes, noEvents, 12345);

    REQUIRE(plan.links.size() == 3);
    REQUIRE(isConnected(plan.links, nodes.size()));
  }

  SECTION("the same seed plans the same graph") {
    TopologyConfig topology;
    topology.type = TopologyType::RANDOM;
    topology.density = 0.5f;

    const auto first = planTopology(topology, nodes, noEvents, 777);
    const auto second = planTopology(topology, nodes, noEvents, 777);

    REQUIRE(first.links == second.links);
  }

  SECTION("fewer than two nodes plans nothing") {
    TopologyConfig topology;
    topology.type = TopologyType::MESH;

    const auto plan = planTopology(topology, makeNodes(1), noEvents, 42);

    REQUIRE(plan.links.empty());
  }
}

TEST_CASE("Topology planner keeps the links a scenario's events name",
          "[topology][events]") {
  const auto nodes = makeNodes(4);
  TopologyConfig topology;
  topology.type = TopologyType::MESH;

  // connection_events_test.yaml is this shape: a full mesh declared, then a
  // named pair dropped. Before the planner preferred event pairs, that drop
  // reported "0 live endpoint(s) closed" because the random tree never wired
  // the pair.
  EventConfig drop;
  drop.time = 20;
  drop.action = EventAction::CONNECTION_DROP;
  drop.from = "n1";
  drop.to = "n4";

  const auto plan = planTopology(topology, nodes, {drop}, 42);

  REQUIRE(plan.links.size() == 3);
  REQUIRE(hasLink(plan.links, 1001, 1004));
  REQUIRE(isConnected(plan.links, nodes.size()));
}
