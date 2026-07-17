# Ground-Truth Capture Requests

This is the append-only queue for questions that cannot be answered from the public
repository, immutable gateway snapshot, deterministic native fixtures, or existing
capture artifacts. Requests ask an operator with access to the original game and
capture harness for evidence; they never authorize guessing missing behavior.

## Request contract

Every request is one `##` section using the exact fields below. Append new sections at
the end of **Request ledger** without editing or reordering earlier sections.

~~~markdown
## GTR-YYYYMMDD-short-slug — `FOURCC`

- Status: `open`
- Target: `FOURCC` / class or subsystem
- Blocking question: one bounded question that the repository cannot answer
- Evidence requested:
  - exact trace, field dump, screenshot, or short capture needed
- Suggested capture: bounded reproduction path; `operator choice` when unknown
- Acceptance: observable facts that would unblock implementation or classification
- Deliver to: repository path(s) where sanitized evidence and conclusions belong
- Idempotency key: `ground-truth-YYYYMMDD-short-slug`
~~~

Allowed status values are `open`, `captured`, `resolved`, and `declined`. Request IDs
and idempotency keys use lowercase ASCII letters, digits, and hyphens, with a slug no
longer than 42 characters. A request must not include credentials, private host
details, personal correspondence, copyrighted
asset dumps, or claims unsupported by checked-in evidence. Capture output must follow
the repository privacy and asset rules before it is committed.

The authenticated caller identity is not a request field. The gateway-generated
`Proposed-by` commit trailer and durable `open_pr` audit record are authoritative, so a
caller cannot self-assert another contributor's identity in this file.

## Gateway append workflow

`request_ground_truth` is a composition of the existing `fetch`, `check_status`, and
`open_pr` tools, not another repository-write tool:

1. Search for and fetch this file from the active immutable engine snapshot.
2. Call `check_status(branch="master")` and require its resolved commit to equal the
   fetched snapshot commit. If they differ, stop and wait for a snapshot refresh.
3. Append exactly one schema-valid request section. Preserve every prior byte except
   the final-newline normalization needed for the append.
4. Call `open_pr` with one file, `docs/ground_truth_requests.md`; a fresh
   `contrib/ground-truth-<slug>` branch; the request's idempotency key; and
   `expected_base_commit` equal to the fetched snapshot commit. The gateway rejects a
   new branch if `master` advanced, closing the fetch-to-write race without mutation.
5. Human review and the protected `core` and `assets` checks remain required. If a
   concurrent request merges first, refresh the snapshot and retry on a new branch;
   never delete or replace the newly merged request.

Use a separate reviewed PR to change a request's status or add delivered evidence.
The queue records requests and conclusions, not raw private capture data.

## Request ledger

## GTR-20260717-3spr-defaults — `3SPR`

- Status: `open`
- Target: `3SPR` / `C3DSprite`
- Blocking question: What default canvas binding and sprite size/database/index values do bare `3SPR` instances use at runtime?
- Evidence requested:
  - A sanitized runtime field dump for one reachable bare `3SPR` before its first draw.
  - A frame or draw-call trace tying those values to the visible canvas element, if any.
- Suggested capture: operator-selected level containing a bare `3SPR`, using the existing original-game capture harness.
- Acceptance: Evidence pins the default canvas and size values or proves the row intentionally has no visible runtime representation.
- Deliver to: `docs/decomp/C3DSprite.md` and the relevant capture-backed QA note.
- Idempotency key: `ground-truth-20260717-3spr-defaults`

## GTR-20260717-3rok-scatter — `3ROK`

- Status: `open`
- Target: `3ROK` / `C3DRock` origin pool
- Blocking question: Which original runtime controller scatters or repositions the 99 origin-authored `3ROK` rows, and what positions or trigger conditions does it produce?
- Evidence requested:
  - A sanitized position trace for representative `3ROK` instances from load through first placement.
  - The responsible controller/caller identity or call-site evidence when observable.
- Suggested capture: original Level5b runtime from load through the first event that places the rock pool.
- Acceptance: Evidence identifies the placement trigger and enough position semantics to avoid drawing or colliding all rows at the origin.
- Deliver to: the relevant `C3DRock` decomp/behavior note and a focused capture-backed QA note.
- Idempotency key: `ground-truth-20260717-3rok-scatter`

## GTR-20260717-3dai-purpose — `3DAI`

- Status: `open`
- Target: `3DAI` / bare `C3DAI`
- Blocking question: What runtime purpose and state transitions, if any, belong to bare `3DAI` rows rather than concrete AI subclasses?
- Evidence requested:
  - A sanitized runtime trace of a reachable bare `3DAI` row covering activation, state, speed, position, and relevant messages.
  - Evidence that the row is an intentional inert marker if it never activates.
- Suggested capture: operator-selected level containing a bare `3DAI`, from load through the relevant nearby gameplay event.
- Acceptance: Evidence distinguishes an inert marker from an active runtime role and pins any observed transition without inference from subclasses.
- Deliver to: `docs/decomp/C3DAI.md` and the relevant capture-backed QA note.
- Idempotency key: `ground-truth-20260717-3dai-purpose`
