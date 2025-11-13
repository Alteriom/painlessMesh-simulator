# WSL2 Development Setup for painlessMesh Simulator

## Why WSL2?

The painlessMesh simulator is built on top of painlessMesh's existing Boost.Asio test infrastructure, which was designed for Linux/GCC. Using WSL2 on Windows gives you:

- ✅ Native Linux environment on Windows
- ✅ No MSVC compatibility issues
- ✅ Same build environment as CI/CD
- ✅ Access to Linux tools and packages
- ✅ Full filesystem integration with Windows

## Quick Setup

### 1. Install WSL2

Open PowerShell as Administrator:

```powershell
# Enable WSL
wsl --install

# Or if already installed, update to WSL2
wsl --set-default-version 2

# Install Ubuntu (recommended)
wsl --install -d Ubuntu-22.04
```

Restart your computer after installation.

### 2. Launch WSL2

```powershell
# Start Ubuntu
wsl

# Or from Windows Terminal, select "Ubuntu" profile
```

### 3. Install Build Dependencies

Inside WSL2 Ubuntu terminal:

```bash
# Update package lists
sudo apt update

# Install build essentials
sudo apt install -y \
    build-essential \
    cmake \
    ninja-build \
    git \
    libboost-all-dev \
    libyaml-cpp-dev \
    catch2

# Verify installations
gcc --version      # Should be 11.x or later
cmake --version    # Should be 3.22 or later
```

### 4. Access Your Repository

Your Windows drives are mounted under `/mnt/`:

```bash
# Navigate to your Windows repository
cd /mnt/d/Github/painlessMesh-simulator

# Or create a symlink for convenience
ln -s /mnt/d/Github/painlessMesh-simulator ~/painlessMesh-simulator
cd ~/painlessMesh-simulator
```

### 5. Build the Simulator

```bash
# Clean any existing Windows build artifacts first
rm -rf build

# Configure with CMake
mkdir -p build && cd build
cmake .. -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DPAINLESSMESH_PATH="/mnt/d/Github/painlessMesh"

# Build
ninja

# Or use make if Ninja not available
cmake .. -DCMAKE_BUILD_TYPE=Release -DPAINLESSMESH_PATH="/mnt/d/Github/painlessMesh"
make -j$(nproc)
```

### 6. Run Tests

```bash
# Run unit tests
./simulator_tests

# Run the simulator with an example scenario
./painlessmesh-simulator --config ../examples/scenarios/simple_mesh.yaml
```

## Development Workflow

### Using VS Code with WSL2

1. Install the "Remote - WSL" extension in VS Code
2. Open Command Palette (Ctrl+Shift+P)
3. Select "WSL: Connect to WSL"
4. Navigate to your project: `cd /mnt/d/Github/painlessMesh-simulator`
5. Open folder in VS Code: `code .`

Now VS Code runs in WSL2 with:
- Full IntelliSense for Linux
- Integrated terminal in WSL2
- Git operations in Linux environment
- Debugging with GDB

### File Editing

You can edit files from either:
- **Windows**: Use any Windows editor, files auto-sync to WSL2
- **WSL2**: Use vim, nano, or VS Code Remote
- **Best**: VS Code with Remote-WSL extension

### Git Operations

```bash
# Configure git in WSL2 (first time only)
git config --global user.name "Your Name"
git config --global user.email "your.email@example.com"

# Use git normally
git status
git add .
git commit -m "Fix: issue description"
git push
```

## CI/CD Integration

The GitHub Actions workflow (`.github/workflows/ci.yml`) already uses Ubuntu and will match your WSL2 environment exactly.

### Current CI/CD Configuration

```yaml
# .github/workflows/ci.yml
jobs:
  build:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        compiler: [gcc-11, clang-14]
```

Your WSL2 builds will be identical to CI/CD builds, ensuring:
- No "works on my machine" issues
- Consistent dependency versions
- Same compiler behavior

## Troubleshooting

### Issue: CMake cache error (different than the directory)

This happens when you have an existing build from Windows and try to build in WSL2:

```bash
# Clean the build directory
cd /mnt/d/Github/painlessMesh-simulator
rm -rf build

# Start fresh
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release -DPAINLESSMESH_PATH="/mnt/d/Github/painlessMesh"
ninja
```

### Issue: "Permission denied" when building

```bash
# Fix permissions on your repository
sudo chown -R $USER:$USER /mnt/d/Github/painlessMesh-simulator
```

### Issue: CMake can't find Boost

```bash
# Install Boost development files
sudo apt install -y libboost-all-dev

# Verify Boost is found
dpkg -l | grep libboost
```

### Issue: Slow filesystem access

WSL2 filesystem is faster when working in Linux home directory:

```bash
# Copy repository to Linux filesystem (optional)
cp -r /mnt/d/Github/painlessMesh-simulator ~/
cd ~/painlessMesh-simulator

# Work here, sync back to Windows when needed
```

### Issue: "ninja: command not found"

```bash
# Use Make instead of Ninja
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

## Comparing with Windows Native Build

| Aspect | Windows Native (MSVC) | WSL2 (GCC) |
|--------|----------------------|------------|
| Compatibility | Many fixes needed | Works immediately |
| Build Speed | Slower | Faster (Ninja) |
| CI/CD Match | Different | Identical |
| Development | VS Studio | VS Code + Remote |
| Debugging | VS Debugger | GDB |
| Recommended | ❌ Not yet | ✅ **Use this** |

## Next Steps

1. ✅ Set up WSL2 (this guide)
2. ✅ Build simulator in WSL2
3. ✅ Run existing tests
4. 🚀 Start implementing simulator features:
   - YAML scenario loader
   - Metrics collection
   - Network simulation
   - Visualization
5. 🔄 Windows native support later (contribute MSVC fixes upstream)

## Additional Resources

- [WSL2 Documentation](https://docs.microsoft.com/en-us/windows/wsl/)
- [VS Code Remote-WSL](https://code.visualstudio.com/docs/remote/wsl)
- [painlessMesh Documentation](https://gitlab.com/painlessMesh/painlessMesh)
- [Simulator Architecture](./SIMULATOR_PLAN.md)

---

**Status**: Using WSL2, you can start building simulator features immediately instead of fighting MSVC compatibility issues.
