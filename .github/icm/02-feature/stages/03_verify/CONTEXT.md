# 03_verify — checks, documents, pull request

One job: the pull request a reviewer can merge, with the user's documents brought along.

## Inputs
- Working (this run): `.github/icm/02-feature/stages/02_implement/output/change.md`
- Reference (every run): `.github/icm/_shared/documents.md`
- Reference (every run): `.github/icm/_shared/review.md`

Do NOT load: the other pipelines, other runs' output.

## Process
1. Confirm the checks of stage 02 still pass on the final tree (rebuild in the image if anything changed since).
2. Bring the documents along, by [documents.md](../../../_shared/documents.md): `docs/CONFIGURATION_GUIDE.md` for a new key or action, the guide of the area (network, events, firmware), `README.md` "Command-Line Options" for a CLI option, and `DEVELOPMENT_STATUS.md` for what now works.
3. Write the pull request: the issue it closes, what it adds, how a scenario uses it, the checks and results, the follow-up issues for what was left out. No build output in the diff.
4. After the human check, open it against `main`. Answer review findings by [review.md](../../../_shared/review.md): three bot rounds at most, out-of-scope suggestions become issues, then hand it to a person. The agent never merges.

## Outputs
- pull-request.md → output/ — the title and body; the checks with results; the documents updated; the follow-up issues.

## Human check
A person reads `pull-request.md` and the documents changed and says whether to open it; the reviewer, not the author, merges.
