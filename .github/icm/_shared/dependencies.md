---
verified_at: 2026-10-15
verified_commit: b28ff129d49b7a79cae81e69b205c1bf3821234b
---

# Dependencies — the painlessMesh pin, Dependabot, and what each bump can break

## painlessMesh, the library under test

- A submodule at `external/painlessMesh` (`.gitmodules`), tracking `Feat/next-release` for the v2.0.0 work; the comment there says to move back to `main` once v2.0.0 merges. The pinned commit is what CI builds; `DEVELOPMENT_STATUS.md` names the line it is on.
- `.github/workflows/upstream-drift.yml` builds, tests and runs the integration check against painlessMesh `main` and `Feat/next-release` regardless of the pin, weekly and on any pull request that moves the pin; on the schedule, a failure opens (or comments on) the issue "Simulator no longer builds against painlessMesh upstream". That issue is a fix run (the `ci-red` route), and its answer is a fix plus a pin move, or a recorded reason the drift is accepted.
- Moving the pin: `git -C external/painlessMesh fetch origin <branch>`, check out the commit, then commit the gitlink. What breaks first is a link error when upstream moves code out of headers into a new `.cpp` (add it to the `_pm_src` loop in `CMakeLists.txt`), then call sites in `src/core/` and `src/firmware/`. `docs/V2_RELEASE_REVIEW.md` "Answer" is the worked example of a 680-commit move.
- What the simulator does not yet model of v2.0.0 (ACK, message tracking, gateway) is listed in `DEVELOPMENT_STATUS.md` "Not modelled"; a pin move does not add it.

## Dependabot

`.github/dependabot.yml`: weekly, two ecosystems: `docker` (the `ubuntu` base of `Dockerfile`) and `github-actions` (the actions in `ci.yml` and `upstream-drift.yml`). Minor and patch updates are grouped into one pull request per ecosystem; majors come one by one. Nothing else is managed: Boost and yaml-cpp come from the image's apt packages, Catch2 is pinned to a tag in `CMakeLists.txt`.

| Bump | What it can break | Proof it did not |
|---|---|---|
| `ubuntu` base image | compiler, Boost and yaml-cpp versions together; the `FindBoost` policy (`CMP0167` in `CMakeLists.txt`) | the pull request's `CI Success`: build, `ctest` and the integration check all run in the new image |
| `actions/*`, `docker/*`, `codecov/*` | inputs or defaults of a step; artifact names and retention (`upload-artifact`) | the pull request's own run of every job that uses the action; Codecov is allowed to fail, so read its step |
| the actions of `icm-check.yml` | nothing to merge here: they are pinned by commit and vendored unchanged from the ICM standard; take the standard's next version instead | the standard's own pull request |
