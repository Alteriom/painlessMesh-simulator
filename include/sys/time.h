/**
 * @file sys/time.h
 * @brief Stub header for Windows compatibility
 * 
 * This is a stub header that gets included when Arduino.h tries to include <sys/time.h>
 * on Windows. The actual implementation is in simulator/platform_compat.hpp which must
 * be included first.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef _SIMULATOR_SYS_TIME_H
#define _SIMULATOR_SYS_TIME_H

#ifdef _WIN32
// On Windows, we expect platform_compat.hpp to be included first
// which provides gettimeofday() and timeval
#ifndef SIMULATOR_PLATFORM_COMPAT_HPP
#error "platform_compat.hpp must be included before sys/time.h on Windows"
#endif
#else
// On Unix/Linux/macOS, just include the real header
#include_next <sys/time.h>
// Ensure functions and types are in global namespace
using ::timeval;
using ::gettimeofday;
#endif

#endif // _SIMULATOR_SYS_TIME_H
