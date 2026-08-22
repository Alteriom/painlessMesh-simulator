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
#include <thread>
#include <chrono>
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
    explicit_drops_.erase(linkKey(nodeId, peer));
    partition_cuts_.erase(linkKey(nodeId, peer));
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

size_t NodeManager::establishConnectivity(
    const std::vector<std::pair<uint32_t, uint32_t>>& links) {
  size_t wired = 0;
  for (const auto& link : links) {
    if (connectNodes(link.first, link.second)) {
      ++wired;
    }
  }
  return wired;
}

bool NodeManager::settleLink(uint32_t a, uint32_t b) {
  // A loopback handshake completes in a millisecond or two, so this returns
  // almost immediately for a link the mesh accepts. The budget is only spent
  // on a link painlessMesh declines to hold -- a redundant edge -- and 100ms
  // of that at startup beats the alternative, which was a mesh with no live
  // links at all.
  //
  // Returns whether both endpoints actually came up. A timeout is a real
  // failure: reporting a link "wired" or "re-established" when the handshake
  // never completed is exactly the false assurance settlement exists to
  // remove, so the caller must see it.
  constexpr int kSettleBudgetMs = 100;
  auto nodeA = getNode(a);
  auto nodeB = getNode(b);
  const auto deadline = std::chrono::steady_clock::now() +
                        std::chrono::milliseconds(kSettleBudgetMs);
  while (std::chrono::steady_clock::now() < deadline) {
    updateAll();
    if (nodeA && nodeB && nodeA->isConnectedTo(b) && nodeB->isConnectedTo(a)) {
      return true;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return false;
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
  // MeshTest::connect() only *starts* an asynchronous TCP connect. Every caller
  // -- startup wiring, a heal, a restore, a node rejoining -- has a next step
  // that assumes the link exists, and a same-second dependent event got a
  // refusal because it ran before the handshake completed. Settle here so no
  // caller has to remember, and report the truth: if the link never came up,
  // this is not a connection, and returning true would let startup count it
  // wired and a heal report it restored while dependent traffic is still
  // refused. The topology edge stays recorded -- the scenario still intends
  // it, and a later heal or reconnect may bring it up -- but the count does
  // not lie about now.
  return settleLink(fromNode, toNode);
}

size_t NodeManager::severConnection(uint32_t a, uint32_t b) {
  size_t closed = 0;
  auto nodeA = getNode(a);
  auto nodeB = getNode(b);
  // Close from both ends: the FIN propagates, but doing it explicitly makes
  // the cut take effect within the same update tick rather than the next one.
  if (nodeA && nodeA->disconnectFrom(b)) ++closed;
  if (nodeB && nodeB->disconnectFrom(a)) ++closed;
  return closed;
}

size_t NodeManager::dropLink(uint32_t a, uint32_t b) {
  // A link can be down for more than one reason at once -- an explicit
  // connection_drop AND an active partition crossing the same edge. Each reason
  // is tracked independently, and the link comes back only when *every* reason
  // is cleared: connection_restore clears the explicit drop, heal_partition
  // clears the partition cut. So this records the explicit reason and leaves
  // any partition reason untouched.
  explicit_drops_.insert(linkKey(a, b));
  return severConnection(a, b);
}

NodeManager::RestoreOutcome NodeManager::restoreLink(uint32_t a, uint32_t b) {
  // A restore re-establishes a link the topology declared; it is not a licence
  // to invent a new route. connectNodes() records a fresh topology edge, so
  // restoring a pair the declared topology never had would silently add an
  // undeclared link and change later partition, heal and reconnect behaviour.
  // Refuse it -- the same guard healNetwork() already applies to its edges.
  if (!topology_.count(a) || !topology_.at(a).count(b)) {
    return RestoreOutcome::NothingToDo;
  }
  // connection_restore is the counterpart of connection_drop: it clears the
  // explicit reason only. If an active partition still cuts this edge, the link
  // stays down until heal_partition -- reconnecting it here would bridge the
  // two groups early. Leave the partition cut in place and defer.
  explicit_drops_.erase(linkKey(a, b));
  if (partition_cuts_.count(linkKey(a, b))) {
    return RestoreOutcome::NothingToDo;  // still partitioned; the heal restores it
  }

  auto nodeA = getNode(a);
  auto nodeB = getNode(b);
  if (!nodeA || !nodeB || !nodeA->isRunning() || !nodeB->isRunning()) {
    return RestoreOutcome::NothingToDo;  // a node is down; start reconnects it
  }
  if (nodeA->isConnectedTo(b) || nodeB->isConnectedTo(a)) {
    return RestoreOutcome::NothingToDo;  // already live
  }
  // Both endpoints are up and the link should come back. If the handshake does
  // not settle, that is a real failure the caller must surface, not a no-op.
  return connectNodes(a, b) ? RestoreOutcome::Reestablished
                            : RestoreOutcome::Failed;
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
          severConnection(x, y);
          // Record the partition reason. It coexists with any explicit drop on
          // the same edge: a heal clears only this reason, so an edge that is
          // also explicitly dropped stays down until its own restore.
          partition_cuts_.insert(linkKey(x, y));
          if (wired) ++cut;
        }
      }
    }
  }
  return cut;
}

NodeManager::HealResult NodeManager::healNetwork() {
  // Heal only the links a partition cut. An explicit connection_drop is a
  // deliberate, persistent failure with its own connection_restore -- it is
  // never in partition_cuts_, so it is never touched here.
  //
  // A cut that cannot be rebuilt now -- a node still down, or a handshake that
  // does not settle -- is kept pending rather than discarded, so a later heal
  // can retry it instead of leaving the mesh partitioned with no state to act
  // on.
  std::set<std::pair<uint32_t, uint32_t>> pending;
  HealResult result;
  for (const auto& link : partition_cuts_) {
    // Only edges the topology actually had are worth rebuilding; a partition
    // marks every cross pair cut, most of which were never wired. A marker on a
    // non-edge is simply spent.
    if (!topology_.count(link.first) || !topology_.at(link.first).count(link.second)) {
      continue;
    }
    // The partition reason is resolved for this edge either way. If the edge is
    // also an explicit drop, it stays down under that reason -- do not reconnect
    // it, and do not keep it as a partition cut.
    if (explicit_drops_.count(link)) {
      continue;
    }
    auto a = getNode(link.first);
    auto b = getNode(link.second);
    if (!a || !b || !a->isRunning() || !b->isRunning()) {
      // A node is down: the partition has still ended, so release the cut
      // rather than holding it. The edge is no longer severed, so when that
      // node starts again reconnectNode() re-establishes it -- a second heal
      // is not required. Holding it here instead would leave the node
      // permanently detached, since reconnectNode() skips severed edges.
      continue;
    }
    if (a->isConnectedTo(link.second) || b->isConnectedTo(link.first)) {
      continue;  // already live
    }
    if (connectNodes(link.first, link.second)) {
      ++result.restored;
    } else {
      // Both endpoints are up but the handshake did not settle -- a genuine
      // transient failure. Retain it so a later heal retries; reconnectNode()
      // will not help here because neither node is restarting. Report it so the
      // heal event can surface a heal that did not complete.
      pending.insert(link);
      ++result.failed;
    }
  }
  partition_cuts_ = pending;
  return result;
}

NodeManager::ReconnectResult NodeManager::reconnectNode(uint32_t nodeId) {
  ReconnectResult result;
  auto node = getNode(nodeId);
  if (!node || !node->isRunning()) {
    return result;
  }
  auto peers = topology_.find(nodeId);
  if (peers == topology_.end()) {
    return result;
  }

  // Copy: connectNodes() writes to topology_, which would invalidate the
  // iteration over the very set we are walking.
  const std::set<uint32_t> peer_ids = peers->second;
  for (uint32_t peer_id : peer_ids) {
    if (isLinkSevered(nodeId, peer_id)) continue;
    auto peer = getNode(peer_id);
    if (!peer || !peer->isRunning()) continue;
    // Judge on the restarted node's own view only. The peer can still be
    // holding a connection object for the socket this node closed on its way
    // down -- it learns otherwise on its next poll -- and treating that stale
    // view as "already connected" leaves the node permanently detached. If the
    // link really is live, the node's own view says so.
    if (node->isConnectedTo(peer_id)) continue;
    // This peer is eligible and should reconnect. A false here is a genuine
    // settlement failure, distinct from the severed/down/already-live skips
    // above -- the caller surfaces it rather than reporting a clean rejoin.
    if (connectNodes(nodeId, peer_id)) {
      ++result.reconnected;
    } else {
      ++result.failed;
    }
  }
  return result;
}

bool NodeManager::isLinkSevered(uint32_t a, uint32_t b) const {
  const auto key = linkKey(a, b);
  return explicit_drops_.count(key) > 0 || partition_cuts_.count(key) > 0;
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
