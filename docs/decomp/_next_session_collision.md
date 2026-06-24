# Next Session — Ground & World Collision Overhaul (native-port)

> Self-contained kickoff for a dedicated collision initiative (parallel to the main
> `_next_session.md` behavior-coverage track). Paste this, or point a fresh agent at this path.
> Scoped 2026-06-24. Mission: replace the engine's patchwork ground/collision with a
> decomp-faithful **mesh collision world** built from each level's `BLOCKING_*` collider meshes.

You are working in `/home/scotty/jn-engine` on branch `native-port`. This is a faithful
reimplementation: the decomp specs + authored `.gam`/OMT data are ground truth.

## STATUS — Phases 0–3 DONE, Phase 4 BLOCKED (updated 2026-06-24)
- **Phase 0 ✅** `src/engine/collision.{c,h}` — `CollisionWorld` (XZ-grid triangle world from
  GROUND + BLOCKING_*/BLOCK_* colliders, minus the visible `Blocks_In/Out` toys). Unified
  `collision_is_collider` / `collision_is_invisible` predicates. Commit `a7256be`.
- **Phase 1 ✅** mesh ground-follow via `collision_ground_height`; old `world_terrain_height` deleted.
  Safety floor demoted to an env-gated backstop (`JN_SAFETY_FLOOR`, default **ON**). Commit `c55186d`.
- **Phase 2 ✅** walls + slide + step-up via `collision_resolve_horizontal`; `JN_TEST_COLLIDE` headless
  seam (17/17 levels pass). Commit `2041177`.
- **Phase 3 ✅** per-entity `entity_terrain_collides` gate (`TerrainColl==0`/`HasCollision==0` pass
  through; player always collides). Forward-looking — only the player has `ENTITY_FLAG_PHYSICS`. Commit `f81517d`.
- **Phase 4 ⛔ BLOCKED — do NOT execute without addressing this first.** **Only `level1`, `level2`,
  `level3a` ship a `GROUND` floor mesh.** Every other level's walkable surface is the procedural
  `ground.c` plane on the safety floor — there is **no GROUND/BLOCK collider under the player** there.
  So (a) the safety floor can't be deleted (it's the de-facto floor for ~22 levels), and (b) retiring
  the procedural ground would drop those levels into a sky-void. The real prerequisite is giving the
  GROUND-less levels a walkable collider mesh (the "include visible floor meshes" idea, but per-level
  and evidence-driven). Until then Phase 4 only meaningfully applies to the 3 GROUND levels, where the
  plane is already occluded by `GROUND.glb` (near-invisible to retire). Phase 4 also moves audited
  pixels and its public deploy is gated on explicit user approval.

Everything below is the **original kickoff** (kept for context). Validation gates that DID run for
Phases 0–3: `make` clean, `JN_TEST_COLLIDE` 17/17, `audit_faithfulness.py` 0 findings, `make web` +
`qa_web_verify.py` 16/16.

## Orient yourself first (read in this order)
1. `~/AGENTS.md` (== `~/CLAUDE.md`) — machine/workflow conventions, anti-silo policy.
2. `docs/native_port_plan.md` §1 implementation contract + §3 validation; `docs/PROJECT_HISTORY.md`
   (esp. §Invariants and the BLOCKING-volume notes ~lines 555-624, 890-977) + `docs/ARCHITECTURE.md`.
3. Decomp specs (the collision model): `docs/decomp/C3DPlayer.md` (ground probes + jump/fall phases),
   `docs/decomp/C3DObject.md` (`InitPhysics3D`), `docs/decomp/C3DAnimated.md` (`HasCollision`
   enable/disable pair), `docs/decomp/C3DAIOmtObj.md` + `C3DOmtObj.md` (`TerrainColl` semantics),
   `docs/decomp/C3DTerrain.md` (3TER — NOT placed in any level).
4. Current code touchpoints:
   - `src/engine/physics.c` — `physics_step`, `world_terrain_height`, `sample_mesh_height_xz`,
     `placement_is_collider`, `world_safety_floor_height`, `resolve_axis`.
   - `src/engine/ground.c` — the procedural visible-ground plane (render-only).
   - `src/engine/world.h/.c` — `Entity` (flags, `half_extents`, `on_ground`), `WorldPlacement`,
     `World` (`placements`, `ground_y`, `safety_floor_*`), `world_query_segment`.
   - `src/game/main.c` — `load_level` (~394), the swap path (~1946), `configure_safety_floor` (~429),
     `ground_init`/`ground_draw` wiring, and the placement render-skip / `BLOCK*` classification
     (~843-911).
   - `src/engine/assets/placement_loader.c` + `assets/glb/omt/<level>_placements.txt`.

## Current state (the gap)
- **Floor-only world collision.** `physics.c:world_terrain_height()` samples the top-face height of
  placements named `GROUND*`/`BLOCK*` under the feet (STEP=100 cap). **No walls** — you walk through
  buildings/fences unless a SOLID *entity* covers them (most world geometry is static
  `WorldPlacement`, not entities).
- **Fake visible ground.** `ground.c` draws a procedural flat plane (amplitude 0 in the live path,
  `mud.png`) under the real OMT terrain — a Phase-12 capture-diff leftover.
- **Fall-through crutch.** `main.c:configure_safety_floor()` adds a plane 1000 below `ground_y`.

## Decomp target model
- The **`BLOCKING_*`/`BLOCK_*` invisible OMT meshes are the collision world** (never drawn; ~54 of 196
  placements in level1; present in 20/25 placement files). The visible walkable floor is a real OMT
  mesh — level1's is literally `GROUND.glb` (drawn AND a collider). Buildings are blocked by their
  `BLOCKING_*` twins (e.g. `BLOCKING_PHONE`), so we collide against BLOCKING meshes, not the visible
  building meshes.
- The player ray-probes against that world: core query `FUN_0047c210` (NOT decompiled — match
  behavior, not bytes), `GroundAheadPredicate` (slot 83, forward+down ray), `ProbePlayerRayBlend`
  (slot 91), authored offsets (forward 200, down -350, -40/-100), and a **jump/fall phase machine**
  (`UpdateJumpFallMove`, `jump_or_motion_phase` 0..5).
- Per-object gates: **`HasCollision`** (C3DAnimated `0x584`, paired enable/disable helpers) and
  **`TerrainColl`** (`0` disables terrain/collision hooks, `1` explicit enable). Entities already clear
  `ENTITY_FLAG_SOLID` on `HasCollision==0`; the terrain side must honor `TerrainColl`.

## Confirmed decisions (do not relitigate)
- **Full, phased** overhaul (Phases 0-4 below), each phase shippable + gated.
- **Also retire the procedural visible ground** (Phase 4) — the only phase that moves audited pixels;
  gate it on a per-level terrain-coverage check + audit re-baseline.
- Ladder/fence special surfaces (`HandlePlayerCollisionSurface`, material ids `0xb4`/`0x3c..0x45`) are
  OUT OF SCOPE — a later feature on this base.

## The work (phased)
**Phase 0 — `CollisionWorld`** (new `src/engine/collision.{c,h}`): build once per level from
`world->placements`; load each collider via `model_cache_get(pl->ase_path)` and bake its triangles into
world space in the collision basis `(x, 0, -z)` (same basis as `physics.c:114`). **Unify the collider
predicate** (currently split: `physics.c:placement_is_collider` vs `main.c` BLOCK-skip) into one shared
`collision_is_collider(name)` — include `BLOCKING_*`/`BLOCK_*` + visible floor meshes (`GROUND`, roads,
dirt, steps), exclude the visible toys `Blocks_Out`/`Blocks_In`. Build a uniform XZ grid
(cell→triangle list) — required for per-frame cost natively + WASM. API:
`collision_ground_height(cw,x,z,ref_y,*out_y,n[3])` (generalize `sample_mesh_height_xz` + return
normal), `collision_resolve_horizontal(cw,pos,half,vel)` (push out of walls + slide + step-up under
STEP), `collision_segment(cw,p,q,n)`. Store `CollisionWorld *collision` on `World`; build in
`load_level`/swap after `placements_load`, free in `world_destroy`.

**Phase 1 — Mesh ground-follow; delete safety floor** (`physics.c`): replace the `world_terrain_height`
+ `world_safety_floor_height` block (`physics.c:180-198`) with `collision_ground_height`; keep
gravity/`on_ground`. Remove `configure_safety_floor` + the `safety_floor_*` fields once no level falls
through (keep behind `JN_SAFETY_FLOOR=1` for one phase, then delete).

**Phase 2 — Walls + slide + step-up** (`physics.c`): after horizontal integration (X/Z passes at
`physics.c:158-170`), call `collision_resolve_horizontal` so BLOCKING walls/fences block, with
wall-slide and curb step-up (block above STEP; slope-clamp steep faces to walls). Keep the
entity-vs-entity AABB pass (`resolve_axis`) for dynamic props/doors/platforms.

**Phase 3 — Generalize + flags + tuning**: confirm all `ENTITY_FLAG_PHYSICS` entities
(NPCs/enemies/vehicles) behave; entities with `TerrainColl==0`/`HasCollision==0` skip terrain collision
(mirror `behavior_prop.c`/`behavior_omtobj.c`). Tune STEP/slope from decomp/`.gam`; document any value
not pinned by data.

**Phase 4 — Retire procedural ground** (`ground.c`, `main.c`): compute each level's terrain-coverage XZ
AABB; stop drawing the `ground.c` plane where OMT terrain covers the play area; keep a documented flat
backdrop only for uncovered areas (no sky-void). Re-baseline `audit_faithfulness.py` — removing the fake
plane where real terrain exists is more faithful, but verify 0 NEW findings.

## Validation (every phase)
1. `make`
2. New `JN_TEST_COLLIDE` headless seam (add beside the other `JN_TEST_*` hooks in `main.c`): drop the
   player at spawn, run N physics ticks, assert it lands on the **mesh** floor (not a safety plane) and
   that stepping toward a known `BLOCKING` wall clamps position. Exercise level1 (Retroville floor + a
   building wall), level1c (fences/perimeter), and a vertical level.
3. `JN_SCREENSHOT` on level1 / level1c / level3. Run headless:
   `LD_LIBRARY_PATH=$HOME/toolchain/usr/lib/x86_64-linux-gnu:$HOME/sdl2/lib JN_SCREENSHOT=1
   JN_SCREENSHOT_PATH=/tmp/x.png xvfb-run -a ./jnengine --level <lvl>`.
4. `python3 tools/audit_faithfulness.py` → **0 findings (all 35)** every phase (Phase 4 needs the
   explicit re-baseline check).
5. `make web` (`source ~/emsdk/emsdk_env.sh` first) → `python3 tools/qa_web_verify.py` → **16/16**
   (confirm grid-query cost is fine in WASM).
- Optional fidelity: motion-diff vs the marked `.omtc`; compare ground-follow/wall behavior against the
  XP original over VNC (project methodology).

## Hard rules
- Faithful reimplementation: confirm behavior against `docs/decomp/<Class>.md` bodies, not offset scans;
  authored `.gam`/OMT data + the decomp are the only source of truth. `FUN_0047c210` isn't decompiled →
  match behavior, document the divergence.
- Keep direct `--level`/audit/screenshot launches rendering identical until Phase 4 (collision is
  invisible). There is a large PRE-EXISTING dirty asset tree — do NOT stage or touch it; commit only
  `src/**`, `tools/**` if touched, and wave-end docs.
- Commit per phase (`feat(collision): …`); at wave end update `docs/PROJECT_HISTORY.md` and this file.
  **Public WASM deploy is gated on explicit user approval.**

## Definition of done
The `BLOCKING_*` mesh set is the collision world behind an XZ-grid `CollisionWorld`; the player and all
physics entities follow the mesh floor (no fall-through, safety floor deleted), are blocked by and slide
along BLOCKING walls, and step up curbs; `HasCollision`/`TerrainColl` gate participation; the procedural
ground is retired where real terrain covers the area. `audit_faithfulness.py` 0 findings (all 35),
`make web` + `qa_web_verify.py` 16/16, with `PROJECT_HISTORY.md` + this file updated.
