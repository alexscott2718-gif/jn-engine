# C3DKitty

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DKitty` |
| Base chain | `C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004a44dc`, `004a44ec`, `004a493c`, `004a4978`, `004a498c` |
| Ctor(s) | constructor/factory block `FUN_0042b800`; registers FourCC `3KIT` at `0042b911` |
| Dtor(s) | adjusted scalar deleting destructor at `0042b950`; cleanup/vtable reset helper `0042b980` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DKitty` is the concrete `3KIT` cat/kitty AI leaf. It derives directly from `C3DAI`, not `C3DFriends`, so it has no friend talk table. Its behavior is asset setup, AI movement inherited from `C3DAI`, task-state visibility gating, and lifecycle management of an unresolved effect/sound handle keyed by id `0x53`.

## Field Map

Offsets below are byte offsets from the outer `C3DKitty` allocation pointer used by the constructor and raw hooks, unless marked inherited. Inherited `C3DAI` fields are documented on the base spec.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x430` | char buffer/string | `TaskName` | `CLocalGameObject`; `.gam` `3KIT`; `0042ba90`, `0042bb00` | Kitty rows use `"kitty1"`/`"kitty2"`. The task state gates visibility/effect lifetime at threshold `10`. |
| inherited `0x644` | float | `VisibleRange` | `C3DAI`; `.gam` `3KIT` | Current rows use `2500.0..4000.0`. |
| inherited `0x648` | char buffer/string | `PatrolPoint` | `C3DAI`; `.gam` `3KIT` | Current rows use `"cat1"` and `"runpuss1"`. |
| inherited `0x6ac` | char buffer/string | `TargetName` | `C3DAI`; `.gam` `3KIT` | Current rows target `"JIM1"` or `"none"`. |
| inherited `0x80c` | float | `FOV` | `C3DAI`; `.gam` `3KIT` | Current rows use `90` and `359`. |
| inherited `0x87c` | int | `AIState` | `C3DAI`; `.gam` `3KIT` | Current rows use `1`. |
| inherited `0x89c` | float | `WanderRange` | `C3DAI`; `.gam` `3KIT` | Current rows use `1500.0`. |
| `0x57c` | handle/pointer | `kitty_texture_canvas_handle` | `0042b9d0` | Passed to the inherited material/canvas slot after loading `cat.png`. Exact owner type is unresolved. |
| `0x7f8` | char buffer/string | `kitty_stop_anim_0` | ctor `0042b800` | Constructor copies `STOP`. |
| `0x820` | char buffer/string | `kitty_walk_or_default_anim` | ctor `0042b800` | Constructor copies the inherited/default string at `004eca54`; semantic name unresolved. |
| `0x870` | char buffer/string | `kitty_stop_anim_1` | ctor `0042b800` | Constructor copies `STOP`. |
| `0x898` | char buffer/string | `kitty_attack_anim` | ctor `0042b800` | Constructor copies `ATTACK`. |
| `0x908` | byte | `kitty_runtime_flag` | ctor `0042b800` | Constructor clears this flag. Exact meaning unresolved. |
| `0x994` | handle/int | `kitty_effect_handle` | ctor `0042b800`, `0042ba90`, `0042bb00`, `0042bb40`, `0042bb60` | Initialized to `-1`; when active, stores the handle returned by `FUN_004589c0(this, -1, 0x53, 1)`. Released through `FUN_0047d7a0(handle, 0)`. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `0042b9d0` | `InitObjectKitty` | Traces `InitObject()`, calls `C3DAI::InitObject`, runs an inherited adjusted setup slot at `0x108`, registers cat animations, loads `cat.png`, applies a `10.0` inherited shape/range constant, and selects `STOP`. | non-trivial |
| 10 | `00407eb0` | `ResetAI` | Inherited `C3DAI` reset helper. | inherited |
| 241 | `00408000` | `UpdateAI` | Inherited `C3DAI` update. | inherited |
| 259 | `0042ba90` | `PostLoadKitty` | Calls `C3DAI::PostLoadAI`, reads the `TaskName` state, hides Kitty if the state is `>= 10`, otherwise shows Kitty and starts the id `0x53` effect if no handle exists. | non-trivial |
| 264 | `00419aa0` | `RefreshTaskStateShared` | Shared helper currently owned by `C3DArrow`. Calls a common no-op hook, reads the task state for this object's inherited task name, and writes the same value back. | inherited/shared |
| 265 | `0042bb00` | `ReleaseKittyEffectAfterTask` | Runs `C3DAnimated` slot 265, then releases `kitty_effect_handle` and resets it to `-1` once the task state reaches `10`. | raw block |
| 272 | `0042bb40` | `SuspendKittyEffect` | Calls `C3DAnimated` slot 272 and releases the active id `0x53` effect handle when present. | non-trivial |
| 273 | `0042bb60` | `ResumeKittyEffect` | Calls `C3DAnimated` slot 273 and reacquires the id `0x53` effect handle when the field is not `-1`. | non-trivial |
| vtable 3 slot 2 | `0042b950` | scalar deleting destructor | Runs the Kitty cleanup/vtable reset helper, destroys the adjusted streamer/string subobject, and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 67 | `00472970` | `CGameObject::vfunc_00_013` | Kitty does not override the asset-registration slot used by some animated leaves; assets are registered from slot 7 directly. | inherited |

## Runtime Behavior

Kitty does not add a new movement integrator. Its movement, patrol, target lookup, and per-frame AI are inherited from `C3DAI`. The concrete leaf manages visibility and an effect/sound handle around its task state.

```c
C3DKitty::PostLoadKitty():
    C3DAI::PostLoadAI()
    state = get_task_state(TaskName)

    if state >= 10:
        inherited_enable_or_visibility_slot(false)
        return

    inherited_enable_or_visibility_slot(true)
    if kitty_effect_handle == -1:
        kitty_effect_handle = FUN_004589c0(this, -1, 0x53, 1)
```

```c
C3DKitty::ReleaseKittyEffectAfterTask(arg):
    C3DAnimated::ApplyLevelGate(arg)
    if get_task_state(TaskName) >= 10:
        FUN_0047d7a0(kitty_effect_handle, 0)
        kitty_effect_handle = -1
```

```c
C3DKitty::SuspendKittyEffect():
    C3DAnimated::slot_272()
    if kitty_effect_handle != -1:
        FUN_0047d7a0(kitty_effect_handle, 0)

C3DKitty::ResumeKittyEffect():
    C3DAnimated::slot_273()
    if kitty_effect_handle != -1:
        kitty_effect_handle = FUN_004589c0(this, -1, 0x53, 1)
```

## Constants And Wiring

### `.gam` Placeable Properties

`3KIT` appears 2 times across the level `.gam` files. It serializes common object/animated fields plus inherited `C3DAI` fields. No friend talk properties are present because Kitty does not derive from `C3DFriends`.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"C3DKITTY"` | Base object tag and lookup identity. |
| `RotateToDest` | flag4 | inherited | `01010101` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `860571988` | FourCC/object id value for `3KIT`. |
| `PositionX` | float | inherited | `3620..7010` | Base placement transform. |
| `PositionY` | float | inherited | `791..818` | Base placement transform. |
| `PositionZ` | float | inherited | `1660..5090` | Base placement transform. |
| `RotationX` | float | inherited | `0` | Base placement transform. |
| `RotationY` | float | inherited | `0..90` | Base placement transform and initial facing. |
| `RotationZ` | float | inherited | `0` | Base placement transform. |
| `TaskName` | str | inherited `0x430` | `"kitty1"`, `"kitty2"` | Task state controls Kitty visibility/effect lifetime at threshold `10`. |
| `Debug` | int | inherited | `0` | Base debug flag. |
| `RequiredLevel` | int | inherited | `-1..0` | Inherited animated/progress lower gate. |
| `ExactLevel` | int | inherited | `0` | Inherited animated/progress exact gate. |
| `RemoveLevel` | int | inherited | `-1` | Inherited animated/progress upper gate. |
| `HasCollision` | int | inherited | `1` | Inherited collision toggle. |
| `InitiallyVisible` | int | inherited | `0..1` | Inherited initial visibility. |
| `CanMove` | int | inherited | `1` | Inherited transform/update gate. |
| `SecondPass` | int | inherited | `0` | Inherited render/update pass flag. |
| `PatrolPoint` | str | inherited `0x648` | `"cat1"`, `"runpuss1"` | Resolved by inherited `C3DAI` patrol logic. |
| `VisibleRange` | float | inherited `0x644` | `2500..4000` | Compared by inherited `C3DAI` target/range logic. |
| `FOV` | float | inherited `0x80c` | `90`, `359` | Used by inherited AI facing/visibility helpers. |
| `TargetName` | str | inherited `0x6ac` | `"JIM1"`, `"none"` | Resolved by inherited `C3DAI::PostLoadAI`. |
| `AIState` | int | inherited `0x87c` | `1` | Copied into inherited runtime AI state. |
| `WanderRange` | float | inherited `0x89c` | `1500` | Used by inherited AI wander/search helpers. |
| `PickupLink` | str | inherited | `"none"` | Inherited animated link field; only one Kitty row includes this serialized property. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3KIT` | Concrete placeable class id for Kitty. | ctor `0042b800`; `push 0x334b4954` at `0042b911` |
| `C3DKITTY` | Concrete object/type string. | string `.data:004ef878`; constructor string path |
| `HIATTACK`, `HIWALK`, `HISTOP` | Kitty animation aliases registered during init. | `0042b9d0` |
| `ATTACK`, `STOP` | Constructor/default animation strings. | ctor `0042b800`; strings `.data:004ee550`, `004ed040` |
| task threshold `10` | Kitty hides and releases its effect once `TaskName` state reaches this value. | `0042ba90`, `0042bb00` |
| effect id `0x53` | Id passed to `FUN_004589c0` when acquiring `kitty_effect_handle`. | `0042ba90`, `0042bb60` |
| `10.0` | Shape/range constant applied during init through an inherited adjusted slot. | `0042b9d0`; immediate `0x41200000` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| texture | `cat.png` | `0042b9d0`; string `.data:004ef884` | Loaded during Kitty init and passed to inherited canvas/material setup. |
| animation | `HIATTACK` -> `catrun.ase` | `0042b9d0` | Attack/run animation. |
| animation | `HIWALK` -> `catrun.ase` | `0042b9d0` | Walk alias uses the same ASE as attack. |
| animation | `HISTOP` -> `catsit.ase` | `0042b9d0` | Stop/sit animation. |
| animation default | `STOP` | `0042b9d0`; string `.data:004ed040` | Selected after texture/canvas setup. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, objdump over `/home/scotty/xp-jnbg-original/Neutron.exe`, asset scan, and `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Name `FUN_004589c0` and `FUN_0047d7a0`; current evidence suggests effect/sound handle acquire/release, but the exact subsystem owner is unresolved.
- Name the inherited slot at offset `0x110` used as Kitty's visibility/enable toggle.
- Resolve the runtime flags at `0x908` and the animation string fields at `0x7f8`, `0x820`, `0x870`, and `0x898` against the inherited `C3DAI`/`C3DAnimated` struct.
- Runtime-check the `kitty1`/`kitty2` task threshold behavior and id `0x53` effect before marking the class `validated`.

## Notes

- Evidence: `DumpClass.java C3DKitty /tmp/decomp_C3DKitty.md` (`slots=391`, `owned_methods=4`, `offsets=1`), local `objdump` windows over `0042b800..0042bc00`, and `.gam` schema for `3KIT`.
- `3KIT -> C3DKitty` was backfilled in `docs/_gam_classids.tsv` from RTTI/class dump evidence during this spec, then `python3 tools/gam_schema.py` regenerated `docs/gam_schema.md`.
- `assets/ase/cattalk.ASE` exists on disk, but no direct `C3DKitty` string/reference was found in this class's asset-registration path.
