# Docker Build Environment Guide

This guide explains how to use Docker for building and testing the painlessMesh Simulator in a reproducible, controlled environment.

## Table of Contents

- [Why Docker?](#why-docker)
- [Quick Start](#quick-start)
- [Available Environments](#available-environments)
- [Building with Docker](#building-with-docker)
- [CI/CD Integration](#cicd-integration)
- [Troubleshooting](#troubleshooting)
- [Advanced Usage](#advanced-usage)

## Why Docker?

### Problems Solved

1. **Dependency Management**: No more "works on my machine" issues
2. **Consistent Builds**: Same environment for CI and local development
3. **Multi-Platform Support**: Build for Windows from Linux/Mac
4. **Version Control**: Dependencies are versioned in Dockerfiles
5. **Isolation**: No conflicts with system packages
6. **Agent-Friendly**: Easy for AI agents to use during sessions

### Traditional vs Docker Approach

#### Traditional Approach
```bash
# Install dependencies (varies by OS)
sudo apt-get install cmake ninja-build libboost-dev ...  # Ubuntu
brew install cmake ninja boost ...                       # macOS
choco install cmake ninja boost ...                      # Windows

# Version conflicts?
# Missing dependencies?
# Different compiler versions?
```

#### Docker Approach
```bash
# One command, works everywhere
docker run --rm -v $PWD:/workspace -w /workspace \
  painlessmesh-simulator:linux-build \
  bash -c "cmake -B build && cmake --build build"
```

## Quick Start

### Prerequisites

Install Docker:
- **Linux**: `sudo apt-get install docker.io docker-compose`
- **macOS**: [Docker Desktop for Mac](https://docs.docker.com/desktop/install/mac-install/)
- **Windows**: [Docker Desktop for Windows](https://docs.docker.com/desktop/install/windows-install/)

### Build Everything

```bash
# Clone repository
git clone --recursive https://github.com/Alteriom/painlessMesh-simulator.git
cd painlessMesh-simulator

# Build for Linux
./docker/build-linux.sh

# Cross-compile for Windows
./docker/build-windows.sh

# Run tests
./docker/test.sh
```

That's it! No dependencies to install, everything is in the container.

## Available Environments

### 1. Linux Build Environment

**Image**: `painlessmesh-simulator:linux-build`  
**File**: `docker/Dockerfile.linux-build`

**Includes**:
- GCC and Clang compilers
- CMake, Ninja
- Boost, yaml-cpp, ncurses
- clang-format, cppcheck
- Coverage tools (lcov, gcovr)

**Use for**:
- Production builds
- CI/CD pipelines
- Quick compilation checks

### 2. Windows Cross-Compilation

**Image**: `painlessmesh-simulator:windows-cross`  
**File**: `docker/Dockerfile.windows-cross`

**Includes**:
- MinGW-w64 cross-compiler
- vcpkg package manager
- Windows libraries (Boost, etc.)
- CMake with Windows toolchain

**Use for**:
- Building Windows binaries from Linux
- CI/CD Windows builds
- Testing Windows compatibility

### 3. Development Environment

**Image**: `painlessmesh-simulator:dev`  
**File**: `docker/Dockerfile.dev`

**Includes**:
- All build tools
- Debuggers (GDB, Valgrind)
- Documentation tools (Doxygen)
- Python with analysis libraries
- Text editors (vim, nano)

**Use for**:
- Interactive development
- Debugging
- Documentation generation
- Experimentation

## Building with Docker

### Using Helper Scripts (Recommended)

#### Linux Build

```bash
# Release build with GCC (default)
./docker/build-linux.sh

# Debug build with GCC
./docker/build-linux.sh Debug

# Release build with Clang
./docker/build-linux.sh Release clang

# Debug build with Clang
./docker/build-linux.sh Debug clang
```

#### Windows Build

```bash
# Release build (default)
./docker/build-windows.sh

# Debug build
./docker/build-windows.sh Debug
```

#### Running Tests

```bash
# Run all tests
./docker/test.sh

# Run tests with Debug build
./docker/test.sh Debug
```

### Using docker-compose

```bash
# Build all Docker images
docker-compose build

# Run Linux build
docker-compose run --rm linux-build bash -c "cmake -B build && cmake --build build"

# Run Windows cross-build
docker-compose run --rm windows-cross bash -c "cmake -B build-win && cmake --build build-win"

# Interactive development
docker-compose run --rm dev
```

### Manual Docker Commands

#### Build Docker Image

```bash
docker build -t painlessmesh-simulator:linux-build -f docker/Dockerfile.linux-build .
```

#### Run Build

```bash
# Configure
docker run --rm -v $PWD:/workspace -w /workspace \
  painlessmesh-simulator:linux-build \
  cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -B build

# Build
docker run --rm -v $PWD:/workspace -w /workspace \
  painlessmesh-simulator:linux-build \
  cmake --build build
```

#### Run Tests

```bash
docker run --rm -v $PWD:/workspace -w /workspace/build \
  painlessmesh-simulator:linux-build \
  ctest --output-on-failure
```

### Interactive Development

```bash
# Start development container
docker-compose run --rm dev

# Inside container, you can:
cmake -G Ninja -B build
cmake --build build
cd build && ctest
gdb ./bin/simulator_tests
```

## CI/CD Integration

### GitHub Actions

The project includes two CI workflows:

1. **ci.yml**: Traditional CI (direct dependency installation)
2. **ci-docker.yml**: Docker-based CI (recommended)

#### Enabling Docker CI

The Docker CI workflow (`.github/workflows/ci-docker.yml`) provides:

- ✅ Reproducible builds
- ✅ Faster builds with layer caching
- ✅ Consistent environment across all jobs
- ✅ Easy to debug (can run locally)

#### CI Workflow Features

**Build Matrix**:
- Linux: GCC/Clang × Debug/Release
- Windows: MinGW cross-compilation
- macOS: Native build (no Docker)

**Quality Checks**:
- Code formatting (clang-format)
- Static analysis (cppcheck)
- Unit tests with coverage
- Integration tests
- Performance benchmarks (scheduled)

**Caching**:
- Docker layer caching via GitHub Cache
- Build artifact caching
- Significant speedup on subsequent runs

### Local CI Testing

Run CI jobs locally before pushing:

```bash
# Lint check
docker run --rm -v $PWD:/workspace -w /workspace \
  painlessmesh-simulator:linux-build \
  clang-format --dry-run --Werror src/**/*.cpp

# Build (same as CI)
./docker/build-linux.sh Release gcc

# Test (same as CI)
./docker/test.sh Debug
```

## Troubleshooting

### Docker Permission Denied

**Problem**: `Got permission denied while trying to connect to the Docker daemon socket`

**Solution**:
```bash
# Add user to docker group (Linux)
sudo usermod -aG docker $USER
# Log out and back in

# Or use sudo
sudo docker build ...
```

### Build Fails: Submodules Not Initialized

**Problem**: `painlessMesh submodule not found`

**Solution**:
```bash
# Initialize submodules
git submodule update --init --recursive

# Then rebuild
./docker/build-linux.sh
```

### Out of Disk Space

**Problem**: Docker using too much disk space

**Solution**:
```bash
# Clean up old containers and images
docker system prune -a

# Check disk usage
docker system df
```

### Build is Slow

**Problem**: First build takes a long time

**Solution**:
- First build downloads and installs dependencies (normal)
- Subsequent builds are faster due to layer caching
- Use `DOCKER_BUILDKIT=1` for parallel builds:
  ```bash
  DOCKER_BUILDKIT=1 docker build -t painlessmesh-simulator:linux-build -f docker/Dockerfile.linux-build .
  ```

### Windows Cross-Compilation Fails

**Problem**: MinGW or vcpkg errors

**Solution**:
1. Rebuild Docker image: `docker-compose build windows-cross`
2. Check vcpkg logs in container
3. Verify CMake toolchain file path: `/opt/vcpkg/scripts/buildsystems/vcpkg.cmake`

### Container Can't Access Files

**Problem**: Permission errors inside container

**Solution**:
```bash
# Run as current user
docker run --rm --user $(id -u):$(id -g) \
  -v $PWD:/workspace -w /workspace \
  painlessmesh-simulator:linux-build \
  cmake --build build
```

### CMake Configuration Fails

**Problem**: Boost or other libraries not found

**Solution**:
1. Verify Docker image built correctly:
   ```bash
   docker run --rm painlessmesh-simulator:linux-build dpkg -l | grep boost
   ```
2. Check CMakeLists.txt for version requirements
3. Update Dockerfile if needed

## Advanced Usage

### Custom Build Options

```bash
# Enable benchmarks
docker run --rm -v $PWD:/workspace -w /workspace \
  painlessmesh-simulator:linux-build \
  cmake -G Ninja -DENABLE_BENCHMARKS=ON -B build

# Enable coverage
docker run --rm -v $PWD:/workspace -w /workspace \
  painlessmesh-simulator:linux-build \
  cmake -G Ninja -DENABLE_COVERAGE=ON -DCMAKE_BUILD_TYPE=Debug -B build
```

### Debugging in Container

```bash
# Start dev container
docker-compose run --rm dev

# Inside container
cd /workspace
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -B build
cmake --build build
gdb --args ./build/bin/simulator_tests
```

### Custom Docker Images

Modify Dockerfiles to add tools or change versions:

```dockerfile
# In docker/Dockerfile.linux-build
RUN apt-get install -y \
    libboost-dev \
    my-custom-library  # Add this
```

Rebuild:
```bash
docker build -t painlessmesh-simulator:linux-build -f docker/Dockerfile.linux-build .
```

### Persistent Build Cache

Use named volumes to preserve builds:

```yaml
# docker-compose.yml
volumes:
  - .:/workspace
  - build-cache:/workspace/build  # Persists between runs
```

### Multi-Stage Builds

For smaller production images:

```dockerfile
# Build stage
FROM painlessmesh-simulator:linux-build AS builder
WORKDIR /workspace
COPY . .
RUN cmake -B build && cmake --build build

# Runtime stage
FROM ubuntu:22.04
COPY --from=builder /workspace/build/bin/* /usr/local/bin/
CMD ["painlessmesh-simulator"]
```

### Running Benchmarks

```bash
# Build with benchmarks enabled
docker run --rm -v $PWD:/workspace -w /workspace \
  painlessmesh-simulator:linux-build \
  bash -c "cmake -G Ninja -DENABLE_BENCHMARKS=ON -DCMAKE_BUILD_TYPE=Release -B build && \
           cmake --build build"

# Run benchmarks
docker run --rm -v $PWD:/workspace -w /workspace/build \
  painlessmesh-simulator:linux-build \
  ./bin/benchmarks
```

## Performance Comparison

### Build Times (Approximate)

| Method | First Build | Subsequent Builds |
|--------|-------------|-------------------|
| Native | 5-10 min | 30-60 sec |
| Docker (no cache) | 10-15 min | 30-60 sec |
| Docker (with cache) | 2-3 min | 30-60 sec |

### Disk Space

| Component | Size |
|-----------|------|
| linux-build image | ~800 MB |
| windows-cross image | ~2 GB |
| dev image | ~1 GB |
| Build artifacts | ~100-200 MB |

## Best Practices

### For Developers

1. **Use helper scripts**: `./docker/build-linux.sh` is easier than manual commands
2. **Test locally**: Run Docker builds before pushing to CI
3. **Clean regularly**: `docker system prune` to free disk space
4. **Use dev image**: For interactive work, use `docker-compose run dev`

### For CI/CD

1. **Enable caching**: Use GitHub Actions cache for Docker layers
2. **Matrix builds**: Test multiple compilers and build types
3. **Artifact upload**: Save build outputs for debugging
4. **Continue-on-error**: Don't fail entire CI for optional checks

### For Agents

1. **Use scripts**: Helper scripts are simpler than raw Docker commands
2. **Check logs**: Container output shows build progress
3. **Clean state**: Each Docker run is a fresh environment
4. **Mount workspace**: Always mount project directory as volume

## Docker vs Native Comparison

| Aspect | Native Build | Docker Build |
|--------|--------------|--------------|
| Setup Time | 30-60 min (install deps) | 10-15 min (first image build) |
| Build Speed | Fastest | Slightly slower |
| Consistency | Varies by system | Always identical |
| Multi-platform | Difficult | Easy |
| Isolation | No | Yes |
| CI/CD | Complex | Simple |
| Agent-friendly | No | Yes |

## Conclusion

Docker provides a controlled, reproducible build environment that works consistently across all platforms and is ideal for CI/CD and agent-assisted development.

**Recommended Workflow**:
1. Use Docker for CI/CD (`.github/workflows/ci-docker.yml`)
2. Use Docker for multi-platform builds
3. Use native builds for day-to-day development (faster)
4. Use Docker dev image for debugging or experimentation

## References

- [Docker Documentation](https://docs.docker.com/)
- [docker-compose Documentation](https://docs.docker.com/compose/)
- [GitHub Actions with Docker](https://docs.github.com/en/actions/creating-actions/creating-a-docker-container-action)
- [CMake Cross-Compilation](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html)

## Support

For Docker-related issues:
1. Check this guide first
2. Review `docker/README.md` for technical details
3. Check Docker logs: `docker logs <container-id>`
4. Open an issue with Docker version and error output
