# 03-deps — review a dependency bump

One run is one bump: a Dependabot pull request (the `ubuntu` base image or a GitHub Action), or a move of the painlessMesh submodule pin. The outcome is a recommendation a person acts on: merge, hold with a reason, or close.

Stages; inputs, outputs and checks are in [CONTEXT.md](CONTEXT.md):

1. [Review](stages/01_review/CONTEXT.md) — what moved, what it can break, what the pull request's own CI proved.
