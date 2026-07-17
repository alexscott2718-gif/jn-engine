# Next Session — Portable Collaboration Handoff

Updated 2026-07-17. The active branch is `master`; `native-port` and `linked` are retired,
fully merged campaign branches. The native C engine is the product. Godot remains retired.

## What just landed

The Sol collaborator-kit campaign made the repository independently buildable and testable:

- stock Ubuntu/system-library bootstrap, Docker image, and devcontainer;
- deterministic offscreen fixed-step execution with exact state and PNG dumps;
- asset-free `fixture0` covering spawn, production AI, trigger, projectile, animation, camera,
  render, and screenshot paths;
- `make check` for the asset-free build/determinism/golden core;
- `make check-assets` for Level 1 goldens, accepted capture/native comparison, and all linkage
  certificates;
- protected GitHub CI contexts `core` and `assets`, with phone-inspectable visual artifacts;
- a manual llvmpipe-only golden-regeneration workflow that opens a PR;
- complete-tree relocation through `JN_ASSET_ROOT` and `scripts/fetch_assets.sh`; and
- contributor/agent guidance for work without any dev-machine access.

Current baseline `master` is `b36b794`, which merged the ground-truth request
ledger and collaboration handoff from engine PR #15. The remaining menu targets are explicitly
evidence-blocked: CGameType pause/help/update globals and target slots;
CMainMenu's missing `DAT_004f8164` contents/rollover helper/audio/handoff;
CMenuElement's canvas owner/polarity/target protocol; and C2DInGameMenu's
counter producers/death predicate/raw helpers/`RestartLevel.tsk`. Do not port
those from the native keyboard list or one HUD frame; recover the listed
bodies/data first. The collaboration implementation is merged; its production
deployment gate is the immediate frontier described below.

The CI mutation gate was exercised live in PR 7: reversing fixture AI movement failed both
required jobs, and the uploaded archive contained all eight actual and expected PNG pairs.

## Read first

1. `AGENTS.md` — operating rules, machine boundary, and remote validation contract.
2. `CONTRIBUTING.md` — north-star test and PR requirements.
3. `docs/ASSETS.md` — asset root and fetch-backend contract.
4. `docs/PROJECT_HISTORY.md` — durable campaign record.
5. The relevant `docs/decomp/<Class>.md` before any behavior change.

Do not work against `docs/godot_bridge_plan.md`. Do not change settled renderer invariants without
new measurement.

## Standard validation

```bash
./scripts/bootstrap.sh
make check
make check-assets
```

`make check` must remain asset-free. Golden frames are canonical only when generated inside the
Docker llvmpipe environment. Never update them from a physical GPU.

For a relocated complete tree:

```bash
JN_ASSET_ROOT=/path/to/assets make check-assets
```

## Ownership tiers for the 13 open engine tasks

Completed 2026-07-16: the native menu/HUD text renderer now uses the shipped
`fontsmall.png` atlas for front-end labels and the level-clear banner. The remaining tasks are:

### Remote-ownable

1. Wire AITrigger `AIState`/`AISpeed` into the general `C3DAI` state machine.
2. Port the `PlayerControlled` cutscene input lock and restore timing.
3. Finish the Goddard mode-vector/orbit/effect helper tail when checked-in evidence pins the
   unresolved vectors; never guess them.
4. Finish the separate Goddard energy/menu side-effect tail.
5. Implement the active `C3DShrinkRay` shrink-to-moving-pickup mechanic.

### Alex-only: requires new original-game capture

6. `3SPR`: default canvas and size.
7. `3ROK`: runtime scatter/reposition controller for the origin pool.
8. Bare `3DAI`: original runtime purpose/state.

Record requests for these instead of inventing behavior.

### Low ROI: `no_visual_unused`

9. `3HAR` / `C3DHarrier`
10. `3MIN` / `C3DMine`
11. `3MIS` / `C3DMissile`
12. `3POD`
13. `3TAN` / `C3DTank`

These have no current `.gam` placements; leave them labeled unless new reach evidence appears.

## Recommended next campaign: gateway collaboration tools

Implement the remaining gateway-owned write tools so contributors do not receive a maintainer
token:

- [x] `check_status`: gateway PR
  [#1](https://github.com/alexscott2718-gif/jn-engine-ai-gateway/pull/1) merged, deployed,
  served to ChatGPT, exercised, and audited; production closeout completed 2026-07-16.
- [x] `open_pr`: gateway PR
  [#6](https://github.com/alexscott2718-gif/jn-engine-contributor-mcp/pull/6) merged with all
  required checks green on 2026-07-17. It uses a third dedicated credential, validates
  `contrib/*` branches and paths, records idempotency keys, sends mutations exactly once, and
  writes sanitized fsynced audit records. Production closeout completed later on 2026-07-17:
  source `8bef00e` is deployed with a distinct mode-`0600` credential and writes enabled only for
  the authenticated engine profile; both snapshots were refreshed; ChatGPT listed all seven
  tools; and `check_status(branch=master)` returned both protected contexts successful while
  appending a durable sanitized `github:alexscott2718-gif` audit record. `open_pr` is live. The
  separate Actions-read credential was renewed rather than reusing the write token.
- [x] `claim_task` / `release_task`: gateway PR
  [#7](https://github.com/alexscott2718-gif/jn-engine-contributor-mcp/pull/7) merged as
  `c06521f` after all required checks and review. It binds exact committed tasks to the
  authenticated owner, enforces 15-minute through
  24-hour expiry, idempotent replay and claim-ID-guarded release, and uses a dedicated locked,
  fsynced, schema-versioned claim ledger separate from the rotatable status/PR audit log. The
  deployment runbook defines a 48 MiB maintenance threshold, 64 MiB hard bound, and safe 24-hour
  drain-and-archive compaction/recovery. All 359 local CI-equivalent tests and the public-tree scan
  passed; the updated remote `test`, CodeQL Python, and CodeQL Actions checks were green at merge.
  This is **not deployed**: production remains the seven-tool `8bef00e` service.
- [x] `request_ground_truth`: implemented as a safe composition, not a tenth tool.
  Engine PR [#15](https://github.com/alexscott2718-gif/jn-engine/pull/15), implementation commit
  `c3d14f1`, adds the fixed append-only `docs/ground_truth_requests.md` schema, a `make check`
  validator, and seeded requests for `3SPR`, `3ROK`, and bare `3DAI`. Gateway PR
  [#8](https://github.com/alexscott2718-gif/jn-engine-contributor-mcp/pull/8), implementation commit
  `a6b6279`, adds optional `expected_base_commit` enforcement to `open_pr`, rejecting a stale new
  branch before mutation while preserving idempotent replay. Gateway #8 merged as `c4d8ff4` and
  engine #15 merged as `b36b794`, with required checks green. The gateway remains exactly nine
  tools after both changes. The merged work is **not deployed**.

Gateway `check_status` architecture remains documented in `docs/collaboration_tools.md`; the
implementation and deployment closeouts are recorded in `docs/PROJECT_HISTORY.md`.

Treat authentication, branch allowlists, path validation, audit logging, request idempotency, and
PR-only enforcement as part of the feature—not deployment details. This is gateway work, not an
engine-runtime change.

## Closeout state and one preserved branch

- GitHub `master` protection requires strict `core` and `assets` checks and includes admins.
- `linked` and `native-port` were fully merged before cleanup.
- `modify-source` was **not merged** at audit time. It contains the unique camera-patch scaffold
  commits `ce67fe9` and `a450cc3`; do not delete it until a maintainer chooses merge or archival.
- There were no stashes, extra worktrees, or stray empty `z` file.

## Definition of done for the next session

- Keep the deployed `open_pr` credential separate from collaborator and Actions credentials; the
  request composition must use that existing path and must not broaden token scope.
- Refresh the deployed gateway source and the separate JN Gateway Repository review snapshot to
  exact merge `c4d8ff4`. Refresh the engine snapshot to exact merge `b36b794`, then recreate only
  the authenticated engine profile. Do not turn the read-only source profile into a write service.
- Confirm the dedicated `task_claims.ndjson` initializes on the audit mount with mode `0600`
  alongside, but separate from, the general audit log. This deploy-time check is required evidence
  for the `fcae7af` ledger split.
- Verify an authenticated exact nine-tool listing plus claim, replay, competing-owner conflict,
  expiry, guarded release, expected-base stale rejection, and corresponding mode-`0600` durable
  audit records. Do not create a duplicate ground-truth request for the three seeded rows.
- In a fresh independent connector session, repeat a live claim/replay/release cycle and an
  `expected_base_commit` rejection so the closeout has a second set of eyes rather than only the
  deploying agent's evidence.
- Record the final merge commits, live snapshot commits, and audit-path evidence in
  `docs/PROJECT_HISTORY.md`, then rewrite this handoff to the next engine frontier.
- Do not treat merged source as deployed behavior: production remains the seven-tool
  `8bef00e` service until the live listing and audit verification prove otherwise.
