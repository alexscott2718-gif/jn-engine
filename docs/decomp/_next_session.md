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
- **Still unimplemented:** the rest of the enemy roster (Digger/Tank/Tesla/Harrier/turret/mine/
  laser), friends/NPCs (only the player), vehicles (visual-only), pickups→ability wiring, and the
  game-flow/level-controller layer (`main.c` is a generic loop, not a port of
  `CJimmyGame`/tasks/menus/cutscenes). ~90 of 208 specs have a doc but no runtime behavior.
- Implementation contract is `EntityVTable` in `src/engine/world.h`, registered by FourCC in
  `src/game/entities.c`; per-frame via `entity_update()` (`main.c:1418`); `.gam` params via
  `gam_prop_f/i`. Full contract in plan §1.

## Your task this session: Wave N3 (player combat + the pickups family)
Per plan §4, build on N2's `behavior_projectile.c` and the health model. Replace the generic
`vt_item` with per-type behaviors where the spec differs, and wire inventory → ability:
- `C3DBalloon`, `C3DBaseball`/`C3DBaseballPickup`, `C3DBubble`/`C3DBubblePickup`,
  `C3DGraplingHook`/`C3DHook`, `C3DShrinkRay`, `C3DMetalPickup`, `C3DHelmet`.
  (`docs/decomp/C3DPickupItem.md`, `C3DPickupType.md`, `CPickupType.md`.)
- Gate the player's **F**-key throw (already wired in `behavior_player.c` as the N2 seed) behind
  actually carrying the baseball, and route the thrown/fired items through `behavior_projectile.c`.
- The shared projectile + `gamestate` health/hit model from N2 are the foundation — reuse them.

**Remaining enemy roster (defer or fold into a later N2.x pass):** `C3DDigger`, `C3DTank`,
`C3DTesla`, `C3DHarrier`, `C3DEnemyAircraft`, `C3DMine`, `C3DLaserTrigger`, `C3DYokTurret`,
`C3DYokianShip`. The ranged ones (turret/tank/tesla/harrier/mine) are the natural first consumers
of enemy-side `behavior_projectile.c` (`PROJ_TEAM_ENEMY`).

**Done when:** picking up an item grants its ability and the player can use the thrown/fired items
against the Wave N2 enemies, `tools/audit_faithfulness.py` stays at 0 findings, and affected levels
pass `JN_SCREENSHOT` spot checks.

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
Wave N3 grants at least one pickup's ability on collection and lets the player use a thrown/fired
item (through `behavior_projectile.c`) to defeat a Wave N2 enemy, validation is clean, and per-class
commits are pushed on `native-port`. If the wave closes, append a Wave N3 paragraph to
`PROJECT_HISTORY.md` and update this handoff to point at Wave N4 (vehicles).
