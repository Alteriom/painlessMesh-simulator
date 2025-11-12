# Docker Build Environment

This directory contains Docker-based build environments for the painlessMesh Simulator, providing reproducible builds across different platforms and consistent environments for both CI/CD and local development.

## Overview

### Available Containers

1. **linux-build**: Linux build environment (Ubuntu 22.04)
   - GCC and Clang compilers
   - All project dependencies pre-installed
   - Suitable for CI/CD and production builds

2. **windows-cross**: Windows cross-compilation environment
   - MinGW-w64 cross-compiler
   - vcpkg for Windows dependencies
   - Cross-compile Windows binaries from Linux

3. **dev**: Complete development environment
   - All build tools plus debugging utilities
   - GDB, Valgrind, Doxygen, etc.
   - Suitable for interactive development sessions

## Quick Start

### Prerequisites

- Docker 20.10+ or Docker Desktop
- docker-compose (optional, for easier management)

### Using Helper Scripts

The simplest way to build is using the provided scripts:

```bash
# Build for Linux (Release, GCC)
./docker/build-linux.sh

# Build for Linux (Debug, Clang)
./docker/build-linux.sh Debug clang

# Cross-compile for Windows
./docker/build-windows.sh

# Run tests
./docker/test.sh
```

### Using docker-compose

```bash
# Build all images
docker-compose build

# Run Linux build
docker-compose run --rm linux-build bash

# Interactive development environment
docker-compose run --rm dev
```

## Detailed Usage

### Building Docker Images

Build individual images:

```bash
# Linux build environment
docker build -t painlessmesh-simulator:linux-build -f docker/Dockerfile.linux-build .

# Windows cross-compilation
docker build -t painlessmesh-simulator:windows-cross -f docker/Dockerfile.windows-cross .

# Development environment
docker build -t painlessmesh-simulator:dev -f docker/Dockerfile.dev .
```

### Running Builds

#### Linux Build

```bash
# Configure
docker run --rm -v $(pwd):/workspace -w /workspace \
  painlessmesh-simulator:linux-build \
  cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -B build

# Build
docker run --rm -v $(pwd):/workspace -w /workspace \
  painlessmesh-simulator:linux-build \
  cmake --build build --config Release
```

#### Windows Cross-Compilation

```bash
# Configure for Windows
docker run --rm -v $(pwd):/workspace -w /workspace \
  painlessmesh-simulator:windows-cross \
  cmake -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_TARGET_TRIPLET=x64-mingw-static \
    -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
    -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
    -B build-windows

# Build
docker run --rm -v $(pwd):/workspace -w /workspace \
  painlessmesh-simulator:windows-cross \
  cmake --build build-windows --config Release
```

### Running Tests

```bash
# Run tests in Docker
docker run --rm -v $(pwd):/workspace -w /workspace/build \
  painlessmesh-simulator:linux-build \
  ctest --output-on-failure --verbose
```

### Interactive Development

```bash
# Start interactive development environment
docker-compose run --rm dev

# Inside container:
# - Edit code with vim/nano
# - Build: cmake -G Ninja -B build && cmake --build build
# - Test: cd build && ctest
# - Debug: gdb ./bin/simulator_tests
```

## CI/CD Integration

### GitHub Actions

The CI/CD workflow can use these Docker images for consistent builds:

```yaml
jobs:
  build-linux:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive
      
      - name: Build Docker image
        run: docker build -t painlessmesh-simulator:linux-build -f docker/Dockerfile.linux-build .
      
      - name: Configure and build
        run: |
          docker run --rm -v $PWD:/workspace -w /workspace \
            painlessmesh-simulator:linux-build \
            bash -c "cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -B build && cmake --build build"
      
      - name: Run tests
        run: |
          docker run --rm -v $PWD:/workspace -w /workspace/build \
            painlessmesh-simulator:linux-build \
            ctest --output-on-failure
```

## Advantages

### For Development

- **Reproducibility**: Same environment every time
- **No local dependencies**: Everything in container
- **Multi-platform**: Build for Windows from Linux/Mac
- **Isolation**: No conflicts with system packages

### For CI/CD

- **Consistency**: Same environment as local builds
- **Speed**: Docker layer caching speeds up builds
- **Reliability**: Known-good configuration
- **Portability**: Works on any CI system with Docker

### For Agents

- **Easy setup**: Single command to build
- **Pre-configured**: All tools ready to use
- **Predictable**: No surprises with dependencies
- **Clean state**: Fresh environment each run

## Customization

### Modifying Dependencies

Edit the appropriate Dockerfile to add/remove packages:

```dockerfile
# Add a new library
RUN apt-get update && apt-get install -y \
    libsomenewlib-dev
```

### Changing Compiler Versions

```dockerfile
# Use specific GCC version
RUN apt-get install -y gcc-11 g++-11
ENV CC=gcc-11
ENV CXX=g++-11
```

### Adding Python Tools

```dockerfile
# Development environment
RUN pip3 install --no-cache-dir \
    conan \
    cmake-format
```

## Troubleshooting

### Docker permission denied

```bash
# Add user to docker group (Linux)
sudo usermod -aG docker $USER
# Log out and back in

# Or use sudo
sudo docker build ...
```

### Out of disk space

```bash
# Clean up old images and containers
docker system prune -a

# Remove specific image
docker rmi painlessmesh-simulator:linux-build
```

### Build fails in Docker but works locally

- Check that submodules are initialized: `git submodule update --init --recursive`
- Verify Docker has enough resources (RAM, CPU)
- Check for host-specific paths in CMakeLists.txt

### Windows cross-compilation errors

- Ensure vcpkg packages are installed correctly
- Check CMake finds the right toolchain file
- Verify MinGW-w64 paths are correct

## Performance Tips

### Faster Builds

```bash
# Use BuildKit for parallel builds
DOCKER_BUILDKIT=1 docker build -t painlessmesh-simulator:linux-build -f docker/Dockerfile.linux-build .

# Cache builds between runs
docker-compose up --build
```

### Persistent Build Cache

Use named volumes to preserve build artifacts:

```yaml
volumes:
  - .:/workspace
  - build-cache:/workspace/build  # Reused between runs
```

## Structure

```
docker/
├── Dockerfile.linux-build      # Linux build environment
├── Dockerfile.windows-cross    # Windows cross-compilation
├── Dockerfile.dev             # Development environment
├── build-linux.sh             # Linux build helper script
├── build-windows.sh           # Windows build helper script
├── test.sh                    # Test runner script
└── README.md                  # This file

../docker-compose.yml          # Orchestration configuration
```

## Version Information

- **Base Image**: Ubuntu 22.04 LTS
- **GCC**: 11.x (system default)
- **Clang**: 14.x (system default)
- **CMake**: 3.22+ (system package)
- **Boost**: 1.74+ (system package)
- **MinGW-w64**: Latest (for Windows cross-compilation)

## Further Reading

- [Docker Documentation](https://docs.docker.com/)
- [docker-compose Documentation](https://docs.docker.com/compose/)
- [CMake Cross-Compilation](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html)
- [vcpkg Documentation](https://vcpkg.io/)

## Support

For issues with Docker builds:
1. Check this README
2. Review build scripts for examples
3. Open an issue with Docker version and error output
4. Include output of `docker version` and `docker info`
