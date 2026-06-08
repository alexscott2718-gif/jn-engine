# C3DJeep

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DJeep` |
| Base chain | `C3DCar -> C3DVehicle -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004a2adc`, `004a2aec`, `004a2f3c`, `004a2f78`, `004a2f8c` |
| Ctor(s) | constructor/factory block `004211a0`; registers FourCC `3JEE` at `00421284` |
| Dtor(s) | scalar deleting destructor at `00421300`; cleanup helper at `00421330` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DJeep` is a concrete `C3DCar` drivable-vehicle leaf. The class name and FourCC say Jeep, but the init path uses original asset strings for `omt\scooter.omt`, `SCOOT`, and `SCOOTSTOP`; repo assets also contain parsed `scooter.omt` plus scooter ASE/audio assets. It inherits wheel layout, speed, steering, camera, and engine-sound behavior from `C3DCar`/`C3DVehicle`, then adds a scooter/child-object sync path, Jeep-specific input predicates, and a hide-all-parts helper.

## Field Map

Offsets below are byte offsets from the primary slot-1 `C3DJeep` gameplay pointer unless noted. Raw vtable-4 methods are entered on an adjusted outer pointer, so raw outer `0x578` corresponds to inherited primary `0x4b8`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x4b8..0x4cc` | pointer[6] | `wheel_parts` | `C3DCar`; vtable-4 slot 93 `00421800` | The six wheel/part children hidden along with the Jeep. Populated by inherited `C3DCar` layout helpers. |
| inherited `0x548` | int/pointer | `jeep_forwarded_field_0x548` | ctor `004211a0`; slot 259 `004217d0` | Constructor clears it. Slot 259 forwards it through an adjusted outer slot at offset `0xe0`. Final semantic name is unresolved. |
| inherited `0x590` | int | `drive_state` | ctor `004211a0`; `C3DVehicle`; raw car camera helper | Constructor seeds `1`, selecting the initial vehicle drive/camera state. |
| inherited `0x59b` | byte/bool | `camera_flag_default_true` | ctor `004211a0`; `C3DVehicle` | `C3DVehicle` normally seeds this true; Jeep constructor clears the raw outer byte at `0x65b`, changing the inherited camera/control default. |
| inherited `0x59c` | byte/bool | `vehicle_stop_or_special_mode` | raw helper `00421910`; `C3DVehicle` | Used by Jeep-specific motion/friction helper as an early special-mode branch. |
| inherited `0x5d0` | int | `engine_sound_handle` | `C3DCar`; slots 272/273 | Same sound handle field documented by `C3DCar`. Jeep adds explicit stop/reacquire helpers around sound/effect id `0x2e`. |
| inherited `0x5d8` | byte/bool | `input_impulse_latch` | update `00421580`; ctor through `C3DCar` | Cleared when no relevant input globals are active. Set after Jeep applies a one-shot inherited force/impulse while input is active. |
| `0x5dc` | pointer | `scoot_child_object` | ctor `004211a0`; update `00421580` | Optional attached scooter/passenger child. When set, update advances it, copies Jeep position/orientation into it, and switches its animation between `SCOOT` and `SCOOTSTOP`. Constructor clears it. |
| `0x5e0` | float | `scoot_anim_timer` | ctor `004211a0`; update `00421580` | Accumulates `dt` and resets when it reaches `0.3`, at which point the child animation string is refreshed. |
| inherited `0x55c`, `0x564`, `0x57c`, `0x580` | mixed vehicle runtime | `vehicle_runtime_reset_fields` | update `00421580` | If no `scoot_child_object` is attached, the update path clears several inherited vehicle speed/steering/camera runtime fields and sets one inherited float to `10.0`. |

No `.gam` rows are present for `3JEE`; the class is code-spawned or controlled by non-`.gam` level/game logic.

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| ctor | `004211a0` | `CtorJeep3JEE` | Constructs `C3DCar`, installs Jeep vtables, registers object/type strings, clears Jeep child/timer fields, calls `InitObjectJeep`, registers FourCC `3JEE`, applies inherited scalar `150.0`, seeds `drive_state=1`, clears inherited flags, and finalizes. | non-trivial |
| 7 | `004213f0` | `InitObjectJeep` | Runs `C3DVehicle::InitObjectVehicle`, uses the `omt\scooter.omt` path through the global database/load path, binds vehicle database entry `0` from `DAT_005099c4`, applies inherited scalar `5515.0`, normalizes/finalizes the bound visual, and selects/updates the object. | non-trivial |
| 8 | `004213e0` | `UnInitObjectJeep` | Tail jump to the inherited `C3DObject` uninit path. | inherited/thunk |
| 10 | `00412ff0` | `C3DCar::ResetCarState` | Inherited car reset. | inherited |
| 11 | `00421570` | `InitPhysicsJeepOnce` | If inherited `physics_initialized` is clear, tail-jumps to `C3DVehicle::InitPhysicsVehicle`; otherwise returns. | non-trivial |
| 241 | `00421580` | `UpdateJeep` | Runs `C3DCar::UpdateCar`, updates and transform-syncs `scoot_child_object` when attached, applies a one-shot input impulse, refreshes child `SCOOT`/`SCOOTSTOP` animation every `0.3s`, or resets inherited vehicle runtime when no child is present. | non-trivial |
| 243 | `00421880` | `UpdateJeepInput` | Gates `C3DVehicle::UpdateVehicleInput` behind global locks/predicates. If input byte `DAT_00509834` is active, sets global transition/action flag `DAT_004f8182` before delegating. | non-trivial |
| 259 | `004217d0` | `ForwardJeepFieldToOuter` | Forwards `jeep_forwarded_field_0x548` to an adjusted outer slot at vtable offset `0xe0`. Exact runtime meaning is unresolved. | raw |
| 270 | `004218c0` | `EnterJeepJimmyLockOrExit` | Resolves object tag `JIM1`, calls a high-offset method on the resolved object, sets its byte `0x1d65`, calls an adjusted outer slot at `0x164`, and sets global `DAT_004f8182`. | non-trivial |
| 272 | `00421bb0` | `StopJeepEngineSound` | If `engine_sound_handle != -1`, calls `FUN_0047d7a0(handle, 0)`. | non-trivial |
| 273 | `00421bd0` | `ReacquireJeepEngineSound` | If `engine_sound_handle != -1`, allocates/replaces a sound/effect handle using id `0x2e` and stores it back. The inverted-looking guard needs runtime validation. | non-trivial |
| vtable 1 slot 221 | `00421910` | `ApplyJeepMotionConstraint` | Raw helper that checks vehicle flags, speed, transform vectors, rear/contact state, and input globals, then may call inherited movement/force setters. | raw |
| vtable 4 slots 72-75 | `00421c10`, `00421c40`, `00421c70`, `00421cb0` | `CheckJeepControlPredicates` | Jeep-specific input predicates over digital globals and analog axes `DAT_00509aa0`/`DAT_00509aa4`; slots 74/75 require positive rear/contact count at raw outer `0x602`. | non-trivial |
| vtable 4 slot 93 | `00421800` | `HideJeepAndWheelParts` | If the Jeep or any of six wheel/part children are visible/enabled, forces their state flag to `1` and calls slot `0x58` with `0`, effectively hiding/disabling them. | non-trivial |

## Runtime Behavior

```c
C3DJeep::InitObjectJeep():
    C3DVehicle::InitObjectVehicle()
    load_or_select_vehicle_database("omt\\scooter.omt")
    entry = vehicle_db_lookup(DAT_005099c4, 0)
    inherited_bind_shape_or_model(entry)
    inherited_apply_scalar(5515.0)
    normalize_or_finalize_bound_visual()
    inherited_select_or_update()
```

```c
C3DJeep::UpdateJeep(dt):
    if !engine_allows_update():
        return

    C3DCar::UpdateCar(dt)

    if scoot_child_object == null:
        clear inherited vehicle runtime fields
        inherited_set_speed(0)
        return

    scoot_child_object->update(dt)

    if no relevant input globals are active:
        input_impulse_latch = false
    else if !input_impulse_latch and inherited_predicate_allows_impulse():
        inherited_apply_force_or_impulse(0, 700.0, 0)
        input_impulse_latch = true

    copy Jeep world position into scoot_child_object
    copy Jeep world angle/vector state into scoot_child_object

    scoot_anim_timer += dt
    if scoot_anim_timer >= 0.3:
        if inherited_get_speed() >= 0:
            scoot_child_object->set_anim("SCOOT")
        else:
            scoot_child_object->set_anim("SCOOTSTOP")
        scoot_anim_timer = 0
```

`UpdateJeepInput` is a wrapper around the inherited vehicle input integrator. It skips input while a global lock is active, while engine/frame updates are blocked, or while `DAT_004f8181` is set. Otherwise it optionally marks `DAT_004f8182` and delegates to `C3DVehicle::UpdateVehicleInput(dt)`.

## Constants And Wiring

| Name / Id | Use | Evidence |
|---|---|---|
| `3JEE` | Jeep concrete class id. | ctor `004211a0`; registrar site `00421284`; `_gam_classids.tsv` |
| `C3DJEEP` | Runtime type string. | constructor string reference `0x4ecc80`; also used by missiles, AI cars, and turrets as a target class string. |
| `C3DJeep()` | Constructor trace/string. | string reference `0x4eeee8` |
| `omt\scooter.omt` | OMT/database path used by Jeep init. | string bytes at `0x4eef2c`; `InitObjectJeep` |
| vehicle database entry `0` | Visual/model binding. | `FUN_00477ba0(DAT_005099c4, 0)` in `InitObjectJeep` |
| `SCOOT`, `SCOOTSTOP` | Child animation names. | update slot `00421580`; strings `0x4eef4c`, `0x4eef54` |
| `JIM1` | Object tag resolved by slot 270. | string at `0x4ec7f8`; `FUN_00474070("JIM1")` |
| `150.0` | Constructor-applied inherited scalar. | ctor push `0x43160000` |
| `5515.0` | Init-applied inherited scalar. | init push `0x45ac5800` through vtable offset `0x108` |
| `0.3` | Child animation refresh interval. | update compares `scoot_anim_timer` against double at `.rdata 0x4a3110` |
| `700.0` | One-shot input impulse argument. | update push `0x442f0000` before inherited force slot |
| sound/effect id `0x2e` | Jeep/C3DCar engine sound handle. | slots 272/273 and inherited `C3DCar` update |

`docs/gam_schema.md` has no `3JEE` rows in the shipped level corpus.

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `scooter.omt` | `InitObjectJeep`; `assets/parsed/scooter/scooter.json` | Parsed original path is `/home/scotty/xp-jnbg-original/omt/scooter.omt`. |
| OMT textures | `scooterwheel2`, `goddard128` | `assets/parsed/scooter/scooter.json` | Two images in the original JNBG `scooter.omt`. |
| ASE candidates | `assets/ase/jimscooter.ASE`, `assets/ase/jimscooterstop.ASE`, `assets/ase/godscooter.ASE`, `assets/ase/godscooter2.ASE` | repo asset scan | Useful visual leads; the Jeep-owned code binds through OMT database entry `0`, not direct ASE filenames. |
| sound candidate | `assets/parsed/soundeffects/soundeffects_audio/0013_Scooter.wav` | repo asset scan | Name matches the scooter branch; executable sound id mapping still needs validation. |

## Confidence

Confidence: Medium

Validation: Static Ghidra dump, local disassembly of `004211a0..00421d70`, raw string-table checks, class-id scan, existing `C3DCar`/`C3DWheel`/`C3DVehicle` specs, and local parsed scooter asset metadata only; not runtime-validated.

Open questions:
- Name the attached `scoot_child_object` producer and identify which object/class owns the `SCOOT`/`SCOOTSTOP` animation table.
- Resolve the exact semantics of `jeep_forwarded_field_0x548` and slot 259.
- Repair Ghidra function boundaries for raw helpers `00421580`, `00421910`, and `00421c10..00421cb0` to improve decompiler output.
- Runtime-check the sound helper guard in slot 273; statically it only reacquires when the handle is not `-1`.
- Confirm whether the `C3DJeep` gameplay object should be named scooter in higher-level docs even though RTTI/FourCC use Jeep.

## Notes

- Evidence: `DumpClass.java C3DJeep /tmp/decomp_C3DJeep.md` (`slots=390`, `owned_methods=7`, `offsets=1`).
- Raw evidence: `DumpFunctions.java /tmp/decomp_C3DJeep_raw.md` plus local objdump ranges `004211a0..00421d70`.
- No `tools/gam_schema.py` regeneration was needed because `3JEE` has no `.gam` rows and was already named in `_gam_classids.tsv`.
