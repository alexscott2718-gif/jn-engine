# Next Session — Kickoff Prompt (Native Linux Port)

> Paste this (or point the agent at this path) to resume work. The decomp **spec** campaign
> is finished; the active campaign is now the **native C port** of the gameplay classes.
> Living doc: update "Current state" + "Your task" at each wave boundary so the next fresh
> session starts oriented. Source of truth: the per-class specs (`docs/decomp/*.md`), the
> ledger, `PROJECT_HISTORY.md`, and the wave plan (`docs/native_port_plan.md`).

---

You are porting `Neutron.exe`'s gameplay classes into **runtime behavior in the native C
engine** (`jnengine`), using the already-written behavioral specs as the source of truth.
This is a shared, committed campaign — read the shared docs, don't rely on tool-private memory.

## Orient yourself first (read in this order)
1. `~/AGENTS.md` (== `~/CLAUDE.md`; machine/workflow conventions, anti-silo policy)
2. `~/jn-engine/docs/native_port_plan.md` — **the execution plan** (§1 implementation
   contract, §3 validation, §4 the waves, §5 sequencing). **Start here.**
3. `~/jn-engine/docs/PROJECT_HISTORY.md` + `docs/ARCHITECTURE.md` — engine state + invariants.
4. `~/jn-engine/docs/decomp_ledger.csv` — 208 rows, all `status=spec`; per-class behavior
   source of truth is `docs/decomp/<Class>.md`.
5. Exemplar behaviors before writing any: `src/game/behaviors/behavior_walker.c` (AI + nav),
   `behavior_player.c` (physics/input), `behavior_button.c` (activation wiring).

## Current state (branch: `native-port`)
- **Decomp spec campaign DONE:** all 208 `C*` gameplay classes have `docs/decomp/<Class>.md`
  at `status=spec`. Do not re-derive specs from the binary; consume them.
- **Godot retired (2026-06-22):** `docs/godot_bridge_plan.md` is superseded. The specs feed
  the **native port**; the C engine in `src/` is the product.
- **Native engine today (see plan §0 survey):** renderer + level geometry + mechanical props
  (fan/switch/geyser/pendulum/button/door/platform/checkpoint) + player movement + Carl's
  patrol walker all work. **Wave N1 DONE:** `behavior_base.c` covers C3DObject/C3DAnimated
  lifecycle gates and trigger activation, `movement_base.c`/`behavior_flying.c` cover the
  C3DFlyingObject movement base, and `behavior_ai.c` owns reusable idle/seek/patrol helpers
  with `vt_walker` repointed through it.
- **Wave N2 DONE (enemies/AI — Yokian family):** `behavior_enemy.c` ports the `C3DYokian`
  humanoids (3SOL/3GUA/3SPY) on the `C3DAI` seek/scan/attack machine — acquire the player in
  `VisibleRange`+FOV, chase via `behavior_ai` seek, melee-strike on cooldown (soldiers are melee
  in the decomp; no enemy projectile). `behavior_projectile.c` (FourCC `PROJ`) is the shared
  team-tagged spawn→integrate→`world_query_segment`→overlap-damage path; the player's **F**-key
  baseball defeats Yokians (`C3DYokian::ReactToHitObject` ← `C3DBASEBALL`). Player health is in
  `gamestate.c` (`gamestate_damage_player`); enemy knockout uses `Entity.hp` + KO dwell. Validation
  (level6): soldier chased the player into attack range and struck (health 100→70); a thrown
  baseball logged `[ENEMY] 3SOL 'yoksol' defeated`. `audit_faithfulness.py` = 0 findings; WASM
  deployed (`jnengine.83768ba3.js`, assets `f056e20c`); `qa_web_verify.py` passed. Headless combat
  exerciser: `JN_TEST_THROW=<tick>` throws at the nearest Yokian.
- **Wave N3 DONE (player combat + pickups family):** `behavior_balloon.c` ports `C3DBalloon` (`3BAL`,
  60 real instances) — Jimmy's touch releases it (it drifts up), a thrown baseball (`PROJ_TEAM_PLAYER`)
  pops it for score (decomp distance-bonus + 10/200 base). The **F** throw is now gated behind
  `gamestate_has_tool("baseball")`, granted in-level by a `3PIC` awarding `PIC_NUMBER==6`
  (level1c/level2a/Level2b) or by `behavior_pickup.c`'s ability pickups (`3BPU` baseball, `3BUP` bubble,
  `3HEL` helmet, `3MEP` score; abilities = gamestate inventory tools). Validation: Level2 logged
  `[BALLOON] popped … (+69 pts)`, N2 defeat un-regressed, `audit_faithfulness.py` 0 findings,
  `qa_web_verify.py` 16/16. `JN_TEST_BASEBALL=1` grants the baseball; the headless `JN_TEST_THROW` hook
  now also targets the nearest `3BAL`. Deferred: `3SHR`/`3GRA`/`3BUB`/`3BAS` (0 instances; props/effects/
  objects, not pickup→ability) and `3HOO` (an AI object → N2 track); `3MEP`'s Goddard beacon waits on
  `C3DGoddard`.
- **Wave N4 DONE (vehicles):** `behavior_vehicle.c`. `vt_rocket` (`3ROC` C3DRocketShip, placed every level)
  is **player-rideable** — walk in + **E** to board, fly via N1 `behavior_flying_update_base` (move keys +
  SPACE/CTRL), **E** to dismount; the rocket integrates its own position (not a `PHYSICS` entity) and snaps
  the flag-cleared player onto it so the camera follows. `vt_ai_vehicle` (`3SUV`/`3SBU`/`3SAI`) is
  **self-driving** via the N1 `behavior_ai` patrol primitive. Validation: Level1 rocket climbed 156→210
  (`JN_TEST_RIDE=<tick>`); Level2 bus drove ~391 units along `bus01`; audit 0 findings; `qa_web_verify` 16/16.
  Deferred: full `C3DVehicle` car-sim (`3CAR`=Carl; car leaves `3JEE`/`3NCA`/`3NC2`/`3POD`/`3SUB`/`3BOA`/`3WHE`
  have 0 instances) + SUV light-cone / AICar horn / SailBoat bob (need C3DLightCone/effect/Goddard).
- **Wave N5 DONE (game-flow / level controllers):** the controller layer that was faked by a generic
  `main.c` loop is ported in four pieces. `task_loader.c` ports **`CTaskList`** — the `.tsk` deserializer,
  byte-exact with `tools/tsk_parser.py` (`NewGame.tsk` → `level1b.gam`, spawn `470.5965/609.2417/-87.7631`,
  12-entity table incl. `SCENE=30`); falls back to a baked NewGame default since the `.tsk` binaries aren't
  committed. `game_flow.c` ports **`CJimmyGame`** — `InitGame` seeds lives=5/`mission_value`=100/active=1, the
  **40 zero-method `CLevel*Game`/`CLevelVR0N` leaves collapse into one level table**, death routes through the
  lives/restart flow (`C2DInGameMenu` semantics; `gamestate` no longer auto-refills, exposes
  `gamestate_player_is_down()`), and the win condition bridges `gamestate`'s level-clear to
  `game_flow_level_objective_met()`. `behavior_cutscene.c` ports **`C3DCutSceneCamera`/`C3DMultiCutSceneCamera`**
  (`3CAM`/`3MCA`) — each `3CAM` registers a shot (CameraTarget+TargOffset+dists+ZoomSpeed+LookVoffset) and the
  runtime sequences the level's shots through the follow-cam slot. `menu.c` ports **`CMainMenu`** — a `--menu`
  front-end routing New Game → `NewGame.tsk` → `level1b` (campaign mode on) / the 8 VR levels / Quit through the
  level-swap machinery. Campaign entry / cutscene / menu are gated behind `--newgame` / `--menu` / `JN_CUTSCENE`,
  so direct `--level X` launches keep campaign mode OFF and the audit + matched-camera validators render exactly
  as before. The `RequiredLevel` visibility gate stays env-driven (`JN_PROGRESS_LEVEL`) — mapping campaign
  progress → the original's ~550-range thresholds is a documented open question (no save-progress ground truth).
  Validation: `--newgame` starts at `level1b` (lives=5); `--menu` auto-confirms New Game → swaps into `level1b`;
  `level1a` sequences its 3 cutscene shots; `audit_faithfulness.py` 0 findings; WASM rebuilt and
  `qa_web_verify.py` 16/16. Web deploy (`./tools/deploy_wasm.sh`) is the only wave-end step left and is
  outward-facing — gated on user approval.
- **Still unimplemented:** the rest of the enemy roster (Digger/Tank/Tesla/Harrier/turret/mine/
  laser), and friends/NPCs (only the player). ~60 of 208 specs have a doc but no runtime behavior. The big
  structural waves (N1 base framework, N2 enemies, N3 combat/pickups, N4 vehicles, N5 game-flow) are all landed.
- Implementation contract is `EntityVTable` in `src/engine/world.h`, registered by FourCC in
  `src/game/entities.c`; per-frame via `entity_update()` (`main.c:1418`); `.gam` params via
  `gam_prop_f/i`. Full contract in plan §1.

## Your task this session: Wave N2.x (remaining enemy roster) + friends/NPCs
The big structural waves (N1–N5) are landed. The largest remaining *gameplay* gap is the rest of the enemy
roster and the friend/NPC cast. Hold the decomp discipline — confirm behavior against `docs/decomp/<Class>.md`
(the decompiled body, not an offset scan), build on the existing N1–N4 modules, and **pick targets from the
Asset Catalog's behavior column** — `python3 tools/build_asset_catalog.py` then read `docs/asset_catalog/`
(or the live `exentt.com/JN-assets/catalog/`); it already lists every used-in-level FourCC, its instance count
× level reach, its visual, and whether a native vtable exists, so the wave is validated on levels that actually
place each class (no more ad-hoc corpus grepping).

**Remaining enemy roster (consume the existing `behavior_ai.c` + `behavior_projectile.c`):** `C3DDigger`,
`C3DTank`, `C3DTesla`, `C3DHarrier`, `C3DEnemyAircraft`, `C3DMine`, `C3DLaserTrigger`, `C3DYokTurret`,
`C3DYokianShip`, plus `C3DHook` (`3HOO`, the level4b AI hook). The **ranged ones** (turret/tank/harrier/tesla)
are the natural first consumers of **enemy-side `behavior_projectile.c` (`PROJ_TEAM_ENEMY`)** — that path
already exists from N2 (player side) and just needs an enemy spawner + the player-damage hit branch.
`C3DMine`/`C3DLaserTrigger` are proximity/trigger damagers (lean on `behavior_trigger_spawn_base`).

**Friends / NPCs:** only the player is ported. The `CTaskList` entity-state table (MOM/LIBBY/BENNY/SCENE/
REACTOR — seeded via `game_flow_entity_state(tag)`) is the mission-actor hook for these; the `3CAM` cutscene
TargetActAnim/FaceObject and the `SCENE=30` sequencer state are the wiring points.

**Two small N5 tails worth picking up:** (1) a real **text renderer** for the menu/HUD (today `menu.c` draws
bars + logs labels; `C2DInGameMenu` counters print numerals); (2) plumbing the `PlayerControlled` string prop
onto the entity so cutscenes can lock player input (deferred in N5).

**Done when:** at least the ranged-enemy track fires `PROJ_TEAM_ENEMY` projectiles that damage the player and
can be defeated, validated against the levels that place them; `tools/audit_faithfulness.py` stays at 0 findings,
affected levels pass `JN_SCREENSHOT` spot checks, and (wave end) `qa_web_verify.py` 16/16 with a one-line
PROJECT_HISTORY paragraph + this handoff updated.

## Inner loop (per class)
1. Read `docs/decomp/<Class>.md` (§Field map, §Per-frame behavior, §Constants). Confirm
   behavior against the documented decompiled body — not an offset/immediate scan.
2. Write/extend `src/game/behaviors/behavior_<x>.c`; read authored params with `gam_prop_*`.
   Add an `Entity` field only when several behaviors need it; else reuse scratch/`props`.
3. Register the FourCC → vtable in `src/game/entities.c`.
4. Build (`make`), then validate per plan §3: `JN_SCREENSHOT` on affected levels,
   `tools/audit_faithfulness.py`, and motion-diff vs the marked `.omtc` where dynamics matter.
5. At wave end, build/deploy the public WASM version with `./tools/deploy_wasm.sh`, then run
   `python3 tools/qa_web_verify.py`.
6. Commit per class: `port(<Class>): <one-line behavior>`.

## Hard rules
- Commit ONLY port artifacts: `src/**`, `tools/**`, and (at wave end) `PROJECT_HISTORY.md`.
  There is a large PRE-EXISTING dirty asset tree — do NOT stage or touch it.
- Don't relitigate settled invariants (matrix convention, `PROJ[3][3]=1`, no X-mirror,
  `canvas_id=Canv+1`, DIFFUSE alpha 0, no fog) — see `PROJECT_HISTORY.md` §Invariants.
- Capture (`.omtc`) is a **validator**, never a runtime dependency.
- The per-class specs + ledger + `PROJECT_HISTORY.md` + `native_port_plan.md` are the only
  source of truth.

## Definition of done for this session
The ranged-enemy track (turret/tank/harrier/tesla) fires `PROJ_TEAM_ENEMY` projectiles that damage the player
and can be defeated, validated on the levels that place those FourCCs; the proximity damagers (mine/laser) hurt
on contact; validation is clean (`audit_faithfulness.py` 0 findings, `JN_SCREENSHOT` spot checks) and per-class
commits are made on `native-port`. At wave end, rebuild + run `qa_web_verify.py` (16 checks), append a paragraph
to `PROJECT_HISTORY.md`, and update this handoff. With N1–N5 already landed, the campaign's base framework,
enemies, combat, vehicles, and game-flow are all ported — this wave fills the remaining enemy/NPC gameplay gap.
