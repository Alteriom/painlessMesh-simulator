# 02_implement — the code and its tests

One job: the capability as the spec says, with the tests that prove it, green in the `builder` image.

## Inputs
- Working (this run): `.github/icm/02-feature/stages/01_spec/output/spec.md`
- Reference (every run): `.github/icm/_shared/conventions.md`
- Reference (every run): `.github/icm/_shared/build-and-test.md`
- Reference (only when the change touches the build, the painlessMesh sources it compiles or firmware registration): `.github/icm/_shared/architecture.md`

Do NOT load: the other pipelines, other runs' output, `.github/icm/_shared/documents.md`.

## Process
1. Write the tests from the spec first and watch them fail in the image.
2. Implement in the places the spec names, by [conventions.md](../../../_shared/conventions.md). A new `test/test_*.cpp` joins the `simulator_tests` list in `CMakeLists.txt`; a new firmware self-registers; a newly implemented action comes off the unsupported list of `scripts/ci_integration_check.sh`.
3. A new shipped scenario under `examples/scenarios/` must pass `--validate-only` and, if the integration check runs it, its assertions.
4. Run Debug and Release builds, `ctest` and the integration check in the image; keep the output. Do not grow the change past the spec: a reviewer's or your own extra idea is an issue.

## Outputs
- change.md → output/ — each file changed and why, against the spec; the tests added; the build, `ctest` and integration results from the image; anything the spec asked that is not done, and why.

## Human check
A person reads the diff beside the spec: it does what the spec says and no more, and the tests would fail without it.
