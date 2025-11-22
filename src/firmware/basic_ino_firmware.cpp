/**
 * @file basic_ino_firmware.cpp
 * @brief Wrapper for painlessMesh basic.ino example
 * 
 * This file wraps the basic.ino example from painlessMesh so it can run
 * in the simulator. This is the most common starting point for users,
 * so validating it works correctly is critical.
 * 
 * Original .ino: external/painlessMesh/examples/basic/basic.ino
 * 
 * Tests validated:
 * - Basic mesh formation
 * - Broadcast messaging
 * - Message reception
 * - Connection callbacks
 * - Time synchronization
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include "simulator/firmware/ino_firmware_wrapper.hpp"
#include "simulator/firmware/firmware_factory.hpp"
#include <TaskSchedulerDeclarations.h>
#include <iostream>
#include <sstream>

namespace simulator {
namespace firmware {

/**
 * @brief Firmware wrapper for basic.ino example
 * 
 * This class wraps the basic.ino example which demonstrates:
 * - Sending broadcast messages at random intervals (1-5 seconds)
 * - Receiving and printing messages
 * - Handling connection events
 * - Time synchronization callbacks
 */
class BasicInoFirmware : public InoFirmwareWrapper {
public:
  BasicInoFirmware() : InoFirmwareWrapper("basic.ino"),
    taskSendMessage(TASK_SECOND * 1, TASK_FOREVER,
                    std::bind(&BasicInoFirmware::sendMessage, this)) {
    std::cout << "[INO] Basic firmware created\n";
  }

protected:
  InoFirmwareInterface createInoInterface() override {
    InoFirmwareInterface iface;
    iface.setup = std::bind(&BasicInoFirmware::ino_setup, this);
    iface.loop = std::bind(&BasicInoFirmware::ino_loop, this);
    iface.receivedCallback = std::bind(&BasicInoFirmware::ino_receivedCallback,
                                       this, std::placeholders::_1, std::placeholders::_2);
    iface.newConnectionCallback = std::bind(&BasicInoFirmware::ino_newConnectionCallback,
                                            this, std::placeholders::_1);
    iface.changedConnectionCallback = std::bind(&BasicInoFirmware::ino_changedConnectionCallback, this);
    iface.nodeTimeAdjustedCallback = std::bind(&BasicInoFirmware::ino_nodeTimeAdjustedCallback,
                                               this, std::placeholders::_1);
    return iface;
  }

private:
  // =======================================================================
  // .ino file functions - adapted from basic.ino
  // =======================================================================
  
  /**
   * @brief Send message function (called by task)
   * 
   * Original code from basic.ino:
   *   String msg = "Hello from node ";
   *   msg += mesh.getNodeId();
   *   mesh.sendBroadcast( msg );
   *   taskSendMessage.setInterval( random( TASK_SECOND * 1, TASK_SECOND * 5 ));
   */
  void sendMessage() {
    auto* mesh = getMesh();
    if (!mesh) return;
    
    std::ostringstream msg_stream;
    msg_stream << "Hello from node " << getNodeId();
    String msg = msg_stream.str();
    
    mesh->sendBroadcast(msg);
    messages_sent++;
    
    // Set random interval between 1-5 seconds (simulating basic.ino behavior)
    uint32_t random_interval = TASK_SECOND * (1 + (rand() % 4));
    taskSendMessage.setInterval(random_interval);
    
    if (messages_sent % 5 == 0) {
      std::cout << "[INO] basic.ino: Node " << getNodeId() 
                << " sent message #" << messages_sent << "\n";
    }
  }
  
  /**
   * @brief Setup function from basic.ino
   * 
   * Original code:
   *   mesh.setDebugMsgTypes( ERROR | STARTUP | CONNECTION );
   *   mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
   *   mesh.onReceive(&receivedCallback);
   *   mesh.onNewConnection(&newConnectionCallback);
   *   mesh.onChangedConnections(&changedConnectionCallback);
   *   mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
   *   userScheduler.addTask( taskSendMessage );
   *   taskSendMessage.enable();
   */
  void ino_setup() {
    std::cout << "[INO] basic.ino: setup() on node " << getNodeId() << "\n";
    
    auto* mesh = getMesh();
    auto* userScheduler = getScheduler();
    
    if (!mesh || !userScheduler) {
      std::cout << "[INO] basic.ino: ERROR - mesh or scheduler not initialized\n";
      return;
    }
    
    // Mesh is already initialized by the simulator framework
    // Callbacks are already registered by InoFirmwareWrapper
    
    // Add the periodic send message task
    userScheduler->addTask(taskSendMessage);
    taskSendMessage.enable();
    
    setup_completed = true;
    std::cout << "[INO] basic.ino: Setup complete on node " << getNodeId() << "\n";
  }
  
  /**
   * @brief Loop function from basic.ino
   * 
   * Original code:
   *   mesh.update();
   * 
   * Note: mesh.update() is called by the simulator framework
   */
  void ino_loop() {
    // Main loop - mesh.update() is called by simulator
    loop_count++;
  }
  
  /**
   * @brief Received callback from basic.ino
   * 
   * Original code:
   *   Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
   */
  void ino_receivedCallback(uint32_t from, String& msg) {
    messages_received++;
    
    // Track which nodes we've received from
    if (received_from.find(from) == received_from.end()) {
      received_from[from] = 0;
    }
    received_from[from]++;
    
    if (messages_received % 10 == 0) {
      std::cout << "[INO] basic.ino: Node " << getNodeId() 
                << " received message #" << messages_received 
                << " from " << from << "\n";
    }
  }
  
  /**
   * @brief New connection callback from basic.ino
   * 
   * Original code:
   *   Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
   */
  void ino_newConnectionCallback(uint32_t nodeId) {
    new_connections++;
    std::cout << "[INO] basic.ino: Node " << getNodeId() 
              << " new connection: " << nodeId << "\n";
  }
  
  /**
   * @brief Changed connections callback from basic.ino
   * 
   * Original code:
   *   Serial.printf("Changed connections\n");
   */
  void ino_changedConnectionCallback() {
    topology_changes++;
    
    auto* mesh = getMesh();
    if (mesh && topology_changes % 5 == 0) {
      auto node_list = mesh->getNodeList();
      std::cout << "[INO] basic.ino: Node " << getNodeId() 
                << " topology changed (connections: " << node_list.size() << ")\n";
    }
  }
  
  /**
   * @brief Time adjusted callback from basic.ino
   * 
   * Original code:
   *   Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
   */
  void ino_nodeTimeAdjustedCallback(int32_t offset) {
    time_adjustments++;
    
    if (time_adjustments % 10 == 0) {
      auto* mesh = getMesh();
      if (mesh) {
        std::cout << "[INO] basic.ino: Node " << getNodeId() 
                  << " time adjusted (offset: " << offset 
                  << " us, total adjustments: " << time_adjustments << ")\n";
      }
    }
  }

public:
  // Test tracking variables
  bool setup_completed = false;
  uint32_t loop_count = 0;
  uint32_t messages_sent = 0;
  uint32_t messages_received = 0;
  uint32_t new_connections = 0;
  uint32_t topology_changes = 0;
  uint32_t time_adjustments = 0;
  std::map<uint32_t, uint32_t> received_from;  // nodeId -> message count

private:
  Task taskSendMessage;
};

// Register firmware so it can be loaded by name
REGISTER_FIRMWARE(BasicInoFirmware, BasicInoFirmware)

} // namespace firmware
} // namespace simulator
