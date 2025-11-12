# GitHub MCP Quick Reference

**Quick access guide for AI agents working with painlessMesh-simulator**

## Essential Commands

### Authentication Check

```python
mcp_github_github_get_me()
# Returns: {"login": "sparck75", "id": 14064405, ...}
```

### Issue Operations

```python
# List open Phase 1 issues
mcp_github_github_list_issues(
  owner="Alteriom",
  repo="painlessMesh-simulator",
  labels=["phase-1"],
  state="OPEN"
)

# Create new issue
mcp_github_github_issue_write(
  method="create",
  owner="Alteriom",
  repo="painlessMesh-simulator",
  title="[Phase X] Task Name",
  body="## Objective\n\n...\n\n## Tasks\n\n- [ ] Task 1",
  labels=["phase-x", "enhancement"]
)

# Update issue
mcp_github_github_issue_write(
  method="update",
  owner="Alteriom",
  repo="painlessMesh-simulator",
  issue_number=123,
  state="closed",
  state_reason="completed"
)

# Add comment
mcp_github_github_add_issue_comment(
  owner="Alteriom",
  repo="painlessMesh-simulator",
  issue_number=123,
  body="Progress update: Implemented X, testing Y"
)
```

### Pull Request Operations

```python
# Create PR
mcp_github_github_create_pull_request(
  owner="Alteriom",
  repo="painlessMesh-simulator",
  title="Implement VirtualNode class",
  head="feature/virtual-node",
  base="main",
  body="Closes #4\n\n## Changes\n- Implemented VirtualNode\n- Added tests\n- Updated docs"
)

# Get PR details
mcp_github_github_pull_request_read(
  method="get",
  owner="Alteriom",
  repo="painlessMesh-simulator",
  pullNumber=10
)

# Request Copilot review
mcp_github_github_request_copilot_review(
  owner="Alteriom",
  repo="painlessMesh-simulator",
  pullNumber=10
)
```

### Repository Operations

```python
# Create branch
mcp_github_github_create_branch(
  owner="Alteriom",
  repo="painlessMesh-simulator",
  branch="feature/new-feature",
  from_branch="main"
)

# Read file
mcp_github_github_get_file_contents(
  owner="Alteriom",
  repo="painlessMesh-simulator",
  path="docs/SIMULATOR_PLAN.md"
)

# Push files
mcp_github_github_push_files(
  owner="Alteriom",
  repo="painlessMesh-simulator",
  branch="feature/new-feature",
  files=[
    {"path": "src/core/virtual_node.cpp", "content": "..."},
    {"path": "include/simulator/virtual_node.hpp", "content": "..."}
  ],
  message="feat(core): Implement VirtualNode class\n\nAdd core VirtualNode implementation with lifecycle management"
)
```

### Search Operations

```python
# Search code
mcp_github_github_search_code(
  query="VirtualNode language:cpp repo:Alteriom/painlessMesh-simulator"
)

# Search issues
mcp_github_github_search_issues(
  query="is:open label:phase-1 label:core",
  owner="Alteriom",
  repo="painlessMesh-simulator"
)
```

## Session Start Checklist

```markdown
- [ ] Verify auth: `mcp_github_github_get_me()`
- [ ] List open issues: `mcp_github_github_list_issues(...)`
- [ ] Read copilot instructions: `.github/copilot-instructions.md`
- [ ] Review simulator plan: `docs/SIMULATOR_PLAN.md`
- [ ] Check current phase: Look at issue labels
- [ ] Identify next task: Review dependencies
```

## Repository Constants

```python
OWNER = "Alteriom"
REPO = "painlessMesh-simulator"
DEFAULT_BRANCH = "main"
```

## Common Labels

- `phase-1`, `phase-2`, `phase-3`, `phase-4` - Project phases
- `core`, `infrastructure`, `testing`, `documentation` - Component types
- `c++`, `yaml`, `cli` - Technology tags
- `enhancement`, `bug`, `question` - Issue types
- `good first issue`, `help wanted` - Contributor tags

## Commit Message Format

```
<type>(<scope>): <subject>

<body>

<footer>
```

Types: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`

Example:

```
feat(core): Implement VirtualNode class

- Add VirtualNode lifecycle management
- Implement mesh callbacks
- Add metrics collection
- Write unit tests with 85% coverage

Closes #4
```

## File Paths Reference

### Documentation

- `.github/copilot-instructions.md` - Coding standards
- `docs/SIMULATOR_PLAN.md` - Technical specification
- `docs/SIMULATOR_QUICKSTART.md` - Getting started guide
- `README.md` - Project overview

### Source Code Structure

```
include/simulator/          # Public headers
  virtual_node.hpp
  node_manager.hpp
  config_loader.hpp
src/
  core/                     # Core implementation
  config/                   # Configuration
  cli/                      # CLI parser
  main.cpp                  # Entry point
test/                       # Unit tests
examples/
  scenarios/                # YAML configs
  firmware/                 # Example firmware
```

## Error Handling

### Authentication Failed

```python
# Symptom: Tool calls fail with auth error
# Solution: Check authentication
user = mcp_github_github_get_me()
# Should return user info, not error
```

### Permission Denied

```python
# Symptom: "Resource not accessible by integration"
# Solution: Verify repo access and permissions
# User sparck75 should have full access to Alteriom org
```

### Tool Not Found

```python
# Symptom: Tool not available
# Solution: Activate tool category
activate_github_commit_and_release_management()
```

## Best Practices

1. **Always work in branches** - Never commit to `main`
2. **Write tests first** - Follow TDD approach
3. **Update docs** - Keep documentation synchronized
4. **Link issues** - Use `Closes #123` in PRs
5. **Clear messages** - Write descriptive commits
6. **Review code** - Request Copilot review before merge
7. **Check CI** - Ensure tests pass

## Useful Queries

### Find all open core issues

```python
mcp_github_github_search_issues(
  query="is:open label:core repo:Alteriom/painlessMesh-simulator"
)
```

### Find PRs needing review

```python
mcp_github_github_list_pull_requests(
  owner="Alteriom",
  repo="painlessMesh-simulator",
  state="open"
)
```

### Search for specific class usage

```python
mcp_github_github_search_code(
  query="VirtualNode language:cpp repo:Alteriom/painlessMesh-simulator"
)
```

## Links

- Repository: <https://github.com/Alteriom/painlessMesh-simulator>
- Issues: <https://github.com/Alteriom/painlessMesh-simulator/issues>
- PRs: <https://github.com/Alteriom/painlessMesh-simulator/pulls>
- Full MCP Config: `.github/MCP_CONFIGURATION.md`

---

**Last Updated**: November 12, 2025  
**Status**: Active and operational
