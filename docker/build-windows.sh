#!/bin/bash
# Cross-compile painlessMesh Simulator for Windows using Docker

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== painlessMesh Simulator - Windows Cross-Build ===${NC}"

# Parse arguments
BUILD_TYPE="${1:-Release}"

echo -e "${GREEN}Configuration:${NC}"
echo "  Build Type: ${BUILD_TYPE}"
echo "  Target: Windows x64"

# Build Docker image if needed
echo -e "${BLUE}Building Docker image...${NC}"
docker build -t painlessmesh-simulator:windows-cross -f "${SCRIPT_DIR}/Dockerfile.windows-cross" "${PROJECT_ROOT}"

# Create build directory
BUILD_DIR="${PROJECT_ROOT}/build/docker-windows-${BUILD_TYPE}"
mkdir -p "${BUILD_DIR}"

echo -e "${BLUE}Configuring CMake for Windows cross-compilation...${NC}"
docker run --rm \
    -v "${PROJECT_ROOT}:/workspace" \
    -w /workspace \
    painlessmesh-simulator:windows-cross \
    cmake -G Ninja \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -DCMAKE_SYSTEM_NAME=Windows \
        -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
        -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
        -B "build/docker-windows-${BUILD_TYPE}"

echo -e "${BLUE}Building...${NC}"
docker run --rm \
    -v "${PROJECT_ROOT}:/workspace" \
    -w /workspace \
    painlessmesh-simulator:windows-cross \
    cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}"

echo -e "${GREEN}Build complete!${NC}"
echo "  Binaries: ${BUILD_DIR}/bin/"
echo "  Libraries: ${BUILD_DIR}/lib/"
echo ""
echo -e "${BLUE}Note:${NC} Test executables (.exe) can be run with Wine:"
echo "  wine ${BUILD_DIR}/bin/simulator_tests.exe"
