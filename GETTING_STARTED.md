# Getting Started with painlessMesh Simulator

## TL;DR - Quick Start

**Docker (All Platforms - Fastest Setup):**
```bash
# Build and test (no dependencies needed!)
./docker-quickstart.sh build
./docker-quickstart.sh test

# Windows PowerShell:
.\docker-quickstart.ps1 build
.\docker-quickstart.ps1 test
```

**Windows Users (WSL2):**
```powershell
# 1. Install WSL2 (PowerShell as Admin)
wsl --install -d Ubuntu-22.04

# 2. Inside WSL2:
sudo apt install -y build-essential cmake ninja-build libboost-all-dev libyaml-cpp-dev
cd /mnt/d/Github/painlessMesh-simulator
mkdir -p build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DPAINLESSMESH_PATH='/mnt/d/Github/painlessMesh' ..
ninja
```

**Linux/macOS Users:**
```bash
# Install dependencies
sudo apt install cmake ninja-build libboost-dev libyaml-cpp-dev  # Ubuntu/Debian
brew install cmake ninja boost yaml-cpp  # macOS

# Build
mkdir build && cd build
cmake -G Ninja ..
ninja
```

## What is this?

The painlessMesh Device Simulator lets you test mesh network firmware with **100+ virtual nodes** on your desktop, without any physical hardware.

## Why should I care?

- ✅ **Test at scale** - Validate with 100+ nodes instead of 3-5 physical devices
- ✅ **Fast iteration** - No flashing, no wiring, instant feedback
- ✅ **CI/CD integration** - Automated firmware testing in your pipeline
- ✅ **Reproducible scenarios** - YAML configuration for consistent testing
- ✅ **Cost effective** - Test without buying hundreds of ESP32s

## Platform Choice

### Option 1: Docker (Recommended for Quick Start)

**Best for**: Quick testing, CI/CD, no local dependency installation

```bash
# Build and test in one command
./docker-quickstart.sh build   # Linux/macOS
.\docker-quickstart.ps1 build  # Windows

# Interactive development
./docker-quickstart.sh dev
```

**Pros:**
- ✅ Zero local dependency installation
- ✅ Consistent builds across all platforms
- ✅ Perfect for CI/CD
- ✅ Isolated from host system

**→ [Docker Guide](docs/DOCKER_GUIDE.md)** - Complete Docker documentation

### Option 2: WSL2 (Recommended for Development)

**Best for**: Active development, debugging, IDE integration

The simulator builds on painlessMesh's existing Linux-based test infrastructure. Using WSL2 gives you:

- ✅ Native Linux environment on Windows
- ✅ No MSVC compatibility issues
- ✅ Matches CI/CD environment exactly
- ✅ Full VS Code integration
- ✅ 10-minute setup

**→ [WSL2 Setup Guide](docs/WSL2_SETUP_GUIDE.md)** - Complete step-by-step instructions

### Why not Windows native?

We tried! Building with MSVC requires extensive compatibility fixes:
- Windows socket API differences
- ERROR/min/max macro conflicts  
- MSVC stricter template friend access
- ~95% complete but tedious remaining work

Since painlessMesh targets ESP32/ESP8266 (not Windows), and CI/CD uses Linux, **WSL2 is the pragmatic choice**.

**→ [Windows Build Decision](docs/WINDOWS_BUILD_DECISION.md)** - Full explanation

### Linux/macOS

Works out of the box with GCC/Clang! This is the native development environment.

## Next Steps

### 1. Set Up Your Environment

**Windows:** Follow [WSL2 Setup Guide](docs/WSL2_SETUP_GUIDE.md)  
**Linux/macOS:** Install dependencies (see TL;DR above)

### 2. Understand the Project

- **[Executive Summary](docs/SIMULATOR_SUMMARY.md)** - 5-minute overview (for managers)
- **[Quick Start Guide](docs/SIMULATOR_QUICKSTART.md)** - Usage examples (for developers)
- **[Technical Spec](docs/SIMULATOR_PLAN.md)** - Complete design (for implementers)
- **[Documentation Index](docs/SIMULATOR_INDEX.md)** - Navigate all docs

### 3. Start Building

Current Status: **Phase 1 - Foundation Setup** 🚧

We're building:
1. **Core Infrastructure** (Phase 1) - VirtualNode, NodeManager, Configuration
2. **Scenario Engine** (Phase 2) - Event-driven testing
3. **Firmware Integration** (Phase 3) - Load your own firmware
4. **Visualization** (Phase 4) - Terminal UI, metrics
5. **Polish** (Phase 5) - Documentation, release

**→ [CONTRIBUTING.md](CONTRIBUTING.md)** - How to contribute

### 4. Create Issues (Coming Soon)

GitHub issue templates are ready in `.github/ISSUE_TEMPLATES_PHASE1/`:

```bash
# Test GitHub CLI authentication
./scripts/test-gh-auth.sh

# Create all Phase 1 implementation issues
./scripts/create-phase1-issues.sh
```

## Example: Simple Mesh Test

Create `test.yaml`:
```yaml
simulation:
  name: "My First Mesh"
  duration: 60

nodes:
  - template: "basic"
    count: 10
    config:
      mesh_prefix: "TestMesh"
      mesh_password: "password"

topology:
  type: "random"
```

Run it:
```bash
./painlessmesh-simulator --config test.yaml
```

## Architecture Overview

```
Your Firmware (C++)
       ↓
FirmwareBase Interface
       ↓
VirtualNode (simulated ESP32)
       ↓
painlessMesh Library
       ↓
Boost.Asio (network simulation)
       ↓
Your Desktop (no hardware!)
```

## Common Questions

**Q: Does this replace physical testing?**  
A: No, it complements it. Use simulator for scale/edge cases, hardware for final validation.

**Q: Can I test my existing ESP32 firmware?**  
A: Yes! Implement the `FirmwareBase` interface and the simulator will run it.

**Q: How fast can it simulate?**  
A: Depends on node count. 10 nodes = real-time, 100 nodes = ~5x slower.

**Q: What about CI/CD?**  
A: Perfect for it! GitHub Actions example included in docs.

**Q: Is Windows native build supported?**  
A: Not yet. We have ~95% of fixes done, but WSL2 is recommended for now.

## Technology Stack

- **Language:** C++14
- **Networking:** Boost.Asio (same as painlessMesh)
- **Mesh Library:** painlessMesh (as submodule)
- **Configuration:** yaml-cpp
- **Build:** CMake + Ninja
- **Testing:** Catch2

## Get Help

- **Issues:** [GitHub Issues](https://github.com/Alteriom/painlessMesh-simulator/issues)
- **Discussions:** [GitHub Discussions](https://github.com/Alteriom/painlessMesh-simulator/discussions)
- **Documentation:** [Full Index](docs/SIMULATOR_INDEX.md)

## Contributing

We welcome contributions! See [CONTRIBUTING.md](CONTRIBUTING.md) for:
- Code style guidelines
- Development workflow
- Testing requirements
- PR process

## License

MIT License - See [LICENSE](LICENSE) file

---

**Ready to start?**

→ Windows: [WSL2 Setup Guide](docs/WSL2_SETUP_GUIDE.md)  
→ Linux/macOS: Install dependencies and build  
→ All Platforms: [Quick Start Guide](docs/SIMULATOR_QUICKSTART.md)

**Questions?**

→ [GitHub Discussions](https://github.com/Alteriom/painlessMesh-simulator/discussions)

---

**Status:** 🚧 Under Active Development  
**Version:** 0.1.0-alpha  
**Last Updated:** November 12, 2025
