# 01-fix — fix a defect

One run is one defect: reproduced in the `builder` image, fixed at its cause, and proposed as a pull request with the evidence. A defect may be in the simulator or in what it assumes of painlessMesh; a defect in painlessMesh itself is reported there, not patched here.

Stages, in order; inputs, outputs and checks for each are in [CONTEXT.md](CONTEXT.md):

1. [Reproduce](stages/01_reproduce/CONTEXT.md) — make it fail on purpose.
2. [Fix](stages/02_fix/CONTEXT.md) — change the cause, watch the reproduction pass.
3. [Verify](stages/03_verify/CONTEXT.md) — what CI will run, the records, the pull request.
