# Docker Guide for painlessMesh Simulator

This guide covers using Docker to build and run the painlessMesh simulator without installing dependencies on your host system.

## Quick Start

### Build and Run a Simulation

```bash
# Build the Docker image
docker-compose build simulator

# Run an example simulation
docker-compose run --rm simulator painlessmesh-simulator \
  --config examples/scenarios/simple_mesh.yaml
```

### Interactive Development

```bash
# Start development container with source mounted
docker-compose run --rm dev

# Inside container:
mkdir -p build && cd build
cmake -G Ninja ..
ninja
./simulator_tests
```

## Docker Images

The project provides three Docker image targets:

### 1. Runtime Image (Minimal)

**Purpose**: Run simulations with minimal overhead  
**Size**: ~200 MB  
**Includes**: Only runtime libraries and executables

```bash
# Build
docker build --target runtime -t painlessmesh-simulator:runtime .

# Run
docker run --rm -v $(pwd)/results:/opt/simulator/results \
  painlessmesh-simulator:runtime \
  painlessmesh-simulator --config examples/scenarios/simple_mesh.yaml
```

### 2. Builder Image

**Purpose**: Build the simulator  
**Size**: ~800 MB  
**Includes**: All build tools and dependencies

```bash
# Build
docker build --target builder -t painlessmesh-simulator:builder .

# Use for building
docker-compose run --rm build
```

### 3. Development Image

**Purpose**: Interactive development and debugging  
**Size**: ~900 MB  
**Includes**: Build tools + debugger + editors

```bash
# Start interactive session
docker-compose run --rm dev

# Inside container you get:
# - Full build environment
# - GDB for debugging
# - Valgrind for memory analysis
# - vim/nano for quick edits
```

## Usage Patterns

### Build the Simulator

```bash
# Using docker-compose (recommended)
docker-compose run --rm build

# Using plain Docker
docker build --target builder -t painlessmesh-simulator:builder .
docker run --rm -v $(pwd):/build painlessmesh-simulator:builder \
  bash -c "cd build && cmake -G Ninja .. && ninja"
```

### Run Tests

```bash
# Using docker-compose
docker-compose run --rm test

# Run specific test
docker-compose run --rm test ./simulator_tests "VirtualNode*"
```

### Run a Simulation

```bash
# Simple mesh with default settings
docker-compose run --rm simulator

# Custom scenario
docker-compose run --rm simulator painlessmesh-simulator \
  --config examples/scenarios/stress_test.yaml \
  --duration 120 \
  --time-scale 5.0

# With results output
docker run --rm \
  -v $(pwd)/examples:/opt/simulator/examples:ro \
  -v $(pwd)/results:/opt/simulator/results \
  painlessmesh-simulator:runtime \
  painlessmesh-simulator \
  --config examples/scenarios/simple_mesh.yaml \
  --output results/
```

### Interactive Development

```bash
# Start dev container
docker-compose run --rm dev

# Inside container:
cd /workspace
mkdir -p build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ..
ninja

# Run with debugger
gdb --args ./simulator_tests

# Check for memory leaks
valgrind --leak-check=full ./simulator_tests
```

## Volume Mounts

The docker-compose.yml configures several volume mounts:

### Development Container
- `./` → `/workspace` - Full source code access
- `build-cache` - Persistent build artifacts (faster rebuilds)

### Runtime Container
- `./examples` → `/opt/simulator/examples` (read-only)
- `./results` → `/opt/simulator/results` (read-write)

### Custom Firmware Testing

```bash
# Mount your firmware directory
docker run --rm \
  -v $(pwd)/my-firmware:/firmware:ro \
  -v $(pwd)/results:/opt/simulator/results \
  painlessmesh-simulator:runtime \
  painlessmesh-simulator \
  --config examples/scenarios/firmware_test.yaml \
  --firmware-path /firmware
```

## CI/CD Integration

### GitHub Actions

```yaml
name: Docker Build and Test

on: [push, pull_request]

jobs:
  docker-test:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
        with:
          submodules: recursive
      
      - name: Build Docker image
        run: docker-compose build test
      
      - name: Run tests in Docker
        run: docker-compose run --rm test
      
      - name: Build simulator
        run: docker-compose run --rm build
      
      - name: Run example simulation
        run: |
          docker-compose run --rm simulator \
            painlessmesh-simulator \
            --config examples/scenarios/simple_mesh.yaml \
            --duration 30
```

### GitLab CI

```yaml
docker-build:
  stage: build
  image: docker:latest
  services:
    - docker:dind
  script:
    - docker build --target builder -t $CI_REGISTRY_IMAGE:builder .
    - docker build --target runtime -t $CI_REGISTRY_IMAGE:latest .
    - docker push $CI_REGISTRY_IMAGE:builder
    - docker push $CI_REGISTRY_IMAGE:latest

test:
  stage: test
  image: $CI_REGISTRY_IMAGE:builder
  script:
    - mkdir build && cd build
    - cmake -G Ninja ..
    - ninja test
```

## Docker Compose Commands Reference

### Build Commands

```bash
# Build all images
docker-compose build

# Build specific service
docker-compose build dev
docker-compose build simulator

# Force rebuild (no cache)
docker-compose build --no-cache
```

### Run Commands

```bash
# Run and remove container after exit
docker-compose run --rm dev

# Run in background
docker-compose up -d simulator

# View logs
docker-compose logs -f simulator

# Stop all containers
docker-compose down
```

### Maintenance Commands

```bash
# Clean up stopped containers
docker-compose down -v

# Remove build cache volume
docker volume rm painlessmesh-simulator_build-cache

# Prune unused Docker resources
docker system prune -a
```

## Multi-Architecture Builds

Build for multiple platforms (useful for ARM-based systems like Raspberry Pi):

```bash
# Set up buildx
docker buildx create --use

# Build for multiple architectures
docker buildx build \
  --platform linux/amd64,linux/arm64 \
  --target runtime \
  -t painlessmesh-simulator:latest \
  --push \
  .
```

## Performance Considerations

### Build Cache

Docker layer caching significantly speeds up rebuilds:

```dockerfile
# Copy only dependency files first
COPY CMakeLists.txt /build/
COPY external/ /build/external/

# Install dependencies (cached layer)
RUN apt-get update && apt-get install -y ...

# Copy rest of source (changes frequently)
COPY . /build/
```

### Multi-stage Benefits

The multi-stage build provides:
- **Smaller runtime images** (~200 MB vs ~900 MB)
- **Faster deployment** (less data to transfer)
- **Better security** (no build tools in runtime)

### Volume Performance

For better I/O performance on macOS/Windows:

```yaml
volumes:
  - .:/workspace:delegated  # Better write performance
  - build-cache:/workspace/build:cached  # Better read performance
```

## Troubleshooting

### Build Fails with "submodule not found"

```bash
# Initialize submodules before building
git submodule update --init --recursive

# Or rebuild with submodule init
docker-compose build --no-cache
```

### Container Can't Access Host Files

```bash
# Check volume mounts
docker-compose config

# Verify file permissions (Linux)
ls -la examples/

# On Windows with WSL2, use WSL2 paths
docker run -v /mnt/d/Github/painlessMesh-simulator:/workspace ...
```

### "Cannot find painlessmesh-simulator executable"

This happens if the build stage failed. Check logs:

```bash
# Build with verbose output
docker-compose build --progress=plain build

# Check if executable was created
docker-compose run --rm dev ls -la /build/build/
```

### High Memory Usage

Limit Docker resource usage:

```yaml
services:
  simulator:
    deploy:
      resources:
        limits:
          cpus: '2.0'
          memory: 2G
```

## Comparing Docker vs WSL2 vs Native

| Aspect | Docker | WSL2 | Native Linux |
|--------|--------|------|--------------|
| Setup Time | 5 min | 10 min | 5 min |
| Overhead | Low (~5%) | Minimal (~2%) | None |
| Isolation | High | Medium | None |
| Portability | High | Windows only | Platform-specific |
| Build Speed | Good | Better | Best |
| CI/CD | Excellent | Good | Good |
| Recommended For | CI/CD, deployment | Windows dev | Linux dev |

## Best Practices

1. **Use docker-compose** for local development (simpler commands)
2. **Use multi-stage builds** for production (smaller images)
3. **Mount volumes** for persistent data (configs, results)
4. **Use .dockerignore** to reduce build context size
5. **Pin base image versions** for reproducibility (ubuntu:22.04, not ubuntu:latest)
6. **Clean up regularly** to free disk space (docker system prune)

## Advanced: Custom Base Image

Create a custom base image with painlessMesh pre-built:

```dockerfile
FROM ubuntu:22.04 AS painlessmesh-base

RUN apt-get update && apt-get install -y \
    libboost-dev libyaml-cpp-dev git cmake

WORKDIR /opt/painlessmesh
RUN git clone https://github.com/Alteriom/painlessMesh.git . && \
    mkdir build && cd build && \
    cmake .. && make install

# Use in simulator Dockerfile
FROM painlessmesh-base AS builder
COPY . /build
# Build simulator against pre-installed painlessMesh
```

## Resources

- **Docker Documentation**: https://docs.docker.com/
- **Docker Compose**: https://docs.docker.com/compose/
- **Multi-stage Builds**: https://docs.docker.com/build/building/multi-stage/
- **Best Practices**: https://docs.docker.com/develop/dev-best-practices/

## Support

- **Issues**: Open an issue if Docker builds fail
- **Discussions**: Share your Docker setup improvements
- **CI Examples**: See `.github/workflows/` for GitHub Actions integration

---

**Quick Reference:**

```bash
# Development
docker-compose run --rm dev

# Build
docker-compose run --rm build

# Test
docker-compose run --rm test

# Run simulation
docker-compose run --rm simulator painlessmesh-simulator \
  --config examples/scenarios/simple_mesh.yaml
```
