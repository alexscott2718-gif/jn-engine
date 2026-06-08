# C3DSheen

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DSheen` |
| Base chain | `C3DFriends -> C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004b3cfc`, `004b3d0c`, `004b415c`, `004b4198`, `004b41ac` |
| Ctor(s) | constructor/factory block `FUN_0043f920`; registers FourCC `3SHE` at `0043f9e2` |
| Dtor(s) | adjusted scalar deleting destructor at `0043fa90`; cleanup/vtable reset helper `0043fac0` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DSheen` is the concrete `3SHE` friend/NPC leaf for Sheen. It inherits the `C3DFriends` talk-trigger table and `C3DAI` state machine, adds Sheen-specific ASE/PNG asset registration, and has small raw hooks for level/task visibility and Sheen-specific talk progress rewards.

## Field Map

Offsets below are byte offsets from the outer `C3DSheen` allocation pointer used by the constructor and Sheen leaf hooks, unless marked inherited. Inherited `C3DFriends`/`C3DAI` property offsets are documented on those base specs; constructor stores for those fields often appear `0xc0` bytes higher than the primary inherited offsets.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x644` | float | `VisibleRange` | `C3DAI`; `.gam` `3SHE`; ctor `0043f920` | Sheen rows and constructor default use `500.0`, lower than the base AI default. |
| inherited `0x648` | char buffer/string | `PatrolPoint` | `C3DAI`; `.gam` `3SHE` | Current rows use `"none"`, `"shn1"`, or `"walks1"`. |
| inherited `0x6ac` | char buffer/string | `TargetName` | `C3DAI`; `.gam` `3SHE` | All current rows target `"JIM1"`. |
| inherited `0x80c` | float | `FOV` | `C3DAI`; `.gam` `3SHE` | Current rows range `90..359`. |
| inherited `0x87c` | int | `AIState` | `C3DAI`; `.gam` `3SHE`; ctor `0043f920` | Serialized initial AI state. Current rows use `1` or `2`; constructor seeds a default `2`. |
| inherited `0x89c` | float | `WanderRange` | `C3DAI`; `.gam` `3SHE` | Current rows use `1500.0`. |
| inherited `0x8d4..0xa78` | talk table | `TalkState0..4`, `TalkTrigger0..4` | `C3DFriends`; `.gam` `3SHE` | Sheen rows use only `TalkState0`/`TalkTrigger0`; remaining slots are disabled with `-1`/`"none"`. |
| `0x57c` | handle/pointer | `sheen_texture_canvas_handle` | `0043fb10` | Passed to the inherited material/canvas slot after loading `sheen.png`. Exact owner type is unresolved. |
| `0x635` | bool | `assets_registered` | ctor `0043f920`, `0043fb10` | Cleared by the constructor; `RegisterSheenAssets` sets it after one-time mesh/texture setup. |
| `0x800` | int/flag | `sheen_reset_state` | raw `0043fd10` | Cleared after the Sheen reset helper snapshots transform data and calls `C3DFriends::ResetFriendsRuntime`. |
| `this - 0x7c..this - 0x74` | vec3 cache | `reset_transform_cache` | raw `0043fd10` | Stores components sampled from inherited transform slots before the reset hook. Pointer adjustment still needs struct cleanup. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `00415090` | `InitObjectSharedFriendVariant` | Vtable target is currently owned by `C3DBenny` in the dump, but Sheen's constructor calls it after clearing `assets_registered`; treat as a shared friend leaf init/setup hook until sibling specs name it. | inherited/shared |
| 10 | `0043fd10` | `ResetSheenRuntimeTransform` | Raw helper. Samples inherited transform vectors through slots `0x164`, `0x314`, and `0x310`, writes a small transform cache, calls slot `0x2c`, calls `C3DFriends::ResetFriendsRuntime`, then clears `0x800`. | raw block |
| 241 | `0041b950` | `UpdateFriendsTalkMarker` | Inherited `C3DFriends` per-frame AI/talk marker update. | inherited |
| 259 | `0042d010` | `PostLoadAIThunk` | Thin jump to `C3DAI::PostLoadAI` at `00409480`. | inherited thunk |
| 265 | `0043fbf0` | `ApplySheenProgressVisibility` | Raw helper. Runs `C3DAnimated` slot 265, reads `SCENE` task state through `FUN_0045fea0`, checks the current level FourCC, and toggles an inherited visibility/enable slot based on Sheen's allowed progress window. | raw block |
| vtable 3 slot 2 | `0043fa90` | scalar deleting destructor | Runs the Sheen cleanup/vtable reset helper, destroys the adjusted streamer/string subobject, and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 67 | `0043fb10` | `RegisterSheenAssets` | One-time asset registration. Binds Sheen animation aliases to ASE files, loads `sheen.png`, sets the default `STOP` animation, and applies two inherited shape/scale constants. | non-trivial |
| vtable 4 slot 96 | `0043fca0` | `HandleSheenTalkProgressReward` | Calls `C3DFriends::StartFriendTalkPulse`, then maps exact `SCENE` states `0x118`, `0x136`, and `0x168` to follow-up states `0x122`, `0x140`, and `0x17c`; the `0x136` path also calls `FUN_004038c0(1, 4, 1)`. | raw block |

## Per-Frame Behavior

Sheen does not add a new per-frame movement integrator. Normal update is inherited from `C3DFriends` and `C3DAI`; Sheen's leaf behavior is limited to asset setup, post-load/task gating, reset cleanup, and a talk-progress hook.

```c
C3DSheen::ApplySheenProgressVisibility(arg):
    C3DAnimated::slot_265(arg)
    scene_state = get_task_state("SCENE")
    level = current_game->level_fourcc

    if level == "LEV1":
        enabled = 0x118 <= scene_state && scene_state < 0x12c
    else if level == "LEV2":
        enabled = 0x136 <= scene_state && scene_state < 0x14a
    else if level == "LEV4":
        enabled = 0x168 <= scene_state && scene_state < 0x186
    else:
        enabled = true

    inherited_enable_or_visibility_slot(enabled)
```

```c
C3DSheen::HandleSheenTalkProgressReward():
    C3DFriends::StartFriendTalkPulse()
    switch get_task_state("SCENE"):
    case 0x118:
        set_task_state("SCENE", 0x122)
        break
    case 0x136:
        FUN_004038c0(1, 4, 1)
        set_task_state("SCENE", 0x140)
        break
    case 0x168:
        set_task_state("SCENE", 0x17c)
        break
```

## Constants And Wiring

### `.gam` Placeable Properties

`3SHE` appears 4 times across the level `.gam` files. It serializes common object/animated fields, inherited AI fields, and inherited friend talk fields. No Sheen-only property is registered beyond the concrete class id.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"C3DSHEEN"`, `"Sheen1"`, `"Sheen2"`, `"sheen3"` | Base object tag and lookup identity. |
| `RotateToDest` | flag4 | inherited | `00010101`, `01010101` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `861096005` | FourCC/object id value for `3SHE`. |
| `PositionX` | float | inherited | `-139..8970` | Base placement transform. |
| `PositionY` | float | inherited | `2.54..89.7` | Base placement transform. |
| `PositionZ` | float | inherited | `-698..6990` | Base placement transform. |
| `RotationX` | float | inherited | `0..0.0413` | Base placement transform. |
| `RotationY` | float | inherited | `0..160` | Base placement transform and initial facing. |
| `RotationZ` | float | inherited | `0..0.411` | Base placement transform. |
| `TaskName` | str | inherited | `"Scene"`, `"scene"` | Matches Sheen raw hooks that read the `SCENE` task state. |
| `Debug` | int | inherited | `0` | Base debug flag. |
| `RequiredLevel` | int | inherited | `0..360` | Inherited animated/progress lower gate. |
| `ExactLevel` | int | inherited | `-1` | Inherited animated/progress exact gate. |
| `RemoveLevel` | int | inherited | `-1..380` | Inherited animated/progress upper gate. |
| `HasCollision` | int | inherited | `1` | Inherited collision toggle. |
| `InitiallyVisible` | int | inherited | `-1` | Inherited initial visibility. |
| `CanMove` | int | inherited | `1` | Inherited transform/update gate. |
| `SecondPass` | int | inherited | `0` | Inherited render/update pass flag. |
| `PickupLink` | str | inherited | `"none"` | Inherited animated link field; present on 3 rows. |
| `PatrolPoint` | str | inherited `0x648` | `"none"`, `"shn1"`, `"walks1"` | Resolved by inherited `C3DAI` patrol logic. |
| `VisibleRange` | float | inherited `0x644` | `500` | Compared by inherited `C3DAI` target/range logic. |
| `FOV` | float | inherited `0x80c` | `90..359` | Used by inherited AI facing/visibility helpers. |
| `TargetName` | str | inherited `0x6ac` | `"JIM1"` | Resolved by inherited `C3DAI::PostLoadAI`. |
| `AIState` | int | inherited `0x87c` | `1..2` | Copied into inherited runtime AI state. |
| `WanderRange` | float | inherited `0x89c` | `1500` | Used by inherited AI wander/search helpers. |
| `TalkState0` | int | inherited `0x8d4` | `0..360` | Gate for `TalkTrigger0` in `C3DFriends`. |
| `TalkTrigger0` | str | inherited `0x8e8` | `"exchange"`, `"givetickets"`, `"none"`, `"sewerpart"` | Resolved by inherited friend talk activation. |
| `TalkState1..4` | int | inherited `0x8d8..0x8e4` | `-1` | Disabled remaining friend talk gates. |
| `TalkTrigger1..4` | str | inherited `0x94c..0xa78` | `"none"` | Disabled remaining friend talk triggers. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3SHE` | Concrete placeable class id for Sheen. | ctor `0043f920`; `push 0x33534845` at `0043f9e2` |
| `C3DSHEEN` | Concrete class/object string. | string `.data:004f0a8c`; constructor string path |
| `SCENE` | Task-state key read and advanced by Sheen raw hooks. | strings `.data:004ed220`; raw `0043fbf0`, `0043fca0` |
| `LEV1`, `LEV2`, `LEV4` | Level FourCC gates for Sheen visibility windows. | raw `0043fbf0` compares current level id from `DAT_00509948 + 0x490` |
| `0x118..0x12b`, `0x136..0x149`, `0x168..0x185` | Visibility windows for `LEV1`, `LEV2`, and `LEV4`. | raw `0043fbf0` |
| `0x122`, `0x140`, `0x17c` | Follow-up task states set after Sheen talk starts at `0x118`, `0x136`, or `0x168`. | raw `0043fca0` |
| `150.0`, `0.5` | Shape/scale constants applied after asset registration through inherited adjusted slots. | `0043fb10`; immediates `0x43160000`, `0x3f000000` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| texture | `sheen.png` | `0043fb10`; string `.data:004f0a98` | Loaded once when `assets_registered` is clear. |
| animation | `HISTOP` -> `shenstop.ase` | `0043fb10` | Default stop/idle animation. |
| animation | `HITALK` -> `shentalk.ase` | `0043fb10` | Talk animation. |
| animation | `HIWALK` -> `shenwalk.ase` | `0043fb10` | Walk animation. |
| animation | `HIWAVE` -> `shenwave.ase` | `0043fb10` | Wave animation. |
| animation | `HIPICK` -> `shenpick.ase` | `0043fb10` | Pick/give animation. |
| animation default | `STOP` | `0043fb10`; string `.data:004ed040` | Selected after texture/canvas setup. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, objdump over `/home/scotty/xp-jnbg-original/Neutron.exe`, and `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Create proper Ghidra functions for raw targets `0043fbf0`, `0043fca0`, and `0043fd10`; `DumpFunctions` could not decompile them because they are not currently function symbols.
- Name the inherited slot at offset `0x110` used as Sheen's task-window visibility/enable toggle.
- Resolve why Sheen's slot 7 and slot 11 targets are currently owned by sibling classes (`C3DBenny` and `C3DGoddard`) in the dump.
- Apply structs deeply enough to separate outer allocation offsets, primary gameplay offsets, and adjusted vtable-4 offsets cleanly.
- Runtime-check Sheen talk triggers in the `0x118`, `0x136`, and `0x168` task states before marking the class `validated`.

## Notes

- Evidence: `DumpClass.java C3DSheen /tmp/decomp_C3DSheen.md` (`slots=394`, `owned_methods=1`, `offsets=0`), `DumpFunctions.java /tmp/decomp_C3DSheen_raw.md 0043fbf0 0043fca0 0043fd10 0042d010`, and local `objdump` for raw method boundaries.
- The only decompiled owned method is `0043fb10`; the other Sheen leaf behaviors are confirmed by `.rdata` vtable targets and contiguous disassembly but still need Ghidra function-boundary repair.
