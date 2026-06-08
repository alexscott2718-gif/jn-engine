# C3DLibby

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DLibby` |
| Base chain | `C3DFriends -> C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004a5c28`, `004a5c38`, `004a6088`, `004a60c4`, `004a60d8` |
| Ctor(s) | constructor/factory block `FUN_0042cd40`; registers FourCC `3LIB` at `0042ce02` |
| Dtor(s) | adjusted scalar deleting destructor at `0042ceb0`; cleanup/vtable reset helper `0042cee0` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DLibby` is the concrete `3LIB` friend/NPC leaf for Libby. It inherits `C3DFriends` talk activation and `C3DAI` movement, registers Libby-specific animations and texture, and adds task-state hooks for Libby visibility, talk-progress rewards, and a one-shot reward path when a specific scene state is reached.

## Field Map

Offsets below are byte offsets from the outer `C3DLibby` allocation pointer used by the constructor and Libby leaf hooks, unless marked inherited. Inherited `C3DFriends`/`C3DAI` fields are documented on those base specs.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x644` | float | `VisibleRange` | `C3DAI`; `.gam` `3LIB`; ctor `0042cd40` | Current rows use `500..700`; constructor default stores `500.0`. |
| inherited `0x648` | char buffer/string | `PatrolPoint` | `C3DAI`; `.gam` `3LIB` | Current rows use `"Lib1"`, `"lib1"`, or `"none"`. |
| inherited `0x6ac` | char buffer/string | `TargetName` | `C3DAI`; `.gam` `3LIB` | All current rows target `"JIM1"`. |
| inherited `0x760` | char buffer/string | `anim_run` / movement anim slot | `C3DAI`; raw `0042d230` | Libby visibility path writes `WALK` into this inherited animation slot before forcing movement state in some level windows. |
| inherited `0x80c` | float | `FOV` | `C3DAI`; `.gam` `3LIB` | Current rows range `90..359`. |
| inherited `0x87c` | int | `AIState` | `C3DAI`; `.gam` `3LIB`; ctor `0042cd40` | Serialized initial AI state. Current rows use `1` or `2`; constructor seeds a default `3` in the runtime/default state slots. |
| inherited `0x89c` | float | `WanderRange` | `C3DAI`; `.gam` `3LIB` | Current rows range `-1..1500`. |
| inherited `0x8d4..0xa78` | talk table | `TalkState0..4`, `TalkTrigger0..4` | `C3DFriends`; `.gam` `3LIB` | Libby rows use up to three talk triggers; remaining slots are disabled with `-1`/`"none"`. |
| `0x57c` | handle/pointer | `libby_texture_canvas_handle` | `0042cf30` | Passed to the inherited material/canvas slot after loading `libby.png`. Exact owner type is unresolved. |
| `0x635` | bool | `assets_registered` | ctor `0042cd40`, `0042cf30` | Cleared by the constructor; `RegisterLibbyAssets` sets it after one-time mesh/texture setup. |
| `0x800` | int/flag | `libby_reset_state` | raw `0042d190` | Cleared after the Libby reset helper snapshots transform data and calls `C3DFriends::ResetFriendsRuntime`. |
| `this - 0x7c..this - 0x74` | vec3 cache | `reset_transform_cache` | raw `0042d190` | Stores components sampled from inherited transform slots before the reset hook. Pointer adjustment still needs struct cleanup. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `00415090` | `InitObjectSharedFriendVariant` | Vtable target is currently owned by `C3DBenny` in the dump, but Libby's constructor calls it after clearing `assets_registered`; treat as shared friend leaf setup until sibling specs name it. | inherited/shared |
| 10 | `0042d190` | `ResetLibbyRuntimeTransform` | Raw helper. Samples inherited transform vectors through slots `0x164`, `0x314`, and `0x310`, writes a small transform cache, calls `C3DFriends::ResetFriendsRuntime`, then clears `0x800`. | raw block |
| 241 | `0041b950` | `UpdateFriendsTalkMarker` | Inherited `C3DFriends` per-frame AI/talk marker update. | inherited |
| 259 | `0042d010` | `PostLoadAIThunk` | Thin jump to `C3DAI::PostLoadAI` at `00409480`. | inherited thunk |
| 264 | `00419aa0` | `SharedArrowOwnedHelper` | Vtable target is currently owned by `C3DArrow` in the dump. Keep as a shared/foreign helper until function-boundary and sibling analysis explain the reuse. | inherited/shared |
| 265 | `0042d230` | `ApplyLibbyProgressVisibility` | Raw helper. Runs `C3DAnimated` slot 265, reads `SCENE`, checks current level FourCC, toggles an inherited visibility/enable slot, and forces movement/animation state in selected windows. | raw block |
| vtable 3 slot 2 | `0042ceb0` | scalar deleting destructor | Runs the Libby cleanup/vtable reset helper, destroys the adjusted streamer/string subobject, and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 67 | `0042cf30` | `RegisterLibbyAssets` | One-time asset registration. Binds Libby animation aliases to ASE files, loads `libby.png`, selects `STOP`, and applies two inherited shape/scale constants. | non-trivial |
| vtable 4 slot 90 | `0042d020` | `SetLibbyState3AndScene104Reward` | Calls `C3DFriends::SetFriendState3`; when `SCENE == 0x104`, also calls `FUN_004038c0(2, 7, 1)` and `FUN_004061d0(7, 0)`. | raw block |
| vtable 4 slot 96 | `0042d060` | `HandleLibbyTalkProgressReward` | Calls `C3DFriends::StartFriendTalkPulse`, then maps exact `SCENE` states `0xff`, `0x12c`, `0x15e`, and `0x18f` to follow-up states `0x104`, `0x136`, `0x168`, and `0x19a`. | raw block |

## Per-Frame Behavior

Libby does not add a new movement integrator. Normal movement and talk-marker maintenance are inherited from `C3DFriends`/`C3DAI`; Libby's leaf behavior is task-state gating, talk rewards, and a small movement-state override for scripted windows.

```c
C3DLibby::ApplyLibbyProgressVisibility(arg):
    C3DAnimated::slot_265(arg)
    scene_state = get_task_state("SCENE")
    level = current_game->level_fourcc

    if level == "LEV1":
        if 0xfa <= scene_state && scene_state < 0x10e:
            if scene_state == 0xfa:
                SetAIState(3)
            inherited_enable_or_visibility_slot(true)
        else if 0x12c <= scene_state && scene_state < 0x172:
            set_animation_string("WALK")
            SetAIState(0)
            inherited_enable_or_visibility_slot(true)
        else:
            inherited_enable_or_visibility_slot(false)
    else if level == "LV3C":
        if 0x190 <= scene_state && scene_state < 0x1a4:
            set_animation_string("WALK")
            inherited_enable_or_visibility_slot(true)
        else:
            inherited_enable_or_visibility_slot(false)
    else if level == "LV4A":
        inherited_enable_or_visibility_slot(scene_state >= 0x1f4)
    else if level == "LV4D":
        set_animation_string("WALK")
        inherited_enable_or_visibility_slot(true)
    else:
        inherited_enable_or_visibility_slot(true)
```

```c
C3DLibby::HandleLibbyTalkProgressReward():
    C3DFriends::StartFriendTalkPulse()
    switch get_task_state("SCENE"):
    case 0xff:
        set_task_state("SCENE", 0x104)
        break
    case 0x12c:
        set_task_state("SCENE", 0x136)
        break
    case 0x15e:
        set_task_state("SCENE", 0x168)
        break
    case 0x18f:
        set_task_state("SCENE", 0x19a)
        break
```

## Constants And Wiring

### `.gam` Placeable Properties

`3LIB` appears 3 times across the level `.gam` files. It serializes common object/animated fields, inherited AI fields, and inherited friend talk fields. No Libby-only property is registered beyond the concrete class id.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"C3DLIBBY"`, `"libby2"` | Base object tag and lookup identity. |
| `RotateToDest` | flag4 | inherited | `00010101`, `01010101` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `860637506` | FourCC/object id value for `3LIB`. |
| `PositionX` | float | inherited | `-101..10100` | Base placement transform. |
| `PositionY` | float | inherited | `4.71..36.5` | Base placement transform. |
| `PositionZ` | float | inherited | `-915..1040` | Base placement transform. |
| `RotationX` | float | inherited | `0` | Base placement transform. |
| `RotationY` | float | inherited | `0..187` | Base placement transform and initial facing. |
| `RotationZ` | float | inherited | `0` | Base placement transform. |
| `TaskName` | str | inherited | `"Scene"`, `"scene"` | Matches Libby raw hooks that read the `SCENE` task state. |
| `Debug` | int | inherited | `0..1` | Base debug flag. |
| `RequiredLevel` | int | inherited | `0..400` | Inherited animated/progress lower gate. |
| `ExactLevel` | int | inherited | `-1` | Inherited animated/progress exact gate. |
| `RemoveLevel` | int | inherited | `-1..420` | Inherited animated/progress upper gate. |
| `HasCollision` | int | inherited | `1` | Inherited collision toggle. |
| `InitiallyVisible` | int | inherited | `-1` | Inherited initial visibility. |
| `CanMove` | int | inherited | `1` | Inherited transform/update gate. |
| `SecondPass` | int | inherited | `0` | Inherited render/update pass flag. |
| `PickupLink` | str | inherited | `"none"` | Inherited animated link field. |
| `PatrolPoint` | str | inherited `0x648` | `"Lib1"`, `"lib1"`, `"none"` | Resolved by inherited `C3DAI` patrol logic. |
| `VisibleRange` | float | inherited `0x644` | `500..700` | Compared by inherited `C3DAI` target/range logic. |
| `FOV` | float | inherited `0x80c` | `90..359` | Used by inherited AI facing/visibility helpers. |
| `TargetName` | str | inherited `0x6ac` | `"JIM1"` | Resolved by inherited `C3DAI::PostLoadAI`. |
| `AIState` | int | inherited `0x87c` | `1..2` | Copied into inherited runtime AI state. |
| `WanderRange` | float | inherited `0x89c` | `-1..1500` | Used by inherited AI wander/search helpers. |
| `TalkState0` | int | inherited `0x8d4` | `0..400` | Gate for `TalkTrigger0` in `C3DFriends`. |
| `TalkTrigger0` | str | inherited `0x8e8` | `"needcindy"`, `"none"`, `"seecindy"` | Resolved by inherited friend talk activation. |
| `TalkState1` | int | inherited `0x8d8` | `-1..300` | Optional second friend-talk gate. |
| `TalkTrigger1` | str | inherited `0x94c` | `"none"`, `"seesheen"` | Optional second friend-talk trigger. |
| `TalkState2` | int | inherited `0x8dc` | `-1..350` | Optional third friend-talk gate. |
| `TalkTrigger2` | str | inherited `0x9b0` | `"none"`, `"sheencandybar"` | Optional third friend-talk trigger. |
| `TalkState3..4` | int | inherited `0x8e0..0x8e4` | `-1` | Disabled remaining friend talk gates. |
| `TalkTrigger3..4` | str | inherited `0xa14..0xa78` | `"none"` | Disabled remaining friend talk triggers. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3LIB` | Concrete placeable class id for Libby. | ctor `0042cd40`; `push 0x334c4942` at `0042ce02` |
| `C3DLIBBY` | Concrete object/type string. | string `.data:004ef710`; constructor string path |
| `C3DLibby` | Concrete class string. | string `.data:004ef9e0`; constructor string path |
| `SCENE` | Task-state key read and advanced by Libby raw hooks. | string `.data:004ed220`; raw `0042d020`, `0042d060`, `0042d230` |
| `LEV1`, `LV3C`, `LV4A`, `LV4D` | Level FourCC gates for Libby visibility windows and movement-state overrides. | raw `0042d230`; class-id table confirms storage names |
| `0xfa..0x10d`, `0x12c..0x171`, `0x190..0x1a3`, `>=0x1f4` | Visibility windows for `LEV1`, `LV3C`, and `LV4A`; `LV4D` is always enabled. | raw `0042d230` |
| `0xff -> 0x104`, `0x12c -> 0x136`, `0x15e -> 0x168`, `0x18f -> 0x19a` | Talk-progress state transitions after Libby talk starts. | raw `0042d060` jump table |
| `FUN_004038c0(2, 7, 1)`, `FUN_004061d0(7, 0)` | Additional side effects when vtable-4 slot 90 runs at `SCENE == 0x104`. | raw `0042d020` |
| `150.0`, `0.5` | Shape/scale constants applied after asset registration through inherited adjusted slots. | `0042cf30`; immediates `0x43160000`, `0x3f000000` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| texture | `libby.png` | `0042cf30`; string `.data:004ef9f4` | Loaded once when `assets_registered` is clear. |
| animation | `HISTOP` -> `libystop.ase` | `0042cf30` | Default stop/idle animation. |
| animation | `HIPHONE` -> `libbyphone.ase` | `0042cf30` | Phone animation. |
| animation | `HIRUN` -> `libyrun.ase` | `0042cf30` | Run animation. |
| animation | `HIWALK` -> `libywalk.ase` | `0042cf30` | Walk animation. |
| animation | `HITALK` -> `libytalk.ase` | `0042cf30` | Talk animation. |
| animation default | `STOP` | `0042cf30`; string `.data:004ed040` | Selected after texture/canvas setup. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, objdump over `/home/scotty/xp-jnbg-original/Neutron.exe`, and `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Create proper Ghidra functions for raw targets `0042d190`, `0042d020`, `0042d060`, and `0042d230`; `DumpFunctions` could not decompile them because they are not currently function symbols.
- Name the inherited slot at offset `0x110` used as Libby's task-window visibility/enable toggle and slot `0x120` used for animation/movement string setup.
- Resolve why Libby's slot 7 and slot 264 targets are currently owned by sibling/foreign classes (`C3DBenny` and `C3DArrow`) in the dump.
- Confirm the runtime effect of the `SCENE == 0x104` reward side effects in vtable-4 slot 90.
- Runtime-check Libby talk triggers and LV3C/LV4D visibility paths before marking the class `validated`.

## Notes

- Evidence: `DumpClass.java C3DLibby /tmp/decomp_C3DLibby.md` (`slots=394`, `owned_methods=1`, `offsets=0`), `DumpFunctions.java /tmp/decomp_C3DLibby_raw.md 0042d190 0042d020 0042d060 0042d230 0042d010`, and local `objdump` for raw method boundaries.
- The only decompiled owned method is `0042cf30`; the reset/task/visibility hooks are confirmed by `.rdata` vtable targets and contiguous disassembly but still need Ghidra function-boundary repair.
