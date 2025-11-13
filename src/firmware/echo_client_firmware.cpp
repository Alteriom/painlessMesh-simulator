/**
 * @file echo_client_firmware.cpp
 * @brief Implementation and registration of EchoClientFirmware
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include "simulator/firmware/echo_client_firmware.hpp"
#include "simulator/firmware/firmware_factory.hpp"

using namespace simulator::firmware;

// Register the EchoClient firmware with the factory
REGISTER_FIRMWARE(EchoClient, EchoClientFirmware)
