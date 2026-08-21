/**
 * @file firmware_base.hpp
 * @brief Base interface for custom firmware implementations
 * 
 * This file contains the FirmwareBase abstract class which serves as the
 * foundation for custom firmware implementations in the painlessMesh simulator.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef SIMULATOR_FIRMWARE_BASE_HPP
#define SIMULATOR_FIRMWARE_BASE_HPP

#include <string>
#include <map>
#include <list>
#include <cstdint>
#include <memory>
#include <functional>
#include <vector>

// Forward declarations
class Scheduler;
class Task;

namespace painlessmesh {
template <typename T>
class Mesh;
class Connection;
}

// Use std::string as String (compatible with PAINLESSMESH_ENABLE_STD_STRING)
using String = std::string;

namespace simulator {
namespace firmware {

/**
 * @brief Abstract base class for custom firmware implementations
 * 
 * FirmwareBase provides the interface for custom firmware that can be loaded
 * into virtual nodes. Firmware implementations control node behavior by
 * implementing lifecycle methods (setup/loop) and responding to mesh events.
 * 
 * Example implementation:
 * @code
 * class SensorFirmware : public FirmwareBase {
 * public:
 *   SensorFirmware() : FirmwareBase("SensorNode") {}
 *   
 *   void setup() override {
 *     // Initialize sensor, set up periodic tasks
 *     // registerTask(), not scheduler_->addTask(): the base disables
 *     // registered tasks while the node is stopped or crashed.
 *     registerTask(sensor_task_);
 *   }
 *   
 *   void loop() override {
 *     // Called every update cycle
 *   }
 *   
 *   void onReceive(uint32_t from, String& msg) override {
 *     // Handle received messages
 *   }
 *
 * private:
 *   Task sensor_task_{TASK_SECOND, TASK_FOREVER,
 *                     std::bind(&SensorFirmware::readAndBroadcast, this)};
 * };
 * @endcode
 */
class FirmwareBase {
public:
  /**
   * @brief Constructor
   * 
   * @param name Firmware name/identifier
   */
  explicit FirmwareBase(const std::string& name) : name_(name) {}
  
  /**
   * @brief Virtual destructor
   */
  virtual ~FirmwareBase() = default;
  
  /**
   * @brief Initialize firmware with mesh and scheduler
   * 
   * Called by the node after firmware is loaded but before setup().
   * Stores references to mesh, scheduler, and configuration.
   * 
   * @param mesh Pointer to mesh instance
   * @param scheduler Pointer to task scheduler
   * @param nodeId Node ID assigned to this firmware
   * @param config Configuration map (from YAML firmware_config)
   */
  void initialize(painlessmesh::Mesh<painlessmesh::Connection>* mesh,
                  Scheduler* scheduler,
                  uint32_t nodeId,
                  const std::map<String, String>& config) {
    mesh_ = mesh;
    scheduler_ = scheduler;
    node_id_ = nodeId;
    config_ = config;
    initialized_ = true;
  }
  
  /**
   * @brief Re-point this firmware at a rebuilt mesh instance
   *
   * A node that stops and starts again gets a fresh painlessMesh object (see
   * VirtualNode::start()). The firmware's cached pointer would otherwise dangle
   * on the destroyed one. Deliberately does not re-run setup(): the firmware's
   * scheduler tasks are still registered -- disabled by suspend() and put back
   * by resume() -- and adding them twice corrupts the scheduler's task list.
   *
   * @param mesh The new mesh instance
   */
  void rebindMesh(painlessmesh::Mesh<painlessmesh::Connection>* mesh) {
    mesh_ = mesh;
  }

  /**
   * @brief Halt this firmware's periodic work while its node is down
   *
   * Every node shares one Scheduler (NodeManager owns it) and
   * NodeManager::updateAll() executes it wholesale, so a task registered by a
   * stopped node's firmware keeps running: it sends through torn-down routing
   * state and books transmissions that never happened. VirtualNode::stop() and
   * crash() call this; start() calls resume().
   *
   * Disables every task registered through registerTask() and blocks the
   * sendBroadcast()/sendSingle() helpers. Tasks a firmware added to the
   * scheduler directly are not covered -- register them through
   * registerTask() instead.
   */
  void suspend();

  /**
   * @brief Resume periodic work after the node starts again
   *
   * Restores each registered task to the enabled state it held at suspend()
   * rather than enabling all of them: a firmware that had already disabled a
   * task of its own accord (LibraryValidation does this once its tests finish)
   * must not find it running again after a restart.
   */
  void resume();

  /**
   * @brief Whether this firmware is currently suspended
   */
  bool isSuspended() const { return suspended_; }

  /**
   * @brief Check if firmware has been initialized
   * 
   * @return true if initialize() has been called, false otherwise
   */
  bool isInitialized() const { return initialized_; }
  
  /**
   * @brief Setup firmware (called once after initialization)
   * 
   * Override this method to perform firmware initialization:
   * - Set up periodic tasks
   * - Initialize sensors/peripherals
   * - Register callbacks
   * - Configure mesh settings
   * 
   * The mesh is already initialized when this is called.
   */
  virtual void setup() = 0;
  
  /**
   * @brief Main loop (called every update cycle)
   * 
   * Override this method to implement periodic behavior:
   * - Poll sensors
   * - Send messages
   * - Update state
   * 
   * Keep this method fast as it's called frequently.
   * Use scheduler for time-based tasks instead of delays.
   */
  virtual void loop() = 0;
  
  /**
   * @brief Callback for received messages
   * 
   * @param from Source node ID
   * @param msg Message content
   * 
   * Override to handle incoming mesh messages.
   * Default implementation does nothing.
   */
  virtual void onReceive(uint32_t from, String& msg) {
    (void)from;
    (void)msg;
  }
  
  /**
   * @brief Callback for new mesh connections
   * 
   * @param nodeId ID of newly connected node
   * 
   * Override to respond to new connections.
   * Default implementation does nothing.
   */
  virtual void onNewConnection(uint32_t nodeId) {
    (void)nodeId;
  }
  
  /**
   * @brief Callback for connection topology changes
   * 
   * Override to respond to mesh topology changes.
   * Default implementation does nothing.
   */
  virtual void onChangedConnections() {}
  
  /**
   * @brief Callback for node time adjustments
   * 
   * @param offset Time offset in microseconds
   * 
   * Override to respond to time synchronization events.
   * Default implementation does nothing.
   */
  virtual void onNodeTimeAdjusted(int32_t offset) {
    (void)offset;
  }
  
  /**
   * @brief Gets the firmware name
   * 
   * @return Firmware name/identifier
   */
  std::string getName() const { return name_; }
  
  /**
   * @brief Gets the firmware version
   * 
   * Override this method to provide a custom version string.
   * 
   * @return Firmware version (default: "1.0.0")
   */
  virtual String getVersion() const { return "1.0.0"; }
  
  /**
   * @brief Gets the node ID
   * 
   * @return Node ID assigned during initialization
   */
  uint32_t getNodeId() const { return node_id_; }

  /**
   * @brief Installs a hook invoked for every message this firmware sends
   *
   * Firmware sends straight through mesh_, so the owning VirtualNode has no way
   * to observe a send and its messages_sent metric was permanently zero. The
   * node installs this hook when it loads the firmware.
   *
   * @param cb Receives the payload size in bytes
   */
  void setMessageSentCallback(std::function<void(size_t)> cb) {
    on_message_sent_ = std::move(cb);
  }
  
  /**
   * @brief Gets a configuration value
   * 
   * @param key Configuration key
   * @param defaultValue Default value if key not found
   * @return Configuration value or default
   */
  String getConfig(const String& key, const String& defaultValue) const {
    auto it = config_.find(key);
    if (it != config_.end()) {
      return it->second;
    }
    return defaultValue;
  }
  
  /**
   * @brief Checks if a configuration key exists
   * 
   * @param key Configuration key
   * @return true if key exists, false otherwise
   */
  bool hasConfig(const String& key) const {
    return config_.find(key) != config_.end();
  }

protected:
  /**
   * @brief Register a periodic task with the shared scheduler
   *
   * Use this instead of scheduler_->addTask() directly. Tasks registered here
   * are tracked, so suspend() can stop them for the duration of a node's
   * downtime and resume() can put them back exactly as they were.
   *
   * @param task Task owned by the firmware (must outlive the scheduler run)
   * @param enable_now Enable the task immediately, as setup() normally wants
   * @return true if the task was registered, false if there is no scheduler
   */
  bool registerTask(Task& task, bool enable_now = true);

  /**
   * @brief Send a broadcast message to all nodes in the mesh
   *
   * painlessMesh returns false when the broadcast reached nobody -- a node with
   * no live connections, for instance. The send accounting only fires on a
   * true return, so a rejected attempt is not booked as a transmission.
   * Callers keeping their own counters should test the result the same way.
   *
   * @param msg Message to broadcast
   * @return true if the mesh accepted the message for delivery
   */
  bool sendBroadcast(const String& msg);
  
  /**
   * @brief Send a message to a specific node
   *
   * As sendBroadcast(): false when the mesh has no route to @p dest, and a
   * false return is not counted.
   *
   * @param dest Destination node ID
   * @param msg Message to send
   * @return true if the mesh accepted the message for delivery
   */
  bool sendSingle(uint32_t dest, const String& msg);
  
  /**
   * @brief Get the current mesh time
   * 
   * Helper method that wraps mesh_->getNodeTime() with null check.
   * 
   * @return Current mesh time in microseconds, or 0 if mesh not initialized
   */
  uint32_t getNodeTime() const;
  
  /**
   * @brief Get list of all connected nodes
   * 
   * Helper method that wraps mesh_->getNodeList() with null check.
   * 
   * @return List of node IDs currently connected in the mesh, or empty list if mesh not initialized
   */
  std::list<uint32_t> getNodeList() const;

  std::string name_;                                      ///< Firmware name
  painlessmesh::Mesh<painlessmesh::Connection>* mesh_{nullptr};  ///< Mesh instance
  Scheduler* scheduler_{nullptr};                         ///< Task scheduler
  uint32_t node_id_{0};                                   ///< Node ID
  std::map<String, String> config_;                       ///< Configuration map
  bool initialized_{false};                               ///< Initialization flag
  bool suspended_{false};                                 ///< Node is down: no tasks, no sends
  std::vector<Task*> tasks_;                              ///< Tasks registered via registerTask()
  std::vector<bool> enabled_before_suspend_;              ///< Per-task enable state at suspend()
  std::function<void(size_t)> on_message_sent_;           ///< Send accounting hook
};

} // namespace firmware
} // namespace simulator

#endif // SIMULATOR_FIRMWARE_BASE_HPP
