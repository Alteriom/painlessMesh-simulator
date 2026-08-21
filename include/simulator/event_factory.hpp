#ifndef SIMULATOR_EVENT_FACTORY_HPP
#define SIMULATOR_EVENT_FACTORY_HPP

#include "simulator/config_loader.hpp"
#include "simulator/event.hpp"
#include "simulator/event_scheduler.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace simulator {

/**
 * @brief Translates parsed scenario events into schedulable Event objects
 *
 * The config loader parses and validates a scenario's `events:` timeline into
 * EventConfig records, and the concrete Event classes have existed since the
 * event system landed -- but nothing ever joined the two, so every scenario ran
 * as a static mesh and the timeline was silently discarded. This factory is that
 * missing link.
 */
class EventFactory {
public:
  /**
   * @brief Builds one Event from a parsed scenario event
   *
   * @param config     The parsed event
   * @param idToNodeId Maps a scenario's string node ids to their numeric ids
   * @return The event, or nullptr if the action has no runtime implementation
   */
  static std::unique_ptr<Event> create(
      const EventConfig& config,
      const std::map<std::string, uint32_t>& idToNodeId);

  /**
   * @brief Schedules every event in a scenario
   *
   * @param events     The scenario's parsed timeline
   * @param idToNodeId Maps a scenario's string node ids to their numeric ids
   * @param scheduler  Receives the constructed events
   * @param[out] skipped Descriptions of events that could not be built
   * @return Number of events scheduled
   */
  static size_t scheduleAll(
      const std::vector<EventConfig>& events,
      const std::map<std::string, uint32_t>& idToNodeId,
      EventScheduler& scheduler,
      std::vector<std::string>& skipped);
};

}  // namespace simulator

#endif  // SIMULATOR_EVENT_FACTORY_HPP
