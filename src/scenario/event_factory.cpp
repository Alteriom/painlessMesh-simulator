#include "simulator/event_factory.hpp"

#include "simulator/events/connection_degrade_event.hpp"
#include "simulator/events/connection_drop_event.hpp"
#include "simulator/events/connection_restore_event.hpp"
#include "simulator/events/network_heal_event.hpp"
#include "simulator/events/network_partition_event.hpp"
#include "simulator/events/node_crash_event.hpp"
#include "simulator/events/node_restart_event.hpp"
#include "simulator/events/node_start_event.hpp"
#include "simulator/events/node_stop_event.hpp"

#include <iostream>

namespace simulator {
namespace {

/**
 * @brief Resolves a scenario node id to its numeric id
 * @throws std::runtime_error when the scenario names a node it never declared
 */
uint32_t resolve(const std::string& id,
                 const std::map<std::string, uint32_t>& idToNodeId) {
  auto it = idToNodeId.find(id);
  if (it == idToNodeId.end()) {
    throw std::runtime_error("event targets unknown node: " + id);
  }
  return it->second;
}

/// Resolves the first of `target` / `targets[0]` that is set.
uint32_t resolveTarget(const EventConfig& c,
                       const std::map<std::string, uint32_t>& idToNodeId) {
  if (!c.target.empty()) return resolve(c.target, idToNodeId);
  if (!c.targets.empty()) return resolve(c.targets.front(), idToNodeId);
  throw std::runtime_error("event has no target node");
}

/// Resolves a link event's two endpoints from `targets` or `from`/`to`.
std::pair<uint32_t, uint32_t> resolveLink(
    const EventConfig& c, const std::map<std::string, uint32_t>& idToNodeId) {
  if (c.targets.size() >= 2) {
    return {resolve(c.targets[0], idToNodeId), resolve(c.targets[1], idToNodeId)};
  }
  if (!c.from.empty() && !c.to.empty()) {
    return {resolve(c.from, idToNodeId), resolve(c.to, idToNodeId)};
  }
  throw std::runtime_error("connection event needs two endpoints");
}

}  // namespace

std::unique_ptr<Event> EventFactory::create(
    const EventConfig& config,
    const std::map<std::string, uint32_t>& idToNodeId) {
  switch (config.action) {
    case EventAction::START_NODE:
      return std::unique_ptr<Event>(
          new NodeStartEvent(resolveTarget(config, idToNodeId)));

    case EventAction::STOP_NODE:
      return std::unique_ptr<Event>(
          new NodeStopEvent(resolveTarget(config, idToNodeId), config.graceful));

    case EventAction::CRASH_NODE:
      return std::unique_ptr<Event>(
          new NodeCrashEvent(resolveTarget(config, idToNodeId)));

    case EventAction::RESTART_NODE:
      return std::unique_ptr<Event>(
          new NodeRestartEvent(resolveTarget(config, idToNodeId)));

    case EventAction::CONNECTION_DROP: {
      auto link = resolveLink(config, idToNodeId);
      return std::unique_ptr<Event>(
          new ConnectionDropEvent(link.first, link.second));
    }

    case EventAction::CONNECTION_RESTORE: {
      auto link = resolveLink(config, idToNodeId);
      return std::unique_ptr<Event>(
          new ConnectionRestoreEvent(link.first, link.second));
    }

    case EventAction::CONNECTION_DEGRADE: {
      auto link = resolveLink(config, idToNodeId);
      return std::unique_ptr<Event>(new ConnectionDegradeEvent(
          link.first, link.second, config.latency, config.packet_loss));
    }

    case EventAction::PARTITION_NETWORK: {
      std::vector<std::vector<uint32_t>> groups;
      groups.reserve(config.groups.size());
      for (const auto& group : config.groups) {
        std::vector<uint32_t> resolved;
        resolved.reserve(group.size());
        for (const auto& id : group) resolved.push_back(resolve(id, idToNodeId));
        groups.push_back(std::move(resolved));
      }
      if (groups.size() < 2) {
        throw std::runtime_error("partition needs at least two groups");
      }
      return std::unique_ptr<Event>(new NetworkPartitionEvent(groups));
    }

    case EventAction::HEAL_PARTITION:
      return std::unique_ptr<Event>(new NetworkHealEvent());

    // Parsed and validated by the config loader, but with no runtime
    // implementation yet. Reported to the caller rather than dropped silently.
    case EventAction::REMOVE_NODE:
    case EventAction::ADD_NODES:
    case EventAction::BREAK_LINK:
    case EventAction::RESTORE_LINK:
    case EventAction::INJECT_MESSAGE:
    case EventAction::SET_NETWORK_QUALITY:
    default:
      return nullptr;
  }
}

size_t EventFactory::scheduleAll(
    const std::vector<EventConfig>& events,
    const std::map<std::string, uint32_t>& idToNodeId,
    EventScheduler& scheduler,
    std::vector<std::string>& skipped) {
  size_t scheduled = 0;
  for (const auto& config : events) {
    try {
      auto event = create(config, idToNodeId);
      if (!event) {
        skipped.push_back("t=" + std::to_string(config.time) + " " +
                          (config.description.empty() ? std::string("event")
                                                      : config.description) +
                          " (action not implemented)");
        continue;
      }
      scheduler.scheduleEvent(std::move(event), config.time);
      ++scheduled;
    } catch (const std::exception& e) {
      skipped.push_back("t=" + std::to_string(config.time) + " " +
                        (config.description.empty() ? std::string("event")
                                                    : config.description) +
                        " (" + e.what() + ")");
    }
  }
  return scheduled;
}

}  // namespace simulator
