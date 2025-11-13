/**
 * @file config_loader.hpp
 * @brief Configuration loader for simulation scenarios
 * 
 * This file contains the ConfigLoader class which loads and validates
 * YAML configuration files for painlessMesh simulation scenarios.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef SIMULATOR_CONFIG_LOADER_HPP
#define SIMULATOR_CONFIG_LOADER_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <map>
#include <boost/optional.hpp>
#include "simulator/network_simulator.hpp"

namespace YAML {
  class Node;
}

namespace simulator {

/**
 * @brief Network topology types
 */
enum class TopologyType {
  RANDOM,    ///< Random connections based on density
  STAR,      ///< Star topology with central hub
  RING,      ///< Ring topology with bidirectional links
  MESH,      ///< Full mesh (all nodes connected)
  CUSTOM     ///< Custom connections defined explicitly
};

/**
 * @brief Specific connection latency override
 */
struct ConnectionLatencyConfig {
  std::string from;                      ///< Source node ID
  std::string to;                        ///< Destination node ID
  LatencyConfig config;                  ///< Latency configuration
};

/**
 * @brief Specific connection packet loss override
 */
struct ConnectionPacketLossConfig {
  std::string from;                      ///< Source node ID
  std::string to;                        ///< Destination node ID
  PacketLossConfig config;               ///< Packet loss configuration
};

/**
 * @brief Specific connection bandwidth override
 */
struct ConnectionBandwidthConfig {
  std::string from;                      ///< Source node ID
  std::string to;                        ///< Destination node ID
  BandwidthConfig config;                ///< Bandwidth configuration
};

/**
 * @brief Network quality configuration
 */
struct NetworkConfig {
  LatencyConfig default_latency;                           ///< Default latency settings
  std::vector<ConnectionLatencyConfig> specific_latencies; ///< Per-connection latency overrides
  PacketLossConfig default_packet_loss;                    ///< Default packet loss settings
  std::vector<ConnectionPacketLossConfig> specific_packet_losses; ///< Per-connection packet loss overrides
  BandwidthConfig default_bandwidth;                       ///< Default bandwidth settings
  std::vector<ConnectionBandwidthConfig> specific_bandwidths; ///< Per-connection bandwidth overrides
  float packet_loss = 0.0f;                                ///< Legacy packet loss rate (0.0-1.0)
  uint64_t bandwidth = 1000000;                            ///< Legacy bandwidth in bits per second
};

/**
 * @brief Simulation parameters
 */
struct SimulationConfig {
  std::string name;                      ///< Simulation name
  std::string description;               ///< Optional description
  uint32_t duration = 0;                 ///< Duration in seconds (0 = infinite)
  float time_scale = 1.0f;               ///< Time scale multiplier (1.0 = real-time)
  uint32_t seed = 0;                     ///< Random seed (0 = random)
};

/**
 * @brief Node configuration (extends NodeConfig from virtual_node.hpp)
 */
struct NodeConfigExtended {
  std::string id;                        ///< Node identifier (string)
  uint32_t nodeId = 0;                   ///< Numeric node ID
  std::string type;                      ///< Node type (sensor, bridge, etc.)
  std::string firmware;                  ///< Firmware path
  std::vector<int> position;             ///< Position [x, y] for visualization
  
  // Core mesh configuration
  std::string mesh_prefix;               ///< Mesh network SSID prefix
  std::string mesh_password;             ///< Mesh network password
  uint16_t mesh_port = 5555;             ///< Mesh network port
  
  // Extended configuration (firmware-specific)
  boost::optional<uint32_t> sensor_interval;      ///< Sensor reading interval (ms)
  boost::optional<std::string> mqtt_broker;       ///< MQTT broker address
  boost::optional<uint16_t> mqtt_port;            ///< MQTT broker port
  boost::optional<std::string> mqtt_topic_prefix; ///< MQTT topic prefix
};

/**
 * @brief Node template for batch generation
 */
struct NodeTemplate {
  std::string template_name;             ///< Template identifier
  uint32_t count = 1;                    ///< Number of nodes to generate
  std::string id_prefix;                 ///< ID prefix for generated nodes
  NodeConfigExtended base_config;        ///< Base configuration for template
};

/**
 * @brief Network topology configuration
 */
struct TopologyConfig {
  TopologyType type = TopologyType::RANDOM;  ///< Topology type
  boost::optional<std::string> hub;            ///< Hub node ID (for star topology)
  float density = 0.3f;                      ///< Connection density (for random)
  bool bidirectional = true;                 ///< Bidirectional links (for ring)
  std::vector<std::pair<std::string, std::string>> connections;  ///< Custom connections
};

/**
 * @brief Event action types
 */
enum class EventAction {
  STOP_NODE,           ///< Stop a node
  START_NODE,          ///< Start a node
  RESTART_NODE,        ///< Restart a node
  CRASH_NODE,          ///< Crash a node (ungraceful stop)
  REMOVE_NODE,         ///< Remove a node
  ADD_NODES,           ///< Add new nodes
  PARTITION_NETWORK,   ///< Partition network into groups
  HEAL_PARTITION,      ///< Heal network partition
  BREAK_LINK,          ///< Break link between nodes
  RESTORE_LINK,        ///< Restore link between nodes
  INJECT_MESSAGE,      ///< Inject a message
  SET_NETWORK_QUALITY, ///< Change network quality
  CONNECTION_DROP,     ///< Drop connection between nodes
  CONNECTION_RESTORE,  ///< Restore dropped connection
  CONNECTION_DEGRADE   ///< Degrade connection quality
};

/**
 * @brief Scheduled event configuration
 */
struct EventConfig {
  uint32_t time = 0;                     ///< Event time in seconds
  EventAction action;                    ///< Event action type
  std::string target;                    ///< Target node ID
  std::vector<std::string> targets;      ///< Multiple target node IDs
  std::string description;               ///< Event description
  
  // Action-specific parameters
  std::vector<std::vector<std::string>> groups;  ///< Network partition groups
  std::string from;                      ///< Message source
  std::string to;                        ///< Message destination
  std::string payload;                   ///< Message payload
  float quality = 1.0f;                  ///< Network quality (0.0-1.0)
  uint32_t count = 0;                    ///< Node count (for add_nodes)
  std::string template_name;             ///< Template name (for add_nodes)
  std::string id_prefix;                 ///< ID prefix (for add_nodes)
  bool graceful = true;                  ///< Graceful shutdown (for stop_node)
  uint32_t delay = 0;                    ///< Delay in seconds (for restart_node)
  uint32_t latency = 500;                ///< Latency in ms (for connection_degrade)
  float packet_loss = 0.30f;             ///< Packet loss 0.0-1.0 (for connection_degrade)
};

/**
 * @brief Metrics collection configuration
 */
struct MetricsConfig {
  std::string output;                    ///< Output file path
  uint32_t interval = 5;                 ///< Collection interval in seconds
  std::vector<std::string> collect;      ///< Metrics to collect
  std::vector<std::string> export_formats;  ///< Export formats (csv, json, graphviz)
};

/**
 * @brief Complete scenario configuration
 */
struct ScenarioConfig {
  SimulationConfig simulation;           ///< Simulation parameters
  NetworkConfig network;                 ///< Network configuration
  std::vector<NodeConfigExtended> nodes; ///< Node configurations
  std::vector<NodeTemplate> templates;   ///< Node templates
  TopologyConfig topology;               ///< Topology configuration
  std::vector<EventConfig> events;       ///< Scheduled events
  MetricsConfig metrics;                 ///< Metrics configuration
};

/**
 * @brief Validation error information
 */
struct ValidationError {
  std::string field;                     ///< Field with error
  std::string message;                   ///< Error message
  std::string suggestion;                ///< Suggested fix (optional)
};

/**
 * @brief Configuration loader and validator
 * 
 * The ConfigLoader class provides functionality to load YAML configuration
 * files, parse them into structured data, and validate their correctness.
 * 
 * Example usage:
 * @code
 * ConfigLoader loader;
 * auto config = loader.loadFromFile("scenario.yaml");
 * if (config) {
 *   // Use configuration
 * } else {
 *   auto errors = loader.getValidationErrors(*config);
 *   for (const auto& err : errors) {
 *     std::cerr << err.field << ": " << err.message << std::endl;
 *   }
 * }
 * @endcode
 */
class ConfigLoader {
public:
  /**
   * @brief Default constructor
   */
  ConfigLoader() = default;
  
  /**
   * @brief Loads configuration from YAML file
   * 
   * @param filepath Path to YAML configuration file
   * @return ScenarioConfig if successful, empty optional on error
   * 
   * @throws std::runtime_error if file cannot be opened or parsed
   */
  boost::optional<ScenarioConfig> loadFromFile(const std::string& filepath);
  
  /**
   * @brief Loads configuration from YAML string
   * 
   * @param yaml_content YAML configuration as string
   * @return ScenarioConfig if successful, empty optional on error
   * 
   * @throws std::runtime_error if YAML cannot be parsed
   */
  boost::optional<ScenarioConfig> loadFromString(const std::string& yaml_content);
  
  /**
   * @brief Validates a configuration
   * 
   * @param config Configuration to validate
   * @return true if configuration is valid, false otherwise
   */
  bool validate(const ScenarioConfig& config);
  
  /**
   * @brief Gets validation errors for a configuration
   * 
   * @param config Configuration to validate
   * @return Vector of validation errors (empty if valid)
   */
  std::vector<ValidationError> getValidationErrors(const ScenarioConfig& config);
  
  /**
   * @brief Expands node templates into concrete nodes
   * 
   * @param config Configuration with templates to expand
   * 
   * This modifies the config in-place, expanding all templates
   * into individual node configurations.
   */
  void expandTemplates(ScenarioConfig& config);
  
  /**
   * @brief Gets the last error message
   * 
   * @return Error message from last failed operation
   */
  std::string getLastError() const { return last_error_; }
  
  /**
   * @brief Generates unique node ID from string
   * 
   * @param id_str String identifier
   * @return uint32_t node ID
   */
  uint32_t generateNodeId(const std::string& id_str);

private:
  std::string last_error_;               ///< Last error message
  
  /**
   * @brief Parses simulation configuration
   * 
   * @param node YAML node
   * @return SimulationConfig
   */
  SimulationConfig parseSimulation(const YAML::Node& node);
  
  /**
   * @brief Parses network configuration
   * 
   * @param node YAML node
   * @return NetworkConfig
   */
  NetworkConfig parseNetwork(const YAML::Node& node);
  
  /**
   * @brief Parses node configuration
   * 
   * @param node YAML node
   * @return NodeConfigExtended
   */
  NodeConfigExtended parseNode(const YAML::Node& node);
  
  /**
   * @brief Parses node template
   * 
   * @param node YAML node
   * @return NodeTemplate
   */
  NodeTemplate parseTemplate(const YAML::Node& node);
  
  /**
   * @brief Parses topology configuration
   * 
   * @param node YAML node
   * @return TopologyConfig
   */
  TopologyConfig parseTopology(const YAML::Node& node);
  
  /**
   * @brief Parses event configuration
   * 
   * @param node YAML node
   * @return EventConfig
   */
  EventConfig parseEvent(const YAML::Node& node);
  
  /**
   * @brief Parses metrics configuration
   * 
   * @param node YAML node
   * @return MetricsConfig
   */
  MetricsConfig parseMetrics(const YAML::Node& node);
  
  /**
   * @brief Validates simulation configuration
   * 
   * @param config Simulation config
   * @param errors Vector to append errors to
   */
  void validateSimulation(const SimulationConfig& config, 
                         std::vector<ValidationError>& errors);
  
  /**
   * @brief Validates network configuration
   * 
   * @param config Network config
   * @param errors Vector to append errors to
   */
  void validateNetwork(const NetworkConfig& config,
                      std::vector<ValidationError>& errors);
  
  /**
   * @brief Validates node configuration
   * 
   * @param config Node config
   * @param errors Vector to append errors to
   */
  void validateNode(const NodeConfigExtended& config,
                   std::vector<ValidationError>& errors);
  
  /**
   * @brief Validates topology configuration
   * 
   * @param config Topology config
   * @param all_nodes All node configurations
   * @param errors Vector to append errors to
   */
  void validateTopology(const TopologyConfig& config,
                       const std::vector<NodeConfigExtended>& all_nodes,
                       std::vector<ValidationError>& errors);
  
  /**
   * @brief Validates event configuration
   * 
   * @param config Event config
   * @param simulation_duration Simulation duration
   * @param all_nodes All node configurations
   * @param errors Vector to append errors to
   */
  void validateEvent(const EventConfig& config,
                    uint32_t simulation_duration,
                    const std::vector<NodeConfigExtended>& all_nodes,
                    std::vector<ValidationError>& errors);
  
  /**
   * @brief Converts string to TopologyType
   * 
   * @param type_str String representation
   * @return TopologyType
   */
  TopologyType stringToTopologyType(const std::string& type_str);
  
  /**
   * @brief Converts string to EventAction
   * 
   * @param action_str String representation
   * @return EventAction
   */
  EventAction stringToEventAction(const std::string& action_str);
};

} // namespace simulator

#endif // SIMULATOR_CONFIG_LOADER_HPP
