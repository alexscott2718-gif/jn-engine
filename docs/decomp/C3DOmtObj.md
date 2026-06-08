# C3DOmtObj

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DOmtObj` |
| Base chain | `C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004ac3a4`, `004ac3b4`, `004ac804`, `004ac840`, `004ac854` |
| Ctor(s) | FourCC factory/constructor at `00434530` for `3OMT`; adjusted cleanup helper at `00434700` |
| Dtor(s) | adjusted scalar deleting destructor at `004346d0`; destructor thunks at `00434830`, `00434840`, `00434850` |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

Offsets below are byte offsets from the primary `C3DAnimated` gameplay pointer used by slot-1 methods and the property registrar. The constructor is entered with the outer allocation pointer and stores these fields at `outer + 0xc0 + offset`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x034` | float | `Radius` | constructor property registration; `.gam` `3OMT` | Collision/selection radius passed through the inherited object radius field. Constructor default is `5.0`. |
| inherited `0x578` | int | `RequiredLevel` | `C3DAnimated`; `.gam` `3OMT` | Level/progress lower gate. |
| inherited `0x57c` | int | `ExactLevel` | `C3DAnimated`; `.gam` `3OMT` | Exact level/progress gate. |
| inherited `0x580` | int | `RemoveLevel` | `C3DAnimated`; `.gam` `3OMT` | Level/progress upper gate. |
| inherited `0x584` | int | `HasCollision` | `C3DAnimated`; slot 259 | If zero, slot 259 disables several inherited collision/interaction hooks after shape load. |
| inherited `0x588` | int | `InitiallyVisible` | `C3DAnimated`; `.gam` `3OMT` | Initial visibility toggle applied by inherited post-load flags. |
| inherited `0x58c` | int | `CanMove` | `C3DAnimated`; `.gam` `3OMT` | Transform update gate. |
| inherited `0x590` | int | `SecondPass` | `C3DAnimated`; slot 259 | If one, slot 259 applies an inherited second-pass/material setup hook before inherited post-load flags. |
| inherited `0x595` | char buffer/string | `PickupLink` | `C3DAnimated`; `.gam` `3OMT` | Lazy object link used by animated objects. |
| `0x5fc` | char buffer/string | `OmtDatabase` | constructor property registration; slot 259; `.gam` `3OMT` | OMT database filename. Constructor default is `"objects.omt"`. |
| `0x660` | int | `OmtIndex` | constructor property registration; slot 259; `.gam` `3OMT` | Index of the `3DSh` shape inside `OmtDatabase`. Constructor default is `4`. |
| `0x4a8` | pointer | `omt_database_handle` | slot 259 | Runtime handle returned by `FUN_0046a910(OmtDatabase)`. Stored before the `3DSh` lookup. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `00434530` | `CtorOmtObj3OMT` | Constructs `C3DAnimated`, installs five adjusted vftables, registers class strings `C3DOMTOBJ`/`C3DOMTOBJ()`, runs inherited object initialization, registers `OmtDatabase`, `OmtIndex`, and `Radius`, binds FourCC `3OMT`, and seeds defaults (`OmtDatabase="objects.omt"`, `OmtIndex=4`, radius `5.0`). | non-trivial |
| 259 | `00434750` | `LoadOmtShapeAndApplyFlags` | Resolves `OmtDatabase` through `FUN_0046a910`, stores the database handle, loads a `3DSh` shape by `OmtIndex` through `FUN_00477ba0`, assigns it to the adjusted outer object, normalizes material state through `FUN_00477550`, applies `SecondPass`, disables inherited collision/interaction hooks when `HasCollision == 0`, then runs `C3DAnimated::ApplyInitialAnimatedFlags`. | non-trivial |
| vtable 3 slot 2 | `004346d0` | `ScalarDeletingDestructor` | Runs cleanup helper `00434700`, destroys the adjusted `OMediaClassStreamer` subobject, and frees the adjusted allocation when the delete flag is set. | non-trivial |

## Per-Frame Behavior

`C3DOmtObj` does not add a per-frame update loop. It inherits `C3DAnimated::UpdateAnimated`; the class-owned logic is the post-load mesh binding path:

```c
C3DOmtObj::LoadOmtShapeAndApplyFlags():
    omt_database_handle = OMT_LoadByName(OmtDatabase)
    shape = OMT_Get3DShape(omt_database_handle, OmtIndex)
    set_inherited_shape(shape)
    normalize_shape_material_state(current_shape())

    if SecondPass == 1:
        apply_inherited_second_pass_material_mode()

    if HasCollision == 0:
        disable_inherited_collision_and_interaction_hooks()

    C3DAnimated::ApplyInitialAnimatedFlags()
```

The `OMT_LoadByName` and `OMT_Get3DShape` names are descriptive aliases used in older project notes for `FUN_0046a910` and `FUN_00477ba0`; the executable still needs final symbol names.

## Constants And Wiring

`C3DOmtObj` maps to placeable FourCC `3OMT` (`FUN_00434530`). The current corpus has 20 `3OMT` instances. `docs/gam_schema.md` already reports `C3DOMTOBJ` as the dominant object tag/class string for the row; the top FourCC map still needs the Phase 0.6 class-id backfill.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str | inherited | 20 samples including `"C3DOMTOBJ"`, `"beam"`, `"bench01"`, `"bench02"` | Common object lookup and trigger/tag wiring. |
| `PositionX`, `PositionY`, `PositionZ` | float (`3`) | inherited transform | X `-2320..11200`, Y `-585..1480`, Z `-2880..8400` | Consumed by `C3DObject::InitPhysics3D`. |
| `RotationX`, `RotationY`, `RotationZ` | float (`3`) | inherited transform | X `0..30`, Y `0..300`, Z `0` | Consumed by inherited OMedia transform bridge. |
| `RequiredLevel`, `ExactLevel`, `RemoveLevel` | inherited animated group | inherited | `RequiredLevel=-1..410`, `ExactLevel=-1`, `RemoveLevel=-1..400` | Consumed by `C3DAnimated::ApplyLevelGate`. |
| `HasCollision`, `InitiallyVisible`, `CanMove`, `SecondPass`, `PickupLink` | inherited animated group | inherited | `HasCollision=-1..0`, `InitiallyVisible=-1..0`, `CanMove=0..1`, `SecondPass=0..1`, `PickupLink` mostly `"none"` | Consumed by slot 259 and inherited animated gates. |
| `OmtDatabase` | str (`1`) | `0x5fc` | all `"objects.omt"` | Resolved by `FUN_0046a910` before shape lookup. |
| `OmtIndex` | int (`6`) | `0x660` | `6..29` | Passed to `FUN_00477ba0` to fetch a `3DSh` shape. |
| `Radius` | float (`3`) | inherited `0x034` | `117..713` | Registered here; collision/selection consumer is inherited. |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `objects.omt` | constructor default; `.gam` `3OMT` | Database file opened by `FUN_0046a910`. |
| 3D shape | `3DSh` at `OmtIndex` | slot 259 | `FUN_00477ba0` looks up the shape object and returns the assigned OMedia shape pointer. |
| material state | current shape materials | `FUN_00477550` | Iterates shape material slots and writes default material state before inherited flags apply. |

## Confidence

Confidence: Medium

Validation: Static Ghidra + local `objdump` disassembly + `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Name `FUN_0046a910`, `FUN_00477ba0`, and `FUN_00477550` in the Ghidra project.
- Backfill `docs/_gam_classids.tsv` for `3OMT` and regenerate `docs/gam_schema.md` so the top FourCC table names `C3DOmtObj`.
- Name the inherited outer-object slots used for shape assignment, current-shape retrieval, second-pass setup, and collision/interaction disabling.
- Resolve the exact owner of the inherited `Radius` field at active offset `0x034`.
- Apply real `C3DOmtObj` structs so Ghidra stops printing seed offsets like `this[0x17f]`.

## Notes

- Evidence: `DumpClass.java C3DOmtObj /tmp/decomp_C3DOmtObj.md` (`slots=368`, `owned_methods=1`, `offsets=1`).
- Local disassembly over `/home/scotty/xp-jnbg-original/Neutron.exe` confirms constructor defaults and property registration at `00434530..004346b8`, and shape-load behavior at `00434750..004347f8`.
- Older project notes already use `OMT_LoadByName` for `FUN_0046a910` and `OMT_Get3DShape` for `FUN_00477ba0`; this spec keeps those as descriptive aliases, not final recovered symbols.
