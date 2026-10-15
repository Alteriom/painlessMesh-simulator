# Shared reference

What every job on this repository draws on. Each file points at the repository file that owns a fact; follow the pointer for the detail, and when the two disagree the owning file is right.

| File | Answers |
|---|---|
| [architecture.md](architecture.md) | where each question is answered in `src/` and `include/simulator/`; what the build depends on that is easy to break; what works today |
| [build-and-test.md](build-and-test.md) | building and testing in the `builder` image, what each CI job runs and when, what counts as evidence |
| [conventions.md](conventions.md) | C++14 and the house style, tests, pull requests, build output |
| [review.md](review.md) | the review contract: scope, severity floor, no ratchet, the round budget, the terminal state, where findings go |
| [dependencies.md](dependencies.md) | the painlessMesh pin and the drift job, Dependabot, what each bump can break |
| [documents.md](documents.md) | every document a person reads here, who owns its facts, and the known drift |

Each stage's contract names the files it loads.
