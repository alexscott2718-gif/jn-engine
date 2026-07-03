# C3DPlayer

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DPlayer` |
| Base chain | `C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004af6b4`, `004af6c4`, `004afb14`, `004afb50`, `004afb64` |
| Ctor(s) | TODO; string evidence includes `C3DPlayer()` at `.data:004f05a0` |
| Dtor(s) | adjusted scalar deleting destructor at `00437710`; cleanup helper `00437790` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DPlayer` is the shared controllable-player controller under `C3DJimmy`. It registers class id `3PLA` and provides walking/flying/sitting dispatch, camera-follow math, climb/fence special states, animation-state transitions, and level-load handoff helpers. It is not listed as a concrete `.gam` placeable in `docs/gam_schema.md`; the playable Jimmy instance is handled by derived `C3DJimmy` (`3JIM`).

## Field Map

Offsets below are byte offsets from the primary `C3DPlayer` pointer unless noted. Ghidra currently prints several references as `this[N].vftable` or as adjusted `this[-0x30]`/`this+0xc0`; the table keeps raw byte offsets visible so later struct work can refine names without losing evidence.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| `0x608` | float | `current_speed` | `00437890`, `00437940`, `00439900`, `0043a750` | Horizontal/player action speed accumulator; reset to zero by reset/stop helpers and fed to slot `0x26c` after update. |
| `0x614` | float | `lean_angle` | `00437890`, `00437940` | Cleared during reset and when walking state returns to neutral. |
| `0x618` | float | `previous_lean_angle` | `00437890` | Cleared with `lean_angle` on reset. |
| `0x61c` | float | `initial_or_ground_y` | `00437890` | Seeded from current world position Y during reset. |
| `0x664` | char/string | `StartPoint` | `00437830` | Registered property name `StartPoint` before class id `3PLA`; no `.gam` rows observed for `3PLA`. |
| `0x6bc` | float | `walk_accel_step` | `00439900` | Added into `0x6c8` on active movement paths before clamping to `0x6c4`. |
| `0x6c0` | float | `walk_decel_step` | `00438bc0`, `00439900` | Deceleration/return-to-zero step for `walk_speed`. |
| `0x6c4` | float | `walk_speed_cap` | `00438bc0`, `00439900` | Max clamp for `walk_speed`; also used to normalize camera/player bobbing. |
| `0x6c8` | float | `walk_speed` | `00438bc0`, `00439900`, `0043a900` | Local ground/action speed accumulator used by camera-follow, jump/fence/ladder entry, and movement helpers. |
| `0x6d4` | float | `turn_or_yaw_rate` | `00437c40`, `00439900`, `0043a120` | Turn delta/rate accumulator derived from input axes and clamped differently by helper state. |
| `0x6d8` | float | `scaled_yaw_delta` | `00437c40`, `00437f90` | Per-frame copy of `turn_or_yaw_rate` after multiplying by `dt`. |
| `0x6e4` | bool | `accelerating_or_action_move` | `00438bc0`, `00439900`, `0043a900` | Movement-state flag set on acceleration/action paths and cleared when special states end. |
| `0x6e5` | bool | `decelerating_or_returning` | `00439900` | Companion movement-state flag used to choose camera/transform offsets. |
| `0x6e6` | bool | `turn_input_latched` | `00439900`, `0043a120` | Latched when analog/key turn input is non-zero, cleared when direct axis input is zero. |
| `0x704` | int16 | `player_mode` | `00437940`, `00438a60`, `0043aa40` | Main player dispatch mode. `0` routes to walking A, `1`/`2` to flying/walking B style dispatch, `3` to sitting/special dispatch; negative values skip mode update. |
| `0x710` | float | `saved_vertical_target_y` | `00438bc0` | Stored when vertical/camera offset is first computed, reused if the transition remains active. |
| `0x714..0x71c` | vec3 | `camera_origin_before_update` | `00438bc0` | Snapshot of `DAT_00509a50+0x44..0x4c` before camera-follow deltas are applied. |
| `0x724` | char buffer | `load_level_arg0` | `0043b5a0` | Copied from `PlayerLoadLevel` argument strings. |
| `0x728` | int16 | `transient_substate` | `00437940` | Cleared when ground action state returns to neutral. |
| `0x774` | float/char buffer | `elapsed_or_load_level_arg1` | `00437940`, `0043aa40`, `0043b5a0` | As float, elapsed player timer; as buffer in load-level helper, copied string destination. |
| `0x778..0x784` | vec4 | `timed_state_restore_transform` | `00437940` | Transform restored when the timed special state expires. |
| `0x7a8` | float | `timed_special_state_remaining` | `00437940` | Counts down while `DAT_004f83e0` special animation/state is active. |
| `0x7c4` | int16 | `motion_submode` | `00437f90`, `00439900`, `0043a7f0` | Secondary ground/jump/flying submode used to branch turn clamps and camera probes. |
| `0x7c8` | float | `camera_bump_timer` | `00438a60`, `00437740` | When positive, lifts `DAT_00509a50+0x48` by `dt * 150` and counts down to zero. Default zero; also the camera local X (side) offset in the walking-camera projections (`evidence/walking_camera_record_write.md`). |
| `0x7cc` | float | `camera_forward_distance` | `00437740`, `00438bc0`, `00439900` | Default `200.0`; the walking-camera eye local Y (height) offset — projection axis convention (x side, y up, z forward); `evidence/walking_camera_record_write.md`. Field name predates the axis fix. |
| `0x7d0` | float | `camera_vertical_offset` | `00437740`, `00438bc0`, `00439900` | Default `-350.0`; the walking-camera eye local Z (forward) offset — 350 behind the player. Field name predates the axis fix. |
| `0x7d8` | float | `camera_side_offset` | `00437740`, `00438bc0`, `0043a120` | Default `0.0`; passed to transform projection helpers. |
| `0x7dc` | float | `camera_height_offset` | `00437740`, `00438bc0`, `0043a120` | Default `80.0`; the walking-camera look-point local Y (head height). |
| `0x7e0` | float | `camera_extra_offset` | `00437740`, `00438bc0`, `0043a120` | Default `0.0`; passed to transform projection helpers. |
| `0x7e8` | int16 | `jump_or_motion_phase` | `00437f90` | Multi-state jump/fall/action phase; transitions among `0..5`, switches animations, and triggers `FALL`/`JUMP`. |
| `0x7ec` | float | `turn_probe_offset_a` | `00437f90`, `00439900` | Helper-specific offset, commonly `-40.0` or `-100.0`, used in ground/camera probes. |
| `0x7f0` | float | `turn_probe_offset_b` | `00437f90`, `00439900` | Helper-specific offset, commonly `10.0`, used in alternate probe branch. |
| `0x7f8` | pointer | `linked_motion_object` | `00437f90`, `00438bc0` | Optional linked object whose `0x6c0` field is driven during jump/fall/action transitions. |
| `0x7fc` | bool | `load_level_pending_flag` | `0043b5a0` | Cleared when `PlayerLoadLevel` copies its arguments. |
| `0x804..0x813` | vec4 | `cached_camera_probe` | `00438bc0` | Stores a transform/probe result reused by later camera offset code. |
| `0x828` | bool | `vertical_action_active` | `00438bc0` | Selects vertical adjustment path and later clears when not held. |
| `0x82a` | int16 | `camera_side_state` | `00438bc0` | Side/edge state chosen from analog input and timers. |
| `0x830` | float | `camera_side_timer` | `00438bc0` | Timer for side/edge camera transition. |
| `0x834` | float | `special_state_timer2` | `0043aff0` | Cleared when entering ladder/fence special states. |
| `0x838..0x844` | vec4 | `special_state_restore_transform` | `0043a900`, `0043aff0` | Saved transform restored after `FENCE`/`LADDER` animation ends. |
| `0x848` | float | `input_pitch_accumulator` | `00438bc0` | Incremented from input sample returned by `FUN_0046a460`. |
| `0x84c` | float | `camera_or_body_pitch` | `00438bc0` | Driven by analog/global input and used to compute camera target offsets. |
| `0x850` | bool | `edge_or_vertical_lock` | `00438bc0` | Suppresses some vertical transition paths. |
| `0x851` | bool | `smooth_camera_mode` | `00438bc0` | Chooses high-smoothing path for camera deltas. |
| `0x854` | float | `camera_x_scale` | `00437740`, `00438bc0` | Default `1.6`; scales camera delta when not in vertical action mode. |
| `0x858` | float | `camera_y_scale` | `00437740`, `00438bc0` | Default `1.0`; scales camera delta when not in vertical action mode. |
| `0x85c` | float | `camera_z_scale` | `00437740`, `00438bc0` | Default `1.6`; scales camera delta when not in vertical action mode. |
| `0x868` | float | `special_state_grace` | `0043aff0` | Set to `5.0` when entering ladder/fence special states. |
| `0x86c` | float | `jump_gate_timer` | `00437f90` | Branches jump/fall camera adjustment when below a threshold. |
| `0x870` | float | `turn_speed_bias` | `00437c40` | Added into ground velocity/camera movement calculation. |
| `0x874` | float | `jump_phase_timer` | `00437f90` | Counts jump/fall phase duration and drives phase transitions. |
| `0x878` | float | `left_camera_timer` | `00438bc0` | Timer paired with `camera_side_state == -1`. |
| `0x87c` | float | `right_camera_timer` | `00438bc0` | Timer paired with `camera_side_state == 1`. |
| `0x884` | float | `linked_motion_scale` | `00437f90`, `00438bc0` | Divisor/scale for linked object speed and percent trace. |
| `0x888` | float | `special_anim_duration` | `0043a900`, `0043aff0` | Cleared when special states end; seeded to `4.0` for `FENCE`, `6.0` for `LADDER`. |
| `0x88c` | char buffer | `load_level_arg2` | `0043b5a0` | Copied from `PlayerLoadLevel` argument strings. |
| `0x8f0` | char buffer | `load_level_arg3` | `0043b5a0` | Copied from `PlayerLoadLevel` argument strings and logged. |
| `0x8f8` | float | `action_anim_timer` | `00438a60` | Cleared with `action_anim_latched` when an animation starts/continues. |
| `0x8fc` | bool | `action_anim_latched` | `00437940`, `00438a60` | Prevents repeated default animation selection until an active animation clears it. |
| `0x955` | char buffer | `load_level_arg4` | `0043b5a0` | Copied argument buffer for load-level transition. |
| `0x9a5` | bool | `has_saved_load_position` | `0043b5a0` | If false, helper snapshots current transform into `0x9a8`. |
| `0x9a8..0x9b7` | vec4 | `saved_load_position` | `0043b5a0` | Current transform saved before load-level transition work. |
| `0x9b8` | float | `scratch_anim_elapsed` | `00437c40`, `00438bc0` | Timer for scratch/buttons/play animation selection and clearing. |
| `0x9bc` | char buffer | `scratch_anim_name` | `00437c40`, `0043aff0` | Stores transient animation names such as `SCRATCH`, `PLAY`, `BUTTONS`. |
| `0x9d0` | subobject | `class_streamer_subobject` | `00437710` | Embedded `OMediaClassStreamer` destroyed by scalar deleting destructor. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `00437830` | `InitObjectPlayer` | Traces `C3DPlayer::InitObject()`, runs `C3DAnimated::InitObject`, registers `StartPoint` at `0x664`, registers class id `3PLA`, then calls inherited setup/reset slots. | non-trivial |
| 10 | `00437890` | `ResetPlayerRuntime` | Runs inherited local reset, zeros `current_speed`, `lean_angle`, and `previous_lean_angle`, calls inherited transform reset, and seeds `initial_or_ground_y` from current position Y. | non-trivial |
| 221 | `0043aa40` | `HandlePlayerCollisionSurface` | After the elapsed timer exceeds `3.0`, reads material/collision ids, dispatches id ranges `0x78..0x81` and `0x6e..0x77` to inherited handlers, and uses ray tests to enter `LADDER` (`0xb4`) or `FENCE` (`0x3c..0x45`) special states. | non-trivial |
| 241 | `00437940` | `UpdatePlayerState` | Main update gate. Calls `C3DAnimated::UpdateAnimated`, handles timed special-state expiry, checks the global active-player pointer, selects idle/left/right/shoot/walk animations, dispatches walking/flying submodes, and forwards `current_speed` to slot `0x26c`. | non-trivial |
| 243 | `00438a60` | `DispatchPlayerModeCamera` | Clears action-animation latch, processes `camera_bump_timer`, and dispatches `player_mode` to slots `0x124`, `0x128`, or `0x12c` with trace strings `Walking A`, `Flying`, `Walking B`, and `Sitting`. | non-trivial |
| 246 | `0043a420` | `UpdateRotateToTargetDelta` | If update/input gates are true, compares current and target transform fields at `0x224..0x27c`, wraps angular deltas, scales them by `dt`, and calls inherited slot `0x334`. | non-trivial |
| 260 | `0043a750` | `StopPlayerMotion` | Runs a no-op/base hook, zeros `current_speed`, zeroes inherited velocity with slot `0x2c4`, and calls primary slot `0x148` to stop/reset current animation. | non-trivial |
| 261 | `0043a790` | `TouchOrCollisionFallback` | If the touched object is not class/tag `4ecc44`, delegates to inherited touch handler; otherwise clears velocity for non-active-player instances. | non-trivial |
| vtable 4 slot 65 | `0043a900` | `OnPlayerAnimEnded` | Handles `AnimEnded`. For `FENCE`/`LADDER`, returns to `STOP`, clears `DAT_004f83e0`, restores saved transform, clears movement flags, and re-enables inherited state. For `SPLAT`/`HIT`, returns to `STOP`. | non-trivial |
| vtable 4 slot 72 | `0043a5d0` | `ProjectNoisyCameraTarget` | Builds a transform/position from current orientation plus trig offsets and writes a projected vec4 result to the caller. Fully decoded 2026-07-02: transform_local (00472980) with pitch hard-indexed 0 and yaw + `0x6d4` turn lead; `evidence/walking_camera_record_write.md`. Fully decoded 2026-07-02: transform_local (00472980) with pitch hard-indexed 0 and yaw + `0x6d4` turn lead; `evidence/walking_camera_record_write.md`. | non-trivial |
| vtable 4 slot 73 | `00438bc0` | `UpdateWalkingCameraA` | Large camera/movement helper. Samples input, manages scratch/buttons/play animation timers, updates `walk_speed`, vertical/edge state, probe transforms, and `DAT_00509a50` position/angles. Record write fully decoded; `evidence/walking_camera_record_write.md`. | non-trivial |
| vtable 4 slot 74 | `00439900` | `UpdateWalkingCameraB` | Companion movement/camera helper with turn input latching, acceleration/deceleration of `walk_speed`, turn clamps, and camera target/probe updates. Record write fully decoded, ported natively (camera_record_walkcam_write), and certified (`C3DPlayer`/walking-camera-record); `evidence/walking_camera_record_write.md`. | non-trivial |
| vtable 4 slot 75 | `0043a120` | `UpdateSittingOrSmoothCamera` | Companion camera helper that applies turn input, uses configured camera offsets, probes target transforms, and smooths `DAT_00509a50+0x44..0x4c`/angles toward the result. | non-trivial |
| vtable 4 slot 76 | `00437c40` | `UpdateGroundMoveA` | Ground movement helper. Applies turn delta, computes forward velocity from `walk_speed`, writes inherited velocity, updates camera/position interpolation, and resets scratch animation timer when no active player pointer exists. | non-trivial |
| vtable 4 slot 77 | `00437f90` | `UpdateJumpFallMove` | Jump/fall state machine. Seeds probe offsets, applies turn/velocity, transitions `jump_or_motion_phase`, selects `JUMP`/`FALL`/walking animations, and adjusts linked object motion when present. | non-trivial |
| vtable 4 slot 83 | `0043a7f0` | `GroundAheadPredicate` | Returns true when `motion_submode == 0`, a short forward/down ray probe hits through `FUN_0047c210`, and a random threshold passes. | non-trivial |
| vtable 4 slot 84 | `0043aff0` | `SetPlayerAnimationState` | Sanitizes requested animation names, skips duplicate/blocked states, handles transitions among `STOP`, walking, `EDGE`, `JUMP`, `LEFT`, `RIGHT`, `FENCE`, `LADDER`, `SPLAT`, and `HIT`, and starts special-state transforms/timers for fence/ladder. | non-trivial |
| vtable 4 slot 87 | `0042a7a0` | `HasPlayerInput` | Returns false while `DAT_004f83e0` is active; otherwise true if any of `DAT_00509834`, `DAT_00509845`, `DAT_00509849`, or `DAT_00509ac4` is set. | non-trivial |
| vtable 4 slot 90 | `0043b5a0` | `PlayerLoadLevel` | Copies four level-transition strings into player buffers, marks a global/current object flag, snapshots player position if needed, calls global game/menu hooks, and logs `Calling PlayerLoadLevel`. | non-trivial |
| vtable 4 slot 91 | `0043b820` | `ProbePlayerRayBlend` | Casts between caller vectors and current player position. On hit, writes a blended collision point back to the caller and returns a constant float; otherwise returns the original scalar. Decoded: the walking-camera collision — eye pulled 75% toward the ray hit, returns 1.5 on hit / the vec w (1.0) free; `evidence/walking_camera_record_write.md`. | non-trivial |
| vtable 4 slot 92 | `00437740` | `SetPlayerDefaultConstants` | Seeds camera/action constants: `0x7cc=200.0`, `0x7c8=0`, `0x7d8=0`, `0x7e0=0`, `0x7d0=-350.0`, `0x7dc=80.0`, `0x854=1.6`, `0x858=1.0`, `0x85c=1.6`. | non-trivial |
| vtable 3 slot 2 | `00437710` | scalar deleting destructor | Runs cleanup helper `00437790`, destroys the embedded `OMediaClassStreamer` at adjusted offset `0x9d0`, and frees adjusted allocation when the delete flag is set. | non-trivial |

## Per-Frame Behavior

```c
C3DPlayer::UpdatePlayerState(dt):
    if !engine_allows_update():
        return

    elapsed_timer += dt
    C3DAnimated::UpdateAnimated(dt)

    if timed_special_state_remaining > 0:
        timed_special_state_remaining -= dt
        if timed_special_state_remaining <= 0 and global_special_state:
            set_animation(STOP, restart=false)
            global_special_state = false
            camera_bump_timer = 0
            restore_transform(timed_state_restore_transform)
            inherited_enable_or_visible(true)
            timed_special_state_remaining = 0

    if DAT_005099e4 != this:
        return
    if DAT_004f83e0 or DAT_004f8181:
        return

    switch player_mode:
    case 2:
        call mode helper slot 0x13c
        break
    case 0, 1:
        if animation not locked and inherited movement test passes:
            if current_speed <= 10:
                choose STOP, LEFT, RIGHT, or SHOOT from transient_substate/action flags
            else:
                set walking/run animation
            transient_substate = 0
            player_mode = 0
            lean_angle = 0

        if player_mode == 1:
            call mode helper slot 0x134
        else:
            if current_speed < 10 and current animation is walking/run:
                set_animation(STOP)
            call mode helper slot 0x130
        break
    default:
        break

    update_speed_dependent_state(current_speed) // slot 0x26c
```

Mode/camera dispatch:

```c
C3DPlayer::DispatchPlayerModeCamera(dt):
    if active_player is null:
        return

    if (animation_is_active() or animation_is_starting()) and action_anim_latched:
        action_anim_latched = false
        action_anim_timer = 0

    if camera_bump_timer > 0:
        sample_input()
        DAT_00509a50.position.y += dt * 150.0
        camera_bump_timer = max(camera_bump_timer - dt, 0)
        return

    switch player_mode:
    case 0:
        trace("Walking A")
        call slot 0x124(dt)
        trace("Walking B")
        break
    case 1:
        trace("Flying")
        call slot 0x128(dt)
        break
    case 2:
        call slot 0x128(dt)
        break
    case 3:
        trace("Sitting")
        call slot 0x12c(dt)
        break
```

Fence/ladder collision entry:

```c
C3DPlayer::HandlePlayerCollisionSurface(hit):
    if elapsed_timer <= 3.0:
        return

    material_id = hit ? hit.material_id : -1
    if material_id in 0x78..0x81:
        inherited_surface_handler_160(hit)
    if material_id in 0x6e..0x77:
        inherited_surface_handler_164()

    if player_mode != 0:
        return
    if material_id not in 0x3c..0x45 and material_id != 0xb4:
        return

    if material_id == 0xb4 and ray_probe_ladder_hits():
        if current_animation != "LADDER":
            yaw = angles_from_probe_vector()
            set_rotation_y(yaw)
            set_transform(probe_offset_50_or_25)
            set_animation("LADDER")
        return

    if ray_probe_fence_hits():
        yaw = angles_from_probe_vector()
        set_rotation_y(yaw)
        set_transform(probe_offset)
        set_animation("FENCE")
```

## Constants And Wiring

`C3DPlayer` registers class id `3PLA` via raw immediate `0x33504c41` at `00437871` (`ALP3` little-endian in the class-id scan). No `3PLA` instance table appears in `docs/gam_schema.md`, so the only class-local registered property currently confirmed is:

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `StartPoint` | string (`1`) | `0x664` | no `3PLA` `.gam` rows observed | Registered by `InitObjectPlayer`; likely used by derived player/level spawn logic, but no owned-method read is confirmed in this class dump. |

Animation/state strings consumed by owned methods:

| String | Address | Consuming Logic |
|---|---:|---|
| `STOP` | `004ed040` | Default/idle recovery animation in update and special-state cleanup. |
| `RIGHT` | `004f05dc` | Ground animation selection for positive side substate. |
| `LEFT` | `004f05e4` | Ground animation selection for negative side substate. |
| `JUMP` | `004f05ec` | Jump/fall helper and animation transition helper. |
| `FALL` | `004ef3e8` | Jump/fall state transitions. |
| `SHOOT` | `004ef448` | Selected when an active animation test succeeds and global mode is not `4`. |
| `BACK` | `004f0648` | Animation helper transition state. |
| `BUTTONS` | `004f0650` | Scratch/buttons/play random animation helper. |
| `PLAY` | `004f0658` | Scratch/buttons/play random animation helper. |
| `SCRATCH` | `004f0660` | Scratch/buttons/play random animation helper. |
| `LADDER` | `004f0668` | Collision special-state entry and animation-ended cleanup. |
| `FENCE` | `004f0670` | Collision special-state entry and animation-ended cleanup. |
| `EDGE` | `004f0684` | Animation transition helper. |
| `SPLAT` | `004ef6e0` | Animation-ended cleanup and transition guard. |
| `HIT` | `004ef2e4` | Animation-ended cleanup and transition guard. |

Notable globals:

| Global | Meaning |
|---|---|
| `DAT_005099e4` | Active player/update target pointer; many helpers return early when it does not match this object. |
| `DAT_004f83e0` | Special animation/state lock; blocks input and normal animation changes until cleared. |
| `DAT_004f8181` | Additional global player lock checked by the main update. |
| `DAT_00509a50` | Camera/player target record; position at `+0x44..0x4c`, angles at `+0x50..0x52`. |
| `DAT_00509834`, `DAT_00509845`, `DAT_00509849`, `DAT_00509ac4` | Input globals used by `HasPlayerInput`. |
| `DAT_0050985d..DAT_00509860`, `DAT_00509aa0`, `DAT_00509aa4` | Input/axis globals used by walking/camera helpers. Exact control names are still open. |

## Assets

`C3DPlayer` itself does not load a fixed mesh, texture, or OMedia database asset. It consumes animation/state names supplied by the inherited `C3DAnimated` loader and by derived classes.

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| class id | `3PLA` | `00437830` / `docs/_gam_classids.tsv` | Player base class id; not observed as concrete `.gam` instances. |
| registered property | `StartPoint` | `00437830` | Spawn/start string registered at offset `0x664`. |
| animation names | see Constants table | `.data` strings and transition helper `0043aff0` | These are state names, not files. |

## Confidence

Confidence: Medium

Validation: Static Ghidra function recovery + local disassembly only; not runtime-validated.

Open questions:
- ~~Create Ghidra functions for raw helper entry points `00437890`, `00437c40`, `00437f90`, `00438bc0`, `00439900`, `0043a120`, `0043a420`, `0043a5d0`, `0043a790`, `0043a7f0`, `0043aff0`, `0043b5a0`, and `0043b820` so the next dump can produce decompiler bodies instead of `(function not found)`.~~ **DONE 2026-07-02**: see `docs/decomp/evidence/c3dplayer_movement_target3.md`; remaining work is signature/slot naming, not function-boundary creation.
- Resolve inherited transform slots (`0x120`, `0x124`, `0x128`, `0x12c`, `0x130`, `0x134`, `0x138`, `0x13c`, `0x140`, `0x144`, `0x148`, `0x150`, `0x154`, `0x158`, `0x15c`, `0x16c`, `0x218`, `0x248`, `0x264`, `0x270`, `0x278`, `0x2bc`, `0x2c4`, `0x310`, `0x314`, `0x328`, `0x330`, `0x334`, `0x338`, `0x384`, `0x38c`, `0x410`) against named OMedia/CGameObject methods.
- Name the input globals and map them to keyboard/controller actions.
- Confirm whether `StartPoint` is consumed in `C3DJimmy`/level-load code rather than in `C3DPlayer` itself.
- Runtime-check ladder/fence/splat transitions against XP capture or a controlled debug trace before marking validated.

## Notes

- Evidence: `DumpClass.java C3DPlayer /tmp/decomp_C3DPlayer.md` (`slots=389`, `owned_methods=9`, `offsets=3`).
- Extra evidence: `objdump -D -Mintel` over `/home/scotty/xp-jnbg-original/Neutron.exe` for `00437c40..00438c00`, `00438bc0..0043a880`, and `0043a880..0043b980`.
- `DumpFunctions.java /tmp/decomp_C3DPlayer_raw.md ...` reports the raw helper addresses as `(function not found)` because those entry points are not yet function-defined in Ghidra, even though the vtable and disassembly show normal code bodies.
- The prior campaign invariant that `FUN_0041a140` is the flying/player-family integrator remains documented in `C3DFlyingObject`; `C3DPlayer` directly overrides the player update/camera/action controller and derives from `C3DAnimated`, not directly from `C3DFlyingObject`.

## Target 3 Recovered Movement L1

Target 3 from `docs/ghidra_recovery_plan.md` is now complete at L1. The
function boundaries were created in `~/ghidra-projects/JN_decomp` and dumped
to `docs/decomp/evidence/c3dplayer_movement_target3.md`.

Signature caveat: the helper pass created function bodies but did not finish
all stack-argument prototypes. In particular, `UpdateGroundMoveA_00437c40` and
`UpdateJumpFallMove_00437f90` still show the per-frame `dt` as
`unaff_retaddr`/temporary values in raw Ghidra output. Treat this as recovered
control-flow and offset evidence, not final C signatures.

Recovered body map:

```c
ResetPlayerRuntime:
    inherited_reset()
    current_speed = 0
    lean_angle = previous_lean_angle = 0
    initial_or_ground_y = transform_slot_0x328().y

UpdateGroundMoveA(dt):
    if !global_special_lock:
        if current_motion_vector_nonzero() or turn_speed_bias != 0:
            scaled_yaw_delta = turn_or_yaw_rate
            rotate_self(turn_or_yaw_rate * dt)        // slot 0x334
            project walk_speed/turn_speed_bias through engine trig table
    write inherited velocity through slot 0x2c4
    if no active-player pointer:
        clear scratch_anim_name and scratch_anim_timer
    if grounded/probe slots succeed:
        normalize probe vector, compute angles, write adjusted velocity

UpdateJumpFallMove(dt):
    turn_probe_offset_a = -40.0
    turn_probe_offset_b = 10.0
    project turn/walk velocity as in ground move
    branch on jump_or_motion_phase
    if linked_motion_object burn time expires in phase 3/4:
        jump_or_motion_phase = 0
        motion_submode = 1
        set FALL
        zero linked burn
    otherwise transition among phases 0..5, JUMP/FALL, linked-object scale,
    and ground/contact tests

UpdateWalkingCameraA(dt):
    process side/input globals and special lock state
    randomize idle action animation among SCRATCH/BUTTONS/PLAY
    accelerate/decelerate walk_speed with walk_accel_step/walk_decel_step
    snapshot camera_origin_before_update
    update camera probe fields and DAT_00509a50 position/angles

UpdateWalkingCameraB(dt):
    turn_or_yaw_rate += input_turn * dt or keyboard_turn * dt * 100
    forward input accelerates walk_speed toward walk_speed_cap
    no-input high-speed tail subtracts walk_decel_step * 0.5 above cap * 0.90909094
    reverse/brake subtracts walk_decel_step or walk_decel_step * 0.5
    clamp turn_or_yaw_rate to +/-50 in motion_submode 2, otherwise +/-30
    update camera target and DAT_00509a50 position/angles

UpdateSittingOrSmoothCamera(dt):
    use the same turn-input ramp
    smooth DAT_00509a50 toward an offset target with 1.2 position scaling

UpdateRotateToTargetDelta(dt):
    compare current/target transform fields at 0x224..0x27c
    wrap angular deltas to [-180,180]
    call rotation slot 0x334 with multipliers 4.5, 3.7, 4.5
```

Additional recovered helpers:

- `ProjectNoisyCameraTarget_0043a5d0` projects a local offset through current
  transform/trig-table state and writes a vec4 result with `w=1.0`.
- `TouchOrCollisionFallback_0043a790` delegates non-`C3DGODDARD` touches to
  inherited touch handling; Goddard contact zeros velocity for non-active
  players.
- `GroundAheadPredicate_0043a7f0` runs only when `motion_submode == 0`, probes
  from a transformed point down by `2000.0`, and requires the hit/random
  threshold to exceed `0x46`.
- `SetPlayerAnimationState_0043aff0` now has a recovered transition body for
  `STOP`, `WALK`, `EDGE`, `JUMP`, `SWING`, `BACK`, `FALL`, `LEFT`, `RIGHT`,
  `HIT`, `FENCE`, and `LADDER`. Fence/ladder store restore transforms at
  `0x838..0x844`, zero velocity, set `DAT_004f83e0`, set durations `4.0` and
  `6.0`, set grace `5.0`, and play sound ids `0x92`/`0x8f`.
- `PlayerLoadLevel_0043b5a0` copies transition strings into the player buffers,
  sets `DAT_00509980+0x1fe`, snapshots current position to `0x9a8..0x9b4` when
  needed, calls global game slot `0x100`, and stops the current animation.
- `ProbePlayerRayBlend_0043b820` raycasts between the caller vector and player
  position, writes a 75-percent blended hit point on success, and returns
  `1.5`.

## Native Linkage (linked-parity branch)

Aspect: **`free-roam-feel`** — status `linked-blocked` (note updated
2026-07-02 after target 3 Ghidra recovery).

The earlier L1 blocker is retired for the target 3 helper set: Ghidra now has
function-defined bodies for `UpdateGroundMoveA` `00437c40`,
`UpdateJumpFallMove` `00437f90`, `UpdateWalkingCameraA` `00438bc0`,
`UpdateWalkingCameraB` `00439900`, `SetPlayerAnimationState` `0043aff0`, and
the doc's remaining raw helper list. The recovered bodies pin the real
accumulate/clamp walk-speed machine, turn-rate clamps, jump/fall phase state,
camera smoothing, scratch/action animation randomization, fence/ladder
special-state setup, load-level handoff, and ray/probe helpers. Remaining L1
cleanup is naming/signature polish for inherited slots and input globals, not
missing function boundaries.

The aspect is still not certifiable because **L2 fails by design**.
`src/game/behaviors/behavior_player.c` implements the approved simple
tank-turn movement (instant velocity; its own comment: the data-driven
accel/decel physics is DEFERRED after producing ice-skating / wrong
turn+speed). The dormant data-driven ramp (`src/engine/movement_base.c`,
`movement_base_flying_step`) is a tuned approximation with constants that
trace to no decomp address (`0.909f` decel window, `decel * 0.5f`,
`dt * 6.0f` lean smoothing, `+/-45` lean clamp) — it is the L4
"ice-skating" cautionary example, not a transcription. There is no fidelity
claim to certify: the same disposition shape as `C3DCheckPoint`/progress and
`C3DPickupItem`/collection.

An input-trace oracle wrapped around the tank-turn code would compare the
native design against itself — the circular-oracle failure the linkage gate
exists to prevent. What linking player movement-logic actually requires now:

1. A product/native-port decision to replace the approved tank-turn movement
   with a 1:1 port of the recovered walk-speed/jump/camera state machine.
2. Signature and inherited-slot naming cleanup as needed to make that port
   maintainable.
3. Only then a headless input-trace oracle plus mutation test.

### Not covered / open

- Free-roam FEEL confirmation (by-eye / capture-with-input) is untouched by
  the above and remains native-port territory regardless.
