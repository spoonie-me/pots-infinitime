# pots-infinitime — Claude Code notes

## PR policy — single-dev repo

Roi is the only developer on this repo. There is no second reviewer coming, ever. So:

- Do **not** open PRs as draft by default — open them ready for review from the start, unless explicitly asked for a draft.
- Once CI is green and there are no merge conflicts, **merge the PR** (squash merge) instead of waiting for approval or asking permission. Don't leave finished, green PRs sitting open.
- Only pause and ask before merging if: CI is failing and the fix is ambiguous, the change is destructive/high-risk (schema migration, deleting data, prod config), or Roi has explicitly said "don't merge until I look."
- Drop the "watch this PR forever" posture for routine work — once merged, unsubscribe and stop polling. Only keep long-running PR watches for things that genuinely need iteration (e.g., waiting on a slow external CI, a real second reviewer if one's ever added).
