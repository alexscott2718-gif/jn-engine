# Next Session — Portable Collaboration Handoff

Updated 2026-07-14. The active branch is `master`; `native-port` and `linked` are retired,
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

## Ownership tiers for the 14 open engine tasks

### Remote-ownable

1. Wire AITrigger `AIState`/`AISpeed` into the general `C3DAI` state machine.
2. Add the menu/HUD text renderer.
3. Port the `PlayerControlled` cutscene input lock and restore timing.
4. Finish the Goddard mode-vector/orbit/effect helper tail when checked-in evidence pins the
   unresolved vectors; never guess them.
5. Finish the separate Goddard energy/menu side-effect tail.
6. Implement the active `C3DShrinkRay` shrink-to-moving-pickup mechanic.

### Alex-only: requires new original-game capture

7. `3SPR`: default canvas and size.
8. `3ROK`: runtime scatter/reposition controller for the origin pool.
9. Bare `3DAI`: original runtime purpose/state.

Record requests for these instead of inventing behavior.

### Low ROI: `no_visual_unused`

10. `3HAR` / `C3DHarrier`
11. `3MIN` / `C3DMine`
12. `3MIS` / `C3DMissile`
13. `3POD`
14. `3TAN` / `C3DTank`

These have no current `.gam` placements; leave them labeled unless new reach evidence appears.

## Recommended next campaign: gateway collaboration tools

Implement gateway-owned write tools so contributors do not receive a maintainer token:

1. `open_pr`: server-side credential, creates/updates a contributor branch and opens a PR; it
   must never push directly to protected `master`.
2. `check_status`: reports required `core`/`assets` state and artifact links.
3. `claim_task` / `release_task`: prevents duplicate work with auditable ownership and expiry.
4. `request_ground_truth`: appends a structured request to
   `docs/ground_truth_requests.md` for the capture-only rows above.

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

- Keep changes on a branch from current `master` and submit one focused PR.
- Run the proportional local gates, then require both protected CI contexts.
- Prove any new oracle/golden catches a real mutation.
- Append durable results to `docs/PROJECT_HISTORY.md`.
- Rewrite this handoff to the new frontier before stopping.
