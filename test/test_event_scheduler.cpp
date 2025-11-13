/**
 * @file test_event_scheduler.cpp
 * @brief Unit tests for Event and EventScheduler classes
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include <catch2/catch_test_macros.hpp>
#include "simulator/event.hpp"
#include "simulator/event_scheduler.hpp"
#include "simulator/node_manager.hpp"
#include "simulator/network_simulator.hpp"
#include <boost/asio.hpp>
#include <vector>
#include <string>

using namespace simulator;

/**
 * @brief Test event that records when it was executed
 */
class TestEvent : public Event {
public:
  explicit TestEvent(const std::string& name) : name_(name), executed_(false) {}
  
  void execute(NodeManager& manager, NetworkSimulator& network) override {
    executed_ = true;
    executionTime_ = getScheduledTime();
  }
  
  std::string getDescription() const override {
    return "TestEvent: " + name_;
  }
  
  bool wasExecuted() const { return executed_; }
  uint32_t getExecutionTime() const { return executionTime_; }

private:
  std::string name_;
  bool executed_;
  uint32_t executionTime_ = 0;
};

/**
 * @brief Test event that throws an exception
 */
class FailingEvent : public Event {
public:
  void execute(NodeManager& manager, NetworkSimulator& network) override {
    throw std::runtime_error("Intentional test failure");
  }
  
  std::string getDescription() const override {
    return "FailingEvent";
  }
};

/**
 * @brief Test event that modifies a counter
 */
class CounterEvent : public Event {
public:
  explicit CounterEvent(int& counter) : counter_(counter) {}
  
  void execute(NodeManager& manager, NetworkSimulator& network) override {
    counter_++;
  }
  
  std::string getDescription() const override {
    return "CounterEvent";
  }

private:
  int& counter_;
};

TEST_CASE("Event base class", "[event]") {
  SECTION("can set and get scheduled time") {
    auto event = std::make_unique<TestEvent>("test");
    
    REQUIRE(event->getScheduledTime() == 0);
    
    event->setScheduledTime(30);
    REQUIRE(event->getScheduledTime() == 30);
    
    event->setScheduledTime(100);
    REQUIRE(event->getScheduledTime() == 100);
  }
  
  SECTION("provides description") {
    auto event = std::make_unique<TestEvent>("my-test-event");
    REQUIRE(event->getDescription() == "TestEvent: my-test-event");
  }
}

TEST_CASE("EventScheduler construction", "[event_scheduler]") {
  SECTION("can be created") {
    REQUIRE_NOTHROW(EventScheduler());
  }
  
  SECTION("starts with no pending events") {
    EventScheduler scheduler;
    REQUIRE_FALSE(scheduler.hasPendingEvents());
    REQUIRE(scheduler.getPendingEventCount() == 0);
    REQUIRE(scheduler.getNextEventTime() == UINT32_MAX);
  }
}

TEST_CASE("EventScheduler event scheduling", "[event_scheduler]") {
  EventScheduler scheduler;
  
  SECTION("can schedule a single event") {
    auto event = std::make_unique<TestEvent>("event1");
    scheduler.scheduleEvent(std::move(event), 30);
    
    REQUIRE(scheduler.hasPendingEvents());
    REQUIRE(scheduler.getPendingEventCount() == 1);
    REQUIRE(scheduler.getNextEventTime() == 30);
  }
  
  SECTION("can schedule multiple events") {
    auto event1 = std::make_unique<TestEvent>("event1");
    auto event2 = std::make_unique<TestEvent>("event2");
    auto event3 = std::make_unique<TestEvent>("event3");
    
    scheduler.scheduleEvent(std::move(event1), 30);
    scheduler.scheduleEvent(std::move(event2), 45);
    scheduler.scheduleEvent(std::move(event3), 60);
    
    REQUIRE(scheduler.hasPendingEvents());
    REQUIRE(scheduler.getPendingEventCount() == 3);
    REQUIRE(scheduler.getNextEventTime() == 30);
  }
  
  SECTION("rejects null event") {
    REQUIRE_THROWS_AS(scheduler.scheduleEvent(nullptr, 30), std::invalid_argument);
  }
  
  SECTION("orders events by time") {
    // Schedule events out of order
    auto event1 = std::make_unique<TestEvent>("event1");
    auto event2 = std::make_unique<TestEvent>("event2");
    auto event3 = std::make_unique<TestEvent>("event3");
    
    scheduler.scheduleEvent(std::move(event3), 60);
    scheduler.scheduleEvent(std::move(event1), 30);
    scheduler.scheduleEvent(std::move(event2), 45);
    
    // Next event should be the earliest one
    REQUIRE(scheduler.getNextEventTime() == 30);
  }
}

TEST_CASE("EventScheduler event processing", "[event_scheduler]") {
  boost::asio::io_context io;
  NodeManager manager(io);
  NetworkSimulator network;
  
  SECTION("processes events at correct time") {
    EventScheduler scheduler;
    
    auto event1 = std::make_unique<TestEvent>("event1");
    scheduler.scheduleEvent(std::move(event1), 30);
    
    // Before scheduled time - should not execute
    uint32_t executed = scheduler.processEvents(29, manager, network);
    REQUIRE(executed == 0);
    REQUIRE(scheduler.hasPendingEvents());
    
    // At scheduled time - should execute
    executed = scheduler.processEvents(30, manager, network);
    REQUIRE(executed == 1);
    REQUIRE_FALSE(scheduler.hasPendingEvents());
  }
  
  SECTION("processes multiple events in chronological order") {
    EventScheduler scheduler;
    
    int counter = 0;
    
    auto event1 = std::make_unique<CounterEvent>(counter);
    auto event2 = std::make_unique<CounterEvent>(counter);
    auto event3 = std::make_unique<CounterEvent>(counter);
    
    scheduler.scheduleEvent(std::move(event1), 10);
    scheduler.scheduleEvent(std::move(event2), 20);
    scheduler.scheduleEvent(std::move(event3), 30);
    
    // Process at t=15 - only event1 should execute
    uint32_t executed = scheduler.processEvents(15, manager, network);
    REQUIRE(executed == 1);
    REQUIRE(counter == 1);
    REQUIRE(scheduler.getPendingEventCount() == 2);
    
    // Process at t=25 - event2 should execute
    executed = scheduler.processEvents(25, manager, network);
    REQUIRE(executed == 1);
    REQUIRE(counter == 2);
    REQUIRE(scheduler.getPendingEventCount() == 1);
    
    // Process at t=35 - event3 should execute
    executed = scheduler.processEvents(35, manager, network);
    REQUIRE(executed == 1);
    REQUIRE(counter == 3);
    REQUIRE(scheduler.getPendingEventCount() == 0);
  }
  
  SECTION("processes all ready events in single call") {
    EventScheduler scheduler;
    
    int counter = 0;
    
    auto event1 = std::make_unique<CounterEvent>(counter);
    auto event2 = std::make_unique<CounterEvent>(counter);
    auto event3 = std::make_unique<CounterEvent>(counter);
    
    scheduler.scheduleEvent(std::move(event1), 10);
    scheduler.scheduleEvent(std::move(event2), 20);
    scheduler.scheduleEvent(std::move(event3), 30);
    
    // Process at t=35 - all events should execute
    uint32_t executed = scheduler.processEvents(35, manager, network);
    REQUIRE(executed == 3);
    REQUIRE(counter == 3);
    REQUIRE_FALSE(scheduler.hasPendingEvents());
  }
  
  SECTION("handles events at same time") {
    EventScheduler scheduler;
    
    int counter = 0;
    
    auto event1 = std::make_unique<CounterEvent>(counter);
    auto event2 = std::make_unique<CounterEvent>(counter);
    auto event3 = std::make_unique<CounterEvent>(counter);
    
    // All events at same time
    scheduler.scheduleEvent(std::move(event1), 30);
    scheduler.scheduleEvent(std::move(event2), 30);
    scheduler.scheduleEvent(std::move(event3), 30);
    
    // Process at t=30 - all should execute
    uint32_t executed = scheduler.processEvents(30, manager, network);
    REQUIRE(executed == 3);
    REQUIRE(counter == 3);
  }
  
  SECTION("continues processing after event failure") {
    EventScheduler scheduler;
    
    int counter = 0;
    
    auto event1 = std::make_unique<CounterEvent>(counter);
    auto event2 = std::make_unique<FailingEvent>();
    auto event3 = std::make_unique<CounterEvent>(counter);
    
    scheduler.scheduleEvent(std::move(event1), 10);
    scheduler.scheduleEvent(std::move(event2), 20);
    scheduler.scheduleEvent(std::move(event3), 30);
    
    // Process all events - should execute all despite failure
    uint32_t executed = scheduler.processEvents(40, manager, network);
    REQUIRE(executed == 2);  // Only 2 succeeded
    REQUIRE(counter == 2);   // Both counter events executed
  }
  
  SECTION("does not execute future events") {
    EventScheduler scheduler;
    
    int counter = 0;
    
    auto event1 = std::make_unique<CounterEvent>(counter);
    auto event2 = std::make_unique<CounterEvent>(counter);
    
    scheduler.scheduleEvent(std::move(event1), 30);
    scheduler.scheduleEvent(std::move(event2), 60);
    
    // Process at t=45 - only event1 should execute
    uint32_t executed = scheduler.processEvents(45, manager, network);
    REQUIRE(executed == 1);
    REQUIRE(counter == 1);
    REQUIRE(scheduler.getPendingEventCount() == 1);
    REQUIRE(scheduler.getNextEventTime() == 60);
  }
}

TEST_CASE("EventScheduler clear", "[event_scheduler]") {
  EventScheduler scheduler;
  
  SECTION("clears all pending events") {
    auto event1 = std::make_unique<TestEvent>("event1");
    auto event2 = std::make_unique<TestEvent>("event2");
    auto event3 = std::make_unique<TestEvent>("event3");
    
    scheduler.scheduleEvent(std::move(event1), 10);
    scheduler.scheduleEvent(std::move(event2), 20);
    scheduler.scheduleEvent(std::move(event3), 30);
    
    REQUIRE(scheduler.getPendingEventCount() == 3);
    
    scheduler.clear();
    
    REQUIRE_FALSE(scheduler.hasPendingEvents());
    REQUIRE(scheduler.getPendingEventCount() == 0);
    REQUIRE(scheduler.getNextEventTime() == UINT32_MAX);
  }
  
  SECTION("can schedule events after clear") {
    auto event1 = std::make_unique<TestEvent>("event1");
    scheduler.scheduleEvent(std::move(event1), 10);
    
    scheduler.clear();
    
    auto event2 = std::make_unique<TestEvent>("event2");
    scheduler.scheduleEvent(std::move(event2), 20);
    
    REQUIRE(scheduler.getPendingEventCount() == 1);
    REQUIRE(scheduler.getNextEventTime() == 20);
  }
}

TEST_CASE("EventScheduler event ordering", "[event_scheduler]") {
  boost::asio::io_context io;
  NodeManager manager(io);
  NetworkSimulator network;
  EventScheduler scheduler;
  
  SECTION("executes 10 events in correct order") {
    std::vector<uint32_t> execution_order;
    
    // Create event that records execution order
    class OrderRecordingEvent : public Event {
    public:
      OrderRecordingEvent(std::vector<uint32_t>& order, uint32_t id) 
        : order_(order), id_(id) {}
      
      void execute(NodeManager& manager, NetworkSimulator& network) override {
        order_.push_back(id_);
      }
      
      std::string getDescription() const override {
        return "OrderRecordingEvent #" + std::to_string(id_);
      }
    
    private:
      std::vector<uint32_t>& order_;
      uint32_t id_;
    };
    
    // Schedule 10 events at various times
    scheduler.scheduleEvent(std::make_unique<OrderRecordingEvent>(execution_order, 5), 50);
    scheduler.scheduleEvent(std::make_unique<OrderRecordingEvent>(execution_order, 1), 10);
    scheduler.scheduleEvent(std::make_unique<OrderRecordingEvent>(execution_order, 8), 80);
    scheduler.scheduleEvent(std::make_unique<OrderRecordingEvent>(execution_order, 3), 30);
    scheduler.scheduleEvent(std::make_unique<OrderRecordingEvent>(execution_order, 7), 70);
    scheduler.scheduleEvent(std::make_unique<OrderRecordingEvent>(execution_order, 2), 20);
    scheduler.scheduleEvent(std::make_unique<OrderRecordingEvent>(execution_order, 9), 90);
    scheduler.scheduleEvent(std::make_unique<OrderRecordingEvent>(execution_order, 4), 40);
    scheduler.scheduleEvent(std::make_unique<OrderRecordingEvent>(execution_order, 6), 60);
    scheduler.scheduleEvent(std::make_unique<OrderRecordingEvent>(execution_order, 10), 100);
    
    // Process all events
    uint32_t executed = scheduler.processEvents(150, manager, network);
    
    REQUIRE(executed == 10);
    REQUIRE(execution_order.size() == 10);
    
    // Verify they executed in chronological order
    REQUIRE(execution_order[0] == 1);   // t=10
    REQUIRE(execution_order[1] == 2);   // t=20
    REQUIRE(execution_order[2] == 3);   // t=30
    REQUIRE(execution_order[3] == 4);   // t=40
    REQUIRE(execution_order[4] == 5);   // t=50
    REQUIRE(execution_order[5] == 6);   // t=60
    REQUIRE(execution_order[6] == 7);   // t=70
    REQUIRE(execution_order[7] == 8);   // t=80
    REQUIRE(execution_order[8] == 9);   // t=90
    REQUIRE(execution_order[9] == 10);  // t=100
  }
}

TEST_CASE("EventScheduler integration test", "[event_scheduler][integration]") {
  boost::asio::io_context io;
  NodeManager manager(io);
  NetworkSimulator network;
  EventScheduler scheduler;
  
  SECTION("simulate 60 second scenario with multiple events") {
    int event_count = 0;
    
    class SimulatedEvent : public Event {
    public:
      SimulatedEvent(int& counter, const std::string& desc) 
        : counter_(counter), desc_(desc) {}
      
      void execute(NodeManager& manager, NetworkSimulator& network) override {
        counter_++;
      }
      
      std::string getDescription() const override {
        return desc_;
      }
    
    private:
      int& counter_;
      std::string desc_;
    };
    
    // Schedule events throughout 60 second simulation
    scheduler.scheduleEvent(
      std::make_unique<SimulatedEvent>(event_count, "Warm-up complete"), 5);
    scheduler.scheduleEvent(
      std::make_unique<SimulatedEvent>(event_count, "Node failure"), 15);
    scheduler.scheduleEvent(
      std::make_unique<SimulatedEvent>(event_count, "Network partition"), 25);
    scheduler.scheduleEvent(
      std::make_unique<SimulatedEvent>(event_count, "High latency"), 35);
    scheduler.scheduleEvent(
      std::make_unique<SimulatedEvent>(event_count, "Heal partition"), 45);
    scheduler.scheduleEvent(
      std::make_unique<SimulatedEvent>(event_count, "Restore latency"), 55);
    
    REQUIRE(scheduler.getPendingEventCount() == 6);
    
    // Simulate time progression
    for (uint32_t t = 0; t <= 60; t += 5) {
      scheduler.processEvents(t, manager, network);
    }
    
    // All events should have executed
    REQUIRE(event_count == 6);
    REQUIRE_FALSE(scheduler.hasPendingEvents());
  }
}
