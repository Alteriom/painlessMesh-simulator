/**
 * @file virtual_node.cpp
 * @brief Implementation of VirtualNode class
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

// IMPORTANT: Include platform_compat.hpp FIRST on Windows
#include "simulator/platform_compat.hpp"

// Include Arduino compatibility header which sets up proper header ordering
// This Arduino.h is in include/simulator/boost/Arduino.h and will be found
// via the include path set in CMakeLists.txt
#include "Arduino.h"

#include "simulator/virtual_node.hpp"
#include "simulator/firmware/firmware_base.hpp"
#include "simulator/firmware/firmware_factory.hpp"

#include <stdexcept>
#include <iostream>

// Include painlessMesh headers
#include <TaskSchedulerDeclarations.h>
#include "painlessmesh/mesh.hpp"
#include "painlessmesh/connection.hpp"

namespace painlessmesh {
namespace logger {
extern LogClass Log;
}
}

using PMesh = painlessmesh::Mesh<painlessmesh::Connection>;

/**
 * @brief MeshTest wrapper class for testing
 * 
 * This class extends painlessMesh to work with Boost.Asio
 * for network simulation.
 */
class MeshTest : public PMesh {
public:
  MeshTest(Scheduler* scheduler, uint32_t id, boost::asio::io_context& io)
      : io_service(io) {
    this->nodeId = id;
    this->init(scheduler, this->nodeId);
    pServer = std::make_shared<AsyncServer>(io_service, this->nodeId);
    painlessmesh::tcp::initServer<painlessmesh::Connection, PMesh>(*pServer, (*this));
  }

  void connect(MeshTest& mesh) {
    auto pClient = new AsyncClient(io_service);
    painlessmesh::tcp::connect<painlessmesh::Connection, PMesh>(
        (*pClient), 
        boost::asio::ip::make_address("127.0.0.1"), 
        mesh.nodeId,
        (*this));
  }

  std::shared_ptr<AsyncServer> pServer;
  boost::asio::io_context& io_service;
};

namespace simulator {

VirtualNode::VirtualNode(uint32_t nodeId, 
                         const NodeConfig& config,
                         Scheduler* scheduler,
                         boost::asio::io_context& io)
    : node_id_(nodeId),
      scheduler_(scheduler),
      io_(io),
      running_(false),
      network_quality_(1.0f),
      config_(config) {
  
  // Validate node ID
  if (nodeId == 0) {
    throw std::invalid_argument("Node ID must be non-zero");
  }
  
  // Validate scheduler
  if (scheduler == nullptr) {
    throw std::invalid_argument("Scheduler cannot be null");
  }
  
  // Initialize metrics with current timestamp
  metrics_.start_time = std::chrono::steady_clock::now();
  metrics_.messages_sent = 0;
  metrics_.messages_received = 0;
  metrics_.bytes_sent = 0;
  metrics_.bytes_received = 0;
  
  // Create mesh instance
  try {
    mesh_ = std::make_unique<MeshTest>(scheduler_, node_id_, io_);
  } catch (const std::exception& e) {
    throw std::runtime_error(std::string("Failed to create mesh instance: ") + e.what());
  }
}

VirtualNode::~VirtualNode() {
  if (running_) {
    stop();
  }
}

void VirtualNode::start() {
  if (running_) {
    throw std::runtime_error("Node is already running");
  }
  
  if (!mesh_) {
    throw std::runtime_error("Mesh instance not initialized");
  }
  
  // Record start time
  metrics_.start_time = std::chrono::steady_clock::now();
  
  // Set up mesh callbacks (will route to firmware if loaded)
  routeCallbacksToFirmware();
  
  // Setup firmware after mesh initialized
  setupFirmware();
  
  running_ = true;
  
  std::cout << "[INFO] Node " << node_id_ << " started";
  if (firmware_) {
    std::cout << " with firmware: " << firmware_->getName();
  }
  std::cout << std::endl;
}

void VirtualNode::stop() {
  if (!running_) {
    return;
  }
  
  // Update uptime before stopping
  auto now = std::chrono::steady_clock::now();
  auto uptime = std::chrono::duration_cast<std::chrono::milliseconds>(
    now - metrics_.start_time).count();
  metrics_.total_uptime_ms += uptime;
  
  if (mesh_) {
    mesh_->stop();
  }
  
  running_ = false;
}

void VirtualNode::crash() {
  if (!running_) {
    return;
  }
  
  // Update uptime and crash count
  auto now = std::chrono::steady_clock::now();
  auto uptime = std::chrono::duration_cast<std::chrono::milliseconds>(
    now - metrics_.start_time).count();
  metrics_.total_uptime_ms += uptime;
  metrics_.crash_count++;
  
  // Abrupt stop - no cleanup, simulating power failure
  // We still call mesh_->stop() but this represents an ungraceful shutdown
  if (mesh_) {
    mesh_->stop();
  }
  
  running_ = false;
}

void VirtualNode::restart() {
  // Graceful stop followed by start
  stop();
  start();
}

void VirtualNode::update() {
  if (!running_) {
    return;
  }
  
  if (mesh_) {
    mesh_->update();
  }
  
  // Call firmware loop
  if (firmware_ && firmware_initialized_) {
    firmware_->loop();
  }
  
  // Poll IO context to process network events
  io_.poll();
}

painlessmesh::Mesh<painlessmesh::Connection>& VirtualNode::getMesh() {
  if (!mesh_) {
    throw std::runtime_error("Mesh instance not initialized");
  }
  return *mesh_;
}

const painlessmesh::Mesh<painlessmesh::Connection>& VirtualNode::getMesh() const {
  if (!mesh_) {
    throw std::runtime_error("Mesh instance not initialized");
  }
  return *mesh_;
}

void VirtualNode::setNetworkQuality(float quality) {
  if (quality < 0.0f || quality > 1.0f) {
    throw std::invalid_argument("Network quality must be between 0.0 and 1.0");
  }
  network_quality_ = quality;
  // TODO: Implement actual network quality simulation
}

void VirtualNode::connectTo(VirtualNode& other) {
  if (!mesh_) {
    throw std::runtime_error("Mesh instance not initialized");
  }
  
  if (!other.mesh_) {
    throw std::runtime_error("Target mesh instance not initialized");
  }
  
  // Connect this node to the other node
  mesh_->connect(*other.mesh_);
}

void VirtualNode::onReceive(uint32_t from, std::string& msg) {
  metrics_.messages_received++;
  metrics_.bytes_received += msg.size();
  
  // Route to firmware if loaded
  if (firmware_ && firmware_initialized_) {
    String msgStr(msg.c_str());
    firmware_->onReceive(from, msgStr);
  }
  
  // Optional: Log received message for debugging
  // std::cout << "Node " << node_id_ << " received message from " << from 
  //           << " (" << msg.size() << " bytes)" << std::endl;
}

void VirtualNode::onNewConnection(uint32_t nodeId) {
  // Route to firmware if loaded
  if (firmware_ && firmware_initialized_) {
    firmware_->onNewConnection(nodeId);
  }
  
  // Optional: Log new connection
  // std::cout << "Node " << node_id_ << " connected to " << nodeId << std::endl;
}

void VirtualNode::onChangedConnections() {
  // Route to firmware if loaded
  if (firmware_ && firmware_initialized_) {
    firmware_->onChangedConnections();
  }
  
  // Optional: Log topology change
  // std::cout << "Node " << node_id_ << " topology changed" << std::endl;
}

uint64_t VirtualNode::getUptime() const {
  if (!running_) {
    return 0;
  }
  
  auto now = std::chrono::steady_clock::now();
  auto uptime = std::chrono::duration_cast<std::chrono::milliseconds>(
    now - metrics_.start_time).count();
  return static_cast<uint64_t>(uptime);
}

bool VirtualNode::loadFirmware(const std::string& firmwareName) {
  if (firmwareName.empty()) {
    return true;  // No firmware is valid (Phase 1 behavior)
  }
  
  firmware_ = firmware::FirmwareFactory::instance().create(firmwareName);
  if (!firmware_) {
    std::cerr << "[ERROR] Failed to load firmware: " << firmwareName 
              << " for node " << node_id_ << std::endl;
    return false;
  }
  
  std::cout << "[INFO] Loaded firmware '" << firmware_->getName() 
            << "' for node " << node_id_ << std::endl;
  return true;
}

void VirtualNode::loadFirmware(std::unique_ptr<firmware::FirmwareBase> firmware) {
  firmware_ = std::move(firmware);
  if (firmware_) {
    std::cout << "[INFO] Loaded firmware '" << firmware_->getName() 
              << "' for node " << node_id_ << std::endl;
  }
}

bool VirtualNode::hasFirmware() const {
  return firmware_ != nullptr;
}

firmware::FirmwareBase* VirtualNode::getFirmware() const {
  return firmware_.get();
}

void VirtualNode::setupFirmware() {
  if (!firmware_ || firmware_initialized_) {
    return;
  }
  
  // Convert config to map
  std::map<String, String> configMap;
  configMap[String("mesh_prefix")] = String(config_.meshPrefix.c_str());
  configMap[String("mesh_password")] = String(config_.meshPassword.c_str());
  
  // Add custom firmware config
  for (const auto& pair : config_.firmwareConfig) {
    configMap[String(pair.first.c_str())] = String(pair.second.c_str());
  }
  
  // Initialize firmware
  firmware_->initialize(mesh_.get(), scheduler_, node_id_, configMap);
  
  // Call firmware setup
  firmware_->setup();
  firmware_initialized_ = true;
  
  std::cout << "[INFO] Firmware '" << firmware_->getName() 
            << "' initialized for node " << node_id_ << std::endl;
}

void VirtualNode::routeCallbacksToFirmware() {
  // Route onReceive callback
  mesh_->onReceive([this](uint32_t from, std::string& msg) {
    this->onReceive(from, msg);
  });
  
  // Route onNewConnection callback
  mesh_->onNewConnection([this](uint32_t nodeId) {
    this->onNewConnection(nodeId);
  });
  
  // Route onChangedConnections callback
  mesh_->onChangedConnections([this]() {
    this->onChangedConnections();
  });
}

} // namespace simulator
