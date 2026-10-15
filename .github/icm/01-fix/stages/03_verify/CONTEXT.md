# 03_verify — checks, records, pull request

One job: the pull request a reviewer can merge, with the evidence CI will look for.

## Inputs
- Working (this run): `.github/icm/01-fix/stages/02_fix/output/change.md`
- Reference (every run): `.github/icm/_shared/build-and-test.md`
- Reference (every run): `.github/icm/_shared/review.md`
- Reference (only when the fix changes what a document says): `.github/icm/_shared/documents.md`

Do NOT load: the other pipelines, other runs' output.

## Process
1. Run what CI runs that the change can affect ([build-and-test.md](../../../_shared/build-and-test.md)): Debug and Release builds, `ctest`, the integration check; the drift build too if the change touches what painlessMesh code the build compiles.
2. Record: correct `DEVELOPMENT_STATUS.md` if the fix changes what works, a guide in `docs/` if a line there became wrong, and `.github/icm/_shared/` if a pointer moved.
3. Write the pull request: what was wrong, how you know (the reproduction), what the change does, the checks and results, and `Fixes #<n>`. No build output in the diff.
4. After the human check, open it against `main` (`gh pr create --base main --body-file <this stage's output/pull-request.md>`). Answer review findings by [review.md](../../../_shared/review.md): three bot rounds at most, then hand it to a person. The agent never merges.

## Outputs
- pull-request.md → output/ — the title and body; the checks run with results; the records updated.

## Human check
A person reads `pull-request.md` and its evidence and says whether to open it; the reviewer, not the author, merges.
