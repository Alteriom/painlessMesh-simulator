/**
 * @file firmware_base.cpp
 * @brief Implementation of FirmwareBase helper methods
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include "simulator/firmware/firmware_base.hpp"
#include "Arduino.h"  // For TSTRING typedef
#include <TaskSchedulerDeclarations.h>
#include "painlessmesh/mesh.hpp"
#include <list>

namespace simulator {
namespace firmware {

bool FirmwareBase::registerTask(Task& task, bool enable_now) {
  if (!scheduler_) {
    return false;
  }
  scheduler_->addTask(task);
  tasks_.push_back(&task);
  if (enable_now) {
    task.enable();
  }
  return true;
}

void FirmwareBase::suspend() {
  if (suspended_) {
    return;
  }
  enabled_before_suspend_.clear();
  enabled_before_suspend_.reserve(tasks_.size());
  for (Task* task : tasks_) {
    enabled_before_suspend_.push_back(task->isEnabled());
    task->disable();
  }
  suspended_ = true;
}

void FirmwareBase::resume() {
  if (!suspended_) {
    return;
  }
  for (size_t i = 0; i < tasks_.size(); ++i) {
    if (i < enabled_before_suspend_.size() && enabled_before_suspend_[i]) {
      tasks_[i]->enable();
    }
  }
  enabled_before_suspend_.clear();
  suspended_ = false;
}

bool FirmwareBase::sendBroadcast(const String& msg) {
  // A suspended firmware belongs to a node that is down. Its mesh pointer is
  // still non-null -- it addresses the stopped instance until start() rebuilds
  // one -- so without this guard the send reaches torn-down routing state and
  // the accounting hook books a transmission that never left the node.
  if (suspended_ || !mesh_) {
    return false;
  }
  String msg_copy = msg;  // painlessMesh modifies the message
  // router::broadcast() returns the number of nodes it reached; Mesh turns a
  // zero into false. Counting regardless would report an isolated node's
  // traffic as delivered, which is what the metrics and the gate read.
  if (!mesh_->sendBroadcast(msg_copy)) {
    return false;
  }
  if (on_message_sent_) on_message_sent_(msg.length());
  return true;
}

bool FirmwareBase::sendSingle(uint32_t dest, const String& msg) {
  if (suspended_ || !mesh_) {
    return false;
  }
  String msg_copy = msg;  // painlessMesh modifies the message
  if (!mesh_->sendSingle(dest, msg_copy)) {
    return false;
  }
  if (on_message_sent_) on_message_sent_(msg.length());
  return true;
}

uint32_t FirmwareBase::getNodeTime() const {
  return mesh_ ? mesh_->getNodeTime() : 0;
}

std::list<uint32_t> FirmwareBase::getNodeList() const {
  return mesh_ ? mesh_->getNodeList() : std::list<uint32_t>();
}

} // namespace firmware
} // namespace simulator
