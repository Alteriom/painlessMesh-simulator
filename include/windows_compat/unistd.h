/**
 * @file unistd.h
 * @brief Stub header for Windows compatibility
 * 
 * This is a stub header that gets included when Arduino.h tries to include <unistd.h>
 * on Windows. The actual implementation is in simulator/platform_compat.hpp which must
 * be included first.
 * 
 * For Unix/Linux/macOS builds, this file should NOT be in the include path at all.
 * It will cause conflicts with the system unistd.h.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifdef _WIN32
// This header is ONLY for Windows builds
#ifndef SIMULATOR_UNISTD_H_COMPAT
#define SIMULATOR_UNISTD_H_COMPAT

// On Windows, we expect platform_compat.hpp to be included first
// which provides usleep()
#ifndef SIMULATOR_PLATFORM_COMPAT_HPP
#error "platform_compat.hpp must be included before unistd.h on Windows"
#endif

#endif // SIMULATOR_UNISTD_H_COMPAT
#else
// On Unix/Linux/macOS, this stub header should NOT be used!
// Include the system header directly
#include <unistd.h>
#endif
