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
#include <limits>

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

  SECTION("a non-finite or out-of-range density does not crash the planner") {
    // Validation rejects such a density and the run returns 2, but planTopology
    // also runs during the validation sweep, so it is reached with the bad value
    // still in hand. static_cast<size_t> of a NaN/inf/negative product is UB;
    // the density clamps to the [0, 1] fraction it is defined to be. (finding 74)
    //
    // This is a success-path/totality guard: the RANDOM plan reduces to a
    // spanning tree regardless of density, and float->size_t for a non-finite
    // value is benign on this platform, so the UB is not a deterministic failure
    // to assert against here. The check pins that planTopology stays total --
    // never crashes, never returns a wild size -- for any density a caller hands
    // it, which is the guarantee the clamp exists to provide.
    for (float bad : {std::numeric_limits<float>::quiet_NaN(),
                      std::numeric_limits<float>::infinity(),
                      -std::numeric_limits<float>::infinity(),
                      -1.0f}) {
      TopologyConfig topology;
      topology.type = TopologyType::RANDOM;
      topology.density = bad;
      const auto plan = planTopology(topology, nodes, noEvents, 42);
      // Clamped toward 0 -> just the spanning tree, never a crash or a wild size.
      REQUIRE(plan.links.size() == nodes.size() - 1);
      REQUIRE(isConnected(plan.links, nodes.size()));
    }

    // A density above 1 clamps to a full graph, not something larger.
    TopologyConfig dense;
    dense.type = TopologyType::RANDOM;
    dense.density = 9.0f;
    const auto plan = planTopology(dense, nodes, noEvents, 42);
    REQUIRE(plan.links.size() <= nodes.size() * (nodes.size() - 1) / 2);
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

TEST_CASE("Topology planner seeds partition solving with hard-preferred links",
          "[topology][events]") {
  // A link event on n2-n3 is force-kept by spanningSubset ahead of the group
  // edges. If the solver ignores it, it picks n1-n3 which the link then
  // displaces, wrongly failing the [[n0,n1,n3],[n2]] partition. Seeding the
  // search with n2-n3 makes it build the compatible tree n2-n3, n1-n3, n0-n1.
  const auto nodes = makeNodes(4);  // n1..n4 -> n0..n3
  TopologyConfig topology;
  topology.type = TopologyType::MESH;

  EventConfig drop;  // hard-preferred link on n3-n4 (i.e. n2-n3)
  drop.action = EventAction::CONNECTION_DROP;
  drop.from = "n3";
  drop.to = "n4";

  EventConfig p1;
  p1.action = EventAction::PARTITION_NETWORK;
  p1.groups = {{"n1"}, {"n2", "n3", "n4"}};

  EventConfig p2;
  p2.action = EventAction::PARTITION_NETWORK;
  p2.groups = {{"n1", "n2", "n4"}, {"n3"}};

  const auto plan = planTopology(topology, nodes, {drop, p1, p2}, 42);

  REQUIRE(plan.infeasible_partitions.empty());
  REQUIRE(plan.unwireable_preferred.empty());
  REQUIRE(hasLink(plan.links, 1003, 1004));  // the link-event pair survives
  REQUIRE(isConnected(plan.links, nodes.size()));
}

TEST_CASE("Topology planner backtracks over intra-group edge choices",
          "[topology][events]") {
  // [[n0,n2,n3],[n1]] and [[n1,n2,n3],[n0]] fail any greedy edge pick (0-2,1-2
  // then 0-3/1-3), but 0-2,1-2,2-3 satisfies both. Only backtracking over WHICH
  // intra-group edge to keep finds it.
  const auto nodes = makeNodes(4);  // n1..n4 -> n0..n3
  TopologyConfig topology;
  topology.type = TopologyType::MESH;

  EventConfig p1;
  p1.action = EventAction::PARTITION_NETWORK;
  p1.groups = {{"n1", "n3", "n4"}, {"n2"}};

  EventConfig p2;
  p2.action = EventAction::PARTITION_NETWORK;
  p2.groups = {{"n2", "n3", "n4"}, {"n1"}};

  const auto plan = planTopology(topology, nodes, {p1, p2}, 42);

  REQUIRE(plan.infeasible_partitions.empty());
  REQUIRE(isConnected(plan.links, nodes.size()));
}

TEST_CASE("Topology planner measures induced, not global, group connectivity",
          "[topology][events]") {
  // Groups [[n0,n1,n3],[n2]] and [[n1,n2,n3],[n0]]. If group connectivity is
  // read from the global union-find, the second group looks connected via the
  // external node n0 and the solver stops early, so the final induced check
  // rejects it. The tree n0-n1, n1-n2, n1-n3 satisfies both; the planner must
  // find it.
  const auto nodes = makeNodes(4);  // n1..n4 -> treat as n0..n3
  TopologyConfig topology;
  topology.type = TopologyType::MESH;

  EventConfig p1;
  p1.action = EventAction::PARTITION_NETWORK;
  p1.groups = {{"n1", "n2", "n4"}, {"n3"}};

  EventConfig p2;
  p2.action = EventAction::PARTITION_NETWORK;
  p2.groups = {{"n2", "n3", "n4"}, {"n1"}};

  const auto plan = planTopology(topology, nodes, {p1, p2}, 42);

  REQUIRE(plan.infeasible_partitions.empty());
  REQUIRE(isConnected(plan.links, nodes.size()));
}

TEST_CASE("Topology planner solves overlapping groups jointly, not greedily",
          "[topology][events]") {
  // Harder overlap: [[z,x,y],[w]] then [[x,y],[z,w]] on a 4-node mesh. A greedy
  // clique keeps z-x, z-y and rejects x-y as cyclic, wrongly flagging [x,y]
  // infeasible; the tree z-x, x-y, z-w satisfies both. The joint solver must
  // find it.
  const auto nodes = makeNodes(4);  // n1=z, n2=x, n3=y, n4=w
  TopologyConfig topology;
  topology.type = TopologyType::MESH;

  EventConfig p1;
  p1.action = EventAction::PARTITION_NETWORK;
  p1.groups = {{"n1", "n2", "n3"}, {"n4"}};

  EventConfig p2;
  p2.action = EventAction::PARTITION_NETWORK;
  p2.groups = {{"n2", "n3"}, {"n1", "n4"}};

  const auto plan = planTopology(topology, nodes, {p1, p2}, 42);

  REQUIRE(plan.infeasible_partitions.empty());
  REQUIRE(isConnected(plan.links, nodes.size()));
}

TEST_CASE("Topology planner keeps overlapping partition groups connected",
          "[topology][events]") {
  // Reviewer's case: a 4-node mesh with two partition events whose groups
  // overlap -- [n1,n2,n3] and [n1,n3,n4]. A per-group path is greedy and would
  // reject this, but the tree 1-2, 1-3, 3-4 keeps both groups connected. The
  // planner must find it, not report infeasible.
  const auto nodes = makeNodes(4);
  TopologyConfig topology;
  topology.type = TopologyType::MESH;

  EventConfig p1;
  p1.action = EventAction::PARTITION_NETWORK;
  p1.groups = {{"n1", "n2", "n3"}, {"n4"}};

  EventConfig p2;
  p2.action = EventAction::PARTITION_NETWORK;
  p2.groups = {{"n1", "n3", "n4"}, {"n2"}};

  const auto plan = planTopology(topology, nodes, {p1, p2}, 42);

  REQUIRE(plan.infeasible_partitions.empty());
  REQUIRE(isConnected(plan.links, nodes.size()));
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

TEST_CASE("validateEventTimeline rejects sequences that must fail at runtime",
          "[topology_planner][timeline]") {
  // planTopology() checks static feasibility; this walks the lifecycle so a
  // sequence broken by an earlier event is rejected at --validate-only, not at
  // run time. (finding 79)
  const auto nodes = makeNodes(3);  // n1=1001, n2=1002, n3=1003
  // A line n1 - n2 - n3.
  const std::vector<PlannedLink> line{{1001, 1002}, {1002, 1003}};

  auto stopEvent = [](uint32_t t, const std::string& target) {
    EventConfig e; e.time = t; e.action = EventAction::STOP_NODE; e.target = target;
    return e;
  };

  SECTION("a partition whose group a prior stop_node split") {
    EventConfig part; part.time = 10; part.action = EventAction::PARTITION_NETWORK;
    part.groups = {{"n1", "n2"}, {"n3"}};  // group {n1,n2} needs n2, which is down
    const auto problems =
        validateEventTimeline({stopEvent(5, "n2"), part}, line, nodes);
    REQUIRE(problems.size() == 1);
    REQUIRE(problems[0].find("network_partition") != std::string::npos);
  }

  SECTION("an injection from a sender stopped earlier") {
    EventConfig inj; inj.time = 10; inj.action = EventAction::INJECT_MESSAGE;
    inj.from = "n1"; inj.to = "n3";
    const auto problems =
        validateEventTimeline({stopEvent(5, "n1"), inj}, line, nodes);
    REQUIRE(problems.size() == 1);
    REQUIRE(problems[0].find("inject_message") != std::string::npos);
  }

  SECTION("a partition after stopping a leaf is fine") {
    // Stop n1 (a leaf); partition [[n2,n3],[n1]] still yields two components:
    // the stopped n1 as its own, and the connected {n2,n3}.
    EventConfig part; part.time = 10; part.action = EventAction::PARTITION_NETWORK;
    part.groups = {{"n2", "n3"}, {"n1"}};
    const auto problems =
        validateEventTimeline({stopEvent(5, "n1"), part}, line, nodes);
    REQUIRE(problems.empty());
  }

  SECTION("an injection from a running sender is fine") {
    EventConfig inj; inj.time = 10; inj.action = EventAction::INJECT_MESSAGE;
    inj.from = "n1"; inj.to = "n3";
    const auto problems = validateEventTimeline({inj}, line, nodes);
    REQUIRE(problems.empty());
  }

  SECTION("a restart that brings the node back before the partition is fine") {
    EventConfig restart; restart.time = 5; restart.action = EventAction::RESTART_NODE;
    restart.target = "n2"; restart.delay = 2;  // down 5..7, up by t=7
    EventConfig part; part.time = 10; part.action = EventAction::PARTITION_NETWORK;
    part.groups = {{"n1", "n2"}, {"n3"}};  // n2 is running again at t=10
    const auto problems = validateEventTimeline({restart, part}, line, nodes);
    REQUIRE(problems.empty());
  }

  SECTION("an unmodelled action suspends the checks that depend on it") {
    // start_all_nodes parses as UNKNOWN; it could have restarted n2, so the
    // component count the partition needs is no longer knowable and must not
    // be guessed at (the unknown action fails validation on its own).
    EventConfig unknown; unknown.time = 6; unknown.action = EventAction::UNKNOWN;
    unknown.action_raw = "start_all_nodes";
    EventConfig part; part.time = 10; part.action = EventAction::PARTITION_NETWORK;
    part.groups = {{"n1", "n2"}, {"n3"}};
    const auto problems =
        validateEventTimeline({stopEvent(5, "n2"), unknown, part}, line, nodes);
    REQUIRE(problems.empty());
  }

  SECTION("a broken step before an unmodelled action is still reported") {
    // The walker used to scan the whole list up front and return on any
    // unmodelled action, discarding even the steps that ran before it with
    // fully known state. Those are deterministic failures and must survive.
    // (issue 63)
    EventConfig inj; inj.time = 10; inj.action = EventAction::INJECT_MESSAGE;
    inj.from = "n1"; inj.to = "n3";
    EventConfig unknown; unknown.time = 100; unknown.action = EventAction::UNKNOWN;
    unknown.action_raw = "start_all_nodes";
    const auto problems =
        validateEventTimeline({stopEvent(5, "n1"), inj, unknown}, line, nodes);
    REQUIRE(problems.size() == 1);
    REQUIRE(problems[0].find("inject_message") != std::string::npos);
  }

  SECTION("an explicit event after an unmodelled one makes its target known again") {
    // start_all_nodes leaves every node's running state unknown, but the later
    // stop_node names n1 outright, so the injection after it is once again a
    // certain failure.
    EventConfig unknown; unknown.time = 5; unknown.action = EventAction::UNKNOWN;
    unknown.action_raw = "start_all_nodes";
    EventConfig inj; inj.time = 20; inj.action = EventAction::INJECT_MESSAGE;
    inj.from = "n1"; inj.to = "n3";
    const auto problems =
        validateEventTimeline({unknown, stopEvent(10, "n1"), inj}, line, nodes);
    REQUIRE(problems.size() == 1);
    REQUIRE(problems[0].find("inject_message") != std::string::npos);
  }

  SECTION("an unmodelled action clears a stop the walker had recorded") {
    // The mirror image: start_all_nodes may have brought n1 back, so the walker
    // must not claim the later injection fails.
    EventConfig unknown; unknown.time = 10; unknown.action = EventAction::UNKNOWN;
    unknown.action_raw = "start_all_nodes";
    EventConfig inj; inj.time = 20; inj.action = EventAction::INJECT_MESSAGE;
    inj.from = "n1"; inj.to = "n3";
    const auto problems =
        validateEventTimeline({stopEvent(5, "n1"), unknown, inj}, line, nodes);
    REQUIRE(problems.empty());
  }

  SECTION("add_nodes suspends the component count but not the injection check") {
    // add_nodes changes the node set out from under componentCount(), so the
    // partition -- which a prior stop_node would otherwise break -- is not
    // judged. An injection from a node an explicit stop_node named afterwards
    // still is, so exactly one problem comes back rather than none or two.
    EventConfig add; add.time = 10; add.action = EventAction::ADD_NODES;
    EventConfig part; part.time = 30; part.action = EventAction::PARTITION_NETWORK;
    part.groups = {{"n1", "n2"}, {"n3"}};  // n2 is down: 3 components, not 2
    EventConfig inj; inj.time = 40; inj.action = EventAction::INJECT_MESSAGE;
    inj.from = "n1"; inj.to = "n3";
    const auto problems = validateEventTimeline(
        {stopEvent(5, "n2"), add, part, stopEvent(35, "n1"), inj}, line, nodes);
    REQUIRE(problems.size() == 1);
    REQUIRE(problems[0].find("inject_message") != std::string::npos);
  }
}

TEST_CASE("validateEventTimeline checks injection reachability, not just the sender",
          "[topology_planner][timeline]") {
  // A running sender is only half of what the runtime needs. sendSingle() asks
  // findRoute() for a connection whose subtree holds the destination and
  // sendBroadcast() counts the peers it managed to enqueue to; either returning
  // false makes MessageInjectEvent throw, so the run exits 1. Earlier topology
  // events are what take those routes away. (issue 62)
  const auto nodes = makeNodes(3);  // n1=1001, n2=1002, n3=1003
  const std::vector<PlannedLink> line{{1001, 1002}, {1002, 1003}};

  auto injectEvent = [](uint32_t t, const std::string& from, const std::string& to) {
    EventConfig e; e.time = t; e.action = EventAction::INJECT_MESSAGE;
    e.from = from; e.to = to;
    return e;
  };
  auto dropEvent = [](uint32_t t, const std::string& a, const std::string& b) {
    EventConfig e; e.time = t; e.action = EventAction::CONNECTION_DROP;
    e.from = a; e.to = b;
    return e;
  };

  SECTION("a destination cut off by an earlier connection_drop") {
    // The issue's own case: drop B-C on a line A-B-C, then send A -> C.
    const auto problems = validateEventTimeline(
        {dropEvent(5, "n2", "n3"), injectEvent(10, "n1", "n3")}, line, nodes);
    REQUIRE(problems.size() == 1);
    REQUIRE(problems[0].find("inject_message") != std::string::npos);
    REQUIRE(problems[0].find("unreachable") != std::string::npos);
  }

  SECTION("a destination the same drop still leaves reachable") {
    // n1 -> n2 survives the n2-n3 drop; only the far side is cut off.
    const auto problems = validateEventTimeline(
        {dropEvent(5, "n2", "n3"), injectEvent(10, "n1", "n2")}, line, nodes);
    REQUIRE(problems.empty());
  }

  SECTION("a connection_restore before the injection puts the route back") {
    EventConfig restore; restore.time = 8;
    restore.action = EventAction::CONNECTION_RESTORE;
    restore.from = "n2"; restore.to = "n3";
    const auto problems = validateEventTimeline(
        {dropEvent(5, "n2", "n3"), restore, injectEvent(10, "n1", "n3")},
        line, nodes);
    REQUIRE(problems.empty());
  }

  SECTION("a destination stranded by a stopped relay") {
    // n2 is the only path between n1 and n3, so stopping it strands n3 even
    // though both endpoints of the injection are running.
    EventConfig stop; stop.time = 5; stop.action = EventAction::STOP_NODE;
    stop.target = "n2";
    const auto problems =
        validateEventTimeline({stop, injectEvent(10, "n1", "n3")}, line, nodes);
    REQUIRE(problems.size() == 1);
    REQUIRE(problems[0].find("unreachable") != std::string::npos);
  }

  SECTION("a destination on the far side of an active partition") {
    EventConfig part; part.time = 10; part.action = EventAction::PARTITION_NETWORK;
    part.groups = {{"n1", "n2"}, {"n3"}};
    const auto problems =
        validateEventTimeline({part, injectEvent(20, "n1", "n3")}, line, nodes);
    REQUIRE(problems.size() == 1);
    REQUIRE(problems[0].find("unreachable") != std::string::npos);
  }

  SECTION("the same partition healed before the injection") {
    EventConfig part; part.time = 10; part.action = EventAction::PARTITION_NETWORK;
    part.groups = {{"n1", "n2"}, {"n3"}};
    EventConfig heal; heal.time = 20; heal.action = EventAction::HEAL_PARTITION;
    const auto problems = validateEventTimeline(
        {part, heal, injectEvent(20, "n1", "n3")}, line, nodes);
    REQUIRE(problems.empty());
  }

  SECTION("a broadcast from a sender left with no live peer") {
    // sendBroadcast() enqueues to direct connections only and returns false
    // when it reaches none, so a sender with every incident link down fails
    // even though the rest of the mesh is intact.
    const auto problems = validateEventTimeline(
        {dropEvent(5, "n1", "n2"), injectEvent(10, "n1", "")}, line, nodes);
    REQUIRE(problems.size() == 1);
    REQUIRE(problems[0].find("no live peer") != std::string::npos);
  }

  SECTION("a broadcast spelled 'broadcast' is the same check") {
    const auto problems = validateEventTimeline(
        {dropEvent(5, "n1", "n2"), injectEvent(10, "n1", "broadcast")},
        line, nodes);
    REQUIRE(problems.size() == 1);
    REQUIRE(problems[0].find("no live peer") != std::string::npos);
  }

  SECTION("a broadcast from a sender that still has one peer is fine") {
    // n2 keeps n1 as a peer after the n2-n3 drop; a broadcast only needs one.
    const auto problems = validateEventTimeline(
        {dropEvent(5, "n2", "n3"), injectEvent(10, "n2", "broadcast")},
        line, nodes);
    REQUIRE(problems.empty());
  }

  SECTION("a stopped sender is reported once, not twice") {
    // The sender being down already refuses the injection; adding a second
    // problem for the destination it therefore cannot reach is noise.
    EventConfig stop; stop.time = 5; stop.action = EventAction::STOP_NODE;
    stop.target = "n1";
    const auto problems =
        validateEventTimeline({stop, injectEvent(10, "n1", "n3")}, line, nodes);
    REQUIRE(problems.size() == 1);
    REQUIRE(problems[0].find("is stopped at that point") != std::string::npos);
  }

  SECTION("the sender spelled as 'target', the way EventFactory accepts it") {
    EventConfig inj; inj.time = 10; inj.action = EventAction::INJECT_MESSAGE;
    inj.target = "n1"; inj.to = "n3";  // `target` is EventFactory's sender alias
    const auto problems =
        validateEventTimeline({dropEvent(5, "n2", "n3"), inj}, line, nodes);
    REQUIRE(problems.size() == 1);
    REQUIRE(problems[0].find("unreachable") != std::string::npos);
  }

  SECTION("an unmodelled action suspends the reachability check") {
    // start_all_nodes could relink anything, so the walker must not claim the
    // destination is unreachable -- the same rule the component count follows.
    EventConfig unknown; unknown.time = 7; unknown.action = EventAction::UNKNOWN;
    unknown.action_raw = "start_all_nodes";
    const auto problems = validateEventTimeline(
        {dropEvent(5, "n2", "n3"), unknown, injectEvent(10, "n1", "n3")},
        line, nodes);
    REQUIRE(problems.empty());
  }

  SECTION("an unwired topology is not judged") {
    // With no planned links there is no adjacency to reason over; every
    // destination would look unreachable.
    const auto problems =
        validateEventTimeline({injectEvent(10, "n1", "n3")}, {}, nodes);
    REQUIRE(problems.empty());
  }
}
