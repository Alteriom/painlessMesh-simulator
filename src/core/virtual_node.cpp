/**
 * @file virtual_node.cpp
 * @brief Implementation of VirtualNode class
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

// Include platform compatibility layer FIRST (before any painlessMesh headers)
#include "simulator/platform_compat.hpp"

#include "simulator/virtual_node.hpp"

#include <stdexcept>
#include <iostream>

// Include painlessMesh headers - order matters!
#define ARDUINO_ARCH_ESP8266
#define PAINLESSMESH_BOOST

#include <TaskSchedulerDeclarations.h>
#include "painlessmesh/mesh.hpp"
#include "painlessmesh/connection.hpp"
#include "boost/asynctcp.hpp"

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
      network_quality_(1.0f) {
  
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
  
  // Set up mesh callbacks
  mesh_->onReceive([this](uint32_t from, std::string& msg) {
    this->onReceive(from, msg);
  });
  
  mesh_->onNewConnection([this](uint32_t nodeId) {
    this->onNewConnection(nodeId);
  });
  
  mesh_->onChangedConnections([this]() {
    this->onChangedConnections();
  });
  
  running_ = true;
}

void VirtualNode::stop() {
  if (!running_) {
    return;
  }
  
  if (mesh_) {
    mesh_->stop();
  }
  
  running_ = false;
}

void VirtualNode::update() {
  if (!running_) {
    return;
  }
  
  if (mesh_) {
    mesh_->update();
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

void VirtualNode::onReceive(uint32_t from, std::string& msg) {
  metrics_.messages_received++;
  metrics_.bytes_received += msg.size();
  
  // Optional: Log received message for debugging
  // std::cout << "Node " << node_id_ << " received message from " << from 
  //           << " (" << msg.size() << " bytes)" << std::endl;
}

void VirtualNode::onNewConnection(uint32_t nodeId) {
  // Optional: Log new connection
  // std::cout << "Node " << node_id_ << " connected to " << nodeId << std::endl;
}

void VirtualNode::onChangedConnections() {
  // Optional: Log topology change
  // std::cout << "Node " << node_id_ << " topology changed" << std::endl;
}

} // namespace simulator
