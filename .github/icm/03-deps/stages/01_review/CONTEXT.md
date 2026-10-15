# 01_review — what moved, what it can break

One job: a merge, hold or close recommendation for one dependency bump, with the evidence.

## Inputs
- Working (this run): the bump — a Dependabot pull request (`gh pr view <n> --comments`, `gh pr diff <n>`, `gh pr checks <n>`), or the painlessMesh commit or branch the pin is to move to.
- Reference (every run): `.github/icm/_shared/dependencies.md`
- Reference (only when the bump is the painlessMesh pin or the base image): `.github/icm/_shared/build-and-test.md`

Do NOT load: the other pipelines, other runs' output, `.github/icm/_shared/review.md`, `.github/icm/_shared/documents.md`.

## Process
1. Name what moved and from which version to which; for a grouped Dependabot pull request, every package in it. Read the release notes of a major.
2. Look up what it can break in [dependencies.md](../../../_shared/dependencies.md) and check that part of the diff and of the pull request's run: every job that uses a moved action, or for the base image the build, `ctest` and the integration check.
3. For the painlessMesh pin: the pull request's `Upstream Drift` run is the proof; read the upstream changes since the old pin (`git -C external/painlessMesh log --oneline <old>..<new>`) for moved `.cpp` files and changed APIs the simulator calls, and say whether `DEVELOPMENT_STATUS.md` needs a line.
4. If a bump touches `.github/workflows/icm-check.yml`, recommend closing it: that file is vendored from the ICM standard.
5. Recommend merge, hold (with what would unblock it) or close (with why). Never merge.

## Outputs
- review.md → output/ — what moved; what it can break and where that was checked; the CI result; the recommendation and its reason.

## Human check
A person reads `review.md` and the pull request's checks, then merges, holds or closes it themselves.
