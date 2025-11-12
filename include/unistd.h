/**
 * @file unistd.h
 * @brief Stub header for Windows compatibility
 * 
 * This is a stub header that gets included when Arduino.h tries to include <unistd.h>
 * on Windows. The actual implementation is in simulator/platform_compat.hpp which must
 * be included first.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef _UNISTD_H
#define _UNISTD_H

#ifdef _WIN32
// On Windows, we expect platform_compat.hpp to be included first
// which provides usleep()
#ifndef SIMULATOR_PLATFORM_COMPAT_HPP
#error "platform_compat.hpp must be included before unistd.h on Windows"
#endif
#else
// On Unix/Linux/macOS, just include the real header
#include_next <unistd.h>
#endif

#endif // _UNISTD_H
