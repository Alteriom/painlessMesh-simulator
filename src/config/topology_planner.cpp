/**
 * @file topology_planner.cpp
 * @brief Implementation of the scenario topology planner
 *
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include "simulator/topology_planner.hpp"

#include <algorithm>
#include <map>
#include <random>
#include <string>

namespace simulator {

namespace {

/// Deterministic fallback for a zero seed reaching this pure function directly
/// (e.g. a unit test). The run's entry point resolves simulation.seed == 0 to a
/// real random seed before calling, honouring SimulationConfig's "0 = random"
/// contract; this keeps planTopology itself total and reproducible for any
/// input, including 0.
constexpr uint32_t kDefaultSeed = 20260821u;

std::vector<uint32_t> nodeIds(const std::vector<NodeConfigExtended>& nodes) {
  std::vector<uint32_t> ids;
  ids.reserve(nodes.size());
  for (const auto& node : nodes) {
    ids.push_back(node.nodeId);
  }
  return ids;
}

bool resolveId(const std::vector<NodeConfigExtended>& nodes,
               const std::string& name, uint32_t& out) {
  for (const auto& node : nodes) {
    if (node.id == name) {
      out = node.nodeId;
      return true;
    }
  }
  return false;
}

/// True for events that act on a specific physical link, and so need that link
/// wired to do anything: connection drop/restore/degrade and break/restore.
///
/// Deliberately NOT inject_message. An injection traverses the mesh -- often
/// several hops, which is the whole point of a routing scenario -- so
/// preferring its source/destination as a direct edge would rewire the graph
/// around the very probe meant to exercise it. issue_138_message_routing
/// declares a mesh and injects across it to test multi-hop delivery; biasing
/// the tree toward those pairs would hide exactly the routing failures it
/// exists to catch.
bool actsOnAPhysicalLink(EventAction action) {
  switch (action) {
    case EventAction::CONNECTION_DROP:
    case EventAction::CONNECTION_RESTORE:
    case EventAction::CONNECTION_DEGRADE:
    case EventAction::BREAK_LINK:
    case EventAction::RESTORE_LINK:
      return true;
    default:
      return false;
  }
}

/// Node pairs a scenario's link-manipulation events name. When the declared
/// graph has to be reduced, these are kept first: a `connection_drop
/// node-1 <-> node-2` against a link that was never wired reports "0 live
/// endpoint(s) closed", which is exactly the silence this planner exists to
/// remove. Injection pairs are excluded -- see actsOnAPhysicalLink().
std::vector<PlannedLink> eventPairs(const std::vector<EventConfig>& events,
                                    const std::vector<NodeConfigExtended>& nodes) {
  std::vector<PlannedLink> pairs;
  for (const auto& event : events) {
    if (!actsOnAPhysicalLink(event.action)) {
      continue;
    }
    // A link event names its endpoints as `targets: [a, b]` OR `from`/`to`;
    // EventFactory::resolveLink() accepts both, so this must too, or a drop
    // declared with the targets syntax loses its preferred edge and the
    // spanning-tree reduction can discard the very pair it means to cut.
    std::string a;
    std::string b;
    if (event.targets.size() >= 2) {
      a = event.targets[0];
      b = event.targets[1];
    } else {
      a = event.from;
      b = event.to;
    }
    if (a.empty() || b.empty()) {
      continue;
    }
    uint32_t from = 0;
    uint32_t to = 0;
    if (resolveId(nodes, a, from) && resolveId(nodes, b, to) && from != to) {
      pairs.emplace_back(from, to);
    }
  }
  return pairs;
}

bool samePair(const PlannedLink& a, const PlannedLink& b) {
  return (a.first == b.first && a.second == b.second) ||
         (a.first == b.second && a.second == b.first);
}

/// Union-find root with path compression
uint32_t findRoot(std::map<uint32_t, uint32_t>& parent, uint32_t x) {
  while (parent[x] != x) {
    parent[x] = parent[parent[x]];
    x = parent[x];
  }
  return x;
}

/// Edges that keep each partition group internally connected. A network_partition
/// cuts only cross-group links, so if the spanning tree does not already connect
/// a group internally, partitioning it fragments that group into extra
/// components -- e.g. a 4-node mesh reduced to a star around n1, partitioned
/// [[n1,n2],[n3,n4]], leaves n3 and n4 with no edge between them and yields
/// three components, not two. Emitting a spanning path within each group as a
/// preference makes the reduction keep those edges, so the partition produces
/// exactly the requested components.
std::vector<PlannedLink> partitionGroupEdges(
    const std::vector<EventConfig>& events,
    const std::vector<NodeConfigExtended>& nodes) {
  std::vector<PlannedLink> edges;
  for (const auto& event : events) {
    if (event.action != EventAction::PARTITION_NETWORK) {
      continue;
    }
    for (const auto& group : event.groups) {
      // Resolve the group to node ids present in the scenario.
      std::vector<uint32_t> ids;
      for (const auto& id : group) {
        uint32_t resolved = 0;
        if (resolveId(nodes, id, resolved)) {  // unknown ids flagged in validation
          ids.push_back(resolved);
        }
      }
      // Emit EVERY intra-group pair, not just a single path. A fixed path chosen
      // per group in event order is greedy: with overlapping partition events it
      // can drop an edge as cyclic and reject a topology that does have a valid
      // spanning tree keeping all groups connected. Offering the whole
      // intra-group clique lets spanningSubset()'s union-find pick a consistent
      // set -- for any group not yet connected, some intra-group pair bridges
      // two of its components and is kept.
      for (size_t i = 0; i < ids.size(); ++i) {
        for (size_t j = i + 1; j < ids.size(); ++j) {
          if (ids[i] != ids[j]) {
            edges.emplace_back(ids[i], ids[j]);
          }
        }
      }
    }
  }
  return edges;
}

/// Reduce a declared graph to a spanning forest, preferring @p preferred edges
///
/// painlessMesh converges to a spanning tree whatever it is handed, so this
/// picks which tree deterministically instead of leaving it to a race between
/// overlapping handshakes.
std::vector<PlannedLink> spanningSubset(const std::vector<PlannedLink>& declared,
                                        const std::vector<PlannedLink>& preferred,
                                        const std::vector<PlannedLink>& softPreferred,
                                        std::vector<std::string>& warnings,
                                        std::vector<PlannedLink>& unwireablePreferred) {
  std::map<uint32_t, uint32_t> parent;
  for (const auto& link : declared) {
    parent[link.first] = link.first;
    parent[link.second] = link.second;
  }

  // Ordering priority: hard-preferred edges first (a link event names them, and
  // one dropped to a cycle is a hard error), then soft-preferred edges
  // (intra-partition-group links -- kept when they fit, but silently skipped
  // when they cannot, since a group need not be a subtree), then the rest in
  // declared order. Stable throughout.
  std::vector<PlannedLink> ordered;
  ordered.reserve(declared.size());
  auto push_matching = [&](const std::vector<PlannedLink>& wants) {
    for (const auto& want : wants) {
      for (const auto& link : declared) {
        if (samePair(link, want) &&
            std::none_of(ordered.begin(), ordered.end(),
                         [&](const PlannedLink& l) { return samePair(l, link); })) {
          ordered.push_back(link);
        }
      }
    }
  };
  push_matching(preferred);
  push_matching(softPreferred);
  for (const auto& link : declared) {
    if (std::none_of(ordered.begin(), ordered.end(),
                     [&](const PlannedLink& l) { return samePair(l, link); })) {
      ordered.push_back(link);
    }
  }

  std::vector<PlannedLink> kept;
  for (const auto& link : ordered) {
    const uint32_t ra = findRoot(parent, link.first);
    const uint32_t rb = findRoot(parent, link.second);
    if (ra == rb) {
      // Would close a cycle. Say so for the pairs an event names, since that
      // event will find nothing to act on.
      if (std::any_of(preferred.begin(), preferred.end(),
                      [&](const PlannedLink& p) { return samePair(p, link); })) {
        warnings.push_back(
            "an event names the link " + std::to_string(link.first) + " <-> " +
            std::to_string(link.second) +
            ", but keeping it would close a cycle painlessMesh will not hold");
        unwireablePreferred.push_back(link);
      }
      continue;
    }
    parent[ra] = rb;
    kept.push_back(link);
  }
  return kept;
}

}  // namespace

std::string topologyTypeName(TopologyType type) {
  switch (type) {
    case TopologyType::RANDOM: return "random";
    case TopologyType::STAR:   return "star";
    case TopologyType::RING:   return "ring";
    case TopologyType::MESH:   return "mesh";
    case TopologyType::CUSTOM: return "custom";
  }
  return "unknown";
}

TopologyPlan planTopology(const TopologyConfig& topology,
                          const std::vector<NodeConfigExtended>& nodes,
                          const std::vector<EventConfig>& events,
                          uint32_t seed) {
  TopologyPlan plan;
  const auto ids = nodeIds(nodes);
  if (ids.size() < 2) {
    return plan;  // nothing to wire
  }

  std::vector<PlannedLink> declared;

  switch (topology.type) {
    case TopologyType::MESH: {
      for (size_t i = 0; i < ids.size(); ++i) {
        for (size_t j = i + 1; j < ids.size(); ++j) {
          declared.emplace_back(ids[i], ids[j]);
        }
      }
      break;
    }

    case TopologyType::STAR: {
      uint32_t hub = ids.front();
      if (topology.hub.is_initialized() &&
          !resolveId(nodes, topology.hub.get(), hub)) {
        plan.warnings.push_back(
            "star hub '" + topology.hub.get() +
            "' is not a node in this scenario; using the first node as hub");
        hub = ids.front();
      } else if (!topology.hub.is_initialized()) {
        plan.warnings.push_back(
            "star topology declared without a hub; using the first node");
      }
      for (uint32_t id : ids) {
        if (id != hub) {
          declared.emplace_back(hub, id);
        }
      }
      break;
    }

    case TopologyType::RING: {
      for (size_t i = 0; i + 1 < ids.size(); ++i) {
        declared.emplace_back(ids[i], ids[i + 1]);
      }
      if (ids.size() > 2) {
        declared.emplace_back(ids.back(), ids.front());
      }
      if (!topology.bidirectional) {
        // A link here is one TCP connection and carries both directions. There
        // is no half-duplex mode to fall back on, so say so rather than
        // pretending the flag did something.
        plan.warnings.push_back(
            "ring declared bidirectional: false, but a simulated link is a "
            "single connection carrying both directions -- wired as "
            "bidirectional");
      }
      break;
    }

    case TopologyType::CUSTOM: {
      for (const auto& conn : topology.connections) {
        uint32_t from = 0;
        uint32_t to = 0;
        if (!resolveId(nodes, conn.first, from) ||
            !resolveId(nodes, conn.second, to)) {
          plan.warnings.push_back("custom connection '" + conn.first + "' <-> '" +
                                  conn.second + "' names an unknown node; skipped");
          continue;
        }
        if (from == to) {
          plan.warnings.push_back("custom connection '" + conn.first +
                                  "' <-> '" + conn.second +
                                  "' is a self-link; skipped");
          continue;
        }
        declared.emplace_back(from, to);
      }
      break;
    }

    case TopologyType::RANDOM: {
      std::mt19937 rng(seed != 0 ? seed : kDefaultSeed);

      // Spanning tree first. Density is a target, not a licence to partition
      // the mesh before the run starts -- and this tree is exactly what every
      // scenario was given before the planner existed.
      for (size_t i = 1; i < ids.size(); ++i) {
        std::uniform_int_distribution<size_t> pick(0, i - 1);
        declared.emplace_back(ids[i], ids[pick(rng)]);
      }

      const size_t possible = ids.size() * (ids.size() - 1) / 2;
      const size_t target = static_cast<size_t>(
          static_cast<double>(topology.density) * static_cast<double>(possible) + 0.5);
      if (target > declared.size()) {
        // Collect the pairs the tree did not use, shuffle once, and take the
        // shortfall. Shuffling beats rejection sampling: it terminates.
        std::vector<PlannedLink> spare;
        spare.reserve(possible - declared.size());
        for (size_t i = 0; i < ids.size(); ++i) {
          for (size_t j = i + 1; j < ids.size(); ++j) {
            const PlannedLink pair(ids[i], ids[j]);
            const bool already = std::any_of(
                declared.begin(), declared.end(), [&](const PlannedLink& l) {
                  return (l.first == pair.first && l.second == pair.second) ||
                         (l.first == pair.second && l.second == pair.first);
                });
            if (!already) {
              spare.push_back(pair);
            }
          }
        }
        std::shuffle(spare.begin(), spare.end(), rng);
        const size_t extra = std::min(target - declared.size(), spare.size());
        declared.insert(declared.end(), spare.begin(), spare.begin() + extra);
      }
      break;
    }
  }

  plan.declared = declared.size();

  // Guarantee that a pair a link event names is actually a candidate to wire.
  // A `random` topology draws its edges at random, so a connection_drop pair
  // may simply be absent from `declared` -- and spanningSubset() only reorders
  // edges already present, it cannot invent one. Without this the drop would
  // run against no live link and report "0 live endpoint(s) closed", the very
  // silence the preferred-edge logic exists to remove. Adding the event pairs
  // as candidates (they are valid node pairs) makes the guarantee real for
  // every topology type; spanningSubset() still prefers them into the tree.
  const auto preferred = eventPairs(events, nodes);
  const auto groupEdges = partitionGroupEdges(events, nodes);
  std::vector<PlannedLink> candidates = declared;

  // For a `random` topology only, add an event-named pair the random draw did
  // not include, so a connection_drop has a live link to cut, and add
  // intra-partition-group edges so each group stays internally connected. The
  // other modes declare an explicit, intentional graph: inserting an event's
  // pair there would change the topology and slip an undeclared edge past
  // restoreLink()'s guard. An event naming a non-edge of an explicit topology is
  // a config error left to fail at runtime, not papered over here.
  if (topology.type == TopologyType::RANDOM) {
    auto add_missing = [&](const std::vector<PlannedLink>& wants) {
      for (const auto& want : wants) {
        const bool present = std::any_of(
            candidates.begin(), candidates.end(),
            [&](const PlannedLink& l) { return samePair(l, want); });
        if (!present) {
          candidates.push_back(want);
        }
      }
    };
    add_missing(preferred);
    add_missing(groupEdges);
  } else {
    // An explicit topology (mesh/star/ring/custom) is wired exactly as declared.
    // A link event naming a pair that is not one of its edges has nothing to act
    // on -- dropLink() would record a phantom pair and close zero endpoints, and
    // the run would still exit 0. That is a configuration error, so flag the
    // pair as unwireable; the entry point fails the run on it, the same as a
    // cyclic event link. (mesh declares every pair, so this only bites star,
    // ring and custom.)
    for (const auto& want : preferred) {
      const bool declaredEdge = std::any_of(
          declared.begin(), declared.end(),
          [&](const PlannedLink& l) { return samePair(l, want); });
      if (!declaredEdge) {
        plan.warnings.push_back(
            "an event names the link " + std::to_string(want.first) + " <-> " +
            std::to_string(want.second) +
            ", which is not an edge of the declared " +
            topologyTypeName(topology.type) + " topology");
        plan.unwireable_preferred.push_back(want);
      }
    }
  }

  plan.links = spanningSubset(candidates, preferred, groupEdges, plan.warnings,
                              plan.unwireable_preferred);
  if (plan.links.size() < declared.size()) {
    plan.warnings.push_back(
        topologyTypeName(topology.type) + " declares " +
        std::to_string(declared.size()) + " link(s); painlessMesh holds a " +
        "spanning tree, so " + std::to_string(declared.size() - plan.links.size()) +
        " surplus link(s) were not wired (wiring them all at once leaves the " +
        "mesh with no live links at all)");
  }
  // A partition cuts only cross-group links, so each group must be internally
  // connected in the final plan or the partition fragments it. Detect that here
  // and record it, so validation rejects the scenario rather than throwing only
  // when the event fires.
  for (const auto& event : events) {
    if (event.action != EventAction::PARTITION_NETWORK) {
      continue;
    }
    for (const auto& group : event.groups) {
      // Resolve the group to node ids present in the plan.
      std::vector<uint32_t> ids;
      for (const auto& id : group) {
        uint32_t resolved = 0;
        if (resolveId(nodes, id, resolved)) {
          ids.push_back(resolved);
        }
      }
      if (ids.size() < 2) {
        continue;  // a single node is trivially connected
      }
      // Union-find over the plan's intra-group edges only.
      std::map<uint32_t, uint32_t> gp;
      for (uint32_t id : ids) gp[id] = id;
      for (const auto& link : plan.links) {
        const bool a_in = std::find(ids.begin(), ids.end(), link.first) != ids.end();
        const bool b_in = std::find(ids.begin(), ids.end(), link.second) != ids.end();
        if (a_in && b_in) {
          gp[findRoot(gp, link.first)] = findRoot(gp, link.second);
        }
      }
      const uint32_t root = findRoot(gp, ids.front());
      const bool connected = std::all_of(ids.begin(), ids.end(),
          [&](uint32_t id) { return findRoot(gp, id) == root; });
      if (!connected) {
        std::string members;
        for (size_t i = 0; i < group.size(); ++i) {
          members += (i ? ", " : "") + group[i];
        }
        plan.infeasible_partitions.push_back(
            "partition group [" + members + "] is not internally connected in "
            "the " + topologyTypeName(topology.type) + " topology");
      }
    }
  }

  return plan;
}

} // namespace simulator
