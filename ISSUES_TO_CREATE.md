# Phase 1 Issues Ready to Create

This document provides instructions for creating the Phase 1 implementation issues for the painlessMesh simulator.

## Status

✅ **All issue templates are prepared and ready**

The following 5 Phase 1 issues are ready to be created:

1. **[Phase 1.1] Add painlessMesh as Git Submodule** - Infrastructure setup
2. **[Phase 1.1] Implement VirtualNode Class** - Core node simulation
3. **[Phase 1.1] Implement NodeManager Class** - Multi-node management
4. **[Phase 1.2] Implement Configuration Loader** - YAML configuration system
5. **[Phase 1.1] Implement CLI Application** - Command-line interface

## How to Create Issues

### Option 1: Automated Script (Recommended)

The repository includes a script that creates all issues automatically:

```bash
# Navigate to repository root
cd /path/to/painlessMesh-simulator

# First, authenticate with GitHub CLI (one-time setup)
gh auth login

# Or set a token
export GH_TOKEN=your_github_token_here

# Test authentication
./scripts/test-gh-auth.sh

# Create all Phase 1 issues
./scripts/create-phase1-issues.sh
```

The script will:
- ✅ Check authentication
- ✅ Verify repository access
- ✅ Create all 5 issues with proper titles, labels, and content
- ✅ Add rate limiting to be nice to GitHub API
- ✅ Show progress and summary

### Option 2: GitHub CLI (Manual, One-by-One)

Create issues individually using `gh` CLI:

```bash
# Issue #1: Add painlessMesh Submodule
gh issue create \
  --repo Alteriom/painlessMesh-simulator \
  --title "[Phase 1.1] Add painlessMesh as Git Submodule" \
  --body-file .github/ISSUE_TEMPLATES_PHASE1/01-add-painlessmesh-submodule.md \
  --label "phase-1,infrastructure,good first issue"

# Issue #2: Implement VirtualNode
gh issue create \
  --repo Alteriom/painlessMesh-simulator \
  --title "[Phase 1.1] Implement VirtualNode Class" \
  --body-file .github/ISSUE_TEMPLATES_PHASE1/02-implement-virtualnode-class.md \
  --label "phase-1,core,c++"

# Issue #3: Implement NodeManager
gh issue create \
  --repo Alteriom/painlessMesh-simulator \
  --title "[Phase 1.1] Implement NodeManager Class" \
  --body-file .github/ISSUE_TEMPLATES_PHASE1/03-implement-nodemanager-class.md \
  --label "phase-1,core,c++"

# Issue #4: Implement ConfigLoader
gh issue create \
  --repo Alteriom/painlessMesh-simulator \
  --title "[Phase 1.2] Implement Configuration Loader" \
  --body-file .github/ISSUE_TEMPLATES_PHASE1/04-implement-config-loader.md \
  --label "phase-1,configuration,yaml"

# Issue #5: Implement CLI
gh issue create \
  --repo Alteriom/painlessMesh-simulator \
  --title "[Phase 1.1] Implement CLI Application" \
  --body-file .github/ISSUE_TEMPLATES_PHASE1/05-implement-cli-application.md \
  --label "phase-1,cli,c++"
```

### Option 3: GitHub Web Interface (Manual)

Create issues through the GitHub web interface:

1. Go to https://github.com/Alteriom/painlessMesh-simulator/issues/new
2. For each issue template in `.github/ISSUE_TEMPLATES_PHASE1/`:
   - Copy the title (first line without `#`)
   - Copy the entire content as the issue body
   - Add the labels mentioned in the template
   - Click "Submit new issue"

**Templates to create:**
- `.github/ISSUE_TEMPLATES_PHASE1/01-add-painlessmesh-submodule.md`
- `.github/ISSUE_TEMPLATES_PHASE1/02-implement-virtualnode-class.md`
- `.github/ISSUE_TEMPLATES_PHASE1/03-implement-nodemanager-class.md`
- `.github/ISSUE_TEMPLATES_PHASE1/04-implement-config-loader.md`
- `.github/ISSUE_TEMPLATES_PHASE1/05-implement-cli-application.md`

## Issue Details

### Issue #1: Add painlessMesh as Git Submodule
- **Labels**: `phase-1`, `infrastructure`, `good first issue`
- **Estimated Time**: 1-2 hours
- **Dependencies**: None (start here)
- **Blocks**: All other Phase 1 tasks
- **Template**: `.github/ISSUE_TEMPLATES_PHASE1/01-add-painlessmesh-submodule.md`

### Issue #2: Implement VirtualNode Class
- **Labels**: `phase-1`, `core`, `c++`
- **Estimated Time**: 4-6 hours
- **Dependencies**: Issue #1
- **Blocks**: Issue #3
- **Template**: `.github/ISSUE_TEMPLATES_PHASE1/02-implement-virtualnode-class.md`

### Issue #3: Implement NodeManager Class
- **Labels**: `phase-1`, `core`, `c++`
- **Estimated Time**: 4-6 hours
- **Dependencies**: Issue #2
- **Blocks**: Issue #5
- **Template**: `.github/ISSUE_TEMPLATES_PHASE1/03-implement-nodemanager-class.md`

### Issue #4: Implement Configuration Loader
- **Labels**: `phase-1`, `configuration`, `yaml`
- **Estimated Time**: 6-8 hours
- **Dependencies**: Issues #1, #2
- **Blocks**: Issue #5
- **Template**: `.github/ISSUE_TEMPLATES_PHASE1/04-implement-config-loader.md`

### Issue #5: Implement CLI Application
- **Labels**: `phase-1`, `cli`, `c++`
- **Estimated Time**: 4-6 hours
- **Dependencies**: Issues #2, #3, #4
- **Blocks**: Phase 1 completion
- **Template**: `.github/ISSUE_TEMPLATES_PHASE1/05-implement-cli-application.md`

## Implementation Order

Follow this order for optimal workflow:

1. **Issue #1** - Add painlessMesh submodule (prerequisite for all)
2. **Issue #2** - Implement VirtualNode (foundation)
3. **Issue #3** - Implement NodeManager (builds on VirtualNode)
4. **Issue #4** - Implement ConfigLoader (parallel to #3 if desired)
5. **Issue #5** - Implement CLI (integrates everything)

## After Creating Issues

Once all issues are created:

1. ✅ **Review Issues**: Check that all details are correct
2. ✅ **Assign Issues**: Assign to team members or agents
   - Use `@cpp-simulator-agent` for C++ implementation
   - Use `@scenario-config-agent` for YAML/config work
   - Use `@documentation-agent` for documentation
3. ✅ **Start Implementation**: Begin with Issue #1
4. ✅ **Track Progress**: Use GitHub project board or milestones

## Phase 1 Deliverables

After completing all 5 issues, the simulator will be able to:

- ✅ Load YAML configuration files
- ✅ Create 10+ virtual mesh nodes
- ✅ Form mesh network automatically
- ✅ Run simulation for specified duration
- ✅ Report basic statistics

## Success Criteria

**Phase 1 Complete** when:
- ✅ All 5 issues closed
- ✅ CI/CD pipeline passes
- ✅ Can run 10-node simulation successfully
- ✅ Unit test coverage ≥ 80%
- ✅ Documentation updated
- ✅ Code reviewed and merged

## Next Steps

1. **Create the issues** using one of the methods above
2. **Verify issues appear** at https://github.com/Alteriom/painlessMesh-simulator/issues
3. **Begin Phase 1 implementation** starting with Issue #1

## Resources

- **Technical Specification**: `docs/SIMULATOR_PLAN.md`
- **Quick Start Guide**: `docs/SIMULATOR_QUICKSTART.md`
- **Coding Standards**: `.github/copilot-instructions.md`
- **Contributing Guide**: `CONTRIBUTING.md`
- **Issue Templates**: `.github/ISSUE_TEMPLATES_PHASE1/`

## Troubleshooting

### GitHub CLI Not Authenticated

```bash
# Run authentication
gh auth login

# Or set token
export GH_TOKEN=your_token_here
export GITHUB_TOKEN=your_token_here
```

### Script Fails to Create Issues

1. Check authentication: `./scripts/test-gh-auth.sh`
2. Verify repository access: `gh repo view Alteriom/painlessMesh-simulator`
3. Check you have write access to the repository
4. Try creating one issue manually to test permissions

### Labels Don't Exist

GitHub will automatically create labels if they don't exist. The script handles this automatically.

---

**Total Estimated Time**: 19-28 hours  
**Target Duration**: 2 weeks (Weeks 1-2)  
**Milestone**: Phase 1 - Core Infrastructure

**Ready to create?** Choose one of the methods above and create all 5 issues!
