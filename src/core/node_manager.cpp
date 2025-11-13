/**
 * @file node_manager.cpp
 * @brief Implementation of NodeManager class
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

// IMPORTANT: Include platform_compat.hpp FIRST on Windows
#include "simulator/platform_compat.hpp"

#include "simulator/node_manager.hpp"
#include "simulator/virtual_node.hpp"
#include <stdexcept>
#include <cstdlib>
#include <TaskSchedulerDeclarations.h>

namespace simulator {

NodeManager::NodeManager(boost::asio::io_context& io)
  : io_(io)
  , scheduler_(new Scheduler())
{
}

NodeManager::~NodeManager() {
  // Stop all nodes before destruction
  stopAll();
}

std::shared_ptr<VirtualNode> NodeManager::createNode(const NodeConfig& config) {
  // Validate node ID
  if (config.nodeId == 0) {
    throw std::invalid_argument("Node ID must be non-zero");
  }
  
  // Check for duplicate node ID
  if (nodes_.count(config.nodeId) > 0) {
    throw std::runtime_error("Node ID already exists: " + std::to_string(config.nodeId));
  }
  
  // Enforce maximum node limit
  if (nodes_.size() >= MAX_NODES) {
    throw std::runtime_error("Maximum node count reached: " + std::to_string(MAX_NODES));
  }
  
  // Create the node
  auto node = std::make_shared<VirtualNode>(
    config.nodeId,
    config,
    scheduler_.get(),
    io_
  );
  
  // Store in map
  nodes_[config.nodeId] = node;
  
  return node;
}

bool NodeManager::removeNode(uint32_t nodeId) {
  auto it = nodes_.find(nodeId);
  if (it == nodes_.end()) {
    return false;
  }
  
  // Stop the node if it's running
  if (it->second->isRunning()) {
    it->second->stop();
  }
  
  // Remove from map
  nodes_.erase(it);
  
  return true;
}

void NodeManager::startAll() {
  for (auto& pair : nodes_) {
    if (!pair.second->isRunning()) {
      pair.second->start();
    }
  }
}

void NodeManager::stopAll() {
  for (auto& pair : nodes_) {
    if (pair.second->isRunning()) {
      pair.second->stop();
    }
  }
}

void NodeManager::updateAll() {
  // Process scheduler tasks
  scheduler_->execute();
  
  // Update each node
  for (auto& pair : nodes_) {
    pair.second->update();
  }
  
  // Poll IO context to process network events
  io_.poll();
}

void NodeManager::establishConnectivity() {
  if (nodes_.empty()) {
    return;
  }
  
  // Create a vector of node pointers for easier access
  std::vector<std::shared_ptr<VirtualNode>> node_list;
  node_list.reserve(nodes_.size());
  for (auto& pair : nodes_) {
    node_list.push_back(pair.second);
  }
  
  // Connect each node (starting from the second) to a random previous node
  // This creates a connected tree topology
  for (size_t i = 1; i < node_list.size(); ++i) {
    // Connect to a random node among the previously added nodes
    size_t target_idx = std::rand() % i;
    node_list[i]->connectTo(*node_list[target_idx]);
  }
}

std::shared_ptr<VirtualNode> NodeManager::getNode(uint32_t nodeId) {
  auto it = nodes_.find(nodeId);
  if (it != nodes_.end()) {
    return it->second;
  }
  return nullptr;
}

std::shared_ptr<const VirtualNode> NodeManager::getNode(uint32_t nodeId) const {
  auto it = nodes_.find(nodeId);
  if (it != nodes_.end()) {
    return it->second;
  }
  return nullptr;
}

std::vector<uint32_t> NodeManager::getNodeIds() const {
  std::vector<uint32_t> ids;
  ids.reserve(nodes_.size());
  
  for (const auto& pair : nodes_) {
    ids.push_back(pair.first);
  }
  
  return ids;
}

std::vector<std::shared_ptr<VirtualNode>> NodeManager::getAllNodes() const {
  std::vector<std::shared_ptr<VirtualNode>> result;
  result.reserve(nodes_.size());
  
  for (const auto& pair : nodes_) {
    result.push_back(pair.second);
  }
  
  return result;
}

bool NodeManager::hasNode(uint32_t nodeId) const {
  return nodes_.count(nodeId) > 0;
}

} // namespace simulator
