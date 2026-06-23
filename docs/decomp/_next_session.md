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
- **Wave N2.y actor/NPC coverage DONE (2026-06-23):** cleared the friend/NPC cast and the broadest hole. Shared
  `vt_friend` (`behavior_friend.c`) ports the `C3DFriends`/`C3DAI` idle leaves `3NIC`/`3SHE`/`3ULT`/`3LIB`/`3HUG`/
  `3BEN`/`3MOM`/`3KIT` (idle-or-patrol on the C3DAI base, turn to watch Jimmy in VisibleRange, InitiallyVisible/
  level gate; talk rewards deferred). `3FLE`/`C3DFleetCommander` (a `C3DYokian`) routes to `vt_yokian` + the
  Yokian hit-reaction set. Shared `vt_escort` (`behavior_escort.c`) ports `3SUM`/`C3DSumo` + `3PIR`/`C3DPirate`:
  Jimmy contact teleports the player to the actor's serialized `StartPoint` marker on a cooldown (carried sequence
  + counter-0x1b gate deferred). Headliner `3AIT`/`C3DAITrigger` (`behavior_ai_trigger.c`): invisible
  self-detecting mission-wiring volume that mutates a named `AITarget` (hide/show, AINewPos marker teleport,
  AINewRotY, AIPatrol repoint) then dispatches `ToggleObject`/`NextTrigger`; conservatively gated (arm-on-exit +
  `TouchActivated` + `ActivateBy`/`IsA` + `TimesToTrigger`) so it never fires during the 2-tick probes and the 121
  chain-dispatched rows stay inert (no scripted-chain dispatch yet). Added a generic string-prop bag to the `.gam`
  loader (`gam_str()`) plus a `JN_TEST_AITRIG` headless hook (level6 `jimend` teleports the player to `jnspot`,
  verified). Validation: `make`, `make web`, `JN_SCREENSHOT` probes, `audit_faithfulness.py` 0 findings (all 35
  levels), `qa_web_verify.py` 16/16.
- **Actor focus closeout DONE — 3PHO / 3RCK / 3HUM (2026-06-23):** cleared the final three rows of the
  actor/gameplay focus section. `3PHO`/`C3DPhoneBooth` (`behavior_phonebooth.c`, `vt_phonebooth`): SOLID red
  phone-booth prop (phone.glb) honoring the IV/HasCollision gates (one Level1 booth is IV=0/non-solid) + Jimmy-only
  proximity contact; the decompiled touch handler gates on `IsA("C3DJIMMY")` but its player-side effect
  (`vfunc_01_016` → `player.method_0x1d4`) is unresolved in the decomp and deferred — and the booth tag
  (`C3DPHONEBOOTH`) is *not* the player's StartPoint marker (`PHONEBOOTH`, a separate `STRT`), so it is not a spawn
  anchor. `3RCK`/`C3DRocket` (`behavior_rocket_ai.c`, `vt_rocket_ai`): the *placed* C3DAI patrol rocket (9 `.gam`
  rows, NOT code-spawned; distinct from the rideable `3ROC`) flies its authored PatrolPoint chain via the shared
  `behavior_ai` primitive — validated on level1e (patrols toward `RC1` at 600 u/s); smoke-puff pool + objects.omt
  id-15 sprite deferred (rendered hidden). `3HUM`/`C3DHumphrey` (`behavior_humphrey.c`, `vt_humphrey`): the
  C3DEnemy clone-controller hides itself on spawn (faithful `PostLoadHideHumphrey`; supersedes the prior
  fall-through that drew an idle humpstop mesh) and the `SCENE==0x5a` clone-reveal gate (`CLONE1..CLONE7`) is wired
  as decompiled but **dormant** — SCENE only comes from the CTaskList table (`SCENE=30`) and no SCENE sequencer is
  ported, so it never reaches 90. Validation: `make`, `make web`, `JN_SCREENSHOT` (Level1/Level2 booth renders;
  Humphreys hidden), `audit_faithfulness.py` 0 findings, `qa_web_verify.py` 16/16.
- **Base/effect tail pass 1 DONE — 3NEU / 3RED / 3ARR (2026-06-23):** `behavior_neutron.c` ports the two neutron
  billboard pickup classes. `3NEU`/`C3DNeutron` now uses the concrete class spec (not just inherited
  `C3DSprite`): runtime `sprites.omt` frames 0..12, idle frames 0..7, Jimmy overlap collection sound/burst/hide,
  then respawn. `3RED`/`C3DRedNeutron` adds authored `Radius`, red pulse/tint, red-neutron sound, burst/hide, and
  conservative `NextTrigger` forwarding when the target exposes native `on_trigger` (full scripted trigger-chain
  dispatch remains deferred with the trigger system). `3ARR`/`C3DArrow` (`behavior_arrow.c`) is a thin
  C3DSpriteType nav-billboard vtable over the existing `sprites.omt` chunk-33 visual path, honoring the shared
  progress/visibility gate. The sprite resolver now allows `sprites.omt` frame 0 only for the neutron runtime
  classes so `3PIC`/index-0 fallback behavior is unchanged. Validation: `make`, `make web`, focused
  `JN_SCREENSHOT` probes, explicit overlap logs for `[NEUTRON]`/`[REDNEUTRON]`, `audit_faithfulness.py` 0 findings,
  `qa_web_verify.py` 16/16.
- **Base/effect tail pass 2 DONE — 3LIO / 3OMT / 3CON (2026-06-23):** `3LIO`/`C3DLightObj`
  (`behavior_lightobj.c`, `vt_lightobj`) is an invisible light-data row that runs the shared visibility/progress
  gate and preserves authored color/alpha/pulse/sound props without inventing a native lighting side effect.
  `3OMT`/`C3DOmtObj` (`behavior_omtobj.c`, `vt_omtobj`) ports the gameplay half of the authored OMT shape props:
  the visual resolver already binds `OmtDatabase`/`OmtIndex`, while the vtable now applies inherited gates,
  `Radius`-derived collision extents, and `HasCollision==0` solidity clearing. `3CON`/`C3DCone`
  (`behavior_cone.c`, `vt_cone`) is a non-solid C3DSpriteType decor leaf over the existing `sprites.omt` chunk-41
  billboard path. Validation: `make`, focused `JN_SCREENSHOT` probes (`level4c`, `Level3C`, `level2a`, `Level2b`),
  `audit_faithfulness.py` 0 findings after each class, `make web`, and `qa_web_verify.py` 16/16. The refreshed
  catalog now reports **93** used FourCCs, **61** with native vtables, and **32** still missing.
- **Base/effect tail pass 3 DONE — 3TRO / 3LEA / 3TAR / 3AIO (2026-06-23):** four more base/resolver + effect rows,
  with each spec read first to fix the prior handoff's class guesses. `3TRO`/`C3DVRTrophy` (`behavior_trophy.c`,
  `vt_trophy`) is the VR challenge-level reward: Jimmy's contact collects it (hide + stop triggering) and signals
  `game_flow_level_objective_met()` — a no-op flag when campaign mode is off, so the audit/screenshot harnesses are
  unchanged and the 2-tick probes never overlap it; visual already resolves trophy.ASE. `3LEA`/`C3DLeaves`
  (`behavior_leaves.c`) and `3TAR`/`C3DShadow` (`behavior_shadow.c`) are zero-owned-method `C3DSpriteType`/
  `C3DPermanentSprite` decor billboards — non-solid inherited-gate leaves in the `vt_cone` mould. `3AIO`/`C3DAIOmtObj`
  (`behavior_ai_omtobj.c`) mirrors `behavior_omtobj.c` (OMT shape + `Radius` extents + `HasCollision==0`/`TerrainColl==0`
  solidity clearing); per its spec it deliberately skips `C3DAI::PostLoadAI`, so the placed crashpod/pod/friedeggs rows
  are static props with no runtime seek/patrol. **Deferred with documented reasons** (the two highest *raw-reach* rows
  are the riskiest): `3ROK`/`C3DRock` (99 instances all at `(0,0,0)` with `CanMove=1`/`RotateToDest` — a
  runtime-repositioned pool whose repositioning controller is not ported; drawing the origin pool would regress);
  `3YCA`/`C3DYokCargo` (per-frame visibility gate keyed on level=="LEV5" + `SCENE>489`, which needs the unported SCENE
  sequencer — and the cargo_ship mesh is already visible in 7/8 levels); `3SPR`/`C3DSprite` (rows carry zero serialized
  `SpriteSize`/`SpriteDatabase`/`SpriteIndex`, so the default canvas is an unresolved spec open question — drawing it
  would guess). Validation: per-class `make` (each commit builds), `JN_SCREENSHOT` on placing levels (`Level1` for
  `3LEA`+`3AIO`, `Level3C` for `3TAR`, `VR01`/`VR07` for `3TRO`), `audit_faithfulness.py` 0 findings (all 35 levels),
  `make web`, and `qa_web_verify.py` 16/16. The refreshed catalog now reports **93** used FourCCs, **65** with native
  vtables, and **28** still missing.
- **Still unimplemented:** the refreshed generated behavior lens reports **28** used-in-level FourCCs with no
  native behavior (**93** used FourCCs total, **65** used FourCCs with native vtables). The actor/gameplay focus
  section in `docs/asset_catalog/behavior_todo.md` is still **empty** — the remaining holes are base/resolver and
  effect/prop rows. The top of the queue is now the three **deferred** rows (`3ROK`/`C3DRock` origin pool,
  `3YCA`/`C3DYokCargo` SCENE-gated cargo, `3SPR`/`C3DSprite` no-canvas) — skip them unless new evidence appears
  (see the pass-3 bullet). The next *portable* rows are `3FIS`/`C3DDarwinFish` (darwin set-dressing creature),
  `3SWN` (doorfowl mesh), `3STA` (stalactite), `3ANI`/`C3DAnimatedSprite`, `3FOW`/`C3DFowl`, `3SPA` (powerline),
  `3GIR` (plant), `3LIG`/`C3DLight`, `3DAI`/`C3DAI`, then the lower-reach props/effects after them.
  Code-spawned or currently unplaced enemy specs (`3TAN`/
  `C3DTank`, `3HAR`/`C3DHarrier`, `3MIN`/`C3DMine`, `3MIS`/`C3DMissile`) still have zero current `.gam` rows; treat
  them as database-spawned/code-spawned/unplaced until separate evidence says otherwise. The big structural waves
  (N1 base framework, N2 enemies, N3 combat/pickups, N4 vehicles, N5 game-flow) are all landed.
- Implementation contract is `EntityVTable` in `src/engine/world.h`, registered by FourCC in
  `src/game/entities.c`; per-frame via `entity_update()` (`main.c:1418`); `.gam` params via
  `gam_prop_f/i`. Full contract in plan §1.

## Your task this session: continue the base/resolver + effect long tail
The big structural waves (N1–N5), the placed enemy/hazard slice (N2.x), the friend/NPC cast + AI mission
trigger (N2.y), the **actor/gameplay focus closeout (3PHO/3RCK/3HUM)**, and three base/effect tail passes
(1: `3NEU`/`3RED`/`3ARR`; 2: `3LIO`/`3OMT`/`3CON`; 3: `3TRO`/`3LEA`/`3TAR`/`3AIO`) are all landed. The
actor/gameplay focus section of the lens is **empty** — the remaining work is the base/resolver + effect/prop
long tail. Hold the decomp discipline — confirm behavior against
`docs/decomp/<Class>.md` (the decompiled body, not an offset scan), build on the existing modules, and **pick
targets from the lens**:

```bash
python3 tools/build_asset_catalog.py
sed -n '1,180p' docs/asset_catalog/behavior_todo.md
```

`behavior_todo.md` formalizes the strict query (`instances > 0 && native_behavior == null`), ranks by
`instances * level_count`, and splits out an actor/gameplay focus section (now empty). Use it to validate on
levels that actually place each class. The live catalog remains useful for previews:
<https://exentt.com/JN-assets/catalog/>.

**Three deferred rows sit at the top of the raw-reach queue — skip them unless new evidence appears:**
- `3ROK` / `C3DRock` — 99 inst / 1 level (Level5b), all serialized at `(0,0,0)` with `CanMove=1`/`RotateToDest`. A
  runtime-repositioned pool; no controller that scatters them is ported yet, so drawing the origin pool regresses.
  A spec exists (`C3DRock`, 1 owned method) — the blocker is *positioning*, not behavior.
- `3YCA` / `C3DYokCargo` — 11 inst / 8 levels. Per-frame visibility gate keyed on level=="LEV5" + `SCENE>489`; the
  SCENE sequencer isn't ported (SCENE stays at the CTaskList table value), and the cargo_ship mesh is already
  visible in 7/8 levels, so a thin port changes almost nothing. Revisit when a SCENE sequencer lands.
- `3SPR` / `C3DSprite` — 15 inst / 4 levels. Rows carry **zero** serialized `SpriteSize`/`SpriteDatabase`/
  `SpriteIndex`; the spec's own open question is the default canvas. Drawing it would guess (resolver gap, not a
  behavior gap; the faithful C3DSprite has no per-frame integrator).

**Next *portable* rows from the refreshed lens (read each spec first):**
- `3FIS` / `C3DDarwinFish` — 12 inst / 5 levels, darwin*.ASE set-dressing creature (HIWALK/HISHRINK/HISTOP anims).
  C3DAI-derived but background; an idle/wander leaf is faithful enough (lean on `behavior_ai`/`behavior_friend`).
- `3SWN` (doorfowl mesh), `3STA` (stalactite prop), `3ANI`/`C3DAnimatedSprite`, `3FOW`/`C3DFowl`, `3SPA`
  (powerline), `3GIR` (plant), `3LIG`/`C3DLight`, `3DAI`/`C3DAI`,
  then the lower-reach props/effects round out the tail. Confirm each FourCC→class via `docs/_gam_classids.tsv`
  (the lens "Class" column is blank for several) before coding.

NB on the just-closed focus rows: `3RCK`/`C3DRocket` turned out to be a **placed** C3DAI patrol rocket (9 `.gam`
rows), not the code-spawned projectile the prior handoff guessed — reading the spec first mattered. The
C3DPhoneBooth touch effect and the Humphrey `SCENE==0x5a` clone reveal are both wired-but-dormant / deferred (no
ground truth or no SCENE sequencer); see the closeout bullet under Current state.

**Friend / NPC wiring now available:** `vt_friend` (`behavior_friend.c`) is the shared C3DFriends/C3DAI idle base;
the talk-reward path is still deferred. The `.gam` loader now has a generic string-prop bag (`gam_str()`) for
unclaimed string fields, alongside the numeric `gam_prop_f/i`. Use the `CTaskList` entity-state table
(`game_flow_entity_state(tag)`) for mission-actor visibility/state gates, and lean on `behavior_ai.c` for
idle/patrol movement before adding per-character special cases.

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
Pick the next base/resolver or effect row(s) from `behavior_todo.md`, read each class spec before coding, and land
one commit per class on `native-port`. Validate each class with `make`, affected-level `JN_SCREENSHOT` checks, and
`tools/audit_faithfulness.py`; at wave end refresh `behavior_todo.md`, run `tools/qa_web_verify.py` (16 checks),
append `PROJECT_HISTORY.md`, and update this handoff. With N1–N5 plus the N2.x / N2.y slices, the actor-focus
closeout (`3PHO`/`3RCK`/`3HUM`), and the base/effect tail passes (1: `3NEU`/`3RED`/`3ARR`; 2: `3LIO`/`3OMT`/`3CON`;
3: `3TRO`/`3LEA`/`3TAR`/`3AIO`) landed, the campaign's base framework, enemies, combat, vehicles, game-flow, placed
ranged enemies/trigger hazards, the full friend/NPC cast, the escort actors, the AI mission-trigger volume, the
phone-booth prop, the placed patrol rocket, the hidden Humphrey clone-controller, neutron + red-neutron pickups,
nav-arrow sprite gates, light-data rows, authored OMT-shape props, cone/leaves/shadow sprite decor, the AI OMT prop,
and the VR trophy win-condition pickup are ported (**65/93** used FourCCs now have native vtables) — the
actor/gameplay focus section is empty and the remaining work is the base/resolver + effect long tail. The next
*portable* rows are `3FIS`, `3SWN`, `3STA`, `3ANI`, `3FOW`, `3SPA`, `3GIR`, `3LIG`, `3DAI`, plus lower-reach rows;
`3ROK`, `3YCA`, and `3SPR` are deferred with documented reasons (origin pool / unported SCENE gate / no serialized
canvas).
