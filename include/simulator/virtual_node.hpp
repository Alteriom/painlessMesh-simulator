/**
 * @file virtual_node.hpp
 * @brief Virtual node implementation for mesh simulation
 * 
 * This file contains the VirtualNode class which simulates a single
 * ESP32/ESP8266 device running painlessMesh.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef SIMULATOR_VIRTUAL_NODE_HPP
#define SIMULATOR_VIRTUAL_NODE_HPP

#include <memory>
#include <cstdint>
#include <chrono>
#include <string>
#include <boost/asio.hpp>

// Forward declarations
class Scheduler;
class MeshTest;

namespace painlessmesh {
template <typename T>
class Mesh;
class Connection;
}

namespace simulator {

/**
 * @brief Configuration parameters for a virtual node
 */
struct NodeConfig {
  uint32_t nodeId;                    ///< Unique node identifier
  std::string meshPrefix;             ///< Mesh network SSID prefix
  std::string meshPassword;           ///< Mesh network password
  uint16_t meshPort = 5555;           ///< Mesh network port
};

/**
 * @brief Performance and statistics metrics for a node
 */
struct NodeMetrics {
  uint32_t messages_sent = 0;         ///< Total messages sent
  uint32_t messages_received = 0;     ///< Total messages received
  uint64_t bytes_sent = 0;            ///< Total bytes sent
  uint64_t bytes_received = 0;        ///< Total bytes received
  std::chrono::steady_clock::time_point start_time;  ///< Node start timestamp
  uint32_t crash_count = 0;           ///< Number of times node has crashed
  uint64_t total_uptime_ms = 0;       ///< Total uptime in milliseconds across all sessions
};

/**
 * @brief Virtual node representing a simulated ESP32/ESP8266 device
 * 
 * The VirtualNode class encapsulates a single painlessMesh node instance
 * with its own scheduler and IO context. It provides lifecycle management,
 * metrics collection, and configuration for the simulated device.
 * 
 * Example usage:
 * @code
 * Scheduler scheduler;
 * boost::asio::io_context io;
 * NodeConfig config;
 * config.nodeId = 1001;
 * config.meshPrefix = "TestMesh";
 * config.meshPassword = "password";
 * 
 * VirtualNode node(1001, config, &scheduler, io);
 * node.start();
 * // ... run simulation ...
 * node.stop();
 * @endcode
 */
class VirtualNode {
public:
  /**
   * @brief Constructs a virtual mesh node
   * 
   * @param nodeId Unique node identifier (must be non-zero)
   * @param config Node configuration parameters
   * @param scheduler Task scheduler for async operations
   * @param io Boost.Asio IO context for networking
   * 
   * @throws std::invalid_argument if nodeId is 0
   * @throws std::runtime_error if initialization fails
   * 
   * @note The node must be started explicitly with start()
   */
  VirtualNode(uint32_t nodeId, 
              const NodeConfig& config,
              Scheduler* scheduler,
              boost::asio::io_context& io);
  
  /**
   * @brief Destructor - ensures proper cleanup
   */
  ~VirtualNode();
  
  // Prevent copying
  VirtualNode(const VirtualNode&) = delete;
  VirtualNode& operator=(const VirtualNode&) = delete;
  
  // Allow moving
  VirtualNode(VirtualNode&&) = default;
  VirtualNode& operator=(VirtualNode&&) = default;
  
  /**
   * @brief Starts the mesh node
   * 
   * Initializes the mesh instance, sets up callbacks, and begins
   * network operations.
   * 
   * @throws std::runtime_error if node is already running
   */
  void start();
  
  /**
   * @brief Stops the mesh node gracefully
   * 
   * Disconnects from mesh, cancels pending operations, and releases
   * network resources. Updates uptime metrics before stopping.
   */
  void stop();
  
  /**
   * @brief Crashes the node ungracefully
   * 
   * Simulates a node crash or power failure by stopping immediately
   * without cleanup. Increments crash count and updates metrics.
   * Unlike stop(), this does not properly disconnect from the mesh,
   * simulating an unexpected failure.
   */
  void crash();
  
  /**
   * @brief Restarts the node
   * 
   * Stops the node gracefully, then starts it again. This simulates
   * a controlled restart scenario.
   */
  void restart();
  
  /**
   * @brief Updates the node state
   * 
   * Should be called periodically to process scheduled tasks and
   * network events.
   */
  void update();
  
  /**
   * @brief Gets the unique node identifier
   * 
   * @return Node ID assigned during construction
   */
  uint32_t getNodeId() const { return node_id_; }
  
  /**
   * @brief Gets reference to the underlying mesh instance
   * 
   * @return Reference to painlessMesh instance
   * @throws std::runtime_error if mesh not initialized
   */
  painlessmesh::Mesh<painlessmesh::Connection>& getMesh();
  
  /**
   * @brief Gets const reference to the underlying mesh instance
   * 
   * @return Const reference to painlessMesh instance
   * @throws std::runtime_error if mesh not initialized
   */
  const painlessmesh::Mesh<painlessmesh::Connection>& getMesh() const;
  
  /**
   * @brief Gets current performance metrics
   * 
   * @return Copy of current metrics structure
   */
  NodeMetrics getMetrics() const { return metrics_; }
  
  /**
   * @brief Checks if node is currently running
   * 
   * @return true if node is started, false otherwise
   */
  bool isRunning() const { return running_; }
  
  /**
   * @brief Gets the current uptime of the node
   * 
   * @return Uptime in milliseconds since last start, or 0 if not running
   */
  uint64_t getUptime() const;
  
  /**
   * @brief Gets the total number of crashes
   * 
   * @return Number of times the node has crashed
   */
  uint32_t getCrashCount() const { return metrics_.crash_count; }
  
  /**
   * @brief Sets simulated network quality
   * 
   * @param quality Quality factor from 0.0 (worst) to 1.0 (best)
   * 
   * This can be used to simulate packet loss, latency, etc.
   * Implementation is currently a stub for future enhancement.
   */
  void setNetworkQuality(float quality);
  
  /**
   * @brief Connect this node to another node
   * 
   * @param other The node to connect to
   * 
   * Creates a mesh connection from this node to the specified target node.
   * This simulates the WiFi mesh connection that would occur naturally
   * in a real mesh network.
   */
  void connectTo(VirtualNode& other);

private:
  uint32_t node_id_;                   ///< Unique node identifier
  std::unique_ptr<MeshTest> mesh_;     ///< Mesh instance wrapper
  Scheduler* scheduler_;               ///< Task scheduler reference
  boost::asio::io_context& io_;        ///< IO context reference
  NodeMetrics metrics_;                ///< Performance metrics
  bool running_{false};                ///< Running state flag
  float network_quality_{1.0f};        ///< Network quality (0.0-1.0)
  
  /**
   * @brief Callback for received messages
   * 
   * @param from Source node ID
   * @param msg Message content
   */
  void onReceive(uint32_t from, std::string& msg);
  
  /**
   * @brief Callback for new connections
   * 
   * @param nodeId ID of newly connected node
   */
  void onNewConnection(uint32_t nodeId);
  
  /**
   * @brief Callback for connection topology changes
   */
  void onChangedConnections();
};

} // namespace simulator

#endif // SIMULATOR_VIRTUAL_NODE_HPP
