# Windows Build Guide

This guide explains how to build the painlessMesh simulator on Windows using the same setup as the CI/CD pipeline.

## Prerequisites

### Required Software

1. **Visual Studio 2022** (Community Edition or higher)
   - Download: https://visualstudio.microsoft.com/downloads/
   - Workloads needed:
     - "Desktop development with C++"
     - "C++ CMake tools for Windows"

2. **Git for Windows**
   - Download: https://git-scm.com/download/win
   - Ensure Git is in your PATH

3. **vcpkg** (Package Manager)
   - Will be installed via script below

## Quick Start (Automated)

### Option 1: Using PowerShell Script

Run the automated build script that mimics the CI/CD pipeline:

```powershell
# Navigate to project root
cd d:\Github\painlessMesh-simulator

# Run the build script
.\scripts\build-windows-ci.ps1
```

This script will:
- Install vcpkg if not present
- Install required dependencies (boost, yaml-cpp)
- Configure CMake with the same settings as CI
- Build the project in Release mode

### Option 2: Manual Build (Step-by-Step)

If you prefer to understand each step or need to troubleshoot:

#### Step 1: Install vcpkg

```powershell
# Clone vcpkg to C:\vcpkg (same location as CI)
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg

# Bootstrap vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
```

#### Step 2: Install Dependencies

```powershell
# Install boost and yaml-cpp for x64-windows
C:\vcpkg\vcpkg.exe install boost-system:x64-windows boost-asio:x64-windows yaml-cpp:x64-windows
```

**Note**: The CI pipeline only installs `boost-system` and `boost-asio`, but `yaml-cpp` is needed for configuration loading.

#### Step 3: Configure CMake

```powershell
# From project root
cmake -G "Visual Studio 17 2022" `
  -A x64 `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -B build
```

#### Step 4: Build

```powershell
# Build the project
cmake --build build --config Release
```

#### Step 5: Run (Once Implemented)

```powershell
# Run the simulator
.\build\bin\Release\painlessmesh-simulator.exe --config examples\scenarios\simple_mesh.yaml
```

## Using Docker (Alternative Method)

If you want an even more accurate replica of the CI environment, you can use Docker with Windows containers.

### Prerequisites

1. **Docker Desktop for Windows**
   - Download: https://www.docker.com/products/docker-desktop
   - Enable Windows containers (not Linux containers)

2. **Windows Container Support**
   - Windows 10/11 Pro, Enterprise, or Server
   - Hyper-V enabled

### Build and Run with Docker

```powershell
# Build the Docker image (mimics CI environment)
docker build -f docker\Dockerfile.windows -t painlessmesh-simulator-windows .

# Run a container
docker run -it --rm painlessmesh-simulator-windows

# Or build inside the container
docker run -it --rm -v ${PWD}:C:\workspace painlessmesh-simulator-windows powershell
```

## Build Options

### Debug Build

For debugging with Visual Studio:

```powershell
cmake -G "Visual Studio 17 2022" `
  -A x64 `
  -DCMAKE_BUILD_TYPE=Debug `
  -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -B build-debug

cmake --build build-debug --config Debug
```

Then open `build-debug\painlessmesh-simulator.sln` in Visual Studio.

### With Testing Enabled

```powershell
cmake -G "Visual Studio 17 2022" `
  -A x64 `
  -DCMAKE_BUILD_TYPE=Release `
  -DENABLE_TESTING=ON `
  -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -B build

cmake --build build --config Release

# Run tests
cd build
ctest -C Release --output-on-failure
```

## Troubleshooting

### vcpkg Installation Failed

**Problem**: `bootstrap-vcpkg.bat` fails or vcpkg.exe not created.

**Solution**:
- Ensure you have Visual Studio Build Tools installed
- Run `vcpkg integrate install` after bootstrap
- Try running from "Developer Command Prompt for VS 2022"

### CMake Cannot Find Boost or yaml-cpp

**Problem**: CMake configuration fails with "Could not find Boost" or similar.

**Solution**:
- Verify vcpkg packages are installed: `C:\vcpkg\vcpkg.exe list`
- Ensure you're using the correct toolchain file path
- Check that the architecture matches (`x64-windows`)

### Build Fails with C++ Standard Errors

**Problem**: Compiler errors about C++14 features.

**Solution**:
- Ensure Visual Studio 2017 or later is installed
- Update Visual Studio to the latest version
- Check CMakeLists.txt has `set(CMAKE_CXX_STANDARD 14)`

### Missing DLLs at Runtime

**Problem**: "The code execution cannot proceed because [some].dll was not found"

**Solution**:
- Copy required DLLs from vcpkg installed directory:
  ```powershell
  copy C:\vcpkg\installed\x64-windows\bin\*.dll build\bin\Release\
  ```
- Or add vcpkg bin directory to PATH:
  ```powershell
  $env:PATH = "C:\vcpkg\installed\x64-windows\bin;$env:PATH"
  ```

### CMake Generates for Wrong Architecture

**Problem**: Build fails with x86/x64 mismatch errors.

**Solution**:
- Always specify `-A x64` when running CMake
- Delete the build directory and reconfigure:
  ```powershell
  Remove-Item -Recurse -Force build
  ```

## Performance Considerations

### Build Performance

To speed up builds:

1. **Use Ninja instead of Visual Studio generator**:
   ```powershell
   # Install Ninja via vcpkg or Chocolatey
   choco install ninja
   
   # Use Ninja generator
   cmake -G Ninja `
     -DCMAKE_BUILD_TYPE=Release `
     -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" `
     -B build
   
   cmake --build build
   ```

2. **Enable parallel builds**:
   ```powershell
   cmake --build build --config Release -- /maxcpucount
   ```

3. **Use ccache** (if available):
   ```powershell
   cmake -DCMAKE_CXX_COMPILER_LAUNCHER=ccache ...
   ```

### Runtime Performance

Windows builds may have different performance characteristics than Linux:

- **Thread Scheduling**: Windows thread scheduler differs from Linux
- **Network I/O**: Boost.Asio performance may vary
- **Memory Management**: Different allocator behavior

For benchmarking, always compare Windows builds against other Windows builds, not cross-platform.

## Continuous Integration Parity

The automated script (`scripts\build-windows-ci.ps1`) ensures:

✅ Same Visual Studio version (VS 2022)
✅ Same architecture (x64)
✅ Same package manager (vcpkg)
✅ Same dependencies (boost-system, boost-asio, yaml-cpp)
✅ Same build configuration (Release)
✅ Same CMake generator options

## Development Workflow

### Recommended Setup

1. **Install Visual Studio 2022** with CMake tools
2. **Open project as CMake project** in Visual Studio:
   - File → Open → CMake...
   - Select `CMakeLists.txt`
   - Visual Studio will auto-configure

3. **Configure CMake settings** in Visual Studio:
   - Project → CMake Settings
   - Add toolchain file path: `C:\vcpkg\scripts\buildsystems\vcpkg.cmake`

4. **Build and debug** directly in Visual Studio:
   - Build → Build All
   - Debug → Start Debugging (F5)

### VS Code Setup

If using VS Code instead:

1. Install extensions:
   - C/C++ (Microsoft)
   - CMake Tools (Microsoft)

2. Configure `.vscode/settings.json`:
   ```json
   {
     "cmake.configureSettings": {
       "CMAKE_TOOLCHAIN_FILE": "C:/vcpkg/scripts/buildsystems/vcpkg.cmake"
     },
     "cmake.generator": "Visual Studio 17 2022"
   }
   ```

3. Configure and build using CMake Tools extension

## Cleaning Build Artifacts

To start fresh:

```powershell
# Remove build directory
Remove-Item -Recurse -Force build

# Remove CMake cache
Remove-Item -Force CMakeCache.txt, CMakeFiles -ErrorAction SilentlyContinue

# Remove vcpkg installed packages (if needed)
C:\vcpkg\vcpkg.exe remove boost-system:x64-windows boost-asio:x64-windows yaml-cpp:x64-windows
```

## Advanced: Multi-Configuration Builds

Visual Studio is a multi-configuration generator. You can build both Debug and Release from the same CMake configuration:

```powershell
# Configure once
cmake -G "Visual Studio 17 2022" `
  -A x64 `
  -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -B build

# Build Debug
cmake --build build --config Debug

# Build Release
cmake --build build --config Release

# Both outputs available:
# build\bin\Debug\painlessmesh-simulator.exe
# build\bin\Release\painlessmesh-simulator.exe
```

## CI/CD Pipeline Differences

The CI pipeline differs slightly from local builds:

| Aspect | CI Pipeline | Local Build |
|--------|-------------|-------------|
| OS | Windows Server 2022 | Windows 10/11 |
| VS Version | VS 2022 (latest) | VS 2022 (your version) |
| vcpkg Location | `C:\vcpkg` | `C:\vcpkg` (recommended) |
| Clean Build | Always | Only if you delete build/ |
| yaml-cpp | Not installed (bug?) | Should be installed |

**Note**: The CI pipeline appears to be missing `yaml-cpp` installation. This should be added to the workflow.

## Next Steps

Once the build succeeds:

1. **Run unit tests**: `ctest -C Release --output-on-failure`
2. **Try example scenarios**: See `examples/scenarios/*.yaml`
3. **Develop custom firmware**: See `docs/FIRMWARE_DEVELOPMENT.md`
4. **Contribute**: See `CONTRIBUTING.md`

## Support

If you encounter issues:

1. Check [Troubleshooting](#troubleshooting) section above
2. Review [GitHub Issues](https://github.com/Alteriom/painlessMesh-simulator/issues)
3. Open a new issue with:
   - Error message / logs
   - Windows version
   - Visual Studio version
   - Steps to reproduce

---

**Last Updated**: 2025-11-12
**Tested With**: Visual Studio 2022 (17.8+), Windows 10/11, vcpkg (2024.11.16)
