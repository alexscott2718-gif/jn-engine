# C3DCarl

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DCarl` |
| Base chain | `C3DFriends -> C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `00495354`, `00495364`, `004957b4`, `004957f0`, `00495804` |
| Ctor(s) | constructor/factory block `FUN_00413af0`; registers FourCC `3CAR` at `00413bb3` |
| Dtor(s) | adjusted scalar deleting destructor at `00413c70`; cleanup/vtable reset helper `00413ca0` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DCarl` is the concrete `3CAR` friend/NPC leaf for Carl. It inherits `C3DFriends` talk activation and `C3DAI` movement, registers Carl-specific animations and texture, and adds task-state hooks for Carl visibility, Carl rescue/escape placement, and Carl talk-progress rewards.

## Field Map

Offsets below are byte offsets from the outer `C3DCarl` allocation pointer used by the constructor and Carl leaf hooks, unless marked inherited. Inherited `C3DFriends`/`C3DAI` fields are documented on those base specs.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x430` | char buffer/string | `TaskName` | `CLocalGameObject`; `.gam` `3CAR`; raw `00413eb0` | Carl slot 264 reads this task name, fetches its task state, and writes the same state back through `FUN_0045f990`. |
| inherited `0x644` | float | `VisibleRange` | `C3DAI`; `.gam` `3CAR`; ctor `00413af0` | Current rows use `500..750`; constructor default stores `500.0`. |
| inherited `0x648` | char buffer/string | `PatrolPoint` | `C3DAI`; `.gam` `3CAR` | Current rows use tags such as `"CARL1"`, `"carl1"`, `"crl1"`, and `"look1"`. |
| inherited `0x6ac` | char buffer/string | `TargetName` | `C3DAI`; `.gam` `3CAR` | Current rows target `"JIM1"` or `"Jim1"`. |
| inherited `0x80c` | float | `FOV` | `C3DAI`; `.gam` `3CAR` | Current rows range `90..359`. |
| inherited `0x87c` | int | `AIState` | `C3DAI`; `.gam` `3CAR`; ctor `00413af0` | Serialized initial AI state. Current rows range `1..9`; constructor seeds a default `1`. |
| inherited `0x89c` | float | `WanderRange` | `C3DAI`; `.gam` `3CAR` | Current rows range `-1..1500`. |
| inherited `0x8d4..0xa78` | talk table | `TalkState0..4`, `TalkTrigger0..4` | `C3DFriends`; `.gam` `3CAR` | Carl rows use `TalkTrigger0`, sometimes `TalkTrigger1`; remaining slots are disabled with `-1`/`"none"`. |
| `0x57c` | handle/pointer | `carl_texture_canvas_handle` | `00413d60` | Passed to the inherited material/canvas slot after loading `carl.png`. Exact owner type is unresolved. |
| `0x635` | bool | `assets_registered` | ctor `00413af0`, `00413d60` | Cleared by the constructor; `RegisterCarlAssets` sets it after one-time mesh/texture setup. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `00415090` | `InitObjectSharedFriendVariant` | Vtable target is currently owned by `C3DBenny` in the dump, but Carl's constructor calls it after clearing `assets_registered`; treat as shared friend leaf setup until sibling specs name it. | inherited/shared |
| 8 | `00413d30` | `UnInitObjectCarl` | Logs/traces `"UnInitObject()"`, calls `C3DAnimated::UnInitObjectAnimated`, then runs the common no-op/cleanup hook. | non-trivial |
| 10 | `0041b790` | `ResetFriendsRuntime` | Inherited `C3DFriends` runtime reset. | inherited |
| 241 | `0041b950` | `UpdateFriendsTalkMarker` | Inherited `C3DFriends` per-frame AI/talk marker update. | inherited |
| 259 | `0042d010` | `PostLoadAIThunk` | Thin jump to `C3DAI::PostLoadAI` at `00409480`. | inherited thunk |
| 264 | `00413eb0` | `SyncCarlTaskAndTeleportToCarl8` | Raw helper. Refreshes the task state named by inherited `TaskName`; when `SCENE == 0x46`, resolves object tag `CARL8` and copies its transform into Carl through inherited transform slots. | raw block |
| 265 | `00413f30` | `ApplyCarlProgressVisibility` | Raw helper. Reads `SCENE`, runs `C3DAnimated` slot 265, checks current level FourCC, and toggles an inherited visibility/enable slot for Carl-specific progress windows; the `LV1B` path also moves Carl to `CARLESC3`. | raw block |
| vtable 3 slot 2 | `00413c70` | scalar deleting destructor | Runs the Carl cleanup/vtable reset helper, destroys the adjusted streamer/string subobject, and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 67 | `00413d60` | `RegisterCarlAssets` | One-time asset registration. Binds Carl animation aliases to ASE files, loads `carl.png`, selects `STOP`, and applies two inherited shape/scale constants. | non-trivial |
| vtable 4 slot 96 | `00413e50` | `HandleCarlTalkProgressReward` | Calls `C3DFriends::StartFriendTalkPulse`, then maps exact `SCENE` states `0x32`, `0x41`, and `0x10e` to follow-up states `0x3c`, `0x46`, and `0x118`. | raw block |

## Per-Frame Behavior

Carl does not add a new movement integrator. Normal movement and talk-marker maintenance are inherited from `C3DFriends`/`C3DAI`; Carl's leaf behavior is task-state gating and scripted placement.

```c
C3DCarl::ApplyCarlProgressVisibility(arg):
    scene_state = get_task_state("SCENE")
    C3DAnimated::slot_265(arg)
    level = current_game->level_fourcc

    if level == "LEV1":
        enabled = 0x3c <= scene_state && scene_state < 0x5a
    else if level == "LEV2":
        enabled = 0x10e <= scene_state && scene_state < 0x122
    else if level == "LEV3":
        enabled = 0x17c <= scene_state && scene_state < 0x186
    else if level == "LV4A":
        enabled = scene_state >= 0x1f4
    else if level == "LV1B":
        if 0 <= scene_state && scene_state <= 0x3c:
            if scene_state > 0x1e:
                marker = find_object("CARLESC3")
                if marker:
                    set_ai_or_motion_mode(6)
                    copy_transform_from(marker)
            if scene_state < 0x3c:
                inherited_enable_or_visibility_slot(true)
        else:
            inherited_enable_or_visibility_slot(false)
        return
    else:
        enabled = true

    inherited_enable_or_visibility_slot(enabled)
```

```c
C3DCarl::HandleCarlTalkProgressReward():
    C3DFriends::StartFriendTalkPulse()
    switch get_task_state("SCENE"):
    case 0x32:
        set_task_state("SCENE", 0x3c)
        break
    case 0x41:
        set_task_state("SCENE", 0x46)
        break
    case 0x10e:
        set_task_state("SCENE", 0x118)
        break
```

## Constants And Wiring

### `.gam` Placeable Properties

`3CAR` appears 9 times across the level `.gam` files. It serializes common object/animated fields, inherited AI fields, and inherited friend talk fields. No Carl-only property is registered beyond the concrete class id.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"C3DCARL"`, `"Carl3"`, `"Carl4"` | Base object tag and lookup identity. |
| `RotateToDest` | flag4 | inherited | `01010101` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `860045650` | FourCC/object id value for `3CAR`. |
| `PositionX` | float | inherited | `-4380..6900` | Base placement transform. |
| `PositionY` | float | inherited | `-6110..1790` | Base placement transform. |
| `PositionZ` | float | inherited | `-39100..5280` | Base placement transform. |
| `RotationX` | float | inherited | `0` | Base placement transform. |
| `RotationY` | float | inherited | `0..300` | Base placement transform and initial facing. |
| `RotationZ` | float | inherited | `0` | Base placement transform. |
| `TaskName` | str | inherited `0x430` | `"SCENE"`, `"Scene"`, `"scene"` | Consumed by Carl slot 264 and matches raw hooks that read the `SCENE` task state. |
| `Debug` | int | inherited | `0` | Base debug flag. |
| `RequiredLevel` | int | inherited | `-1..380` | Inherited animated/progress lower gate. |
| `ExactLevel` | int | inherited | `-1..390` | Inherited animated/progress exact gate. |
| `RemoveLevel` | int | inherited | `-1..400` | Inherited animated/progress upper gate. |
| `HasCollision` | int | inherited | `0..1` | Inherited collision toggle. |
| `InitiallyVisible` | int | inherited | `-1` | Inherited initial visibility. |
| `CanMove` | int | inherited | `1` | Inherited transform/update gate. |
| `SecondPass` | int | inherited | `0` | Inherited render/update pass flag. |
| `PickupLink` | str | inherited | `"none"` | Inherited animated link field; present on 8 rows. |
| `PatrolPoint` | str | inherited `0x648` | `"CARL1"`, `"carl1"`, `"crl1"`, `"look1"` | Resolved by inherited `C3DAI` patrol logic. |
| `VisibleRange` | float | inherited `0x644` | `500..750` | Compared by inherited `C3DAI` target/range logic. |
| `FOV` | float | inherited `0x80c` | `90..359` | Used by inherited AI facing/visibility helpers. |
| `TargetName` | str | inherited `0x6ac` | `"JIM1"`, `"Jim1"` | Resolved by inherited `C3DAI::PostLoadAI`. |
| `AIState` | int | inherited `0x87c` | `1..9` | Copied into inherited runtime AI state. |
| `WanderRange` | float | inherited `0x89c` | `-1..1500` | Used by inherited AI wander/search helpers. |
| `TalkState0` | int | inherited `0x8d4` | `0..380` | Gate for `TalkTrigger0` in `C3DFriends`. |
| `TalkTrigger0` | str | inherited `0x8e8` | `"getstaken"`, `"inhaler"`, `"neutron1a"`, `"neutron1b"` | Resolved by inherited friend talk activation. |
| `TalkState1` | int | inherited `0x8d8` | `-1..70` | Optional second friend-talk gate. |
| `TalkTrigger1` | str | inherited `0x94c` | `"neutron1c"`, `"none"` | Optional second friend-talk trigger. |
| `TalkState2..4` | int | inherited `0x8dc..0x8e4` | `-1` | Disabled remaining friend talk gates. |
| `TalkTrigger2..4` | str | inherited `0x9b0..0xa78` | `"none"` | Disabled remaining friend talk triggers. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3CAR` | Concrete placeable class id for Carl. | ctor `00413af0`; `push 0x33434152` at `00413bb3` |
| `C3DCARL` | Concrete class/object string. | string `.data:004edb8c`; constructor string path |
| `SCENE` | Task-state key read and advanced by Carl raw hooks. | string `.data:004ed220`; raw `00413e50`, `00413f30` |
| `CARL8` | Object tag copied by slot 264 when `SCENE == 0x46`. | raw `00413eb0`; string `.data:004edc40` |
| `CARLESC3` | Object tag copied on the `LV1B` visibility/placement path after `SCENE > 0x1e`. | raw `00413f30`; string `.data:004edc48` |
| `LEV1`, `LEV2`, `LEV3`, `LV4A`, `LV1B` | Level FourCC gates for Carl visibility windows and placement. | raw `00413f30`; class-id table confirms storage names for `LV4A`/`LV1B` |
| `0x3c..0x59`, `0x10e..0x121`, `0x17c..0x185`, `>=0x1f4` | Visibility windows for `LEV1`, `LEV2`, `LEV3`, and `LV4A`. | raw `00413f30` |
| `0x32 -> 0x3c`, `0x41 -> 0x46`, `0x10e -> 0x118` | Talk-progress state transitions after Carl talk starts. | raw `00413e50` |
| `100.0`, `0.75` | Shape/scale constants applied after asset registration through inherited adjusted slots. | `00413d60`; immediates `0x42c80000`, `0x3f400000` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| texture | `carl.png` | `00413d60`; string `.data:004edbb4` | Loaded once when `assets_registered` is clear. |
| animation | `HISTOP` -> `carlstop.ase` | `00413d60` | Default stop/idle animation. |
| animation | `HIINHALE` -> `carlinhale.ase` | `00413d60` | Inhale animation. |
| animation | `HICHEER` -> `carlcheer.ase` | `00413d60` | Cheer animation. |
| animation | `HITELE` -> `carlteleport.ase` | `00413d60` | Teleport animation. |
| animation | `HITALK` -> `carltalk.ase` | `00413d60` | Talk animation. |
| animation | `HIWALK` -> `carlwalk.ase` | `00413d60` | Walk animation. |
| animation default | `STOP` | `00413d60`; string `.data:004ed040` | Selected after texture/canvas setup. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, objdump over `/home/scotty/xp-jnbg-original/Neutron.exe`, and `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Create proper Ghidra functions for raw targets `00413eb0`, `00413e50`, and `00413f30`; `DumpFunctions` could not decompile them because they are not currently function symbols.
- Name the inherited slot at offset `0x110` used as Carl's task-window visibility/enable toggle.
- Confirm the exact runtime effect of the `LV1B` state `0x3c` path, which copies/places Carl but returns without an explicit visibility toggle.
- Resolve why Carl's slot 7 target is currently owned by sibling class `C3DBenny` in the dump.
- Runtime-check the `CARL8` and `CARLESC3` placement paths before marking the class `validated`.

## Notes

- Evidence: `DumpClass.java C3DCarl /tmp/decomp_C3DCarl.md` (`slots=394`, `owned_methods=2`, `offsets=0`), `DumpFunctions.java /tmp/decomp_C3DCarl_raw.md 00413eb0 00413f30 00413e50 0042d010`, and local `objdump` for raw method boundaries.
- The decompiler lifted only `UnInitObjectCarl` and `RegisterCarlAssets`; the task/visibility hooks are confirmed by `.rdata` vtable targets and contiguous disassembly but still need Ghidra function-boundary repair.
