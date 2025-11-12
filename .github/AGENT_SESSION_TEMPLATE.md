# AI Agent Session Template

**Use this template when starting a new coding session**

## Session Initialization

### 1. Verify GitHub Access

```python
# Check authentication
user = mcp_github_github_get_me()
print(f"Authenticated as: {user['login']}")
# Expected: sparck75
```

### 2. Understand Current State

```python
# List open issues by phase
phase1_issues = mcp_github_github_list_issues(
  owner="Alteriom",
  repo="painlessMesh-simulator",
  labels=["phase-1"],
  state="OPEN"
)

# Check active PRs
active_prs = mcp_github_github_list_pull_requests(
  owner="Alteriom",
  repo="painlessMesh-simulator",
  state="open"
)
```

### 3. Review Documentation

- [ ] Read `.github/copilot-instructions.md` - Coding standards
- [ ] Review `docs/SIMULATOR_PLAN.md` - Technical specification
- [ ] Check `README.md` - Current project state
- [ ] Review relevant issue descriptions

### 4. Identify Next Task

Based on issue priorities and dependencies:

- Current Phase: **Phase 1** (Core Infrastructure)
- Next Issue: **#___ - [Title]**
- Dependencies: **[List prerequisite issues]**
- Blockers: **[Any issues blocking progress]**

## Work Plan

### Issue: #___ - [Title]

**Objective**: [Brief description]

**Estimated Time**: X hours

**Dependencies**:

- [ ] Issue #X completed
- [ ] Documentation reviewed
- [ ] Build environment verified

**Deliverables**:

- [ ] Implementation complete
- [ ] Unit tests written (80%+ coverage)
- [ ] Integration tests passed
- [ ] Documentation updated
- [ ] Code reviewed

### Implementation Steps

1. **Create Feature Branch**

   ```python
   mcp_github_github_create_branch(
     owner="Alteriom",
     repo="painlessMesh-simulator",
     branch="feature/[feature-name]",
     from_branch="main"
   )
   ```

2. **Implement Core Functionality**
   - [ ] Write failing tests (TDD)
   - [ ] Implement minimal code to pass tests
   - [ ] Refactor and optimize
   - [ ] Add documentation

3. **Verify Quality**
   - [ ] Run unit tests
   - [ ] Check code coverage
   - [ ] Run linting/formatting
   - [ ] Verify no warnings

4. **Update Documentation**
   - [ ] Update API docs (Doxygen comments)
   - [ ] Update README if needed
   - [ ] Add examples
   - [ ] Update CHANGELOG

5. **Create Pull Request**

   ```python
   mcp_github_github_create_pull_request(
     owner="Alteriom",
     repo="painlessMesh-simulator",
     title="feat(component): Brief description",
     head="feature/[feature-name]",
     base="main",
     body="""
     Closes #___
     
     ## Summary
     [Brief overview]
     
     ## Changes
     - Implementation detail 1
     - Implementation detail 2
     
     ## Testing
     - Unit test coverage: XX%
     - All tests passing
     
     ## Documentation
     - API docs updated
     - Examples added
     """
   )
   ```

6. **Request Review**

   ```python
   # Get PR number from previous call
   mcp_github_github_request_copilot_review(
     owner="Alteriom",
     repo="painlessMesh-simulator",
     pullNumber=PR_NUMBER
   )
   ```

## Progress Tracking

### During Implementation

Update issue with progress comments:

```python
mcp_github_github_add_issue_comment(
  owner="Alteriom",
  repo="painlessMesh-simulator",
  issue_number=ISSUE_NUMBER,
  body="""
  ## Progress Update
  
  **Completed**:
  - [x] Core implementation
  - [x] Unit tests
  
  **In Progress**:
  - [ ] Integration tests
  - [ ] Documentation
  
  **Blockers**: None
  
  **ETA**: X hours remaining
  """
)
```

### On Completion

Close issue with summary:

```python
mcp_github_github_issue_write(
  method="update",
  owner="Alteriom",
  repo="painlessMesh-simulator",
  issue_number=ISSUE_NUMBER,
  state="closed",
  state_reason="completed"
)

# Add final comment
mcp_github_github_add_issue_comment(
  owner="Alteriom",
  repo="painlessMesh-simulator",
  issue_number=ISSUE_NUMBER,
  body="""
  ## ✅ Completed
  
  - Implementation: PR #___
  - Tests: XX% coverage
  - Documentation: Updated
  - Review: Approved
  
  **Next Steps**: Issue #___ (depends on this)
  """
)
```

## Code Quality Checklist

Before creating PR, verify:

- [ ] **Tests**: All tests pass, coverage ≥ 80%
- [ ] **Style**: Follows `.github/copilot-instructions.md`
- [ ] **Documentation**: All public APIs documented
- [ ] **Examples**: Usage examples provided
- [ ] **Build**: Compiles without warnings
- [ ] **Formatting**: Code properly formatted
- [ ] **Commit Messages**: Follow standard format
- [ ] **No Debugging Code**: Removed console.log, debug prints, etc.

## Commit Message Format

```text
<type>(<scope>): <subject>

<body>

<footer>
```

**Types**: feat, fix, docs, style, refactor, test, chore

**Example**:

```text
feat(core): Implement VirtualNode lifecycle management

- Add start() and stop() methods
- Implement mesh callbacks
- Add metrics collection
- Write comprehensive unit tests

Closes #4
```

## Common Pitfalls to Avoid

1. ❌ **Committing to main** - Always use feature branches
2. ❌ **Skipping tests** - TDD is mandatory
3. ❌ **Missing docs** - Document as you code
4. ❌ **Large commits** - Keep commits atomic
5. ❌ **Ignoring CI** - Fix failures immediately
6. ❌ **Breaking changes** - Maintain backward compatibility
7. ❌ **Copy-paste** - Understand and adapt code
8. ❌ **Premature optimization** - Profile first

## Session End Checklist

Before ending session:

- [ ] All changes committed and pushed
- [ ] PR created (if ready)
- [ ] Issue updated with progress
- [ ] Documentation current
- [ ] No uncommitted work
- [ ] CI passing
- [ ] Next task identified

## Emergency Procedures

### Build Failure

1. Check error messages carefully
2. Review CMake configuration
3. Verify dependencies installed
4. Check for missing files
5. Review build instructions in README

### Test Failure

1. Run failing test in isolation
2. Check test expectations
3. Verify implementation correctness
4. Update test if requirements changed
5. Document any intentional behavior changes

### Merge Conflict

1. Fetch latest main branch
2. Rebase feature branch
3. Resolve conflicts carefully
4. Re-run tests
5. Verify build still works

## Resources

- **Quick Reference**: `.github/MCP_QUICK_REFERENCE.md`
- **Full MCP Config**: `.github/MCP_CONFIGURATION.md`
- **Coding Standards**: `.github/copilot-instructions.md`
- **Technical Spec**: `docs/SIMULATOR_PLAN.md`
- **Quick Start**: `docs/SIMULATOR_QUICKSTART.md`

## Notes

Use this section for session-specific notes:

```markdown
## Session Notes - [Date]

**Goal**: [What you're working on]

**Progress**:
- [x] Completed X
- [ ] In progress Y

**Decisions**:
- Chose approach A because B
- Deferred feature C to Phase 2

**Questions**:
- Need clarification on X
- Should we consider Y?

**Next Session**:
- Continue with Z
- Review feedback on PR #___
```

---

**Template Version**: 1.0  
**Last Updated**: November 12, 2025  
**Status**: Active
