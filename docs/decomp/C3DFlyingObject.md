# C3DFlyingObject

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DFlyingObject` |
| Base chain | `C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `0049cfc4`, `0049cfd4`, `0049d424`, `0049d460`, `0049d474` |
| Ctor(s) | TODO |
| Dtor(s) | adjusted scalar deleting destructor at `00419db0` |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

Offsets below are byte offsets from the primary `C3DFlyingObject` pointer. Ghidra currently prints many references as `this[N].vftable`; offsets are converted as `N * 4`, with byte subfields noted where needed.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| `0x5fc` | float | `AccelRate` | `.gam` registration at `00419f70`; integrator `0041a140` | Forward acceleration step added to current speed. |
| `0x600` | float | `DecelRate` | `.gam` registration; integrator | Deceleration/braking rate used when input is released or reverse/down input is active. |
| `0x604` | float | `MaxSpeed` | `.gam` registration; integrator | Horizontal speed cap. Debug/all-keys path temporarily forces current/max speed to `2500.0`. |
| `0x608` | float | `current_speed` | `0041a0b0`, `0041a140` | Runtime horizontal speed accumulator. |
| `0x614` | float | `lean_angle` | `0041a0b0`, `0041a140`, `0041a6c0` | Left/right lean accumulator clamped to `[-45, 45]`. |
| `0x618` | float | `previous_lean_angle` | `0041a0b0`, `0041a140` | Previous lean copied before applying roll/turn deltas. |
| `0x61c` | float | `initial_or_ground_y` | `0041a0b0` | Seeded from current world position Y during reset/setup. |
| `0x620` | float | `MaxHeight` | `.gam` registration; integrator | Upper Y/world-height clamp; when reached, vertical velocity is zeroed or forced downward. |
| `0x624` | float | `UpRate` | `.gam` registration; integrator | Positive vertical velocity target when up input is active. |
| `0x628` | float | `DownRate` | `.gam` registration; integrator | Negative vertical velocity target when down input is active. |
| `0x62c` | float | `MaxVertVelocity` | `.gam` registration; integrator | Symmetric clamp for vertical velocity. |
| `0x630` | float | `NewGravity` | `.gam` registration | Registered tuning field; no confirmed owned-method read in the current dump. |
| `0x638` | byte flags | `flight_input_flags` | `0041a0b0`, `0041a140`, `0041a6c0` | Per-frame input/turn/up/down flags packed into the `0x18e` seed word. |
| `0x65c` | float | `vertical_velocity` | `0041a0b0`, `0041a140` | Runtime vertical velocity toward `UpRate`/`DownRate`, clamped by `MaxVertVelocity`. |
| `0x660` | float | `AccelLean` | `.gam` registration; integrator | Lean amount applied while accelerating/up input is active. |
| `0x664` | float | `DecelLean` | `.gam` registration; integrator | Lean amount applied while braking/down input is active. |
| `0x670` | float | `barrel_roll_timer` | `0041afc0`, `0041a140`, raw `0041adf0` | Set by `BARRELROLL`/`BARRELROLL2` pickup links and counted down by movement/facing logic. |
| `0x674` | float | `roll_boost_timer` | `0041a140` | One-shot roll/boost timer triggered by input globals while moving. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `00419f70` | `InitObjectFlying` | Traces `"InitObject()"`, runs `C3DAnimated::InitObject`, registers the ten flying movement tuning fields, then registers class id `3FLY` through inherited class-id setter. | non-trivial |
| 10 | `0041a0b0` | `ResetFlyingRuntime` | Runs `CLocalGameObject::ResetObject`, clears speed/velocity/roll flags, calls several inherited transform reset setters, and seeds `initial_or_ground_y` from the current world position. | non-trivial |
| 16 | `0041afc0` | `HandlePickupCollision` | Extends `CGameObject` collision/link handling; if the other object is a `3PIC` pickup with tag `BARRELROLL` or `BARRELROLL2`, seeds `barrel_roll_timer` to `1.2` or `2.4`. | non-trivial |
| 241 | `0041a140` | `UpdateFlyingMovement` | Main per-frame integrator. Runs `C3DAnimated::Update`, reads movement input globals, updates horizontal speed, vertical velocity, lean, roll timers, height clamps, world velocity, and camera/record deltas. | non-trivial |
| 243 | `0041a6c0` | `UpdateFlyingCameraRecord` | Uses global input/camera state and this object's transform to update `DAT_00509a50` position and angle fields for flying-camera/target behavior. | TODO |
| 246 | `0041adf0` | `UpdateFlyingRotateToDest` | Raw vtable target not defined as a Ghidra function. Disassembly shows it gates inherited `RotateToDest`, computes shortest-arc deltas against destination rotation, and forwards scaled Euler deltas through slot `0x334` unless roll state blocks it. | non-trivial |
| 257 | `0041afb0` | `ResetSelfHook` | Tail-jumps to inherited local-game-object behavior after calling the common no-op hook and reset slot. | trivial |
| vtable 3 slot 2 | `00419db0` | scalar deleting destructor | Runs local cleanup helper, destroys the embedded `OMediaClassStreamer`, and frees the adjusted allocation when requested. | non-trivial |

## Per-Frame Behavior

```c
C3DFlyingObject::UpdateFlyingMovement(dt):
    C3DAnimated::UpdateAnimated(dt)

    velocity = get_velocity_or_forward_vector()
    speed_len = length(velocity.xyz)
    elapsed_flight_time += dt

    update_roll_trigger_flags_from_input(speed_len)
    if roll_boost_timer > 0:
        apply_roll_delta(dt * 180)
        roll_boost_timer = max(roll_boost_timer - dt, 0)
    else if speed_len != 0:
        apply_lean_delta(lean_angle * dt)

    if barrel_roll_timer > 0:
        barrel_roll_timer = max(barrel_roll_timer - dt, 0)
        apply_barrel_roll_delta(dt * 270)

    if forward_or_up_input:
        current_speed = min(current_speed + AccelRate, MaxSpeed)
    else if current_speed > MaxSpeed * 0.909:
        current_speed -= DecelRate * 0.5

    if brake_or_down_input:
        current_speed = max(current_speed - DecelRate * scale, 0)

    lean_angle = clamp(lean_angle + steering_delta, -45, 45)
    vertical_velocity = choose UpRate, DownRate, gravity fallback, or zero
    vertical_velocity = clamp(vertical_velocity, -MaxVertVelocity, MaxVertVelocity)

    if world_y >= MaxHeight:
        clamp_world_y(MaxHeight)
        vertical_velocity = min(vertical_velocity, 0)

    write_world_velocity(current_speed, vertical_velocity, lean_angle)
    update DAT_00509a50 flying camera/target record
```

The exact input globals are still unnamed. Observed groups include keyboard/button globals near `DAT_0050985d..DAT_00509860`, `DAT_0050988a..DAT_00509891`, and analog axes `_DAT_00509aa0/_DAT_00509aa4`.

## Constants And Wiring

`C3DFlyingObject` registers class id `3FLY`, but `3FLY` is a movement base rather than a frequent concrete `.gam` object type. In the current schema, the inherited flying properties are visible on derived placeables such as `3ROC` (`C3DRocketShip`).

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `MaxHeight` | float (`3`) | `0x620` | 32 values, `1500..4000`; common `4000` | Upper world-height clamp. |
| `MaxSpeed` | float (`3`) | `0x604` | 32 values, all `1400` | Horizontal speed cap. |
| `AccelRate` | float (`3`) | `0x5fc` | 32 values, all `400` | Forward acceleration. |
| `DecelRate` | float (`3`) | `0x600` | 32 values, all `1` | Braking/deceleration scalar. |
| `UpRate` | float (`3`) | `0x624` | 32 values, all `650` | Positive vertical velocity target. |
| `DownRate` | float (`3`) | `0x628` | 32 values, all `-650` | Negative vertical velocity target. |
| `MaxVertVelocity` | float (`3`) | `0x62c` | 32 values, all `650` | Vertical velocity clamp. |
| `NewGravity` | float (`3`) | `0x630` | 32 values, all `0` | Registered but not confirmed consumed in owned methods. |
| `AccelLean` | float (`3`) | `0x660` | 32 values, all `20` | Lean while accelerating/up input is active. |
| `DecelLean` | float (`3`) | `0x664` | 32 values, all `-20` | Lean while decelerating/down input is active. |

Pickup tags:

| Tag | Effect |
|---|---|
| `BARRELROLL` | Sets `barrel_roll_timer` to `1.2`. |
| `BARRELROLL2` | Sets `barrel_roll_timer` to `2.4`. |

## Assets

No direct mesh, canvas, sound, or animation asset name is referenced by `C3DFlyingObject` itself. Derived classes supply the concrete animation and shape assets through `C3DAnimated`.

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| class id | `3FLY` | `00419f70` | Registered movement base id. |
| pickup tag | `BARRELROLL`, `BARRELROLL2` | `0041afc0` | Consumed from `3PIC` pickup object tags. |

## Confidence

Confidence: Medium

Validation: Static Ghidra + local disassembly only; not runtime-validated.

Open questions:
- Create/label Ghidra function `0041adf0` and re-run `DumpClass` so slot 246 decompiles normally.
- Name the input globals and map them to keyboard/controller controls.
- Resolve transform setter/getter slots (`0x2c0`, `0x2c4`, `0x310`, `0x328`, `0x334`, `0x36c`, `0x38c`) against OMedia structs.
- Confirm why `NewGravity` is registered but not obviously consumed in the current owned-method dump.
- Validate `DAT_00509a50` flying-camera record updates against a captured frame or runtime trace.

## Notes

- Evidence: `DumpClass.java C3DFlyingObject /tmp/decomp_C3DFlyingObject.md` (`slots=372`, `owned_methods=7`, `offsets=11`).
- Slot 241 is the settled player/flying per-frame integrator `FUN_0041a140`; this spec keeps that durable link for Phase 2 player work.
- Extra check: `DumpFunctions.java /tmp/decomp_C3DFlyingObject_extra.md 0041adf0 ...` reports no function in Ghidra, but `objdump` over `/home/scotty/xp-jnbg-original/Neutron.exe` shows a normal function body from `0041adf0..0041af9f`.
- String evidence includes `C3DFlyingObject()`, `C3DFLYINGOBJECT`, `~C3DFlyingObject()`, the ten movement property names, and `BARRELROLL`/`BARRELROLL2`.
