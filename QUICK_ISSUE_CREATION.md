# Quick Issue Creation Guide

## TL;DR - Create All Issues Now

Run this single command to create all Phase 1 issues:

```bash
# Authenticate first (one-time)
gh auth login

# Then create all issues
./scripts/create-phase1-issues.sh
```

## What Gets Created

5 Phase 1 implementation issues:

| # | Title | Labels | Time | Dependencies |
|---|-------|--------|------|--------------|
| 1 | [Phase 1.1] Add painlessMesh as Git Submodule | phase-1, infrastructure, good first issue | 1-2h | None |
| 2 | [Phase 1.1] Implement VirtualNode Class | phase-1, core, c++ | 4-6h | #1 |
| 3 | [Phase 1.1] Implement NodeManager Class | phase-1, core, c++ | 4-6h | #2 |
| 4 | [Phase 1.2] Implement Configuration Loader | phase-1, configuration, yaml | 6-8h | #1, #2 |
| 5 | [Phase 1.1] Implement CLI Application | phase-1, cli, c++ | 4-6h | #2, #3, #4 |

**Total**: 19-28 hours over 2 weeks

## Implementation Order

```
1. Issue #1 (Submodule)
   ↓
2. Issue #2 (VirtualNode)
   ↓
3. Issue #3 (NodeManager) + Issue #4 (ConfigLoader) [parallel]
   ↓
4. Issue #5 (CLI)
```

## One-Line Commands

If you prefer to create issues one at a time:

```bash
# Issue #1
gh issue create --repo Alteriom/painlessMesh-simulator --title "[Phase 1.1] Add painlessMesh as Git Submodule" --body-file .github/ISSUE_TEMPLATES_PHASE1/01-add-painlessmesh-submodule.md --label "phase-1,infrastructure,good first issue"

# Issue #2
gh issue create --repo Alteriom/painlessMesh-simulator --title "[Phase 1.1] Implement VirtualNode Class" --body-file .github/ISSUE_TEMPLATES_PHASE1/02-implement-virtualnode-class.md --label "phase-1,core,c++"

# Issue #3
gh issue create --repo Alteriom/painlessMesh-simulator --title "[Phase 1.1] Implement NodeManager Class" --body-file .github/ISSUE_TEMPLATES_PHASE1/03-implement-nodemanager-class.md --label "phase-1,core,c++"

# Issue #4
gh issue create --repo Alteriom/painlessMesh-simulator --title "[Phase 1.2] Implement Configuration Loader" --body-file .github/ISSUE_TEMPLATES_PHASE1/04-implement-config-loader.md --label "phase-1,configuration,yaml"

# Issue #5
gh issue create --repo Alteriom/painlessMesh-simulator --title "[Phase 1.1] Implement CLI Application" --body-file .github/ISSUE_TEMPLATES_PHASE1/05-implement-cli-application.md --label "phase-1,cli,c++"
```

## Verify Issues Created

```bash
gh issue list --repo Alteriom/painlessMesh-simulator --label phase-1
```

## Next Steps After Creation

1. Assign issues to team/agents
2. Start with Issue #1
3. Track progress in GitHub Projects
4. Use custom agents for specialized work:
   - `@cpp-simulator-agent` for C++ code
   - `@scenario-config-agent` for YAML
   - `@documentation-agent` for docs

---

**Full details**: See `ISSUES_TO_CREATE.md`
