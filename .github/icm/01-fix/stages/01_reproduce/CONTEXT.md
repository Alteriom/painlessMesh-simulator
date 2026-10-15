# 01_reproduce — reproduce the reported defect

One job: turn the report into something that fails on demand, in the `builder` image, for the reason reported.

## Inputs
- Working (this run): the report — read whole with what it links: an issue (`gh issue view <n> --comments`), a red run (`gh run view <run-id> --log-failed`), the drift issue `Upstream Drift` opened, a scenario that does the wrong thing.
- Reference (every run): `.github/icm/_shared/architecture.md`
- Reference (every run): `.github/icm/_shared/build-and-test.md`
- Reference (only when the report is the upstream drift issue or names the painlessMesh pin): `.github/icm/_shared/dependencies.md`

Do NOT load: the other pipelines, other runs' output, `.github/icm/_shared/review.md`, the planning-era documents.

## Process
1. Restate the report as observed versus expected, with where it was seen: a scenario and its command line, a test, a CI job, the drift job and the painlessMesh branch it built.
2. Find the code path with [architecture.md](../../../_shared/architecture.md) and name the suspected cause as `path:line`. Check `DEVELOPMENT_STATUS.md` "What does not work yet" first: an unimplemented action or an unmodelled surface is a feature, not a defect (the `fix-to-feature` route).
3. Write the failing test: a `TEST_CASE` in `test/`, or for run-level behaviour a scenario plus an assertion in `scripts/ci_integration_check.sh`. It must drive the simulator's code, never a copy of it.
4. Run it in the `builder` image ([build-and-test.md](../../../_shared/build-and-test.md)) and keep the failing output. A failure only outside the image (for example "bind: Permission denied" on a bare runner) is not a reproduction: say so, and stop.

## Outputs
- reproduction.md → output/ — the restated report; suspected cause (`path:line`); the test or scenario and the command; the failing output and that it ran in the image; what it does not cover (hardware, radio timing).

## Human check
A person runs the command, or reads the output, and confirms it fails for the reported reason and not a neighbouring one.
