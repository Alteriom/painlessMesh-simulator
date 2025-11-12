# GitHub Actions Workflows

## Active Workflows

### ci-docker.yml
Main CI/CD pipeline using Docker for reproducible builds.

**Features:**
- Docker-based builds for consistent environments
- Multi-platform support (Linux, Windows cross-compile, macOS native)
- Build matrix: gcc/clang × Debug/Release (4 combinations)
- Automated testing and coverage
- Docker layer caching for faster builds

**Triggers:**
- Push to master, main, develop, feature/** branches
- Pull requests to master, main, develop
- Nightly cron schedule (2 AM UTC)

## Disabled Workflows

### ci.yml (Disabled)
The original CI workflow has been disabled to avoid duplicate builds.

The Docker-based workflow (ci-docker.yml) provides all the same functionality plus:
- Reproducible builds across all environments
- Faster builds with layer caching
- Better dependency management
- Multi-platform support

## Build Matrix

The Linux builds use a matrix strategy to test multiple configurations:
- **Compilers**: GCC, Clang
- **Build Types**: Debug, Release
- **Total combinations**: 4 builds per run

This ensures code works correctly across different compilers and build configurations.

To reduce CI time during development, you can:
1. Push to a non-CI branch (not matching the trigger patterns)
2. Use `[skip ci]` in commit messages
3. Temporarily reduce the matrix in ci-docker.yml

## Permissions

The workflow requires the following permissions:
- `contents: read` - Read repository contents
- `actions: write` - Upload artifacts and use caching
