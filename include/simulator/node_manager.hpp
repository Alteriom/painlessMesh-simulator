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
  
  // Queries
  
  /**
   * @brief Get number of managed nodes
   * 
   * @return Count of nodes currently managed
   */
  size_t getNodeCount() const { return nodes_.size(); }
  
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
  uint32_t next_node_id_{1000};                                   ///< Next auto-assigned node ID
};

} // namespace simulator

#endif // SIMULATOR_NODE_MANAGER_HPP
