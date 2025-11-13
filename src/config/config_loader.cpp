/**
 * @file config_loader.cpp
 * @brief Implementation of configuration loader
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

// IMPORTANT: Include platform_compat.hpp FIRST on Windows
#include "simulator/platform_compat.hpp"

#include "simulator/config_loader.hpp"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <functional>

namespace simulator {

namespace {
  // Helper to check if YAML node has key
  bool hasKey(const YAML::Node& node, const std::string& key) {
    return node[key].IsDefined();
  }
  
  // Helper to safely get string with default
  std::string getString(const YAML::Node& node, const std::string& key, 
                       const std::string& default_value = "") {
    return hasKey(node, key) ? node[key].as<std::string>() : default_value;
  }
  
  // Helper to safely get uint32_t with default
  uint32_t getUInt32(const YAML::Node& node, const std::string& key, 
                     uint32_t default_value = 0) {
    return hasKey(node, key) ? node[key].as<uint32_t>() : default_value;
  }
  
  // Helper to safely get uint16_t with default
  uint16_t getUInt16(const YAML::Node& node, const std::string& key, 
                     uint16_t default_value = 0) {
    return hasKey(node, key) ? node[key].as<uint16_t>() : default_value;
  }
  
  // Helper to safely get float with default
  float getFloat(const YAML::Node& node, const std::string& key, 
                 float default_value = 0.0f) {
    return hasKey(node, key) ? node[key].as<float>() : default_value;
  }
  
  // Helper to safely get uint64_t with default
  uint64_t getUInt64(const YAML::Node& node, const std::string& key, 
                     uint64_t default_value = 0) {
    return hasKey(node, key) ? node[key].as<uint64_t>() : default_value;
  }
  
  // Helper to safely get bool with default
  bool getBool(const YAML::Node& node, const std::string& key, 
               bool default_value = false) {
    return hasKey(node, key) ? node[key].as<bool>() : default_value;
  }
}

boost::optional<ScenarioConfig> ConfigLoader::loadFromFile(const std::string& filepath) {
  try {
    std::ifstream file(filepath);
    if (!file.is_open()) {
      last_error_ = "Failed to open file: " + filepath;
      return boost::none;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return loadFromString(buffer.str());
    
  } catch (const std::exception& e) {
    last_error_ = std::string("Error loading file: ") + e.what();
    return boost::none;
  }
}

boost::optional<ScenarioConfig> ConfigLoader::loadFromString(const std::string& yaml_content) {
  try {
    YAML::Node root = YAML::Load(yaml_content);
    
    ScenarioConfig config;
    
    // Parse simulation section
    if (hasKey(root, "simulation")) {
      config.simulation = parseSimulation(root["simulation"]);
    }
    
    // Parse network section
    if (hasKey(root, "network")) {
      config.network = parseNetwork(root["network"]);
    }
    
    // Parse nodes section
    if (hasKey(root, "nodes") && root["nodes"].IsSequence()) {
      for (const auto& node_yaml : root["nodes"]) {
        // Check if this is a template or individual node
        if (hasKey(node_yaml, "template")) {
          config.templates.push_back(parseTemplate(node_yaml));
        } else {
          config.nodes.push_back(parseNode(node_yaml));
        }
      }
    }
    
    // Parse topology section
    if (hasKey(root, "topology")) {
      config.topology = parseTopology(root["topology"]);
    }
    
    // Parse events section
    if (hasKey(root, "events") && root["events"].IsSequence()) {
      for (const auto& event_yaml : root["events"]) {
        config.events.push_back(parseEvent(event_yaml));
      }
    }
    
    // Parse metrics section
    if (hasKey(root, "metrics")) {
      config.metrics = parseMetrics(root["metrics"]);
    }
    
    // Note: Template expansion is done later by caller to allow inspection
    // Call expandTemplates(config) after loading if needed
    
    return config;
    
  } catch (const YAML::Exception& e) {
    last_error_ = std::string("YAML parsing error: ") + e.what();
    return boost::none;
  } catch (const std::exception& e) {
    last_error_ = std::string("Configuration error: ") + e.what();
    return boost::none;
  }
}

SimulationConfig ConfigLoader::parseSimulation(const YAML::Node& node) {
  SimulationConfig config;
  config.name = getString(node, "name", "");  // No default - required field
  config.description = getString(node, "description");
  config.duration = getUInt32(node, "duration", 0);
  config.time_scale = getFloat(node, "time_scale", 1.0f);
  config.seed = getUInt32(node, "seed", 0);
  
  return config;
}

NetworkConfig ConfigLoader::parseNetwork(const YAML::Node& node) {
  NetworkConfig config;
  
  // Parse latency
  if (hasKey(node, "latency")) {
    const auto& latency_node = node["latency"];
    
    // Parse default latency
    if (hasKey(latency_node, "default")) {
      const auto& default_node = latency_node["default"];
      config.default_latency.min_ms = getUInt32(default_node, "min", 10);
      config.default_latency.max_ms = getUInt32(default_node, "max", 50);
      std::string dist_str = getString(default_node, "distribution", "normal");
      try {
        config.default_latency.distribution = stringToDistributionType(dist_str);
      } catch (const std::exception& e) {
        // Default to normal if invalid
        config.default_latency.distribution = DistributionType::NORMAL;
      }
    } else {
      // Old format: latency directly under "latency" key
      config.default_latency.min_ms = getUInt32(latency_node, "min", 10);
      config.default_latency.max_ms = getUInt32(latency_node, "max", 50);
      std::string dist_str = getString(latency_node, "distribution", "normal");
      try {
        config.default_latency.distribution = stringToDistributionType(dist_str);
      } catch (const std::exception& e) {
        config.default_latency.distribution = DistributionType::NORMAL;
      }
    }
    
    // Parse specific connections
    if (hasKey(latency_node, "specific_connections") && 
        latency_node["specific_connections"].IsSequence()) {
      for (const auto& conn_node : latency_node["specific_connections"]) {
        ConnectionLatencyConfig conn_config;
        conn_config.from = getString(conn_node, "from");
        conn_config.to = getString(conn_node, "to");
        conn_config.config.min_ms = getUInt32(conn_node, "min", config.default_latency.min_ms);
        conn_config.config.max_ms = getUInt32(conn_node, "max", config.default_latency.max_ms);
        
        std::string dist_str = getString(conn_node, "distribution", 
                                        distributionTypeToString(config.default_latency.distribution));
        try {
          conn_config.config.distribution = stringToDistributionType(dist_str);
        } catch (const std::exception& e) {
          conn_config.config.distribution = config.default_latency.distribution;
        }
        
        config.specific_latencies.push_back(conn_config);
      }
    }
  }
  
  // Parse packet_loss configuration
  if (hasKey(node, "packet_loss")) {
    const auto& packet_loss_node = node["packet_loss"];
    
    if (packet_loss_node.IsMap()) {
      // New structured format
      
      // Parse default packet loss
      if (hasKey(packet_loss_node, "default")) {
        const auto& default_node = packet_loss_node["default"];
        config.default_packet_loss.probability = getFloat(default_node, "probability", 0.0f);
        config.default_packet_loss.burst_mode = getBool(default_node, "burst_mode", false);
        config.default_packet_loss.burst_length = getUInt32(default_node, "burst_length", 3);
      }
      
      // Parse specific connections
      if (hasKey(packet_loss_node, "specific_connections") && 
          packet_loss_node["specific_connections"].IsSequence()) {
        for (const auto& conn_node : packet_loss_node["specific_connections"]) {
          ConnectionPacketLossConfig conn_config;
          conn_config.from = getString(conn_node, "from");
          conn_config.to = getString(conn_node, "to");
          conn_config.config.probability = getFloat(conn_node, "probability", 
                                                    config.default_packet_loss.probability);
          conn_config.config.burst_mode = getBool(conn_node, "burst_mode", 
                                                  config.default_packet_loss.burst_mode);
          conn_config.config.burst_length = getUInt32(conn_node, "burst_length", 
                                                      config.default_packet_loss.burst_length);
          
          config.specific_packet_losses.push_back(conn_config);
        }
      }
    } else {
      // Legacy format: simple float value
      float legacy_loss = packet_loss_node.as<float>();
      config.default_packet_loss.probability = legacy_loss;
      config.packet_loss = legacy_loss;  // Keep for backward compatibility
    }
  } else {
    // If no packet_loss key, check for legacy format at top level
    config.packet_loss = getFloat(node, "packet_loss", 0.0f);
    config.default_packet_loss.probability = config.packet_loss;
  }
  
  config.bandwidth = getUInt64(node, "bandwidth", 1000000);
  
  return config;
}

NodeConfigExtended ConfigLoader::parseNode(const YAML::Node& node) {
  NodeConfigExtended config;
  config.id = getString(node, "id");
  config.nodeId = generateNodeId(config.id);
  config.type = getString(node, "type");
  config.firmware = getString(node, "firmware");
  
  // Parse position
  if (hasKey(node, "position") && node["position"].IsSequence()) {
    for (const auto& pos : node["position"]) {
      config.position.push_back(pos.as<int>());
    }
  }
  
  // Parse config section
  if (hasKey(node, "config")) {
    const auto& cfg = node["config"];
    config.mesh_prefix = getString(cfg, "mesh_prefix");
    config.mesh_password = getString(cfg, "mesh_password");
    config.mesh_port = getUInt16(cfg, "mesh_port", 5555);
    
    // Extended configuration
    if (hasKey(cfg, "sensor_interval")) {
      config.sensor_interval = cfg["sensor_interval"].as<uint32_t>();
    }
    if (hasKey(cfg, "mqtt_broker")) {
      config.mqtt_broker = cfg["mqtt_broker"].as<std::string>();
    }
    if (hasKey(cfg, "mqtt_port")) {
      config.mqtt_port = cfg["mqtt_port"].as<uint16_t>();
    }
    if (hasKey(cfg, "mqtt_topic_prefix")) {
      config.mqtt_topic_prefix = cfg["mqtt_topic_prefix"].as<std::string>();
    }
  }
  
  return config;
}

NodeTemplate ConfigLoader::parseTemplate(const YAML::Node& node) {
  NodeTemplate tmpl;
  tmpl.template_name = getString(node, "template");
  tmpl.count = getUInt32(node, "count", 1);
  tmpl.id_prefix = getString(node, "id_prefix", tmpl.template_name + "-");
  
  // Parse base configuration
  tmpl.base_config.firmware = getString(node, "firmware");
  tmpl.base_config.type = tmpl.template_name;
  
  if (hasKey(node, "config")) {
    const auto& cfg = node["config"];
    tmpl.base_config.mesh_prefix = getString(cfg, "mesh_prefix");
    tmpl.base_config.mesh_password = getString(cfg, "mesh_password");
    tmpl.base_config.mesh_port = getUInt16(cfg, "mesh_port", 5555);
    
    if (hasKey(cfg, "sensor_interval")) {
      tmpl.base_config.sensor_interval = cfg["sensor_interval"].as<uint32_t>();
    }
    if (hasKey(cfg, "mqtt_broker")) {
      tmpl.base_config.mqtt_broker = cfg["mqtt_broker"].as<std::string>();
    }
    if (hasKey(cfg, "mqtt_port")) {
      tmpl.base_config.mqtt_port = cfg["mqtt_port"].as<uint16_t>();
    }
    if (hasKey(cfg, "mqtt_topic_prefix")) {
      tmpl.base_config.mqtt_topic_prefix = cfg["mqtt_topic_prefix"].as<std::string>();
    }
  }
  
  return tmpl;
}

TopologyConfig ConfigLoader::parseTopology(const YAML::Node& node) {
  TopologyConfig config;
  
  std::string type_str = getString(node, "type", "random");
  config.type = stringToTopologyType(type_str);
  
  if (hasKey(node, "hub")) {
    config.hub = node["hub"].as<std::string>();
  }
  
  config.density = getFloat(node, "density", 0.3f);
  config.bidirectional = getBool(node, "bidirectional", true);
  
  // Parse custom connections
  if (hasKey(node, "connections") && node["connections"].IsSequence()) {
    for (const auto& conn : node["connections"]) {
      if (conn.IsSequence() && conn.size() >= 2) {
        std::string from = conn[0].as<std::string>();
        std::string to = conn[1].as<std::string>();
        config.connections.push_back(std::make_pair(from, to));
      }
    }
  }
  
  return config;
}

EventConfig ConfigLoader::parseEvent(const YAML::Node& node) {
  EventConfig config;
  config.time = getUInt32(node, "time", 0);
  
  std::string action_str = getString(node, "action");
  config.action = stringToEventAction(action_str);
  
  config.target = getString(node, "target");
  config.description = getString(node, "description");
  config.from = getString(node, "from");
  config.to = getString(node, "to");
  config.payload = getString(node, "payload");
  config.quality = getFloat(node, "quality", 1.0f);
  config.count = getUInt32(node, "count", 0);
  config.template_name = getString(node, "template");
  config.id_prefix = getString(node, "id_prefix");
  
  // Parse targets array
  if (hasKey(node, "targets") && node["targets"].IsSequence()) {
    for (const auto& target : node["targets"]) {
      config.targets.push_back(target.as<std::string>());
    }
  }
  
  // Parse groups array
  if (hasKey(node, "groups") && node["groups"].IsSequence()) {
    for (const auto& group : node["groups"]) {
      if (group.IsSequence()) {
        std::vector<std::string> group_nodes;
        for (const auto& node_id : group) {
          group_nodes.push_back(node_id.as<std::string>());
        }
        config.groups.push_back(group_nodes);
      }
    }
  }
  
  return config;
}

MetricsConfig ConfigLoader::parseMetrics(const YAML::Node& node) {
  MetricsConfig config;
  config.output = getString(node, "output");
  config.interval = getUInt32(node, "interval", 5);
  
  // Parse collect array
  if (hasKey(node, "collect") && node["collect"].IsSequence()) {
    for (const auto& metric : node["collect"]) {
      config.collect.push_back(metric.as<std::string>());
    }
  }
  
  // Parse export array
  if (hasKey(node, "export") && node["export"].IsSequence()) {
    for (const auto& format : node["export"]) {
      config.export_formats.push_back(format.as<std::string>());
    }
  }
  
  return config;
}

void ConfigLoader::expandTemplates(ScenarioConfig& config) {
  std::cout << "[DEBUG] Expanding templates. Initial node count: " << config.nodes.size() << std::endl;
  for (const auto& tmpl : config.templates) {
    std::cout << "[DEBUG] Expanding template '" << tmpl.template_name << "' with count=" << tmpl.count << ", prefix='" << tmpl.id_prefix << "'" << std::endl;
    for (uint32_t i = 0; i < tmpl.count; ++i) {
      NodeConfigExtended node = tmpl.base_config;
      node.id = tmpl.id_prefix + std::to_string(i);
      node.nodeId = generateNodeId(node.id);
      std::cout << "[DEBUG] Created node: id='" << node.id << "', nodeId=" << node.nodeId << std::endl;
      config.nodes.push_back(node);
    }
  }
  std::cout << "[DEBUG] Template expansion complete. Final node count: " << config.nodes.size() << std::endl;
}

bool ConfigLoader::validate(const ScenarioConfig& config) {
  auto errors = getValidationErrors(config);
  
  if (!errors.empty()) {
    std::stringstream ss;
    ss << "Configuration validation failed with " << errors.size() << " error(s):";
    for (const auto& err : errors) {
      ss << "\n  - " << err.field << ": " << err.message;
      if (!err.suggestion.empty()) {
        ss << " (" << err.suggestion << ")";
      }
    }
    last_error_ = ss.str();
    return false;
  }
  
  return true;
}

std::vector<ValidationError> ConfigLoader::getValidationErrors(const ScenarioConfig& config) {
  std::vector<ValidationError> errors;
  
  validateSimulation(config.simulation, errors);
  validateNetwork(config.network, errors);
  
  // Validate nodes
  for (const auto& node : config.nodes) {
    validateNode(node, errors);
  }
  
  // Check for duplicate node IDs
  std::vector<std::string> node_ids;
  for (const auto& node : config.nodes) {
    if (std::find(node_ids.begin(), node_ids.end(), node.id) != node_ids.end()) {
      ValidationError err;
      err.field = "nodes";
      err.message = "Duplicate node ID: " + node.id;
      err.suggestion = "Ensure all node IDs are unique";
      errors.push_back(err);
    }
    node_ids.push_back(node.id);
  }
  
  // Check for at least one node
  if (config.nodes.empty()) {
    ValidationError err;
    err.field = "nodes";
    err.message = "No nodes defined";
    err.suggestion = "Add at least one node or template";
    errors.push_back(err);
  }
  
  validateTopology(config.topology, config.nodes, errors);
  
  // Validate events
  for (const auto& event : config.events) {
    validateEvent(event, config.simulation.duration, config.nodes, errors);
  }
  
  return errors;
}

void ConfigLoader::validateSimulation(const SimulationConfig& config,
                                      std::vector<ValidationError>& errors) {
  if (config.name.empty()) {
    ValidationError err;
    err.field = "simulation.name";
    err.message = "Simulation name is required";
    err.suggestion = "Add a descriptive name for your simulation";
    errors.push_back(err);
  }
  
  if (config.time_scale <= 0.0f) {
    ValidationError err;
    err.field = "simulation.time_scale";
    err.message = "Time scale must be positive";
    err.suggestion = "Use 1.0 for real-time, >1.0 for faster simulation";
    errors.push_back(err);
  }
}

void ConfigLoader::validateNetwork(const NetworkConfig& config,
                                   std::vector<ValidationError>& errors) {
  // Validate default latency
  if (!config.default_latency.isValid()) {
    ValidationError err;
    err.field = "network.latency.default";
    err.message = "Minimum latency cannot be greater than maximum";
    err.suggestion = "Set min <= max";
    errors.push_back(err);
  }
  
  // Validate specific connection latencies
  for (size_t i = 0; i < config.specific_latencies.size(); ++i) {
    const auto& conn = config.specific_latencies[i];
    
    if (conn.from.empty()) {
      ValidationError err;
      err.field = "network.latency.specific_connections[" + std::to_string(i) + "].from";
      err.message = "Source node ID cannot be empty";
      err.suggestion = "Specify a valid node ID";
      errors.push_back(err);
    }
    
    if (conn.to.empty()) {
      ValidationError err;
      err.field = "network.latency.specific_connections[" + std::to_string(i) + "].to";
      err.message = "Destination node ID cannot be empty";
      err.suggestion = "Specify a valid node ID";
      errors.push_back(err);
    }
    
    if (!conn.config.isValid()) {
      ValidationError err;
      err.field = "network.latency.specific_connections[" + std::to_string(i) + "]";
      err.message = "Minimum latency cannot be greater than maximum for connection " + 
                    conn.from + " -> " + conn.to;
      err.suggestion = "Set min <= max";
      errors.push_back(err);
    }
  }
  
  // Validate default packet loss
  if (!config.default_packet_loss.isValid()) {
    ValidationError err;
    err.field = "network.packet_loss.default";
    err.message = "Invalid packet loss configuration";
    err.suggestion = "Probability must be 0.0-1.0, burst_length must be > 0";
    errors.push_back(err);
  }
  
  // Validate specific connection packet losses
  for (size_t i = 0; i < config.specific_packet_losses.size(); ++i) {
    const auto& conn = config.specific_packet_losses[i];
    
    if (conn.from.empty()) {
      ValidationError err;
      err.field = "network.packet_loss.specific_connections[" + std::to_string(i) + "].from";
      err.message = "Source node ID cannot be empty";
      err.suggestion = "Specify a valid node ID";
      errors.push_back(err);
    }
    
    if (conn.to.empty()) {
      ValidationError err;
      err.field = "network.packet_loss.specific_connections[" + std::to_string(i) + "].to";
      err.message = "Destination node ID cannot be empty";
      err.suggestion = "Specify a valid node ID";
      errors.push_back(err);
    }
    
    if (!conn.config.isValid()) {
      ValidationError err;
      err.field = "network.packet_loss.specific_connections[" + std::to_string(i) + "]";
      err.message = "Invalid packet loss configuration for connection " + 
                    conn.from + " -> " + conn.to;
      err.suggestion = "Probability must be 0.0-1.0, burst_length must be > 0";
      errors.push_back(err);
    }
  }
  
  // Legacy packet_loss validation
  if (config.packet_loss < 0.0f || config.packet_loss > 1.0f) {
    ValidationError err;
    err.field = "network.packet_loss";
    err.message = "Packet loss must be between 0.0 and 1.0";
    err.suggestion = "Use 0.01 for 1% packet loss";
    errors.push_back(err);
  }
  
  if (config.bandwidth == 0) {
    ValidationError err;
    err.field = "network.bandwidth";
    err.message = "Bandwidth cannot be zero";
    err.suggestion = "Specify bandwidth in bits per second";
    errors.push_back(err);
  }
}

void ConfigLoader::validateNode(const NodeConfigExtended& config,
                               std::vector<ValidationError>& errors) {
  if (config.id.empty()) {
    ValidationError err;
    err.field = "node.id";
    err.message = "Node ID is required";
    err.suggestion = "Provide a unique identifier for each node";
    errors.push_back(err);
  }
  
  if (config.mesh_prefix.empty()) {
    ValidationError err;
    err.field = "node.config.mesh_prefix";
    err.message = "Mesh prefix is required for node: " + config.id;
    err.suggestion = "Set mesh_prefix in node configuration";
    errors.push_back(err);
  }
  
  if (config.mesh_password.empty()) {
    ValidationError err;
    err.field = "node.config.mesh_password";
    err.message = "Mesh password is required for node: " + config.id;
    err.suggestion = "Set mesh_password in node configuration";
    errors.push_back(err);
  }
  
  if (config.mesh_port == 0) {
    ValidationError err;
    err.field = "node.config.mesh_port";
    err.message = "Invalid mesh port for node: " + config.id;
    err.suggestion = "Use default port 5555 or specify a valid port";
    errors.push_back(err);
  }
}

void ConfigLoader::validateTopology(const TopologyConfig& config,
                                    const std::vector<NodeConfigExtended>& all_nodes,
                                    std::vector<ValidationError>& errors) {
  // Check hub exists for star topology
  if (config.type == TopologyType::STAR && config.hub.is_initialized()) {
    bool hub_found = false;
    for (const auto& node : all_nodes) {
      if (node.id == config.hub.get()) {
        hub_found = true;
        break;
      }
    }
    if (!hub_found) {
      ValidationError err;
      err.field = "topology.hub";
      err.message = "Hub node not found: " + config.hub.get();
      err.suggestion = "Ensure hub node ID matches an existing node";
      errors.push_back(err);
    }
  } else if (config.type == TopologyType::STAR && !config.hub.is_initialized()) {
    ValidationError err;
    err.field = "topology.hub";
    err.message = "Hub node required for star topology";
    err.suggestion = "Specify which node should be the central hub";
    errors.push_back(err);
  }
  
  // Validate density for random topology
  if (config.type == TopologyType::RANDOM) {
    if (config.density < 0.0f || config.density > 1.0f) {
      ValidationError err;
      err.field = "topology.density";
      err.message = "Density must be between 0.0 and 1.0";
      err.suggestion = "Use 0.3 for sparse, 0.7 for dense networks";
      errors.push_back(err);
    }
  }
  
  // Validate custom connections
  if (config.type == TopologyType::CUSTOM) {
    if (config.connections.empty()) {
      ValidationError err;
      err.field = "topology.connections";
      err.message = "Custom topology requires connection definitions";
      err.suggestion = "Add connections array with [node1, node2] pairs";
      errors.push_back(err);
    }
    
    for (const auto& conn : config.connections) {
      bool from_found = false;
      bool to_found = false;
      
      for (const auto& node : all_nodes) {
        if (node.id == conn.first) from_found = true;
        if (node.id == conn.second) to_found = true;
      }
      
      if (!from_found) {
        ValidationError err;
        err.field = "topology.connections";
        err.message = "Connection references non-existent node: " + conn.first;
        err.suggestion = "Ensure all connection nodes exist";
        errors.push_back(err);
      }
      if (!to_found) {
        ValidationError err;
        err.field = "topology.connections";
        err.message = "Connection references non-existent node: " + conn.second;
        err.suggestion = "Ensure all connection nodes exist";
        errors.push_back(err);
      }
    }
  }
}

void ConfigLoader::validateEvent(const EventConfig& config,
                                 uint32_t simulation_duration,
                                 const std::vector<NodeConfigExtended>& all_nodes,
                                 std::vector<ValidationError>& errors) {
  // Check event time is within simulation duration
  if (simulation_duration > 0 && config.time > simulation_duration) {
    ValidationError err;
    err.field = "event.time";
    err.message = "Event time " + std::to_string(config.time) + 
                  "s exceeds simulation duration " + std::to_string(simulation_duration) + "s";
    err.suggestion = "Ensure all event times are within simulation duration";
    errors.push_back(err);
  }
  
  // Validate target node exists
  if (!config.target.empty()) {
    bool target_found = false;
    for (const auto& node : all_nodes) {
      if (node.id == config.target) {
        target_found = true;
        break;
      }
    }
    if (!target_found) {
      ValidationError err;
      err.field = "event.target";
      err.message = "Event references non-existent node: " + config.target;
      err.suggestion = "Ensure target node exists";
      errors.push_back(err);
    }
  }
  
  // Validate network quality
  if (config.action == EventAction::SET_NETWORK_QUALITY) {
    if (config.quality < 0.0f || config.quality > 1.0f) {
      ValidationError err;
      err.field = "event.quality";
      err.message = "Network quality must be between 0.0 and 1.0";
      err.suggestion = "Use 0.0 for worst, 1.0 for best quality";
      errors.push_back(err);
    }
  }
}

TopologyType ConfigLoader::stringToTopologyType(const std::string& type_str) {
  std::string lower = type_str;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  
  if (lower == "random") return TopologyType::RANDOM;
  if (lower == "star") return TopologyType::STAR;
  if (lower == "ring") return TopologyType::RING;
  if (lower == "mesh") return TopologyType::MESH;
  if (lower == "custom") return TopologyType::CUSTOM;
  
  return TopologyType::RANDOM; // Default
}

EventAction ConfigLoader::stringToEventAction(const std::string& action_str) {
  std::string lower = action_str;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  
  if (lower == "stop_node") return EventAction::STOP_NODE;
  if (lower == "start_node") return EventAction::START_NODE;
  if (lower == "restart_node") return EventAction::RESTART_NODE;
  if (lower == "remove_node") return EventAction::REMOVE_NODE;
  if (lower == "add_nodes") return EventAction::ADD_NODES;
  if (lower == "partition_network") return EventAction::PARTITION_NETWORK;
  if (lower == "heal_partition") return EventAction::HEAL_PARTITION;
  if (lower == "break_link") return EventAction::BREAK_LINK;
  if (lower == "restore_link") return EventAction::RESTORE_LINK;
  if (lower == "inject_message") return EventAction::INJECT_MESSAGE;
  if (lower == "set_network_quality") return EventAction::SET_NETWORK_QUALITY;
  
  throw std::runtime_error("Unknown event action: " + action_str);
}

uint32_t ConfigLoader::generateNodeId(const std::string& id_str) {
  // Simple hash function to generate numeric ID from string
  // Using std::hash for consistency
  std::hash<std::string> hasher;
  size_t hash = hasher(id_str);
  
  // Ensure non-zero and within uint32_t range
  uint32_t node_id = static_cast<uint32_t>(hash & 0x7FFFFFFF);
  if (node_id == 0) {
    node_id = 1;
  }
  
  return node_id;
}

} // namespace simulator
