/**
 * @file event_scheduler.hpp
 * @brief Event scheduler for managing time-based simulation events
 * 
 * This file contains the EventScheduler class which manages scheduling
 * and execution of events during a simulation run using a priority queue.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef SIMULATOR_EVENT_SCHEDULER_HPP
#define SIMULATOR_EVENT_SCHEDULER_HPP

#include "simulator/event.hpp"
#include <memory>
#include <queue>
#include <vector>
#include <functional>

namespace simulator {

/**
 * @brief Manages scheduling and execution of simulation events
 * 
 * The EventScheduler class provides a priority queue-based event
 * scheduling system. Events are scheduled at specific simulation times
 * and executed in chronological order.
 * 
 * Example usage:
 * @code
 * EventScheduler scheduler;
 * 
 * // Schedule events
 * auto event1 = std::make_unique<NodeCrashEvent>(1001);
 * scheduler.scheduleEvent(std::move(event1), 30);
 * 
 * auto event2 = std::make_unique<ConnectionDropEvent>(1001, 1002);
 * scheduler.scheduleEvent(std::move(event2), 45);
 * 
 * // In simulation loop
 * while (running) {
 *   uint32_t currentTime = getSimulationTime();
 *   scheduler.processEvents(currentTime, manager, network);
 *   // ... rest of simulation update
 * }
 * @endcode
 * 
 * @note The EventScheduler does not manage time itself. The simulation
 *       loop is responsible for tracking time and calling processEvents()
 *       with the current time.
 */
class EventScheduler {
public:
  /**
   * @brief Default constructor
   */
  EventScheduler() = default;
  
  /**
   * @brief Destructor
   */
  ~EventScheduler() = default;
  
  // Prevent copying
  EventScheduler(const EventScheduler&) = delete;
  EventScheduler& operator=(const EventScheduler&) = delete;
  
  // Allow moving
  EventScheduler(EventScheduler&&) = default;
  EventScheduler& operator=(EventScheduler&&) = default;
  
  /**
   * @brief Schedule an event for execution at a specific time
   * 
   * The event will be added to the priority queue and executed when
   * processEvents() is called with a time >= the scheduled time.
   * 
   * @param event Unique pointer to event (ownership transferred)
   * @param time Scheduled time in seconds since simulation start
   * 
   * @throws std::invalid_argument if event is nullptr
   */
  void scheduleEvent(std::unique_ptr<Event> event, uint32_t time);
  
  /**
   * @brief Process all events scheduled at or before current time
   * 
   * This method executes all events whose scheduled time is less than
   * or equal to currentTime. Events are executed in chronological order.
   * Events scheduled at the same time are executed in the order they
   * were added.
   * 
   * @param currentTime Current simulation time in seconds
   * @param manager Node manager for event execution
   * @param network Network simulator for event execution
   * 
   * @return Number of events executed
   * 
   * @note If an event throws an exception during execution, the exception
   *       is logged and the scheduler continues processing remaining events.
   */
  uint32_t processEvents(uint32_t currentTime, NodeManager& manager, NetworkSimulator& network);
  
  /**
   * @brief Check if there are pending events
   * 
   * @return true if events are queued, false if queue is empty
   */
  bool hasPendingEvents() const;
  
  /**
   * @brief Get the number of pending events
   * 
   * @return Count of events in the queue
   */
  size_t getPendingEventCount() const;
  
  /**
   * @brief Get the time of the next scheduled event
   * 
   * @return Time in seconds of next event, or UINT32_MAX if no events
   */
  uint32_t getNextEventTime() const;
  
  /**
   * @brief Clear all pending events
   * 
   * Removes all events from the queue without executing them.
   */
  void clear();

private:
  /**
   * @brief Comparator for event priority queue
   * 
   * Orders events by scheduled time (earliest first).
   * For events with the same time, maintains insertion order (FIFO).
   */
  struct EventComparator {
    bool operator()(const std::unique_ptr<Event>& a, const std::unique_ptr<Event>& b) const {
      // Return true if a should come after b (min-heap)
      return a->getScheduledTime() > b->getScheduledTime();
    }
  };
  
  // Priority queue of events (min-heap based on scheduled time)
  std::priority_queue<std::unique_ptr<Event>, 
                      std::vector<std::unique_ptr<Event>>, 
                      EventComparator> eventQueue_;
};

} // namespace simulator

#endif // SIMULATOR_EVENT_SCHEDULER_HPP
