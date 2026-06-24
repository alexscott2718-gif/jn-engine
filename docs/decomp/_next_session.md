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
  objects, not pickup→ability) and `3HOO` (an AI object → N2 track). The `3MEP` Goddard beacon is now live
  through the later C3DGoddard runtime slice.
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
- **Base/effect tail pass 4 DONE — 3LIG / 3FIS / 3GIR / 3SPA / 3STA (2026-06-23):** five more used FourCCs, each
  spec read first. `3LIG`/`C3DLight` (`behavior_light.c`, `vt_light`) is the OMediaLight scene-light data row —
  twelve authored light props, no visual/collision/per-frame body; lighting is measured OFF so it stays an inert
  gated data row with no invented side effect (sibling of `behavior_lightobj.c`/`3LIO`). `3FIS`/`C3DDarwinFish` +
  `3GIR`/`C3DGirlEatingPlant` share `vt_creature` (`behavior_creature.c`): both `C3DEnemy -> C3DPickupType -> C3DAI`
  set-dressing leaves own **no** per-frame method (only an asset registrar), so the port is the inherited C3DAI
  idle-or-patrol base (non-solid, InitiallyVisible/level gate) with no combat/pickup — the `vt_friend` idiom minus
  the look-at/talk plumbing. `3SPA`/`C3DSparkWire` is a `C3DTesla` derivative (ItemActive contact hazard) → routed
  to the existing `vt_tesla`, no new module. `3STA`/`C3DStalagtite` (`behavior_stalactite.c`, `vt_stalactite`) is a
  `C3DAnimated` terrain prop whose owned methods are a trigger-activated drop/relay (unported scripted-trigger
  dispatch), so the faithful minimal port is a static, non-solid, gated hanging prop. Deferred with reasons:
  `3FOW`/`C3DFowl` (SCENE-gated `vfunc_01_265` would hide a currently-visible mesh — `3YCA` precedent),
  `3ANI`/`C3DAnimatedSprite` (a real `Sprite1..9` frame animator needing sprite-resolver plumbing — next
  substantive target), `3DAI`/`C3DAI` (bare AI-base dummy at the origin). Validation: per-class `make` (each commit
  builds), `JN_SCREENSHOT` (`level1` for `3FIS`/`3GIR`, `level5a` for `3STA`, `level4b` for `3SPA`, `level4c` for
  `3LIG`), `audit_faithfulness.py` 0 findings (all 35 levels), `make web`, `qa_web_verify.py` 16/16. The refreshed
  catalog now reports **93** used FourCCs, **70** with native vtables, and **23** still missing.
- **Base/resolver tail — animated sprite + swing door DONE (2026-06-23):** the two named "portable" rows landed,
  each spec read first. `3ANI`/`C3DAnimatedSprite` (`behavior_animsprite.c`, `vt_animsprite`) cycles the authored
  `Sprite1..Sprite9` `sprites.omt` frames at `FPS` while `Activated`, advancing `e->sprite_index` so main.c's
  pre-existing 3ANI sprite-draw branch renders the live frame; it honors `Loop` (0=stop & hold, 1=loop, 2=loop +
  re-show) and the misspelled `InitallyVisible`, seeding runtime state from the authored `Activated`/`InitallyVisible`
  (the scripted toggle chain — the carnival `3BUT` buttons that `ActivateButton` the `bottles` rows — is deferred,
  3NEU's `NextTrigger` posture). **Loader fix:** `ENTITY_MAX_PROPS` 24→40 — a 3ANI row authors more numeric props
  than the bag held, so `prop_bag_add` silently dropped the *last* ones (`Sprite7..9`), truncating the bottle
  sequence to 6 frames; 40 clears the densest authored row (~30, a level4c 3MCA). `3SWN`/`C3DSwingDoor`
  (`behavior_swingdoor.c`, `vt_swingdoor`) is the timed yaw-swing door — an activation seeds a `TimeToOpen` countdown
  and swings the door by `OpenSpeed*dt` (a 90° quarter-turn), latching the opposite direction for the next activation
  (modeled as an explicit 4-state phase with a re-trigger cooldown since physics fires `on_trigger` every contact
  frame). It is **non-solid** like `vt_leveldoor` (the AABB can't rotate with the swing; the `TouchActivated=0`
  doors — mummydoor, the Level3D retro doors — have no ported opener), the documented divergence from the original's
  solid Reset; the mesh already drew via main.c's per-instance ASEFile/PNGFile door branch (which applies `e->ry`).
  Added a `JN_TEST_SWING` headless hook. Validation: per-class `make`; Level3C bottles play 177→180→177 and hold;
  Level1/level2a doors swing 0→90.8°/4.7→95.5°; Level1 degenerate 3ANI draws nothing; Level3 mummydoor stays closed;
  `audit_faithfulness.py` 0 findings (all 35 levels); `make web`; `qa_web_verify.py` 16/16; **public WASM deployed**.
  The refreshed catalog now reports **93** used FourCCs, **72** with native vtables, and **21** still missing.
- **Base/resolver + effect long-tail CLOSE-OUT — C3DAI creatures + the gated-prop family (2026-06-23):** the
  lower-reach tail (16 FourCCs) landed in two shapes, each spec read first. (1) The three remaining C3DAI
  "set-dressing creature" leaves — `3DIN`/`C3DDino`, `3CML`/`C3DCamel`, `3SPW`/`C3DSparrow` — route to the existing
  `vt_creature` (idle/patrol on the inherited C3DAI base; Sparrow's level-conditional vulture mesh already
  resolves). (2) The static prop/effect rows share one new `behavior_prop.c`/`vt_prop` — a gated static prop whose
  **solidity comes strictly from authored `HasCollision`** (1→solid `3HYD`/`3TOL`/`3CUB`, 0→non-solid
  `3SPH`/`3TEL`, unset→non-solid so the port never invents an obstacle): `3FLA`, `3HYD`, `3SCR`, `3TEL`, `3SPH`,
  `3CUB` (solid invisible block — procedural-primitive visual is a known resolver gap), `3TOL`, `3OCT`, `3MER`,
  `3TRA`, `3SM1`, `3FUE`, `3TRI`. Each of those owns *some* gameplay method (Octapuke pickup-counter teleport,
  MerryGo ride-attach, RocketFuel SCENE manip, Trigger activate-object cascade), all **deferred** because they need
  an unported subsystem (SCENE sequencer, scripted-trigger chain, unresolved player slots); the minimal port is the
  gate + authored collision over the already-resolved visual. **Record fix (game-owner ground truth):** the shrink
  ray shrinks certain AI (Dino/Darwin/Humphrey…) into small *moving pickups* (why they carry `C3DPickupType` +
  a `HISHRINK` frame) — active mechanic still deferred (transition not decompiled, `3SHR` unplaced), but the
  misleading `vt_creature` "no pickup logic" comment was corrected and the truth recorded on the
  `C3DShrinkRay`/`C3DDino`/`C3DDarwinFish`/`C3DGirlEatingPlant` specs + `behavior_humphrey.c`. Validation: `make`;
  `JN_SCREENSHOT` on all 12 placing levels (all render, no regression); `audit_faithfulness.py` 0 findings (all 35
  levels); `make web`; `qa_web_verify.py` 16/16. Catalog now **93** used FourCCs, **88** native vtables, **5**
  missing. **Public WASM not yet deployed this pass** (gated on explicit approval).
- **SCENE sequencer DONE — task-state story progression (2026-06-24):** the structural move after the portable tail
  exhausted. SCENE is a `CTaskList` task-state value with **no autonomous driver** — it advances only on story
  events. RE (`tools/ghidra/DumpFunctions.java`) recovered the get/set task helpers (`FUN_0045fea0`/`FUN_0045f990`)
  and `C3DAITrigger::ApplyAITriggerStoryProgress` (`FUN_0040caa0`) — a hardcoded `ObjectTag × current-SCENE →
  new-SCENE` patch table (~25 beats) run when the player trips a story trigger. Landed: a **mutable task store**
  (`task_set_entity_state`/`game_flow_set_entity_state`, faithful to `set_task_state` — writes existing tags, no
  append); the patch table in `behavior_ai_trigger.c` (real `ActivateAITrigger` order; reward/counter/menu side
  effects deferred); and the two freed visibility consumers — `3FOW`/`C3DFowl` (per-level SCENE windows) +
  `3YCA`/`C3DYokCargo` (LEV5 `SCENE>489`), both with the `C3DCindy` `SCENE<0 → show` guard so direct `--level`/audit
  launches (no CTaskList) are unchanged. `3HUM` clone reveal was already wired and now fires. (The **talk-reward
  half** — Carl/Cindy/Benny/… set SCENE at dialog gates — landed in the next bullet, 2026-06-24.) Validation (all
  against the *visible* result): `--newgame` + real `teleportexplanation` → `SCENE 0x1e→0x23`; real level4c
  `fowlinv` → `SCENE 0x1cc→0x1d6` closes the fowl window (visible→hidden); level5 cargo hides at `0x1e9` / shows at
  `0x1ea`; `SCENE=0x5a` reveals Humphrey + clones (4/4). `audit_faithfulness.py` 0 findings (all 35); `make web`;
  `qa_web_verify.py` 16/16. RE notes: `docs/decomp/_scene_sequencer.md`. Headless seams: `JN_TEST_SCENE=<tag>`,
  `JN_TEST_SET_SCENE=<int>`, `JN_TEST_VIS=<FourCC>`. **Public WASM not deployed** (gated on approval).
- **Talk-reward path DONE — the friend half of the SCENE writers (2026-06-24):** the OTHER SCENE writer. Each concrete
  friend leaf overrides `C3DFriends` vtable slot 96 (`StartFriendTalkPulse`) with a `Handle<X>TalkProgressReward` hook
  that re-reads SCENE and writes the next beat. Recovered from the nine per-character specs and collapsed into one shared
  FourCC × current-SCENE → new-SCENE table in `behavior_friend.c` (`friend_apply_talk_reward`, mirroring
  `aitrig_apply_story_progress`): Carl/Cindy/Benny/Libby/Nick/Judy/Sheen write SCENE (full table in
  `docs/decomp/_scene_sequencer.md` §1); Hugh (tail-jump only) + UltraLord (deferred inventory at `0x186`) write nothing.
  Driven from ONE centralized entrypoint — **T** talks the nearest in-range friend (`behavior_friend_talk_nearest`,
  faces the player) — so Cindy (`vt_cindy`) and Carl (`vt_walker`), which keep their own modules, are reached through the
  same table with no duplication. Writes via `game_flow_set_entity_state` (no-op without a CTaskList), and talk is
  key/headless-only (never auto), so direct `--level`/audit launches and the 2-tick probes are unaffected. HUD/menu/
  inventory/counter side effects deferred (same helpers as the AITrigger table). Validation (`JN_TEST_TALK=<tag>`): 7
  friends advanced their beats; the Nick LV2A conditional verified both ways (level2 `0x8c→0x91`, level2a unchanged); a
  wrong-beat talk no-ops; `--newgame` advanced Carl `0x32→0x3c`. `audit_faithfulness.py` 0/35; level1/level2 screenshots
  unchanged; `make web`; `qa_web_verify.py` 16/16. Catalog unchanged at **93/90/3** (enriched existing friend vtables, no
  new FourCC). **Public WASM not deployed** (gated on approval). Headless seam: `JN_TEST_TALK=<friendTag>`.
- **Still unimplemented:** the refreshed lens reports **3** used-in-level FourCCs with no native behavior (**93**
  total, **90** with native vtables) — `3ROK`/`C3DRock` origin pool, `3SPR`/`C3DSprite` no serialized canvas, and
  `3DAI`/`C3DAI` origin dummy. These are **resolver/positioning gaps, not SCENE-blocked** (the two SCENE-gated rows
  `3YCA`/`3FOW` are now ported). The actor/gameplay focus section of `behavior_todo.md` is **empty** and the full
  queue is deferred-only. Both SCENE writers (player-trigger + friend-talk) are now ported. A **C3DGoddard runtime
  slice** is also landed: code-spawned `3GOD`, disabled-level gates, follow/fetch modes, and the `3MEP` beacon. **The
  C3DGoddard scripted-control tail also landed (2026-06-24):** the `C3DAITrigger` C3DAI-target branch is wired for
  Goddard (`behavior_goddard_apply_ai_state`, called from `behavior_ai_trigger.c`) — the ~30 `3AIT` rows whose
  `AITarget == C3DGODDARD` (`SITGODDARD`/`GOGODDARD`/`GODDARDDIS`/`PUTGODDARD`/`movegoddard`/`rescuecat*`/…) now map
  their authored `AIState`/`AISpeed`/`AIPatrol` into Goddard's mode machine (AIPatrol→PATROL the chain, AIState 4→
  FOLLOW, else→HOLD), so a scripted teleport/sit/patrol is no longer instantly overridden by follow mode. Validation:
  `JN_TEST_SCENE=GOGODDARD --newgame` → Goddard patrolled `GODDARDPAT1`→`GODDARDPAT2`; `SITGODDARD`→HOLD at `gstart`;
  `PUTGODDARD` (Level2) → FOLLOW; `3MEP` fetch unregressed; `audit_faithfulness.py` 0 findings, `make web`,
  `qa_web_verify.py` 16/16. (`AIAnim` selection + the `flag()`/`menu()`/`counter()` reward side effects of these tags
  stay deferred to the HUD/menu subsystems.) Public WASM still live at the prior `jnengine.d75fb823.js` / assets
  `b4e7d620` (this pass not redeployed). The next substantive moves are (a) **remaining C3DGoddard tails** (the raw
  mode-vector/orbit/effect helper cluster — slots 95–99 — blocked on the unresolved static vectors
  `DAT_004f81b0..DAT_004f8208`; plus the non-SCENE `JIMEND`/`RECHARGE`/`BONUSSCREEN` energy/menu side effects that need
  HUD/menu); (b) **combat depth** (the general AITrigger `AIState`/`AISpeed` → C3DAI state machine for *enemies*;
  code-spawned `3MIN`/`3MIS`/`3TAN`/`3HAR`); (c) the **N5 tails** (text renderer / `PlayerControlled` input lock — the
  latter also surfaces the deferred HUD/menu side effects of both SCENE writers); (d) a
  **runtime-repositioning controller** (`3ROK` origin pool) / per-instance ASE resolver work (`3SPR`, `3TRA` visual);
  or (e) the deferred **active shrink mechanic**.
  Code-spawned / unplaced enemy specs (`3TAN`/`C3DTank`, `3HAR`/`C3DHarrier`, `3MIN`/`C3DMine`, `3MIS`/`C3DMissile`,
  `3POD`, `3SHR`) still have zero current `.gam` rows. The big structural waves (N1–N5) are all landed.
- Implementation contract is `EntityVTable` in `src/engine/world.h`, registered by FourCC in
  `src/game/entities.c`; per-frame via `entity_update()` (`main.c:1418`); `.gam` params via
  `gam_prop_f/i`. Full contract in plan §1.

## Your task this session: pick one of the remaining live moves
The portable behavior tail is exhausted (`90/93` native vtables; the 3 remaining are `3ROK`/`3SPR`/`3DAI`
resolver/positioning gaps — see the deferred list below, leave them). The structural waves (N1–N5), the
N2.x/N2.y slices, the actor-focus closeout, the four base/effect tail passes, the `3ANI`/`3SWN` pass, the
long-tail close-out, the **SCENE sequencer** (player-trigger half), and — 2026-06-24 — the **talk-reward path**
(the NPC half of the SCENE writers) are all landed. **Both SCENE writers are now ported**: `behavior_ai_trigger.c`
(`aitrig_apply_story_progress`) for player triggers, `behavior_friend.c` (`friend_apply_talk_reward`) for friend
talks. The first **C3DGoddard** runtime slice is also landed (`3GOD` companion + `3MEP` fetch beacon), but the
Goddard/energy AITrigger side effects are still deferred. See `docs/decomp/_scene_sequencer.md` and
`docs/decomp/C3DGoddard.md` for the full machine.

**Parallel initiative — Ground & World Collision Overhaul (scoped 2026-06-24):** a separate, decomp-faithful
mesh-collision campaign has its own self-contained kickoff at **`docs/decomp/_next_session_collision.md`** (replace the
floor-only `world_terrain_height` + safety-floor crutch with a `CollisionWorld` built from each level's `BLOCKING_*`
collider meshes — mesh ground-follow, wall blocking + slide + step-up, gated by `HasCollision`/`TerrainColl`, and retire
the procedural `ground.c` plane). Pick it up as its own track when prioritized.

**The live options (no forced default — pick by what unblocks the most, and confirm with the user first):**
- **(a) C3DGoddard tails / Goddard-energy side effects** — the AITrigger *scripted-control* half is now DONE (the
  `AITarget == C3DGODDARD` rows drive Goddard HOLD/PATROL/FOLLOW via `behavior_goddard_apply_ai_state`). What remains
  is (i) the raw mode-vector/orbit/effect helper cluster (slots 95–99: `SetGoddardModeVector`/
  `ApplyGoddardOffsetMotion`/`AdvanceGoddardModeState`) — **blocked** on the unresolved six static vectors
  `DAT_004f81b0..DAT_004f8208`, so don't guess them; and (ii) the non-SCENE energy/menu beats (`JIMEND`/`RECHARGE`/
  `BONUSSCREEN`, the `GOGODDARD` `flag(0,2)+menu(6)` reward) that need the HUD/menu/energy subsystems. Read
  `docs/decomp/C3DGoddard.md` (Goddard-script tail) first.
- **(b) Combat depth** — wire AITrigger `AIState`/`AISpeed` into the C3DAI state machine, and/or the code-spawned
  enemy specs that have zero `.gam` placement (`3MIN`/`3MIS`/`3TAN`/`3HAR` — they need a spawner/emitter).
- **(c) Text renderer** for the menu/HUD — today `menu.c` draws bars + logs labels and `C2DInGameMenu` prints
  numerals; a real glyph renderer would also let the deferred talk/AITrigger counter-popup side effects surface.
- **(d) `PlayerControlled` input lock** — the `PlayerControlled=="NULL" → force player STOP` path is visible in
  `ActivateAITrigger`; an N5 tail. Plumb the string prop onto the entity so cutscenes can lock player input.
- The 3 deferred resolver rows (`3ROK`/`3SPR`/`3DAI`) stay parked — assessed 2026-06-24, none is "free" (3DAI is
  authored-empty; 3SPR needs the unresolved default-canvas size; 3ROK needs an emitter/repositioning controller).
- The deferred **active shrink mechanic** (`C3DShrinkRay` → AI become moving pickups) is also still open.

**Deferred HUD/menu side effects (carry forward):** both SCENE writers keep ONLY the SCENE/task-state writes. The
inventory-grid (`FUN_004038c0`), counter-popup (`FUN_004061d0`/`004061b0`/`004061c0`), story-screen (`FUN_00406f90`),
and energy/Goddard helpers stay deferred until the HUD/menu subsystems land — option (a)/(c) above are the natural
unblockers. The friend talk-reward table (`friend_apply_talk_reward` in `behavior_friend.c`) documents each deferred
side effect inline at its gate, so re-enabling them is a localized follow-up.

Hold the decomp discipline — confirm behavior against `docs/decomp/<Class>.md` (the decompiled body, not an offset
scan), build on the existing modules. Re-derive the lens before starting:

```bash
python3 tools/build_asset_catalog.py
sed -n '1,180p' docs/asset_catalog/behavior_todo.md
```

`behavior_todo.md` formalizes the strict query (`instances > 0 && native_behavior == null`), ranks by
`instances * level_count`, and splits out an actor/gameplay focus section (now empty). The live catalog remains
useful for previews: <https://exentt.com/JN-assets/catalog/>.

**The 3 remaining deferred rows stay deferred until their blocker is ported — skip them otherwise:**
- `3ROK` / `C3DRock` — 99 inst / 1 level (Level5b), all serialized at `(0,0,0)` with `CanMove=1`/`RotateToDest`. A
  runtime-repositioned pool; no controller that scatters them is ported, so drawing the origin pool regresses.
- `3SPR` / `C3DSprite` — 15 inst / 4 levels. Rows carry **zero** serialized `SpriteSize`/`SpriteDatabase`/
  `SpriteIndex`; the default canvas is the spec's own open question. Resolver gap, not a behavior gap.
- `3DAI` / `C3DAI` — 4 inst / 2 levels, authored at `(0,0,0)` and invisible. A bare AI-base dummy; nothing to drive.

(`3YCA`/`C3DYokCargo` and `3FOW`/`C3DFowl` were on this list as "needs the SCENE sequencer" — both are now ported,
2026-06-24, because the SCENE sequencer landed. They use the `C3DCindy` `SCENE<0 → show` guard, so a direct
`--level`/audit launch with no CTaskList is unchanged; the gate only bites in campaign runs.)

NB on the closed long tail: `vt_prop` (`behavior_prop.c`) is the shared **gated static prop** leaf — visibility/
progress gate + **solidity strictly from authored `HasCollision`** (1→solid, 0→non-solid, unset→non-solid) over the
already-resolved visual; every routed class's owned gameplay method is deferred to an unported subsystem (SCENE
sequencer / scripted-trigger chain / unresolved player slots), documented per-class in the file header. The C3DAI
"set-dressing creatures" (`3FIS`/`3GIR`/`3DIN`/`3CML`/`3SPW`) share `vt_creature` — and they are **shrink-ray
targets** (shrink→moving pickup), recorded on their specs + `C3DShrinkRay.md` (the active mechanic is move #3).
`3SPA`/`C3DSparkWire` is literally `C3DTesla`-derived, so it reuses `vt_tesla` rather than a new file.

**Friend / NPC wiring (landed 2026-06-24):** `vt_friend` (`behavior_friend.c`) is the shared C3DFriends/C3DAI
idle base, and the friend talk-reward writer is now ported via `friend_apply_talk_reward` plus the **T** key /
`JN_TEST_TALK=<friendTag>` entrypoints. The `.gam` loader still preserves the generic string props (`gam_str()`,
including `TalkTrigger0..4` in `Entity.talk_trigger[]`) for future dialog/TalkTrigger expansion. The mutable task
store from the SCENE sequencer (`game_flow_set_entity_state` / `game_flow_entity_state(tag)`) is the shared
SCENE/task-state write/read path for both the AITrigger and friend writers.

**Known zero-placement enemy specs:** `3TAN`/`C3DTank`, `3HAR`/`C3DHarrier`, `3MIN`/`C3DMine`, and
`3MIS`/`C3DMissile` still have specs but zero current `.gam` placement. Keep them in the code-spawned /
database-spawned / unused bucket until separate evidence says otherwise.

**Two small N5 tails worth picking up:** (1) a real **text renderer** for the menu/HUD (today `menu.c` draws
bars + logs labels; `C2DInGameMenu` counters print numerals); (2) plumbing the `PlayerControlled` string prop
onto the entity so cutscenes can lock player input (deferred in N5).

**Done when:** after choosing a live move above, the implementation matches the relevant decomp spec/body, keeps
the direct `--level` / audit harness behavior unchanged unless the feature explicitly targets it, and is validated
with `make`, focused headless probes or screenshots for the affected levels, `tools/audit_faithfulness.py` at 0
findings, and (wave end) `tools/qa_web_verify.py` at 16/16. Refresh `behavior_todo.md` when the catalog lens
changes, append a one-line `PROJECT_HISTORY.md` entry, and update this handoff.

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
The current Goddard slice is complete when campaign level loads synthesize a `3GOD` companion only when the level data
references Goddard (or `JN_TEST_GODDARD` is set), the documented disabled levels keep it hidden, `3MEP` can request
mode 5 and collect/release back to mode 2, direct `--level` audit/screenshot launches remain unchanged,
`tools/audit_faithfulness.py` stays at 0 findings, `make web` builds, `tools/qa_web_verify.py` reports 16/16, and
`PROJECT_HISTORY.md` + this handoff describe the new state. That is the current 2026-06-24 baseline.

With N1–N5 plus the N2.x / N2.y slices, the actor-focus closeout (`3PHO`/`3RCK`/`3HUM`), the base/effect tail
passes (1: `3NEU`/`3RED`/`3ARR`; 2: `3LIO`/`3OMT`/`3CON`; 3: `3TRO`/`3LEA`/`3TAR`/`3AIO`; 4:
`3LIG`/`3FIS`/`3GIR`/`3SPA`/`3STA`), the animated-sprite + swing-door pass (`3ANI`/`3SWN`), the **SCENE sequencer**
(task-state store + `C3DAITrigger` story-progress patch table) with freed `3FOW`/`3YCA` gates, the friend
talk-reward writer, and the first `C3DGoddard` runtime slice are ported, **90/93** used FourCCs now have native
vtables. The actor/gameplay focus section is empty; the remaining substantive work is C3DGoddard tails, combat depth,
N5 tails (text renderer / `PlayerControlled` input lock), resolver/positioning (`3ROK`/`3SPR`/`3DAI`), or the active
shrink mechanic.
