# MCP (Model Context Protocol) Server Configuration

## ✅ Status: GitHub MCP Server Configured

The GitHub MCP server is now properly configured and authenticated. The agent has full access to GitHub operations including issue management, pull requests, repository operations, and search capabilities.

**Last Updated**: November 12, 2025

## Available GitHub MCP Tools

### ✅ Issue Management
- `mcp_github_github_issue_write` - Create and update issues
- `mcp_github_github_issue_read` - Read issue details, comments, sub-issues
- `mcp_github_github_list_issues` - List repository issues with filters
- `mcp_github_github_search_issues` - Search issues across repositories
- `mcp_github_github_add_issue_comment` - Add comments to issues
- `mcp_github_github_sub_issue_write` - Manage sub-issues
- `mcp_github_github_list_issue_types` - Get valid issue types for organization
- `mcp_github_github_assign_copilot_to_issue` - Assign Copilot coding agent to issues

### ✅ Pull Request Management
- `mcp_github_github_create_pull_request` - Create new PRs
- `mcp_github_github_update_pull_request` - Update existing PRs
- `mcp_github_github_list_pull_requests` - List PRs with filters
- `mcp_github_github_search_pull_requests` - Search PRs across repositories
- `mcp_github_github_pull_request_read` - Get PR details, diff, status, reviews
- `mcp_github_github_merge_pull_request` - Merge PRs
- `mcp_github_github_update_pull_request_branch` - Update PR branch
- `mcp_github_github_pull_request_review_write` - Create/submit reviews
- `mcp_github_github_add_comment_to_pending_review` - Add review comments
- `mcp_github_github_request_copilot_review` - Request AI code review

### ✅ Repository Management
- `mcp_github_github_create_repository` - Create new repositories
- `mcp_github_github_fork_repository` - Fork repositories
- `mcp_github_github_create_branch` - Create branches
- `mcp_github_github_list_branches` - List repository branches
- `mcp_github_github_list_tags` - List git tags
- `mcp_github_github_get_file_contents` - Read files from GitHub
- `mcp_github_github_create_or_update_file` - Create/update single files
- `mcp_github_github_delete_file` - Delete files
- `mcp_github_github_push_files` - Push multiple files in one commit

### ✅ Code Search
- `mcp_github_github_search_code` - Search code across GitHub
- `mcp_github_github_search_repositories` - Find repositories
- `mcp_github_github_search_users` - Find GitHub users

### ✅ Commit & Release Management
- Available through `activate_github_commit_and_release_management`
- Get commit details, list commits, manage releases

### ✅ Team & Label Management
- `mcp_github_github_get_teams` - Get user's teams
- `mcp_github_github_get_team_members` - Get team members
- `mcp_github_github_get_label` - Get repository labels
- `mcp_github_github_get_me` - Get authenticated user details

## Quick Start for Future Agents

When starting a new coding session, agents should:

### 1. Verify GitHub Access

```bash
# Check authentication
mcp_github_github_get_me()  # Returns authenticated user info
```

### 2. Understand Repository Context

```bash
# List existing issues to understand work in progress
mcp_github_github_list_issues(owner="Alteriom", repo="painlessMesh-simulator", state="OPEN")

# Check recent pull requests
mcp_github_github_list_pull_requests(owner="Alteriom", repo="painlessMesh-simulator", state="open")

# Review repository structure
mcp_github_github_get_file_contents(owner="Alteriom", repo="painlessMesh-simulator", path="/")
```

### 3. Create Issues Programmatically

```python
# Example: Create a new issue
mcp_github_github_issue_write(
  method="create",
  owner="Alteriom",
  repo="painlessMesh-simulator",
  title="[Phase 2] Implement Feature X",
  body="Detailed description...",
  labels=["phase-2", "enhancement"]
)
```

### 4. Work with Pull Requests

```python
# Create a branch and PR
mcp_github_github_create_branch(owner="Alteriom", repo="painlessMesh-simulator", branch="feature/new-feature")
mcp_github_github_push_files(owner="Alteriom", repo="painlessMesh-simulator", branch="feature/new-feature", files=[...])
mcp_github_github_create_pull_request(owner="Alteriom", repo="painlessMesh-simulator", head="feature/new-feature", base="main", title="...")
```

### 5. Assign Work to Copilot Coding Agent

```python
# For complex implementation tasks
mcp_github_github_assign_copilot_to_issue(owner="Alteriom", repo="painlessMesh-simulator", issueNumber=123)
```

## Best Practices for AI Agents

### Issue Creation

1. **Use Templates**: Reference issue templates in `.github/ISSUE_TEMPLATES_PHASE1/` as models
2. **Include Details**: Add acceptance criteria, dependencies, references
3. **Add Labels**: Use appropriate labels (`phase-1`, `core`, `c++`, etc.)
4. **Link Issues**: Reference related issues using `#issue_number`
5. **Estimate Time**: Include time estimates for planning

### Code Changes

1. **Work in Branches**: Never commit directly to `main`
2. **Atomic Commits**: One logical change per commit
3. **Clear Messages**: Follow commit message format in copilot-instructions.md
4. **Review Before Push**: Use local tools to verify changes

### Documentation

1. **Update as You Go**: Keep docs in sync with code changes
2. **Reference Specs**: Link to `docs/SIMULATOR_PLAN.md` and other relevant docs
3. **Code Examples**: Include practical examples in API docs
4. **Changelog**: Track significant changes

### Testing

1. **Write Tests First**: Follow TDD approach per copilot-instructions.md
2. **Run Tests**: Verify tests pass before committing
3. **Coverage**: Maintain 80%+ code coverage
4. **Integration Tests**: Test multi-component interactions

## Repository Access Checklist

### ✅ Capabilities Now Available

- [x] Create, update, and close issues
- [x] Add comments to issues and PRs
- [x] Create and manage pull requests
- [x] Review code and request changes
- [x] Create branches and push code
- [x] Read repository files
- [x] Search code, issues, and PRs
- [x] Assign Copilot coding agent to issues
- [x] Manage labels and milestones
- [x] Fork and create repositories

### 🔄 Recommended Improvements

For even better agent performance, consider:

1. **GitHub Actions Integration**
   - Set up CI/CD workflows (see Phase 1 planning)
   - Enable automatic testing on PR creation
   - Add linting and formatting checks

2. **Issue Templates**
   - Create `.github/ISSUE_TEMPLATE/` forms
   - Standardize bug reports and feature requests
   - Guide contributors with structured input

3. **Branch Protection Rules**
   - Require PR reviews before merge
   - Require CI checks to pass
   - Prevent direct pushes to `main`

4. **Project Boards**
   - Set up GitHub Projects for task tracking
   - Create automation rules (auto-move issues)
   - Link to milestones for phases

5. **Custom Labels**
   - Ensure all needed labels exist: `phase-1`, `phase-2`, `core`, `infrastructure`, `testing`, etc.
   - Use consistent label colors
   - Document label meanings

6. **MCP Server Extensions**
   - Consider adding database MCP for metrics storage
   - Add filesystem MCP for local dev workflow
   - Integrate testing frameworks

## Authentication Details

### Current Setup

- **Method**: GitHub App authentication via VS Code
- **Scope**: Full repository access for Alteriom organization
- **User**: `sparck75` (authenticated)
- **Permissions**: Read/write on issues, PRs, code, etc.

### For GitHub Actions Workflows

If this agent runs within a GitHub Actions workflow, ensure the token is passed:

```yaml
# .github/workflows/copilot-agent.yml
name: Copilot Agent

on:
  workflow_dispatch:
  issues:
    types: [opened, labeled]

jobs:
  copilot-agent:
    runs-on: ubuntu-latest
    permissions:
      contents: write
      issues: write
      pull-requests: write
    
    env:
      # Required for GitHub CLI operations
      GH_TOKEN: ${{ github.token }}
      # OR
      GITHUB_TOKEN: ${{ github.token }}
    
    steps:
      - uses: actions/checkout@v3
      
      - name: Run Copilot Agent
        run: |
          # Agent operations here
          gh issue create --title "Test" --body "Body"
```

### For Direct Execution

If running the agent directly (not in GitHub Actions):

```bash
# Authenticate with GitHub CLI
gh auth login

# Or set token environment variable
export GH_TOKEN="your_github_token"
export GITHUB_TOKEN="your_github_token"

# Verify authentication
gh auth status
```

## Required Permissions

The token needs these permissions:
- **contents**: write (for committing code)
- **issues**: write (for creating/updating issues)
- **pull-requests**: write (for managing PRs)

## Configuration Files

### 1. Repository Settings

Ensure the repository has GitHub Actions enabled:
- Go to repository Settings → Actions → General
- Enable "Allow all actions and reusable workflows"
- Set "Workflow permissions" to "Read and write permissions"

### 2. Copilot Agent Configuration

The agent should have access to the GitHub MCP server with proper authentication.

## Testing MCP Server Configuration

### Test GitHub CLI Authentication

```bash
# Check if authenticated
gh auth status

# Test issue creation
gh issue create \
  --repo Alteriom/painlessMesh-simulator \
  --title "Test Issue" \
  --body "Testing MCP configuration" \
  --label "test"

# Should succeed if properly configured
```

### Test Environment Variables

```bash
# Check if token is available
echo ${GITHUB_TOKEN:+Token is set} || echo "Token NOT set"
echo ${GH_TOKEN:+Token is set} || echo "Token NOT set"

# Verify token has correct permissions
gh api user
gh api repos/Alteriom/painlessMesh-simulator
```

## Reference: Working Configuration

The `alteriom-firmware` repository has this working configuration. Key differences:

1. **Token Available**: Environment has `GITHUB_TOKEN` or `GH_TOKEN` set
2. **Permissions**: Repository settings allow Actions to create issues
3. **MCP Server**: Properly configured with GitHub authentication

## Implementation for This Repository

### Option 1: Agent Environment Configuration

If the Copilot Coding Agent runs in a managed environment, request:

1. Enable `GITHUB_TOKEN` in the agent's execution environment
2. Grant permissions: `contents: write`, `issues: write`, `pull-requests: write`
3. Ensure token is passed to MCP server

### Option 2: Workflow Integration

Create a workflow that:
1. Triggers on agent events
2. Sets up environment with proper token
3. Allows agent to use GitHub CLI

```yaml
# .github/workflows/agent-support.yml
name: Agent Support

on:
  workflow_dispatch:

jobs:
  setup-agent:
    runs-on: ubuntu-latest
    permissions:
      contents: write
      issues: write
      pull-requests: write
    
    env:
      GH_TOKEN: ${{ github.token }}
    
    steps:
      - uses: actions/checkout@v3
      
      - name: Configure GitHub CLI
        run: |
          gh auth status
          echo "GitHub CLI is authenticated"
      
      - name: Test Issue Creation
        run: |
          gh issue list --limit 1
          echo "Issue operations available"
```

## Verification Checklist for New Sessions

When starting a new agent session, verify:

- [ ] GitHub authentication works: `mcp_github_github_get_me()` returns user info
- [ ] Can list repository issues: `mcp_github_github_list_issues(...)` succeeds
- [ ] Can read repository files: `mcp_github_github_get_file_contents(...)` works
- [ ] Repository access confirmed: Alteriom/painlessMesh-simulator accessible
- [ ] Documentation reviewed: Read `.github/copilot-instructions.md`
- [ ] Project plan understood: Reviewed `docs/SIMULATOR_PLAN.md`

## Agent Workflow Best Practices

### Starting a Task

1. **Understand Context**: Read relevant documentation and issue descriptions
2. **Check Dependencies**: Verify prerequisite issues are completed
3. **Review Existing Code**: Use search tools to understand current implementation
4. **Plan Approach**: Break down complex tasks into steps
5. **Create Branch**: Work in a feature branch, never directly on `main`

### During Implementation

1. **Follow TDD**: Write tests first per `.github/copilot-instructions.md`
2. **Commit Frequently**: Small, atomic commits with clear messages
3. **Update Documentation**: Keep docs synchronized with code changes
4. **Run Tests**: Verify tests pass before pushing
5. **Code Review**: Self-review changes before creating PR

### Completing a Task

1. **Final Testing**: Run full test suite
2. **Update Issues**: Add comments about progress and decisions
3. **Create PR**: Write clear PR description linking to issues
4. **Request Review**: Ask for Copilot review or human review
5. **Address Feedback**: Respond to review comments promptly

## Environment Variables Reference

For workflows and automation, these environment variables are used:

```yaml
env:
  GH_TOKEN: ${{ github.token }}           # For GitHub CLI
  GITHUB_TOKEN: ${{ github.token }}       # For GitHub API
  GITHUB_REPOSITORY: Alteriom/painlessMesh-simulator
  GITHUB_REF: refs/heads/main
```

## Common Issues & Solutions

### Cannot Create Issues

**Symptom**: `mcp_github_github_issue_write` fails  
**Solution**: Check authentication with `mcp_github_github_get_me()`

### Permission Denied

**Symptom**: "Resource not accessible by integration"  
**Solution**: Verify user has write access to repository

### Tool Not Available

**Symptom**: Tool not available in session  
**Solution**: Activate tool category first (e.g., `activate_github_commit_and_release_management`)

## References

- [GitHub CLI Documentation](https://cli.github.com/manual/)
- [GitHub Actions Token](https://docs.github.com/en/actions/security-guides/automatic-token-authentication)
- [MCP Specification](https://github.com/anthropics/model-context-protocol)
- [VS Code GitHub Copilot Extension](https://marketplace.visualstudio.com/items?itemName=GitHub.copilot)

## Future Session Template

Use this checklist when starting a new agent session:

```markdown
## Session Initialization Checklist

- [ ] Verify GitHub authentication: `mcp_github_github_get_me()`
- [ ] Review open issues: `mcp_github_github_list_issues(...)`
- [ ] Check documentation: Read `.github/copilot-instructions.md`
- [ ] Review project plan: Read `docs/SIMULATOR_PLAN.md`
- [ ] Check current phase: Review issue labels and milestones
- [ ] Identify blockers: Look for dependency issues
- [ ] Plan work: Create or update issues as needed
- [ ] Set up branch: Create feature branch for changes
- [ ] Run tests: Verify build system works
- [ ] Start implementation: Follow TDD approach
```

## Quick Commands Reference

### Issue Management

```python
# List all Phase 1 issues
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
  body="Description...",
  labels=["phase-x", "type"]
)

# Close issue with reason
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
  body="Status update..."
)
```

### Pull Request Management

```python
# Create PR
mcp_github_github_create_pull_request(
  owner="Alteriom",
  repo="painlessMesh-simulator",
  title="Implement Feature X",
  head="feature/feature-x",
  base="main",
  body="Closes #123\n\n## Changes\n- ..."
)

# Get PR details
mcp_github_github_pull_request_read(
  method="get",
  owner="Alteriom",
  repo="painlessMesh-simulator",
  pullNumber=456
)

# Request Copilot review
mcp_github_github_request_copilot_review(
  owner="Alteriom",
  repo="painlessMesh-simulator",
  pullNumber=456
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
  path="README.md"
)

# Push multiple files
mcp_github_github_push_files(
  owner="Alteriom",
  repo="painlessMesh-simulator",
  branch="feature/new-feature",
  files=[
    {"path": "src/file1.cpp", "content": "..."},
    {"path": "include/file1.hpp", "content": "..."}
  ],
  message="Add new feature implementation"
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
  query="is:open label:phase-1",
  owner="Alteriom",
  repo="painlessMesh-simulator"
)
```

## Summary

The GitHub MCP server is fully operational and provides comprehensive repository management capabilities. Agents can:

- ✅ Create and manage issues programmatically
- ✅ Work with pull requests end-to-end
- ✅ Push code changes and create branches
- ✅ Search code, issues, and repositories
- ✅ Assign work to Copilot coding agents
- ✅ Request automated code reviews
- ✅ Manage all aspects of the repository

This configuration enables fully autonomous agent workflows while maintaining code quality through proper testing, review, and CI/CD integration.

## Additional MCP Servers

Consider adding these MCP servers for enhanced capabilities:

### Database MCP
- Store metrics and simulation results
- Query historical performance data
- Track test execution history

### Filesystem MCP
- Direct local file operations
- Faster than GitHub API for bulk operations
- Better for development workflows

### Testing Framework MCP
- Integrated test execution
- Coverage reporting
- Performance benchmarking

### Container MCP (Already Available)
- Manage Docker containers for testing
- Run simulations in isolated environments
- Test deployment scenarios

---

**Status**: ✅ Fully Configured and Operational  
**Priority**: N/A (Working)  
**Last Verified**: November 12, 2025  
**Authenticated User**: sparck75  
**Access Level**: Full repository access (Alteriom organization)  
**MCP Version**: GitHub MCP Server v1.x (via VS Code GitHub Copilot Extension)

For quick reference, see: `.github/MCP_QUICK_REFERENCE.md`
