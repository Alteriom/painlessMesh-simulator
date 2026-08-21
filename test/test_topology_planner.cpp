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

TEST_CASE("inject_message does not bias the planned topology", "[topology][events]") {
  // An injection traverses the mesh -- often multi-hop, which is the point of a
  // routing scenario. issue_138_message_routing declares a mesh and injects
  // across it; preferring the source/destination as a direct edge would rewire
  // the tree around the probe and hide the routing failures it exists to catch.
  const auto nodes = makeNodes(6);
  TopologyConfig topology;
  topology.type = TopologyType::MESH;

  EventConfig inject;
  inject.time = 10;
  inject.action = EventAction::INJECT_MESSAGE;
  inject.from = "n1";
  inject.to = "n6";

  const auto baseline = planTopology(topology, nodes, {}, 42);
  const auto withInjection = planTopology(topology, nodes, {inject}, 42);

  // Same seed, same declared graph: the injection must change nothing.
  REQUIRE(withInjection.links == baseline.links);
}

TEST_CASE("only link-manipulation events bias the planned topology",
          "[topology][events]") {
  // A connection_drop needs its pair wired to have something to close; an
  // injection does not. The planner must tell them apart.
  const auto nodes = makeNodes(6);
  TopologyConfig topology;
  topology.type = TopologyType::MESH;

  EventConfig inject;
  inject.action = EventAction::INJECT_MESSAGE;
  inject.from = "n1";
  inject.to = "n6";

  EventConfig drop;
  drop.action = EventAction::CONNECTION_DROP;
  drop.from = "n1";
  drop.to = "n6";

  const auto baseline = planTopology(topology, nodes, {}, 42);
  const auto injected = planTopology(topology, nodes, {inject}, 42);
  const auto dropped = planTopology(topology, nodes, {drop}, 42);

  // The drop forces its pair into the tree; the injection leaves the plan
  // identical to the unbiased baseline. (Asserting the injection's pair is
  // *absent* would be seed-dependent -- a random tree may include it anyway --
  // so the meaningful, seed-independent claim is "no influence".)
  REQUIRE(hasLink(dropped.links, 1001, 1006));
  REQUIRE(injected.links == baseline.links);
}

TEST_CASE("a link event's targets:[a,b] syntax is a preferred edge too",
          "[topology][events]") {
  // EventFactory::resolveLink() accepts targets:[a,b] as well as from/to, so
  // the planner must read both -- otherwise a drop declared with the targets
  // syntax loses its preferred edge and the reduction can discard the very pair
  // it means to cut.
  const auto nodes = makeNodes(4);
  TopologyConfig topology;
  topology.type = TopologyType::MESH;

  EventConfig dropViaTargets;
  dropViaTargets.action = EventAction::CONNECTION_DROP;
  dropViaTargets.targets = {"n1", "n4"};  // targets syntax, from/to empty

  EventConfig dropViaFromTo;
  dropViaFromTo.action = EventAction::CONNECTION_DROP;
  dropViaFromTo.from = "n1";
  dropViaFromTo.to = "n4";

  // The two spellings name the same pair, so they must plan the same graph.
  // (Asserting the pair is merely *present* would be seed-dependent -- a random
  // tree may include n1<->n4 anyway -- so this asserts equivalence instead,
  // which fails the moment the targets spelling is ignored.)
  const auto viaTargets = planTopology(topology, nodes, {dropViaTargets}, 42);
  const auto viaFromTo = planTopology(topology, nodes, {dropViaFromTo}, 42);

  REQUIRE(viaTargets.links == viaFromTo.links);
  REQUIRE(hasLink(viaTargets.links, 1001, 1004));
}

TEST_CASE("a random topology still wires an event-named pair it did not draw",
          "[topology][events]") {
  // random draws its edges at random, so a connection_drop pair may be absent
  // from the generated graph. spanningSubset() can only reorder edges already
  // present, so without adding the event pair as a candidate the drop runs
  // against no live link. The pair must end up in the plan regardless of the
  // draw.
  const auto nodes = makeNodes(5);
  TopologyConfig topology;
  topology.type = TopologyType::RANDOM;
  topology.density = 0.0f;  // a bare spanning tree -- most pairs are absent

  EventConfig drop;
  drop.action = EventAction::CONNECTION_DROP;
  drop.from = "n1";
  drop.to = "n5";

  // Try several seeds: for at least one, n1<->n5 is not in the random tree, and
  // the guarantee must hold for every one of them.
  for (uint32_t seed : {1u, 2u, 3u, 7u, 42u, 100u}) {
    const auto plan = planTopology(topology, nodes, {drop}, seed);
    REQUIRE(hasLink(plan.links, 1001, 1005));
    REQUIRE(isConnected(plan.links, nodes.size()));
  }
}

TEST_CASE("an explicit topology is not silently reshaped by an event",
          "[topology][events]") {
  // The random-only merge must NOT apply to star/ring/custom: inserting an
  // event's pair there would change the declared graph (a leaf-to-leaf drop
  // displacing a hub edge) and slip an undeclared edge past restoreLink()'s
  // runtime guard. A star's plan must stay hub-and-spoke regardless of a
  // leaf-to-leaf event.
  const auto nodes = makeNodes(4);
  TopologyConfig topology;
  topology.type = TopologyType::STAR;
  topology.hub = std::string("n1");

  EventConfig leafDrop;
  leafDrop.action = EventAction::CONNECTION_DROP;
  leafDrop.from = "n2";
  leafDrop.to = "n4";  // two leaves; not a star edge

  const auto withEvent = planTopology(topology, nodes, {leafDrop}, 42);
  const auto without = planTopology(topology, nodes, {}, 42);

  REQUIRE(withEvent.links == without.links);            // unchanged
  REQUIRE_FALSE(hasLink(withEvent.links, 1002, 1004));  // leaf-leaf not added
  REQUIRE(hasLink(withEvent.links, 1001, 1002));        // still a spoke
  // ...and the undeclared leaf-to-leaf edge is a fatal planning error, so its
  // drop does not run as a silent no-op.
  REQUIRE(withEvent.unwireable_preferred.size() == 1);
}

TEST_CASE("event links that form a cycle are reported as unwireable",
          "[topology][events]") {
  // Three drop events naming all edges of a 3-node mesh cannot all be wired:
  // the tree holds two, so the third would close a cycle. That third drop would
  // be a silent no-op, so the planner flags it and the run fails.
  const auto nodes = makeNodes(3);
  TopologyConfig topology;
  topology.type = TopologyType::MESH;

  auto drop = [](const std::string& a, const std::string& b) {
    EventConfig e;
    e.action = EventAction::CONNECTION_DROP;
    e.from = a; e.to = b;
    return e;
  };
  const std::vector<EventConfig> events = {
      drop("n1", "n2"), drop("n2", "n3"), drop("n1", "n3")};

  const auto plan = planTopology(topology, nodes, events, 42);

  REQUIRE(plan.links.size() == 2);                  // a tree
  REQUIRE(plan.unwireable_preferred.size() == 1);   // one drop pair could not fit
}

TEST_CASE("Topology planner flags a partition it cannot keep connected",
          "[topology][events]") {
  // A star cannot keep a group that excludes the hub internally connected. That
  // must be recorded in the plan so validation rejects it, not discovered only
  // when the event throws mid-run.
  const auto nodes = makeNodes(4);  // n1 hub
  TopologyConfig topology;
  topology.type = TopologyType::STAR;
  topology.hub = std::string("n1");

  EventConfig part;
  part.action = EventAction::PARTITION_NETWORK;
  part.groups = {{"n1", "n2"}, {"n3", "n4"}};  // n3,n4 have no star edge

  const auto plan = planTopology(topology, nodes, {part}, 42);

  REQUIRE_FALSE(plan.infeasible_partitions.empty());
}

TEST_CASE("Topology planner accepts a partition it can keep connected",
          "[topology][events]") {
  const auto nodes = makeNodes(4);
  TopologyConfig topology;
  topology.type = TopologyType::MESH;

  EventConfig part;
  part.action = EventAction::PARTITION_NETWORK;
  part.groups = {{"n1", "n2"}, {"n3", "n4"}};

  const auto plan = planTopology(topology, nodes, {part}, 42);

  REQUIRE(plan.infeasible_partitions.empty());
}

TEST_CASE("Topology planner keeps partition groups internally connected",
          "[topology][events]") {
  // A network_partition cuts only cross-group links, so each group must be a
  // connected subtree of the plan or the partition fragments it into extra
  // components. Reducing a 4-node mesh to a star around n1 and partitioning
  // [[n1,n2],[n3,n4]] would otherwise leave n3,n4 with no edge between them.
  const auto nodes = makeNodes(4);
  TopologyConfig topology;
  topology.type = TopologyType::MESH;

  EventConfig part;
  part.action = EventAction::PARTITION_NETWORK;
  part.groups = {{"n1", "n2"}, {"n3", "n4"}};

  const auto plan = planTopology(topology, nodes, {part}, 42);

  // Both intra-group edges are in the plan, so each group is connected.
  REQUIRE(hasLink(plan.links, 1001, 1002));
  REQUIRE(hasLink(plan.links, 1003, 1004));
  REQUIRE(isConnected(plan.links, nodes.size()));  // and the whole is a tree
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
