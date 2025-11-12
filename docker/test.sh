#!/bin/bash
# Run tests using Docker

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}=== painlessMesh Simulator - Run Tests ===${NC}"

# Parse arguments
BUILD_TYPE="${1:-Debug}"

echo -e "${GREEN}Configuration:${NC}"
echo "  Build Type: ${BUILD_TYPE}"

# First build if needed
if [ ! -d "${PROJECT_ROOT}/build/docker-linux-gcc-${BUILD_TYPE}" ]; then
    echo -e "${BLUE}Build directory not found, building first...${NC}"
    "${SCRIPT_DIR}/build-linux.sh" "${BUILD_TYPE}" gcc
fi

BUILD_DIR="${PROJECT_ROOT}/build/docker-linux-gcc-${BUILD_TYPE}"

echo -e "${BLUE}Running tests...${NC}"
docker run --rm \
    -v "${PROJECT_ROOT}:/workspace" \
    -w /workspace \
    painlessmesh-simulator:linux-build \
    bash -c "cd ${BUILD_DIR} && ctest --output-on-failure --verbose"

echo -e "${GREEN}Tests complete!${NC}"
