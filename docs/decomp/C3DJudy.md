# C3DJudy

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DJudy` |
| Base chain | `C3DFriends -> C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004a3e98`, `004a3ea8`, `004a42f8`, `004a4334`, `004a4348` |
| Ctor(s) | constructor/factory block `FUN_0042b2d0`; registers FourCC `3MOM` at `0042b392` |
| Dtor(s) | adjusted scalar deleting destructor at `0042b440`; cleanup/vtable reset helper `0042b470` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DJudy` is the concrete `3MOM` friend/NPC leaf for Judy. It inherits `C3DFriends` talk activation and `C3DAI` movement, registers Judy-specific animations and texture, and adds small raw hooks for reset, task-progress visibility, and Judy talk-progress rewards.

## Field Map

Offsets below are byte offsets from the outer `C3DJudy` allocation pointer used by the constructor and Judy leaf hooks, unless marked inherited. Inherited `C3DFriends`/`C3DAI` fields are documented on those base specs.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x430` | char buffer/string | `TaskName` | `CLocalGameObject`; `.gam` `3MOM`; shared slot `00419aa0` | Slot 264 refreshes the current task state for this inherited task-name string. |
| inherited `0x644` | float | `VisibleRange` | `C3DAI`; `.gam` `3MOM`; ctor `0042b2d0` | Current rows and constructor default use `500.0`. |
| inherited `0x648` | char buffer/string | `PatrolPoint` | `C3DAI`; `.gam` `3MOM` | Current rows use `"JUDY1A"` or `"none"`. |
| inherited `0x6ac` | char buffer/string | `TargetName` | `C3DAI`; `.gam` `3MOM` | Current rows target `"JIM1"`. |
| inherited `0x80c` | float | `FOV` | `C3DAI`; `.gam` `3MOM` | Current rows use `90` and `359`. |
| inherited `0x87c` | int | `AIState` | `C3DAI`; `.gam` `3MOM`; ctor `0042b2d0` | Serialized initial AI state. Current rows use `1` or `2`; constructor seeds a default `2`. |
| inherited `0x89c` | float | `WanderRange` | `C3DAI`; `.gam` `3MOM` | Current rows use `1500.0`. |
| inherited `0x8d4..0xa78` | talk table | `TalkState0..4`, `TalkTrigger0..4` | `C3DFriends`; `.gam` `3MOM` | Judy rows use up to three talk triggers; remaining slots are disabled with `-1`/`"none"`. |
| `0x57c` | handle/pointer | `judy_texture_canvas_handle` | `0042b4c0` | Passed to the inherited material/canvas slot after loading `judy.png`. Exact owner type is unresolved. |
| `0x635` | bool | `assets_registered` | ctor `0042b2d0`, `0042b4c0` | Cleared by the constructor; `RegisterJudyAssets` sets it after one-time mesh/texture setup. |
| `0x704` | float | `judy_visible_range_or_scale_default` | ctor `0042b2d0` | Constructor writes `500.0`, matching other friend leaves that carry a local range/default field. |
| `0x800` | int/flag | `judy_reset_state` | `0042b630` | Cleared after the Judy reset helper snapshots transform data and calls `C3DFriends::ResetFriendsRuntime`. |
| `this - 0x7c..this - 0x74` | vec3 cache | `reset_transform_cache` | `0042b630` | Stores components sampled from inherited transform slots before the reset hook. Pointer adjustment still needs struct cleanup. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `00415090` | `InitObjectSharedFriendVariant` | Shared friend leaf init currently owned by `C3DBenny` in the dump. Traces `InitObject()`, calls `C3DFriends::InitObject`, then calls an inherited adjusted slot at `0x108`. | inherited/shared |
| 10 | `0042b630` | `ResetJudyRuntimeTransform` | Raw helper. Samples inherited transform vectors through slots `0x164`, `0x314`, and `0x310`, writes a small transform cache, calls `C3DFriends::ResetFriendsRuntime`, logs `"ResetObj for"`, then clears `0x800`. | raw block |
| 241 | `0041b950` | `UpdateFriendsTalkMarker` | Inherited `C3DFriends` per-frame AI/talk marker update. | inherited |
| 259 | `00409480` | `PostLoadAI` | Direct inherited `C3DAI::PostLoadAI`. | inherited |
| 264 | `00419aa0` | `RefreshTaskStateShared` | Shared helper currently owned by `C3DArrow`. Calls a common no-op hook, reads the task state for this object's inherited task name, and writes the same value back. | inherited/shared |
| 265 | `0042b6f0` | `ApplyJudyProgressVisibility` | Raw helper. Runs `C3DAnimated` slot 265, reads `SCENE`, checks current level FourCC, and toggles an inherited visibility/enable slot for Judy-specific progress windows. | raw block |
| vtable 3 slot 2 | `0042b440` | scalar deleting destructor | Runs the Judy cleanup/vtable reset helper, destroys the adjusted streamer/string subobject, and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 67 | `0042b4c0` | `RegisterJudyAssets` | One-time asset registration. Binds Judy animation aliases to ASE files, loads `judy.png`, selects `STOP`, and applies two inherited shape/scale constants. | non-trivial |
| vtable 4 slot 90 | `0041bc20` | `SetFriendState3` | Inherited `C3DFriends` state helper. Judy does not add a custom slot-90 reward helper. | inherited |
| vtable 4 slot 96 | `0042b590` | `HandleJudyTalkProgressReward` | Calls `C3DFriends::StartFriendTalkPulse` when the friend-talk flag is active, then maps Judy scene states and adjusts counter id `0x12` on the last branch. | raw block |

## Per-Frame Behavior

Judy does not add a new movement integrator. Normal movement and talk-marker maintenance are inherited from `C3DFriends`/`C3DAI`; Judy's leaf behavior is reset bookkeeping, visibility gating, and a talk-progress hook.

```c
C3DJudy::ApplyJudyProgressVisibility(arg):
    C3DAnimated::slot_265(arg)
    scene_state = get_task_state("SCENE")
    level = current_game->level_fourcc

    if level == "LEV1":
        enabled = 0xc8 <= scene_state && scene_state < 0x104
    else if level == "LV4A":
        enabled = scene_state >= 0x1f4
    else:
        enabled = true

    inherited_enable_or_visibility_slot(enabled)
```

```c
C3DJudy::HandleJudyTalkProgressReward():
    if !friend_talk_active_flag:
        return

    C3DFriends::StartFriendTalkPulse()
    switch get_task_state("SCENE"):
    case 0xc8:
        set_task_state("SCENE", 0xcd)
        FUN_004038c0(1, 3, 1)
        break
    case 0xd2:
        set_task_state("SCENE", 0xdc)
        break
    case 0xe6:
        set_task_state("SCENE", 0xfa)
        FUN_004061c0(0x12, -1)
        if FUN_004061b0(0x12) <= 0:
            FUN_00403910(2, 0x12)
        break
```

## Constants And Wiring

### `.gam` Placeable Properties

`3MOM` appears 2 times across the level `.gam` files. It serializes common object/animated fields, inherited AI fields, and inherited friend talk fields. No Judy-only property is registered beyond the concrete class id.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"C3DJUDY"` | Base object tag and lookup identity. |
| `RotateToDest` | flag4 | inherited | `00010101`, `01010101` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `860704589` | FourCC/object id value for `3MOM`. |
| `PositionX` | float | inherited | `437..8160` | Base placement transform. |
| `PositionY` | float | inherited | `11.8..25.6` | Base placement transform. |
| `PositionZ` | float | inherited | `-1130..1380` | Base placement transform. |
| `RotationX` | float | inherited | `0` | Base placement transform. |
| `RotationY` | float | inherited | `140..233` | Base placement transform and initial facing. |
| `RotationZ` | float | inherited | `0` | Base placement transform. |
| `TaskName` | str | inherited `0x430` | `"Scene"`, `"scene"` | Matches Judy raw hooks that read the `SCENE` task state. |
| `Debug` | int | inherited | `0` | Base debug flag. |
| `RequiredLevel` | int | inherited | `0..200` | Inherited animated/progress lower gate. |
| `ExactLevel` | int | inherited | `-1` | Inherited animated/progress exact gate. |
| `RemoveLevel` | int | inherited | `-1..250` | Inherited animated/progress upper gate. |
| `HasCollision` | int | inherited | `1` | Inherited collision toggle. |
| `InitiallyVisible` | int | inherited | `-1` | Inherited initial visibility. |
| `CanMove` | int | inherited | `1` | Inherited transform/update gate. |
| `SecondPass` | int | inherited | `0` | Inherited render/update pass flag. |
| `PickupLink` | str | inherited | `"none"` | Inherited animated link field. |
| `PatrolPoint` | str | inherited `0x648` | `"JUDY1A"`, `"none"` | Resolved by inherited `C3DAI` patrol logic. |
| `VisibleRange` | float | inherited `0x644` | `500` | Compared by inherited `C3DAI` target/range logic. |
| `FOV` | float | inherited `0x80c` | `90`, `359` | Used by inherited AI facing/visibility helpers. |
| `TargetName` | str | inherited `0x6ac` | `"JIM1"` | Resolved by inherited `C3DAI::PostLoadAI`. |
| `AIState` | int | inherited `0x87c` | `1..2` | Copied into inherited runtime AI state. |
| `WanderRange` | float | inherited `0x89c` | `1500` | Used by inherited AI wander/search helpers. |
| `TalkState0` | int | inherited `0x8d4` | `0..200` | Gate for `TalkTrigger0` in `C3DFriends`. |
| `TalkTrigger0` | str | inherited `0x8e8` | `"none"`, `"requestkey"` | Resolved by inherited friend talk activation. |
| `TalkState1` | int | inherited `0x8d8` | `-1..210` | Optional second friend-talk gate. |
| `TalkTrigger1` | str | inherited `0x94c` | `"gotkey"`, `"none"` | Optional second friend-talk trigger. |
| `TalkState2` | int | inherited `0x8dc` | `-1..230` | Optional third friend-talk gate. |
| `TalkTrigger2` | str | inherited `0x9b0` | `"none"`, `"withwrench"` | Optional third friend-talk trigger. |
| `TalkState3..4` | int | inherited `0x8e0..0x8e4` | `-1` | Disabled remaining friend talk gates. |
| `TalkTrigger3..4` | str | inherited `0xa14..0xa78` | `"none"` | Disabled remaining friend talk triggers. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3MOM` | Concrete placeable class id for Judy. | ctor `0042b2d0`; `push 0x334d4f4d` at `0042b392` |
| `C3DJUDY` | Concrete object/type string. | string `.data:004ef7ec`; constructor string path |
| `C3DJUDY()` | Concrete class string. | string `.data:004ef7e0`; constructor string path |
| `SCENE` | Task-state key read and advanced by Judy raw hooks. | string `.data:004ed220`; raw `0042b590`, `0042b6f0` |
| `LEV1`, `LV4A` | Level FourCC gates for Judy visibility windows. | raw `0042b6f0` |
| `0xc8..0x103`, `>=0x1f4` | Visibility windows for `LEV1` and `LV4A`. | raw `0042b6f0` |
| `0xc8 -> 0xcd`, `0xd2 -> 0xdc`, `0xe6 -> 0xfa` | Talk-progress state transitions after Judy talk starts. | raw `0042b590` |
| `FUN_004038c0(1, 3, 1)` | Side effect on the `0xc8 -> 0xcd` talk-progress branch. | raw `0042b590` |
| counter id `0x12` | Consumed/decremented on the `0xe6 -> 0xfa` branch; when depleted, helper `FUN_00403910(2, 0x12)` runs. | raw `0042b590` |
| `100.0`, `0.75` | Shape/scale constants applied after asset registration through inherited adjusted slots. | `0042b4c0`; immediates `0x42c80000`, `0x3f400000` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| texture | `judy.png` | `0042b4c0`; string `.data:004ef7f4` | Loaded once when `assets_registered` is clear. |
| animation | `HIFIX` -> `judyfix.ase` | `0042b4c0` | Fix/repair animation. |
| animation | `HITALK` -> `judytalk.ase` | `0042b4c0` | Talk animation. |
| animation | `HIWALK` -> `judywalk.ase` | `0042b4c0` | Walk animation. |
| animation | `HISTOP` -> `judystop.ase` | `0042b4c0` | Default stop/idle animation. |
| animation default | `STOP` | `0042b4c0`; string `.data:004ed040` | Selected after texture/canvas setup. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, objdump over `/home/scotty/xp-jnbg-original/Neutron.exe`, and `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Create proper Ghidra functions for raw targets `0042b590`, `0042b630`, and `0042b6f0`; current Ghidra does not own those boundaries cleanly.
- Name the inherited slot at offset `0x110` used as Judy's task-window visibility/enable toggle.
- Identify the semantic owner of counter id `0x12` and the helper trio `FUN_004061b0`, `FUN_004061c0`, and `FUN_00403910`.
- Runtime-check Judy's `requestkey`/`gotkey`/`withwrench` path and visibility gates before marking the class `validated`.

## Notes

- Evidence: `DumpClass.java C3DJudy /tmp/decomp_C3DJudy.md` (`slots=394`, `owned_methods=1`, `offsets=0`), `DumpFunctions.java /tmp/decomp_C3DJudy_funcs.md`, and local `objdump` windows over `0042b2d0..0042b7f0`.
- Judy has much less leaf logic than Libby/Carl/Cindy. Most behavior comes from `C3DFriends` and `C3DAI`; the concrete class mainly supplies assets and scripted scene transitions.
