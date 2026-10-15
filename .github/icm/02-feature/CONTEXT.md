# 02-feature — the pipeline

Agree what it is, build it with its tests, prove it and record it.

| Stage | Job | Input | Output | Human check |
|---|---|---|---|---|
| [`01_spec`](stages/01_spec/CONTEXT.md) | what the issue asks, and its proof | the request | `output/spec.md` | A person approves the spec, or edits it: the scope matches the issue and the proof would fail without the capability. |
| [`02_implement`](stages/02_implement/CONTEXT.md) | the code and its tests | 01_spec's `spec.md` | `output/change.md` | A person reads the diff beside the spec: it does what the spec says and no more, and the tests would fail without it. |
| [`03_verify`](stages/03_verify/CONTEXT.md) | checks, documents, pull request | 02_implement's `change.md` | `output/pull-request.md` | A person reads `pull-request.md` and the documents changed and says whether to open it; the reviewer, not the author, merges. |
