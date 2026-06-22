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
- **Still unimplemented:** the rest of the enemy roster (Digger/Tank/Tesla/Harrier/turret/mine/
  laser), friends/NPCs (only the player), and the **game-flow/level-controller layer (Wave N5)** —
  `main.c` is a generic loop, not a port of `CJimmyGame`/tasks/menus/cutscenes. ~75 of 208 specs have a
  doc but no runtime behavior.
- Implementation contract is `EntityVTable` in `src/engine/world.h`, registered by FourCC in
  `src/game/entities.c`; per-frame via `entity_update()` (`main.c:1418`); `.gam` params via
  `gam_prop_f/i`. Full contract in plan §1.

## Your task this session: Wave N5 (game-flow / level controllers)
Per plan §4 (the biggest structural gap), port the controller layer that today is faked by a generic
`main.c` loop. Hold the decomp discipline — confirm behavior against `docs/decomp/<Class>.md`, table-drive
the repetitive parts, special-case only the outliers:
- `CJimmyGame` master controller + `CLoadLevel` + `CTaskList` → a real **objective / win-condition /
  task-state** layer (replace the ad-hoc `gamestate` level-clear logic). The existing
  `behavior_base.c` progress-gate bridge (`JN_PROGRESS_LEVEL`) and `gamestate` level-swap plumbing are
  the seeds.
- `CMainMenu` / `CMenuElement` / `C2DInGameMenu` → menu, pause, in-game HUD menu.
- The ~40 `CLevel*Game` / `CLevelVR*` → a **data-driven per-level script table** (most are thin hooks;
  a few carry real logic, e.g. `CLevel01FGame` death/restart). Decompile `CLevel01AGame` as the exemplar,
  then diff the rest — don't hand-port 40 near-identical controllers.
- Cutscene sequencing (`CMultiCutSceneCamera` + the cutscene-camera classes) → a scripted camera timeline.
  (`docs/decomp/CJimmyGame.md`, `CTaskList.md`, `CMainMenu.md`, `CLevel01AGame.md`, and the controller specs;
  cross-check each `CLevel*Game` against its `.gam`.)

**Remaining enemy roster (a later N2.x pass):** `C3DDigger`, `C3DTank`, `C3DTesla`, `C3DHarrier`,
`C3DEnemyAircraft`, `C3DMine`, `C3DLaserTrigger`, `C3DYokTurret`, `C3DYokianShip`, plus `C3DHook` (3HOO,
the level4b AI hook). The ranged ones are the natural first consumers of enemy-side
`behavior_projectile.c` (`PROJ_TEAM_ENEMY`).

**Done when:** a level can be entered from a menu, its objectives tracked to a win state, and it transitions
to the next level without per-level C hardcoding; `tools/audit_faithfulness.py` stays at 0 findings, and
affected levels pass `JN_SCREENSHOT` spot checks.

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
Wave N5 lets a level be entered from a menu, tracks its objectives to a win state via a real task layer,
and transitions to the next level without per-level C hardcoding, validation is clean, and per-class
commits are made on `native-port`. If the wave closes, append a Wave N5 paragraph to `PROJECT_HISTORY.md`
and update this handoff (the native-port campaign would then have its base framework, enemies, combat,
vehicles, and game-flow all ported — see `native_port_plan.md` for any remaining N2.x/cleanup tails).
