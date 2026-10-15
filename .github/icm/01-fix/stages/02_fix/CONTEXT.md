# 02_fix — change the cause, not the symptom

One job: the smallest change at the defect's cause that turns the reproduction green without weakening it.

## Inputs
- Working (this run): `.github/icm/01-fix/stages/01_reproduce/output/reproduction.md`
- Reference (every run): `.github/icm/_shared/conventions.md`
- Reference (only when the change touches the build, the painlessMesh sources it compiles or firmware registration): `.github/icm/_shared/architecture.md`
- Reference (only when the fix needs the painlessMesh pin to move): `.github/icm/_shared/dependencies.md`

Do NOT load: the other pipelines, other runs' output, `.github/icm/_shared/documents.md`.

## Process
1. Read the reproduction and confirm its cause in the code before changing anything; if it moved, record where it really is.
2. Change the cause. Do not edit the reproduction to make it pass, and do not add a scenario to the unsupported list of `scripts/ci_integration_check.sh` to make it pass; if the reproduction was wrong, go back to stage 01.
3. Keep to [conventions.md](../../../_shared/conventions.md): C++14, the house style, a comment that says why with what was observed. Stay inside the issue: anything else you notice becomes an issue, not a commit.
4. Run the reproduction (green now), `ctest` and the integration check in the image.

## Outputs
- change.md → output/ — the cause in one paragraph; each file changed and why; the reproduction's passing output; `ctest` and the integration check results; what the fix deliberately leaves alone and the issues filed for it.

## Human check
A person reads the diff beside the reproduction: it removes the cause, not only what the test sees, and nothing was loosened to get green. Edit `change.md` if the explanation is wrong; stage 03 reads it.
