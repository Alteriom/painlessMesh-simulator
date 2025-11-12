# Phase 1 Implementation Issues

This directory contains detailed issue templates for Phase 1 (Core Infrastructure) of the painlessMesh simulator implementation.

## Creating GitHub Issues

To create these as GitHub issues, use one of the following methods:

### Method 1: Manual Creation
1. Go to https://github.com/Alteriom/painlessMesh-simulator/issues/new
2. Copy the content from each `.md` file
3. Set the title from the first heading
4. Add the labels mentioned in each template
5. Create the issue

### Method 2: GitHub CLI
```bash
# Navigate to repository
cd /path/to/painlessMesh-simulator

# Create each issue (requires gh CLI and authentication)
gh issue create --title "[Phase 1.1] Add painlessMesh as Git Submodule" \
  --body-file .github/ISSUE_TEMPLATES_PHASE1/01-add-painlessmesh-submodule.md \
  --label "phase-1,infrastructure,good first issue"

gh issue create --title "[Phase 1.1] Implement VirtualNode Class" \
  --body-file .github/ISSUE_TEMPLATES_PHASE1/02-implement-virtualnode-class.md \
  --label "phase-1,core,c++"

gh issue create --title "[Phase 1.1] Implement NodeManager Class" \
  --body-file .github/ISSUE_TEMPLATES_PHASE1/03-implement-nodemanager-class.md \
  --label "phase-1,core,c++"

gh issue create --title "[Phase 1.2] Implement Configuration Loader" \
  --body-file .github/ISSUE_TEMPLATES_PHASE1/04-implement-config-loader.md \
  --label "phase-1,configuration,yaml"

gh issue create --title "[Phase 1.1] Implement CLI Application" \
  --body-file .github/ISSUE_TEMPLATES_PHASE1/05-implement-cli-application.md \
  --label "phase-1,cli,c++"
```

### Method 3: Bulk Creation Script
```bash
#!/bin/bash
# create_phase1_issues.sh

for file in .github/ISSUE_TEMPLATES_PHASE1/*.md; do
  if [ "$file" != ".github/ISSUE_TEMPLATES_PHASE1/README.md" ]; then
    # Extract title from first heading
    title=$(head -1 "$file" | sed 's/^# //')
    
    # Extract labels from second line
    labels=$(head -2 "$file" | tail -1 | sed 's/.*Labels.*: `\(.*\)`/\1/' | tr ',' '\n' | xargs | tr ' ' ',')
    
    # Create issue
    gh issue create --title "$title" --body-file "$file" --label "$labels"
    
    echo "Created: $title"
    sleep 2 # Rate limiting
  fi
done
```

## Phase 1 Issues Overview

### Milestone 1.1: Basic Simulator Framework (Weeks 1-2, Part 1)

#### Issue #1: Add painlessMesh as Git Submodule
- **File**: `01-add-painlessmesh-submodule.md`
- **Labels**: `phase-1`, `infrastructure`, `good first issue`
- **Estimated Time**: 1-2 hours
- **Dependencies**: None (first task)
- **Blocks**: All other Phase 1 tasks

**Description**: Add painlessMesh library as a git submodule and integrate it into the CMake build system.

#### Issue #2: Implement VirtualNode Class
- **File**: `02-implement-virtualnode-class.md`
- **Labels**: `phase-1`, `core`, `c++`
- **Estimated Time**: 4-6 hours
- **Dependencies**: #1
- **Blocks**: #3

**Description**: Implement the VirtualNode class that represents a single simulated ESP32/ESP8266 mesh device.

#### Issue #3: Implement NodeManager Class
- **File**: `03-implement-nodemanager-class.md`
- **Labels**: `phase-1`, `core`, `c++`
- **Estimated Time**: 4-6 hours
- **Dependencies**: #2
- **Blocks**: #5

**Description**: Implement the NodeManager class that creates and manages multiple VirtualNode instances.

### Milestone 1.2: Configuration System (Weeks 1-2, Part 2)

#### Issue #4: Implement Configuration Loader
- **File**: `04-implement-config-loader.md`
- **Labels**: `phase-1`, `configuration`, `yaml`
- **Estimated Time**: 6-8 hours
- **Dependencies**: #1, #2
- **Blocks**: #5

**Description**: Implement YAML configuration loading system for defining simulation scenarios.

#### Issue #5: Implement CLI Application
- **File**: `05-implement-cli-application.md`
- **Labels**: `phase-1`, `cli`, `c++`
- **Estimated Time**: 4-6 hours
- **Dependencies**: #2, #3, #4
- **Blocks**: Phase 1 completion

**Description**: Implement the command-line interface that ties together all Phase 1 components.

## Implementation Order

Follow this order for optimal workflow:

1. **Issue #1** - Add painlessMesh submodule (prerequisite for all)
2. **Issue #2** - Implement VirtualNode (foundation)
3. **Issue #3** - Implement NodeManager (builds on VirtualNode)
4. **Issue #4** - Implement ConfigLoader (parallel to #3 if desired)
5. **Issue #5** - Implement CLI (integrates everything)

## Deliverables

After completing all Phase 1 issues, the simulator will be able to:

✅ Load YAML configuration files  
✅ Create 10+ virtual mesh nodes  
✅ Form mesh network automatically  
✅ Run simulation for specified duration  
✅ Report basic statistics  

## Success Criteria

**Phase 1 Complete** when:
- All 5 issues closed
- CI/CD pipeline passes
- Can run 10-node simulation successfully
- Unit test coverage ≥ 80%
- Documentation updated
- Code reviewed and merged

## Next Phase

After Phase 1 completion, proceed to:
- **Phase 2**: Scenario Engine (event system, network simulation)

## Getting Help

- **C++ Development**: Use `@cpp-simulator-agent` in GitHub Copilot
- **YAML Configuration**: Use `@scenario-config-agent`
- **Documentation**: Use `@documentation-agent`
- **Questions**: Create discussion in GitHub Discussions
- **Bugs**: Create bug report issue

## References

- **Technical Specification**: `docs/SIMULATOR_PLAN.md`
- **Quick Start Guide**: `docs/SIMULATOR_QUICKSTART.md`
- **Coding Standards**: `.github/copilot-instructions.md`
- **Contributing Guide**: `CONTRIBUTING.md`

---

**Total Estimated Time**: 19-28 hours  
**Target Duration**: 2 weeks (Weeks 1-2)  
**Milestone**: Phase 1 - Core Infrastructure
