# Windows Build Quick Start

This guide will help you build the painlessMesh simulator on Windows using the **exact same configuration** as the CI/CD pipeline.

## 🚀 Quick Start (Automated)

```powershell
# 1. Check prerequisites
.\scripts\verify-windows-prereqs.ps1

# 2. Build (this will install vcpkg and dependencies automatically)
.\scripts\build-windows-ci.ps1
```

That's it! The build script will:
- ✅ Install vcpkg to `C:\vcpkg`
- ✅ Install boost-system, boost-asio, and yaml-cpp
- ✅ Configure CMake with Visual Studio 2022
- ✅ Build in Release mode

## 📋 Prerequisites

### Required

1. **Visual Studio 2022** (any edition)
   - With "Desktop development with C++" workload
   - Download: https://visualstudio.microsoft.com/downloads/

2. **CMake** (one of):
   - Via Visual Studio Installer > Individual Components > "C++ CMake tools"
   - Or standalone: https://cmake.org/download/
   - Or Chocolatey: `choco install cmake`

3. **Git for Windows**
   - Download: https://git-scm.com/download/win

### Optional (auto-installed)

- **vcpkg** - Will be installed to `C:\vcpkg` by build script
- **Dependencies** - boost, yaml-cpp installed via vcpkg

## ⚙️ Build Options

### Default Build (Release)

```powershell
.\scripts\build-windows-ci.ps1
```

### Debug Build

```powershell
.\scripts\build-windows-ci.ps1 -BuildType Debug
```

### Clean Build

```powershell
.\scripts\build-windows-ci.ps1 -Clean
```

### Skip vcpkg Setup (if already installed)

```powershell
.\scripts\build-windows-ci.ps1 -SkipVcpkgInstall
```

### Custom vcpkg Location

```powershell
.\scripts\build-windows-ci.ps1 -VcpkgRoot "D:\tools\vcpkg"
```

## 🐳 Docker Alternative

For an even more accurate CI replica, use Docker with Windows containers:

```powershell
# Build Docker image (requires Docker Desktop with Windows containers)
docker build -f docker\Dockerfile.windows -t painlessmesh-simulator-windows .

# Run interactive container
docker run -it --rm -v ${PWD}:C:\workspace painlessmesh-simulator-windows

# Inside container:
PS C:\workspace> cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=$env:CMAKE_TOOLCHAIN_FILE -B build
PS C:\workspace> cmake --build build --config Release
```

**Note**: Requires Docker Desktop for Windows with Windows containers enabled (not Linux containers).

## 🔧 Manual Build (Step-by-Step)

If you prefer to build manually:

### Step 1: Install vcpkg

```powershell
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
```

### Step 2: Install Dependencies

```powershell
C:\vcpkg\vcpkg.exe install boost-system:x64-windows boost-asio:x64-windows yaml-cpp:x64-windows
```

### Step 3: Configure and Build

```powershell
cmake -G "Visual Studio 17 2022" `
  -A x64 `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" `
  -B build

cmake --build build --config Release
```

## ❓ Troubleshooting

### CMake Not Found

**Problem**: `cmake : The term 'cmake' is not recognized...`

**Solutions**:
1. Install via Visual Studio Installer > Individual Components > "C++ CMake tools"
2. Or download from https://cmake.org/download/ and add to PATH
3. Or use Developer Command Prompt for VS 2022 (has CMake in PATH)

### Missing Visual Studio 2022

**Problem**: Script says Visual Studio not found

**Solution**: Install Visual Studio 2022 with "Desktop development with C++" workload
- Download: https://visualstudio.microsoft.com/downloads/

### vcpkg Installation Fails

**Problem**: Bootstrap fails or packages won't install

**Solutions**:
1. Ensure Visual Studio C++ build tools are installed
2. Run from Developer Command Prompt for VS 2022
3. Check antivirus isn't blocking vcpkg

### Missing DLLs at Runtime

**Problem**: "The code execution cannot proceed because [some].dll was not found"

**Solution**: Copy DLLs from vcpkg:
```powershell
copy C:\vcpkg\installed\x64-windows\bin\*.dll build\bin\Release\
```

### Build Fails with Architecture Mismatch

**Problem**: x86/x64 mismatch errors

**Solution**: Delete `build/` directory and ensure `-A x64` is specified:
```powershell
Remove-Item -Recurse -Force build
.\scripts\build-windows-ci.ps1 -Clean
```

## 📊 Build Output

After successful build, you'll find:

```
build/
├── bin/
│   └── Release/
│       └── painlessmesh-simulator.exe (when implemented)
├── lib/
│   └── Release/
│       └── *.lib (static libraries)
└── painlessmesh-simulator.sln (Visual Studio solution)
```

## 🎯 Next Steps

Once built:

1. **Run tests** (when implemented):
   ```powershell
   cd build
   ctest -C Release --output-on-failure
   ```

2. **Run simulator** (when implemented):
   ```powershell
   .\build\bin\Release\painlessmesh-simulator.exe --config examples\scenarios\simple_mesh.yaml
   ```

3. **Open in Visual Studio**:
   ```powershell
   .\build\painlessmesh-simulator.sln
   ```

4. **Debug** in VS Code or Visual Studio with full debugging support

## 📚 Additional Resources

- **[Detailed Windows Build Guide](WINDOWS_BUILD_GUIDE.md)** - Comprehensive guide with advanced topics
- **[CI/CD Configuration](../.github/workflows/ci.yml)** - See exact CI settings
- **[Contributing Guide](../CONTRIBUTING.md)** - Development workflow

## ⚡ CI/CD Parity

This setup matches the CI/CD pipeline exactly:

| Component | CI/CD | Your Build |
|-----------|-------|------------|
| OS | Windows Server 2022 | Windows 10/11 |
| VS Version | 2022 (latest) | 2022 |
| CMake Generator | Visual Studio 17 2022 | ✅ Same |
| Architecture | x64 | ✅ Same |
| Build Type | Release | ✅ Same |
| Package Manager | vcpkg | ✅ Same |
| Dependencies | boost, yaml-cpp | ✅ Same |
| vcpkg Location | C:\vcpkg | ✅ Same (configurable) |

## 🐛 Getting Help

If you encounter issues:

1. Run `.\scripts\verify-windows-prereqs.ps1` to check setup
2. Check [Troubleshooting](#-troubleshooting) section
3. Review [WINDOWS_BUILD_GUIDE.md](WINDOWS_BUILD_GUIDE.md)
4. Open a GitHub issue with:
   - Error message
   - Windows version
   - Visual Studio version
   - Output of verification script

---

**Last Updated**: 2025-11-12  
**Tested With**: Windows 10/11, Visual Studio 2022 Community, vcpkg 2024.11.16
