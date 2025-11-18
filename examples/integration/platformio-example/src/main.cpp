/**
 * @file main.cpp
 * @brief ESP32 firmware entry point for MyMeshApp
 * 
 * This file is the PlatformIO/Arduino entry point for the ESP32.
 * It creates the mesh network and instantiates the application.
 */

#include <Arduino.h>
#include <MyMeshApp.h>

// Mesh configuration
#define MESH_PREFIX     "MyMeshNetwork"
#define MESH_PASSWORD   "password123"
#define MESH_PORT       5555

painlessMesh mesh;
Scheduler userScheduler;
MyMeshApp* app;

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nStarting mesh node...");
  
  // Configure mesh
  mesh.setDebugMsgTypes(ERROR | STARTUP);  // Adjust debug level as needed
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  
  // Create application
  app = new MyMeshApp(&mesh, &userScheduler);
  app->setup();
  
  // Register mesh callbacks to forward to application
  mesh.onReceive([](uint32_t from, String& msg) {
    app->onReceive(from, msg);
  });
  
  mesh.onNewConnection([](uint32_t nodeId) {
    app->onNewConnection(nodeId);
  });
  
  mesh.onChangedConnections([]() {
    app->onChangedConnections();
  });
  
  Serial.printf("Mesh node started with ID: %u\n", mesh.getNodeId());
}

void loop() {
  mesh.update();
  app->loop();
}
