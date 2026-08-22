/**
 * @file topology_planner.hpp
 * @brief Turns a scenario's declared topology into the links to wire
 *
 * `topology:` was parsed and validated from the first release and never
 * applied: `NodeManager::establishConnectivity()` took no arguments and always
 * built a random spanning tree. 21 of the 23 shipped scenarios declare a
 * topology, so nearly every run was against a graph nobody asked for -- and
 * once link events became live, events addressed edges that did not exist. A
 * full-mesh scenario dropping a named pair reported *"0 live endpoint(s)
 * closed"*.
 *
 * The planner is a pure function so the mapping from declaration to edge list
 * can be tested without standing up sockets.
 *
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef SIMULATOR_TOPOLOGY_PLANNER_HPP
#define SIMULATOR_TOPOLOGY_PLANNER_HPP

#include "simulator/config_loader.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace simulator {

/**
 * @brief A planned link between two nodes, by numeric node id
 */
using PlannedLink = std::pair<uint32_t, uint32_t>;

/**
 * @brief Result of planning a scenario's topology
 */
struct TopologyPlan {
  std::vector<PlannedLink> links;      ///< Links to wire, in a stable order
  size_t declared = 0;                 ///< Links the topology block asked for
  std::vector<std::string> warnings;   ///< Anything the plan could not honour
  /// Event-named links that could not be wired because they would close a cycle
  /// painlessMesh will not hold. A scheduled event on such a pair would be a
  /// silent no-op, so the entry point treats a non-empty list as a hard error.
  std::vector<PlannedLink> unwireable_preferred;
  /// Human-readable descriptions of partition groups the plan cannot keep
  /// internally connected (an explicit topology whose group excludes the only
  /// path between its nodes, or incompatible groups across events). The event
  /// would throw at its timestamp, so the entry point fails on these too.
  std::vector<std::string> infeasible_partitions;
};

/**
 * @brief Plan the links a scenario's declared topology asks for
 *
 * Declared shape per type:
 * - `mesh`: every pair.
 * - `star`: the hub to every other node.
 * - `ring`: consecutive nodes plus the closing link.
 * - `custom`: exactly the declared connections, resolved by string id.
 * - `random`: `density` of all possible pairs.
 *
 * **The declared graph is then reduced to a spanning forest**, because that is
 * all painlessMesh will hold. Measured on 4 nodes with SimpleBroadcast: wiring
 * the 3 links of a star gives 3 live links and 40 sent / 119 received over
 * 20s; wiring the 6 links a `mesh` declares gives **0 live links and not one
 * message** -- the overlapping handshakes make the library tear the whole mesh
 * down. Given a moment between connects it instead prunes back to a spanning
 * tree on its own. Planning the tree directly is the same end state without
 * the wasted connects, and it is deterministic rather than a race.
 *
 * Where the declared graph has more links than a tree, the surplus is dropped
 * in a defined order: pairs named by the scenario's *link-manipulation* events
 * (connection drop/restore/degrade, break/restore) are kept first, so a
 * declared `connection_drop node-1 <-> node-2` has a live link to cut. That
 * exact drop reported *"0 live endpoint(s) closed"* before this existed.
 * `inject_message` pairs are deliberately excluded: an injection traverses the
 * mesh, often multi-hop, so preferring its endpoints as a direct edge would
 * rewire the graph around the very probe meant to exercise routing.
 *
 * Link order is stable and the random draw is seeded from `simulation.seed`,
 * so a scenario wires the same graph on every run.
 *
 * @param topology The scenario's `topology:` block
 * @param nodes The scenario's nodes, for id resolution
 * @param events The scenario's events; pairs named by link-manipulation events
 *               (not injections) are preferred when the graph must be reduced
 * @param seed `simulation.seed`; 0 selects a fixed default so runs stay
 *             reproducible
 * @return The links to wire, what was declared, and any warnings
 */
TopologyPlan planTopology(const TopologyConfig& topology,
                          const std::vector<NodeConfigExtended>& nodes,
                          const std::vector<EventConfig>& events,
                          uint32_t seed);

/**
 * @brief Human-readable name of a topology type, for logging
 */
std::string topologyTypeName(TopologyType type);

/**
 * @brief Reject event timelines that are guaranteed to fail at runtime.
 *
 * planTopology() checks a scenario's *static* feasibility, but the validation
 * sweep otherwise only constructs and queues events -- so a timeline that
 * breaks because of an *earlier* event slips through `--validate-only` and only
 * fails once the run is underway. This walks the events in scheduled order,
 * tracking each node's up/down state (and connection drops / partition cuts),
 * and reports the two deterministic mismatches:
 *
 * - a `network_partition` whose groups no longer form exactly that many
 *   connected components in the live mesh -- e.g. a prior `stop_node` split a
 *   group by taking down its articulation node (matches the runtime throw in
 *   NetworkPartitionEvent);
 * - an `inject_message` whose sender is stopped at that point in the timeline
 *   (matches the runtime "injection refused" throw).
 *
 * @param events The scenario's events (declaration order; equal times keep it)
 * @param wiredLinks The links planTopology() will actually wire (its `links`)
 * @param nodes The scenario's nodes, for id resolution and the full node set
 * @return One problem string per guaranteed-to-fail event; empty if none
 */
std::vector<std::string> validateEventTimeline(
    const std::vector<EventConfig>& events,
    const std::vector<PlannedLink>& wiredLinks,
    const std::vector<NodeConfigExtended>& nodes);

} // namespace simulator

#endif // SIMULATOR_TOPOLOGY_PLANNER_HPP
