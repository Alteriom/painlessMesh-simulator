#pragma once

#include <painlessMesh.h>
#include <TaskSchedulerDeclarations.h>

/**
 * @brief Example mesh application
 * 
 * This class demonstrates platform-agnostic mesh application code
 * that works on both ESP32 hardware and in the simulator.
 * 
 * Key design principles:
 * - Takes mesh and scheduler as constructor parameters (dependency injection)
 * - No platform-specific code (Arduino.h, Serial, digitalWrite, etc.)
 * - Business logic only
 */
class MyMeshApp {
public:
  /**
   * @brief Construct mesh application
   * @param mesh Pointer to painlessMesh instance
   * @param scheduler Pointer to task scheduler
   */
  MyMeshApp(painlessMesh* mesh, Scheduler* scheduler);
  
  /**
   * @brief Initialize the application
   * 
   * Called once after mesh initialization.
   * Sets up tasks and initial state.
   */
  void setup();
  
  /**
   * @brief Main application loop
   * 
   * Called frequently. Keep fast - use scheduler for periodic tasks.
   */
  void loop();
  
  /**
   * @brief Handle received mesh message
   * @param from Sender node ID
   * @param msg Message content
   */
  void onReceive(uint32_t from, String& msg);
  
  /**
   * @brief Handle new mesh connection
   * @param nodeId Newly connected node ID
   */
  void onNewConnection(uint32_t nodeId);
  
  /**
   * @brief Handle mesh topology change
   * 
   * Called when the mesh topology changes (nodes join/leave).
   */
  void onChangedConnections();

private:
  painlessMesh* mesh_;
  Scheduler* scheduler_;
  uint32_t nodeId_;
  
  // Periodic status broadcast task
  Task statusTask_;
  void sendStatusUpdate();
  
  // Message counters
  uint32_t messagesSent_;
  uint32_t messagesReceived_;
};
