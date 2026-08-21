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
#include <algorithm>
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
  
  // Load firmware if specified
  if (!config.firmware.empty()) {
    if (!node->loadFirmware(config.firmware)) {
      std::cerr << "[ERROR] Failed to load firmware for node " << config.nodeId << std::endl;
      // Keep the node so an interactive run still shows the rest of the mesh, but
      // record the failure -- callers gating CI on this run must be able to see it.
      ++firmware_load_failures_;
    }
  }
  
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

  // Forget its edges too, or a later heal/reconnect would try to wire a node
  // that no longer exists.
  for (uint32_t peer : topology_[nodeId]) {
    topology_[peer].erase(nodeId);
    severed_.erase(linkKey(nodeId, peer));
  }
  topology_.erase(nodeId);

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
    connectNodes(node_list[i]->getNodeId(), node_list[target_idx]->getNodeId());
  }
}

bool NodeManager::connectNodes(uint32_t fromNode, uint32_t toNode) {
  if (fromNode == toNode) {
    return false;
  }
  auto from = getNode(fromNode);
  auto to = getNode(toNode);
  if (!from || !to) {
    return false;
  }

  from->connectTo(*to);
  topology_[fromNode].insert(toNode);
  topology_[toNode].insert(fromNode);
  return true;
}

size_t NodeManager::dropLink(uint32_t a, uint32_t b) {
  severed_.insert(linkKey(a, b));

  size_t closed = 0;
  auto nodeA = getNode(a);
  auto nodeB = getNode(b);
  // Close from both ends: the FIN propagates, but doing it explicitly makes
  // the cut take effect within the same update tick rather than the next one.
  if (nodeA && nodeA->disconnectFrom(b)) ++closed;
  if (nodeB && nodeB->disconnectFrom(a)) ++closed;
  return closed;
}

bool NodeManager::restoreLink(uint32_t a, uint32_t b) {
  severed_.erase(linkKey(a, b));

  auto nodeA = getNode(a);
  auto nodeB = getNode(b);
  if (!nodeA || !nodeB || !nodeA->isRunning() || !nodeB->isRunning()) {
    return false;
  }
  if (nodeA->isConnectedTo(b) || nodeB->isConnectedTo(a)) {
    return false;  // already live
  }
  return connectNodes(a, b);
}

size_t NodeManager::partitionNetwork(
    const std::vector<std::vector<uint32_t>>& groups) {
  size_t cut = 0;
  for (size_t i = 0; i < groups.size(); ++i) {
    for (size_t j = i + 1; j < groups.size(); ++j) {
      for (uint32_t x : groups[i]) {
        for (uint32_t y : groups[j]) {
          // Mark every cross pair severed, not just the ones that happen to be
          // wired today: otherwise a later reconnectNode() would quietly bridge
          // the partition back together.
          const bool wired = topology_.count(x) && topology_.at(x).count(y);
          dropLink(x, y);
          if (wired) ++cut;
        }
      }
    }
  }
  return cut;
}

size_t NodeManager::healNetwork() {
  const auto severed = severed_;
  severed_.clear();

  size_t restored = 0;
  for (const auto& link : severed) {
    // Only edges the topology actually had are worth rebuilding; a partition
    // marks every cross pair severed, most of which were never wired.
    if (!topology_.count(link.first) || !topology_.at(link.first).count(link.second)) {
      continue;
    }
    auto a = getNode(link.first);
    auto b = getNode(link.second);
    if (!a || !b || !a->isRunning() || !b->isRunning()) continue;
    if (a->isConnectedTo(link.second) || b->isConnectedTo(link.first)) continue;
    if (connectNodes(link.first, link.second)) ++restored;
  }
  return restored;
}

size_t NodeManager::reconnectNode(uint32_t nodeId) {
  auto node = getNode(nodeId);
  if (!node || !node->isRunning()) {
    return 0;
  }
  auto peers = topology_.find(nodeId);
  if (peers == topology_.end()) {
    return 0;
  }

  size_t reconnected = 0;
  // Copy: connectNodes() writes to topology_, which would invalidate the
  // iteration over the very set we are walking.
  const std::set<uint32_t> peer_ids = peers->second;
  for (uint32_t peer_id : peer_ids) {
    if (isLinkSevered(nodeId, peer_id)) continue;
    auto peer = getNode(peer_id);
    if (!peer || !peer->isRunning()) continue;
    if (node->isConnectedTo(peer_id) || peer->isConnectedTo(nodeId)) continue;
    if (connectNodes(nodeId, peer_id)) ++reconnected;
  }
  return reconnected;
}

bool NodeManager::isLinkSevered(uint32_t a, uint32_t b) const {
  return severed_.count(linkKey(a, b)) > 0;
}

std::vector<uint32_t> NodeManager::getRecordedPeers(uint32_t nodeId) const {
  auto it = topology_.find(nodeId);
  if (it == topology_.end()) {
    return {};
  }
  return std::vector<uint32_t>(it->second.begin(), it->second.end());
}

size_t NodeManager::getTotalConnectionCount() const {
  size_t total = 0;
  for (const auto& pair : nodes_) {
    if (pair.second) total += pair.second->getConnectionCount();
  }
  return total;
}

std::vector<std::vector<uint32_t>> NodeManager::getConnectedComponents() const {
  std::vector<std::vector<uint32_t>> components;
  std::set<uint32_t> unvisited;
  for (const auto& pair : nodes_) unvisited.insert(pair.first);

  while (!unvisited.empty()) {
    std::vector<uint32_t> component;
    std::vector<uint32_t> frontier{*unvisited.begin()};
    unvisited.erase(unvisited.begin());

    while (!frontier.empty()) {
      const uint32_t current = frontier.back();
      frontier.pop_back();
      component.push_back(current);

      auto peers = topology_.find(current);
      if (peers == topology_.end()) continue;
      for (uint32_t peer : peers->second) {
        if (!unvisited.count(peer)) continue;
        if (isLinkSevered(current, peer)) continue;
        unvisited.erase(peer);
        frontier.push_back(peer);
      }
    }

    std::sort(component.begin(), component.end());
    components.push_back(std::move(component));
  }

  std::sort(components.begin(), components.end());
  return components;
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
