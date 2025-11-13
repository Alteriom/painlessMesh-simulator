#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Verifies that all prerequisites for Windows CI-style builds are installed.

.DESCRIPTION
    Checks for:
    - Visual Studio 2022
    - CMake
    - Git
    - vcpkg (optional, can be installed by build script)

.EXAMPLE
    .\scripts\verify-windows-prereqs.ps1
#>

$ErrorActionPreference = "Continue"

function Write-Check { 
    param([string]$Message)
    Write-Host "> Checking $Message..." -ForegroundColor Cyan 
}

function Write-Pass { 
    param([string]$Message)
    Write-Host "  [OK] $Message" -ForegroundColor Green 
}

function Write-Fail { 
    param([string]$Message)
    Write-Host "  [FAIL] $Message" -ForegroundColor Red 
}

function Write-Warn { 
    param([string]$Message)
    Write-Host "  [WARN] $Message" -ForegroundColor Yellow 
}

Write-Host ""
Write-Host "=======================================================" -ForegroundColor Magenta
Write-Host "  Windows Build Prerequisites Check" -ForegroundColor Magenta
Write-Host "=======================================================" -ForegroundColor Magenta
Write-Host ""

$allGood = $true

# Check Visual Studio 2022
Write-Check "Visual Studio 2022"
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vsWhere) {
    $vsPath = & $vsWhere -version "[17.0,18.0)" -property installationPath -latest 2>$null
    if ($vsPath) {
        Write-Pass "Found at: $vsPath"
        
        # Check for C++ workload
        $vcvars = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
        if (Test-Path $vcvars) {
            Write-Pass "C++ build tools are installed"
        } else {
            Write-Warn "C++ build tools may not be installed"
            Write-Host "    Install via: VS Installer > Desktop development with C++" -ForegroundColor Gray
            $allGood = $false
        }
    } else {
        Write-Fail "Visual Studio 2022 not found"
        Write-Host "    Download: https://visualstudio.microsoft.com/downloads/" -ForegroundColor Gray
        $allGood = $false
    }
} else {
    Write-Fail "vswhere.exe not found (Visual Studio not installed)"
    Write-Host "    Download: https://visualstudio.microsoft.com/downloads/" -ForegroundColor Gray
    $allGood = $false
}

# Check CMake
Write-Check "CMake"
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmake) {
    $cmakeVersion = & cmake --version 2>$null | Select-Object -First 1
    Write-Pass "Found: $($cmake.Source)"
    Write-Pass "Version: $cmakeVersion"
} else {
    # Check in common Visual Studio locations
    $vsCMakePaths = @(
        "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Professional\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
    )
    
    $found = $false
    foreach ($path in $vsCMakePaths) {
        if (Test-Path $path) {
            Write-Pass "Found in Visual Studio: $path"
            Write-Warn "CMake not in PATH. Consider adding it or running from Developer Command Prompt"
            $found = $true
            break
        }
    }
    
    if (-not $found) {
        Write-Fail "CMake not found"
        Write-Host "    Install via:" -ForegroundColor Gray
        Write-Host "    - Visual Studio Installer > Individual components > C++ CMake tools" -ForegroundColor Gray
        Write-Host "    - Or download from: https://cmake.org/download/" -ForegroundColor Gray
        Write-Host "    - Or use Chocolatey: choco install cmake" -ForegroundColor Gray
        $allGood = $false
    }
}

# Check Git
Write-Check "Git"
$git = Get-Command git -ErrorAction SilentlyContinue
if ($git) {
    $gitVersion = & git --version 2>$null
    Write-Pass "Found: $($git.Source)"
    Write-Pass "Version: $gitVersion"
} else {
    Write-Fail "Git not found"
    Write-Host "    Download: https://git-scm.com/download/win" -ForegroundColor Gray
    $allGood = $false
}

# Check vcpkg (optional)
Write-Check "vcpkg (optional)"
$vcpkgPaths = @(
    "C:\vcpkg\vcpkg.exe",
    "$env:VCPKG_ROOT\vcpkg.exe"
)

$vcpkgFound = $false
foreach ($path in $vcpkgPaths) {
    if ($path -and (Test-Path $path)) {
        Write-Pass "Found at: $path"
        $vcpkgFound = $true
        
        # Check if packages are installed
        $installed = & $path list 2>$null
        if ($installed -match "boost-system:x64-windows") {
            Write-Pass "boost-system:x64-windows is installed"
        } else {
            Write-Warn "boost-system:x64-windows not installed"
        }
        if ($installed -match "boost-asio:x64-windows") {
            Write-Pass "boost-asio:x64-windows is installed"
        } else {
            Write-Warn "boost-asio:x64-windows not installed"
        }
        if ($installed -match "yaml-cpp:x64-windows") {
            Write-Pass "yaml-cpp:x64-windows is installed"
        } else {
            Write-Warn "yaml-cpp:x64-windows not installed"
        }
        break
    }
}

if (-not $vcpkgFound) {
    Write-Warn "Not found (will be installed by build script)"
    Write-Host "    Or install manually: git clone https://github.com/microsoft/vcpkg.git C:\vcpkg" -ForegroundColor Gray
}

# Check PowerShell version
Write-Check "PowerShell version"
$psVersion = $PSVersionTable.PSVersion
if ($psVersion.Major -ge 5) {
    Write-Pass "Version $psVersion (compatible)"
} else {
    Write-Warn "Version $psVersion (PowerShell 5+ recommended)"
}

# Summary
Write-Host ""
Write-Host "=======================================================" -ForegroundColor Magenta

if ($allGood) {
    Write-Host "  All required prerequisites are installed!" -ForegroundColor Green
    Write-Host ""
    Write-Host "  You can now run:" -ForegroundColor Cyan
    Write-Host "    .\scripts\build-windows-ci.ps1" -ForegroundColor Yellow
} else {
    Write-Host "  Some prerequisites are missing" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "  Please install missing components and run this script again" -ForegroundColor Cyan
    Write-Host "  Or run the build script which will attempt to install vcpkg:" -ForegroundColor Cyan
    Write-Host "    .\scripts\build-windows-ci.ps1" -ForegroundColor Yellow
}

Write-Host "=======================================================" -ForegroundColor Magenta
Write-Host ""

if ($allGood) {
    exit 0
} else {
    exit 1
}
