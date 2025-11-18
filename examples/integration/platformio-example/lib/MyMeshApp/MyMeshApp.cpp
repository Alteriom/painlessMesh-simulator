#include "MyMeshApp.h"

MyMeshApp::MyMeshApp(painlessMesh* mesh, Scheduler* scheduler)
  : mesh_(mesh), 
    scheduler_(scheduler),
    messagesSent_(0),
    messagesReceived_(0) {
  nodeId_ = mesh_->getNodeId();
}

void MyMeshApp::setup() {
  // Setup periodic status broadcast every 30 seconds
  statusTask_.set(30 * TASK_SECOND, TASK_FOREVER, [this]() {
    sendStatusUpdate();
  });
  
  scheduler_->addTask(statusTask_);
  statusTask_.enable();
}

void MyMeshApp::loop() {
  // Fast loop logic (if any)
  // Most periodic work should use the scheduler
}

void MyMeshApp::onReceive(uint32_t from, String& msg) {
  messagesReceived_++;
  
  // Simple message routing based on prefix
  if (msg.startsWith("PING:")) {
    // Respond to ping requests
    String response = "PONG:" + String(nodeId_);
    mesh_->sendSingle(from, response);
    messagesSent_++;
  } else if (msg.startsWith("STATUS:")) {
    // Process status updates from other nodes
    // In a real application, you might parse JSON here
  }
}

void MyMeshApp::onNewConnection(uint32_t nodeId) {
  // Broadcast announcement when new node joins
  String announcement = "HELLO:" + String(nodeId_) + " connected to " + String(nodeId_);
  mesh_->sendBroadcast(announcement);
  messagesSent_++;
}

void MyMeshApp::onChangedConnections() {
  // Handle topology changes
  auto nodeList = mesh_->getNodeList();
  // In a real application, you might:
  // - Update routing tables
  // - Resync data
  // - Adjust communication patterns
}

void MyMeshApp::sendStatusUpdate() {
  // Create status message
  String status = "STATUS:" + String(nodeId_) + 
                  ":TX=" + String(messagesSent_) +
                  ":RX=" + String(messagesReceived_) +
                  ":TIME=" + String(mesh_->getNodeTime());
  
  mesh_->sendBroadcast(status);
  messagesSent_++;
}
