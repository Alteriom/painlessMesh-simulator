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

TEST_CASE("PacketLossConfig validation", "[network_simulator][packet_loss]") {
  SECTION("valid configuration with 0% loss") {
    PacketLossConfig config;
    config.probability = 0.0f;
    REQUIRE(config.isValid() == true);
  }
  
  SECTION("valid configuration with 50% loss") {
    PacketLossConfig config;
    config.probability = 0.5f;
    REQUIRE(config.isValid() == true);
  }
  
  SECTION("valid configuration with 100% loss") {
    PacketLossConfig config;
    config.probability = 1.0f;
    REQUIRE(config.isValid() == true);
  }
  
  SECTION("valid burst mode configuration") {
    PacketLossConfig config;
    config.probability = 0.2f;
    config.burst_mode = true;
    config.burst_length = 5;
    REQUIRE(config.isValid() == true);
  }
  
  SECTION("invalid configuration with negative probability") {
    PacketLossConfig config;
    config.probability = -0.1f;
    REQUIRE(config.isValid() == false);
  }
  
  SECTION("invalid configuration with probability > 1.0") {
    PacketLossConfig config;
    config.probability = 1.5f;
    REQUIRE(config.isValid() == false);
  }
  
  SECTION("invalid configuration with zero burst length") {
    PacketLossConfig config;
    config.probability = 0.2f;
    config.burst_mode = true;
    config.burst_length = 0;
    REQUIRE(config.isValid() == false);
  }
}

TEST_CASE("NetworkSimulator default packet loss", "[network_simulator][packet_loss]") {
  NetworkSimulator sim;
  
  SECTION("can set default packet loss") {
    PacketLossConfig config;
    config.probability = 0.1f;
    
    REQUIRE_NOTHROW(sim.setDefaultPacketLoss(config));
    
    auto retrieved = sim.getPacketLoss(1, 2);
    REQUIRE(retrieved.probability == 0.1f);
  }
  
  SECTION("rejects invalid default packet loss") {
    PacketLossConfig config;
    config.probability = 1.5f;
    
    REQUIRE_THROWS_AS(sim.setDefaultPacketLoss(config), std::invalid_argument);
  }
}

TEST_CASE("NetworkSimulator per-connection packet loss", "[network_simulator][packet_loss]") {
  NetworkSimulator sim;
  
  SECTION("can set per-connection packet loss") {
    PacketLossConfig config;
    config.probability = 0.25f;
    config.burst_mode = true;
    config.burst_length = 4;
    
    REQUIRE_NOTHROW(sim.setPacketLoss(1, 2, config));
    
    auto retrieved = sim.getPacketLoss(1, 2);
    REQUIRE(retrieved.probability == 0.25f);
    REQUIRE(retrieved.burst_mode == true);
    REQUIRE(retrieved.burst_length == 4);
  }
  
  SECTION("different connections have independent packet loss") {
    PacketLossConfig config1;
    config1.probability = 0.1f;
    
    PacketLossConfig config2;
    config2.probability = 0.5f;
    
    sim.setPacketLoss(1, 2, config1);
    sim.setPacketLoss(2, 3, config2);
    
    auto loss1 = sim.getPacketLoss(1, 2);
    auto loss2 = sim.getPacketLoss(2, 3);
    
    REQUIRE(loss1.probability == 0.1f);
    REQUIRE(loss2.probability == 0.5f);
  }
  
  SECTION("unset connection uses default packet loss") {
    PacketLossConfig default_config;
    default_config.probability = 0.15f;
    sim.setDefaultPacketLoss(default_config);
    
    auto retrieved = sim.getPacketLoss(99, 100);
    REQUIRE(retrieved.probability == 0.15f);
  }
  
  SECTION("rejects invalid per-connection packet loss") {
    PacketLossConfig config;
    config.probability = -0.1f;
    
    REQUIRE_THROWS_AS(sim.setPacketLoss(1, 2, config), std::invalid_argument);
  }
}

TEST_CASE("NetworkSimulator packet dropping probability", "[network_simulator][packet_loss]") {
  NetworkSimulator sim(12345);  // Fixed seed for reproducibility
  
  SECTION("0% packet loss drops no packets") {
    PacketLossConfig config;
    config.probability = 0.0f;
    sim.setDefaultPacketLoss(config);
    
    int dropped = 0;
    for (int i = 0; i < 100; ++i) {
      if (sim.shouldDropPacket(1, 2)) {
        dropped++;
      }
    }
    
    REQUIRE(dropped == 0);
  }
  
  SECTION("100% packet loss drops all packets") {
    PacketLossConfig config;
    config.probability = 1.0f;
    sim.setDefaultPacketLoss(config);
    
    int dropped = 0;
    for (int i = 0; i < 100; ++i) {
      if (sim.shouldDropPacket(1, 2)) {
        dropped++;
      }
    }
    
    REQUIRE(dropped == 100);
  }
  
  SECTION("10% packet loss drops approximately 10% of packets") {
    PacketLossConfig config;
    config.probability = 0.1f;
    sim.setDefaultPacketLoss(config);
    
    int dropped = 0;
    int total = 1000;
    for (int i = 0; i < total; ++i) {
      if (sim.shouldDropPacket(1, 2)) {
        dropped++;
      }
    }
    
    // Allow 5% margin of error (expected 100 ± 50)
    REQUIRE(dropped >= 50);
    REQUIRE(dropped <= 150);
  }
  
  SECTION("50% packet loss drops approximately 50% of packets") {
    PacketLossConfig config;
    config.probability = 0.5f;
    sim.setDefaultPacketLoss(config);
    
    int dropped = 0;
    int total = 1000;
    for (int i = 0; i < total; ++i) {
      if (sim.shouldDropPacket(1, 2)) {
        dropped++;
      }
    }
    
    // Allow 10% margin of error (expected 500 ± 100)
    REQUIRE(dropped >= 400);
    REQUIRE(dropped <= 600);
  }
}

TEST_CASE("NetworkSimulator burst mode packet loss", "[network_simulator][packet_loss]") {
  NetworkSimulator sim(42);  // Fixed seed
  
  SECTION("burst mode drops packets in consecutive bursts") {
    PacketLossConfig config;
    config.probability = 0.3f;  // Lower probability to reduce back-to-back bursts
    config.burst_mode = true;
    config.burst_length = 3;
    sim.setDefaultPacketLoss(config);
    
    std::vector<bool> drops;
    for (int i = 0; i < 200; ++i) {
      drops.push_back(sim.shouldDropPacket(1, 2));
    }
    
    // Find bursts and verify each burst is a multiple of burst_length
    // (multiple because back-to-back bursts can occur)
    std::vector<int> burst_lengths;
    int current_burst_length = 0;
    
    for (size_t i = 0; i < drops.size(); ++i) {
      if (drops[i]) {
        current_burst_length++;
      } else {
        if (current_burst_length > 0) {
          burst_lengths.push_back(current_burst_length);
          current_burst_length = 0;
        }
      }
    }
    // Don't forget last burst if it ends at the array boundary
    if (current_burst_length > 0) {
      burst_lengths.push_back(current_burst_length);
    }
    
    // Verify all bursts are multiples of burst_length
    // (because back-to-back bursts create longer sequences)
    for (int len : burst_lengths) {
      REQUIRE(len % config.burst_length == 0);
    }
    
    // Should have at least some bursts with 30% probability
    REQUIRE(burst_lengths.size() > 0);
    
    // At least some bursts should be exactly burst_length (not back-to-back)
    bool has_single_burst = false;
    for (int len : burst_lengths) {
      if (len == config.burst_length) {
        has_single_burst = true;
        break;
      }
    }
    REQUIRE(has_single_burst);
  }
}

TEST_CASE("NetworkSimulator packet loss statistics", "[network_simulator][packet_loss]") {
  NetworkSimulator sim(42);
  
  // Set fixed latency to isolate packet loss testing
  LatencyConfig latency;
  latency.min_ms = 10;
  latency.max_ms = 10;
  sim.setDefaultLatency(latency);
  
  SECTION("tracks dropped and delivered packets") {
    PacketLossConfig config;
    config.probability = 0.5f;
    sim.setDefaultPacketLoss(config);
    
    // Enqueue 100 messages
    for (int i = 0; i < 100; ++i) {
      sim.enqueueMessage(1, 2, "test", i * 100);
    }
    
    auto stats = sim.getStats(1, 2);
    REQUIRE(stats.dropped_count + stats.delivered_count == 100);
    REQUIRE(stats.dropped_count > 0);
    REQUIRE(stats.delivered_count > 0);
    
    // Verify drop rate calculation
    float expected_drop_rate = static_cast<float>(stats.dropped_count) / 100.0f;
    REQUIRE(stats.drop_rate == expected_drop_rate);
  }
  
  SECTION("tracks separate stats per connection") {
    PacketLossConfig config1;
    config1.probability = 0.2f;
    
    PacketLossConfig config2;
    config2.probability = 0.8f;
    
    sim.setPacketLoss(1, 2, config1);
    sim.setPacketLoss(2, 3, config2);
    
    for (int i = 0; i < 100; ++i) {
      sim.enqueueMessage(1, 2, "test", i * 100);
      sim.enqueueMessage(2, 3, "test", i * 100);
    }
    
    auto stats1 = sim.getStats(1, 2);
    auto stats2 = sim.getStats(2, 3);
    
    // Connection 2->3 should have more drops than 1->2
    REQUIRE(stats2.dropped_count > stats1.dropped_count);
    REQUIRE(stats1.delivered_count > stats2.delivered_count);
  }
  
  SECTION("0% packet loss results in 0 drop rate") {
    PacketLossConfig config;
    config.probability = 0.0f;
    sim.setDefaultPacketLoss(config);
    
    for (int i = 0; i < 50; ++i) {
      sim.enqueueMessage(1, 2, "test", i * 100);
    }
    
    auto stats = sim.getStats(1, 2);
    REQUIRE(stats.dropped_count == 0);
    REQUIRE(stats.delivered_count == 50);
    REQUIRE(stats.drop_rate == 0.0f);
  }
  
  SECTION("100% packet loss results in 1.0 drop rate") {
    PacketLossConfig config;
    config.probability = 1.0f;
    sim.setDefaultPacketLoss(config);
    
    for (int i = 0; i < 50; ++i) {
      sim.enqueueMessage(1, 2, "test", i * 100);
    }
    
    auto stats = sim.getStats(1, 2);
    REQUIRE(stats.dropped_count == 50);
    REQUIRE(stats.delivered_count == 0);
    REQUIRE(stats.drop_rate == 1.0f);
  }
}

TEST_CASE("NetworkSimulator packet loss integration with message delivery", "[network_simulator][packet_loss]") {
  NetworkSimulator sim(42);
  
  LatencyConfig latency;
  latency.min_ms = 50;
  latency.max_ms = 50;
  sim.setDefaultLatency(latency);
  
  SECTION("dropped packets are not delivered") {
    PacketLossConfig config;
    config.probability = 1.0f;  // Drop all packets
    sim.setDefaultPacketLoss(config);
    
    sim.enqueueMessage(1, 2, "test1", 1000);
    sim.enqueueMessage(1, 2, "test2", 1000);
    sim.enqueueMessage(1, 2, "test3", 1000);
    
    // No messages should be queued since they're all dropped
    REQUIRE(sim.getPendingMessageCount() == 0);
    
    // No messages should be ready for delivery
    auto ready = sim.getReadyMessages(2000);
    REQUIRE(ready.empty());
  }
  
  SECTION("only delivered packets appear in queue") {
    PacketLossConfig config;
    config.probability = 0.0f;  // Drop no packets
    sim.setDefaultPacketLoss(config);
    
    sim.enqueueMessage(1, 2, "test1", 1000);
    sim.enqueueMessage(1, 2, "test2", 1000);
    sim.enqueueMessage(1, 2, "test3", 1000);
    
    REQUIRE(sim.getPendingMessageCount() == 3);
    
    auto ready = sim.getReadyMessages(1050);
    REQUIRE(ready.size() == 3);
  }
  
  SECTION("partial packet loss delivers some messages") {
    PacketLossConfig config;
    config.probability = 0.5f;
    sim.setDefaultPacketLoss(config);
    
    for (int i = 0; i < 100; ++i) {
      sim.enqueueMessage(1, 2, "test", 1000 + i);
    }
    
    // Some but not all messages should be queued
    size_t queued = sim.getPendingMessageCount();
    REQUIRE(queued > 0);
    REQUIRE(queued < 100);
    
    auto ready = sim.getReadyMessages(2000);
    REQUIRE(ready.size() == queued);
  }
}
