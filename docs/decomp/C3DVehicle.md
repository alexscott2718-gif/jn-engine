# C3DVehicle

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DVehicle` |
| Base chain | `C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004d5250`, `004d5260`, `004d56b0`, `004d56ec`, `004d5700` |
| Ctor(s) | constructor at `00464b90`; class-id registrar in slot 7 at `00465030` binds `3VEH` |
| Dtor(s) | adjusted scalar deleting destructor at `00464df0`; cleanup helper at `00464e20` |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

Offsets below are byte offsets from the primary slot-1 `C3DVehicle` gameplay pointer unless noted. Vtable-4 helper methods are entered on an adjusted OMedia-side subobject; in those helpers, `this + 0xc0` is the primary gameplay pointer, so raw adjusted offsets such as outer `0x578` correspond to primary `0x4b8`.

`C3DVehicle` is a concrete vehicle-control base under `C3DObject`. It owns a six-child wheel/part layout, vehicle speed and steering runtime, wheel suspension geometry, input-state flags, and an optional vehicle-specific `OMediaViewPort`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| `0x4b8..0x4cc` | pointer[6] | `wheel_parts` | constructor `00464b90`; update `00465220`; helpers `00464f00`, `00464f30`, `00467080` | Six child vehicle parts, treated as wheel/ground-contact objects. Front pair is index `0..1`; remaining four count as the rear/other group. |
| `0x4dc..0x53c` | struct[6] | `wheel_local_records` | constructor; wheel solver `00467080`; raw helper `00465170` | Six 16-byte local wheel placement records. Constructor seeds the leading float of each record to `1.0` and clears the three vector components; wheel solver transforms these local offsets through the vehicle transform. |
| `0x540` | int16 | `front_grounded_count` | update `00465220` | Count of active/grounded wheel children among indices `0..1`. |
| `0x542` | int16 | `rear_grounded_count` | update `00465220` | Count of active/grounded wheel children among indices `2..5`. |
| `0x544` | int16 | `total_grounded_count` | update `00465220`; wheel solver `00467080` | Sum of the two contact counts. Used to decide whether steering/velocity and camera correction are applied. |
| `0x548` | float | `vehicle_tuning_seed` | constructor; reset `004650a0` | Constructor seeds this to `100.0` and passes it into `SetVehicleTuningBlock`; reset forwards it through an adjusted outer hook. Exact semantic name is unresolved. |
| `0x54c..0x56c` | float block | `vehicle_tuning_block` | constructor; slot 56 `00464f60`; raw slot 57 `00464fd0`; input integrator `00465830`; wheel solver `00467080` | Derived vehicle speed/suspension tuning block. It contains defaults derived from seed `100.0`: `100.0`, `700.0`, `700.0`, `1200.0`, `0.0`, `2250.0`, `0.0`, `1485.0`, `1485.0`. |
| `0x550` | float | `accel_rate` | slot 56 defaults; input integrator `00465830` | Acceleration scalar used to increase current speed toward the target cap. Default from seed `100.0` is `700.0`. |
| `0x558` | float | `decel_rate` | slot 56 defaults; input integrator `00465830` | Deceleration/braking scalar. Default from seed `100.0` is `1200.0`. |
| `0x55c` | float | `speed_command_or_scaled_speed` | reset `004650a0`; input integrator `00465830`; raw slot 60 `004669f0` | Per-frame speed command. When not accelerating, it is derived from current speed by `* 1.5151515`; when hard acceleration is active it is forced to max speed. |
| `0x560` | float | `max_speed` | slot 56 defaults; input integrator `00465830`; update `00465220` | Base speed cap. Default from seed `100.0` is `2250.0`; the integrator derives `0.66 * max_speed` or `0.825 * max_speed` target caps. |
| `0x564` | float | `current_speed_sample` | reset `004650a0`; input integrator `00465830` | Cached value returned by the inherited speed getter at vtable offset `0x268`. |
| `0x568` | float | `target_speed_cap` | input integrator `00465830` | Derived speed cap for normal/boost acceleration. |
| `0x570`, `0x574` | float | `wheel_vertical_clamp_range` | constructor; wheel solver `00467080` | Constructor seeds both to `15.0`; wheel probing uses them to clamp wheel vertical displacement. |
| `0x57c` | float | `steering_angle_deg` | reset `004650a0`; update `00465220`; input integrator `00465830` | Steering accumulator clamped to `[-45, 45]`. Direct input changes it at `200 deg/s`. |
| `0x580` | float | `steering_force` | reset `004650a0`; update `00465220` | Per-frame steering/turning force derived from contact counts, current speed, and `steering_angle_deg`. |
| `0x584` | float | `cached_world_y` | reset `004650a0` | Cached component from the inherited transform getter. Exact use after reset is unresolved. |
| `0x58c` | float | `vehicle_timer_or_blend` | raw slot 60 `004669f0`; slot 80 `00467000` | Timer/blend accumulator. Slot 80 resets it to zero. |
| `0x590` | int | `drive_state` | input integrator `00465830`; camera helpers `00465e00..004669f0` | Vehicle camera/drive state enum. Observed values include `1`, `2`, `3`, `6`, `8`, `0xb`, `0xc`, `0xd`, `0xf`, and `0x14`. |
| `0x594` | int | `saved_drive_state` | input integrator `00465830` | Temporarily stores the pre-override `drive_state` while state `8` is active. |
| `0x598..0x5a3` | byte flags | `vehicle_input_flags` | constructor; reset; update; input integrator | Packed runtime flags for accelerate/brake/reverse/steering/camera and solver bypasses. Exact bit names need runtime validation. |
| `0x59b` | byte/bool | `camera_flag_default_true` | constructor | Constructor sets this flag to `1`; exact behavior is unresolved. |
| `0x59c` | byte/bool | `motion_stop_or_solver_bypass` | constructor; update `00465220`; raw slot 60 `004669f0`; slot 80 `00467000` | When set, update dispatches an adjusted outer hook and returns early. Slot 80 sets it after stopping motion. |
| `0x5a0..0x5a2` | byte flags | `analog_solver_flags` | constructor; raw slot 61 `00466b30`; slot 80 `00467000` | Analog/camera solver flags updated from axes `DAT_00509aa0` and `DAT_00509aa4`; slot 80 clears `0x5a0`. |
| `0x5a4` | pointer | `vehicle_viewport` | constructor; slots 88/89; input integrator guard | Lazily allocated `OMediaViewPort` used for a vehicle-specific view. Also checked as a null guard by one reverse/brake path. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| ctor | `00464b90` | `CtorVehicle` | Constructs `C3DObject`, installs all five adjusted vftables, registers class strings `C3DVEHICLE`/`C3DVehicle()`, initializes six wheel pointers and local wheel records, seeds vehicle tuning defaults from `100.0`, clears runtime flags, optionally creates the viewport when the adjusted flag at outer `0x65e` is already set, and lazily allocates global `DAT_004fc69c`. | non-trivial |
| 7 | `00465030` | `InitObjectVehicle` | Traces `"InitObject()"`, runs `C3DObject::InitObject3D`, and registers class id `3VEH` through the inherited class-id setter. | non-trivial |
| 10 | `004650a0` | `ResetVehicleState` | Runs `CLocalGameObject::ResetObject`, clears speed/steering runtime, resets inherited speed through slot `0x26c`, caches transform basis/world components, forwards one inherited runtime field through the adjusted outer vtable, and clears two vehicle flags. | non-trivial |
| 11 | `00465070` | `InitPhysicsVehicle` | Traces `"InitPhysics()"` and delegates to `C3DObject::InitPhysics3D`. | trivial |
| 241 | `00465220` | `UpdateVehicle` | Runs `C3DObject::Update3DObject`, counts grounded wheel children, computes steering force and traction from current speed and contact counts, applies velocity/turn deltas through inherited movement slots, and writes the final speed back through slot `0x26c`. | non-trivial |
| 243 | `00465830` | `UpdateVehicleInput` | Per-frame input/state integrator. Reads adjusted outer control slots, updates steering at `200 deg/s`, clamps steering to `[-45, 45]`, applies acceleration/deceleration/reverse logic, sets `drive_state`, and calls the inherited speed setter plus an outer post-input hook. | non-trivial |
| 248 | `00467060` | `PullAnglesAndVehicleHook` | Calls an adjusted outer hook at offset `0x144`, then delegates to `C3DObject::PullWorldAnglesToGameObject`. | non-trivial |
| vtable 3 slot 2 | `00464df0` | `ScalarDeletingDestructor` | Runs cleanup helper `00464e20`, destroys the adjusted `OMediaClassStreamer` subobject, and frees the adjusted allocation when requested. | non-trivial |
| vtable 4 slot 54 | `00464f00` | `ShowVehicleParts` | Calls the inherited visibility/enabled setter for the vehicle, then forwards enabled state `1` to each non-null wheel child. | non-trivial |
| vtable 4 slot 55 | `00464f30` | `HideVehicleParts` | Clears the vehicle enabled/visible state and forwards enabled state `0` to each non-null wheel child. | non-trivial |
| vtable 4 slot 56 | `00464f60` | `SetVehicleTuningBlock` | Derives the primary `0x54c..0x56c` tuning block from one float. Default construction calls it with `100.0`. | non-trivial |
| vtable 4 slot 57 | `00464fd0` | `OffsetVehicleTuningBlock` | Raw vtable target. Adds a scalar offset into the same tuning block and recomputes the derived `0.66` scaled bounds. | TODO |
| vtable 4 slot 58 | `00465170` | `MoveVehicleAndWheelOffsets` | Raw vtable target. Applies a delta to each wheel local offset, forwards the delta to the primary object, then writes the resulting world position into `DAT_00509a50 + 0x44`. | TODO |
| vtable 4 slot 59 | `00465e00` | `UpdateVehicleCameraRecord` | Raw vtable target. Large camera/drive-state helper that writes global camera record angles and positions using `drive_state`, current transform, and several vehicle flags. | TODO |
| vtable 4 slot 60 | `004669f0` | `ApplyVehicleDragOrSkidForces` | Raw vtable target. Updates primary `0x58c`, optionally stops vehicle movement, and otherwise applies side/friction forces from current transform vectors. | TODO |
| vtable 4 slot 61 | `00466b30` | `UpdateAnalogVehicleInput` | Raw vtable target. Reads analog axes `DAT_00509aa0`/`DAT_00509aa4`, writes steering/analog state at primary `0x57c`, toggles flags `0x5a1/0x5a2`, and updates `target_speed_cap`. | TODO |
| vtable 4 slot 62 | `00466be0` | `CheckKeyChord` | Raw vtable target. Tail wrapper around `FUN_0046a460(arg0, arg1)`. | TODO |
| vtable 4 slots 63-71 | `00466c00..00466c80` | `CheckNumberKey1To9` | Raw input predicates. Each calls `FUN_0046a3e0` with keycodes `0x31..0x39`. | TODO |
| vtable 4 slots 72-79 | `00466c90..00466db0` | `CheckVehicleControlPredicate` | Raw input predicates over globals near `DAT_00509834`, `DAT_00509849`, `DAT_0050985d..60`, `DAT_0050987a`, `DAT_00509890`, `DAT_0050993a`, and `DAT_00509ac4..c6`; some also require positive rear/contact count at primary `0x542`. | TODO |
| vtable 4 slot 80 | `00467000` | `StopVehicleMotion` | Resets inherited speed and velocity, sets primary flag `0x59c`, clears primary flag `0x5a0`, and zeros primary `0x58c`. | non-trivial |
| vtable 4 slot 81 | `00467080` | `UpdateWheelTransforms` | Wheel transform/suspension solver. Iterates six wheel children, updates child positions/orientations from local wheel records, probes terrain/collision where enabled, updates wheel spin, and applies camera correction when contact counts and vehicle state allow. | non-trivial |
| vtable 4 slot 83 | `00467990` | `SetDefaultVehicleCameraOffset` | Calls inherited slot `0x36c` with `(340.0, 0, 0)` and sets adjusted field `0x35c` to `2.0`. | non-trivial |
| vtable 4 slots 84-87 | `004679c0..00467c90` | `VehicleVectorTransformHelpers` | Raw helper cluster that transforms vectors through vehicle orientation/current transform for camera and wheel logic. | TODO |
| vtable 4 slot 88 | `00467db0` | `CreateVehicleViewport` | Lazily allocates an `OMediaViewPort`, binds world/camera/supervisor globals `DAT_00509a4c`, `DAT_00509a30`, and `DAT_00509a48`, then configures viewport rectangles from global display dimensions. | non-trivial |
| vtable 4 slot 89 | `00467f70` | `DestroyVehicleViewport` | Releases the lazy `vehicle_viewport` through its adjusted delete/release slot and clears the pointer. | non-trivial |

## Per-Frame Behavior

```c
C3DVehicle::UpdateVehicle(dt):
    C3DObject::Update3DObject(dt)
    if !engine_allows_update() or !is_enabled():
        return

    front_grounded_count = 0
    rear_grounded_count = 0
    for index, wheel in wheel_parts[0..5]:
        if wheel != null and wheel->is_grounded_or_active():
            if index <= 1:
                front_grounded_count++
            else:
                rear_grounded_count++
    total_grounded_count = front_grounded_count + rear_grounded_count

    if vehicle_input_flags.bypass_update:
        adjusted_outer_vehicle_hook(dt)
        return

    speed = inherited_get_speed()
    if speed > 0 and front_grounded_count > 0:
        steering_force = f(speed, front_grounded_count, steering_angle_deg, flags)
        inherited_apply_turn_delta(steering_force * dt)

    if inherited_can_move() and total_grounded_count > 1:
        velocity = compute_traction_velocity(speed, steering_angle_deg, flags)
        inherited_set_velocity_or_force(velocity)

    inherited_set_speed(speed)
```

```c
C3DVehicle::UpdateVehicleInput(dt):
    current_speed_sample = inherited_get_speed()
    read adjusted outer analog/control slots

    steering_angle_deg += steering_input * dt * 200.0
    steering_angle_deg = clamp(steering_angle_deg, -45.0, 45.0)

    target_speed_cap = max_speed * (boost_or_fast_mode ? 0.825 : 0.66)
    current_speed = accelerate_or_decelerate(current_speed_sample,
                                             target_speed_cap,
                                             accel_rate,
                                             decel_rate,
                                             reverse_allowed)

    speed_command_or_scaled_speed =
        accelerating_hard ? max_speed : current_speed * 1.5151515

    drive_state = derive_state_from_number_keys_and_control_predicates()
    inherited_set_speed(current_speed)
    adjusted_outer_post_input_hook(dt)
```

`UpdateWheelTransforms` is the second major per-frame path. It is called through the adjusted vtable and works from the physical outer object, so its visible offsets differ by `0xc0`. It clears child wheel forces, transforms each local wheel record through the vehicle transform, clamps vertical displacement against `wheel_vertical_clamp_range`, updates child wheel orientation/spin, and performs a final camera correction when update is enabled and the vehicle has either wheel contact or an active movement state.

## Constants And Wiring

`C3DVehicle` registers class id `3VEH` in `00465030`, but the current 35-level `.gam` corpus has no `3VEH` rows. This spec is therefore based on binary/vtable behavior, not per-instance schema rows. Related vehicle branch IDs visible in the class-id scan include `3JEE` (`C3DJeep` factory at `004211a0`), `3TAN` (`C3DTank` factory at `004450f0`), and `3WHE` (`C3DWheel` factory at `00468220`).

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `3VEH` | FourCC | n/a | no current `.gam` rows | Registered by slot 7. |
| wheel count | constant | n/a | `6` | Constructor, visibility helpers, update contact count, and wheel transform solver all iterate six wheel/part pointers. |
| front wheel count | constant | n/a | indices `0..1` | Update treats the first two wheel children as the front steering/contact group. |
| steering clamp | float | `0x57c` | `-45.0..45.0` | Input integrator clamps steering angle. |
| steering rate | float | `0x57c` | `200.0 deg/s` | Input integrator adds/subtracts `dt * 200.0`. |
| normal speed cap scale | float | `0x568` | `0.66 * max_speed` | Used when boost/fast mode is inactive. |
| boost speed cap scale | float | `0x568` | `0.825 * max_speed` | Used when one outer control predicate is active. |
| reverse cap scale | float | speed path | `-0.4 * target_speed_cap` | Reverse/brake path clamps negative speed to this value. |
| default tuning seed | float | `0x548`, `0x54c..0x56c` | `100.0` | Constructor calls `SetVehicleTuningBlock(100.0)`. |
| default accel scalar | float | `0x550`, `0x554` | `input + 600.0`; default `700.0` | Slot 56 writes both fields from the seed. |
| default decel scalar | float | `0x558` | `1200.0` | Slot 56 writes the fixed braking/deceleration value. |
| default max speed | float | `0x560` | `input * 4.5 + 1800.0`; default `2250.0` | Slot 56/57 derive vehicle speed cap. |
| default normal target cap | float | `0x568`, `0x56c` | `0.66 * max_speed`; default `1485.0` | Slot 56/57 derive target cap defaults. |
| vertical clamp defaults | float | `0x570`, `0x574` | `15.0` | Constructor seeds wheel vertical clamp bounds. |
| default camera offset | float | adjusted camera field | `(340.0, 0, 0)` and `2.0` | Slot 83 seeds camera/offset behavior. |
| key predicates | keys | vtable 4 slots 63-71 | ASCII `1` through `9` | Raw predicates call `FUN_0046a3e0(0x31..0x39)` and feed `drive_state`. |

## Assets

`C3DVehicle` does not directly name a mesh, texture, canvas, sound, or OMT asset. Derived vehicle classes and attached wheel/part objects supply concrete render assets.

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| class id | `3VEH` | slot 7 `00465030` | Registered vehicle base id; absent from current `.gam` rows. |
| child object class | `3WHE` / `C3DWheel` | class-id scan | Likely wheel child class consumed through the six `wheel_parts` pointers; exact creation/attachment path is in derived vehicle classes. |
| viewport | `OMediaViewPort` | slots 88/89 | Optional vehicle-specific viewport attached to global world/camera/supervisor objects. |
| global camera record | `DAT_00509a50` | raw slot 58; camera helpers | Vehicle helpers update position/angle fields used by camera or replay state. |

## Confidence

Confidence: Medium

Validation: Static Ghidra + local `objdump` disassembly + `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Create Ghidra functions for raw vehicle vtable targets `00464fd0`, `00465170`, `00465e00`, `004669f0`, `00466af0`, `00466b20`, `00466b30`, `00466be0`, `00466c00..00466db0`, and `004679c0..00467c90`, then re-run `DumpClass`.
- Name the inherited movement/transform slots (`0x268`, `0x26c`, `0x2ac`, `0x2b4`, `0x2b8`, `0x2bc`, `0x2c4`, `0x2cc`, `0x328`, `0x334`, `0x36c`, `0x38c`) against OMedia/CGameObject structs.
- Determine where derived classes populate `wheel_parts`, `max_speed`, `accel_rate`, and `decel_rate`.
- Map vehicle input globals and key predicates to actual keyboard/controller controls.
- Validate whether `vehicle_viewport` is used in original gameplay or only in unused/debug vehicle modes.

## Notes

- Evidence: `DumpClass.java C3DVehicle /tmp/decomp_C3DVehicle.md` (`slots=386`, `owned_methods=15`, `offsets=4`).
- Constructor/default evidence comes from local disassembly over `/home/scotty/xp-jnbg-original/Neutron.exe` at `00464b90..00464ddf`.
- Raw helper evidence comes from vtable slots plus `objdump` at `00464fd0..00466db0` and `004679c0..00467daf`; `DumpFunctions.java` currently reports these addresses as undefined functions in Ghidra.
- `docs/gam_schema.md` has no `3VEH`, `3JEE`, `3TAN`, or `3WHE` rows in the original level corpus, so no property table was back-filled for this class.
