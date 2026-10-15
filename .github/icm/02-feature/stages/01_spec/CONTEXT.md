# 01_spec — what the issue asks, and its proof

One job: a spec a person can approve, anchored to the issue, naming how a scenario will declare the capability and how a run will prove it.

## Inputs
- Working (this run): the request — the issue (`gh issue view <n> --comments`), or a fix run's `reproduction.md` that turned out to need a capability.
- Reference (every run): `.github/icm/_shared/architecture.md`
- Reference (only when the capability adds or changes a scenario key or an event action): `docs/CONFIGURATION_GUIDE.md`
- Reference (only when the capability is a firmware): `docs/FIRMWARE_DEVELOPMENT_GUIDE.md`

Do NOT load: the other pipelines, other runs' output, `.github/icm/_shared/review.md`, the planning-era documents.

## Process
1. Restate what the issue asks, and what it does not: anything beyond it goes in a list of follow-up issues, not in this spec.
2. Find where it lands with [architecture.md](../../../_shared/architecture.md): the parser (`src/config/config_loader.cpp`), the event factory and a class under `src/scenario/events/`, `NetworkSimulator`, a firmware with `REGISTER_FIRMWARE`, the CLI. If the action is already parsed but has no runtime class (`DEVELOPMENT_STATUS.md` "Unimplemented event actions"), the spec says which scenarios it unblocks in `scripts/ci_integration_check.sh`.
3. Write the scenario lines a user will write, and the assertion a run will be held to: a Catch2 case, and for run-level behaviour a check in `scripts/ci_integration_check.sh`.
4. Note what painlessMesh must offer for it at the pinned commit; if it needs a newer painlessMesh, that is a pin move first (the `deps` route).

## Outputs
- spec.md → output/ — the issue and what is out of scope (with follow-up issues); where it lands, file by file; the scenario lines; the tests and assertions that will prove it; the painlessMesh it needs.

## Human check
A person approves the spec, or edits it: the scope matches the issue and the proof would fail without the capability.
