# C3DWheel

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DWheel` |
| Base chain | `C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004d58ac`, `004d58bc`, `004d5d0c`, `004d5d48`, `004d5d5c` |
| Ctor(s) | constructor/factory block `00468020`; class-id registrar `3WHE` in slot 7 at `00468247` |
| Dtor(s) | scalar deleting destructor thunk at `00468150`; cleanup helper at `00468180` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DWheel` is the code-spawned wheel child used by the vehicle branch. `C3DCar` creates it through `FUN_00459cb0("3WHE")`, stores it in inherited `C3DVehicle::wheel_parts`, and drives its transform through the vehicle wheel solver. `C3DWheel` itself is a thin `C3DObject` leaf: it registers `3WHE`, binds a vehicle-database visual by caller-supplied index, and otherwise inherits object update/render behavior.

## Field Map

Offsets below are byte offsets from the primary slot-1 `C3DWheel` gameplay pointer unless noted.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x49e` | byte/bool | `physics_initialized` | `CLocalGameObject`; `004682a0` | Guards one-shot physics initialization. When clear, wheel physics calls `C3DObject::InitPhysics3D`, sets the guard, and clears two inherited flags. |
| `0x4b8` | int/pointer | `wheel_runtime_field_0x4b8` | ctor `00468020` | Constructor clears this field. No confirmed consumer in the wheel-owned methods; it sits at the same primary offset used by `C3DVehicle::wheel_parts` in vehicle objects, but here belongs to the wheel allocation itself. |
| inherited `0x005` | byte/bool | `inherited_flag_0x5` | `004682a0` | Cleared after the one-shot physics setup. Exact base flag name is unresolved. |
| inherited `0x00a` | byte/bool | `inherited_flag_0xa` | `004682a0` | Cleared after the one-shot physics setup. Exact base flag name is unresolved. |

`C3DWheel` has no confirmed serialized `.gam` fields. It has no rows in the shipped `.gam` schema and is allocated by vehicle logic.

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| ctor | `00468020` | `CtorWheel3WHE` | Constructs `C3DObject`, installs all five `C3DWheel` vtables, registers strings `C3DWHEEL`/`C3DWheel()`, calls `InitObjectWheel`, clears `wheel_runtime_field_0x4b8`, clears several inherited flags through base setters, applies inherited scalar `1.0`, and finalizes. | non-trivial |
| 7 | `00468220` | `InitObjectWheel` | Traces `"InitObject()"`, runs `C3DObject::InitObject3D`, registers class id `3WHE` through the inherited class-id setter, applies inherited slot `0x1b0` with `0`, and finalizes. | non-trivial |
| 10 | `004587f0` | `CLocalGameObject::ResetObject` | Inherited reset behavior. | inherited |
| 11 | `004682a0` | `InitPhysicsWheel` | Traces `"InitPhysics()"`; if not already initialized, runs `C3DObject::InitPhysics3D`, sets inherited `physics_initialized`, clears two inherited flags, and finalizes. | non-trivial |
| 12 | `00462480` | `C3DObject::UnInitPhysics3D` | Inherited uninit physics. | inherited |
| 232 | `004682f0` | `WheelFalsePredicate` | Returns `false`. Exact slot semantic is unresolved; it overrides a `CGameObject` target. | trivial |
| 241 | `004682e0` | `UpdateWheel` | Tail wrapper to `C3DObject::Update3DObject(dt)`. | non-trivial |
| 243 | `00462650` | `C3DObject::BuildCameraOrLightRecord` | Inherited record builder. | inherited |
| vtable 3 slot 2 | `00468150` | `ScalarDeletingDestructor` | Adjusts to the primary object, runs cleanup helper `00468180`, destroys the tail `OMediaClassStreamer` subobject, and frees allocation when requested. | non-trivial |
| vtable 4 slot 54 | `00468270` | `BindWheelVehicleDatabaseEntry` | Looks up `FUN_00477ba0(DAT_005099c4, index)` and passes the resulting vehicle database entry to the inherited shape/model binding slot at vtable offset `0xac`. | non-trivial |

## Runtime Behavior

```c
C3DWheel::InitObjectWheel():
    trace("InitObject()")
    C3DObject::InitObject3D()
    register_class_id("3WHE")
    inherited_set_flag_or_index(0)
    finalize()
```

```c
C3DWheel::InitPhysicsWheel():
    trace("InitPhysics()")
    if !physics_initialized:
        C3DObject::InitPhysics3D()
        physics_initialized = true
        inherited_flag_0x5 = false
        inherited_flag_0xa = false
    finalize()
```

```c
C3DWheel::BindWheelVehicleDatabaseEntry(index):
    entry = vehicle_db_lookup(DAT_005099c4, index)
    inherited_bind_shape_or_model(entry)
```

Per-frame update is inherited. Slot 241 simply calls `C3DObject::Update3DObject(dt)`. The parent vehicle is responsible for wheel placement, spin, suspension probing, visibility forwarding, and local wheel-record updates.

## Constants And Wiring

| Name / Id | Use | Evidence |
|---|---|---|
| `3WHE` | Code-spawned wheel child class id. | `InitObjectWheel` at `00468247`; `docs/_gam_classids.tsv` |
| `C3DWHEEL` | Runtime type/object string. | constructor string reference `0x4f4eb4`; `CGameObject` has a wheel-specific transform bridge note for this string. |
| `C3DWheel()` | Constructor trace/string. | constructor string reference `0x4f4ea8` |
| `DAT_005099c4` | Vehicle database handle used for the wheel visual. | `BindWheelVehicleDatabaseEntry` at `00468270`; same database handle used by `C3DTank` visual binding. |
| class-id lookup sites | Vehicle parent creation of wheels. | `C3DCar` helpers call `FUN_00459cb0("3WHE")` at `00412b17` and `00412d20`. |
| default scalar `1.0` | Constructor-applied inherited scalar. | ctor push `0x3f800000` before inherited slot `004711e0` |

`docs/gam_schema.md` has no `3WHE` rows. The class is therefore documented as a runtime child object rather than a placeable schema class.

## Assets

`C3DWheel` does not directly load filenames in the owned methods. Its visual is selected through the vehicle database (`DAT_005099c4`) by slot 54.

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| vehicle database entry | caller-supplied index | `00468270` | Exact wheel database index is supplied by parent vehicle setup paths. |
| local asset candidate | `assets/ase/wheel.ASE` | repo asset scan and executable string scan | Likely wheel model, but not directly referenced by the inspected wheel-owned methods. |
| local texture candidate | `assets/png/wheel.png` | repo asset scan and executable string scan | Likely wheel texture, but final database index mapping remains open. |

## Confidence

Confidence: Medium

Validation: Static Ghidra dump, local `objdump` over `00468020..004682ff`, class-id scan, existing `C3DCar`/`C3DVehicle` specs, and local asset string scan only; not runtime-validated.

Open questions:
- Name the inherited flags cleared at offsets `0x5` and `0xa` during wheel physics setup.
- Identify the concrete vehicle database index used for standard wheels and map it to `wheel.ASE` / `wheel.png`.
- Repair Ghidra function boundaries for raw targets `00468270`, `004682e0`, and `004682f0` so their slot names can be confirmed from decompiler output.
- Finish the `CGameObject` wheel-specific transform bridge (`004740c0`) to explain why it special-cases `C3DWHEEL`.

## Notes

- Evidence: `DumpClass.java C3DWheel /tmp/decomp_C3DWheel.md` (`slots=351`, `owned_methods=2`, `offsets=0`).
- Raw evidence: `DumpFunctions.java /tmp/decomp_C3DWheel_raw.md 00468020 00468220 004682a0` plus local objdump for `00468020..004682ff`.
- No `tools/gam_schema.py` regeneration was needed because `3WHE` has no `.gam` rows and was already named in `_gam_classids.tsv`.
