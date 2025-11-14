# Library Validation Quick Start

## TL;DR

The simulator uses a **two-tier firmware architecture**:

1. **Tier 1: Library Validation** - Tests painlessMesh library itself ✅
2. **Tier 2: Application Firmware** - Tests your applications (built on validated library)

This ensures we validate the library before trusting application firmware.

## 5-Minute Quick Start

### Run Library Validation

```bash
# Build simulator
cd painlessMesh-simulator
mkdir build && cd build
cmake -G Ninja ..
ninja

# Run library validation (3 minutes)
./bin/painlessmesh-simulator --config ../examples/scenarios/library_validation.yaml
```

### Check Results

Look for this in the output:
```
========== Library Validation Report ==========
Overall: PASSED ✅
Tests: 45/45 passed (0 failed)

API Coverage:
  Tested: 25/25 (100% pass rate)
===============================================
```

## What Just Happened?

The `LibraryValidationFirmware` systematically tested:
- ✅ Mesh initialization and lifecycle
- ✅ Message sending (broadcast and single)
- ✅ Message reception callbacks
- ✅ Connection management
- ✅ Time synchronization
- ✅ Topology discovery

These are the **painlessMesh library APIs**, not application logic.

## Why This Matters

### Before (Problem)
- Had application firmware examples
- No systematic library validation
- Unclear if simulator matched real hardware

### After (Solution)
- Systematic library validation (Tier 1) ✅
- Application firmware built on validated APIs (Tier 2)
- Clear API coverage tracking
- Confidence in simulator accuracy

## Key Concepts

### Library Validation Firmware (Tier 1)

**Purpose**: Validate painlessMesh library APIs work correctly

**Characteristics**:
- Tests library, not applications
- Systematic API coverage
- Pass/fail validation
- Foundation for trust

**Example**: `LibraryValidationFirmware`

### Application Firmware (Tier 2)

**Purpose**: Demonstrate real-world use cases

**Characteristics**:
- Application-specific logic
- Uses validated APIs
- Real-world patterns
- User examples

**Examples**: `SimpleBroadcastFirmware`, `EchoServerFirmware`

## Quick Reference

### When to Use Library Validation

- ✅ Before releasing simulator
- ✅ After painlessMesh updates
- ✅ When adding new APIs
- ✅ In CI/CD pipelines
- ✅ When debugging simulator

### When to Use Application Firmware

- ✅ Building IoT applications
- ✅ Testing firmware logic
- ✅ Creating examples
- ✅ Integration testing
- ✅ Custom scenarios

## Validated APIs

### ✅ Core APIs (Validated)
- `init()`, `stop()`, `update()`
- `sendBroadcast()`, `sendSingle()`
- `onReceive()`, `onNewConnection()`
- `getNodeTime()`, `getNodeList()`
- `onChangedConnections()`, `onNodeTimeAdjusted()`

### ⏳ Extended APIs (To Be Validated)
- Priority messaging
- Root node APIs
- Delay measurement
- Bridge functionality
- OTA updates

## Common Commands

### Run Full Validation
```bash
./painlessmesh-simulator --config examples/scenarios/library_validation.yaml
```

### Quick Validation (1 minute)
```bash
./painlessmesh-simulator \
  --config examples/scenarios/library_validation.yaml \
  --duration 60
```

### Validation with Debug Logging
```bash
./painlessmesh-simulator \
  --config examples/scenarios/library_validation.yaml \
  --log-level DEBUG
```

## Understanding the Output

### Success
```
[VAL] Library Validation Firmware starting on node 10001
[VAL] Node 10001 role: COORDINATOR
[VAL] Testing Mesh Lifecycle APIs
[VAL] Test: init() - PASSED ✅
[VAL] Test: getNodeTime() - PASSED ✅
...
[VAL] Completing RESILIENCE_TESTS phase

========== Library Validation Report ==========
Overall: PASSED ✅
===============================================
```

### Failure
```
[VAL] Test: sendBroadcast() - FAILED ❌
[ERROR] Message delivery failed

========== Library Validation Report ==========
Overall: FAILED ❌
Tests: 44/45 passed (1 failed)

Failed Tests:
  - sendBroadcast(): Message delivery failed
===============================================
```

## Scenario Configuration

### Coordinator Node (Runs Tests)
```yaml
- id: "validator"
  firmware: "library_validation"
  firmware_config:
    role: "coordinator"           # Runs tests
    test_duration: "30"           # 30s per phase
    enable_detailed_logging: "true"
```

### Participant Nodes (Respond to Tests)
```yaml
- template: "participant"
  count: 9
  firmware: "library_validation"
  firmware_config:
    role: "participant"           # Responds to tests
```

## Development Workflow

### 1. Check Validated APIs
```bash
# See which APIs are validated
cat docs/FIRMWARE_DEVELOPMENT_GUIDE.md | grep "✅"
```

### 2. Develop Your Firmware
```cpp
class MyFirmware : public FirmwareBase {
  void setup() override {
    // Use only validated APIs
    sendBroadcast("Hello");
  }
};
```

### 3. Test Your Firmware
```bash
# Create scenario
vim examples/scenarios/my_test.yaml

# Run test
./painlessmesh-simulator --config examples/scenarios/my_test.yaml
```

## Documentation

### Quick Reads (5 minutes)
- [LIBRARY_VALIDATION_QUICK_START.md](LIBRARY_VALIDATION_QUICK_START.md) ← You are here
- [LIBRARY_VALIDATION_SUMMARY.md](LIBRARY_VALIDATION_SUMMARY.md) - Executive summary

### Detailed Guides (15-30 minutes)
- [LIBRARY_VALIDATION_PLAN.md](LIBRARY_VALIDATION_PLAN.md) - Complete technical spec
- [FIRMWARE_DEVELOPMENT_GUIDE.md](FIRMWARE_DEVELOPMENT_GUIDE.md) - Development guide

## FAQ

**Q: Do I need to run validation every time?**
A: No. Run when developing simulator, before releases, or in CI/CD.

**Q: Can I skip library validation?**
A: Not recommended. It ensures simulator accuracy.

**Q: What if validation fails?**
A: Investigate if it's a simulator bug, library bug, or test bug.

**Q: How do I add tests?**
A: Edit `LibraryValidationFirmware.cpp`, add test method, run validation.

**Q: Can I use untested APIs?**
A: Yes, but simulator behavior may not match real hardware.

## Next Steps

1. ✅ Run library validation
2. ✅ Review validation report
3. ✅ Read [LIBRARY_VALIDATION_SUMMARY.md](LIBRARY_VALIDATION_SUMMARY.md)
4. ✅ Read [FIRMWARE_DEVELOPMENT_GUIDE.md](FIRMWARE_DEVELOPMENT_GUIDE.md)
5. ✅ Develop your application firmware
6. ✅ Contribute validation tests for untested APIs

## Contributing

Found an untested API? Want to improve validation?

1. Check [LIBRARY_VALIDATION_PLAN.md](LIBRARY_VALIDATION_PLAN.md) for coverage
2. Add test to `LibraryValidationFirmware`
3. Run validation scenario
4. Submit PR with test + updated docs

---

**Quick Links**:
- [Main README](../README.md)
- [Library Validation Summary](LIBRARY_VALIDATION_SUMMARY.md)
- [Firmware Development Guide](FIRMWARE_DEVELOPMENT_GUIDE.md)
- [Validation Scenario](../examples/scenarios/library_validation.yaml)

**Version:** 1.0 | **Updated:** 2025-11-14 | **Status:** Active
