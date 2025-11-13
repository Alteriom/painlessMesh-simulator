/**
 * @file event.hpp
 * @brief Event base class for scenario-based simulation events
 * 
 * This file contains the Event base class which defines the interface
 * for all simulation events that can be scheduled and executed during
 * a simulation run.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef SIMULATOR_EVENT_HPP
#define SIMULATOR_EVENT_HPP

#include <cstdint>
#include <string>
#include <memory>

// Forward declarations
namespace simulator {
  class NodeManager;
  class NetworkSimulator;
}

namespace simulator {

/**
 * @brief Base class for all simulation events
 * 
 * The Event class provides the interface for all events that can be
 * scheduled and executed during a simulation. Events are executed at
 * a specific simulation time and can modify the state of nodes or
 * the network.
 * 
 * To create a custom event type:
 * @code
 * class MyEvent : public Event {
 * public:
 *   void execute(NodeManager& manager, NetworkSimulator& network) override {
 *     // Implement event logic here
 *   }
 *   
 *   std::string getDescription() const override {
 *     return "My custom event";
 *   }
 * };
 * @endcode
 * 
 * Events are scheduled using the EventScheduler:
 * @code
 * EventScheduler scheduler;
 * auto event = std::make_unique<MyEvent>();
 * scheduler.scheduleEvent(std::move(event), 30);  // Execute at t=30s
 * @endcode
 * 
 * @note Event implementations should be stateless or store minimal state.
 *       All simulation state should be managed by NodeManager and NetworkSimulator.
 */
class Event {
public:
  /**
   * @brief Virtual destructor
   */
  virtual ~Event() = default;
  
  /**
   * @brief Execute the event
   * 
   * This method is called by the EventScheduler when the event's
   * scheduled time is reached. Implementations should modify the
   * simulation state as needed.
   * 
   * @param manager Node manager for accessing and modifying nodes
   * @param network Network simulator for modifying network conditions
   * 
   * @throws std::runtime_error if event execution fails
   */
  virtual void execute(NodeManager& manager, NetworkSimulator& network) = 0;
  
  /**
   * @brief Get a human-readable description of the event
   * 
   * @return String describing what this event does
   */
  virtual std::string getDescription() const = 0;
  
  /**
   * @brief Get the scheduled execution time
   * 
   * @return Scheduled time in seconds since simulation start
   */
  uint32_t getScheduledTime() const { return scheduledTime_; }
  
  /**
   * @brief Set the scheduled execution time
   * 
   * @param time Time in seconds since simulation start
   */
  void setScheduledTime(uint32_t time) { scheduledTime_ = time; }

protected:
  uint32_t scheduledTime_ = 0;  ///< Scheduled execution time in seconds
};

} // namespace simulator

#endif // SIMULATOR_EVENT_HPP
