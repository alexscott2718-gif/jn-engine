# C3DCindy

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DCindy` |
| Base chain | `C3DFriends -> C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `00496524`, `00496534`, `00496984`, `004969c0`, `004969d4` |
| Ctor(s) | constructor/factory block `FUN_00414db0`; registers FourCC `3CIN` at `00414e72` |
| Dtor(s) | adjusted scalar deleting destructor at `00414f20`; cleanup/vtable reset helper `00414f50` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DCindy` is the concrete `3CIN` friend/NPC leaf for Cindy. It inherits `C3DFriends` talk activation and `C3DAI` movement, registers Cindy-specific animations and texture, and adds task-state hooks for Cindy visibility plus scene-state reward/progress transitions.

## Field Map

Offsets below are byte offsets from the outer `C3DCindy` allocation pointer used by the constructor and Cindy leaf hooks, unless marked inherited. Inherited `C3DFriends`/`C3DAI` fields are documented on those base specs.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x430` | char buffer/string | `TaskName` | `CLocalGameObject`; `.gam` `3CIN`; shared slot `00419aa0` | Slot 264 refreshes the current task state for this inherited task-name string. |
| inherited `0x644` | float | `VisibleRange` | `C3DAI`; `.gam` `3CIN`; ctor `00414db0` | Current rows use `500..1000`; constructor default stores `500.0`. |
| inherited `0x648` | char buffer/string | `PatrolPoint` | `C3DAI`; `.gam` `3CIN` | Current rows use `"CINDY1"`, `"c1"`, `"cin2"`, or `"none"`. |
| inherited `0x6ac` | char buffer/string | `TargetName` | `C3DAI`; `.gam` `3CIN` | Current rows target `"JIM1"` or `"none"`. |
| inherited `0x80c` | float | `FOV` | `C3DAI`; `.gam` `3CIN` | Current rows range `90..359`. |
| inherited `0x87c` | int | `AIState` | `C3DAI`; `.gam` `3CIN`; ctor `00414db0` | Serialized initial AI state. Current rows range `1..6`; constructor seeds a default `2`. |
| inherited `0x89c` | float | `WanderRange` | `C3DAI`; `.gam` `3CIN` | Current rows use `1500.0`. |
| inherited `0x8d4..0xa78` | talk table | `TalkState0..4`, `TalkTrigger0..4` | `C3DFriends`; `.gam` `3CIN` | Cindy rows use only `TalkState0`/`TalkTrigger0`; remaining slots are disabled with `-1`/`"none"`. |
| `0x57c` | handle/pointer | `cindy_texture_canvas_handle` | `00414fa0` | Passed to the inherited material/canvas slot after loading `cindy.png`. Exact owner type is unresolved. |
| `0x635` | bool | `assets_registered` | ctor `00414db0`, `00414fa0` | Cleared by the constructor; `RegisterCindyAssets` sets it after one-time mesh/texture setup. |
| `0x704` | float | `cindy_visible_range_or_scale_default` | ctor `00414db0`, `004150d0` | Written with `500.0` by the constructor and again by the Cindy talk-progress hook; likely overlaps inherited visible-range/default storage through current struct adjustments. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `00415090` | `InitObjectSharedFriendVariant` | Shared friend leaf init. Traces `InitObject()`, calls `C3DFriends::InitObject`, then calls an inherited adjusted slot at `0x108`. Dump ownership currently attributes this target to `C3DBenny`. | inherited/shared |
| 10 | `0041b790` | `ResetFriendsRuntime` | Inherited `C3DFriends` runtime reset. | inherited |
| 241 | `0041b950` | `UpdateFriendsTalkMarker` | Inherited `C3DFriends` per-frame AI/talk marker update. | inherited |
| 259 | `00409480` | `PostLoadAI` | Direct inherited `C3DAI::PostLoadAI`. | inherited |
| 264 | `00419aa0` | `RefreshTaskStateShared` | Shared helper currently owned by `C3DArrow`. Calls a common no-op hook, reads the task state for this object's inherited task name, and writes the same value back. | inherited/shared |
| 265 | `00415180` | `ApplyCindyProgressVisibility` | Raw helper. Runs `C3DAnimated` slot 265, reads `SCENE`, checks current level FourCC, and toggles an inherited visibility/enable slot for Cindy-specific progress windows. | raw block |
| vtable 3 slot 2 | `00414f20` | scalar deleting destructor | Runs the Cindy cleanup/vtable reset helper, destroys the adjusted streamer/string subobject, and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 67 | `00414fa0` | `RegisterCindyAssets` | One-time asset registration. Binds Cindy animation aliases to ASE files, loads `cindy.png`, selects `STOP`, and applies two inherited shape/scale constants. | non-trivial |
| vtable 4 slot 90 | `00415120` | `SetCindyState3AndScene10eReward` | Raw helper. Calls `C3DFriends::SetFriendState3`; when `SCENE == 0x10e`, consumes/adjusts counter id `7`, calls player/state helpers, and triggers `FUN_00406f90(4)`. | raw block |
| vtable 4 slot 96 | `004150d0` | `HandleCindyTalkProgressReward` | Calls `C3DFriends::StartFriendTalkPulse`, writes `500.0` to `0x704`, and when `SCENE == 0x104`, calls `FUN_004038c0(0, 7, 1)` and advances `SCENE` to `0x10e`. | non-trivial |

## Per-Frame Behavior

Cindy does not add a new movement integrator. Normal movement and talk-marker maintenance are inherited from `C3DFriends`/`C3DAI`; Cindy's leaf behavior is task-state gating and two scene-specific reward/progress hooks.

```c
C3DCindy::ApplyCindyProgressVisibility(arg):
    C3DAnimated::slot_265(arg)
    scene_state = get_task_state("SCENE")
    level = current_game->level_fourcc

    if level == "LEV4":
        enabled = 0x104 <= scene_state && scene_state < 0x118
    else if level == "LV3D":
        enabled = 0x190 <= scene_state && scene_state < 0x1a4
    else if level == "LV4A":
        enabled = scene_state >= 0x1f4
    else:
        enabled = true

    inherited_enable_or_visibility_slot(enabled)
```

```c
C3DCindy::HandleCindyTalkProgressReward():
    C3DFriends::StartFriendTalkPulse()
    field_0x704 = 500.0
    if get_task_state("SCENE") == 0x104:
        FUN_004038c0(0, 7, 1)
        set_task_state("SCENE", 0x10e)

C3DCindy::SetCindyState3AndScene10eReward():
    C3DFriends::SetFriendState3()
    if get_task_state("SCENE") == 0x10e:
        count = FUN_004061b0(7)
        if count >= 1:
            FUN_0042adc0(count * 100)
            FUN_004061c0(7, -count)
            FUN_00403910(2, 7)
        FUN_00406f90(4)
```

## Constants And Wiring

### `.gam` Placeable Properties

`3CIN` appears 6 times across the level `.gam` files. It serializes common object/animated fields, inherited AI fields, and inherited friend talk fields. No Cindy-only property is registered beyond the concrete class id.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"C3DCINDY"` | Base object tag and lookup identity. |
| `RotateToDest` | flag4 | inherited | `00010101`, `01010101` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `860047694` | FourCC/object id value for `3CIN`. |
| `PositionX` | float | inherited | `-2300..2140` | Base placement transform. |
| `PositionY` | float | inherited | `-4150..1800` | Base placement transform. |
| `PositionZ` | float | inherited | `-417..4940` | Base placement transform. |
| `RotationX` | float | inherited | `0..360` | Base placement transform. |
| `RotationY` | float | inherited | `0..200` | Base placement transform and initial facing. |
| `RotationZ` | float | inherited | `0..0.422` | Base placement transform. |
| `TaskName` | str | inherited `0x430` | `"Scene"`, `"scene"` | Matches Cindy raw hooks that read the `SCENE` task state. |
| `Debug` | int | inherited | `0` | Base debug flag. |
| `RequiredLevel` | int | inherited | `-1..0` | Inherited animated/progress lower gate. |
| `ExactLevel` | int | inherited | `-1..420` | Inherited animated/progress exact gate. |
| `RemoveLevel` | int | inherited | `-1..420` | Inherited animated/progress upper gate. |
| `HasCollision` | int | inherited | `0..1` | Inherited collision toggle. |
| `InitiallyVisible` | int | inherited | `-1` | Inherited initial visibility. |
| `CanMove` | int | inherited | `1` | Inherited transform/update gate. |
| `SecondPass` | int | inherited | `0` | Inherited render/update pass flag. |
| `PickupLink` | str | inherited | `"none"` | Inherited animated link field; present on 5 rows. |
| `PatrolPoint` | str | inherited `0x648` | `"CINDY1"`, `"c1"`, `"cin2"`, `"none"` | Resolved by inherited `C3DAI` patrol logic. |
| `VisibleRange` | float | inherited `0x644` | `500..1000` | Compared by inherited `C3DAI` target/range logic. |
| `FOV` | float | inherited `0x80c` | `90..359` | Used by inherited AI facing/visibility helpers. |
| `TargetName` | str | inherited `0x6ac` | `"JIM1"`, `"none"` | Resolved by inherited `C3DAI::PostLoadAI`. |
| `AIState` | int | inherited `0x87c` | `1..6` | Copied into inherited runtime AI state. |
| `WanderRange` | float | inherited `0x89c` | `1500` | Used by inherited AI wander/search helpers. |
| `TalkState0` | int | inherited `0x8d4` | `0..410` | Gate for `TalkTrigger0` in `C3DFriends`. |
| `TalkTrigger0` | str | inherited `0x8e8` | `"getclaw"`, `"needpasscard"`, `"none"` | Resolved by inherited friend talk activation. |
| `TalkState1..4` | int | inherited `0x8d8..0x8e4` | `-1` | Disabled remaining friend talk gates. |
| `TalkTrigger1..4` | str | inherited `0x94c..0xa78` | `"none"` | Disabled remaining friend talk triggers. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3CIN` | Concrete placeable class id for Cindy. | ctor `00414db0`; `push 0x3343494e` at `00414e72` |
| `C3DCINDY` | Concrete object/type string. | string `.data:004edd6c`; constructor string path |
| `C3DCindy` | Concrete class string. | string `.data:004edd60`; constructor string path |
| `SCENE` | Task-state key read and advanced by Cindy raw hooks. | string `.data:004ed220`; raw `004150d0`, `00415120`, `00415180` |
| `LEV4`, `LV3D`, `LV4A` | Level FourCC gates for Cindy visibility windows. | raw `00415180`; class-id table confirms storage names |
| `0x104..0x117`, `0x190..0x1a3`, `>=0x1f4` | Visibility windows for `LEV4`, `LV3D`, and `LV4A`. | raw `00415180` |
| `0x104 -> 0x10e` | Talk-progress state transition after Cindy talk starts. | `004150d0` |
| counter id `7` | Counter/resource consumed by `SetCindyState3AndScene10eReward`. | raw `00415120` |
| `150.0`, `0.5` | Shape/scale constants applied after asset registration through inherited adjusted slots. | `00414fa0`; immediates `0x43160000`, `0x3f000000` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| texture | `cindy.png` | `00414fa0`; string `.data:004edd78` | Loaded once when `assets_registered` is clear. |
| animation | `HISTOP` -> `cindstop.ase` | `00414fa0` | Default stop/idle animation. |
| animation | `HITELE` -> `cindteleport.ase` | `00414fa0` | Teleport animation. |
| animation | `HICHEER` -> `cindycheer.ase` | `00414fa0` | Cheer animation. |
| animation | `HITALK` -> `cindtalk.ase` | `00414fa0` | Talk animation. |
| animation | `HIWALK` -> `cindwalk.ase` | `00414fa0` | Walk animation. |
| animation | `HIWAVE` -> `cindwave.ase` | `00414fa0` | Wave animation. |
| animation default | `STOP` | `00414fa0`; string `.data:004ed040` | Selected after texture/canvas setup. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, objdump over `/home/scotty/xp-jnbg-original/Neutron.exe`, and `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Create a proper Ghidra function for raw target `00415180`; `DumpFunctions` could not decompile it because it is not currently a function symbol.
- Name the inherited slot at offset `0x110` used as Cindy's task-window visibility/enable toggle.
- Resolve why Cindy's slot 7 and slot 264 targets are currently owned by sibling/foreign classes (`C3DBenny` and `C3DArrow`) in the dump.
- Identify the semantic owner of counter id `7` and helper calls `FUN_004061b0`, `FUN_004061c0`, and `FUN_00406f90`.
- Runtime-check Cindy talk triggers and `SCENE == 0x10e` counter consumption before marking the class `validated`.

## Notes

- Evidence: `DumpClass.java C3DCindy /tmp/decomp_C3DCindy.md` (`slots=394`, `owned_methods=2`, `offsets=0`), `DumpFunctions.java /tmp/decomp_C3DCindy_raw.md 00415180 00415120 004150d0 00409480 00419aa0`, and local `objdump` for raw method boundaries.
- Ghidra already decompiles `RegisterCindyAssets` and `HandleCindyTalkProgressReward`; the slot-90 reward helper and slot-265 visibility hook are confirmed by `.rdata` vtable targets and contiguous disassembly.
