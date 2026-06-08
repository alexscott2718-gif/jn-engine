# C3DMissile

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DMissile` |
| Base chain | `C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004a7e44`, `004a7e54`, `004a82a4`, `004a82e0`, `004a82f4` |
| Ctor(s) | constructor/factory at `0042f500` for `3MIS` |
| Dtor(s) | adjusted scalar deleting destructor at `0042f620`; cleanup helper at `0042f650`; destructor thunks at `0042f9a0`, `0042f9b0`, and `0042f9c0` |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

Offsets below are byte offsets from the primary slot-1 `C3DMissile` pointer, which is the active pointer stored in `C3DYokTurret::missile_pool`. The constructor and vtable-4 reset helper also touch the outer allocation pointer; those writes are `0xc0` higher in outer space.

`C3DMissile` is a code-spawned animated 3D projectile. It has no current `.gam` rows; Yokian turrets preallocate three active pointers, copy muzzle transform/orientation into one pointer at launch, arm its runtime flags, and reuse the pool.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| active `0x600` / outer `0x6c0` | byte/bool | `flight_active` | ctor `0042f500`; update `0042f760`; turret fire `0044cd50`; reset `0042f900` | Set to `1` by `C3DYokTurret::FireYokTurretMissile`; when set, the missile writes movement vectors and advances `flight_elapsed`. Reset/hide clears it. |
| active `0x601` / outer `0x6c1` | byte/bool | `reset_or_inactive_latch` | ctor `0042f500`; reset `0042f900` | Constructor clears it; reset/hide sets it to `1`. No local consumer was found in the class-specific vtable slots. |
| active `0x604` / outer `0x6c4` | float | `flight_elapsed` | ctor `0042f500`; update `0042f760`; turret fire `0044cd50`; reset `0042f900` | Accumulates `dt` while `flight_active`; launch and timeout reset it to `0`. Timeout threshold is the shared double constant at `.rdata:0048e5d0` (`0.5`). |
| active `0x608` / outer `0x6c8` | pointer | `last_valid_hit_object` | collision slot `0042f800`; ctor `0042f500` | Set to the collided object when it reports `C3DROCKET`, `C3DJIMMY`, or `C3DJEEP`; initialized to `0`. No local follow-up consumer was found before the next class body. |
| inherited adjusted state | byte/flags | render/collision toggles | ctor `0042f500`; reset `0042f900`; turret fire `0044cd50` | The missile repeatedly calls inherited setters at vtable offsets `0xa0`, `0xa8`, `0xd0`, `0x110`, `0x214`, and `0x2c4`. Exact names remain inherited/OMedia struct work, but the launch path clearly shows/hides, enables/disables, and clears velocity-like state. |

## Vtable Methods

`DumpClass` reports three owned methods, but four additional raw vtable targets in the missile address range are not defined as Ghidra functions yet. They are included below because the vtable points at them directly.

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| ctor | `0042f500` | `CtorMissile3MIS` | Constructs `C3DAnimated`, installs five adjusted missile vftables, names the class `C3DMISSILE` / `C3DMissile()`, runs missile init, binds FourCC `3MIS`, clears runtime flags, and seeds inherited render/collision state. | non-trivial |
| 7 | `0042f6a0` | `InitObjectMissile` | Runs the inherited animated init, initializes the adjusted 3D object, loads `missile.ase` as `HIDEFAULT`, loads `missile.png` in texture slot `0`, assigns the material/texture, applies material mode `(0, 6, 1)`, applies scalar `40.0`, selects `DEFAULT`, and applies scale/radius-like scalar `0.6`. | non-trivial |
| 16 | `0042f800` | `HandleMissileCollision` | Raw vtable target. Calls inherited collision handling, checks the other object's type string against `C3DROCKET`, `C3DJIMMY`, and `C3DJEEP`, then runs an outer missile hook and stores the other pointer in `last_valid_hit_object` on match. | raw block |
| 220 | `0042f8a0` | `MissileEventOrTransformHookA` | Raw vtable target. Copies a vector argument through helper `00409f60`, calls inherited helper `00472690`, then invokes the outer missile hook at vtable offset `0x120`. | raw block |
| 221 | `0042f860` | `MissileEventOrTransformHookB` | Raw vtable target. Same wrapper shape as slot 220, but calls inherited helper `00458890` before the outer hook. | raw block |
| 223 | `0042f8e0` | `FinalizeMissileHook` | Runs inherited `CGameObject` final/update helper `00472970`, then invokes the outer missile hook at vtable offset `0x120`. | non-trivial |
| 241 | `0042f760` | `UpdateMissileFlight` | Raw per-frame slot. Runs `C3DAnimated::UpdateAnimated(dt)`; if `flight_active`, writes a movement vector using the current transform and `-DAT_005cfc04`, accumulates `flight_elapsed`, and when the elapsed time reaches `0.5` resets the timer and calls inherited state/collision toggles with `1`. | raw block |
| vtable 4 slot 72 | `0042f900` | `ResetHideMissile` | Clears `flight_active`, sets `reset_or_inactive_latch`, calls an outer visibility/state setter with `1`, then clears adjusted visible/collision/velocity-like state through inherited slots and resets `flight_elapsed`. | non-trivial |

Inherited behavior remains important:

| Inherited Slot | Address | Owner | Behavior |
|---:|---|---|---|
| 8 | `0040e670` | `C3DAnimated` | Animated uninit/asset cleanup. |
| 241 base | `0040e050` | `C3DAnimated` | Base animated update called before missile flight work. |
| 259 | `0040e7b0` | `C3DAnimated` | Applies initial animated visibility/collision flags. |
| vtable 4 slot 54/56/60/61/63/66 | `0040d4a0` etc. | `C3DAnimated` | Asset/shape/texture loading used by `InitObjectMissile`. |

## Per-Frame Behavior

```c
C3DMissile::UpdateMissileFlight(dt):
    C3DAnimated::UpdateAnimated(dt)

    if !flight_active:
        return

    forward_a = inherited_get_transform_vector()
    forward_b = inherited_get_transform_vector()
    inherited_set_velocity_or_delta(forward_b.x, -DAT_005cfc04, forward_a.z)

    flight_elapsed += dt
    if flight_elapsed >= 0.5:
        flight_elapsed = 0
        inherited_collision_or_state_setter_a(true)
        inherited_collision_or_state_setter_b(true)
```

```c
C3DMissile::HandleMissileCollision(other):
    inherited_collision_handler(other)

    if other.is_type("C3DROCKET") ||
       other.is_type("C3DJIMMY") ||
       other.is_type("C3DJEEP"):
        outer_missile_hook()
        last_valid_hit_object = other
```

```c
C3DMissile::ResetHideMissile():
    reset_or_inactive_latch = true
    flight_active = false
    inherited_outer_state_setter(true)
    inherited_adjusted_visible_or_collision_setter(false)
    inherited_adjusted_animation_or_draw_setter(false)
    flight_elapsed = 0
    inherited_adjusted_velocity_setter(0, 0, 0)
    inherited_collision_or_state_setter_a(false)
    inherited_collision_or_state_setter_b(false)
```

Launch wiring is external:

```c
C3DYokTurret::FireYokTurretMissile():
    missile = missile_pool[next_missile_index] // active pointer
    hide/reconfigure missile
    copy turret muzzle transform and orientation to missile
    missile->inherited_flag_49c = 0
    missile->flight_active = true
    missile->flight_elapsed = 0
    inherited_adjusted_animation_or_draw_setter(true)
    inherited_scale_or_radius(50.0)
    next_missile_index = (next_missile_index + 1) % 3
```

## Constants And Wiring

`C3DMissile` binds class id `3MIS`, but the current `docs/gam_schema.md` corpus has no `3MIS` rows. The class-id scan already names the factory as `C3DMissile()`.

| Property / Constant | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `3MIS` | FourCC | n/a | no current `.gam` rows | Bound by constructor/factory `0042f500`; spawned by `C3DYokTurret`, not placed by level data in the current corpus. |
| `flight_active` | byte/bool | active `0x600` | `0` / `1` | Set by turret fire; gates the missile flight update. |
| `flight_elapsed` | float | active `0x604` | `0..0.5` | Lifetime/timer for active flight. |
| timeout | double | `.rdata:0048e5d0` | `0.5` | When `flight_elapsed >= 0.5`, the missile resets timer and toggles inherited collision/state slots. |
| launch scalar | float | inherited slot `0x110` | ctor `40.0`; turret fire `50.0` | Scale/radius-like inherited setter; exact inherited name still open. |
| scale/radius scalar | float | inherited slot `0x114` | `0.6` | Applied during missile init after selecting `DEFAULT`. |
| type string | str | n/a | `C3DROCKET` | Collision target accepted by `HandleMissileCollision`. |
| type string | str | n/a | `C3DJIMMY` | Collision target accepted by `HandleMissileCollision`. |
| type string | str | n/a | `C3DJEEP` | Collision target accepted by `HandleMissileCollision`. |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| class string | `C3DMISSILE` | string `.data:004ef328`; init `0042f500` | Uppercase runtime class/type name. |
| ctor string | `C3DMissile()` | string `.data:004efbe4`; constructor `0042f500` | Constructor/debug identity. |
| debug trace string | `C3DGraplingHook::InitObject()` prefix | string `.data:004edea8`; init `0042f6a0` | Copy-pasted trace string also seen in `C3DCursor`; not class identity. |
| animation/shape alias | `HIDEFAULT` | init `0042f6a0` | Bound to `missile.ase`. |
| ASE model | `missile.ase` | string `.data:004efc00`; init `0042f6a0` | Missile 3D shape. |
| texture | `missile.png` | string `.data:004efbf4`; init `0042f6a0` | Loaded into texture slot `0`. |
| animation name | `DEFAULT` | string `.data:004ee39c`; init `0042f6a0` | Selected after material setup. |
| effect/sound id | `0x89` | `C3DYokTurret::FireYokTurretMissile` | Fired at the turret muzzle immediately before launching a pooled missile. |

## Confidence

Confidence: Medium

Validation: Static Ghidra, local `objdump` over `/home/scotty/xp-jnbg-original/Neutron.exe`, `C3DYokTurret` launch cross-check, and class-id/schema cross-check only; not runtime-validated.

Open questions:
- Create Ghidra functions for raw vtable targets `0042f760`, `0042f800`, `0042f860`, and `0042f8a0`, then re-run `DumpClass` so owned method count and offset extraction include them.
- Resolve inherited/OMedia slot names for offsets `0xa0`, `0xa8`, `0xd0`, `0x110`, `0x114`, `0x120`, `0x214`, `0x270`, `0x278`, and `0x2c4`.
- Identify the runtime value and owner of `DAT_005cfc04`, which is negated into the missile movement vector.
- Confirm whether `last_valid_hit_object` is consumed by an inherited hook after collision or only recorded for generic object messaging.

## Notes

- Evidence: `DumpClass.java C3DMissile /tmp/decomp_C3DMissile.md` (`slots=369`, `owned_methods=3`, `offsets=1`), local disassembly over `0042f500..0042f96f`, exact string scans around `004ef328` and `004efbe4`, and turret fire helper `0044cd50`.
- `DumpFunctions.java` reports the raw update/collision addresses as undefined functions in the current Ghidra project, so their behavior is taken from direct vtable targets plus `objdump` rather than decompiler output.
- `docs/_gam_classids.tsv` has `3MIS -> C3DMissile()`; `docs/gam_schema.md` has no level-instance row for `3MIS`.
