#!/bin/bash
# test-gh-auth.sh
# Test GitHub CLI authentication and permissions

set -e

REPO="Alteriom/painlessMesh-simulator"

echo "================================================"
echo "  GitHub CLI Authentication Test"
echo "================================================"
echo ""

# Test 1: Check if gh is installed
echo "Test 1: GitHub CLI Installation"
if command -v gh &> /dev/null; then
    gh_version=$(gh --version | head -n 1)
    echo "✓ GitHub CLI installed: $gh_version"
else
    echo "✗ GitHub CLI not installed"
    echo "  Install from: https://cli.github.com/"
    exit 1
fi
echo ""

# Test 2: Check environment variables
echo "Test 2: Environment Variables"
if [ -n "$GITHUB_TOKEN" ]; then
    echo "✓ GITHUB_TOKEN is set"
elif [ -n "$GH_TOKEN" ]; then
    echo "✓ GH_TOKEN is set"
else
    echo "✗ Neither GITHUB_TOKEN nor GH_TOKEN is set"
    echo "  For GitHub Actions, set: GH_TOKEN: \${{ github.token }}"
    echo "  For local use, run: gh auth login"
fi
echo ""

# Test 3: Authentication status
echo "Test 3: Authentication Status"
if gh auth status &> /dev/null; then
    echo "✓ Authenticated with GitHub"
    gh auth status 2>&1 | grep -E "Logged in|Token" || true
else
    echo "✗ Not authenticated"
    echo "  Run: gh auth login"
    exit 1
fi
echo ""

# Test 4: Repository access
echo "Test 4: Repository Access"
if gh repo view "$REPO" &> /dev/null; then
    echo "✓ Can access repository: $REPO"
else
    echo "✗ Cannot access repository: $REPO"
    exit 1
fi
echo ""

# Test 5: Issue permissions
echo "Test 5: Issue Permissions"
if gh issue list --repo "$REPO" --limit 1 &> /dev/null; then
    echo "✓ Can list issues"
else
    echo "✗ Cannot list issues"
    exit 1
fi
echo ""

# Test 6: API access
echo "Test 6: GitHub API Access"
if gh api user &> /dev/null; then
    username=$(gh api user | grep -o '"login": "[^"]*' | cut -d'"' -f4)
    echo "✓ API access working (user: $username)"
else
    echo "✗ Cannot access GitHub API"
    exit 1
fi
echo ""

# Test 7: Token permissions
echo "Test 7: Token Permissions"
echo "Checking scopes..."
scopes=$(gh api /user -i | grep -i "x-oauth-scopes" | cut -d: -f2 | tr ',' '\n' | sed 's/^ */  - /')
if [ -n "$scopes" ]; then
    echo "✓ Token has scopes:"
    echo "$scopes"
else
    echo "⚠ Could not determine token scopes"
fi
echo ""

echo "================================================"
echo "  All Tests Passed!"
echo "================================================"
echo ""
echo "You can now create issues using:"
echo "  ./scripts/create-phase1-issues.sh"
echo ""
echo "Or manually:"
echo "  gh issue create --repo $REPO --title \"Title\" --body \"Body\""
echo ""

exit 0
