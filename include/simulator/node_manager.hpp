/**
 * @file node_manager.hpp
 * @brief Node manager for coordinating multiple virtual mesh nodes
 * 
 * This file contains the NodeManager class which creates, manages, and
 * coordinates multiple VirtualNode instances in the simulation.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef SIMULATOR_NODE_MANAGER_HPP
#define SIMULATOR_NODE_MANAGER_HPP

#include <memory>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include <cstdint>
#include <boost/asio.hpp>
#include "simulator/virtual_node.hpp"

// Forward declaration
class Scheduler;

namespace simulator {

/**
 * @brief Manages lifecycle and coordination of multiple virtual nodes
 * 
 * The NodeManager class is responsible for creating, starting, stopping,
 * and coordinating updates across multiple VirtualNode instances. It
 * maintains a central scheduler and IO context that all nodes share.
 * 
 * Example usage:
 * @code
 * boost::asio::io_context io;
 * NodeManager manager(io);
 * 
 * // Create nodes
 * NodeConfig config1{1001, "TestMesh", "password"};
 * NodeConfig config2{1002, "TestMesh", "password"};
 * auto node1 = manager.createNode(config1);
 * auto node2 = manager.createNode(config2);
 * 
 * // Start all nodes
 * manager.startAll();
 * 
 * // Run simulation
 * for (int i = 0; i < 1000; ++i) {
 *   manager.updateAll();
 * }
 * 
 * // Stop all nodes
 * manager.stopAll();
 * @endcode
 * 
 * @note NodeManager is not thread-safe. All operations should be called
 *       from the same thread.
 */
class NodeManager {
public:
  /**
   * @brief Create a node manager
   * 
   * @param io Boost.Asio IO context for networking
   * 
   * The IO context must remain valid for the lifetime of the NodeManager
   * and all nodes it creates.
   */
  explicit NodeManager(boost::asio::io_context& io);
  
  /**
   * @brief Destructor - stops and cleans up all nodes
   */
  ~NodeManager();
  
  // Prevent copying
  NodeManager(const NodeManager&) = delete;
  NodeManager& operator=(const NodeManager&) = delete;
  
  // Allow moving
  NodeManager(NodeManager&&) = default;
  NodeManager& operator=(NodeManager&&) = default;
  
  // Node lifecycle
  
  /**
   * @brief Create a new virtual node
   * 
   * Creates a new VirtualNode with the specified configuration and
   * adds it to the managed node collection.
   * 
   * @param config Node configuration parameters
   * @return Shared pointer to created node
   * 
   * @throws std::invalid_argument if nodeId is 0
   * @throws std::runtime_error if node with same ID exists
   * @throws std::runtime_error if max nodes reached (MAX_NODES)
   * 
   * @note The node is created but not started automatically.
   *       Call startAll() or node->start() to begin operation.
   */
  std::shared_ptr<VirtualNode> createNode(const NodeConfig& config);
  
  /**
   * @brief Remove a node by ID
   * 
   * Stops the node if running, then removes it from the managed
   * node collection.
   * 
   * @param nodeId ID of node to remove
   * @return true if node was removed, false if not found
   */
  bool removeNode(uint32_t nodeId);
  
  /**
   * @brief Start all nodes
   * 
   * Starts all nodes that are not currently running.
   * Nodes already running are skipped.
   */
  void startAll();
  
  /**
   * @brief Stop all nodes
   * 
   * Stops all nodes that are currently running.
   * Nodes already stopped are skipped.
   */
  void stopAll();
  
  /**
   * @brief Update all nodes
   * 
   * Performs coordinated update of all nodes:
   * 1. Executes scheduler tasks
   * 2. Updates each node
   * 3. Polls IO context
   * 
   * Should be called periodically (e.g., in a simulation loop)
   * to advance the simulation state.
   */
  void updateAll();
  
  /**
   * @brief Establish mesh connectivity between nodes
   * 
   * Creates a random mesh topology where each node connects to
   * at least one other node, forming a connected graph.
   * 
   * This simulates the natural mesh formation that occurs in
   * painlessMesh networks.
   */
  void establishConnectivity();

  /**
   * @brief Wire exactly the links a scenario's topology asks for
   *
   * `topology:` was parsed and validated but never applied -- every run got the
   * random tree above regardless of what the scenario declared, so once link
   * events became live they addressed edges that did not exist. Plan the links
   * with planTopology() and pass them here.
   *
   * Each link is recorded in the adjacency map, so drop, partition, heal and
   * reconnect all operate on the graph the scenario actually declared.
   *
   * @param links Node-id pairs to connect
   * @return Number of links successfully wired
   */
  size_t establishConnectivity(const std::vector<std::pair<uint32_t, uint32_t>>& links);

  /**
   * @brief Pump the mesh until a just-created link settles
   *
   * Wiring every declared link in one tight loop makes painlessMesh tear the
   * whole mesh down: measured on a 4-node full mesh, 6 links wired, 0 live and
   * not one message delivered in 20s. Given a moment between connects it
   * instead prunes the redundant edges and keeps a working spanning tree.
   *
   * @return true if both endpoints report the connection within the budget;
   *         false on timeout -- a real failure the caller must not paper over.
   */
  bool settleLink(uint32_t a, uint32_t b);
  
  // Queries
  
  /**
   * @brief Get number of managed nodes
   * 
   * @return Count of nodes currently managed
   */
  size_t getNodeCount() const { return nodes_.size(); }

  /**
   * @brief Number of nodes whose configured firmware failed to load
   *
   * createNode() deliberately keeps a node alive when its firmware cannot be
   * resolved, so that an interactive run still shows the rest of the mesh.
   * That is the wrong default for CI, where a scenario naming firmware which
   * never loads must not report success. Callers acting as a test gate should
   * treat any non-zero value here as a failed run.
   */
  size_t getFirmwareLoadFailureCount() const { return firmware_load_failures_; }

  /**
   * @brief Number of nodes currently running
   *
   * Distinct from getNodeCount(): a crashed or stopped node is still managed.
   * Progress output used to report the total here, so a scenario that crashed
   * half the mesh still printed the full node count as "running".
   */
  size_t getRunningCount() const {
    size_t running = 0;
    for (const auto& pair : nodes_) {
      if (pair.second && pair.second->isRunning()) ++running;
    }
    return running;
  }
  
  /**
   * @brief Get a specific node by ID
   * 
   * @param nodeId ID of node to retrieve
   * @return Shared pointer to node, or nullptr if not found
   */
  std::shared_ptr<VirtualNode> getNode(uint32_t nodeId);
  
  /**
   * @brief Get a specific node by ID (const version)
   * 
   * @param nodeId ID of node to retrieve
   * @return Shared pointer to const node, or nullptr if not found
   */
  std::shared_ptr<const VirtualNode> getNode(uint32_t nodeId) const;
  
  /**
   * @brief Get list of all node IDs
   * 
   * @return Vector of node IDs in no particular order
   */
  std::vector<uint32_t> getNodeIds() const;
  
  /**
   * @brief Get all managed nodes
   * 
   * @return Vector of shared pointers to all nodes
   */
  std::vector<std::shared_ptr<VirtualNode>> getAllNodes() const;
  
  /**
   * @brief Check if a node with the given ID exists
   * 
   * @param nodeId ID to check
   * @return true if node exists, false otherwise
   */
  bool hasNode(uint32_t nodeId) const;

  // --- Topology control -----------------------------------------------------
  //
  // establishConnectivity() wires the mesh once, at startup, and nothing
  // recorded which node was wired to which. That left two holes: a scenario
  // link event had no way to sever a real connection (it could only mutate the
  // standalone NetworkSimulator, which no delivery path reads), and a node
  // restarted mid-run came back isRunning() == true without rejoining the
  // mesh. The manager now owns the edge list so both are answerable.

  /**
   * @brief Connect two nodes and record the edge in the topology
   *
   * @param fromNode Node that initiates the connection
   * @param toNode Node that accepts it
   * @return true if both nodes exist and the connection was initiated
   */
  bool connectNodes(uint32_t fromNode, uint32_t toNode);

  /**
   * @brief Sever the live link between two nodes
   *
   * Marks the link severed so a later reconnect will not silently restore it,
   * then closes the painlessMesh connection from both ends.
   *
   * @param a First node ID
   * @param b Second node ID
   * @return Number of live connection endpoints actually closed (0, 1 or 2)
   */
  size_t dropLink(uint32_t a, uint32_t b);

  /**
   * @brief Restore a previously severed link
   *
   * @param a First node ID
   * @param b Second node ID
   * @return true if the link was reconnected
   */
  bool restoreLink(uint32_t a, uint32_t b);

  /**
   * @brief Cut every link that crosses a partition boundary
   *
   * @param groups Node ID groups; every pair drawn from two different groups
   *               is severed
   * @return Number of recorded edges cut
   */
  size_t partitionNetwork(const std::vector<std::vector<uint32_t>>& groups);

  /**
   * @brief Clear every severed link and rebuild the recorded topology
   *
   * @return Number of links reconnected
   */
  size_t healNetwork();

  /**
   * @brief Re-attach a node to its recorded peers after a start or restart
   *
   * @param nodeId Node that has just come back up
   * @return Number of links re-established
   */
  size_t reconnectNode(uint32_t nodeId);

  /**
   * @brief Whether a link is currently marked severed by a scenario event
   *
   * @param a First node ID
   * @param b Second node ID
   * @return true if severed
   */
  bool isLinkSevered(uint32_t a, uint32_t b) const;

  /**
   * @brief Recorded peers of a node, whether or not the links are live
   *
   * @param nodeId Node to query
   * @return Peer node IDs (empty if the node is unknown)
   */
  std::vector<uint32_t> getRecordedPeers(uint32_t nodeId) const;

  /**
   * @brief Total number of live mesh connections across all nodes
   *
   * Counts endpoints, so a healthy two-node link contributes 2.
   *
   * @return Live connection endpoint count
   */
  size_t getTotalConnectionCount() const;

  /**
   * @brief Groups of node IDs that can still reach each other
   *
   * Computed from the recorded topology minus severed links, so it answers
   * "did that partition event actually split the mesh?" without waiting for
   * painlessMesh to reconverge.
   *
   * @return One sorted vector of node IDs per connected component
   */
  std::vector<std::vector<uint32_t>> getConnectedComponents() const;

  // Resource limits
  
  /**
   * @brief Maximum number of nodes that can be created
   * 
   * This limit helps prevent excessive resource usage and
   * ensures simulation performance remains reasonable.
   */
  static constexpr size_t MAX_NODES = 1000;

private:
  boost::asio::io_context& io_;                                   ///< IO context reference
  std::unique_ptr<Scheduler> scheduler_;                          ///< Shared scheduler instance
  std::map<uint32_t, std::shared_ptr<VirtualNode>> nodes_;        ///< Map of node ID to node
  size_t firmware_load_failures_ = 0;                             ///< Nodes whose firmware failed to load
  uint32_t next_node_id_{1000};                                   ///< Next auto-assigned node ID
  std::map<uint32_t, std::set<uint32_t>> topology_;               ///< Recorded mesh edges, both directions
  std::set<std::pair<uint32_t, uint32_t>> explicit_drops_;        ///< Cut by connection_drop; persist until connection_restore, (low, high)
  std::set<std::pair<uint32_t, uint32_t>> partition_cuts_;        ///< Cut by partitionNetwork(); healed by healNetwork(), (low, high)

  /// Closes the live connection between a pair without recording why. dropLink()
  /// and partitionNetwork() add the appropriate marker around it.
  size_t severConnection(uint32_t a, uint32_t b);

  /// Normalises a node pair so severance keys are direction-independent.
  static std::pair<uint32_t, uint32_t> linkKey(uint32_t a, uint32_t b) {
    return a < b ? std::make_pair(a, b) : std::make_pair(b, a);
  }
};

} // namespace simulator

#endif // SIMULATOR_NODE_MANAGER_HPP
