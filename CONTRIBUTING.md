# Contributing to painlessMesh Simulator

Thank you for your interest in contributing to the painlessMesh Simulator! This document provides guidelines and information for contributors.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [How to Contribute](#how-to-contribute)
- [Coding Standards](#coding-standards)
- [Testing Guidelines](#testing-guidelines)
- [Documentation](#documentation)
- [Pull Request Process](#pull-request-process)

## Code of Conduct

This project adheres to a code of conduct adapted from the Contributor Covenant. By participating, you are expected to uphold this code. Please report unacceptable behavior to the project maintainers.

### Our Standards

- Be respectful and inclusive
- Welcome newcomers and help them learn
- Focus on what is best for the community
- Show empathy towards other community members

## Getting Started

### Prerequisites

Before you begin, ensure you have:

- Git installed
- C++14 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- CMake 3.10 or higher
- Basic understanding of C++ and mesh networking

### First-Time Setup

1. **Fork the repository** on GitHub
2. **Clone your fork**:
   ```bash
   git clone --recursive https://github.com/YOUR_USERNAME/painlessMesh-simulator.git
   cd painlessMesh-simulator
   ```

3. **Add upstream remote**:
   ```bash
   git remote add upstream https://github.com/Alteriom/painlessMesh-simulator.git
   ```

4. **Install dependencies**:
   ```bash
   # Ubuntu/Debian
   sudo apt-get install cmake ninja-build libboost-dev libyaml-cpp-dev
   
   # macOS
   brew install cmake ninja boost yaml-cpp
   ```

5. **Build the project**:
   ```bash
   mkdir build && cd build
   cmake -G Ninja ..
   ninja
   ```

6. **Run tests**:
   ```bash
   ninja test
   ```

## Development Setup

### Branch Strategy

- `master` or `main`: Stable release branch
- `develop`: Development branch (default for PRs)
- `feature/*`: Feature branches
- `fix/*`: Bug fix branches
- `docs/*`: Documentation branches

### Development Workflow

1. **Create a feature branch**:
   ```bash
   git checkout develop
   git pull upstream develop
   git checkout -b feature/my-feature
   ```

2. **Make your changes**
3. **Test your changes**
4. **Commit with clear messages**
5. **Push to your fork**
6. **Create a Pull Request**

## How to Contribute

### Reporting Bugs

Before creating bug reports, please check existing issues. When creating a bug report, include:

- Clear, descriptive title
- Steps to reproduce
- Expected behavior
- Actual behavior
- Environment details (OS, compiler, versions)
- Relevant logs or screenshots

**Bug Report Template:**
```markdown
**Describe the bug**
A clear description of what the bug is.

**To Reproduce**
Steps to reproduce:
1. Create scenario with...
2. Run with command...
3. See error

**Expected behavior**
What you expected to happen.

**Actual behavior**
What actually happened.

**Environment:**
- OS: [e.g., Ubuntu 22.04]
- Compiler: [e.g., GCC 11.3]
- CMake: [e.g., 3.22]
- Boost: [e.g., 1.74]

**Logs**
```
Relevant log output
```

**Screenshots**
If applicable
```

### Suggesting Enhancements

Enhancement suggestions are welcome! Please include:

- Clear description of the enhancement
- Use cases and motivation
- Examples of how it would be used
- Impact on existing functionality

### Contributing Code

#### Types of Contributions

1. **Bug Fixes**: Fix reported issues
2. **New Features**: Implement planned features
3. **Performance Improvements**: Optimize existing code
4. **Documentation**: Improve or add documentation
5. **Tests**: Add or improve test coverage
6. **Examples**: Create example scenarios or firmware

#### Before You Start

- Check existing issues and PRs
- Discuss major changes in an issue first
- Follow the coding standards
- Write tests for new functionality

## Coding Standards

### C++ Style Guide

Follow the guidelines in `.github/copilot-instructions.md`:

#### Naming Conventions
```cpp
// Classes: PascalCase
class VirtualNode { };

// Functions: camelCase
void startSimulation();

// Variables: snake_case
int node_count;
std::string mesh_prefix;

// Constants: UPPER_SNAKE_CASE
constexpr int MAX_NODES = 1000;

// Private members: trailing underscore
class Node {
private:
  int node_id_;
  std::unique_ptr<Mesh> mesh_;
};
```

#### Code Formatting

Use consistent formatting:

```cpp
// Include order
#include <system_headers>
#include <external_library_headers>
#include "project_headers.hpp"

// Namespace usage
using namespace std;  // Avoid in headers
using std::string;    // Prefer specific imports

// Indentation: 2 spaces
void function() {
  if (condition) {
    doSomething();
  }
}

// Brace style: K&R (opening brace on same line)
class MyClass {
public:
  void method() {
    // code
  }
};
```

#### Documentation

Document all public APIs:

```cpp
/**
 * @brief Creates a virtual mesh node
 * 
 * @param nodeId Unique identifier for the node
 * @param config Node configuration parameters
 * @param scheduler Task scheduler instance
 * @param io Boost.Asio IO context
 * 
 * @throws std::invalid_argument if nodeId is 0
 * @throws std::runtime_error if initialization fails
 * 
 * @return Shared pointer to the created node
 */
std::shared_ptr<VirtualNode> createNode(
  uint32_t nodeId,
  const NodeConfig& config,
  Scheduler* scheduler,
  boost::asio::io_context& io
);
```

### Best Practices

#### Memory Management
```cpp
// ✅ Use smart pointers
auto node = std::make_unique<VirtualNode>(config);

// ❌ Avoid raw pointers
VirtualNode* node = new VirtualNode(config);  // Don't do this
```

#### Error Handling
```cpp
// ✅ Use exceptions for errors
if (nodeId == 0) {
  throw std::invalid_argument("Node ID must be non-zero");
}

// ✅ Return optional for expected failures
std::optional<NodeConfig> loadConfig(const std::string& path);
```

#### Performance
```cpp
// ✅ Pass large objects by const reference
void processData(const std::vector<Data>& data);

// ✅ Use move semantics for transfers
std::vector<Node> nodes = createNodes();  // Move, not copy

// ❌ Avoid unnecessary copies
void processData(std::vector<Data> data);  // Copies!
```

## Testing Guidelines

### Test Structure

Use Catch2 framework:

```cpp
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Component behavior", "[tag]") {
  // Arrange
  auto component = createComponent();
  
  // Act
  auto result = component.doSomething();
  
  // Assert
  REQUIRE(result == expected);
}
```

### Test Categories

1. **Unit Tests**: Test individual components
   ```cpp
   TEST_CASE("VirtualNode lifecycle", "[virtual_node]") {
     // Test node creation, start, stop
   }
   ```

2. **Integration Tests**: Test component interactions
   ```cpp
   TEST_CASE("Multi-node mesh formation", "[integration]") {
     // Test mesh formation with multiple nodes
   }
   ```

3. **System Tests**: Test complete scenarios
   ```bash
   ./painlessmesh-simulator --config test_scenario.yaml --headless
   ```

### Running Tests

```bash
# Build with tests
cmake -G Ninja -DENABLE_TESTING=ON ..
ninja

# Run all tests
ninja test

# Run specific test
./build/bin/simulator_tests "[tag]"

# Run with verbose output
./build/bin/simulator_tests -s
```

### Test Coverage

Aim for 80%+ code coverage:

```bash
# Build with coverage
cmake -G Ninja -DENABLE_COVERAGE=ON ..
ninja

# Run tests
ninja test

# Generate coverage report
ninja coverage
```

## Documentation

### Documentation Types

1. **Code Documentation**: Inline comments and Doxygen
2. **API Reference**: Generated from code
3. **User Guides**: How-to documents
4. **Tutorials**: Step-by-step learning
5. **Examples**: Working code samples

### Writing Documentation

- Use clear, simple language
- Provide complete examples
- Show expected output
- Address common mistakes
- Keep up-to-date with code changes

### Documentation Files

- `README.md`: Project overview
- `docs/ARCHITECTURE.md`: System design
- `docs/API_REFERENCE.md`: API documentation
- `docs/USER_GUIDE.md`: Usage instructions
- `examples/`: Working examples

## Pull Request Process

### Before Submitting

1. **Update from upstream**:
   ```bash
   git checkout develop
   git pull upstream develop
   git checkout feature/my-feature
   git rebase develop
   ```

2. **Run tests**:
   ```bash
   ninja test
   ```

3. **Check formatting**:
   ```bash
   clang-format -i src/**/*.cpp include/**/*.hpp
   ```

4. **Update documentation** if needed

5. **Commit changes**:
   ```bash
   git add .
   git commit -m "feat(component): Add new feature"
   ```

### Commit Message Format

Follow conventional commits:

```
<type>(<scope>): <subject>

<body>

<footer>
```

**Types:**
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation
- `style`: Formatting
- `refactor`: Code restructuring
- `test`: Adding tests
- `chore`: Maintenance

**Examples:**
```
feat(virtual-node): Add firmware loading support

- Implement FirmwareBase interface
- Add firmware factory
- Update VirtualNode to accept firmware
- Add tests for firmware lifecycle

Closes #123
```

```
fix(config): Handle missing mesh_prefix field

Adds validation and default value for mesh_prefix
to prevent crashes when field is missing.

Fixes #456
```

### Creating Pull Request

1. **Push to your fork**:
   ```bash
   git push origin feature/my-feature
   ```

2. **Open PR on GitHub**:
   - Target `develop` branch (not master)
   - Use descriptive title
   - Fill out PR template
   - Link related issues

3. **PR Template**:
   ```markdown
   ## Description
   Brief description of changes
   
   ## Type of Change
   - [ ] Bug fix
   - [ ] New feature
   - [ ] Breaking change
   - [ ] Documentation update
   
   ## Testing
   - [ ] Tests pass locally
   - [ ] Added new tests
   - [ ] Updated documentation
   
   ## Checklist
   - [ ] Code follows style guide
   - [ ] Self-review completed
   - [ ] Comments added for complex code
   - [ ] Documentation updated
   - [ ] No new warnings
   
   ## Related Issues
   Closes #123
   ```

### Code Review Process

1. Maintainers review your PR
2. Address feedback by pushing new commits
3. Once approved, maintainer will merge
4. Delete your feature branch after merge

### Review Criteria

- Code quality and style
- Test coverage
- Documentation completeness
- Performance impact
- Breaking changes
- Security implications

## Development Tips

### IDE Setup

**Visual Studio Code:**
```json
{
  "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
  "cmake.configureOnOpen": true,
  "cmake.buildDirectory": "${workspaceFolder}/build"
}
```

**CLion:**
- Open CMakeLists.txt as project
- Configure CMake with desired generator
- Use built-in debugger and profiler

### Debugging

```cpp
// Add debug output
#define DEBUG_ENABLED 1

#if DEBUG_ENABLED
  std::cout << "Debug: " << variable << std::endl;
#endif

// Use debugger
gdb ./painlessmesh-simulator
(gdb) break main
(gdb) run --config test.yaml
```

### Performance Profiling

```bash
# Build with profiling
cmake -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
ninja

# Profile with perf (Linux)
perf record ./painlessmesh-simulator --config test.yaml
perf report

# Profile with Instruments (macOS)
instruments -t "Time Profiler" ./painlessmesh-simulator
```

## Community

### Communication Channels

- **GitHub Issues**: Bug reports, feature requests
- **GitHub Discussions**: Q&A, ideas, general discussion
- **Pull Requests**: Code contributions

### Getting Help

- Check documentation first
- Search existing issues
- Ask in GitHub Discussions
- Create detailed issue if needed

### Recognition

Contributors are recognized in:
- README.md contributors section
- CHANGELOG.md release notes
- GitHub contributor graph

## License

By contributing, you agree that your contributions will be licensed under the MIT License.

---

**Thank you for contributing to painlessMesh Simulator!**

We appreciate your time and effort in helping make this project better.
