#!/bin/bash
# create-phase1-issues.sh
# Creates GitHub issues from Phase 1 templates
# Requires: gh CLI authenticated (gh auth login or GH_TOKEN set)

set -e

REPO="Alteriom/painlessMesh-simulator"
TEMPLATES_DIR=".github/ISSUE_TEMPLATES_PHASE1"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo "================================================"
echo "  Creating Phase 1 Issues for painlessMesh Simulator"
echo "================================================"
echo ""

# Check if gh CLI is available
if ! command -v gh &> /dev/null; then
    echo -e "${RED}Error: GitHub CLI (gh) is not installed${NC}"
    echo "Install from: https://cli.github.com/"
    exit 1
fi

# Check authentication
echo "Checking GitHub CLI authentication..."
if ! gh auth status &> /dev/null; then
    echo -e "${RED}Error: Not authenticated with GitHub CLI${NC}"
    echo ""
    echo "Please authenticate using one of:"
    echo "  1. gh auth login"
    echo "  2. export GH_TOKEN=your_token"
    echo "  3. export GITHUB_TOKEN=your_token"
    exit 1
fi

echo -e "${GREEN}✓ Authenticated${NC}"
echo ""

# Verify we can access the repository
echo "Verifying repository access..."
if ! gh repo view "$REPO" &> /dev/null; then
    echo -e "${RED}Error: Cannot access repository $REPO${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Repository accessible${NC}"
echo ""

# Check if templates directory exists
if [ ! -d "$TEMPLATES_DIR" ]; then
    echo -e "${RED}Error: Templates directory not found: $TEMPLATES_DIR${NC}"
    exit 1
fi

# Count templates (exclude README.md)
template_count=$(find "$TEMPLATES_DIR" -name "*.md" ! -name "README.md" | wc -l)
echo "Found $template_count issue templates"
echo ""

# Confirm before creating
read -p "Create $template_count issues in $REPO? (y/N) " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Cancelled."
    exit 0
fi

echo ""
echo "Creating issues..."
echo ""

# Counter for created issues
created=0
failed=0

# Process each template
for template_file in "$TEMPLATES_DIR"/*.md; do
    # Skip README
    if [[ "$(basename "$template_file")" == "README.md" ]]; then
        continue
    fi
    
    echo "----------------------------------------"
    
    # Extract title (first line, remove '# ')
    title=$(head -n 1 "$template_file" | sed 's/^# //')
    
    # Extract labels from format: **Labels**: `label1`, `label2`, `label3`
    labels=$(grep -m 1 "^\*\*Labels\*\*:" "$template_file" | sed 's/.*`\(.*\)`.*/\1/' | sed 's/`, `/,/g')
    
    # Extract milestone if present
    milestone=$(grep -m 1 "^\*\*Milestone\*\*:" "$template_file" | sed 's/.*: //' || echo "")
    
    echo "Title: $title"
    echo "Labels: $labels"
    if [ -n "$milestone" ]; then
        echo "Milestone: $milestone"
    fi
    
    # Create the issue
    echo "Creating..."
    
    if [ -n "$labels" ]; then
        issue_url=$(gh issue create \
            --repo "$REPO" \
            --title "$title" \
            --body-file "$template_file" \
            --label "$labels" \
            2>&1)
    else
        issue_url=$(gh issue create \
            --repo "$REPO" \
            --title "$title" \
            --body-file "$template_file" \
            2>&1)
    fi
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ Created: $issue_url${NC}"
        created=$((created + 1))
    else
        echo -e "${RED}✗ Failed: $issue_url${NC}"
        failed=$((failed + 1))
    fi
    
    # Rate limiting - be nice to GitHub API
    sleep 2
done

echo ""
echo "================================================"
echo "  Summary"
echo "================================================"
echo -e "${GREEN}Created: $created${NC}"
if [ $failed -gt 0 ]; then
    echo -e "${RED}Failed: $failed${NC}"
fi
echo ""

if [ $created -gt 0 ]; then
    echo "View issues at: https://github.com/$REPO/issues"
    echo ""
    echo "Next steps:"
    echo "  1. Review created issues"
    echo "  2. Assign to team members or agents"
    echo "  3. Start implementation in order (Issue #1 → #5)"
fi

exit 0
