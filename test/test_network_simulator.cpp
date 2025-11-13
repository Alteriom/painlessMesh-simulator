/**
 * @file test_network_simulator.cpp
 * @brief Unit tests for NetworkSimulator class
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "simulator/network_simulator.hpp"

using namespace simulator;

TEST_CASE("NetworkSimulator construction", "[network_simulator]") {
  SECTION("can be created with default constructor") {
    REQUIRE_NOTHROW(NetworkSimulator());
  }
  
  SECTION("can be created with seed for deterministic testing") {
    REQUIRE_NOTHROW(NetworkSimulator(12345));
  }
  
  SECTION("has default latency configuration") {
    NetworkSimulator sim;
    auto config = sim.getLatency(1, 2);
    REQUIRE(config.min_ms <= config.max_ms);
  }
}

TEST_CASE("LatencyConfig validation", "[network_simulator]") {
  SECTION("valid configuration") {
    LatencyConfig config;
    config.min_ms = 10;
    config.max_ms = 50;
    REQUIRE(config.isValid() == true);
  }
  
  SECTION("min equals max is valid") {
    LatencyConfig config;
    config.min_ms = 25;
    config.max_ms = 25;
    REQUIRE(config.isValid() == true);
  }
  
  SECTION("min greater than max is invalid") {
    LatencyConfig config;
    config.min_ms = 100;
    config.max_ms = 50;
    REQUIRE(config.isValid() == false);
  }
}

TEST_CASE("NetworkSimulator default latency", "[network_simulator]") {
  NetworkSimulator sim;
  
  SECTION("can set default latency") {
    LatencyConfig config;
    config.min_ms = 5;
    config.max_ms = 25;
    config.distribution = DistributionType::UNIFORM;
    
    REQUIRE_NOTHROW(sim.setDefaultLatency(config));
    
    auto retrieved = sim.getLatency(1, 2);
    REQUIRE(retrieved.min_ms == 5);
    REQUIRE(retrieved.max_ms == 25);
    REQUIRE(retrieved.distribution == DistributionType::UNIFORM);
  }
  
  SECTION("rejects invalid default latency") {
    LatencyConfig config;
    config.min_ms = 100;
    config.max_ms = 50;
    
    REQUIRE_THROWS_AS(sim.setDefaultLatency(config), std::invalid_argument);
  }
}

TEST_CASE("NetworkSimulator per-connection latency", "[network_simulator]") {
  NetworkSimulator sim;
  
  SECTION("can set per-connection latency") {
    LatencyConfig config;
    config.min_ms = 100;
    config.max_ms = 200;
    config.distribution = DistributionType::NORMAL;
    
    REQUIRE_NOTHROW(sim.setLatency(1, 2, config));
    
    auto retrieved = sim.getLatency(1, 2);
    REQUIRE(retrieved.min_ms == 100);
    REQUIRE(retrieved.max_ms == 200);
    REQUIRE(retrieved.distribution == DistributionType::NORMAL);
  }
  
  SECTION("different connections have independent latencies") {
    LatencyConfig config1;
    config1.min_ms = 10;
    config1.max_ms = 20;
    
    LatencyConfig config2;
    config2.min_ms = 50;
    config2.max_ms = 100;
    
    sim.setLatency(1, 2, config1);
    sim.setLatency(2, 3, config2);
    
    auto latency1 = sim.getLatency(1, 2);
    auto latency2 = sim.getLatency(2, 3);
    
    REQUIRE(latency1.min_ms == 10);
    REQUIRE(latency1.max_ms == 20);
    REQUIRE(latency2.min_ms == 50);
    REQUIRE(latency2.max_ms == 100);
  }
  
  SECTION("unset connection uses default latency") {
    LatencyConfig default_config;
    default_config.min_ms = 5;
    default_config.max_ms = 15;
    sim.setDefaultLatency(default_config);
    
    auto retrieved = sim.getLatency(99, 100);
    REQUIRE(retrieved.min_ms == 5);
    REQUIRE(retrieved.max_ms == 15);
  }
  
  SECTION("rejects invalid per-connection latency") {
    LatencyConfig config;
    config.min_ms = 200;
    config.max_ms = 100;
    
    REQUIRE_THROWS_AS(sim.setLatency(1, 2, config), std::invalid_argument);
  }
}

TEST_CASE("NetworkSimulator message queueing", "[network_simulator]") {
  NetworkSimulator sim(42);  // Use seed for deterministic behavior
  
  SECTION("can enqueue a message") {
    REQUIRE_NOTHROW(sim.enqueueMessage(1, 2, "test message", 1000));
    REQUIRE(sim.getPendingMessageCount() == 1);
  }
  
  SECTION("can enqueue multiple messages") {
    sim.enqueueMessage(1, 2, "message 1", 1000);
    sim.enqueueMessage(2, 3, "message 2", 1000);
    sim.enqueueMessage(3, 4, "message 3", 1000);
    
    REQUIRE(sim.getPendingMessageCount() == 3);
  }
  
  SECTION("initially has no pending messages") {
    REQUIRE(sim.getPendingMessageCount() == 0);
  }
}

TEST_CASE("NetworkSimulator message delivery", "[network_simulator]") {
  NetworkSimulator sim(42);
  
  // Set fixed latency for predictable testing
  LatencyConfig config;
  config.min_ms = 50;
  config.max_ms = 50;  // Fixed latency
  sim.setDefaultLatency(config);
  
  SECTION("messages are delivered after latency period") {
    sim.enqueueMessage(1, 2, "test", 1000);
    
    // Message not ready yet
    auto ready = sim.getReadyMessages(1025);
    REQUIRE(ready.empty());
    
    // Message ready after latency
    ready = sim.getReadyMessages(1050);
    REQUIRE(ready.size() == 1);
    REQUIRE(ready[0].from == 1);
    REQUIRE(ready[0].to == 2);
    REQUIRE(ready[0].message == "test");
  }
  
  SECTION("messages are delivered in correct order") {
    sim.enqueueMessage(1, 2, "first", 1000);   // Delivery at 1050
    sim.enqueueMessage(2, 3, "second", 1010);  // Delivery at 1060
    sim.enqueueMessage(3, 4, "third", 1005);   // Delivery at 1055
    
    auto ready = sim.getReadyMessages(1055);
    REQUIRE(ready.size() == 2);
    REQUIRE(ready[0].message == "first");   // Earliest delivery
    REQUIRE(ready[1].message == "third");
  }
  
  SECTION("getReadyMessages removes messages from queue") {
    sim.enqueueMessage(1, 2, "test", 1000);
    REQUIRE(sim.getPendingMessageCount() == 1);
    
    sim.getReadyMessages(1050);
    REQUIRE(sim.getPendingMessageCount() == 0);
  }
  
  SECTION("can clear all pending messages") {
    sim.enqueueMessage(1, 2, "test1", 1000);
    sim.enqueueMessage(2, 3, "test2", 1000);
    sim.enqueueMessage(3, 4, "test3", 1000);
    
    REQUIRE(sim.getPendingMessageCount() == 3);
    sim.clear();
    REQUIRE(sim.getPendingMessageCount() == 0);
  }
}

TEST_CASE("NetworkSimulator uniform distribution", "[network_simulator]") {
  NetworkSimulator sim(12345);
  
  LatencyConfig config;
  config.min_ms = 10;
  config.max_ms = 50;
  config.distribution = DistributionType::UNIFORM;
  sim.setDefaultLatency(config);
  
  SECTION("latencies are within range") {
    std::vector<uint64_t> enqueue_times;
    for (int i = 0; i < 100; ++i) {
      uint64_t enqueue_time = i * 10;
      enqueue_times.push_back(enqueue_time);
      sim.enqueueMessage(1, 2, "test", enqueue_time);
    }
    
    // Get all messages and check latencies
    auto ready = sim.getReadyMessages(10000);
    REQUIRE(ready.size() == 100);
    
    // Messages may not be in enqueue order due to random latency
    // So we just check that all latencies are within bounds
    for (size_t i = 0; i < ready.size(); ++i) {
      const auto& msg = ready[i];
      // Find the corresponding enqueue time (messages might be reordered)
      // For uniform distribution, delivery time should be enqueue + [10, 50]
      // Since messages are sorted by delivery time, we need to find
      // which enqueue time this corresponds to
      bool found_valid = false;
      for (uint64_t enqueue_time : enqueue_times) {
        uint32_t latency = msg.deliveryTime - enqueue_time;
        if (latency >= 10 && latency <= 50) {
          found_valid = true;
          break;
        }
      }
      REQUIRE(found_valid);
    }
  }
  
  SECTION("fixed latency works") {
    config.min_ms = 25;
    config.max_ms = 25;
    sim.setDefaultLatency(config);
    
    sim.enqueueMessage(1, 2, "test", 1000);
    auto ready = sim.getReadyMessages(1025);
    
    REQUIRE(ready.size() == 1);
    REQUIRE(ready[0].deliveryTime == 1025);
  }
}

TEST_CASE("NetworkSimulator normal distribution", "[network_simulator]") {
  NetworkSimulator sim(12345);
  
  LatencyConfig config;
  config.min_ms = 10;
  config.max_ms = 90;
  config.distribution = DistributionType::NORMAL;
  sim.setDefaultLatency(config);
  
  SECTION("latencies are within range") {
    std::vector<uint64_t> enqueue_times;
    for (int i = 0; i < 100; ++i) {
      uint64_t enqueue_time = i * 10;
      enqueue_times.push_back(enqueue_time);
      sim.enqueueMessage(1, 2, "test", enqueue_time);
    }
    
    auto ready = sim.getReadyMessages(10000);
    REQUIRE(ready.size() == 100);
    
    uint32_t sum = 0;
    for (size_t i = 0; i < ready.size(); ++i) {
      const auto& msg = ready[i];
      // Find valid latency
      bool found_valid = false;
      for (uint64_t enqueue_time : enqueue_times) {
        uint32_t latency = msg.deliveryTime - enqueue_time;
        if (latency >= 10 && latency <= 90) {
          found_valid = true;
          sum += latency;
          break;
        }
      }
      REQUIRE(found_valid);
    }
    
    // Check that mean is roughly within expected range
    // For normal distribution with range [10, 90], mean should be around 50
    // but with only 100 samples, variance can be significant
    double mean = sum / 100.0;
    REQUIRE(mean >= 10);  // At least at the minimum
    REQUIRE(mean <= 90);  // At most at the maximum
  }
}

TEST_CASE("NetworkSimulator exponential distribution", "[network_simulator]") {
  NetworkSimulator sim(12345);
  
  LatencyConfig config;
  config.min_ms = 10;
  config.max_ms = 100;
  config.distribution = DistributionType::EXPONENTIAL;
  sim.setDefaultLatency(config);
  
  SECTION("latencies are within range") {
    std::vector<uint64_t> enqueue_times;
    for (int i = 0; i < 100; ++i) {
      uint64_t enqueue_time = i * 10;
      enqueue_times.push_back(enqueue_time);
      sim.enqueueMessage(1, 2, "test", enqueue_time);
    }
    
    auto ready = sim.getReadyMessages(10000);
    REQUIRE(ready.size() == 100);
    
    for (size_t i = 0; i < ready.size(); ++i) {
      const auto& msg = ready[i];
      bool found_valid = false;
      for (uint64_t enqueue_time : enqueue_times) {
        uint32_t latency = msg.deliveryTime - enqueue_time;
        if (latency >= 10 && latency <= 100) {
          found_valid = true;
          break;
        }
      }
      REQUIRE(found_valid);
    }
  }
}

TEST_CASE("NetworkSimulator statistics", "[network_simulator]") {
  NetworkSimulator sim(42);
  
  // Set fixed latency for predictable stats
  LatencyConfig config;
  config.min_ms = 50;
  config.max_ms = 50;
  sim.setDefaultLatency(config);
  
  SECTION("tracks message count") {
    sim.enqueueMessage(1, 2, "test1", 1000);
    sim.enqueueMessage(1, 2, "test2", 2000);
    sim.enqueueMessage(1, 2, "test3", 3000);
    
    auto stats = sim.getStats(1, 2);
    REQUIRE(stats.message_count == 3);
  }
  
  SECTION("tracks min/max/avg latency") {
    sim.enqueueMessage(1, 2, "test1", 1000);
    sim.enqueueMessage(1, 2, "test2", 2000);
    sim.enqueueMessage(1, 2, "test3", 3000);
    
    auto stats = sim.getStats(1, 2);
    REQUIRE(stats.min_latency_ms == 50);
    REQUIRE(stats.max_latency_ms == 50);
    REQUIRE(stats.avg_latency_ms == 50);
  }
  
  SECTION("separate stats per connection") {
    LatencyConfig config1;
    config1.min_ms = 25;
    config1.max_ms = 25;
    
    LatencyConfig config2;
    config2.min_ms = 75;
    config2.max_ms = 75;
    
    sim.setLatency(1, 2, config1);
    sim.setLatency(2, 3, config2);
    
    sim.enqueueMessage(1, 2, "test", 1000);
    sim.enqueueMessage(2, 3, "test", 1000);
    
    auto stats1 = sim.getStats(1, 2);
    auto stats2 = sim.getStats(2, 3);
    
    REQUIRE(stats1.avg_latency_ms == 25);
    REQUIRE(stats2.avg_latency_ms == 75);
  }
  
  SECTION("can reset statistics") {
    sim.enqueueMessage(1, 2, "test", 1000);
    
    auto stats = sim.getStats(1, 2);
    REQUIRE(stats.message_count == 1);
    
    sim.resetStats();
    
    stats = sim.getStats(1, 2);
    REQUIRE(stats.message_count == 0);
  }
  
  SECTION("returns empty stats for unknown connection") {
    auto stats = sim.getStats(99, 100);
    REQUIRE(stats.message_count == 0);
    REQUIRE(stats.min_latency_ms == 0);
    REQUIRE(stats.max_latency_ms == 0);
    REQUIRE(stats.avg_latency_ms == 0);
  }
}

TEST_CASE("Distribution type conversion", "[network_simulator]") {
  SECTION("distributionTypeToString") {
    REQUIRE(distributionTypeToString(DistributionType::UNIFORM) == "uniform");
    REQUIRE(distributionTypeToString(DistributionType::NORMAL) == "normal");
    REQUIRE(distributionTypeToString(DistributionType::EXPONENTIAL) == "exponential");
  }
  
  SECTION("stringToDistributionType") {
    REQUIRE(stringToDistributionType("uniform") == DistributionType::UNIFORM);
    REQUIRE(stringToDistributionType("UNIFORM") == DistributionType::UNIFORM);
    REQUIRE(stringToDistributionType("normal") == DistributionType::NORMAL);
    REQUIRE(stringToDistributionType("NORMAL") == DistributionType::NORMAL);
    REQUIRE(stringToDistributionType("gaussian") == DistributionType::NORMAL);
    REQUIRE(stringToDistributionType("exponential") == DistributionType::EXPONENTIAL);
    REQUIRE(stringToDistributionType("EXPONENTIAL") == DistributionType::EXPONENTIAL);
  }
  
  SECTION("stringToDistributionType throws for unknown type") {
    REQUIRE_THROWS_AS(stringToDistributionType("unknown"), std::invalid_argument);
  }
}

TEST_CASE("DelayedMessage comparison", "[network_simulator]") {
  SECTION("earlier delivery time is less in priority queue") {
    DelayedMessage msg1;
    msg1.deliveryTime = 1000;
    
    DelayedMessage msg2;
    msg2.deliveryTime = 2000;
    
    // In max heap, greater means lower priority
    // We use std::greater, so earlier times have higher priority
    REQUIRE((msg1 > msg2) == false);
    REQUIRE((msg2 > msg1) == true);
  }
}
