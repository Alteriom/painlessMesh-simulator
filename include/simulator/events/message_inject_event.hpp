/**
 * @file message_inject_event.hpp
 * @brief Event for injecting a message from a scenario timeline
 *
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef SIMULATOR_MESSAGE_INJECT_EVENT_HPP
#define SIMULATOR_MESSAGE_INJECT_EVENT_HPP

#include "simulator/event.hpp"
#include <cstdint>
#include <string>

namespace simulator {

/**
 * @brief Event that sends a message from one node, optionally to a specific peer
 *
 * The config loader has always parsed `inject_message` timeline entries, but no
 * event class implemented them, so every scenario that used one refused to run.
 * Injection is the cheapest way to ask the question a partition scenario exists
 * to ask -- does a message cross the split, and does it cross again after the
 * heal -- without depending on a firmware's own send cadence.
 *
 * Example usage:
 * @code
 * // Broadcast "ping" from node 1001 at t=30 seconds
 * auto event = std::make_unique<MessageInjectEvent>(1001, 0, "ping");
 * scheduler.scheduleEvent(std::move(event), 30);
 * @endcode
 */
class MessageInjectEvent : public Event {
public:
  /**
   * @brief Construct a message injection event
   *
   * @param fromNode Node that sends the message
   * @param toNode Destination node ID, or 0 to broadcast to the whole mesh
   * @param payload Message body
   */
  MessageInjectEvent(uint32_t fromNode, uint32_t toNode, const std::string& payload);

  /**
   * @brief Send the message
   *
   * @param manager Node manager used to resolve the sender
   * @param network Network simulator (not used)
   *
   * @throws std::runtime_error if the sending node does not exist
   */
  void execute(NodeManager& manager, NetworkSimulator& network) override;

  /**
   * @brief Get event description
   *
   * @return Description string
   */
  std::string getDescription() const override;

  /// @return Sending node ID
  uint32_t getFromNode() const { return fromNode_; }

  /// @return Destination node ID, 0 for broadcast
  uint32_t getToNode() const { return toNode_; }

  /// @return Message payload
  const std::string& getPayload() const { return payload_; }

private:
  uint32_t fromNode_;    ///< Sending node ID
  uint32_t toNode_;      ///< Destination node ID (0 = broadcast)
  std::string payload_;  ///< Message body
};

} // namespace simulator

#endif // SIMULATOR_MESSAGE_INJECT_EVENT_HPP
