# Copilot instructions — painlessMesh-simulator

Read `AGENTS.md` first. It is the entry for every agent on this repository: what the simulator is, where things live, which job to take for what just happened, and the review guidelines. The facts live once, in `.github/icm/_shared/`: the code map (`architecture.md`), building and testing in the `builder` image (`build-and-test.md`), the house style (`conventions.md`), the review contract (`review.md`), the painlessMesh pin and Dependabot (`dependencies.md`), the documents (`documents.md`). When this file and those disagree, they are right.

## For Copilot specifically

- **The coding agent** works one issue per pull request, as the matching pipeline of `AGENTS.md` describes: the issue is the scope, and anything else you notice becomes a new issue. Build and run the tests in the `builder` image of `Dockerfile`, as CI does; on a bare runner six partition tests fail for environment reasons.
- **Code review**: follow `AGENTS.md` "Review guidelines" exactly. Judge against the linked issue, block only on a P0/P1 with a failing scenario, never raise a finding against code your previous round asked for, and stop at the three-round budget.
- **Custom agents** in `.github/agents/` narrow you to one area (C++ core, scenarios, firmware, documents); each starts from `AGENTS.md` too.
- **Suggestions**: C++14 only (`CMakeLists.txt` sets it; no `std::optional`, use `boost::optional`), two-space indent, the naming in `.github/icm/_shared/conventions.md`. Review and test what you accept; never commit build output.
- **When asking for help**, include the scenario or test, the full error, what you tried, and whether it ran in the image.
