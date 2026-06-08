# C3DCar

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DCar` |
| Base chain | `C3DVehicle -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `00494cd8`, `00494ce8`, `00495138`, `00495174`, `00495188` |
| Ctor(s) | constructor/factory block `00412870`; duplicate class-id registrar `3CAR` at `0041294c` |
| Dtor(s) | scalar deleting destructor thunk at `004129b0`; cleanup helper at `004129e0` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DCar` is the vehicle-car branch between `C3DVehicle` and concrete leaves such as `C3DJeep` and `C3DTank`. It adds wheel-child layout helpers, car camera/chase-camera behavior, simple engine-sound pitch tracking, and a reset hook that restores two stored car vectors through inherited transform slots. It is not the shipped `3CAR` placeable Carl NPC even though it registers the same FourCC; the `.gam` corpus maps the current `3CAR` rows to `C3DCarl`.

## Field Map

Offsets below are byte offsets from the primary slot-1 `C3DCar` gameplay pointer unless noted. Raw vtable-4 helper methods are entered on an adjusted outer pointer; in those helpers, outer offset `0x578` corresponds to primary `0x4b8`, matching the inherited `C3DVehicle::wheel_parts` block.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x4b8..0x4cc` | pointer[6] | `wheel_parts` | `C3DVehicle`; raw helpers `00412a90`, `00412c90` | Car layout helpers populate these with `3WHE` / `C3DWheel` child objects. Slot 90 creates/positions four wheels; slot 91 creates/positions six. |
| inherited `0x4dc..0x53c` | struct[6] | `wheel_local_records` | `C3DVehicle`; raw helpers `00412a90`, `00412c90`; reset `00412ff0` | Per-wheel local offsets written by the car wheel-layout helpers and consumed by the inherited wheel solver. |
| inherited raw outer `0x650` / primary `0x590` | int | `drive_state` | `C3DVehicle`; raw camera helper `004130a0` | Car camera helper switches over this state after subtracting one, with cases for states `1..20`. |
| inherited raw outer `0x65c` / primary `0x59c` | byte/bool | `camera_or_solver_special_mode` | raw camera helper `004130a0` | When set and global `DAT_005099a9` is clear, camera position eases from the current camera record toward the car transform instead of using the normal drive-state table. |
| inherited raw outer `0x664` / primary `0x5a4` | pointer | `vehicle_camera_or_view_object` | raw camera helper `004130a0`; `C3DVehicle` viewport helpers | Optional object whose position/angle record is updated from one camera transform path. This aliases the inherited pointer documented by `C3DVehicle`; final semantic name is still open. |
| `0x5a8..0x5b4` | float[4] | `car_vector_a` | ctor `00412870`; reset `00412ff0`; raw wheel helpers | Homogeneous vector restored through inherited vtable offset `0x314` on reset. Constructor seeds the `w` component at `0x5b4` to `1.0`. |
| `0x5b8..0x5c4` | float[4] | `car_vector_b` | ctor `00412870`; reset `00412ff0`; raw wheel helpers | Homogeneous vector restored through inherited vtable offset `0x32c` on reset. Constructor seeds the `w` component at `0x5c4` to `1.0`. |
| `0x5c8` | float | `car_runtime_timer_a` | ctor `00412870`; update `00412ed0` | Cleared by the constructor and incremented by `dt` every update. Exact consumer is unresolved. |
| `0x5cc` | float | `car_runtime_timer_b` | ctor `00412870`; update `00412ed0` | Cleared by the constructor and incremented by `dt` every update. Exact consumer is unresolved. |
| `0x5d0` | int | `engine_sound_handle` | ctor `00412870`; update `00412ed0` | Starts at `-1`. First active update creates/acquires sound/effect handle `0x2e`, then later updates global pitch/frequency state for that handle. |
| `0x5d4` | int | `engine_sound_base_or_cached_pitch` | ctor `00412870`; update `00412ed0` | Constructor seeds `0x2b11` (`11025`). After sound handle allocation it caches `DAT_00695e3c[handle]`; later updates write speed-derived pitch with a floor of `7000`. |
| `0x5d8` | byte/bool | `car_runtime_flag_0x5d8` | ctor `00412870` | Constructor clears this byte. No confirmed consumer yet. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| ctor | `00412870` | `CtorCar3CAR` | Constructs `C3DVehicle`, installs all five `C3DCar` vtables, registers class strings `C3DCAR`/`C3DCar()`, applies inherited setup hooks including a `0.2` scalar, registers duplicate FourCC `3CAR`, seeds two car vectors, clears timers, and initializes engine-sound fields. | non-trivial |
| 7 | `00465030` | `C3DVehicle::InitObjectVehicle` | Inherited vehicle class-id initializer for `3VEH`; `C3DCar` does not override this slot. | inherited |
| 10 | `00412ff0` | `ResetCarState` | Runs `C3DVehicle::ResetVehicleState`, then restores `car_vector_a` through inherited slot `0x314` and `car_vector_b` through inherited slot `0x32c`. | non-trivial |
| 241 | `00412ed0` | `UpdateCar` | Runs `C3DVehicle::UpdateVehicle`, maintains the active engine-sound handle/pitch when the car is enabled and global sound state is active, and increments two car timers by `dt`. | non-trivial |
| 243 | `00465830` | `C3DVehicle::UpdateVehicleInput` | Inherited vehicle input and speed integrator. | inherited |
| 248 | `00467060` | `C3DVehicle` helper | Inherited vehicle angle/hook behavior. | inherited |
| vtable 3 slot 2 | `004129b0` | `ScalarDeletingDestructor` | Adjusts to the primary object, runs cleanup helper `004129e0`, destroys the tail `OMediaClassStreamer` subobject, and frees allocation when requested. | non-trivial |
| vtable 4 slot 59 | `004130a0` | `UpdateCarCameraRecord` | Car-specific camera solver. Builds camera position from current transform, `DAT_00509a50`, `drive_state`, global flags, and collision/path helper `FUN_0047b4b0`; also writes camera angle fields at `DAT_00509a50 + 0x50`. | non-trivial |
| vtable 4 slots 63-71 | `00413a60` | `CarDisableNumberKeyPredicate` | Returns `false`. This disables the inherited number-key vehicle predicates for `C3DCar`. | trivial |
| vtable 4 slot 75 | `00413060` | `CarRearContactControlPredicate` | Returns true only when global control/axis state is active and the inherited rear/contact count at raw outer `0x602` is positive. | non-trivial |
| vtable 4 slots 76-77 | `00413a60` | `CarDisableControlPredicate` | Returns `false` for two inherited control predicates. | trivial |
| vtable 4 slot 81 | `00412f90` | `UpdateCarWheelTransforms` | Prepares two orientation vectors from inherited transform slots and field `0x63c`, then calls `C3DVehicle::UpdateWheelTransforms`. | non-trivial |
| vtable 4 slot 90 | `00412a90` | `CreateFourWheelLayout` | Lazily creates up to four `3WHE` child objects, stores them in inherited wheel slots, writes local wheel offsets from arguments, positions children relative to the car, updates child transforms, and links child/parent relationships. | non-trivial |
| vtable 4 slot 91 | `00412c90` | `CreateSixWheelLayout` | Creates six `3WHE` child objects, writes six wheel local records from arguments, positions/updates each child, disables one inherited child flag, and applies scale/value `80.0` to each wheel. | non-trivial |

## Per-Frame Behavior

```c
C3DCar::UpdateCar(dt):
    C3DVehicle::UpdateVehicle(dt)

    if is_enabled() and DAT_00696008 != 0:
        if engine_sound_handle == -1:
            engine_sound_handle = allocate_sound_or_effect(this, -1, 0x2e, 1)
            engine_sound_base_or_cached_pitch = DAT_00695e3c[engine_sound_handle]
        else:
            pitch = int(get_vehicle_speed() * 51.0 * 0.5263157894736842
                        + engine_sound_base_or_cached_pitch)
            if pitch < 7001:
                pitch = 7000
            DAT_00695e3c[engine_sound_handle] = pitch

    car_runtime_timer_a += dt
    car_runtime_timer_b += dt
```

```c
C3DCar::ResetCarState():
    C3DVehicle::ResetVehicleState()
    inherited_set_vector_a(car_vector_a)
    inherited_set_vector_b(car_vector_b)
```

The camera helper at `004130a0` is the largest car-local behavior. It starts from the current car transform and global camera record `DAT_00509a50`, then either smooths a special camera mode when raw flag `0x65c` is set or switches on `drive_state` to write camera target positions. Several cases use inherited transform helpers with constants such as `-200.0`, `-30.0`, `-500.0`, `125.0`, `50.0`, `300.0`, and `100.0`; the final section optionally runs a path/collision smoothing helper and updates camera angles with wrap handling.

## Constants And Wiring

`C3DCar` registers `3CAR` in its constructor, but the current 35-level `.gam` corpus has `3CAR` rows for `C3DCarl`. The class-id scan therefore contains two `3CAR` registrars:

| FourCC | Registrar | Current schema use |
|---|---|---|
| `3CAR` | `C3DCar` constructor `00412870`, registrar site `0041294c` | Duplicate/non-placeable for the current corpus; no `C3DCar` property rows are proven. |
| `3CAR` | `C3DCarl` constructor `00413af0`, registrar site `00413bb3` | The `.gam` schema maps `3CAR` rows to Carl NPC placement. |

| Name / Id | Use | Evidence |
|---|---|---|
| `C3DCAR` | Car runtime type/object string. | constructor string reference `0x4edb0c` |
| `C3DCar()` | Constructor trace/string. | constructor string reference `0x4edb00` |
| `3WHE` | Wheel child class created by car layout helpers. | raw helper sites `00412b17`, `00412d20`; class-id scan also names `C3DWheel` at `00468220` |
| wheel count | Four-wheel or six-wheel layout | slot 90 loops four children; slot 91 loops six children. |
| `0.2` | inherited scalar applied during construction | constructor push `0x3e4ccccd` before inherited slot `004711e0` |
| `80.0` | wheel child scale/value in six-wheel helper | `00412c90`; child inherited slot `0x110` |
| engine sound/effect id `0x2e` | active car sound handle allocation | update `00412ed0` call to `FUN_004589c0(this, -1, 0x2e, 1)` |
| sound base `11025` | constructor seed for `engine_sound_base_or_cached_pitch` | constructor stores `0x2b11` at outer `0x694` / primary `0x5d4` |
| minimum pitch `7000` | engine sound pitch floor | update `00412ed0` clamps values below `0x1b59` to `0x1b58` |
| speed-to-pitch scale | `51.0 * 0.5263157894736842` | update `00412ed0`; constants at `.rdata 0x495308` and `0x495310` |

## Assets

`C3DCar` does not directly name a mesh, texture, PNG, ASE, OMT, or canvas asset. Concrete leaves bind visual database entries; this base creates/positions wheel child objects and updates an engine-sound pitch table.

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| child class | `3WHE` / `C3DWheel` | `00412a90`, `00412c90`, class-id scan | Wheel children are looked up/created through `FUN_00459cb0("3WHE")` and stored in inherited `wheel_parts`. |
| sound/effect | id `0x2e` | update `00412ed0` | The exact asset name is not recovered; pitch/frequency is written through `DAT_00695e3c[handle]`. |
| global camera record | `DAT_00509a50` | raw camera helper `004130a0` | Position at `+0x44`, angle at `+0x50` are updated by car camera logic. |

## Confidence

Confidence: Medium

Validation: Static Ghidra dump, direct local `objdump` disassembly over `00412870..00413ae0`, class-id scan, existing `C3DVehicle`/`C3DTank` specs, and `.gam` schema duplicate-FourCC cross-check only; not runtime-validated.

Open questions:
- Create Ghidra function boundaries for raw helpers `00412a90`, `00412c90`, `00412f90`, `00413060`, and `004130a0` to improve decompiler output and name their exact argument lists.
- Identify the concrete sound/effect asset behind id `0x2e`.
- Resolve final semantic names for `car_vector_a`, `car_vector_b`, and the two timer fields.
- Decide how to represent duplicate `3CAR` registrars in future schema tooling without confusing `C3DCar` and `C3DCarl`.

## Notes

- Evidence: `DumpClass.java C3DCar /tmp/decomp_C3DCar.md` (`slots=389`, `owned_methods=2`, `offsets=0`).
- Raw evidence: `DumpFunctions.java /tmp/decomp_C3DCar_raw.md 00412870 00412ed0 00412ff0` plus local objdump ranges `00412870..00413ae0`.
- `docs/_gam_classids.tsv` has `3CAR` registrar rows at `0041294c` (`C3DCar`) and `00413bb3` (`C3DCarl`); `docs/gam_schema.md` maps the current `3CAR` rows to `C3DCarl`.
