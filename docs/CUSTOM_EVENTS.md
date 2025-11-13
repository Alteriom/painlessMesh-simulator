# Creating Custom Events

This guide explains how to create custom event types for the painlessMesh simulator's event-driven scenario engine.

## Table of Contents

- [Overview](#overview)
- [Event Base Class](#event-base-class)
- [Creating a Custom Event](#creating-a-custom-event)
- [Event Scheduling](#event-scheduling)
- [Example Events](#example-events)
- [Best Practices](#best-practices)
- [Testing Events](#testing-events)

## Overview

The event system in the painlessMesh simulator enables scenario-based testing by allowing you to schedule and execute events at specific simulation times. Events can:

- Simulate node failures
- Modify network conditions (latency, packet loss, bandwidth)
- Create network partitions
- Send messages between nodes
- Collect metrics at specific times
- Any other custom simulation logic

## Event Base Class

All events inherit from the `Event` base class:

```cpp
namespace simulator {

class Event {
public:
  virtual ~Event() = default;
  
  // Execute the event - implement your logic here
  virtual void execute(NodeManager& manager, NetworkSimulator& network) = 0;
  
  // Provide a description for logging
  virtual std::string getDescription() const = 0;
  
  // Get/set scheduled time
  uint32_t getScheduledTime() const;
  void setScheduledTime(uint32_t time);

protected:
  uint32_t scheduledTime_ = 0;  // Time in seconds since simulation start
};

} // namespace simulator
```

## Creating a Custom Event

### Step 1: Define Your Event Class

Create a header file for your event (e.g., `include/simulator/events/my_event.hpp`):

```cpp
#ifndef SIMULATOR_MY_EVENT_HPP
#define SIMULATOR_MY_EVENT_HPP

#include "simulator/event.hpp"

namespace simulator {

class MyEvent : public Event {
public:
  // Constructor with any parameters your event needs
  explicit MyEvent(uint32_t param1, const std::string& param2);
  
  // Execute the event logic
  void execute(NodeManager& manager, NetworkSimulator& network) override;
  
  // Provide a description
  std::string getDescription() const override;

private:
  uint32_t param1_;
  std::string param2_;
};

} // namespace simulator

#endif
```

### Step 2: Implement the Event Logic

Create an implementation file (e.g., `src/scenario/events/my_event.cpp`):

```cpp
#include "simulator/events/my_event.hpp"
#include "simulator/node_manager.hpp"
#include "simulator/network_simulator.hpp"

namespace simulator {

MyEvent::MyEvent(uint32_t param1, const std::string& param2)
  : param1_(param1), param2_(param2) {
}

void MyEvent::execute(NodeManager& manager, NetworkSimulator& network) {
  // Implement your event logic here
  // You have access to:
  // - manager: for accessing/modifying nodes
  // - network: for modifying network conditions
  
  // Example: Access a specific node
  auto node = manager.getNode(param1_);
  if (node) {
    // Do something with the node
  }
  
  // Example: Modify network conditions
  LatencyConfig latency;
  latency.min_ms = 50;
  latency.max_ms = 200;
  network.setDefaultLatency(latency);
}

std::string MyEvent::getDescription() const {
  return "MyEvent: " + param2_;
}

} // namespace simulator
```

### Step 3: Schedule Your Event

Use the `EventScheduler` to schedule your event:

```cpp
#include "simulator/event_scheduler.hpp"
#include "simulator/events/my_event.hpp"

// Create scheduler
EventScheduler scheduler;

// Create and schedule event
auto event = std::make_unique<MyEvent>(1001, "test");
scheduler.scheduleEvent(std::move(event), 30);  // Execute at t=30s

// In simulation loop
while (running) {
  uint32_t currentTime = getSimulationTime();
  scheduler.processEvents(currentTime, manager, network);
  // ... rest of simulation
}
```

## Event Scheduling

### Basic Scheduling

Schedule a single event:

```cpp
auto event = std::make_unique<MyEvent>(param1, param2);
scheduler.scheduleEvent(std::move(event), 30);  // Execute at t=30 seconds
```

### Multiple Events

Schedule multiple events at different times:

```cpp
// Event at t=10s
scheduler.scheduleEvent(std::make_unique<Event1>(), 10);

// Event at t=20s
scheduler.scheduleEvent(std::make_unique<Event2>(), 20);

// Event at t=30s
scheduler.scheduleEvent(std::make_unique<Event3>(), 30);
```

### Same-Time Events

Multiple events can be scheduled at the same time. They execute in the order they were added:

```cpp
scheduler.scheduleEvent(std::make_unique<Event1>(), 30);
scheduler.scheduleEvent(std::make_unique<Event2>(), 30);
scheduler.scheduleEvent(std::make_unique<Event3>(), 30);
// All three execute at t=30s, in this order
```

### Event Processing

Process events in your simulation loop:

```cpp
while (running) {
  uint32_t currentTime = getCurrentSimulationTime();
  
  // This executes all events scheduled at or before currentTime
  uint32_t executedCount = scheduler.processEvents(currentTime, manager, network);
  
  // Continue with simulation update
  manager.updateAll();
  
  // Small delay
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}
```

## Example Events

### Node Crash Event

Simulates a node failure:

```cpp
class NodeCrashEvent : public Event {
public:
  explicit NodeCrashEvent(uint32_t nodeId) : nodeId_(nodeId) {}
  
  void execute(NodeManager& manager, NetworkSimulator& network) override {
    auto node = manager.getNode(nodeId_);
    if (node && node->isRunning()) {
      node->stop();
    }
  }
  
  std::string getDescription() const override {
    return "Node crash: " + std::to_string(nodeId_);
  }

private:
  uint32_t nodeId_;
};

// Usage:
scheduler.scheduleEvent(std::make_unique<NodeCrashEvent>(1001), 30);
```

### Network Latency Event

Changes network latency:

```cpp
class LatencyEvent : public Event {
public:
  LatencyEvent(uint32_t minMs, uint32_t maxMs) 
    : minMs_(minMs), maxMs_(maxMs) {}
  
  void execute(NodeManager& manager, NetworkSimulator& network) override {
    LatencyConfig config;
    config.min_ms = minMs_;
    config.max_ms = maxMs_;
    config.distribution = DistributionType::NORMAL;
    network.setDefaultLatency(config);
  }
  
  std::string getDescription() const override {
    return "Set latency: " + std::to_string(minMs_) + "-" + 
           std::to_string(maxMs_) + "ms";
  }

private:
  uint32_t minMs_;
  uint32_t maxMs_;
};

// Usage:
scheduler.scheduleEvent(std::make_unique<LatencyEvent>(100, 500), 45);
```

### Packet Loss Event

Simulates packet loss:

```cpp
class PacketLossEvent : public Event {
public:
  explicit PacketLossEvent(float probability) : probability_(probability) {}
  
  void execute(NodeManager& manager, NetworkSimulator& network) override {
    PacketLossConfig config;
    config.probability = probability_;
    network.setDefaultPacketLoss(config);
  }
  
  std::string getDescription() const override {
    return "Set packet loss: " + std::to_string(probability_ * 100) + "%";
  }

private:
  float probability_;
};

// Usage:
scheduler.scheduleEvent(std::make_unique<PacketLossEvent>(0.1f), 60);
```

### Message Broadcast Event

Broadcasts a message from a node:

```cpp
class BroadcastEvent : public Event {
public:
  BroadcastEvent(uint32_t fromNode, const std::string& message)
    : fromNode_(fromNode), message_(message) {}
  
  void execute(NodeManager& manager, NetworkSimulator& network) override {
    auto node = manager.getNode(fromNode_);
    if (node && node->isRunning()) {
      node->getMesh().sendBroadcast(message_);
    }
  }
  
  std::string getDescription() const override {
    return "Broadcast from node " + std::to_string(fromNode_);
  }

private:
  uint32_t fromNode_;
  std::string message_;
};

// Usage:
scheduler.scheduleEvent(
  std::make_unique<BroadcastEvent>(1001, "Hello mesh!"), 15);
```

## Best Practices

### 1. Keep Events Focused

Each event should do one thing well:

```cpp
// Good: Focused event
class NodeStopEvent : public Event { ... };

// Less good: Event that does multiple things
class NodeStopAndChangeLatencyEvent : public Event { ... };
```

Instead, schedule multiple events:

```cpp
scheduler.scheduleEvent(std::make_unique<NodeStopEvent>(1001), 30);
scheduler.scheduleEvent(std::make_unique<LatencyEvent>(100, 200), 30);
```

### 2. Handle Errors Gracefully

Events should handle errors without crashing the simulation:

```cpp
void MyEvent::execute(NodeManager& manager, NetworkSimulator& network) {
  auto node = manager.getNode(nodeId_);
  
  if (!node) {
    // Log error but don't throw
    std::cerr << "Warning: Node " << nodeId_ << " not found\n";
    return;
  }
  
  // Continue with event logic
}
```

### 3. Provide Descriptive Messages

Make descriptions informative for logging:

```cpp
std::string getDescription() const override {
  return "Network partition: isolate nodes " + 
         std::to_string(node1_) + " and " + std::to_string(node2_);
}
```

### 4. Make Events Stateless

Events should not maintain state between executions. Store all necessary data in constructor:

```cpp
class MyEvent : public Event {
public:
  MyEvent(uint32_t nodeId, int value) 
    : nodeId_(nodeId), value_(value) {}
    
  void execute(NodeManager& manager, NetworkSimulator& network) override {
    // Use nodeId_ and value_ directly
  }

private:
  uint32_t nodeId_;
  int value_;
};
```

### 5. Document Your Events

Add clear documentation:

```cpp
/**
 * @brief Simulates a network partition between two groups of nodes
 * 
 * This event creates a network partition by preventing communication
 * between two groups of nodes. Messages between groups will be dropped.
 * 
 * Example:
 * @code
 * std::vector<uint32_t> group1 = {1001, 1002, 1003};
 * std::vector<uint32_t> group2 = {2001, 2002, 2003};
 * auto event = std::make_unique<PartitionEvent>(group1, group2);
 * scheduler.scheduleEvent(std::move(event), 60);
 * @endcode
 */
class PartitionEvent : public Event { ... };
```

## Testing Events

### Unit Tests

Create unit tests for your events:

```cpp
#include <catch2/catch_test_macros.hpp>
#include "simulator/events/my_event.hpp"

TEST_CASE("MyEvent execution", "[event]") {
  boost::asio::io_context io;
  NodeManager manager(io);
  NetworkSimulator network;
  
  // Create nodes
  NodeConfig config{1001, "TestMesh", "password"};
  manager.createNode(config);
  manager.startAll();
  
  // Create and execute event
  MyEvent event(1001, "test");
  event.execute(manager, network);
  
  // Verify event effects
  auto node = manager.getNode(1001);
  REQUIRE(node->isRunning() == false);  // Or whatever your event does
}
```

### Integration Tests

Test events with the scheduler:

```cpp
TEST_CASE("Event scheduling integration", "[event][integration]") {
  boost::asio::io_context io;
  NodeManager manager(io);
  NetworkSimulator network;
  EventScheduler scheduler;
  
  // Schedule events
  scheduler.scheduleEvent(std::make_unique<MyEvent>(1001, "test1"), 10);
  scheduler.scheduleEvent(std::make_unique<MyEvent>(1002, "test2"), 20);
  
  // Process events
  scheduler.processEvents(15, manager, network);
  
  // Verify only first event executed
  REQUIRE(scheduler.getPendingEventCount() == 1);
}
```

## Advanced Topics

### Conditional Events

Create events that execute based on conditions:

```cpp
class ConditionalEvent : public Event {
public:
  ConditionalEvent(std::function<bool()> condition, 
                  std::unique_ptr<Event> wrappedEvent)
    : condition_(condition), wrappedEvent_(std::move(wrappedEvent)) {}
  
  void execute(NodeManager& manager, NetworkSimulator& network) override {
    if (condition_()) {
      wrappedEvent_->execute(manager, network);
    }
  }
  
  std::string getDescription() const override {
    return "Conditional: " + wrappedEvent_->getDescription();
  }

private:
  std::function<bool()> condition_;
  std::unique_ptr<Event> wrappedEvent_;
};
```

### Repeating Events

Create events that reschedule themselves:

```cpp
class RepeatingEvent : public Event {
public:
  RepeatingEvent(EventScheduler& scheduler, uint32_t interval)
    : scheduler_(scheduler), interval_(interval) {}
  
  void execute(NodeManager& manager, NetworkSimulator& network) override {
    // Do work
    
    // Reschedule for next interval
    auto nextEvent = std::make_unique<RepeatingEvent>(scheduler_, interval_);
    scheduler_.scheduleEvent(std::move(nextEvent), 
                            getScheduledTime() + interval_);
  }
  
  std::string getDescription() const override {
    return "Repeating event (interval: " + std::to_string(interval_) + "s)";
  }

private:
  EventScheduler& scheduler_;
  uint32_t interval_;
};
```

## Summary

The event system provides a flexible foundation for scenario-based testing. Key points:

1. Inherit from `Event` base class
2. Implement `execute()` and `getDescription()`
3. Schedule events with `EventScheduler`
4. Events execute in chronological order
5. Keep events focused and stateless
6. Handle errors gracefully
7. Write tests for your events

For more examples, see:
- `include/simulator/events/node_crash_event.hpp`
- `test/test_event_scheduler.cpp`

## Next Steps

After implementing your custom events:

1. Add them to CMakeLists.txt
2. Write unit tests
3. Test with integration scenarios
4. Document in your scenario YAML files
5. Consider contributing back to the project!
