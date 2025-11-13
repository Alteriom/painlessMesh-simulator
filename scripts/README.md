# Build Scripts

This directory contains automated build and utility scripts for the painlessMesh simulator.

## Windows Build Scripts

### `build-windows-ci.ps1`

**Purpose**: Automated Windows build script that replicates the CI/CD pipeline configuration.

**Features**:
- Installs vcpkg automatically to `C:\vcpkg`
- Installs required dependencies via vcpkg
- Configures CMake with Visual Studio 2022 (x64)
- Builds project in Release or Debug mode
- Supports clean builds and custom vcpkg locations

**Usage**:
```powershell
# Standard Release build
.\scripts\build-windows-ci.ps1

# Debug build
.\scripts\build-windows-ci.ps1 -BuildType Debug

# Clean build (delete build directory first)
.\scripts\build-windows-ci.ps1 -Clean

# Skip vcpkg installation (if already set up)
.\scripts\build-windows-ci.ps1 -SkipVcpkgInstall

# Custom vcpkg location
.\scripts\build-windows-ci.ps1 -VcpkgRoot "D:\tools\vcpkg"

# Combine options
.\scripts\build-windows-ci.ps1 -BuildType Debug -Clean
```

**Parameters**:
- `-SkipVcpkgInstall`: Skip vcpkg and dependency installation
- `-BuildType`: "Debug" or "Release" (default: "Release")
- `-Clean`: Remove existing build directory before building
- `-VcpkgRoot`: Path to vcpkg installation (default: "C:\vcpkg")

**Requirements**:
- Visual Studio 2022 with C++ desktop development workload
- CMake (standalone or included with VS)
- Git for Windows

**See Also**: [Windows Build Quick Start](../docs/WINDOWS_BUILD_QUICKSTART.md)

---

### `verify-windows-prereqs.ps1`

**Purpose**: Verifies that all prerequisites for Windows builds are installed.

**Checks**:
- ✅ Visual Studio 2022 installation and C++ build tools
- ✅ CMake availability (in PATH or VS installation)
- ✅ Git for Windows
- ✅ vcpkg installation (optional)
- ✅ vcpkg packages (boost-system, boost-asio, yaml-cpp)
- ✅ PowerShell version

**Usage**:
```powershell
.\scripts\verify-windows-prereqs.ps1
```

**Exit Codes**:
- `0`: All required prerequisites are installed
- `1`: Some prerequisites are missing

**Example Output**:
```
=======================================================
  Windows Build Prerequisites Check
=======================================================

> Checking Visual Studio 2022...
  [OK] Found at: C:\Program Files\Microsoft Visual Studio\2022\Community
  [OK] C++ build tools are installed
> Checking CMake...
  [OK] Found: C:\Program Files\CMake\bin\cmake.exe
  [OK] Version: cmake version 3.28.1
> Checking Git...
  [OK] Found: C:\Program Files\Git\cmd\git.exe
  [OK] Version: git version 2.43.0.windows.1
> Checking vcpkg (optional)...
  [OK] Found at: C:\vcpkg\vcpkg.exe
  [OK] boost-system:x64-windows is installed
  [OK] boost-asio:x64-windows is installed
  [OK] yaml-cpp:x64-windows is installed

=======================================================
  All required prerequisites are installed!

  You can now run:
    .\scripts\build-windows-ci.ps1
=======================================================
```

**See Also**: [Windows Build Guide](../docs/WINDOWS_BUILD_GUIDE.md#troubleshooting)

---

## Linux/macOS Build Scripts

### `create-phase1-issues.sh`

**Purpose**: Creates GitHub issues for Phase 1 implementation tasks.

**Usage**:
```bash
./scripts/create-phase1-issues.sh
```

**Requirements**:
- GitHub CLI (`gh`) installed and authenticated

**See Also**: [QUICK_ISSUE_CREATION.md](../QUICK_ISSUE_CREATION.md)

---

### `test-gh-auth.sh`

**Purpose**: Tests GitHub CLI authentication.

**Usage**:
```bash
./scripts/test-gh-auth.sh
```

**Requirements**:
- GitHub CLI (`gh`) installed

---

## Script Development Guidelines

When adding new scripts:

1. **Use appropriate shell**:
   - Windows: PowerShell (`.ps1`)
   - Linux/macOS: Bash (`.sh`)

2. **Include script header**:
   ```powershell
   <#
   .SYNOPSIS
       Brief description
   
   .DESCRIPTION
       Detailed description
   
   .PARAMETER ParamName
       Parameter description
   
   .EXAMPLE
       .\script.ps1 -ParamName value
   
   .NOTES
       Additional notes
   #>
   ```

3. **Set error handling**:
   ```powershell
   # PowerShell
   $ErrorActionPreference = "Stop"
   
   # Bash
   set -euo pipefail
   ```

4. **Use colored output**:
   ```powershell
   # PowerShell
   Write-Host "Success" -ForegroundColor Green
   Write-Host "Warning" -ForegroundColor Yellow
   Write-Host "Error" -ForegroundColor Red
   ```

5. **Document in this README**: Add entry above

6. **Make executable** (Linux/macOS):
   ```bash
   chmod +x scripts/your-script.sh
   ```

---

## CI/CD Integration

Scripts in this directory are designed to:
- ✅ Match CI/CD pipeline configuration exactly
- ✅ Provide reproducible builds locally
- ✅ Support both interactive and automated use
- ✅ Handle missing dependencies gracefully
- ✅ Provide clear error messages

---

## Documentation

- **[Windows Build Quick Start](../docs/WINDOWS_BUILD_QUICKSTART.md)** - Get started in 5 minutes
- **[Windows Build Guide](../docs/WINDOWS_BUILD_GUIDE.md)** - Comprehensive guide
- **[Setup Summary](../docs/SETUP_SUMMARY.md)** - Overview of all build tools

---

**Last Updated**: 2025-11-12
