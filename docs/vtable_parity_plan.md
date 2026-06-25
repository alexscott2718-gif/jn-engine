# Vtable Parity Plan

Written 2026-06-25 after the cutscene-camera decomp pass. This is the plan for
linking decomp vtables that matter to the game's core visual feel, gameplay feel,
menus, inventory/tool use, and campaign progression.

The goal is not "port every raw vtable." The goal is to identify the functions
that own visible runtime state and gameplay state, then link those first. A method
is important when it changes camera/view transforms, player input response, actor
pose, UI flow, inventory/tool state, trigger activation, progression state, AI
movement, or vehicle motion. Constructors, destructors, class registration, static
prop setup, and unused leaf classes should stay deferred unless they explain a
visible or playable mismatch.

## Current baseline

- Branch: `native-port`.
- Used-in-level behavior routing: 93 / 93 FourCCs have native vtables.
- Decomp ledger: 208 class rows, all currently spec-level.
- Web build: live at `https://exentt.com/jn-engine/`.
- Cutscene harness: first cut exists, with 114 selectable `3MCA` cutscenes, 136
  `3CAM` shot directors, and 362 authored audio steps.
- Vtable audit: `tools/build_vtable_parity_report.py` generates
  `docs/vtable_linkage_audit.md` from `docs/decomp_ledger.csv` plus curated
  overrides.
- Known open parity issues:
  - cutscene camera/actor fidelity is incomplete.
  - standalone `3CAM` now consumes `ViewFromCamera`, but the exact enum behavior
    is not proven against the original.
  - `FaceObject`, Jimmy `TargetActAnim`/`TargetDeactAnim`, and cutscene input
    lock are partially plumbed; non-player actor animation, `LoopActAnim`, and
    unlock/restore timing need deeper linkage.
  - Jimmy/Carl vehicle insertion and vehicle-specific poses/animations are a
    visual-fidelity dependency for vehicle parity.
  - Cindy location/pathing remains incomplete and deferred pending original-game
    evidence.
  - Goddard's texture is either incorrect or mapped incorrectly.
  - remaining audio reports need desktop/noVNC by-ear validation.

## The actual metric

Do not measure success by "percentage of all vtables ported." That number is noisy
because many vtables are registration-only, inherited shell classes, or unused.

Use this metric instead:

> What percentage of must-link parity systems are native-linked, faithfully ported,
> or still manually approximated?

Each decomp class/function should be classified as one of:

| Status | Meaning |
|---|---|
| `linked` | Native engine behavior directly follows the recovered decomp behavior. |
| `approximated` | Native engine has a manual behavior that works but is not proven faithful. |
| `must-link` | Function affects core visual/gameplay/menu/progression feel and should be decoded/ported. |
| `defer` | Low-visible, constructor-only, destructor-only, cosmetic, or not currently blocking parity. |
| `unused` | No current shipped `.gam` content reaches it. |
| `wontfix-faithful` | Native behavior differs from expectation but matches original-game evidence. |

## Parity domains

These are the top-level audit buckets. Menus are intentionally their own domain;
they are part of the control loop and should not be hidden under inventory.

| Domain | Why it matters |
|---|---|
| Camera / cutscene | Most visible scene parity: framing, focal point, motion, actor/audio timing. |
| Player movement | Tank controls, acceleration, stopping, camera coupling, climb/fence/fall states. |
| Menus / UI flow | Main menu, pause, HUD/menu transitions, selection navigation, input locks. |
| Inventory / items / gadgets | Pickup state, gadget selection, usable tools, tool animations, item-triggered actions. |
| Progression / objectives | Task gates, mission completion, level transitions, save/checkpoint/death/restart. |
| Triggers / story sequencing | `NextTrigger`, activation cascades, scripted control, sound/music/cutscene firing. |
| Animation / actor pose | Actor facing, talk/idle/action animations, texture page flips, animation-ended callbacks. |
| AI / pathing | Friend/NPC/enemy movement, patrol-point semantics, facing, attacks, state changes. |
| Vehicles / special movement | Car/jeep/rocket/boat/flying integrators, steering, wheel/camera/sound coupling. |
| Cosmetic / deferred | Static set dressing, low-impact visuals, unused helpers. |

## Must-link criteria

A vtable function should be promoted to `must-link` if it does any of the following:

- writes camera/view/player/actor transforms during runtime.
- consumes input or changes the player-control lock state.
- starts, stops, loops, or selects an animation that is visible in play or cutscenes.
- changes inventory, pickup, gadget, menu, task, checkpoint, save, or progression state.
- dispatches a trigger, cutscene, music, sound, level load, objective, or task update.
- updates AI pathing, facing, patrol routing, enemy attack state, or friend/NPC placement.
- integrates vehicle/flying movement or couples vehicle motion to camera/audio.

A vtable can usually stay deferred if it is only:

- scalar destruction.
- class id registration.
- inherited boilerplate with no class-owned behavior.
- static prop model/sprite setup already handled by asset resolution.
- an unused class with no current `.gam` rows.

## Priority order

### 1. Camera / cutscene runtime

This remains the first parity track because the harness now exposes failures quickly.

Primary classes and docs:

- `docs/decomp/C3DMultiCutSceneCamera.md`
- `docs/decomp/C3DCutSceneCamera.md`
- `docs/decomp/C3DCamera.md`
- `docs/decomp/CViewPort.md`
- `docs/decomp/CGameType.md`
- `src/game/behaviors/behavior_cutscene.c`

Known targets:

- Validate recovered `3MCA` `CameraTypeN` target-local offset table.
- Decode exact standalone `3CAM` `ViewFromCamera` / `CameraType` enum behavior.
- Validate actor facing and look-at semantics without scene-specific overrides.
- Link non-player `TargetActAnim`, `LoopActAnim`, and `TargetDeactAnim`.
- Validate `PlayerControlled` `NULL`/`none`/`JIM1` semantics and input
  unlock/restore timing.
- Keep camera step duration tied to sound duration where the original does so.

Validation:

- web cutscene selector.
- targeted Playwright checks for selected scene/title/audio handle.
- XP/noVNC or YouTube timestamp comparison by eye for representative scenes.

### 2. Player movement and feel

This owns the "does it feel like the original game" problem.

Primary docs:

- `docs/decomp/C3DPlayer.md`
- `docs/decomp/C3DJimmy.md`
- `docs/jimmy_animation_plan.md`
- `src/game/player.c`
- `src/engine/player_physics.c`

Must-link/deepen:

- `C3DPlayer::UpdatePlayerState` at `00437940`.
- `C3DPlayer::DispatchPlayerModeCamera` at `00438a60`.
- `C3DPlayer::UpdateWalkingCameraA` at `00438bc0`.
- `C3DPlayer::UpdateWalkingCameraB` at `00439900`.
- `C3DPlayer::UpdateSittingOrSmoothCamera` at `0043a120`.
- `C3DPlayer::UpdateRotateToTargetDelta` at `0043a420`.
- `C3DPlayer::ProjectNoisyCameraTarget` at `0043a5d0`.
- `C3DPlayer::StopPlayerMotion` at `0043a750`.
- `C3DPlayer::OnPlayerAnimEnded` at `0043a900`.
- `C3DJimmy` vehicle/action insertion state, including the pose split between
  on-foot movement and vehicle-specific animations.

Validation:

- capture/reference traces for start, stop, turn, run, jump/fall, climb/fence.
- do not regress the current 120 fps stability while matching original response.

### 3. Menus / UI flow

Menus are a first-class parity domain. They are not cosmetic because they own input
state, pause/selection flow, level starts, restart behavior, and HUD/gameplay
transitions.

Primary docs/code:

- `docs/decomp/CMainMenu.md`
- `docs/decomp/CMenuElement.md`
- `docs/decomp/C2DInGameMenu.md`
- `docs/decomp/CGameType.md`
- `src/game/menu.c`
- `src/game/game_flow.c`
- `src/game/hud.c`

Must-link/deepen:

- `CMainMenu` front-end routing: New Game and VR entries.
- `CMenuElement` rollover/activation dispatch.
- `C2DInGameMenu` in-game HUD/menu/death/restart semantics.
- pause/help state under `CGameType`.
- menu input locking/unlocking and transition behavior.
- menu and HUD audio cues.
- any menu selection state that affects level load, restart, or gadget/inventory
  selection.

Validation:

- local and web menu navigation.
- start game, VR selection, pause/resume, death/restart, game-over, campaign toggle.

### 4. Inventory / items / gadgets / tools

This is the next gameplay-control track after menus because tools and inventory are
the player's verbs.

Primary docs/code:

- `docs/decomp/CPickupType.md`
- `docs/decomp/C3DPickupType.md`
- `docs/decomp/C3DPickupItem.md`
- `docs/decomp/C3DJimmy.md`
- `docs/decomp/C3DShrinkRay.md`
- `docs/decomp/C3DGraplingHook.md`
- `docs/decomp/C3DToolChest.md`
- `docs/decomp/C3DBaseballPickup.md`
- `docs/decomp/C3DHelmet.md`
- `src/game/behaviors/behavior_pickup*.c`
- `src/game/inventory*`

Must-link/deepen:

- global pickup/state table behavior.
- inventory and picture/gadget flag updates.
- pickup visibility gates: `RequiredLevel`, `ExactLevel`, task state, pickup index.
- gadget selection and usable-object targeting.
- tool activation animation and cooldown/state lock.
- shrink-ray active transition.
- grappling hook, baseball, helmet, tool chest, and other gadget-specific actions.

Validation:

- pickup collect/recollect behavior.
- item state survives level transition/restart where appropriate.
- gadget animation and effect fire only when the original would allow it.

### 5. Progression / objectives

This is the "can the campaign actually be completed" track.

Primary docs/code:

- `docs/decomp/CGameType.md`
- `docs/decomp/CJimmyGame.md`
- `docs/decomp/CTaskList.md`
- `docs/decomp/CLoadLevel.md`
- `docs/decomp/C3DStartPoint.md`
- `docs/decomp/C3DCheckPoint.md`
- `src/game/game_flow.c`
- `src/game/task_loader.c`
- `src/game/behaviors/behavior_load.c`
- `src/game/behaviors/behavior_checkpoint.c`

Must-link/deepen:

- `.tsk` task table semantics.
- mission-complete checks.
- level-clear and level-load requests.
- start-point and checkpoint restore.
- lives/death/restart/game-over.
- task-gated object visibility and activation.
- campaign/VR routing differences.

Validation:

- campaign playthrough slices.
- scripted start/finish conditions per level.
- death/restart and checkpoint behavior.

### 6. Triggers / story sequencing

Triggers are the authored graph that ties cutscenes, audio, AI, progression, and
tools together.

Primary docs/code:

- `docs/decomp/CTrigger.md`
- `docs/decomp/C3DTriggerType.md`
- `docs/decomp/C3DAITrigger.md`
- `docs/decomp/C3DMusicTrigger.md`
- `docs/decomp/C3DSoundEffect.md`
- `src/game/behaviors/behavior_ai_trigger.c`
- `src/game/behaviors/behavior_cutscene.c`
- `src/game/behaviors/behavior_music.c`
- `src/game/behaviors/behavior_soundfx.c`

Must-link/deepen:

- raw `CTrigger` targets `00447400`, `00447450`, `004476c0`, `00447790`.
- `NextTrigger` cascade.
- `ActivateAnim`.
- `ActivateObject*`.
- `PlayerControlled`.
- sound/music trigger dispatch.
- trigger enter/exit latch semantics.

Validation:

- trigger graph traces for representative levels.
- cutscene/music/audio activation from authored triggers rather than manual launch.

### 7. Animation / actor pose

This track supports cutscenes, NPCs, enemies, and player feel.

Primary docs/code:

- `docs/decomp/C3DAnimated.md`
- `docs/decomp/C3DFriends.md`
- `docs/decomp/C3DJimmy.md`
- `docs/decomp/C3DGoddard.md`
- `docs/decomp/C3DCarl.md`
- `docs/decomp/C3DCindy.md`
- `docs/decomp/C3DYokianSpy.md`
- `src/game/animation*`
- `src/game/entity_visual.c`

Must-link/deepen:

- common animation setter/checker slots.
- animation-ended callbacks.
- actor facing for talk/cutscene states.
- talk, idle, wave, cheer, inhale, and deactivation animations.
- texture-page flips for talk/facial states.
- Goddard texture/mapping bug.

Validation:

- by-eye cutscene review.
- actor pose before/during/after trigger activation.
- Goddard material/UV comparison against original capture.

### 8. AI / pathing

Do this after trigger and animation linkage, because path state depends on both.

Primary docs/code:

- `docs/decomp/C3DAI.md`
- `docs/decomp/C3DPatrolPoint.md`
- `docs/decomp/C3DAITrigger.md`
- `docs/decomp/C3DFriends.md`
- `docs/decomp/C3DCindy.md`
- `docs/decomp/C3DYokian.md`
- `src/game/behaviors/behavior_ai*.c`

Must-link/deepen:

- `C3DAI` base state machine.
- `C3DPatrolPoint::OnArrive` at `00434ea0`.
- wait animation/sound behavior at patrol points.
- friend/NPC route selection.
- enemy facing, shield, attack, and hit reaction.
- Cindy location/pathing only after original/capture evidence confirms the expected
  state.

Validation:

- screenshots/traces against XP/capture evidence.
- AI state logs for path start, wait, next patrol, arrival, and trigger calls.

### 9. Vehicles / special movement

These are feel-critical but should follow player/camera unless they block a level.

Primary docs/code:

- `docs/decomp/C3DVehicle.md`
- `docs/decomp/C3DCar.md`
- `docs/decomp/C3DJeep.md`
- `docs/decomp/C3DRocketShip.md`
- `docs/decomp/C3DFlyingObject.md`
- `docs/decomp/C3DSailBoat.md`
- `src/game/behaviors/behavior_vehicle*.c`

Must-link/deepen:

- base vehicle/flying integrators.
- steering, acceleration, braking, and collision response.
- wheel/suspension visible state.
- vehicle camera override.
- engine/looping sound pitch and stop behavior.
- sailboat/boat behavior where level QA depends on it.

Validation:

- vehicle motion traces.
- camera and audio state during entry/drive/exit.

## First deliverables

1. `tools/build_vtable_parity_report.py`
   - input: `docs/decomp_ledger.csv`, `docs/decomp/*.md`, behavior mapping files,
     and asset catalog/usage data.
   - output: Markdown table grouped by parity domain and status.

2. `docs/vtable_linkage_audit.md`
   - generated or semi-generated report listing the current domain/status for each
     relevant class/function.
   - include a "Top 25 must-link functions" section.

3. Next implementation slice
   - prove exact `3CAM` `ViewFromCamera` / `CameraType` enum behavior.
   - link generic non-player target animation and `LoopActAnim` semantics.
   - validate `PlayerControlled` `NULL`/`none`/`JIM1` lock/unlock timing.
   - validate with `level1b` `LABEXP3` plus at least one Goddard and one Cindy scene.
   - keep Jimmy/Carl vehicle insertion poses on the vehicle track; do not guess
     Carl's vehicle offsets without decomp or original-game evidence.

## Required gates

Run these before committing behavior changes:

```bash
make
python3 tools/audit_faithfulness.py
source ~/emsdk/emsdk_env.sh
make web
python3 tools/qa_web_verify.py
```

For web publication:

```bash
./tools/deploy_wasm.sh
```

For docs-only/public-page updates, mirror the static page into `/var/www/jn-engine/qa/...`
and verify it is reachable from `https://exentt.com/jn-engine/qa/.../`.
