/**
 * @file simple_broadcast_firmware.cpp
 * @brief Implementation and registration of SimpleBroadcastFirmware
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include "simulator/firmware/simple_broadcast_firmware.hpp"
#include "simulator/firmware/firmware_factory.hpp"

using namespace simulator::firmware;

// Register the SimpleBroadcast firmware with the factory
REGISTER_FIRMWARE(SimpleBroadcast, SimpleBroadcastFirmware)
