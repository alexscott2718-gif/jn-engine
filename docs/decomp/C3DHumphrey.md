# C3DHumphrey

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DHumphrey` |
| Base chain | `C3DEnemy -> C3DPickupType -> C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004a1ec0`, `004a1ed0`, `004a2320`, `004a235c`, `004a2370` |
| Ctor(s) | constructor/factory block `FUN_00420730`; registers FourCC `3HUM` at `0042084b` |
| Dtor(s) | adjusted scalar deleting destructor at `00420890`; cleanup/vtable reset helper `004208c0` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DHumphrey` is the concrete `3HUM` Humphrey/clone enemy leaf. It derives from `C3DEnemy`, registers Humphrey shrink/grow/run assets, hides itself after post-load, and owns two raw clone-control hooks that find `CLONE1..CLONE7` by object tag when `SCENE == 0x5a`.

## Field Map

Offsets below are byte offsets from the outer `C3DHumphrey` allocation pointer used by the constructor and raw hooks, unless marked inherited. Inherited `C3DEnemy`/`C3DPickupType`/`C3DAI` fields are documented on those base specs.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x430` | char buffer/string | `TaskName` | `CLocalGameObject`; `.gam` `3HUM`; shared slot `00419aa0` | Rows use `"clone"` and `"scene"`. Humphrey's concrete raw hooks directly read global `SCENE` instead of this field. |
| inherited `0x644` | float | `VisibleRange` | `C3DAI`; `.gam` `3HUM` | Current rows use `825.0..2500.0`. |
| inherited `0x648` | char buffer/string | `PatrolPoint` | `C3DAI`; `.gam` `3HUM` | Current rows use `"GETOUT1"` or `"none"`. |
| inherited `0x6ac` | char buffer/string | `TargetName` | `C3DAI`; `.gam` `3HUM` | Current rows target `"JIM1"`. |
| inherited `0x80c` | float | `FOV` | `C3DAI`; `.gam` `3HUM` | Current rows use `90` and `359`. |
| inherited `0x87c` | int | `AIState` | `C3DAI`; `.gam` `3HUM` | Current rows use `1..6`. |
| inherited `0x89c` | float | `WanderRange` | `C3DAI`; `.gam` `3HUM` | Current rows use `1500.0..2000.0`. |
| `0x57c` | handle/pointer | `humphrey_texture_canvas_handle` | `00420950` | Passed to the inherited material/canvas slot after loading `humphrey.png`. Exact owner type is unresolved. |
| `0x7f8` | char buffer/string | `humphrey_stop_anim_0` | ctor `00420730` | Constructor copies `STOP`. |
| `0x820` | char buffer/string | `humphrey_walk_or_default_anim` | ctor `00420730` | Constructor copies the inherited/default string at `004eca54`; semantic name unresolved. |
| `0x870` | char buffer/string | `humphrey_stop_anim_1` | ctor `00420730` | Constructor copies `STOP`. |
| `0x898` | char buffer/string | `humphrey_attack_anim` | ctor `00420730` | Constructor copies `ATTACK`. |
| `0x908` | byte | `humphrey_runtime_flag_0` | ctor `00420730` | Constructor clears this flag. Exact meaning unresolved. |
| `0x958` | byte | `humphrey_runtime_flag_1` | ctor `00420730` | Constructor sets this flag to `1`. Exact meaning unresolved. |
| `0xc8` | byte | `humphrey_base_flag` | ctor `00420730` | Constructor sets this inherited/base flag to `1`. |

No one-time `assets_registered` byte was observed. Humphrey registers its ASE/PNG assets directly in slot 7.

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `00420950` | `InitObjectHumphrey` | Traces `InitObject()`, calls `C3DPickupType::InitObject`, runs an inherited adjusted setup slot at `0x108`, registers Humphrey animations, loads `humphrey.png`, applies a `20.0` inherited shape/range constant, and selects `STOP`. | non-trivial |
| 10 | `00407eb0` | `ResetAI` | Inherited `C3DAI` reset helper. | inherited |
| 241 | `00408000` | `UpdateAI` | Inherited `C3DAI` update. | inherited |
| 259 | `00420d60` | `PostLoadHideHumphrey` | Calls `C3DPickupType::PostLoadAI`, reads `SCENE`, then hides/disables Humphrey through the inherited slot at `0x110(false)`. | non-trivial |
| 264 | `00419aa0` | `RefreshTaskStateShared` | Shared helper currently owned by `C3DArrow`. Calls a common no-op hook, reads the task state for this object's inherited task name, and writes the same value back. | inherited/shared |
| 265 | `00420a60` | `ActivateHumphreyClonesScene90` | Raw helper. Runs `C3DAnimated` slot 265, then if `SCENE == 0x5a`, finds `CLONE1..CLONE7`, shows each clone, and calls clone slot `0x124(true)`. | raw block |
| vtable 3 slot 2 | `00420890` | scalar deleting destructor | Runs the Humphrey cleanup/vtable reset helper, destroys the adjusted streamer/string subobject, and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 67 | `00472970` | `CGameObject::vfunc_00_013` | Humphrey does not override the separate asset-registration slot used by some animated leaves; assets are registered from slot 7 directly. | inherited |
| vtable 4 slot 90 | `00420bf0` | `PrepareHumphreyClonesScene90` | Raw helper. If `SCENE == 0x5a`, finds `CLONE1..CLONE7`, shows each clone, and calls clone slot `0x124(false)`. | raw block |

## Runtime Behavior

Humphrey inherits movement, patrol, pickup/enemy behavior, and most update logic from `C3DEnemy`/`C3DPickupType`/`C3DAI`. The concrete leaf mainly supplies assets and controls cloned Humphrey objects during scene state `0x5a`.

```c
C3DHumphrey::PostLoadHideHumphrey():
    C3DPickupType::PostLoadAI()
    get_task_state("SCENE")   // value is read but not branched on here
    inherited_enable_or_visibility_slot(false)
```

```c
C3DHumphrey::ActivateHumphreyClonesScene90(arg):
    C3DAnimated::ApplyLevelGate(arg)
    if get_task_state("SCENE") != 0x5a:
        return

    for name in ["CLONE1", "CLONE2", "CLONE3", "CLONE4", "CLONE5", "CLONE6", "CLONE7"]:
        clone = find_object(name)
        if clone:
            clone.inherited_enable_or_visibility_slot(true)
            clone.slot_0x124(true)
```

```c
C3DHumphrey::PrepareHumphreyClonesScene90():
    if get_task_state("SCENE") != 0x5a:
        return

    for name in ["CLONE1", "CLONE2", "CLONE3", "CLONE4", "CLONE5", "CLONE6", "CLONE7"]:
        clone = find_object(name)
        if clone:
            clone.inherited_enable_or_visibility_slot(true)
            clone.slot_0x124(false)
```

The only difference between the two clone loops is the boolean passed to clone slot `0x124`.

## Constants And Wiring

### `.gam` Placeable Properties

`3HUM` appears 10 times across the level `.gam` files. It serializes common object/animated fields plus inherited `C3DAI` fields. No friend talk properties are present.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | `"C3DHUMPHREY"`, `"clone1"`, `"clone2"`, `"clone4"`, ... | Base object tag and lookup identity. Runtime clone lookup uses uppercase `CLONE1..CLONE7` strings. |
| `RotateToDest` | flag4 | inherited | `01010101` | Base movement/rotation flags. |
| `ObjectID` | int | inherited | `860378445` | FourCC/object id value for `3HUM`. |
| `PositionX` | float | inherited | `-6180..8060` | Base placement transform. |
| `PositionY` | float | inherited | `-6.77..922` | Base placement transform. |
| `PositionZ` | float | inherited | `-5270..6510` | Base placement transform. |
| `RotationX` | float | inherited | `0` | Base placement transform. |
| `RotationY` | float | inherited | `0..220` | Base placement transform and initial facing. |
| `RotationZ` | float | inherited | `0` | Base placement transform. |
| `TaskName` | str | inherited `0x430` | `"clone"`, `"scene"` | Shared slot 264 refreshes this task; concrete clone hooks read global `SCENE`. |
| `Debug` | int | inherited | `0` | Base debug flag. |
| `RequiredLevel` | int | inherited | `0..260` | Inherited animated/progress lower gate. |
| `ExactLevel` | int | inherited | `-1` | Inherited animated/progress exact gate. |
| `RemoveLevel` | int | inherited | `-1..300` | Inherited animated/progress upper gate. |
| `HasCollision` | int | inherited | `1` | Inherited collision toggle. |
| `InitiallyVisible` | int | inherited | `-1` | Inherited initial visibility. |
| `CanMove` | int | inherited | `1` | Inherited transform/update gate. |
| `SecondPass` | int | inherited | `0` | Inherited render/update pass flag. |
| `PickupLink` | str | inherited | `"none"` | Inherited animated link field; seven Humphrey rows include this serialized property. |
| `PatrolPoint` | str | inherited `0x648` | `"GETOUT1"`, `"none"` | Resolved by inherited `C3DAI` patrol logic. |
| `VisibleRange` | float | inherited `0x644` | `825..2500` | Compared by inherited `C3DAI` target/range logic. |
| `FOV` | float | inherited `0x80c` | `90`, `359` | Used by inherited AI facing/visibility helpers. |
| `TargetName` | str | inherited `0x6ac` | `"JIM1"` | Resolved by inherited `C3DAI::PostLoadAI`. |
| `AIState` | int | inherited `0x87c` | `1..6` | Copied into inherited runtime AI state. |
| `WanderRange` | float | inherited `0x89c` | `1500..2000` | Used by inherited AI wander/search helpers. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3HUM` | Concrete placeable class id for Humphrey. | ctor `00420730`; `push 0x3348554d` at `0042084b` |
| `C3DHUMPHREY` | Concrete object/type string. | string `.data:004eed24`; constructor string path |
| `C3DHUMPHREY()` | Concrete class string. | string `.data:004eed14`; constructor string path |
| `SCENE` | Task-state key used by post-load and clone hooks. | `00420a60`, `00420bf0`, `00420d60` |
| `0x5a` | Scene state that activates/prepares Humphrey clones. | raw `00420a60`, `00420bf0` |
| `CLONE1..CLONE7` | Object tags looked up by the raw clone hooks. | strings `.data:004eedec..004eee1c` |
| `HIATTACK`, `HIWALK`, `HISHRINK`, `HIRUNSHRUNK`, `HIGROW`, `HISTOP`, `HISTOP2` | Humphrey animation aliases registered during init. | `00420950` |
| `ATTACK`, `STOP` | Constructor/default animation strings. | ctor `00420730`; strings `.data:004ee550`, `004ed040` |
| `20.0` | Shape/range constant applied during init through an inherited adjusted slot. | `00420950`; immediate `0x41a00000` |
| `0.6` | Constructor applies this float through `CGameObject` slot 64. | ctor `00420730`; immediate `0x3f19999a` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| texture | `humphrey.png` | `00420950`; string `.data:004eed44` | Loaded during Humphrey init and passed to inherited canvas/material setup. |
| animation | `HIATTACK` -> `humprun.ase` | `00420950` | Attack/run animation. |
| animation | `HIWALK` -> `humpwalk.ase` | `00420950` | Walk animation. |
| animation | `HISHRINK` -> `humpshrink.ASE` | `00420950` | Shrink animation. |
| animation | `HIRUNSHRUNK` -> `humprunshrunk.ASE` | `00420950` | Run while shrunk. |
| animation | `HIGROW` -> `humpgrow.ASE` | `00420950` | Grow animation. |
| animation | `HISTOP` -> `humpsleep.ase` | `00420950` | Stop/sleep animation. |
| animation | `HISTOP2` -> `humpstop.ase` | `00420950` | Alternate stop animation. |
| animation default | `STOP` | `00420950`; string `.data:004ed040` | Selected after texture/canvas setup. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, objdump over `/home/scotty/xp-jnbg-original/Neutron.exe`, asset scan, and `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Create proper Ghidra functions for raw targets `00420a60` and `00420bf0`; current Ghidra does not own those boundaries cleanly.
- Name the clone slot at offset `0x124`, which receives `true` from slot 265 and `false` from vtable-4 slot 90.
- Resolve constructor flags at `0x908`, `0x958`, and `0xc8`, plus animation string fields at `0x7f8`, `0x820`, `0x870`, and `0x898`.
- Runtime-check the `SCENE == 0x5a` clone sequence before marking the class `validated`.

## Notes

- Evidence: `DumpClass.java C3DHumphrey /tmp/decomp_C3DHumphrey.md` (`slots=391`, `owned_methods=2`, `offsets=0`), `DumpFunctions.java /tmp/decomp_C3DHumphrey_funcs.md`, local `objdump` windows over `00420730..00420de8`, and `.gam` schema for `3HUM`.
- `3HUM -> C3DHumphrey` was backfilled in `docs/_gam_classids.tsv` from RTTI/class dump evidence during this spec, then `python3 tools/gam_schema.py` regenerated `docs/gam_schema.md`.
- `assets/ase/humpsleeplook.ASE` exists on disk, but no direct `C3DHumphrey` string/reference was found in this class's asset-registration path.
