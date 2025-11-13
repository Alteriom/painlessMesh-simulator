/**
 * @file network_simulator.hpp
 * @brief Network simulation with latency and quality simulation
 * 
 * This file contains the NetworkSimulator class which simulates realistic
 * network conditions including latency, packet loss, and bandwidth limits.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef SIMULATOR_NETWORK_SIMULATOR_HPP
#define SIMULATOR_NETWORK_SIMULATOR_HPP

#include <cstdint>
#include <string>
#include <map>
#include <queue>
#include <vector>
#include <random>
#include <functional>
#include <chrono>

namespace simulator {

/**
 * @brief Distribution types for latency simulation
 */
enum class DistributionType {
  UNIFORM,      ///< Uniform distribution (equal probability)
  NORMAL,       ///< Normal (Gaussian) distribution
  EXPONENTIAL   ///< Exponential distribution
};

/**
 * @brief Latency configuration for network connections
 */
struct LatencyConfig {
  uint32_t min_ms = 0;                  ///< Minimum latency in milliseconds
  uint32_t max_ms = 100;                ///< Maximum latency in milliseconds
  DistributionType distribution = DistributionType::UNIFORM;  ///< Distribution type
  
  /**
   * @brief Validates the configuration
   * @return true if valid, false otherwise
   */
  bool isValid() const {
    return min_ms <= max_ms;
  }
};

/**
 * @brief A delayed message in the network simulator
 */
struct DelayedMessage {
  uint32_t from;                        ///< Source node ID
  uint32_t to;                          ///< Destination node ID
  std::string message;                  ///< Message content
  uint64_t deliveryTime;                ///< Delivery time in milliseconds
  
  /**
   * @brief Comparison operator for priority queue (earlier delivery first)
   */
  bool operator>(const DelayedMessage& other) const {
    return deliveryTime > other.deliveryTime;
  }
};

/**
 * @brief Network simulator for realistic mesh network conditions
 * 
 * The NetworkSimulator class provides realistic network simulation including:
 * - Configurable latency with multiple distribution types
 * - Per-connection latency configuration
 * - Message delay queue with priority-based delivery
 * - Latency metrics collection
 * 
 * Example usage:
 * @code
 * NetworkSimulator sim;
 * 
 * // Set default latency
 * LatencyConfig default_latency;
 * default_latency.min_ms = 10;
 * default_latency.max_ms = 50;
 * default_latency.distribution = DistributionType::NORMAL;
 * sim.setDefaultLatency(default_latency);
 * 
 * // Set specific connection latency
 * LatencyConfig high_latency;
 * high_latency.min_ms = 100;
 * high_latency.max_ms = 200;
 * sim.setLatency(1, 2, high_latency);
 * 
 * // Enqueue a message
 * sim.enqueueMessage(1, 2, "Hello", getCurrentTimeMs());
 * 
 * // Process ready messages
 * auto ready = sim.getReadyMessages(getCurrentTimeMs());
 * @endcode
 */
class NetworkSimulator {
public:
  /**
   * @brief Default constructor
   */
  NetworkSimulator();
  
  /**
   * @brief Constructor with seed for deterministic testing
   * 
   * @param seed Random seed for reproducible simulations
   */
  explicit NetworkSimulator(uint32_t seed);
  
  /**
   * @brief Sets the default latency configuration
   * 
   * @param config Default latency configuration
   * @throws std::invalid_argument if config is invalid
   */
  void setDefaultLatency(const LatencyConfig& config);
  
  /**
   * @brief Sets latency for a specific connection
   * 
   * @param fromNode Source node ID
   * @param toNode Destination node ID
   * @param config Latency configuration for this connection
   * @throws std::invalid_argument if config is invalid
   */
  void setLatency(uint32_t fromNode, uint32_t toNode, const LatencyConfig& config);
  
  /**
   * @brief Gets latency configuration for a connection
   * 
   * @param fromNode Source node ID
   * @param toNode Destination node ID
   * @return Latency configuration (specific or default)
   */
  LatencyConfig getLatency(uint32_t fromNode, uint32_t toNode) const;
  
  /**
   * @brief Enqueues a message with simulated latency
   * 
   * @param from Source node ID
   * @param to Destination node ID
   * @param message Message content
   * @param currentTime Current simulation time in milliseconds
   */
  void enqueueMessage(uint32_t from, uint32_t to, const std::string& message, uint64_t currentTime);
  
  /**
   * @brief Gets all messages ready for delivery at current time
   * 
   * @param currentTime Current simulation time in milliseconds
   * @return Vector of messages ready for delivery
   */
  std::vector<DelayedMessage> getReadyMessages(uint64_t currentTime);
  
  /**
   * @brief Gets the number of messages currently in the queue
   * 
   * @return Number of pending messages
   */
  size_t getPendingMessageCount() const;
  
  /**
   * @brief Clears all pending messages
   */
  void clear();
  
  /**
   * @brief Gets statistics for a specific connection
   * 
   * @param fromNode Source node ID
   * @param toNode Destination node ID
   * @return Latency statistics (min, max, avg)
   */
  struct LatencyStats {
    uint32_t min_latency_ms = 0;       ///< Minimum observed latency
    uint32_t max_latency_ms = 0;       ///< Maximum observed latency
    uint32_t avg_latency_ms = 0;       ///< Average latency
    uint64_t message_count = 0;        ///< Number of messages
  };
  
  /**
   * @brief Gets latency statistics for a connection
   * 
   * @param fromNode Source node ID
   * @param toNode Destination node ID
   * @return Latency statistics
   */
  LatencyStats getStats(uint32_t fromNode, uint32_t toNode) const;
  
  /**
   * @brief Resets all statistics
   */
  void resetStats();

private:
  using ConnectionKey = std::pair<uint32_t, uint32_t>;
  
  LatencyConfig default_latency_;                           ///< Default latency configuration
  std::map<ConnectionKey, LatencyConfig> latency_map_;      ///< Per-connection latency config
  std::priority_queue<DelayedMessage, 
                      std::vector<DelayedMessage>,
                      std::greater<DelayedMessage>> message_queue_;  ///< Message delay queue
  
  // Statistics tracking
  struct ConnectionStats {
    uint64_t total_latency_ms = 0;
    uint32_t min_latency_ms = UINT32_MAX;
    uint32_t max_latency_ms = 0;
    uint64_t message_count = 0;
  };
  std::map<ConnectionKey, ConnectionStats> stats_map_;      ///< Connection statistics
  
  // Random number generation
  std::mt19937 rng_;                                        ///< Random number generator
  
  /**
   * @brief Calculates latency for a message based on configuration
   * 
   * @param config Latency configuration
   * @return Calculated latency in milliseconds
   */
  uint32_t calculateLatency(const LatencyConfig& config);
  
  /**
   * @brief Generates a random value using uniform distribution
   * 
   * @param min Minimum value
   * @param max Maximum value
   * @return Random value in range [min, max]
   */
  uint32_t uniformDistribution(uint32_t min, uint32_t max);
  
  /**
   * @brief Generates a random value using normal distribution
   * 
   * @param min Minimum value
   * @param max Maximum value
   * @return Random value, clamped to [min, max]
   */
  uint32_t normalDistribution(uint32_t min, uint32_t max);
  
  /**
   * @brief Generates a random value using exponential distribution
   * 
   * @param min Minimum value
   * @param max Maximum value
   * @return Random value, clamped to [min, max]
   */
  uint32_t exponentialDistribution(uint32_t min, uint32_t max);
  
  /**
   * @brief Records latency statistics for a connection
   * 
   * @param from Source node ID
   * @param to Destination node ID
   * @param latency_ms Latency in milliseconds
   */
  void recordStats(uint32_t from, uint32_t to, uint32_t latency_ms);
};

/**
 * @brief Converts distribution type to string
 * 
 * @param type Distribution type
 * @return String representation
 */
std::string distributionTypeToString(DistributionType type);

/**
 * @brief Converts string to distribution type
 * 
 * @param str String representation
 * @return Distribution type
 * @throws std::invalid_argument if string is not recognized
 */
DistributionType stringToDistributionType(const std::string& str);

} // namespace simulator

#endif // SIMULATOR_NETWORK_SIMULATOR_HPP
