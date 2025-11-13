/**
 * @file event_scheduler.cpp
 * @brief Implementation of event scheduler for managing simulation events
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include "simulator/event_scheduler.hpp"
#include "simulator/node_manager.hpp"
#include "simulator/network_simulator.hpp"
#include <iostream>
#include <stdexcept>

namespace simulator {

void EventScheduler::scheduleEvent(std::unique_ptr<Event> event, uint32_t time) {
  if (!event) {
    throw std::invalid_argument("Cannot schedule null event");
  }
  
  event->setScheduledTime(time);
  eventQueue_.push(std::move(event));
}

uint32_t EventScheduler::processEvents(uint32_t currentTime, NodeManager& manager, NetworkSimulator& network) {
  uint32_t executedCount = 0;
  
  // Process all events scheduled at or before current time
  while (!eventQueue_.empty()) {
    // Peek at the next event
    const auto& nextEvent = eventQueue_.top();
    
    // Check if it's time to execute this event
    if (nextEvent->getScheduledTime() > currentTime) {
      // Next event is in the future, stop processing
      break;
    }
    
    // Get event for execution (moving ownership out of queue)
    auto event = std::move(const_cast<std::unique_ptr<Event>&>(nextEvent));
    eventQueue_.pop();
    
    // Log event execution
    std::cout << "[EVENT] t=" << currentTime << "s: " << event->getDescription() << std::endl;
    
    try {
      // Execute the event
      event->execute(manager, network);
      executedCount++;
    } catch (const std::exception& e) {
      // Log error but continue processing other events
      std::cerr << "[ERROR] Event execution failed: " << e.what() << std::endl;
    }
  }
  
  return executedCount;
}

bool EventScheduler::hasPendingEvents() const {
  return !eventQueue_.empty();
}

size_t EventScheduler::getPendingEventCount() const {
  return eventQueue_.size();
}

uint32_t EventScheduler::getNextEventTime() const {
  if (eventQueue_.empty()) {
    return UINT32_MAX;
  }
  return eventQueue_.top()->getScheduledTime();
}

void EventScheduler::clear() {
  // Clear the priority queue by creating a new empty one
  std::priority_queue<std::unique_ptr<Event>, 
                      std::vector<std::unique_ptr<Event>>, 
                      EventComparator> emptyQueue;
  eventQueue_ = std::move(emptyQueue);
}

} // namespace simulator
