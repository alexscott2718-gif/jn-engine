# C3DAIOmtObj

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DAIOmtObj` |
| Base chain | `C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `0048ec40`, `0048ec50`, `0048f0a0`, `0048f0dc`, `0048f0f0` |
| Ctor(s) | FourCC factory/constructor at `0040ae30` for `3AIO`; adjusted cleanup helper at `0040b020` |
| Dtor(s) | adjusted scalar deleting destructor at `0040aff0`; destructor thunks at `0040b160`, `0040b170`, `0040b180` |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

Offsets below are byte offsets from the primary `C3DAI` gameplay pointer used by slot-1 methods and the property registrar. The constructor is entered with the outer allocation pointer and stores these fields at `outer + 0xc0 + offset`.

`C3DAIOmtObj` is the AI-bearing sibling of `C3DOmtObj`, not a subclass of it. It introduces a similar OMT shape field block at different offsets after the `C3DAI` state block.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x034` | float | `Radius` | constructor property registration; slot 259; `.gam` `3AIO` | Collision/selection radius. Constructor default is `5.0`; slot 259 forwards it through the inherited radius/state setter at vtable offset `0x110`. |
| inherited `0x578..0x595` | mixed | `C3DAnimated` gate fields | `C3DAnimated`; `.gam` `3AIO` | Required/exact/remove level, collision, initial visibility, movement, second-pass, and pickup-link fields. |
| inherited `0x600..0x89c` | mixed | `C3DAI` state block | `C3DAI`; `.gam` `3AIO` | Target, patrol, AI state, FOV, visible range, wander range, and animation-state fields. |
| `0x848` | byte/bool | `terrain_collision_disabled_flag` | slot 259 | Set to `1` when `TerrainColl == 0` before disabling inherited terrain/collision hooks. Exact owner is unresolved. |
| `0x8d4` | char buffer/string | `OmtDatabase` | constructor property registration; slot 259; `.gam` `3AIO` | OMT database filename. Constructor default is `"objects.omt"`. |
| `0x938` | int | `OmtIndex` | constructor property registration; slot 259; `.gam` `3AIO` | Index of the `3DSh` shape inside `OmtDatabase`. Constructor default is `4`. |
| `0x93c` | int | `TerrainColl` | constructor property registration; slot 259; `.gam` `3AIO` | Terrain/collision mode. Constructor default is `-1`; current rows use `-1` or `0`; code also handles `1`. |
| `0x4a8` | pointer | `omt_database_handle` | slot 259 | Runtime handle returned by `FUN_0046a910(OmtDatabase)`. Stored before the `3DSh` lookup. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `0040ae30` | `CtorAIOmtObj3AIO` | Constructs `C3DAI`, installs five adjusted vftables, registers class strings `C3DAIOMTOBJ`/`C3DAIOMTOBJ()`, runs `C3DAI::InitObjectAI`, registers `OmtDatabase`, `OmtIndex`, `Radius`, and `TerrainColl`, binds FourCC `3AIO`, and seeds defaults (`OmtDatabase="objects.omt"`, `OmtIndex=4`, `TerrainColl=-1`, radius `5.0`). | non-trivial |
| 259 | `0040b070` | `LoadAIOmtShapeAndCollision` | Runs `C3DAnimated::ApplyInitialAnimatedFlags`, resolves `OmtDatabase` through `FUN_0046a910`, loads a `3DSh` shape by `OmtIndex` through `FUN_00477ba0`, assigns it to the adjusted outer object, normalizes material state through `FUN_00477550`, forwards `Radius` through an inherited setter, and applies `TerrainColl` to inherited collision hooks. It does not call `C3DAI::PostLoadAI`. | non-trivial |
| vtable 3 slot 2 | `0040aff0` | `ScalarDeletingDestructor` | Runs cleanup helper `0040b020`, destroys the adjusted `OMediaClassStreamer` subobject, and frees the adjusted allocation when the delete flag is set. | non-trivial |

## Per-Frame Behavior

`C3DAIOmtObj` inherits the `C3DAI` update loop, but its owned post-load path skips the normal `C3DAI::PostLoadAI` target-resolution setup. Static evidence therefore says the AI fields are registered and serialized, while the exact runtime setup path for `target_object` and `current_state` remains unresolved for this class.

```c
C3DAIOmtObj::LoadAIOmtShapeAndCollision():
    C3DAnimated::ApplyInitialAnimatedFlags()

    omt_database_handle = OMT_LoadByName(OmtDatabase)
    shape = OMT_Get3DShape(omt_database_handle, OmtIndex)
    set_inherited_shape(shape)
    normalize_shape_material_state(current_shape())
    inherited_set_radius_or_state(Radius)

    if TerrainColl == 0:
        terrain_collision_disabled_flag = true
        set_inherited_collision_hook_a(false)
        set_inherited_collision_hook_b(false)
    else if TerrainColl == 1:
        set_inherited_collision_hook_a(true)
        set_inherited_collision_hook_b(true)
```

The `OMT_LoadByName` and `OMT_Get3DShape` names are descriptive aliases used in older project notes for `FUN_0046a910` and `FUN_00477ba0`; the executable still needs final symbol names.

## Constants And Wiring

`C3DAIOmtObj` maps to placeable FourCC `3AIO` (`FUN_0040ae30`). The current corpus has 7 `3AIO` instances. `docs/gam_schema.md` now names the top FourCC map row as `C3DAIOmtObj`; the per-row object tags use the original uppercase `C3DAIOMTOBJ` string.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | 7 samples including `"C3DAIOMTOBJ"`, `"crashpod"`, `"friedeggs"`, `"pod"` | Common object lookup and trigger/tag wiring. |
| `PositionX`, `PositionY`, `PositionZ` | float (`3`) | inherited transform | X `-15500..15800`, Y `-12.8..2630`, Z `-4580..13200` | Consumed by `C3DObject::InitPhysics3D`. |
| `RotationX`, `RotationY`, `RotationZ` | float (`3`) | inherited transform | X `0..359`, Y `0..270`, Z `0..0.5` | Consumed by inherited OMedia transform bridge. |
| `RequiredLevel`, `ExactLevel`, `RemoveLevel` | inherited animated group | inherited | `RequiredLevel=-1..190`, `ExactLevel=-1`, `RemoveLevel=-1..320` | Consumed by `C3DAnimated::ApplyLevelGate`. |
| `HasCollision`, `InitiallyVisible`, `CanMove`, `SecondPass`, `PickupLink` | inherited animated group | inherited | `HasCollision=0..1`, `InitiallyVisible=-1..0`, `CanMove=0..1`, `SecondPass=0`, `PickupLink` mostly `"none"` | Consumed by inherited animated gates; `HasCollision` is not directly checked by this owned slot. |
| `PatrolPoint`, `VisibleRange`, `FOV`, `TargetName`, `AIState`, `WanderRange` | inherited AI group | inherited | `PatrolPoint` examples `"fallpod01"`, `"none"`, `"pod01"`, `"wop04"`; `VisibleRange=2500`, `FOV=90`, `TargetName` `"JIM1"`/`"none"`, `AIState=1..3`, `WanderRange=1500` | Registered by `C3DAI::InitObjectAI`; normal post-load consumption is skipped by this class-owned slot, so runtime resolution needs validation. |
| `OmtDatabase` | str (`1`) | `0x8d4` | all `"objects.omt"` | Resolved by `FUN_0046a910` before shape lookup. |
| `OmtIndex` | int (`6`) | `0x938` | `6..30` | Passed to `FUN_00477ba0` to fetch a `3DSh` shape. |
| `Radius` | float (`3`) | inherited `0x034` | `17.4..1100` | Forwarded through an inherited radius/state setter in slot 259. |
| `TerrainColl` | int (`6`) | `0x93c` | `-1..0` in current rows | `0` disables inherited terrain/collision hooks; code also handles `1` as an explicit enable mode. |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `objects.omt` | constructor default; `.gam` `3AIO` | Database file opened by `FUN_0046a910`. |
| 3D shape | `3DSh` at `OmtIndex` | slot 259 | `FUN_00477ba0` looks up the shape object and returns the assigned OMedia shape pointer. |
| material state | current shape materials | `FUN_00477550` | Iterates shape material slots and writes default material state before collision flags apply. |

## Confidence

Confidence: Medium

Validation: Static Ghidra + local `objdump` disassembly + `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Confirm whether `C3DAIOmtObj` intentionally bypasses `C3DAI::PostLoadAI`, and how target/state caches are initialized at runtime.
- Name `FUN_0046a910`, `FUN_00477ba0`, and `FUN_00477550` in the Ghidra project.
- Name the inherited radius/state setter at vtable offset `0x110` and collision hooks at offsets `0xa8` and `0xa0`.
- Resolve the exact owner of `terrain_collision_disabled_flag` at active offset `0x848`.
- Apply real `C3DAIOmtObj` structs so Ghidra stops printing seed offsets like `this[0x235]`.

## Notes

- Evidence: `DumpClass.java C3DAIOmtObj /tmp/decomp_C3DAIOmtObj.md` (`slots=391`, `owned_methods=1`, `offsets=1`).
- Local disassembly over `/home/scotty/xp-jnbg-original/Neutron.exe` confirms constructor defaults and property registration at `0040ae30..0040afc7`, and shape/collision behavior at `0040b070..0040b120`.
- The direct OMT field block starts at active offset `0x8d4`; this overlaps `C3DPickupType` sibling fields only in the broad C3DAI-derived layout, not within a single class inheritance path.
