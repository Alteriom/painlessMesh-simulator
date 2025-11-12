/**
 * @file painlessmesh_support.cpp
 * @brief Support objects for painlessMesh integration
 * 
 * This file provides the necessary global objects and includes
 * for the painlessMesh library to function properly in simulation.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#define ARDUINO_ARCH_ESP8266
#define PAINLESSMESH_BOOST

// Define these before including TaskScheduler to compile the implementation
#define _TASK_PRIORITY
#define _TASK_STD_FUNCTION

#include <TaskScheduler.h>

#include "boost/asynctcp.hpp"
#include "painlessmesh/logger.hpp"

// Provide WiFi and ESP instances
WiFiClass WiFi;
ESPClass ESP;

// Provide Log instance for painlessMesh - must be global, not in namespace
using namespace painlessmesh;
painlessmesh::logger::LogClass Log;
