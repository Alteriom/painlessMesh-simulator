/**
 * @file test_config_loader.cpp
 * @brief Unit tests for ConfigLoader class
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include <catch2/catch_test_macros.hpp>

#include "simulator/config_loader.hpp"
#include <fstream>
#include <sstream>

using namespace simulator;

TEST_CASE("ConfigLoader parses simple YAML", "[config_loader]") {
  std::string yaml = R"(
simulation:
  name: "Test Simulation"
  duration: 60
  time_scale: 1.0

nodes:
  - id: "node-1"
    type: "sensor"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"
      mesh_port: 5555

topology:
  type: "random"
  )";
  
  ConfigLoader loader;
  auto config = loader.loadFromString(yaml);
  
  REQUIRE(config.has_value());
  REQUIRE(config->simulation.name == "Test Simulation");
  REQUIRE(config->simulation.duration == 60);
  REQUIRE(config->nodes.size() == 1);
  REQUIRE(config->nodes[0].id == "node-1");
  REQUIRE(config->nodes[0].mesh_prefix == "TestMesh");
}

TEST_CASE("ConfigLoader validates required fields", "[config_loader]") {
  SECTION("missing simulation name") {
    std::string yaml = R"(
simulation:
  duration: 60

nodes:
  - id: "node-1"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"
    )";
    
    ConfigLoader loader;
    auto config = loader.loadFromString(yaml);
    
    REQUIRE(config.has_value());
    auto errors = loader.getValidationErrors(*config);
    REQUIRE(errors.size() > 0);
  }
  
  SECTION("missing mesh_prefix") {
    std::string yaml = R"(
simulation:
  name: "Test"
  duration: 60

nodes:
  - id: "node-1"
    config:
      mesh_password: "password"
    )";
    
    ConfigLoader loader;
    auto config = loader.loadFromString(yaml);
    
    REQUIRE(config.has_value());
    auto errors = loader.getValidationErrors(*config);
    
    bool has_prefix_error = false;
    for (const auto& err : errors) {
      if (err.field.find("mesh_prefix") != std::string::npos) {
        has_prefix_error = true;
        break;
      }
    }
    REQUIRE(has_prefix_error);
  }
  
  SECTION("missing mesh_password") {
    std::string yaml = R"(
simulation:
  name: "Test"
  duration: 60

nodes:
  - id: "node-1"
    config:
      mesh_prefix: "TestMesh"
    )";
    
    ConfigLoader loader;
    auto config = loader.loadFromString(yaml);
    
    REQUIRE(config.has_value());
    auto errors = loader.getValidationErrors(*config);
    
    bool has_password_error = false;
    for (const auto& err : errors) {
      if (err.field.find("mesh_password") != std::string::npos) {
        has_password_error = true;
        break;
      }
    }
    REQUIRE(has_password_error);
  }
}

TEST_CASE("ConfigLoader expands node templates", "[config_loader]") {
  std::string yaml = R"(
simulation:
  name: "Template Test"
  duration: 60

nodes:
  - template: "sensor"
    count: 5
    id_prefix: "sensor-"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"

topology:
  type: "random"
  )";
  
  ConfigLoader loader;
  auto config = loader.loadFromString(yaml);
  
  REQUIRE(config.has_value());
  REQUIRE(config->templates.size() == 1);
  
  // Expand templates
  loader.expandTemplates(*config);
  
  REQUIRE(config->nodes.size() == 5);
  REQUIRE(config->nodes[0].id == "sensor-0");
  REQUIRE(config->nodes[1].id == "sensor-1");
  REQUIRE(config->nodes[4].id == "sensor-4");
  
  for (const auto& node : config->nodes) {
    REQUIRE(node.mesh_prefix == "TestMesh");
    REQUIRE(node.mesh_password == "password");
  }
}

TEST_CASE("ConfigLoader parses network configuration", "[config_loader]") {
  std::string yaml = R"(
simulation:
  name: "Network Test"
  duration: 60

network:
  latency:
    min: 20
    max: 100
    distribution: "normal"
  packet_loss: 0.05
  bandwidth: 2000000

nodes:
  - id: "node-1"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"

topology:
  type: "random"
  )";
  
  ConfigLoader loader;
  auto config = loader.loadFromString(yaml);
  
  REQUIRE(config.has_value());
  REQUIRE(config->network.default_latency.min_ms == 20);
  REQUIRE(config->network.default_latency.max_ms == 100);
  REQUIRE(config->network.default_latency.distribution == DistributionType::NORMAL);
  REQUIRE(config->network.packet_loss == 0.05f);
  REQUIRE(config->network.bandwidth == 2000000);
}

TEST_CASE("ConfigLoader parses specific connection latencies", "[config_loader]") {
  std::string yaml = R"(
simulation:
  name: "Latency Test"
  duration: 60

nodes:
  - template: "sensor"
    count: 3
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"

network:
  latency:
    default:
      min: 10
      max: 50
      distribution: "normal"
    
    specific_connections:
      - from: "sensor-0"
        to: "sensor-1"
        min: 100
        max: 200
        distribution: "uniform"
      
      - from: "sensor-1"
        to: "sensor-2"
        min: 5
        max: 15
        distribution: "exponential"

topology:
  type: "random"
  )";
  
  ConfigLoader loader;
  auto config = loader.loadFromString(yaml);
  
  REQUIRE(config.has_value());
  
  // Check default latency
  REQUIRE(config->network.default_latency.min_ms == 10);
  REQUIRE(config->network.default_latency.max_ms == 50);
  REQUIRE(config->network.default_latency.distribution == DistributionType::NORMAL);
  
  // Check specific connections
  REQUIRE(config->network.specific_latencies.size() == 2);
  
  // First specific connection
  REQUIRE(config->network.specific_latencies[0].from == "sensor-0");
  REQUIRE(config->network.specific_latencies[0].to == "sensor-1");
  REQUIRE(config->network.specific_latencies[0].config.min_ms == 100);
  REQUIRE(config->network.specific_latencies[0].config.max_ms == 200);
  REQUIRE(config->network.specific_latencies[0].config.distribution == DistributionType::UNIFORM);
  
  // Second specific connection
  REQUIRE(config->network.specific_latencies[1].from == "sensor-1");
  REQUIRE(config->network.specific_latencies[1].to == "sensor-2");
  REQUIRE(config->network.specific_latencies[1].config.min_ms == 5);
  REQUIRE(config->network.specific_latencies[1].config.max_ms == 15);
  REQUIRE(config->network.specific_latencies[1].config.distribution == DistributionType::EXPONENTIAL);
}

TEST_CASE("ConfigLoader validates network parameters", "[config_loader]") {
  SECTION("packet loss out of range") {
    std::string yaml = R"(
simulation:
  name: "Test"
  duration: 60

network:
  packet_loss: 1.5

nodes:
  - id: "node-1"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"
    )";
    
    ConfigLoader loader;
    auto config = loader.loadFromString(yaml);
    
    REQUIRE(config.has_value());
    auto errors = loader.getValidationErrors(*config);
    
    bool has_packet_loss_error = false;
    for (const auto& err : errors) {
      if (err.field.find("packet_loss") != std::string::npos) {
        has_packet_loss_error = true;
        break;
      }
    }
    REQUIRE(has_packet_loss_error);
  }
  
  SECTION("invalid latency range") {
    std::string yaml = R"(
simulation:
  name: "Test"
  duration: 60

network:
  latency:
    min: 100
    max: 50

nodes:
  - id: "node-1"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"
    )";
    
    ConfigLoader loader;
    auto config = loader.loadFromString(yaml);
    
    REQUIRE(config.has_value());
    auto errors = loader.getValidationErrors(*config);
    
    bool has_latency_error = false;
    for (const auto& err : errors) {
      if (err.field.find("latency") != std::string::npos) {
        has_latency_error = true;
        break;
      }
    }
    REQUIRE(has_latency_error);
  }
}

TEST_CASE("ConfigLoader parses topology configuration", "[config_loader]") {
  SECTION("random topology") {
    std::string yaml = R"(
simulation:
  name: "Test"
  duration: 60

nodes:
  - id: "node-1"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"

topology:
  type: "random"
  density: 0.5
    )";
    
    ConfigLoader loader;
    auto config = loader.loadFromString(yaml);
    
    REQUIRE(config.has_value());
    REQUIRE(config->topology.type == TopologyType::RANDOM);
    REQUIRE(config->topology.density == 0.5f);
  }
  
  SECTION("star topology") {
    std::string yaml = R"(
simulation:
  name: "Test"
  duration: 60

nodes:
  - id: "hub"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"
  - id: "node-1"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"

topology:
  type: "star"
  hub: "hub"
    )";
    
    ConfigLoader loader;
    auto config = loader.loadFromString(yaml);
    
    REQUIRE(config.has_value());
    REQUIRE(config->topology.type == TopologyType::STAR);
    REQUIRE(config->topology.hub.is_initialized());
    REQUIRE(config->topology.hub.get() == "hub");
  }
  
  SECTION("custom topology") {
    std::string yaml = R"(
simulation:
  name: "Test"
  duration: 60

nodes:
  - id: "node-1"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"
  - id: "node-2"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"

topology:
  type: "custom"
  connections:
    - ["node-1", "node-2"]
    )";
    
    ConfigLoader loader;
    auto config = loader.loadFromString(yaml);
    
    REQUIRE(config.has_value());
    REQUIRE(config->topology.type == TopologyType::CUSTOM);
    REQUIRE(config->topology.connections.size() == 1);
    REQUIRE(config->topology.connections[0].first == "node-1");
    REQUIRE(config->topology.connections[0].second == "node-2");
  }
}

TEST_CASE("ConfigLoader validates topology configuration", "[config_loader]") {
  SECTION("star topology without hub") {
    std::string yaml = R"(
simulation:
  name: "Test"
  duration: 60

nodes:
  - id: "node-1"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"

topology:
  type: "star"
    )";
    
    ConfigLoader loader;
    auto config = loader.loadFromString(yaml);
    
    REQUIRE(config.has_value());
    auto errors = loader.getValidationErrors(*config);
    
    bool has_hub_error = false;
    for (const auto& err : errors) {
      if (err.field.find("hub") != std::string::npos) {
        has_hub_error = true;
        break;
      }
    }
    REQUIRE(has_hub_error);
  }
  
  SECTION("star topology with non-existent hub") {
    std::string yaml = R"(
simulation:
  name: "Test"
  duration: 60

nodes:
  - id: "node-1"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"

topology:
  type: "star"
  hub: "non-existent"
    )";
    
    ConfigLoader loader;
    auto config = loader.loadFromString(yaml);
    
    REQUIRE(config.has_value());
    auto errors = loader.getValidationErrors(*config);
    
    bool has_hub_error = false;
    for (const auto& err : errors) {
      if (err.field.find("hub") != std::string::npos) {
        has_hub_error = true;
        break;
      }
    }
    REQUIRE(has_hub_error);
  }
}

TEST_CASE("ConfigLoader parses events", "[config_loader]") {
  std::string yaml = R"(
simulation:
  name: "Event Test"
  duration: 300

nodes:
  - id: "node-1"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"

topology:
  type: "random"

events:
  - time: 60
    action: "stop_node"
    target: "node-1"
    description: "Test stop"
  
  - time: 120
    action: "start_node"
    target: "node-1"
    description: "Test start"
  )";
  
  ConfigLoader loader;
  auto config = loader.loadFromString(yaml);
  
  REQUIRE(config.has_value());
  REQUIRE(config->events.size() == 2);
  REQUIRE(config->events[0].time == 60);
  REQUIRE(config->events[0].action == EventAction::STOP_NODE);
  REQUIRE(config->events[0].target == "node-1");
  REQUIRE(config->events[1].time == 120);
  REQUIRE(config->events[1].action == EventAction::START_NODE);
}

TEST_CASE("ConfigLoader validates event timing", "[config_loader]") {
  std::string yaml = R"(
simulation:
  name: "Test"
  duration: 60

nodes:
  - id: "node-1"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"

events:
  - time: 120
    action: "stop_node"
    target: "node-1"
  )";
  
  ConfigLoader loader;
  auto config = loader.loadFromString(yaml);
  
  REQUIRE(config.has_value());
  auto errors = loader.getValidationErrors(*config);
  
  bool has_time_error = false;
  for (const auto& err : errors) {
    if (err.field.find("time") != std::string::npos) {
      has_time_error = true;
      break;
    }
  }
  REQUIRE(has_time_error);
}

TEST_CASE("ConfigLoader parses metrics configuration", "[config_loader]") {
  std::string yaml = R"(
simulation:
  name: "Metrics Test"
  duration: 60

nodes:
  - id: "node-1"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"

topology:
  type: "random"

metrics:
  output: "results/test.csv"
  interval: 10
  collect:
    - message_count
    - delivery_rate
  export:
    - csv
    - json
  )";
  
  ConfigLoader loader;
  auto config = loader.loadFromString(yaml);
  
  REQUIRE(config.has_value());
  REQUIRE(config->metrics.output == "results/test.csv");
  REQUIRE(config->metrics.interval == 10);
  REQUIRE(config->metrics.collect.size() == 2);
  REQUIRE(config->metrics.collect[0] == "message_count");
  REQUIRE(config->metrics.export_formats.size() == 2);
  REQUIRE(config->metrics.export_formats[0] == "csv");
}

TEST_CASE("ConfigLoader detects duplicate node IDs", "[config_loader]") {
  std::string yaml = R"(
simulation:
  name: "Test"
  duration: 60

nodes:
  - id: "node-1"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"
  - id: "node-1"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"

topology:
  type: "random"
  )";
  
  ConfigLoader loader;
  auto config = loader.loadFromString(yaml);
  
  REQUIRE(config.has_value());
  auto errors = loader.getValidationErrors(*config);
  
  bool has_duplicate_error = false;
  for (const auto& err : errors) {
    if (err.message.find("Duplicate") != std::string::npos) {
      has_duplicate_error = true;
      break;
    }
  }
  REQUIRE(has_duplicate_error);
}

TEST_CASE("ConfigLoader handles file I/O", "[config_loader]") {
  // Create a temporary YAML file
  std::string temp_file = "/tmp/test_config.yaml";
  std::ofstream file(temp_file);
  file << R"(
simulation:
  name: "File Test"
  duration: 60

nodes:
  - id: "node-1"
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"

topology:
  type: "random"
  )";
  file.close();
  
  ConfigLoader loader;
  auto config = loader.loadFromFile(temp_file);
  
  REQUIRE(config.has_value());
  REQUIRE(config->simulation.name == "File Test");
  
  // Clean up
  std::remove(temp_file.c_str());
}

TEST_CASE("ConfigLoader parses connection events", "[config_loader]") {
  SECTION("connection_drop event") {
    std::string yaml = R"(
simulation:
  name: "Connection Drop Test"
  duration: 60

nodes:
  - id: "node-1"
    mesh_prefix: "TestMesh"
    mesh_password: "password"

events:
  - time: 30
    action: connection_drop
    from: node-1
    to: node-2
    description: "Drop connection"
    )";
    
    ConfigLoader loader;
    auto config = loader.loadFromString(yaml);
    
    REQUIRE(config.has_value());
    REQUIRE(config->events.size() == 1);
    REQUIRE(config->events[0].action == EventAction::CONNECTION_DROP);
    REQUIRE(config->events[0].time == 30);
    REQUIRE(config->events[0].from == "node-1");
    REQUIRE(config->events[0].to == "node-2");
  }
  
  SECTION("connection_restore event") {
    std::string yaml = R"(
simulation:
  name: "Connection Restore Test"
  duration: 60

nodes:
  - id: "node-1"
    mesh_prefix: "TestMesh"
    mesh_password: "password"

events:
  - time: 60
    action: connection_restore
    from: node-1
    to: node-2
    )";
    
    ConfigLoader loader;
    auto config = loader.loadFromString(yaml);
    
    REQUIRE(config.has_value());
    REQUIRE(config->events.size() == 1);
    REQUIRE(config->events[0].action == EventAction::CONNECTION_RESTORE);
    REQUIRE(config->events[0].time == 60);
    REQUIRE(config->events[0].from == "node-1");
    REQUIRE(config->events[0].to == "node-2");
  }
  
  SECTION("connection_degrade event with default parameters") {
    std::string yaml = R"(
simulation:
  name: "Connection Degrade Test"
  duration: 60

nodes:
  - id: "node-1"
    mesh_prefix: "TestMesh"
    mesh_password: "password"

events:
  - time: 45
    action: connection_degrade
    from: node-3
    to: node-4
    )";
    
    ConfigLoader loader;
    auto config = loader.loadFromString(yaml);
    
    REQUIRE(config.has_value());
    REQUIRE(config->events.size() == 1);
    REQUIRE(config->events[0].action == EventAction::CONNECTION_DEGRADE);
    REQUIRE(config->events[0].time == 45);
    REQUIRE(config->events[0].from == "node-3");
    REQUIRE(config->events[0].to == "node-4");
    REQUIRE(config->events[0].latency == 500);      // Default
    REQUIRE(config->events[0].packet_loss == 0.30f); // Default
  }
  
  SECTION("connection_degrade event with custom parameters") {
    std::string yaml = R"(
simulation:
  name: "Connection Degrade Test"
  duration: 60

nodes:
  - id: "node-1"
    mesh_prefix: "TestMesh"
    mesh_password: "password"

events:
  - time: 45
    action: connection_degrade
    from: node-3
    to: node-4
    latency: 1000
    packet_loss: 0.50
    )";
    
    ConfigLoader loader;
    auto config = loader.loadFromString(yaml);
    
    REQUIRE(config.has_value());
    REQUIRE(config->events.size() == 1);
    REQUIRE(config->events[0].action == EventAction::CONNECTION_DEGRADE);
    REQUIRE(config->events[0].latency == 1000);
    REQUIRE(config->events[0].packet_loss == 0.50f);
  }
  
  SECTION("multiple connection events") {
    std::string yaml = R"(
simulation:
  name: "Multiple Connection Events"
  duration: 120

nodes:
  - id: "node-1"
    mesh_prefix: "TestMesh"
    mesh_password: "password"

events:
  - time: 20
    action: connection_drop
    from: node-1
    to: node-2
  
  - time: 40
    action: connection_degrade
    from: node-2
    to: node-3
    latency: 800
    packet_loss: 0.35
  
  - time: 60
    action: connection_restore
    from: node-1
    to: node-2
    )";
    
    ConfigLoader loader;
    auto config = loader.loadFromString(yaml);
    
    REQUIRE(config.has_value());
    REQUIRE(config->events.size() == 3);
    
    REQUIRE(config->events[0].action == EventAction::CONNECTION_DROP);
    REQUIRE(config->events[0].time == 20);
    
    REQUIRE(config->events[1].action == EventAction::CONNECTION_DEGRADE);
    REQUIRE(config->events[1].time == 40);
    REQUIRE(config->events[1].latency == 800);
    REQUIRE(config->events[1].packet_loss == 0.35f);
    
    REQUIRE(config->events[2].action == EventAction::CONNECTION_RESTORE);
    REQUIRE(config->events[2].time == 60);
  }
}
