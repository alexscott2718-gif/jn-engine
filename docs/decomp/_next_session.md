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

Current branch handoff (`codex/linked-menu-vtables`, 2026-07-16): a fresh
`9a2b908` pass certified the `CGameType::InitGame` camera-record seed
(`00474a10`) and removed the native path's extra angle/mode reset. The oracle
is mutation-sensitive and raises the scoreboard to 15 linked / 16
linked-blocked. The remaining menu targets are explicitly evidence-blocked:
CGameType pause/help/update globals and target slots; CMainMenu's missing
`DAT_004f8164` contents/rollover helper/audio/handoff; CMenuElement's canvas
owner/polarity/target protocol; and C2DInGameMenu's counter producers/death
predicate/raw helpers/`RestartLevel.tsk`. Do not port those from the native
keyboard list or one HUD frame; recover the listed bodies/data first. See the
final `PROJECT_HISTORY.md` section and generated linkage audit.

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
  writes sanitized fsynced audit records. **Deployment is still pending**: the public MCP
  remains the prior six-tool read-only service until the dedicated mode-`0600` credential,
  `ENABLE_WRITE_ACTIONS=true`, snapshot refresh, authenticated seven-tool listing, and audit
  verification are complete. Do not represent `open_pr` as live before those checks.
1. `claim_task` / `release_task`: prevents duplicate work with auditable ownership and expiry.
2. `request_ground_truth`: appends a structured request to
   `docs/ground_truth_requests.md` for the capture-only rows above.

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

- Keep changes on a branch from current `master` and submit one focused PR.
- Run the proportional local gates, then require both protected CI contexts.
- Prove any new oracle/golden catches a real mutation.
- Append durable results to `docs/PROJECT_HISTORY.md`.
- Rewrite this handoff to the new frontier before stopping.
- Do not treat a merged gateway PR as a deployed MCP tool: record the live tool listing and
  audit-path verification before closing a write feature.
