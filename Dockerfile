# painlessMesh Simulator - Multi-stage Docker Build
# 
# This Dockerfile creates a minimal image for building and running
# the painlessMesh simulator with all dependencies included.

# Build stage
FROM ubuntu:22.04 AS builder

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    libboost-system-dev \
    libboost-program-options-dev \
    libboost-date-time-dev \
    libyaml-cpp-dev \
    catch2 \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source files
COPY . /build/

# Initialize submodules if not already done
RUN git submodule update --init --recursive || true

# Create build directory and build
RUN mkdir -p build && cd build && \
    cmake -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    .. && \
    ninja && \
    ninja test || true

# Runtime stage - minimal image with only runtime dependencies
FROM ubuntu:22.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
    libboost-system1.74.0 \
    libboost-program-options1.74.0 \
    libboost-date-time1.74.0 \
    libyaml-cpp0.7 \
    && rm -rf /var/lib/apt/lists/*

# Copy built executables from builder
COPY --from=builder /build/build/simulator_tests /usr/local/bin/
COPY --from=builder /build/build/painlessmesh-simulator /usr/local/bin/ || true

# Copy example scenarios and configurations
COPY --from=builder /build/examples /opt/simulator/examples

# Set working directory
WORKDIR /opt/simulator

# Default command - show help
CMD ["painlessmesh-simulator", "--help"]

# Development stage - includes build tools for iterative development
FROM builder AS development

# Install additional development tools
RUN apt-get update && apt-get install -y \
    gdb \
    valgrind \
    vim \
    nano \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

# Mount point for source code
VOLUME ["/workspace"]

CMD ["/bin/bash"]
