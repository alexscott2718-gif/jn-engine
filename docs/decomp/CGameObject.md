# CGameObject

## Identity

| Item | Value |
|---|---|
| RTTI name | `CGameObject` |
| Base chain | `OMediaClassStreamer` |
| Vftable(s) | `004d5fec` |
| Ctor(s) | TODO |
| Dtor(s) | scalar deleting dtor slot 0 at `0046ffe0` |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

Offsets below are byte offsets. Ghidra currently prints many accesses as `this[N].vftable` because the seeded struct is only 4 bytes wide; those offsets are recorded here as `N * 4`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| `0x000` | pointer | `vftable` | RTTI/vftable markup | Primary `CGameObject` vtable. |
| `0x00a` | flag4/byte | `RotateToDest` | `.gam` registration | Enables smooth rotation toward destination angles. |
| `0x0ec` | float | `PositionX` | `.gam` registration | Base position X. |
| `0x0f0` | float | `PositionY` | `.gam` registration | Base position Y. |
| `0x0f4` | float | `PositionZ` | `.gam` registration | Base position Z. |
| `0x0fc` | float | `RotationX` | `.gam` registration | Base rotation X. |
| `0x100` | float | `RotationY` | `.gam` registration | Base rotation Y. |
| `0x104` | float | `RotationZ` | `.gam` registration | Base rotation Z. |
| `0x224` | float | `current_rot_x` | inferred | Angle accumulator used by add/wrap and `RotateToDest` interpolation. |
| `0x228` | float | `current_rot_y` | inferred | Angle accumulator used by add/wrap and `RotateToDest` interpolation. |
| `0x22c` | float | `current_rot_z` | inferred | Angle accumulator used by add/wrap and `RotateToDest` interpolation. |
| `0x254` | list head | `linked_from_objects` | inferred | Intrusive list updated by link/unlink object relation methods. |
| `0x258` | int | `linked_from_count` | inferred | Count for `linked_from_objects`. |
| `0x260` | list head | `collision_id_links` | inferred | Intrusive list of collision/object-id links. |
| `0x264` | int | `collision_id_link_count` | inferred | Count for `collision_id_links`. |
| `0x26c` | list head | `property_nodes` | registrar | Intrusive list of property registration nodes. |
| `0x270` | int | `property_node_count` | registrar | Count for `property_nodes`. |
| `0x274` | float | `dest_rot_x` | inferred | Target X angle for `RotateToDest`. |
| `0x278` | float | `dest_rot_y` | inferred | Target Y angle for `RotateToDest`. |
| `0x27c` | float | `dest_rot_z` | inferred | Target Z angle for `RotateToDest`. |
| `0x298` | float | `reset_timer` | inferred | Countdown checked in per-frame hook; triggers reset slot when it expires. |
| `0x29c` | float | `rotate_to_dest_rate` | inferred | Per-second interpolation rate for destination rotation. |
| `0x3a0` | char buffer | `ObjectTag` | `.gam` registration | Object tag/name copied at init and exposed to `.gam`. |
| `0x404` | int | `ObjectID` | `.gam` registration | ID used by `EnableCollisionID`/collision-link lookup. |
| `0x408` | handle/id | `cache_handle_a` | inferred | Lazy handle from `DAT_00509948 + 0x144`; invalid value is `-1`. |
| `0x40c` | handle/id | `cache_handle_b` | inferred | Lazy handle from `DAT_00509948 + 0x14c`; invalid value is `-1`. |
| `0x410` | handle/id | `cache_handle_c` | inferred | Lazy handle from `DAT_00509948 + 0x148`; invalid value is `-1`. |
| `0x428` | int | `property_generation` | registrar | Incremented when registering a property node. |

## Vtable Methods

`DumpClass.java CGameObject` walks 263 raw vtable entries and decompiles 80 class-owned methods. This table groups repeated getter/setter families but names every owned behavioral cluster.

| Slot(s) | Address(es) | Name | Behavior | Status |
|---:|---|---|---|---|
| 0 | `0046ffe0` | scalar deleting destructor | Destructor/delete wrapper. | TODO |
| 5 | `004702d0` | `SetObjectTagLike` | Copies default/global string and caller string into the object tag buffers. | non-trivial |
| 7 | `004703b0` | `InitObject` | Clears old property nodes and registers common `.gam` properties. | non-trivial |
| 8 | `00470500` | `UnInitObject` | Trace/log hook then no-op cleanup hook. | trivial |
| 10 | `00470540` | `ResetObject` | Large state reset/copy routine using getter/setter vtable pairs; clears relation lists and flags. | non-trivial |
| 11 | `00470a00` | `InitPhysics` | Trace/log hook; default body otherwise empty. | trivial |
| 12 | `00470a20` | `UnInitPhysics` | Trace/log hook; default body otherwise empty. | trivial |
| 13 | `00472970` | `NullCleanupHook` | Empty release-build stub used by many lifecycle paths. | trivial |
| 16-18 | `00470a90`, `00470b20`, `00470bc0` | lazy collision/cache setup | Adds/removes collision-id entries and lazily populates global handles. | non-trivial |
| 21-22 | `00470e60`, `004732a0` | external dispatch/no-op | Slot 21 dispatches into a supplied object; slot 22 is reused no-op. | TODO |
| 26-108 | `00470e80`..`00472640` | scalar/vector setters | Store flags/scalars/vectors, then call paired change hooks. | non-trivial |
| 129-196 | `004719c0`..`00472300` | simple field accessors | More direct bool/vector accessors over higher offsets. | trivial |
| 205 | `00472480` | `AddRotationWrapped` | Adds Euler deltas and wraps each angle into `[0, 360]`. | non-trivial |
| 219 | `00472660` | `SetDestRotation` | Writes destination rotation and marks `RotateToDest` active. | non-trivial |
| 220 | `00472690` | clamp vector Y | Clamps positive Y component of two vector/quaternion groups to zero through setter slots. | non-trivial |
| 221 | `00458890` | conditional transform update | Calls transform setter only if a predicate slot is true. | TODO |
| 233 | `00473030` | clear flag via setter | Calls a setter with zero. | trivial |
| 234, 236 | `00472f90`, `00473040` | link/unlink object relation | Maintains bidirectional intrusive lists between two `CGameObject` instances. | non-trivial |
| 238 | `00473170` | `EnableCollisionID` | Scans global object list and links objects whose `ObjectID` matches the requested ID. | non-trivial |
| 241 | `00473240` | update/timer hook | Calls pre/post update hooks; decrements `reset_timer`; triggers reset when it crosses zero. | non-trivial |
| 245 | `004732b0` | validate cached handles | Uses `FUN_0047d890` to invalidate three lazy global handles. | non-trivial |
| 246 | `00473320` | `UpdateRotateToDest` | Interpolates current rotation toward destination using `dt * rotate_to_dest_rate` with 180-degree shortest-arc wrapping. | non-trivial |
| 248 | `00473540` | angle conversion helper | Converts transform/orientation values to 14-bit angle units and dispatches through a setter. | TODO |
| 254 | `00473910` | formatted/global counter helper | Calls formatting/helper code and increments `_DAT_00509af0`. | TODO |
| 255 | `00473fa0` | `RegisterProperty` | Allocates a 0x4c property node, copies name/type/pointer metadata, links it into `property_nodes`. | non-trivial |
| 258 | `00474480` | `DeleteAllProperties` | Frees all registered property payloads and list nodes. | non-trivial |
| 261 | `004740c0` | wheel-specific transform bridge | Special-cases `"C3DWHEEL"` and forwards transform state. | TODO |

## Per-Frame Behavior

Default per-frame work is split across update hook slot 241 and rotation interpolation slot 246.

```c
CGameObject::Update(dt):
    pre_update_hook()
    if reset_timer > 0:
        reset_timer -= dt
        if reset_timer <= 0:
            ResetObject()
    post_update_hook()

CGameObject::UpdateRotateToDest(dt):
    if engine_allows_update() && rotate_to_dest_enabled && RotateToDest:
        RotateToDest = false
        delta = shortest_arc(dest_rot - current_rot)   // per axis, +/-180 wrap
        if any axis still differs:
            RotateToDest = true
        step = dt * rotate_to_dest_rate
        current_rot += clamp_by_step(delta, step)
```

Lifecycle/property setup:

```c
CGameObject::InitObject():
    trace("InitObject()")
    DeleteAllProperties()
    property_generation = 0
    RegisterProperty("ObjectTag", &ObjectTag, type=1)
    RegisterProperty("RotateToDest", &RotateToDest, type=2)
    RegisterProperty("ObjectID", &ObjectID, type=6)
    RegisterProperty("PositionX/Y/Z", &Position*, type=3)
    RegisterProperty("RotationX/Y/Z", &Rotation*, type=3)
```

## Constants And Wiring

`CGameObject` itself is not a placeable FourCC class, but every placeable class inherits these common `.gam` properties.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `ObjectTag` | str (`1`) | `0x3a0` | arbitrary tag string | Used by trigger/object wiring and lookup. |
| `RotateToDest` | flag4 (`2`) | `0x00a` | 4-byte flag | Enables destination-rotation interpolation. |
| `ObjectID` | int (`6`) | `0x404` | per-object id | Used by `EnableCollisionID` to link matching objects. |
| `PositionX` | float (`3`) | `0x0ec` | level coordinates | Base transform position. |
| `PositionY` | float (`3`) | `0x0f0` | level coordinates | Base transform position. |
| `PositionZ` | float (`3`) | `0x0f4` | level coordinates | Base transform position. |
| `RotationX` | float (`3`) | `0x0fc` | degrees | Base transform rotation. |
| `RotationY` | float (`3`) | `0x100` | degrees | Base transform rotation. |
| `RotationZ` | float (`3`) | `0x104` | degrees | Base transform rotation. |

## Assets

No direct `.ase`, canvas, sound, or OMT asset name is referenced by class-owned methods.

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| - | - | - | - |

## Confidence

Confidence: Medium.

Validation: Not runtime-validated; static Ghidra-only base spec.

Open questions:
- Identify constructor(s) and the true C++ names for the getter/setter slot families.
- Resolve the unowned/blank vtable entries that `DumpClass` can walk but Ghidra has not yet function-labeled.
- Replace seed-struct pointer arithmetic with a real `CGameObject` structure in Ghidra so offset display stops relying on `N * 4` conversion.
- Confirm whether `current_rot_*` versus registered `Rotation*` are distinct conceptual transforms or decompiler artifacts from inherited OMedia layout.

## Notes

- Evidence: `DumpClass.java CGameObject /tmp/decomp_CGameObject.md` (`slots=263`, `owned_methods=80`, `offsets=9`).
- `FUN_004789a0` is the local free wrapper over `omt_dll_free`; property/list nodes use it consistently.
