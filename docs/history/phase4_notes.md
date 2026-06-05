# Phase 4 — Gameplay Loop

Phase 4 turned the free-fly viewer into a playable Level1 demo. The
WASM/parity gating from the original plan is deferred to a later phase;
the work here is native-only.

## What ships

- **Behavior vtables** — every entity gets a `const EntityVTable *vt`
  resolved once at spawn from its FourCC. Lives in
  `src/game/behaviors/*.c`. Replaces the FourCC `if/else` chain that
  used to be in `entity_update`.
- **Physics module** — `src/engine/physics.c` integrates gravity +
  per-axis AABB resolution against entities that opted into
  `ENTITY_FLAG_SOLID`. Triggers fire when the player overlaps an
  entity flagged `ENTITY_FLAG_TRIGGER`.
- **Follow camera** — `src/game/camera.c`. Orbit yaw/pitch driven by
  LMB-drag; position smoothed toward a behind/above offset from the
  player. Snaps to its target on init via `follow_cam_snap`.
- **Game state** — `src/game/gamestate.c` tracks
  `items_total/items_collected/level_done/level_change_requested`,
  exposed via the window title HUD.
- **Behaviors**: player (3JIM), static decor (3TRE/3ROC/CRAT/STRT),
  LOAD trigger → level-change request, TRIG generic trigger, DOOR
  (raises on trigger), PLAT (oscillates), ITEM (bobs/spins/pickup).

## Architecture

```
                  ┌──────────────────────┐
                  │ entity_bind_vtables  │  (once, after gam_load)
                  └─────────┬────────────┘
                            │ on_spawn, apply_default_extents
                            ▼
   per-tick loop in main.c:
     input_update()
     for e in world: e.vt->on_update(e, dt)     # behavior reads input,
                                                 # platforms oscillate, etc.
     physics_step(world, dt)                    # gravity, AABB, triggers
     follow_cam_update(...)
```

Engine-side files (`src/engine/`) know nothing about specific
FourCCs — they read `vt->flags` (`PHYSICS`/`SOLID`/`TRIGGER`/`PLAYER`).
All FourCC-keyed logic is in `src/game/`.

## Tunables (Level1 unit scale)

| | |
|---|---|
| Gravity | 1600 u/s² (`physics.h`) |
| Player half-extents | 35×60×35 (`behavior_player.c`) |
| Player walk / run | 600 / 1200 u/s |
| Player jump impulse | 650 u/s |
| Tree AABB | 50×200×50 |
| Crate AABB | 40×40×40 |
| Door open rise | 180 u over ~0.75 s |
| Platform oscillation | ±300 u at 0.4 Hz |
| Item AABB | 30×30×30 |

## Level1 reality check

Counting Level1 entities by FourCC:

```
3TRE 87   LOAD 25   STRT 19   DOOR 3   3ROC 2   3JIM 1
```

Level1 ships with no `ITEM`, `PLAT`, `TRIG`, or `CRAT` entities, so
the demo synthesizes 5 ITEMs in a 600-unit ring around the player
spawn (`main.c`). DOOR/LOAD/3TRE behaviors are exercised against
real Level1 data.

## Controls

| Key | Action |
|---|---|
| WASD | walk (camera-relative) |
| SHIFT + WASD | run |
| SPACE | jump (only when grounded) |
| LMB-drag | orbit follow camera |
| ESC | quit |

Window title shows: FPS, items collected/total, level-cleared flag,
player position, and `G` when grounded.

## Deferred to Phase 5 (was originally in Phase 4)

The plan called for per-step WASM parity via a Puppeteer +
screenshot-diff harness, plus a scripted-input replay format. Both
were deferred per user direction — gameplay first, parity later.
When that work resumes:

- Add `tools/qa/compare.py` (pixel-MAE) + `tools/qa/wasm_capture.js`.
- Plumb `JN_SCRIPT=tools/qa/scripts/foo.txt` into `main.c` to
  synthesize input events.
- Commit goldens under `tools/qa/golden/`.
- Lockstep gate the WASM build (`make web`).

## Known gaps to address later

- No bitmap-font HUD — gameplay state lives in the window title.
- No spatial broadphase — physics is `O(n×s)` where `n` = physics
  entities (currently just the player) and `s` = solid count. Fine at
  327 entities.
- Trigger firing has no debounce beyond per-entity `user_flag`; some
  triggers (TRIG) should re-arm.
- Doors/platforms move position directly; they don't push the player
  when blocking, just stop being walkable.
- Player faces direction of movement but has no turning interpolation.
