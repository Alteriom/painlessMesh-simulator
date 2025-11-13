# Windows Compatibility Fixes for painlessMesh

This document describes the fixes needed in the `D:\Github\painlessMesh` repository for Windows compilation.

## Files to Fix

### 1. `src/boost/asynctcp.hpp` (Line ~236)

**Issue**: `setsockopt()` requires `const char*` on Windows, not `int*`

**Fix**:
```cpp
// Around line 234-238
    mAcceptor.open(tcp::v4());
    int one = 1;
#ifdef _WIN32
    setsockopt(mAcceptor.native_handle(), SOL_SOCKET,
               SO_REUSEADDR | SO_REUSEPORT, reinterpret_cast<const char*>(&one), sizeof(one));
#else
    setsockopt(mAcceptor.native_handle(), SOL_SOCKET,
               SO_REUSEADDR | SO_REUSEPORT, &one, sizeof(one));
#endif
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), _port);
```

### 2. `src/painlessmesh/logger.hpp` (Lines ~8-25)

**Issue 1**: Windows defines `ERROR` as a macro, conflicts with enum

**Fix**:
```cpp
namespace logger {
#include <stdarg.h>
#include "Arduino.h"

#define REMOTE_QUEUE_SIZE 10

// Windows defines ERROR as a macro, undefine it to avoid conflicts
#ifdef ERROR
#undef ERROR
#endif

typedef enum {
  ERROR = 1 << 0,
  STARTUP = 1 << 1,
  // ... rest of enum
```

**Issue 2**: `uint` is not a standard C++ type (Lines ~150-158)

**Fix**:
```cpp
// Change all instances of 'uint' to 'uint32_t'
  std::list<std::pair<uint32_t, TSTRING>> &get_remote_queue() {
    return remote_queue;
  }

 private:
  uint16_t types = 0;
  char str[200];
  std::list<std::pair<uint32_t, TSTRING>> remote_queue;
  uint32_t remote_uuid;
```

### 3. `src/painlessmesh/protocol.hpp` (Line ~383)

**Issue**: `uint` is not a standard C++ type

**Fix**:
```cpp
  void reply(uint32_t newT0) {
    msg.t0 = newT0;
    ++msg.type;
```

### 4. `src/painlessmesh/mesh.hpp` (Line ~2288)

**Issue**: Windows `min`/`max` macros conflict with `std::min`/`std::max`

**Fix**:
```cpp
// Wrap std::min and std::max in parentheses to prevent macro expansion
    return (std::max)(0, (std::min)(100, quality));
```

## How to Apply Fixes

Since the simulator now uses your local `D:\Github\painlessMesh` clone, you can:

1. Open each file in the painlessMesh repository
2. Apply the fixes listed above
3. Rebuild the simulator - it will automatically use your fixed version

## Testing

After applying all fixes, rebuild the simulator:

```powershell
cd D:\Github\painlessMesh-simulator\build
cmake --build . --config Release
```

## Contributing Back

Once these fixes work, consider:
1. Creating a branch in the painlessMesh repo
2. Committing these Windows compatibility fixes
3. Opening a pull request to the Alteriom/painlessMesh repository

This will help other Windows developers!
