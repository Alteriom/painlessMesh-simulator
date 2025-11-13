#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Builds the painlessMesh simulator on Windows using the same configuration as the CI/CD pipeline.

.DESCRIPTION
    This script replicates the Windows build job from .github/workflows/ci.yml:
    - Installs vcpkg if not present
    - Installs required dependencies (boost-system, boost-asio, yaml-cpp)
    - Configures CMake with Visual Studio 2022
    - Builds the project in Release mode

.PARAMETER SkipVcpkgInstall
    Skip vcpkg installation and dependency installation (use if already set up)

.PARAMETER BuildType
    Build configuration type (Debug or Release). Default: Release

.PARAMETER Clean
    Remove existing build directory before building

.PARAMETER VcpkgRoot
    Path to vcpkg installation. Default: C:\vcpkg

.EXAMPLE
    .\scripts\build-windows-ci.ps1
    
.EXAMPLE
    .\scripts\build-windows-ci.ps1 -BuildType Debug -Clean

.EXAMPLE
    .\scripts\build-windows-ci.ps1 -SkipVcpkgInstall

.NOTES
    Requires: Visual Studio 2022, Git for Windows
    Mimics: .github/workflows/ci.yml (build-windows job)
#>

param(
    [switch]$SkipVcpkgInstall = $false,
    [ValidateSet("Debug", "Release")]
    [string]$BuildType = "Release",
    [switch]$Clean = $false,
    [string]$VcpkgRoot = "C:\vcpkg"
)

# Script configuration
$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

# Colors for output
function Write-Success { Write-Host "[OK] $args" -ForegroundColor Green }
function Write-Info { Write-Host "> $args" -ForegroundColor Cyan }
function Write-Warning { Write-Host "[WARN] $args" -ForegroundColor Yellow }
function Write-Failure { Write-Host "[FAIL] $args" -ForegroundColor Red }

# Header
Write-Host ""
Write-Host "=======================================================" -ForegroundColor Magenta
Write-Host "  painlessMesh Simulator - Windows CI Build Script" -ForegroundColor Magenta
Write-Host "=======================================================" -ForegroundColor Magenta
Write-Host ""

# Verify project root
if (-not (Test-Path "CMakeLists.txt")) {
    Write-Failure "CMakeLists.txt not found. Please run this script from the project root."
    exit 1
}

# Check for Visual Studio 2022
Write-Info "Checking for Visual Studio 2022..."
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vsWhere)) {
    Write-Failure "vswhere.exe not found. Is Visual Studio installed?"
    exit 1
}

$vsPath = & $vsWhere -version "[17.0,18.0)" -property installationPath -latest
if (-not $vsPath) {
    Write-Failure "Visual Studio 2022 not found. Please install Visual Studio 2022 with C++ desktop development workload."
    Write-Info "Download: https://visualstudio.microsoft.com/downloads/"
    exit 1
}

Write-Success "Found Visual Studio 2022: $vsPath"

# Check for CMake
Write-Info "Checking for CMake..."
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    Write-Failure "CMake not found. Please install CMake or Visual Studio's CMake tools."
    exit 1
}
Write-Success "Found CMake: $($cmake.Source) (version: $(& cmake --version | Select-Object -First 1))"

# vcpkg setup
if (-not $SkipVcpkgInstall) {
    Write-Info "Setting up vcpkg..."
    
    # Install vcpkg if not present
    if (-not (Test-Path $VcpkgRoot)) {
        Write-Info "Cloning vcpkg to $VcpkgRoot..."
        git clone https://github.com/microsoft/vcpkg.git $VcpkgRoot
        if ($LASTEXITCODE -ne 0) {
            Write-Failure "Failed to clone vcpkg"
            exit 1
        }
        Write-Success "vcpkg cloned successfully"
    } else {
        Write-Success "vcpkg already exists at $VcpkgRoot"
    }
    
    # Bootstrap vcpkg
    $vcpkgExe = Join-Path $VcpkgRoot "vcpkg.exe"
    if (-not (Test-Path $vcpkgExe)) {
        Write-Info "Bootstrapping vcpkg..."
        $bootstrapScript = Join-Path $VcpkgRoot "bootstrap-vcpkg.bat"
        & cmd /c $bootstrapScript
        if ($LASTEXITCODE -ne 0) {
            Write-Failure "Failed to bootstrap vcpkg"
            exit 1
        }
        Write-Success "vcpkg bootstrapped successfully"
    } else {
        Write-Success "vcpkg already bootstrapped"
    }
    
    # Install dependencies
    Write-Info "Installing dependencies via vcpkg..."
    Write-Info "This may take several minutes on first run..."
    
    $packages = @(
        "boost-system:x64-windows",
        "boost-asio:x64-windows",
        "yaml-cpp:x64-windows"  # Note: CI is missing this but we need it
    )
    
    foreach ($package in $packages) {
        Write-Info "Installing $package..."
        & $vcpkgExe install $package
        if ($LASTEXITCODE -ne 0) {
            Write-Failure "Failed to install $package"
            exit 1
        }
    }
    
    Write-Success "All dependencies installed successfully"
} else {
    Write-Info "Skipping vcpkg setup (--SkipVcpkgInstall specified)"
}

# Verify vcpkg toolchain file exists
$toolchainFile = Join-Path $VcpkgRoot "scripts\buildsystems\vcpkg.cmake"
if (-not (Test-Path $toolchainFile)) {
    Write-Failure "vcpkg toolchain file not found: $toolchainFile"
    exit 1
}

# Initialize submodules
Write-Info "Initializing git submodules..."
git submodule update --init --recursive
if ($LASTEXITCODE -ne 0) {
    Write-Warning "Failed to update submodules (may already be initialized)"
}

# Clean build directory if requested
$buildDir = "build"
if ($Clean -and (Test-Path $buildDir)) {
    Write-Info "Cleaning existing build directory..."
    Remove-Item -Recurse -Force $buildDir
    Write-Success "Build directory cleaned"
}

# Create build directory
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

# Configure CMake (matching CI configuration)
Write-Info "Configuring CMake..."
Write-Info "Build Type: $BuildType"
Write-Info "Generator: Visual Studio 17 2022"
Write-Info "Architecture: x64"
Write-Info "Toolchain: $toolchainFile"

$cmakeArgs = @(
    "-G", "Visual Studio 17 2022",
    "-A", "x64",
    "-DCMAKE_BUILD_TYPE=$BuildType",
    "-DCMAKE_TOOLCHAIN_FILE=$toolchainFile",
    "-B", $buildDir
)

& cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) {
    Write-Failure "CMake configuration failed"
    exit 1
}

Write-Success "CMake configuration completed"

# Build
Write-Info "Building project ($BuildType configuration)..."
& cmake --build $buildDir --config $BuildType
if ($LASTEXITCODE -ne 0) {
    Write-Failure "Build failed"
    exit 1
}

Write-Success "Build completed successfully"

# Check for output artifacts
Write-Info "Checking build artifacts..."
$artifactPaths = @(
    "build\bin\$BuildType",
    "build\lib\$BuildType"
)

foreach ($path in $artifactPaths) {
    if (Test-Path $path) {
        Write-Success "Found: $path"
        Get-ChildItem $path -File | ForEach-Object {
            Write-Host "  - $($_.Name) ($([math]::Round($_.Length / 1KB, 2)) KB)" -ForegroundColor Gray
        }
    }
}

# Summary
Write-Host ""
Write-Host "=======================================================" -ForegroundColor Green
Write-Host "  Build Successful!" -ForegroundColor Green
Write-Host "=======================================================" -ForegroundColor Green
Write-Host ""
Write-Info "Build configuration: $BuildType"
Write-Info "Build directory: $buildDir"
Write-Info "Executables: $buildDir\bin\$BuildType\"
Write-Info "Libraries: $buildDir\lib\$BuildType\"
Write-Host ""
Write-Info "Next steps:"
Write-Host "  1. Run tests: ctest -C $BuildType --output-on-failure" -ForegroundColor Gray
Write-Host "  2. Run simulator: .\build\bin\$BuildType\painlessmesh-simulator.exe --help" -ForegroundColor Gray
Write-Host "  3. Open in VS: .\build\painlessmesh-simulator.sln" -ForegroundColor Gray
Write-Host ""

exit 0
