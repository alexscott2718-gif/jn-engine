# C3DNick

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DNick` |
| Base chain | `C3DFriends -> C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004ab784`, `004ab794`, `004abbe4`, `004abc20`, `004abc34` |
| Ctor(s) | constructor/factory block `FUN_00433490`; registers FourCC `3NIC` at `00433555` |
| Dtor(s) | adjusted scalar deleting destructor at `00433670`; cleanup/vtable reset helper `004336a0` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DNick` is the concrete `3NIC` friend/NPC leaf for Nick. It inherits normal `C3DFriends` talk activation and `C3DAI` movement, but adds race-specific scene gates, skate/skateboard animation wiring, checkpoint side effects, and a constructed `C3DSkateBoard` child object.

## Field Map

Offsets below are byte offsets from the outer `C3DNick` allocation pointer used by the constructor and Nick leaf hooks, unless marked inherited. Inherited `C3DFriends`/`C3DAI` fields are documented on those base specs.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x430` | char buffer/string | `TaskName` | `CLocalGameObject`; `.gam` `3NIC`; `00433820` | Nick slot 264 refreshes this task state and then applies post-race hide thresholds for `LV2A`/`LV2B`. |
| inherited `0x644` | float | `VisibleRange` | `C3DAI`; `.gam` `3NIC` | Current rows use `550.0..1000.0`. |
| inherited `0x648` | char buffer/string | `PatrolPoint` | `C3DAI`; `.gam` `3NIC`; `00433cb0` | `.gam` rows use `"NICPAT1"` or `"none"`; slot 90 also copies `"NICPAT1"` into the patrol/default buffer. |
| inherited `0x6ac` | char buffer/string | `TargetName` | `C3DAI`; `.gam` `3NIC` | Current rows target `"JIM1"`. |
| inherited `0x80c` | float | `FOV` | `C3DAI`; `.gam` `3NIC` | Current rows use `90` and `359`. |
| inherited `0x87c` | int | `AIState` | `C3DAI`; `.gam` `3NIC`; ctor `00433490` | Current rows use `1..6`; constructor seeds a default `1`. Nick's skate update runs only when the inherited runtime state at `0x608` is `3`. |
| inherited `0x89c` | float | `WanderRange` | `C3DAI`; `.gam` `3NIC` | Current rows use `1500.0`. |
| inherited `0x8d4..0xa78` | talk table | `TalkState0..4`, `TalkTrigger0..4` | `C3DFriends`; `.gam` `3NIC` | Race dialog triggers drive the slot-96 scene transitions. |
| `0x57c` | handle/pointer | `nick_texture_canvas_handle` | `00433730` | Passed to the inherited material/canvas slot after loading `nick.png`. Exact owner type is unresolved. |
| `0x604` | float | `skate_anim_rate_or_speed` | `004339f0` | Updated from the skate phase and clamped to minimums of `200.0` or `400.0` before animation mode swaps. |
| `0x608` | int | `runtime_ai_state` | `004339f0` | Inherited/runtime state; when equal to `3`, Nick updates the skateboard child transform and skate animation mode. |
| `0x635` | bool | `assets_registered` | ctor `00433490`, `00433730` | Cleared by the constructor; `RegisterNickAssets` sets it after one-time mesh/texture setup. |
| `0x704` | float | `nick_visible_range_or_scale_default` | ctor `00433490` | Constructor writes `1000.0`, matching Nick's largest serialized `VisibleRange`. |
| `0x708` | char buffer/string | `nick_default_patrol` | `00433cb0` | Slot 90 copies `"NICPAT1"` here. This overlaps the inherited AI string area; exact struct split is unresolved. |
| `0x760` | char buffer/string | `nick_current_skate_anim` | `004339f0` | Compared against `"SKATE"`; toggled between `"GLIDE"` and `"SKATE"` on a five-second timer. |
| `0x820` | char buffer/string | `nick_forced_anim` | ctor `00433490`, `00433cb0` | Constructor copies an inherited/default string; slot 90 writes `"SKATE"` through an inherited string setter. |
| `0x8c0` | int/flag | `nick_race_runtime_flag` | `00433cb0` | Cleared by slot 90 before checkpoint setup. |
| `0x8cc` | byte | `nick_friend_marker_flag` | `004339f0` | Cleared while runtime AI state is `3`. Semantic owner is inherited friend/AI state. |
| `0xaec` | float | `skate_anim_timer` | `004339f0` | Accumulates frame delta; animation mode swap runs when it reaches `5.0`, then resets to `0`. |
| `0xaf0` | int | `skate_phase_index` | `004339f0` | Used to derive `skate_anim_rate_or_speed` before clamping. |
| `0xba0` | float | `nick_race_offset_or_timer` | `00433cb0` | Slot 90 adds `100.0`. Exact gameplay meaning is unresolved. |
| `0xba8` | adjusted pointer | `skateboard_child` | ctor `00433490`, `00433cb0`, `004339f0` | Constructor allocates a `C3DSkateBoard` object and stores its adjusted object pointer here. Slot 241 syncs transforms; slot 90 enables it for scene `0x8c`. |
| `0xbac` | int | `nick_race_counter_0` | ctor `00433490` | Cleared by the constructor. |
| `0xbb0` | int | `nick_race_counter_1` | ctor `00433490`, `00433b80` | Cleared by the constructor; incremented on talk-progress case `SCENE == 0x7d`. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `00415090` | `InitObjectSharedFriendVariant` | Shared friend leaf init currently owned by `C3DBenny`. Traces `InitObject()`, calls `C3DFriends::InitObject`, then calls an inherited adjusted slot at `0x108`. | inherited/shared |
| 10 | `0041b790` | `ResetFriendsRuntime` | Inherited `C3DFriends` reset helper. | inherited |
| 241 | `004339f0` | `UpdateNickTalkMarkerAndSkate` | Raw helper. Calls `C3DFriends::UpdateFriendsTalkMarker`, then, when Nick is in runtime AI state `3`, syncs the skateboard child transform and toggles skate animation mode on a timer. | raw block |
| 259 | `00409480` | `PostLoadAI` | Direct inherited `C3DAI::PostLoadAI`. | inherited |
| 264 | `00433820` | `RefreshNickTaskAndHidePastRace` | Refreshes the inherited task state like the shared slot 264, then hides Nick after `LV2A` scene `>= 0x8c` or `LV2B` scene `>= 0xa2`. | non-trivial |
| 265 | `004338a0` | `ApplyNickProgressVisibility` | Runs `C3DAnimated` slot 265, reads `SCENE`, and applies Nick-specific level/scene visibility windows. | non-trivial |
| vtable 3 slot 2 | `00433670` | scalar deleting destructor | Runs the Nick cleanup/vtable reset helper, destroys the adjusted streamer/string subobject, and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 67 | `00433730` | `RegisterNickAssets` | One-time asset registration. Binds Nick animation aliases to ASE files, loads `nick.png`, selects `STOP`, and applies two inherited shape/scale constants. | non-trivial |
| vtable 4 slot 90 | `00433cb0` | `PrepareNickRaceState` | Calls `C3DFriends::SetFriendState3`, forces skate/patrol strings, adjusts checkpoint objects, and enables the skateboard child for scene `0x8c`. | non-trivial |
| vtable 4 slot 95 | `0041bb20` | `C3DFriends` state helper | Direct inherited friend helper. | inherited |
| vtable 4 slot 96 | `00433b80` | `HandleNickTalkProgressReward` | Calls `C3DFriends::StartFriendTalkPulse`, then maps Nick race-talk scene states and triggers side effects/counters. | non-trivial |

## Per-Frame Behavior

Nick inherits normal AI movement and friend talk activation. His leaf behavior is concentrated in race visibility windows, race-talk state transitions, and skateboard/skate animation bookkeeping.

```c
C3DNick::ApplyNickProgressVisibility(arg):
    scene = get_task_state("SCENE")
    C3DAnimated::ApplyLevelGate(arg)

    switch current_game->level_fourcc:
    case "LEV1":
        visible = 0x5a <= scene && scene <= 0x6e
        break
    case "LV2A":
        visible = 0x73 <= scene && scene <= 0x8c
        break
    case "LEV2":
    case "LV2B":
        if scene == 0xa2:
            return  // leave the inherited C3DAnimated result alone
        visible = 0x8c <= scene && scene < 0xb4
        break
    case "LV4A":
        visible = scene >= 0x1f4
        break
    default:
        visible = true

    inherited_enable_or_visibility_slot(visible)
```

```c
C3DNick::RefreshNickTaskAndHidePastRace():
    refresh_task_state(TaskName)
    scene = get_task_state(TaskName)

    if current_level == "LV2A" && scene >= 0x8c:
        inherited_enable_or_visibility_slot(false)
    else if current_level == "LV2B" && scene >= 0xa2:
        inherited_enable_or_visibility_slot(false)
```

```c
C3DNick::UpdateNickTalkMarkerAndSkate(dt):
    C3DFriends::UpdateFriendsTalkMarker(dt)
    if runtime_ai_state != 3:
        return

    nick_friend_marker_flag = 0
    if skateboard_child:
        skateboard_child.copy_position_and_angle_from(this)

    skate_anim_timer += dt
    if skate_anim_timer < 5.0:
        return

    if nick_current_skate_anim == "SKATE":
        skate_anim_rate_or_speed = max((6 - skate_phase_index) * 100.0, 200.0)
        nick_current_skate_anim = "GLIDE"
    else:
        skate_anim_rate_or_speed = max((9 - skate_phase_index) * 100.0, 400.0)
        nick_current_skate_anim = "SKATE"

    skate_anim_timer = 0.0
```

```c
C3DNick::HandleNickTalkProgressReward():
    C3DFriends::StartFriendTalkPulse()
    switch get_task_state("SCENE"):
    case 0x73:
        set_task_state("SCENE", 0x78)
        break
    case 0x7d:
        set_task_state("SCENE", 0x78)
        nick_race_counter_1++
        break
    case 0x82:
        FUN_004038c0(0, 6, 1)
        set_task_state("SCENE", 0x8c)
        break
    case 0x8c:
    case 0x8d:
        if current_level != "LV2A":
            set_task_state("SCENE", 0x91)
        break
    case 0x96:
        set_task_state("SCENE", 0x91)
        break
    case 0xa0:
        FUN_004038c0(1, 6, 1)
        FUN_004038c0(2, 0x14, 1)
        FUN_004061d0(0x14, 0)
        set_task_state("SCENE", 0xa2)
        break
```

## Constants And Wiring

### `.gam` Placeable Properties

`3NIC` appears 4 times across the level `.gam` files. It serializes common object/animated fields, inherited AI fields, and inherited friend talk fields. The race-specific behavior is selected by `TaskName`, scene state, and talk triggers rather than by Nick-only serialized properties.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"C3DNICK"`, `"Nick1"`, `"Nick2"` | Base object tag and lookup identity. |
| `RotateToDest` | flag4 | inherited | `01010101`, `18010101` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `860768579` | FourCC/object id value for `3NIC`. |
| `PositionX` | float | inherited | `-1480..-184` | Base placement transform. |
| `PositionY` | float | inherited | `1.8..337` | Base placement transform. |
| `PositionZ` | float | inherited | `-9310..970` | Base placement transform. |
| `RotationX` | float | inherited | `0` | Base placement transform. |
| `RotationY` | float | inherited | `0..290` | Base placement transform and initial facing. |
| `RotationZ` | float | inherited | `0..0.00228` | Base placement transform. |
| `TaskName` | str | inherited `0x430` | `"SCENE"`, `"Scene"`, `"scene"` | Consumed by slot 264 refresh/hide logic; slot 265/96 directly read `SCENE`. |
| `Debug` | int | inherited | `0` | Base debug flag. |
| `RequiredLevel` | int | inherited | `0..140` | Inherited animated/progress lower gate. |
| `ExactLevel` | int | inherited | `-1` | Inherited animated/progress exact gate. |
| `RemoveLevel` | int | inherited | `-1..162` | Inherited animated/progress upper gate. |
| `HasCollision` | int | inherited | `1` | Inherited collision toggle. |
| `InitiallyVisible` | int | inherited | `-1` | Inherited initial visibility. |
| `CanMove` | int | inherited | `1` | Inherited transform/update gate. |
| `SecondPass` | int | inherited | `0` | Inherited render/update pass flag. |
| `PatrolPoint` | str | inherited `0x648` | `"NICPAT1"`, `"none"` | Resolved by inherited `C3DAI`; also forced by slot 90. |
| `VisibleRange` | float | inherited `0x644` | `550..1000` | Compared by inherited `C3DAI` target/range logic. |
| `FOV` | float | inherited `0x80c` | `90`, `359` | Used by inherited AI facing/visibility helpers. |
| `TargetName` | str | inherited `0x6ac` | `"JIM1"` | Resolved by inherited `C3DAI::PostLoadAI`. |
| `AIState` | int | inherited `0x87c` | `1..6` | Copied into inherited runtime AI state. |
| `WanderRange` | float | inherited `0x89c` | `1500` | Used by inherited AI wander/search helpers. |
| `TalkState0` | int | inherited `0x8d4` | `0..140` | Gate for `TalkTrigger0` in `C3DFriends`. |
| `TalkTrigger0` | str | inherited `0x8e8` | `"none"`, `"race1"`, `"race2"` | Race dialog trigger consumed by inherited friend activation. |
| `TalkState1` | int | inherited `0x8d8` | `-1..150` | Optional second friend-talk gate. |
| `TalkTrigger1` | str | inherited `0x94c` | `"loserace1"`, `"none"`, `"race2again"` | Optional second race dialog trigger. |
| `TalkState2` | int | inherited `0x8dc` | `-1..160` | Optional third friend-talk gate. |
| `TalkTrigger2` | str | inherited `0x9b0` | `"none"`, `"winrace1"`, `"winrace2"` | Optional third race dialog trigger. |
| `TalkState3` | int | inherited `0x8e0` | `-1..145` | Optional fourth friend-talk gate. |
| `TalkTrigger3` | str | inherited `0xa14` | `"loserace1"`, `"none"`, `"race2again"` | Optional fourth race dialog trigger. |
| `TalkState4` | int | inherited `0x8e4` | `-1..141` | Optional fifth friend-talk gate. |
| `TalkTrigger4` | str | inherited `0xa78` | `"dirtrace"`, `"none"`, `"race2"` | Optional fifth race dialog trigger. |
| `PickupLink` | str | inherited | `"none"` | Inherited animated link field; three Nick rows include this serialized property. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3NIC` | Concrete placeable class id for Nick. | ctor `00433490`; `push 0x334e4943` at `00433555` |
| `C3DNICK` | Concrete object/type string. | string `.data:004edd00`; constructor string path |
| `C3DNick()` | Concrete class string. | string `.data:004f0070`; constructor string path |
| `C3DSkateBoard` | Constructed child object used by Nick. | ctor allocates size `0x6c4` and calls `004402a0`; string `.data:004f0060` |
| `SCENE` | Task-state key used by Nick visibility and talk-progress hooks. | raw slots `004338a0`, `00433b80`, `00433cb0` |
| `LEV1`, `LEV2`, `LV2A`, `LV2B`, `LV4A` | Level FourCC gates for Nick visibility/race behavior. | raw slots `00433820`, `004338a0`, `00433b80` |
| `0x5a..0x6e`, `0x73..0x8c`, `0x8c..0xb3`, `>=0x1f4` | Visibility windows for `LEV1`, `LV2A`, `LEV2`/`LV2B`, and `LV4A`. | `004338a0` |
| `0x8c`, `0xa2` | Post-race hide thresholds for `LV2A` and `LV2B`. | `00433820` |
| `FINISHLINE`, `STARTLINE`, `C3DCHECKPOINT` | Checkpoint object names/type used by Nick slot 90. | `00433cb0`; strings `.data:004edca4`, `004edcc8`, `004edc94` |
| `SKATE`, `GLIDE`, `NICPAT1` | Nick's race/skate animation and patrol strings. | `004339f0`, `00433cb0`; strings `.data:004f0120`, `004f0118`, `004f0128` |
| `0x73 -> 0x78`, `0x7d -> 0x78`, `0x82 -> 0x8c`, `0x96 -> 0x91`, `0xa0 -> 0xa2` | Nick talk-progress state transitions. | `00433b80` |
| `FUN_004038c0(0, 6, 1)`, `FUN_004038c0(1, 6, 1)`, `FUN_004038c0(2, 0x14, 1)` | Side effects attached to Nick race talk milestones. | `00433b80` |
| counter id `0x14` | Reset/cleared when Nick advances `SCENE` to `0xa2`. | `00433b80`; `FUN_004061d0(0x14, 0)` |
| `150.0`, `0.5` | Shape/scale constants applied after asset registration through inherited adjusted slots. | `00433730`; immediates `0x43160000`, `0x3f000000` |
| `1000.0`, `160.0`, `300.0`, `1` | Constructor defaults for inherited/local Nick AI tuning fields. | ctor `00433490` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| texture | `nick.png` | `00433730`; string `.data:004f0090` | Loaded once when `assets_registered` is clear. |
| animation | `HITALK` -> `nicktalkboard.ase` | `00433730` | Talk animation while holding/riding board. |
| animation | `HIWALK` -> `nickwalk.ase` | `00433730` | Walk animation. |
| animation | `HISKATE` -> `nickskate1.ase` | `00433730` | Skate animation alias. |
| animation | `HIGLIDE` -> `nickskate2.ase` | `00433730` | Glide animation alias. |
| animation | `HISTOP` -> `nickcoin.ase` | `00433730` | Coin/idle variant registered as the stop alias. |
| animation | `HIWAIT` -> `nickstop.ase` | `00433730` | Wait/idle variant. |
| animation default | `STOP` | `00433730`; string `.data:004ed040` | Selected after texture/canvas setup. |
| child object | `C3DSkateBoard` | ctor `00433490` | Allocated by Nick and transformed in slot 241. Its concrete class has a separate spec. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, objdump over `/home/scotty/xp-jnbg-original/Neutron.exe`, asset scan, and `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Create a proper Ghidra function for raw target `004339f0`; current Ghidra does not own that slot boundary even though the vtable points at it.
- Name the inherited slot at offset `0x110` used as Nick's task-window visibility/enable toggle.
- Resolve Nick's race/skate fields at `0x604`, `0x760`, `0xaec`, `0xaf0`, and `0xba0` against the inherited AI animation struct.
- Identify the exact gameplay effect of `FUN_004038c0`, `FUN_004061d0`, and `FUN_00406f90(2)` in Nick's race branches.
- Runtime-check the `race1`/`race2`/`winrace*`/`loserace*` trigger sequence and checkpoint side effects before marking the class `validated`.

## Notes

- Evidence: `DumpClass.java C3DNick /tmp/decomp_C3DNick.md` (`slots=394`, `owned_methods=5`, `offsets=3`), `DumpFunctions.java /tmp/decomp_C3DNick_funcs.md`, and local `objdump` windows over `00433490..00433e30`.
- Nick is a race-controller friend leaf, not just a talk NPC. The `.gam` talk table supplies the trigger labels, while the leaf code maps scene numbers, checkpoint objects, and skateboard visibility/animation around those triggers.
