# C3DTank

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DTank` |
| Base chain | `C3DCar -> C3DVehicle -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004b9908`, `004b9918`, `004b9d68`, `004b9da4`, `004b9db8` |
| Ctor(s) | constructor/factory block `004450f0`; registers FourCC `3TAN` at `004451b5` |
| Dtor(s) | scalar deleting destructor at `00445210`; cleanup helper `00445240`; adjusted destructor thunks `00445360`, `00445370`, `00445380` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DTank` is a thin `C3DCar`/`C3DVehicle` leaf. The current `.gam` corpus has no `3TAN` rows, so the class appears to be code-spawned, unused by shipped level placement, or used through a database path outside `.gam`. Its owned behavior is mostly construction and visual/vehicle tuning: it binds vehicle database entry `3`, applies tank-specific dimensions/tuning, and otherwise inherits car and vehicle behavior.

## Field Map

`C3DTank` introduces no confirmed serialized fields. Offsets below are inherited fields that the constructor seeds directly.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited active/outer | mixed | `C3DCar` and `C3DVehicle` state blocks | base constructors `00412870`, `00465030` | Tank inherits the vehicle input, wheel/part, speed, steering, and camera fields documented in `C3DVehicle` and `C3DCar`. |
| outer `0x628` | float | `tank_vehicle_tuning_200` | ctor `004450f0` | Constructor seeds `200.0`. Exact base-field name still unresolved. |
| outer `0x650` | int/bool | `tank_flag_0x650` | ctor `004450f0` | Constructor sets to `1`. Exact base-field name still unresolved. |
| outer `0x69c` | int/bool | `tank_runtime_flag_0x69c` | ctor `004450f0` | Constructor clears to `0`. Exact consumer unresolved. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `004450f0` | `CtorTank3TAN` | Constructs `C3DCar`, installs tank vtables, registers object/type strings `C3DTANK` and `C3DTank()`, runs `InitObjectTank`, registers FourCC `3TAN`, applies one inherited tuning value `150.0`, seeds three inherited fields, and finalizes. | non-trivial |
| 7 | `00445290` | `InitObjectTank` | Runs `C3DVehicle::InitObject`, fetches vehicle database entry `3` from `DAT_005099c4`, binds it through the inherited shape/model slot, applies tank-specific dimension/tuning constants, applies active tuning value `515.0`, calls an inherited setup hook, and finalizes through the base object slot. | non-trivial |
| 10 | `00412ff0` | `C3DCar::PostLoadOrReset` | Inherited car behavior. | inherited |
| 16 | `00470a90` | `CGameObject::Touch` | Inherited base touch handling; no tank-specific collision override found. | inherited |
| 241 | `00412ed0` | `C3DCar::UpdateCar` | Inherited car update. | inherited |
| 243 | `00445320` | `TankVehicleHelperThunk` | Small tank-local wrapper that forwards its argument to vehicle helper `00465830`. Exact slot name and behavior remain inherited/unknown. | raw thunk |
| 248 | `00467060` | `C3DVehicle` helper | Inherited vehicle behavior. | inherited |
| vtable 3 slot 2 | `00445210` | scalar deleting destructor | Runs cleanup/vtable reset logic and frees the adjusted allocation when requested. | non-trivial |

## Per-Frame Behavior

`C3DTank` does not own a distinct per-frame integrator. It uses `C3DCar::UpdateCar` and the inherited `C3DVehicle` control/wheel helpers.

```c
C3DTank::InitObjectTank():
    C3DVehicle::InitObject()
    model = lookup_database_entry(vehicle_database_handle, 3)
    bind_shape_or_model(model)
    apply_tank_dimension_block(159.0, -130.0, 80.0, 80.0, 14.0, true, true)
    apply_active_vehicle_tuning(515.0)
    inherited_setup_hook()
```

```c
C3DTank::CtorTank3TAN():
    C3DCar::Ctor()
    install_tank_vtables()
    set_object_tag("C3DTANK")
    trace_or_register_ctor_string("C3DTank()")
    InitObjectTank()
    register_fourcc("3TAN")
    inherited_slot_value(150.0)
    tank_runtime_flag_0x69c = 0
    tank_flag_0x650 = 1
    tank_vehicle_tuning_200 = 200.0
```

## Constants And Wiring

`C3DTank` registers `3TAN`, but `docs/gam_schema.md` has no `3TAN` rows in the 35 shipped `.gam` files. No placeable property table is available for this class.

| Name / Id | Use | Evidence |
|---|---|---|
| `3TAN` | Concrete class id for Tank. | ctor `004450f0`; `push 0x3354414e` at `004451b5`; `docs/_gam_classids.tsv` |
| `C3DTANK` | Concrete object/type string. | string `.data:004ef334`; constructor path |
| `C3DTank()` | Constructor trace/RTTI-style string. | string `.data:004f1098`; constructor path |
| `DAT_005099c4` | Vehicle database handle used by tank init. | `CGameType` database handle range; `InitObjectTank` passes it to `FUN_00477ba0` |
| database entry `3` | Tank visual/model entry. | `InitObjectTank`; `FUN_00477ba0(DAT_005099c4, 3)` |
| `159.0`, `-130.0`, `80.0`, `80.0`, `14.0`, `1`, `1` | Tank dimension/tuning block. | `InitObjectTank`; inherited outer slot `0x168` |
| `515.0` | Active tuning value applied after model binding. | `InitObjectTank`; active slot `0x108` |
| `150.0` | Constructor tuning value applied after `3TAN` registration. | ctor `004450f0`; active slot at `00471240` |
| `200.0` | Constructor-seeded inherited field at outer `0x628`. | ctor `004450f0` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| database entry | vehicle database index `3` | `FUN_00477ba0(DAT_005099c4, 3)` | This is the only directly proven tank visual binding. The exact source database filename is inherited from `CGameType` vehicle database setup and still needs final naming. |
| parsed asset candidates | `assets/parsed/level2a` index `14` name `tank`; `assets/parsed/level3c` index `24` name `tank` | repo asset metadata | These local parsed names are useful search leads, but the executable path here does not reference level2a/level3c directly, so they are not proven as the vehicle database entry. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, local disassembly of constructor/init/thunks, `C3DVehicle` spec cross-check, class-id scan, and local asset metadata only; not runtime-validated.

Open questions:
- Name inherited tank-touched fields at outer `0x628`, `0x650`, and `0x69c`.
- Name inherited slot `0x168` and the semantic meaning of the dimension/tuning tuple.
- Identify the concrete vehicle database filename backing `DAT_005099c4` and map database entry `3` to a parsed asset.
- Determine whether `3TAN` is code-spawned in any shipped mode, unused leftover code, or reached by non-`.gam` data.

## Notes

- Evidence: `DumpClass.java C3DTank /tmp/decomp_C3DTank.md` (`slots=389`, `owned_methods=1`, `offsets=0`), local objdump windows over `004450f0..004453b0`, string scans around `004ef334` and `004f1098`, `docs/_gam_classids.tsv`, and `docs/decomp/C3DVehicle.md`.
- No `docs/gam_schema.md` regeneration was needed for this spec because `3TAN` has no current `.gam` rows and was already named in `_gam_classids.tsv`.
