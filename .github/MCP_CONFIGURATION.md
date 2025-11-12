# MCP (Model Context Protocol) Server Configuration

## Issue: GitHub CLI Not Authenticated

The GitHub MCP server requires authentication to create issues, manage repositories, and perform other GitHub operations.

## Problem

When attempting to create GitHub issues via `gh issue create`, the following error occurs:
```
gh: To use GitHub CLI in a GitHub Actions workflow, set the GH_TOKEN environment variable. Example:
  env:
    GH_TOKEN: ${{ github.token }}
```

## Solution

The GitHub Copilot Coding Agent needs the `GITHUB_TOKEN` or `GH_TOKEN` environment variable to be set in the execution environment.

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

## Verification Checklist

- [ ] `GITHUB_TOKEN` or `GH_TOKEN` environment variable is set
- [ ] Token has required permissions (contents, issues, pull-requests)
- [ ] `gh auth status` shows authenticated
- [ ] `gh issue create` command works
- [ ] Repository settings allow Actions to create issues
- [ ] MCP server can access GitHub API

## Troubleshooting

### Error: "You are not logged into any GitHub hosts"

**Cause**: GitHub CLI not authenticated  
**Solution**: Set `GH_TOKEN` environment variable or run `gh auth login`

### Error: "Resource not accessible by integration"

**Cause**: Token lacks required permissions  
**Solution**: Update workflow permissions or repository settings

### Error: "Not Found"

**Cause**: Incorrect repository name or token lacks access  
**Solution**: Verify repository name and token permissions

## Next Steps

1. **Immediate**: Use issue templates (manual creation or script)
2. **Short-term**: Configure agent environment with `GITHUB_TOKEN`
3. **Long-term**: Integrate with workflow automation

## Alternative: Issue Creation Script

Until MCP server is properly configured, use this script:

```bash
#!/bin/bash
# create-issues-from-templates.sh

# Requires: gh CLI authenticated

TEMPLATES_DIR=".github/ISSUE_TEMPLATES_PHASE1"

for file in "$TEMPLATES_DIR"/*.md; do
  if [ "$file" = "$TEMPLATES_DIR/README.md" ]; then
    continue
  fi
  
  # Extract title (first line, remove '# ')
  title=$(head -n 1 "$file" | sed 's/^# //')
  
  # Extract labels (second line format: **Labels**: `label1`, `label2`)
  labels=$(head -n 3 "$file" | grep "Labels" | sed 's/.*: `//;s/`.*//;s/, /,/g')
  
  echo "Creating issue: $title"
  echo "Labels: $labels"
  
  gh issue create \
    --repo Alteriom/painlessMesh-simulator \
    --title "$title" \
    --body-file "$file" \
    --label "$labels"
  
  if [ $? -eq 0 ]; then
    echo "✅ Created: $title"
  else
    echo "❌ Failed: $title"
  fi
  
  # Rate limiting
  sleep 2
done

echo "Done!"
```

## References

- **GitHub CLI Documentation**: https://cli.github.com/manual/
- **GitHub Actions Token**: https://docs.github.com/en/actions/security-guides/automatic-token-authentication
- **MCP Specification**: https://github.com/anthropics/model-context-protocol

---

**Status**: Configuration needed  
**Priority**: Medium (workaround available with templates)  
**Impact**: Cannot create issues via CLI until resolved
