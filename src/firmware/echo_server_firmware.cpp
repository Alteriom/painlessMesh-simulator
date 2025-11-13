/**
 * @file echo_server_firmware.cpp
 * @brief Implementation and registration of EchoServerFirmware
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#include "simulator/firmware/echo_server_firmware.hpp"
#include "simulator/firmware/firmware_factory.hpp"

using namespace simulator::firmware;

// Register the EchoServer firmware with the factory
REGISTER_FIRMWARE(EchoServer, EchoServerFirmware)
