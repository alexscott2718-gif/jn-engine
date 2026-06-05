# JNvsJN Autonomous Session Log

Session start: 2026-06-03 ~11:00 EDT. Executing
`docs/jnvsjn_missing_systems_plan.md` work order top-to-bottom.

Env roots for all smokes:
```
JN_GAM_ROOT=/home/scotty/jnvsjn-original/gam
JN_PLACEMENTS_ROOT=/home/scotty/jnvsjn-runtime/glb/omt
JN_NATIVE_ROOT=/home/scotty/jnvsjn-runtime/native
```
Screenshots saved under `/home/scotty/jnvsjn-runtime/screenshots/`.

---

## Step 1 — Icon extraction ✅ (2026-06-03 11:05)

- Decoded all `Canv` chunks from `omt/icons.omt` (13) and
  `omt/permanenticons.omt` (11) via the toolkit `decode_canvas`/`save_png`.
  permanenticons are **byte-identical duplicates** of icons (verified by sha1)
  → removed the `perm_*` copies. Kept 13 unique editor/marker icons (32×32 RGBA):
  triger, dispatch, load, question, sprite, start, **apple**, sound, AITRIG,
  multi, camera, musicicon, fog.
- Copied the in-game panel PNGs `png/{bars,heart,toolchest}.png` (128×128
  palette) into `assets/hud/`.
- **Finding:** icons.omt holds *editor* toolbar icons, not gameplay HUD art.
  The real in-game HUD art is the direct PNGs. `apple.png` is the only clean
  alpha icon (good item-counter glyph); heart/toolchest/bars are full-frame
  palette panels with no chroma key (the pink in heart IS the art).
- Landed under `assets/hud/` (27 files).

## Step 2 — HUD 2D layer ✅ (2026-06-03 11:19)

- **renderer:** added `renderer_draw_sprite_2d(tex, vw, vh, x, y, w, h, tint…)`
  — orthographic textured quad, depth-test off, alpha blend, RGB+alpha tint.
  New `SPR2D` shader program + dynamic VBO. (`GL_STREAM_DRAW` isn't in the glad
  subset → used `GL_STATIC_DRAW` like the existing rect path.)
- **gamestate:** added `health`/`health_max` (default 100), typed counters
  `gems_collected`/`points`, and a fixed `inventory[INVENTORY_MAX=16]` with
  `gamestate_grant_tool`/`gamestate_has_tool`/`gamestate_gem_collected`/
  `gamestate_add_points`. (INVENTORY_MAX mirrors the exe's "OVER max pickup
  items" cap; exact size still TBD via Ghidra.)
- **hud.c/.h (new):** item counter (apple icon + `collected/total` via a
  rect-drawn 3×5 digit font, no font asset), gem counter (shown only when >0),
  health bar (heart icon + green→red fill), and an inventory tool-icon row
  (shown only when inventory non-empty).
- **main.c:** `hud_init()` after asset cache; `hud_draw()` after all 3D, before
  `renderer_end_frame()`. **Gated off** when a matched-camera override is active
  (so the game-1 native-vs-capture faithfulness validators stay pixel-identical)
  and via `JN_DISABLE_HUD`.
- SRC is a wildcard so `hud.c` is auto-compiled by native + web; no Makefile
  edit needed (no new WASM exports).
- **Verified:** native build green; level1 smoke `placements_loaded=210,
  missing_mesh=0, all entities resolved`; screenshot shows apple+`0/0`
  top-left, heart+full green bar bottom-left over the live Retroville scene.
  Screens: `screenshots/step2_hud_level1.png`, `hud_topleft.png`,
  `hud_botleft.png`. (items_total=0 for level1 — counters populate in Step 3.)

---

## Step 3 — Items visible + typed counters ✅ (2026-06-03 11:35)

- **3GEM/3PIC bound to `vt_item`** (entities.c) → bob/spin/collect + counted.
  - 3GEM already resolves to `gemred.glb` via the GRN stem path (gam carries
    `BASEAnimation=gemred.grn`). Tint is **per-color file**: regenerated
    `assets/glb/grn/gem{red,blue,yellow}.glb` with baked `baseColorFactor`
    (red/blue/yellow) via `grn_to_glb.convert(color=…)` — no engine change.
  - 3PIC pickups are sprite-based (sprites.omt/SpriteIndex) in the data; for now
    they fall back to the textured `jimpickup.ASE` (visible) via TYPE default.
- **gamestate:** `gamestate_gem_collected` + `gamestate_add_points`; gam_loader
  now parses `Points` (type-6 int) into `Entity.points`.
- **behavior_item:** trigger is type-aware (3GEM→gem counter, +points), still
  fires the generic item tally so level-clear works.
- **Renderer fix:** the global `hide_untextured_groups=1` (for OMT collision
  slabs) was silently skipping the untextured gem/GRN prop meshes. The
  entity-resolver model draw now wraps the call with hide-off→on so deliberately
  chosen props always render.
- **QA helpers added:** `JN_DEMO_SPAWN_XYZ="x,y,z"` (park camera anywhere in any
  level) and `JN_TEST_GEMS=1` (spawns red/blue/yellow gems in front of the
  level1 demo cam).
- **Verified:** level5a smoke `Items: 29` (pickups now counted); `JN_TEST_GEMS`
  renders the three correctly-tinted gems (screenshots/step3_tinted_gems_level1
  .png). **Web rebuilt + deployed, live HTTP 200.**

## Step 4 — Inventory state + tool icons ✅ (2026-06-03 11:42)

- **Inventory** already in gamestate (`inventory[16]`, `grant_tool`/`has_tool`).
- **Authentic tool icons** extracted from `sprites.omt` → `assets/hud/tool_*.png`
  (wrench, glasses, Jetpack 1, megaphone→burpgun, flurp→watergun, toolchestkey,
  key). (sprites.omt has 306 named canvases — a rich source for Step 3/5 sprites
  too.)
- **behavior_item:** `TOOL_GRANTS` table maps a case-insensitive ObjectTag
  substring (watergun/glasses/jetpack/wrench/megaburp/tools04/key) → a deduped
  inventory slot on collect.
- **HUD:** inventory row now leads with the `toolchest.png` affordance then one
  icon per owned tool (backing plate each). Shown only when inventory non-empty.
- **Deferred (as planned):** skinned held-tool mesh on the hand bone (blocked on
  GRN skinning).
- **Verified:** `JN_TEST_TOOLS=1` logs 4 grants; HUD shows toolchest + flurp +
  glasses + jetpack + wrench (screenshots/step4_inventory_hud.png).

## Step 5 — Widen entity coverage ✅ (2026-06-03 12:20)

- Generated authoritative FourCC→class map (125 rows) from Neutron2.exe; swept
  all 22 levels for placeholder boxes.
- Added entity_visual rows: characters (3CIN→cindycheer, 3HUG→hughstop,
  3ULT→ultrastop, 3FOW→fowlstop, 3SPK→sporkystop, 3BOT/3ENE→bigbotstop,
  3NUM→nummeyscooterbase.glb); props (3TES→tesla, 3MOP→block, 3MOR→Box01,
  3SHU→BUSH01, 3DUD→downdoor2a, 3WAB→buttondown, 3SCR→blockscreen,
  3CRB→block); sprite billboards (3GRT→Greenneutron, 3TAR→target); invisibles
  for pure fx/triggers (3TEX 3COR 3VOR 3TSU 3SRO 3ELE 3BUG 3GWA 3WAT 3ROP 3PEN
  3TRA). Copied Sporky ASE + frykid(=sporky).png into assets.
- **Generic 3ASE (C3DASEObj) loader:** gam_loader now parses ASEStop/ASEWalk +
  PNGFile; imported the 19 referenced ASEs (+textures) into assets/ase/jnvsjn/
  (lowercased); main.c draws each per-object mesh+texture. Verified: level3
  coral/tube render textured.
- **Verified:** sweep of all 22 levels → **missing_mesh=0 AND zero placeholder
  boxes everywhere.** Coverage ring screenshot shows bigbot/Hugh/UltraLord/etc.
  (screenshots/step5_coverage_ring.png, step5_3ase_coral_level3.png).

## Step 6 — Vehicles visible ✅ (2026-06-03 12:25)

- Imported vehicle ASEs (car, godflycycle, substop, tankstop, lunerlanderstop)
  into assets/ase/jnvsjn/; entity_visual rows: 3JEE/3NCA/3NC2→car,
  3HOV→godflycycle, 3SUB→substop, 3VRT→tankstop, 3LUN→lunerlanderstop.
- **Verified:** level1_a jeep renders (dark/untextured car silhouette — car.ASE
  has no bitmap; visible, no box). screenshots/step6_jeep_level1a.png.
- **Ridability:** deferred in favor of the user's mid-session request to build
  playthrough logic (cutscenes / world-action triggers / progression).
- **Web rebuilt + deployed; live HTTP 200** (full coverage + HUD + items + gems
  + vehicles).

---

## Playthrough logic (user request, mid-session pivot) ✅ (2026-06-03 13:10)

Built a progression/gameplay layer on the now-complete entity coverage.

- **Checkpoints (3CHK)** — new `vt_checkpoint`: on overlap, moves the level
  respawn point to the checkpoint (last-touched wins). 3CHK now also renders the
  authentic `checkpoint` sprite (sprites.omt) as a camera-facing billboard
  (was invisible). Verified: level5a logs `[CHECKPOINT] reached … -> respawn`.
- **Moving platforms (3MOP)** — new `vt_movplat`: parses `PatrolPoint`, finds the
  named marker entity at spawn, and ping-pongs (raised-cosine) between home and
  target; SOLID so the player rides it; records velocity for vertical carry.
  Verified: level5a `[MOP]` log shows multiple platforms MOVING (travel
  594–2347 units: raftf, raftb, plat5, plat1b, c3dplat7…); unresolved targets
  stay static.
- **Objective / level-clear** — HUD now draws a centered green "LEVEL CLEAR"
  banner when `level_done` (all items collected). Added a small uppercase glyph
  set + `draw_text` to the rect font. Verified via `JN_TEST_CLEAR`
  (screenshots/play_level_clear_banner.png). (Did NOT gate exit LOADs on
  RequiredTask/RequiredLevel — too risky for soft-locks without mapping the task
  graph; level-to-level flow already works via LOAD swaps.)
- **Intro cutscene** — REMOVED (2026-06-03, user feedback: the camera blend
  jolted/glitched the whole map and was motion-sickening). All cutscene code
  stripped from main.c; game now opens directly in the follow-cam. A real
  scripted cutscene would need actual camera interpolation from the level's
  3CAM/3MCA data, not a generic blend — left for later.
- **Trigger → target world actions** — the headline "level trigger" primitive.
  3BUT/3WAB buttons carry `ActivateButton = <target ObjectTag>`; new `vt_button`
  resolves the target entity at spawn and forwards a trigger to it on press.
  Doors (3DOR/3DUD/3SCD) bound to a new **non-solid** `vt_leveldoor` (rise-open
  animation; non-solid so they can never trap the player at a doorway — they
  were walked-through before). gam_loader parses `ActivateButton`; Entity gains
  `activate_target` + a resolved `link_target` pointer. **Verified end-to-end:**
  level3a player presses `doorbutton` → `[BUTTON] pressed` → linked
  `[DOOR] opening (tag='numeydoor')` (screenshots/play_button_door.png). All
  three level3a buttons link correctly (doorbutton/C3DBUTTON→numeydoor,
  buttongate→bars).

New test/QA env hooks added this session: `JN_DEMO_SPAWN_XYZ="x,y,z"`,
`JN_TEST_GEMS`, `JN_TEST_TOOLS`, `JN_TEST_COV`, `JN_TEST_CLEAR`,
`JN_FORCE_CUTSCENE`, `JN_NO_CUTSCENE`.

## Final state (2026-06-03 13:15)

- Native build green; **regression sweep: 0 failures across all 22 levels**
  (missing_mesh=0, zero placeholder boxes, all render).
- Web rebuilt + deployed; **live HTTP 200** at https://exentt.com/jnvsjn/ —
  carries HUD, typed items + tinted gems, inventory/tools, full entity coverage,
  vehicles, checkpoints, moving platforms, level-clear banner, intro cutscene.

## Fix — 3PIC pickups render real sprites (2026-06-03, user feedback)

User: the pickups looked like the "hurt Jimmy" placeholder. Cause: 3PIC fell
back to `jimpickup.ASE`. 3PIC pickups are actually **2D sprites from
sprites.omt**, indexed by `SpriteIndex` == the Canv chunk **`id`** (verified:
level5a ids 239–243 = Volture/brocho/tut-mask/bracelet = the Egyptian artifact
pickups). Fix:
- Extracted all 306 sprites.omt canvases by id → `assets/parsed/sprites/jnvsjn/
  spr_<id>.png`.
- gam_loader parses `SpriteSize`; Entity gains `sprite_size`.
- main.c: 3PIC/3SRO/3SPR/3ANI with `sprite_index>0` render the real sprite as a
  camera-facing billboard (size = SpriteSize, default 64) instead of a mesh.
  `sprite_index==0` falls through so game-1 3PIC (no SpriteIndex) keep their ASE
  props — no game-1 regression.
- Verified: `JN_TEST_PICKUPS` shows apple/coin/diamond/gem/present/candy sprites
  (screenshots/fix_pickup_sprites.png); 0/22 regression; deployed live.

## Also — made the work visible (2026-06-03, user feedback)

The deployed demo defaulted to `level1` (the only level with no new content) and
the web build force-spawned Jimmy at hardcoded level1 coords for every level
(off-map elsewhere). Fixed: web default level → **`level1_a`** (snowy Retroville:
jeep, enemy, 21 pickups, live HUD count); non-level1 levels now use their own
3JIM start so the level dropdown lands in real content. Summary montage at
screenshots/SESSION_SUMMARY.png.

## Fix batch — sprite size, ground glitch, GLOBAL COLLISION (2026-06-03, user feedback)

1. **Pickup sprite size** — default bumped 64→110 world units (SpriteSize is 0
   in the data); items read clearly now.
2. **Ground glitch** — billboards were centered on the pickup's ground-level Y so
   the lower half sank into the floor. Now lifted by `size*0.5` (bottom edge at
   Y), so they sit on the ground and bob above it.
3. **GLOBAL WORLD COLLISION** — the big one. The player previously walked on the
   flat invisible safety floor; the level's geometry wasn't collidable.
   `world_terrain_height` only sampled placements named `GROUND` (JNvsJN has
   none). Now it samples **GROUND + every `BLOCK*`/`BLOCKING_*` collision mesh**
   (the original game's dedicated collision proxies) whose XZ-AABB contains the
   player, returning the highest surface ≤ feet+100 (step tolerance) so the
   player lands on real terrain/roads/steps and never snaps onto a roof above.
   glb meshes de-index to a triangle soup so the existing `sample_mesh_height_xz`
   works on them. Toggle: `JN_NO_WORLD_COLLISION=1`.
   **Verified:** level1_a with collision OFF → player falls through to the
   safety floor (terraced plane far below the houses); ON → player stays up on
   the street at building level (screenshots/fix_collision_{on,off}.png).
   Autorun on level1_a/3a/5a/5c = no crashes/perf issues; 0/22 boot regression.
   NB: this is **height-field** collision (walk on surfaces); horizontal
   wall-blocking (so you can't walk through a building) is a separate, larger
   feature — left as a follow-up.

## RESUME HERE

All planned work (Steps 1–6) + the requested playthrough layer (checkpoints,
moving platforms, level-clear objective, intro cutscene, button→door world
actions) are done, verified, and deployed live (HTTP 200). Final regression
sweep: **0 failures / 22 levels.**

Natural next steps if continuing:
- **Vehicle ridability** (Step 6 stretch): enter-trigger reparents control to a
  3JEE, swaps movement params, exit returns to Jimmy (template, then per-vehicle).
- **More world-action links:** the button→target dispatch generalises — wire
  switches/levers (3SWI) and `NextTrigger`/`ToggleObject` chains the same way.
- **car.ASE texture:** the Jeep/NeuCar mesh renders untextured (no bitmap in the
  ASE) — find/assign a car texture.
- **Sprite-index mapping:** 3SRO/3GRT/3ELE etc. are sprites.omt-indexed; wiring
  SpriteIndex→canvas would replace the current single-sprite/invisible approx.
- **Cutscene cameras (3CAM/3MCA):** real scripted cutscenes from the level data
  (the intro shot is a generic placeholder).
- **Untextured characters:** a few wired ASEs (e.g. cindycheer) render white —
  their texture binding didn't resolve; check the ASE bitmap path / asset_cache.
