/**
 * @file platform_compat.hpp
 * @brief Platform compatibility layer for Windows/Unix differences
 * 
 * Provides Windows implementations of Unix-specific functions used by painlessMesh
 * 
 * IMPORTANT: This header MUST be included before any painlessMesh headers.
 * It provides stub sys/time.h and unistd.h headers for Windows builds.
 * 
 * @copyright Copyright (c) 2025 Alteriom
 * @license MIT License
 */

#ifndef SIMULATOR_PLATFORM_COMPAT_HPP
#define SIMULATOR_PLATFORM_COMPAT_HPP

#ifdef _WIN32
// Windows-specific includes and definitions

// Define _WIN32_WINNT before including any Windows headers
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601  // Windows 7
#endif

// CRITICAL: Include winsock2.h BEFORE windows.h to prevent winsock.h inclusion
// Boost.Asio requires winsock2.h, and windows.h would include winsock.h by default
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <windows.h>

#include <cstdint>
#include <ctime>

// Note: timeval is already defined in winsock2.h, so we don't redefine it

// Provide gettimeofday for Windows
inline int gettimeofday(struct timeval* tp, void* tzp) {
    (void)tzp; // Unused
    
    // Get current time in 100-nanosecond intervals since January 1, 1601
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);
    
    // Convert to microseconds
    uint64_t time = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    time /= 10;  // Convert from 100ns to 1us intervals
    
    // Subtract the difference between 1601 and 1970 (in microseconds)
    time -= 11644473600000000ULL;
    
    tp->tv_sec = (long)(time / 1000000UL);
    tp->tv_usec = (long)(time % 1000000UL);
    
    return 0;
}

// Provide usleep for Windows
inline void usleep(unsigned int usec) {
    HANDLE timer;
    LARGE_INTEGER li;
    
    // Convert to 100-nanosecond intervals (negative means relative time)
    li.QuadPart = -(int64_t)usec * 10;
    
    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    if (timer) {
        SetWaitableTimer(timer, &li, 0, NULL, NULL, FALSE);
        WaitForSingleObject(timer, INFINITE);
        CloseHandle(timer);
    }
}

#else
// Unix/Linux/macOS - use standard POSIX headers
#include <sys/time.h>
#include <unistd.h>
#endif

#endif // SIMULATOR_PLATFORM_COMPAT_HPP
