# [Phase 1.1] Add painlessMesh as Git Submodule

**Labels**: `phase-1`, `infrastructure`, `good first issue`  
**Milestone**: Phase 1 - Core Infrastructure  
**Estimated Time**: 1-2 hours

## Objective

Add painlessMesh library as a git submodule to provide the core mesh networking functionality for the simulator.

## Background

The painlessMesh simulator requires the painlessMesh library to simulate ESP32/ESP8266 mesh nodes. We'll integrate it as a git submodule to:
- Keep dependencies versioned and traceable
- Allow easy updates to painlessMesh
- Maintain clean separation between simulator and library code

## Tasks

- [ ] Add painlessMesh repository as submodule:
  ```bash
  git submodule add https://github.com/Alteriom/painlessMesh.git external/painlessMesh
  git submodule update --init --recursive
  ```

- [ ] Update `CMakeLists.txt` to include painlessMesh:
  ```cmake
  # Add painlessMesh as subdirectory
  add_subdirectory(external/painlessMesh)
  
  # Link against painlessMesh in simulator targets
  target_link_libraries(simulator_lib
    PUBLIC
      painlessmesh
      # ... other libs
  )
  ```

- [ ] Verify submodule builds successfully:
  ```bash
  mkdir build && cd build
  cmake -G Ninja ..
  ninja
  ```

- [ ] Update `README.md` with submodule clone instructions:
  - Add note about `--recursive` flag when cloning
  - Document `git submodule update --init --recursive` for existing clones

- [ ] Add submodule management section to `CONTRIBUTING.md`:
  - How to update submodules
  - How to work with submodule changes
  - Common submodule issues

## Acceptance Criteria

✅ **Submodule Added**
- `external/painlessMesh/` directory exists
- `.gitmodules` file correctly references painlessMesh
- Submodule points to Alteriom/painlessMesh repository

✅ **Build System Integration**
- CMake successfully configures with painlessMesh
- Build completes without errors
- painlessMesh library is available to link against

✅ **Documentation Updated**
- README has submodule clone instructions
- CONTRIBUTING has submodule management guide
- Build instructions mention submodule initialization

✅ **Testing**
- Fresh clone with `--recursive` works
- Existing clone with `git submodule update --init --recursive` works
- CI/CD pipeline handles submodules correctly

## Dependencies

**Blocks**: All other Phase 1 tasks depend on this

**Depends on**: None (first task)

## References

- **painlessMesh repository**: https://github.com/Alteriom/painlessMesh
- **Technical spec**: `docs/SIMULATOR_PLAN.md` - "Integration with painlessMesh" section
- **Coding standards**: `.github/copilot-instructions.md` - "Integration with painlessMesh"
- **Git submodules guide**: https://git-scm.com/book/en/v2/Git-Tools-Submodules

## Custom Agent Assistance

Use **@cpp-simulator-agent** for:
- CMake build system integration questions
- Library linking configuration
- Build troubleshooting

## Testing Steps

1. **Fresh clone test**:
   ```bash
   git clone --recursive https://github.com/Alteriom/painlessMesh-simulator.git test-clone
   cd test-clone
   mkdir build && cd build
   cmake -G Ninja ..
   ninja
   ```

2. **Existing clone test**:
   ```bash
   git pull
   git submodule update --init --recursive
   rm -rf build && mkdir build && cd build
   cmake -G Ninja ..
   ninja
   ```

3. **CI/CD test**:
   - Push changes and verify CI pipeline passes
   - Check that submodules are properly initialized in CI

## Notes

- Use Alteriom fork of painlessMesh: https://github.com/Alteriom/painlessMesh
- Submodule should track `master` branch by default
- Consider pinning to a specific commit/tag once stable
- Update `.github/workflows/ci.yml` to handle submodules (`submodules: recursive` in checkout action)

## Success Metrics

- ✅ Submodule properly initialized
- ✅ Build system compiles painlessMesh
- ✅ Documentation complete
- ✅ CI/CD passes
- ✅ No build warnings or errors
