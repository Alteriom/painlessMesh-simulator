/**
 * @file network_simulator.cpp
 * @brief Implementation of NetworkSimulator class
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include "simulator/platform_compat.hpp"
#include "simulator/network_simulator.hpp"

#include <algorithm>
#include <stdexcept>
#include <cmath>

namespace simulator {

NetworkSimulator::NetworkSimulator() 
    : rng_(std::random_device{}()) {
  // Set default latency
  default_latency_.min_ms = 10;
  default_latency_.max_ms = 50;
  default_latency_.distribution = DistributionType::NORMAL;
}

NetworkSimulator::NetworkSimulator(uint32_t seed)
    : rng_(seed) {
  // Set default latency
  default_latency_.min_ms = 10;
  default_latency_.max_ms = 50;
  default_latency_.distribution = DistributionType::NORMAL;
}

void NetworkSimulator::setDefaultLatency(const LatencyConfig& config) {
  if (!config.isValid()) {
    throw std::invalid_argument("Invalid latency configuration: min > max");
  }
  default_latency_ = config;
}

void NetworkSimulator::setLatency(uint32_t fromNode, uint32_t toNode, const LatencyConfig& config) {
  if (!config.isValid()) {
    throw std::invalid_argument("Invalid latency configuration: min > max");
  }
  ConnectionKey key = std::make_pair(fromNode, toNode);
  latency_map_[key] = config;
}

LatencyConfig NetworkSimulator::getLatency(uint32_t fromNode, uint32_t toNode) const {
  ConnectionKey key = std::make_pair(fromNode, toNode);
  auto it = latency_map_.find(key);
  if (it != latency_map_.end()) {
    return it->second;
  }
  return default_latency_;
}

void NetworkSimulator::enqueueMessage(uint32_t from, uint32_t to, 
                                       const std::string& message, 
                                       uint64_t currentTime) {
  // Get latency configuration for this connection
  LatencyConfig config = getLatency(from, to);
  
  // Calculate latency
  uint32_t latency_ms = calculateLatency(config);
  
  // Record statistics
  recordStats(from, to, latency_ms);
  
  // Create delayed message
  DelayedMessage delayed;
  delayed.from = from;
  delayed.to = to;
  delayed.message = message;
  delayed.deliveryTime = currentTime + latency_ms;
  
  // Add to queue
  message_queue_.push(delayed);
}

std::vector<DelayedMessage> NetworkSimulator::getReadyMessages(uint64_t currentTime) {
  std::vector<DelayedMessage> ready;
  
  // Extract all messages ready for delivery
  while (!message_queue_.empty() && message_queue_.top().deliveryTime <= currentTime) {
    ready.push_back(message_queue_.top());
    message_queue_.pop();
  }
  
  return ready;
}

size_t NetworkSimulator::getPendingMessageCount() const {
  return message_queue_.size();
}

void NetworkSimulator::clear() {
  // Clear the queue by creating a new empty one
  std::priority_queue<DelayedMessage, 
                      std::vector<DelayedMessage>,
                      std::greater<DelayedMessage>> empty;
  std::swap(message_queue_, empty);
}

NetworkSimulator::LatencyStats NetworkSimulator::getStats(uint32_t fromNode, uint32_t toNode) const {
  ConnectionKey key = std::make_pair(fromNode, toNode);
  auto it = stats_map_.find(key);
  
  LatencyStats stats;
  if (it != stats_map_.end()) {
    const ConnectionStats& conn_stats = it->second;
    stats.min_latency_ms = conn_stats.min_latency_ms;
    stats.max_latency_ms = conn_stats.max_latency_ms;
    stats.message_count = conn_stats.message_count;
    if (conn_stats.message_count > 0) {
      stats.avg_latency_ms = static_cast<uint32_t>(
        conn_stats.total_latency_ms / conn_stats.message_count
      );
    }
  }
  
  return stats;
}

void NetworkSimulator::resetStats() {
  stats_map_.clear();
}

uint32_t NetworkSimulator::calculateLatency(const LatencyConfig& config) {
  switch (config.distribution) {
    case DistributionType::UNIFORM:
      return uniformDistribution(config.min_ms, config.max_ms);
    
    case DistributionType::NORMAL:
      return normalDistribution(config.min_ms, config.max_ms);
    
    case DistributionType::EXPONENTIAL:
      return exponentialDistribution(config.min_ms, config.max_ms);
    
    default:
      return uniformDistribution(config.min_ms, config.max_ms);
  }
}

uint32_t NetworkSimulator::uniformDistribution(uint32_t min, uint32_t max) {
  if (min == max) {
    return min;
  }
  std::uniform_int_distribution<uint32_t> dist(min, max);
  return dist(rng_);
}

uint32_t NetworkSimulator::normalDistribution(uint32_t min, uint32_t max) {
  if (min == max) {
    return min;
  }
  
  // Use mean and standard deviation
  double mean = (min + max) / 2.0;
  double stddev = (max - min) / 6.0;  // 99.7% of values within [min, max]
  
  std::normal_distribution<double> dist(mean, stddev);
  double value = dist(rng_);
  
  // Clamp to [min, max] and round
  value = std::max(static_cast<double>(min), value);
  value = std::min(static_cast<double>(max), value);
  
  uint32_t result = static_cast<uint32_t>(std::round(value));
  
  // Ensure we stay within bounds after rounding
  result = std::max(min, result);
  result = std::min(max, result);
  
  return result;
}

uint32_t NetworkSimulator::exponentialDistribution(uint32_t min, uint32_t max) {
  if (min == max) {
    return min;
  }
  
  // Use lambda parameter to shape the distribution
  // lambda = 1 / (mean - min)
  // We want the mean to be around 1/3 between min and max
  double range = max - min;
  double lambda = 3.0 / range;
  
  std::exponential_distribution<double> dist(lambda);
  double value = dist(rng_) + min;
  
  // Clamp to [min, max] and round
  value = std::max(static_cast<double>(min), value);
  value = std::min(static_cast<double>(max), value);
  
  uint32_t result = static_cast<uint32_t>(std::round(value));
  
  // Ensure we stay within bounds after rounding
  result = std::max(min, result);
  result = std::min(max, result);
  
  return result;
}

void NetworkSimulator::recordStats(uint32_t from, uint32_t to, uint32_t latency_ms) {
  ConnectionKey key = std::make_pair(from, to);
  ConnectionStats& stats = stats_map_[key];
  
  stats.total_latency_ms += latency_ms;
  stats.message_count++;
  stats.min_latency_ms = std::min(stats.min_latency_ms, latency_ms);
  stats.max_latency_ms = std::max(stats.max_latency_ms, latency_ms);
}

std::string distributionTypeToString(DistributionType type) {
  switch (type) {
    case DistributionType::UNIFORM:
      return "uniform";
    case DistributionType::NORMAL:
      return "normal";
    case DistributionType::EXPONENTIAL:
      return "exponential";
    default:
      return "unknown";
  }
}

DistributionType stringToDistributionType(const std::string& str) {
  std::string lower = str;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
  
  if (lower == "uniform") {
    return DistributionType::UNIFORM;
  } else if (lower == "normal" || lower == "gaussian") {
    return DistributionType::NORMAL;
  } else if (lower == "exponential") {
    return DistributionType::EXPONENTIAL;
  } else {
    throw std::invalid_argument("Unknown distribution type: " + str);
  }
}

} // namespace simulator
