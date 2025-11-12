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

// Include Arduino compatibility header which sets up proper header ordering
// This Arduino.h is in include/simulator/boost/Arduino.h and will be found
// via the include path set in CMakeLists.txt
#include "Arduino.h"

// Now include TaskScheduler implementation
#include <TaskScheduler.h>

#include "painlessmesh/logger.hpp"

// Provide WiFi and ESP instances
WiFiClass WiFi;
ESPClass ESP;

// Provide Log instance for painlessMesh - must be global, not in namespace
using namespace painlessmesh;
painlessmesh::logger::LogClass Log;
