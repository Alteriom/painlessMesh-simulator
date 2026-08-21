/**
 * @file test_event_factory.cpp
 * @brief Unit tests for EventFactory -- the config-to-Event translation layer
 *
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include <catch2/catch_test_macros.hpp>
#include "simulator/config_loader.hpp"
#include "simulator/event_factory.hpp"
#include "simulator/event_scheduler.hpp"
#include "simulator/events/message_inject_event.hpp"
#include <map>
#include <string>
#include <vector>

using namespace simulator;

namespace {

std::map<std::string, uint32_t> testNodes() {
  return {{"node-1", 1001}, {"node-2", 1002}, {"node-3", 1003}};
}

EventConfig makeEvent(EventAction action, uint32_t time = 10) {
  EventConfig c;
  c.action = action;
  c.time = time;
  return c;
}

}  // namespace

TEST_CASE("EventFactory builds node lifecycle events", "[event_factory]") {
  const auto nodes = testNodes();

  SECTION("start_node resolves its target") {
    auto c = makeEvent(EventAction::START_NODE);
    c.target = "node-2";
    REQUIRE(EventFactory::create(c, nodes) != nullptr);
  }

  SECTION("crash_node resolves its target") {
    auto c = makeEvent(EventAction::CRASH_NODE);
    c.target = "node-1";
    REQUIRE(EventFactory::create(c, nodes) != nullptr);
  }

  SECTION("targets[] is accepted in place of target") {
    auto c = makeEvent(EventAction::STOP_NODE);
    c.targets = {"node-3"};
    REQUIRE(EventFactory::create(c, nodes) != nullptr);
  }

  SECTION("an unknown node id is rejected, not silently dropped") {
    auto c = makeEvent(EventAction::RESTART_NODE);
    c.target = "does-not-exist";
    REQUIRE_THROWS(EventFactory::create(c, nodes));
  }

  SECTION("a missing target is rejected") {
    auto c = makeEvent(EventAction::START_NODE);
    REQUIRE_THROWS(EventFactory::create(c, nodes));
  }
}

TEST_CASE("EventFactory builds connection events", "[event_factory]") {
  const auto nodes = testNodes();

  SECTION("two targets form a link") {
    auto c = makeEvent(EventAction::CONNECTION_DROP);
    c.targets = {"node-1", "node-2"};
    REQUIRE(EventFactory::create(c, nodes) != nullptr);
  }

  SECTION("from/to also form a link") {
    auto c = makeEvent(EventAction::CONNECTION_RESTORE);
    c.from = "node-1";
    c.to = "node-3";
    REQUIRE(EventFactory::create(c, nodes) != nullptr);
  }

  SECTION("degrade carries its latency and loss") {
    auto c = makeEvent(EventAction::CONNECTION_DEGRADE);
    c.targets = {"node-1", "node-2"};
    c.latency = 750;
    c.packet_loss = 0.4f;
    REQUIRE(EventFactory::create(c, nodes) != nullptr);
  }

  SECTION("a single endpoint is not a link") {
    auto c = makeEvent(EventAction::CONNECTION_DROP);
    c.targets = {"node-1"};
    REQUIRE_THROWS(EventFactory::create(c, nodes));
  }
}

TEST_CASE("EventFactory builds partition events", "[event_factory]") {
  const auto nodes = testNodes();

  SECTION("two groups partition the mesh") {
    auto c = makeEvent(EventAction::PARTITION_NETWORK);
    c.groups = {{"node-1"}, {"node-2", "node-3"}};
    REQUIRE(EventFactory::create(c, nodes) != nullptr);
  }

  SECTION("a single group is not a partition") {
    auto c = makeEvent(EventAction::PARTITION_NETWORK);
    c.groups = {{"node-1", "node-2", "node-3"}};
    REQUIRE_THROWS(EventFactory::create(c, nodes));
  }

  SECTION("heal needs no targets") {
    REQUIRE(EventFactory::create(makeEvent(EventAction::HEAL_PARTITION), nodes) != nullptr);
  }
}

TEST_CASE("EventFactory reports actions it cannot build", "[event_factory]") {
  const auto nodes = testNodes();
  // Parsed and validated by the config loader, but with no runtime event class.
  REQUIRE(EventFactory::create(makeEvent(EventAction::ADD_NODES), nodes) == nullptr);
  REQUIRE(EventFactory::create(makeEvent(EventAction::BREAK_LINK), nodes) == nullptr);
  REQUIRE(EventFactory::create(makeEvent(EventAction::SET_NETWORK_QUALITY), nodes) ==
          nullptr);
}

TEST_CASE("EventFactory builds message injections", "[event_factory]") {
  const auto nodes = testNodes();

  SECTION("from/to resolve to a directed send") {
    auto c = makeEvent(EventAction::INJECT_MESSAGE);
    c.from = "node-1";
    c.to = "node-2";
    c.payload = "ping";
    auto event = EventFactory::create(c, nodes);
    REQUIRE(event != nullptr);
    auto* inject = dynamic_cast<MessageInjectEvent*>(event.get());
    REQUIRE(inject != nullptr);
    CHECK(inject->getFromNode() == nodes.at("node-1"));
    CHECK(inject->getToNode() == nodes.at("node-2"));
    CHECK(inject->getPayload() == "ping");
  }

  SECTION("the broadcast sentinel resolves to node 0") {
    for (const std::string sentinel : {"broadcast", "all"}) {
      auto c = makeEvent(EventAction::INJECT_MESSAGE);
      c.from = "node-1";
      c.to = sentinel;
      auto event = EventFactory::create(c, nodes);
      REQUIRE(event != nullptr);
      CHECK(dynamic_cast<MessageInjectEvent*>(event.get())->getToNode() == 0);
    }
  }

  SECTION("a missing destination means broadcast") {
    auto c = makeEvent(EventAction::INJECT_MESSAGE);
    c.from = "node-1";
    auto event = EventFactory::create(c, nodes);
    REQUIRE(event != nullptr);
    auto* inject = dynamic_cast<MessageInjectEvent*>(event.get());
    REQUIRE(inject != nullptr);
    CHECK(inject->getToNode() == 0);
  }

  SECTION("target is accepted as a sender alias") {
    auto c = makeEvent(EventAction::INJECT_MESSAGE);
    c.target = "node-2";
    auto event = EventFactory::create(c, nodes);
    REQUIRE(event != nullptr);
    CHECK(dynamic_cast<MessageInjectEvent*>(event.get())->getFromNode() ==
          nodes.at("node-2"));
  }

  SECTION("no sender at all is an error, not a silent drop") {
    REQUIRE_THROWS(EventFactory::create(makeEvent(EventAction::INJECT_MESSAGE), nodes));
  }

  SECTION("an unknown sender is an error") {
    auto c = makeEvent(EventAction::INJECT_MESSAGE);
    c.from = "node-nope";
    REQUIRE_THROWS(EventFactory::create(c, nodes));
  }
}

TEST_CASE("EventFactory models a delayed restart as stop then start",
          "[event_factory]") {
  // restart_node with delay > 0 must simulate the downtime: a stop at t and a
  // start at t + delay, not an atomic restart that ignores delay.
  const auto nodes = testNodes();
  EventScheduler scheduler;
  std::vector<std::string> skipped;

  auto restart = makeEvent(EventAction::RESTART_NODE, 10);
  restart.target = "node-1";
  restart.delay = 8;

  const size_t scheduled = EventFactory::scheduleAll(
      {restart}, nodes, scheduler, skipped);

  REQUIRE(scheduled == 2);                       // stop + start
  REQUIRE(skipped.empty());
  REQUIRE(scheduler.getPendingEventCount() == 2);
  REQUIRE(scheduler.getNextEventTime() == 10);   // the stop fires first
}

TEST_CASE("EventFactory keeps an undelayed restart atomic", "[event_factory]") {
  const auto nodes = testNodes();
  EventScheduler scheduler;
  std::vector<std::string> skipped;

  auto restart = makeEvent(EventAction::RESTART_NODE, 10);
  restart.target = "node-1";  // delay defaults to 0

  const size_t scheduled = EventFactory::scheduleAll(
      {restart}, nodes, scheduler, skipped);

  REQUIRE(scheduled == 1);
  REQUIRE(scheduler.getPendingEventCount() == 1);
}

TEST_CASE("EventFactory::scheduleAll reports what it skipped", "[event_factory]") {
  const auto nodes = testNodes();
  EventScheduler scheduler;
  std::vector<std::string> skipped;

  auto good = makeEvent(EventAction::CRASH_NODE, 5);
  good.target = "node-1";

  auto unbuildable = makeEvent(EventAction::SET_NETWORK_QUALITY, 7);
  unbuildable.description = "degrade everything";

  auto bad_target = makeEvent(EventAction::START_NODE, 9);
  bad_target.target = "ghost";
  bad_target.description = "start a node that was never declared";

  const size_t scheduled = EventFactory::scheduleAll(
      {good, unbuildable, bad_target}, nodes, scheduler, skipped);

  REQUIRE(scheduled == 1);
  REQUIRE(skipped.size() == 2);
  REQUIRE(scheduler.getPendingEventCount() == 1);
  // A skipped event must say why -- silently dropping the timeline is the bug
  // this whole layer exists to fix.
  REQUIRE(skipped[0].find("not implemented") != std::string::npos);
  REQUIRE(skipped[1].find("ghost") != std::string::npos);
}
