/**
 * @file firmware_base.cpp
 * @brief Implementation of FirmwareBase helper methods
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include "simulator/firmware/firmware_base.hpp"
#include "Arduino.h"  // For TSTRING typedef
#include "painlessmesh/mesh.hpp"
#include <list>

namespace simulator {
namespace firmware {

void FirmwareBase::sendBroadcast(const String& msg) {
  if (mesh_) {
    String msg_copy = msg;  // painlessMesh modifies the message
    mesh_->sendBroadcast(msg_copy);
  }
}

void FirmwareBase::sendSingle(uint32_t dest, const String& msg) {
  if (mesh_) {
    String msg_copy = msg;  // painlessMesh modifies the message
    mesh_->sendSingle(dest, msg_copy);
  }
}

uint32_t FirmwareBase::getNodeTime() const {
  return mesh_ ? mesh_->getNodeTime() : 0;
}

std::list<uint32_t> FirmwareBase::getNodeList() const {
  return mesh_ ? mesh_->getNodeList() : std::list<uint32_t>();
}

} // namespace firmware
} // namespace simulator
