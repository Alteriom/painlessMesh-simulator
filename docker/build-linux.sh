#!/bin/bash
# Build painlessMesh Simulator for Linux using Docker

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== painlessMesh Simulator - Linux Build ===${NC}"

# Parse arguments
BUILD_TYPE="${1:-Release}"
COMPILER="${2:-gcc}"

echo -e "${GREEN}Configuration:${NC}"
echo "  Build Type: ${BUILD_TYPE}"
echo "  Compiler: ${COMPILER}"

# Build Docker image if needed
echo -e "${BLUE}Building Docker image...${NC}"
docker build -t painlessmesh-simulator:linux-build -f "${SCRIPT_DIR}/Dockerfile.linux-build" "${PROJECT_ROOT}"

# Create build directory
BUILD_DIR="${PROJECT_ROOT}/build/docker-linux-${COMPILER}-${BUILD_TYPE}"
mkdir -p "${BUILD_DIR}"

# Set compiler
if [ "${COMPILER}" == "clang" ]; then
    CMAKE_C_COMPILER="clang"
    CMAKE_CXX_COMPILER="clang++"
else
    CMAKE_C_COMPILER="gcc"
    CMAKE_CXX_COMPILER="g++"
fi

echo -e "${BLUE}Configuring CMake...${NC}"
docker run --rm \
    -v "${PROJECT_ROOT}:/workspace" \
    -w /workspace \
    painlessmesh-simulator:linux-build \
    cmake -G Ninja \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -DCMAKE_C_COMPILER="${CMAKE_C_COMPILER}" \
        -DCMAKE_CXX_COMPILER="${CMAKE_CXX_COMPILER}" \
        -B "${BUILD_DIR}"

echo -e "${BLUE}Building...${NC}"
docker run --rm \
    -v "${PROJECT_ROOT}:/workspace" \
    -w /workspace \
    painlessmesh-simulator:linux-build \
    cmake --build "${BUILD_DIR}" --config "${BUILD_TYPE}"

echo -e "${GREEN}Build complete!${NC}"
echo "  Binaries: ${BUILD_DIR}/bin/"
echo "  Libraries: ${BUILD_DIR}/lib/"
