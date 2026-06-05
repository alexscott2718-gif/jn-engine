# Retroville Feel Pass — controls, sky, ground, foliage

*Drafted 2026-06-01. Four issues reported from playing the web demo, with root
causes traced in the source. Decisions locked with user 2026-06-01. Plan-doc-
first, phase-by-phase, visual-QA gate each phase.*

## Decisions (locked 2026-06-01)
- **Controls:** original-style **tank turn**. Left/Right (arrows **and** A/D)
  rotate Jimmy's heading; Up/W = forward, Down/S = backpedal. Camera **locked
  directly behind his back**, turning with him — with **optional mouse
  free-look** that orbits temporarily and snaps back behind on movement.
  `jimleft`/`jimright` are retasked as **turn-lean** clips.
- **Skybox:** **faithful** — extract `~/xp-jnbg-original/omt/bluesky3.omt`
  (Level-1's real blue cloud sky) via `omt_asset_toolkit`, render as a slowly
  **rotating** sky dome/cube. Remove the green tint entirely.

## Root causes (traced in source)
1. **Strafe instead of turn** — `game/behaviors/behavior_player.c` is camera-
   relative twin-stick (A/D lateral, W/S fwd/back); `game/camera.c` has
   `manual_yaw=1` (mouse drives yaw) and Jimmy always faces camera-forward.
2. **Fall-through near house/monkey-bars** — `engine/physics.c
   world_terrain_height()` only collides placements **named exactly `"GROUND"`**.
   Any other floor mesh has no collision; with the synthetic ground tile removed
   (commit 8bde7f3) you drop and hit the kill-plane (`main.c:1044`,
   `y < ground_y-2000`) → respawn.
3. **Green sky** — `engine/phase1_sky_tint.h` (top 0.33,0.51,0.37 / bot
   0.32,0.42,0.30 + gray 0.43 scene tint), applied `main.c:527`. Values were
   sampled from an already-wrong capture frame — a feedback loop, not the game.
4. **Foliage billboards** — `engine/renderer.c` billboard path +
   `engine/assets/billboard_overrides.c`: wrong anchor-Y and wrong texture id
   (Goddard food-bowl icon) on foliage; some "trunks" are synthetic green quads.

## Status (2026-06-01)
- **Phase A — DEFERRED** by user ("forget the ground coverage for now and move
  on"). Diagnosis stands: only the single mesh named `"GROUND"` is collidable;
  its bbox covers the monkey-bars XZ so the gap is a tessellation hole, not an
  extent miss. Resume here later.
- **Phase B — DONE.** Tank-turn + locked-behind camera w/ decaying free-look.
- **Phase C — DONE.** Green tint removed; faithful bluesky3 neighborhood
  backdrop + full procedural cloud hemisphere (real sky.png) that rotates.
- **Phase D — DONE.** Billboard override layer retired; trees render from the
  real glTF meshes (correct textures, trunk, baked Y).
- Built native + web, deployed to exentt.com/jn-engine + gateway:8500. Awaiting
  user visual QA (turn-direction sign + feel are the interactive gate).

## Phases (each ends at a visual-QA gate)

### Phase A — Ground collision fix (game-breaking; smallest) ← FIRST
- Broaden `world_terrain_height()` beyond name=="GROUND" to the real walkable
  floor meshes near the house/monkey-bars. Identify which placement name(s)
  cover that spot; collide against the floor mesh set (or all non-billboard,
  non-wall static meshes via a collidable flag) rather than a single name.
- Keep the kill-plane as a backstop only.
- **Gate:** walk to the house/monkey-bars without falling through.

### Phase B — Tank-turn controls + locked-behind camera
- `behavior_player.c`: Left/Right + A/D → turn `e->ry` at a turn-rate; W/Up →
  +forward along `ry`, S/Down → backpedal; remove lateral strafe. Mobile stick:
  y=move, x=turn. Anim: fwd→`jimrun`, back→`jimbackpedal`, turning-in-place→
  `jimleft`/`jimright`, still→`jimstop`.
- `camera.c`: derive `fc->yaw` from the player's heading (sit behind the back),
  auto-recenter; keep mouse orbit as a temporary offset that decays/snaps back
  on movement (free-look option).
- **Gate:** Left/Right turns Jimmy + camera together; forward goes where he
  faces; mouse look snaps back.

### Phase C — Faithful rotating blue cloud skybox
- Extract `bluesky3.omt` → sky mesh + cloud texture (toolkit `omt-extract`).
- New sky-dome render path (replaces/augments the fullscreen gradient): draw the
  textured dome centered on the camera, depth-write off, behind everything, with
  a slow yaw rotation over time. Remove the green gradient + set scene tint to
  neutral (1,1,1). Retire/park `phase1_sky_tint.h` application.
- **Gate:** blue sky with clouds that drift/rotate; no green cast on the scene.

### Phase D — Foliage billboard Y + texture fix
- Fix billboard anchor-Y so foliage sits at the correct height on its trunk.
- Fix the texture-id mapping so foliage uses the leaf texture, not the food-bowl
  icon. Replace synthetic green trunk quads with the real trunk geometry/texture
  (or hide them if they're debug artifacts).
- **Gate:** trees read as trees — trunk + correctly-placed leaves, no food bowls.

## Notes / guardrails
- Don't touch the OMT→glTF pipeline for Jimmy (he's hand-authored ASE).
- Sky dome must follow the camera (translation), only the cloud layer rotates.
- Web demo is the QA surface; rebuild `make web` + deploy after each phase.

## Foliage billboard sizing (2026-06-01, round 6)
- Canopy sizes are capture-measured (level1_billboard_overrides.json, frame 8881
  drawcalls). Only ~12 of ~32 tree/treebranch placements were in that frame; the
  rest defaulted. ALL tree* trunks are identical (358x44) so geometry can't infer
  canopy size — it lives only in the capture.
- Interim: untabled fallback = measured median (tree* 600, treebranch* 500),
  was a flat 450.
- EXACT fix: NOT via brute-scanning the 4GB stream. Decision 2026-06-01 — defer
  to a future **camera capture rig** (no-clip + pose-queue in Neutron.exe, one
  frame per sector) that a contributor will build; far fewer frames / smaller
  files, generalizes to other D3D7 games. See memory `jn-capture-rig-decision`.
  Demo stays on calibrated-median fallbacks until then.
- Cheaper alt to check first: per-tree canopy scale may live in the .omt/.gam
  placement transform (we only kept the center) — would avoid capture entirely.
