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
4. `~/jn-engine/docs/asset_catalog/behavior_todo.md` — generated behavior coverage lens:
   used-in-level FourCCs with no native vtable, ranked by `instances * level_count`.
5. `~/jn-engine/docs/decomp_ledger.csv` — 208 rows, all `status=spec`; per-class behavior
   source of truth is `docs/decomp/<Class>.md`.
6. Exemplar behaviors before writing any: `src/game/behaviors/behavior_walker.c` (AI + nav),
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
  `qa_web_verify.py` 16/16. Public WASM deploy remains outward-facing and gated on explicit approval.
- **Wave N2.x placed actors PARTIAL DONE (2026-06-23):** the first behavior-lens pass landed `3TUR`/
  `C3DYokTurret` (enemy-side `PROJ_TEAM_ENEMY` shots + baseball defeat), `3TES`/`C3DTesla` (active electric
  contact hazard), `3LAS`/`C3DLaserTrigger` (proximity damage + relay/toggle target activation), `3YSH`/
  `C3DYokianShip` (placed patrol actor; **not** the shield helper), `3EYE`/`C3DEYE` (C3DAICar/C3DAI patrol),
  `3DIG`/`C3DDIGGER`, `3HOO`/`C3DHook`, and `3CIN`/`C3DCindy` (first friend/NPC, with `TaskName` and
  `TalkTrigger0..4` string plumbing plus SCENE-window visibility). Validation: `make`, affected
  `JN_SCREENSHOT` probes, `audit_faithfulness.py` 0 findings, and `qa_web_verify.py` 16/16.
- **Still unimplemented:** the refreshed generated behavior lens reports **53** used-in-level FourCCs with no
  native behavior (**93** used FourCCs total, **40** used FourCCs with native vtables). The next frontier is the
  remaining actor/gameplay rows from `docs/asset_catalog/behavior_todo.md`, especially `3AIT`, `3PHO`, `3RCK`,
  `3HUM`, and the friend/NPC cast. Code-spawned or currently unplaced enemy specs (`3TAN`/`C3DTank`, `3HAR`/
  `C3DHarrier`, `3MIN`/`C3DMine`, `3MIS`/`C3DMissile`) still have zero current `.gam` rows; treat them as
  database-spawned/code-spawned/unplaced until separate evidence says otherwise. The big structural waves
  (N1 base framework, N2 enemies, N3 combat/pickups, N4 vehicles, N5 game-flow) are all landed.
- Implementation contract is `EntityVTable` in `src/engine/world.h`, registered by FourCC in
  `src/game/entities.c`; per-frame via `entity_update()` (`main.c:1418`); `.gam` params via
  `gam_prop_f/i`. Full contract in plan §1.

## Your task this session: continue the behavior_todo actor/NPC queue
The big structural waves (N1–N5) are landed, and the first placed enemy/actor slice is now covered. The largest
remaining *gameplay* gap is the rest of the actor/NPC queue. Hold the decomp discipline — confirm behavior
against `docs/decomp/<Class>.md` (the decompiled body, not an offset scan), build on the existing N1–N4 modules,
and **pick targets from the generated behavior lens**:

```bash
python3 tools/build_asset_catalog.py
sed -n '1,180p' docs/asset_catalog/behavior_todo.md
```

`behavior_todo.md` formalizes the strict query (`instances > 0 && native_behavior == null`), ranks by
`instances * level_count`, and splits out an actor/gameplay focus section. Use it to validate on levels that
actually place each class. The live catalog remains useful for previews: <https://exentt.com/JN-assets/catalog/>.

**Next actor/gameplay rows from the refreshed lens:**
- `3AIT` / `C3DAITrigger` — 174 instances, 24 levels. Read the spec carefully; this is now the broadest placed
  behavior hole and likely owns trigger/AI mission wiring rather than visual movement.
- `3PHO` / `C3DPHONEBOOTH` — 21 instances, 17 levels.
- `3RCK` / `C3DRocket` — 9 instances, 5 levels (distinct from the already-rideable `3ROC`/`C3DRocketShip`).
- `3HUM` / `C3DHumphrey` — 10 instances, 4 levels.
- Friend/NPC cast: `3FLE`/`C3DFleetCommander`, `3NIC`/`C3DNick`, `3SHE`/`C3DSheen`, `3BEN`/`C3DBENNY`,
  `3LIB`/`C3DLibby`, `3SUM`/`C3DSumo`, `3HUG`/`C3DHugh`, `3KIT`/`C3DKitty`, `3MOM`/`C3DJUDY`,
  `3ULT`/`C3DUltraLord`, and `3PIR`/`C3DPirate`.

**Friend / NPC wiring now available:** `C3DCindy` added the shared string plumbing (`Entity.task_name` and
`Entity.talk_trigger[5]`) plus `game_flow_current_level()`. Continue to use the `CTaskList` entity-state table
(`game_flow_entity_state(tag)`) for mission-actor visibility/state gates, and lean on `behavior_ai.c` for
friend/NPC idle/patrol movement before adding per-character special cases.

**Known zero-placement enemy specs:** `3TAN`/`C3DTank`, `3HAR`/`C3DHarrier`, `3MIN`/`C3DMine`, and
`3MIS`/`C3DMissile` still have specs but zero current `.gam` placement. Keep them in the code-spawned /
database-spawned / unused bucket until separate evidence says otherwise.

**Two small N5 tails worth picking up:** (1) a real **text renderer** for the menu/HUD (today `menu.c` draws
bars + logs labels; `C2DInGameMenu` counters print numerals); (2) plumbing the `PlayerControlled` string prop
onto the entity so cutscenes can lock player input (deferred in N5).

**Done when:** the selected actor/NPC row(s) have native vtables, are validated against levels that actually
place them, `tools/audit_faithfulness.py` stays at 0 findings, affected levels pass `JN_SCREENSHOT` spot checks,
and (wave end) `qa_web_verify.py` is still 16/16 with a one-line PROJECT_HISTORY paragraph + this handoff and
`behavior_todo.md` refreshed.

## Inner loop (per class)
1. Read `docs/decomp/<Class>.md` (§Field map, §Per-frame behavior, §Constants). Confirm
   behavior against the documented decompiled body — not an offset/immediate scan.
2. Write/extend `src/game/behaviors/behavior_<x>.c`; read authored params with `gam_prop_*`.
   Add an `Entity` field only when several behaviors need it; else reuse scratch/`props`.
3. Register the FourCC → vtable in `src/game/entities.c`.
4. Build (`make`), then validate per plan §3: `JN_SCREENSHOT` on affected levels,
   `tools/audit_faithfulness.py`, and motion-diff vs the marked `.omtc` where dynamics matter.
5. At wave end, refresh the catalog (`python3 tools/build_asset_catalog.py`) and run
   `python3 tools/qa_web_verify.py`. Deploy public WASM only when explicitly requested.
6. Commit per class: `port(<Class>): <one-line behavior>`.

## Hard rules
- Commit ONLY port artifacts: `src/**`, `tools/**`, and (at wave end) the handoff/catalog docs
  (`PROJECT_HISTORY.md`, `docs/decomp/_next_session.md`, `docs/asset_catalog/**`).
  There is a large PRE-EXISTING dirty asset tree — do NOT stage or touch it.
- Don't relitigate settled invariants (matrix convention, `PROJ[3][3]=1`, no X-mirror,
  `canvas_id=Canv+1`, DIFFUSE alpha 0, no fog) — see `PROJECT_HISTORY.md` §Invariants.
- Capture (`.omtc`) is a **validator**, never a runtime dependency.
- The per-class specs + ledger + `PROJECT_HISTORY.md` + `native_port_plan.md` are the only
  source of truth.

## Definition of done for this session
Pick the next actor/gameplay row(s) from `behavior_todo.md`, read each class spec before coding, and land one
commit per class on `native-port`. Validate each class with `make`, affected-level `JN_SCREENSHOT` checks, and
`tools/audit_faithfulness.py`; at wave end refresh `behavior_todo.md`, run `tools/qa_web_verify.py` (16 checks),
append `PROJECT_HISTORY.md`, and update this handoff. With N1–N5 plus the first N2.x slice landed, the campaign's
base framework, enemies, combat, vehicles, game-flow, placed ranged enemies, placed trigger hazards, and first
friend NPC are ported — the remaining work is the long tail of actor/NPC behavior coverage.
