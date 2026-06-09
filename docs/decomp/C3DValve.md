# C3DValve

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DValve` |
| Base chain | `C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004bde10`, `004bde20`, `004be270`, `004be2ac`, `004be2c0` |
| Ctor(s) | installs the `C3DValve` vftables; `InitObject` seeds scale `80.0` and the default anim |
| Dtor(s) | inherited `C3DAnimated` deleting destructor (none owned) |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DValve` is a placeable **steam-valve mechanism**: an animated 3D object that vents
steam on a timed period and reacts to Jimmy touching it. When Jimmy collides with an
active valve it applies a knock-back impulse and plays a sound; the valve's visual
asset is lazily loaded the first time it is shown. Family `mechanisms_moving_parts`
(wave 6).

## Field Map

Offsets from the primary `C3DValve` pointer (`this[N]` slot arithmetic). The
`this[-0x30]` references are the `OMedia3DShapeElement` subobject (the renderable).

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| `0x19c` | float | `SteamPeriod` | `InitObject` registrar | Registered `.gam` property — steam vent cycle period. |
| `this[0x19a]` | int/bool | `update_scratch` | `vfunc_01_010` | Cleared each update tick. |
| `this[0x19b]` | bool | `valve_active` | `vfunc_01_016`, `vfunc_01_259` | Gate: collision interaction and the reset branch only act when set. |
| `this[0x19f]` | pointer | `touching_player` | `vfunc_01_016`, `vfunc_01_017` | Set to the colliding Jimmy on enter, cleared on exit. |
| `this[0x18d]+1` | bool | `assets_loaded` | `vfunc_04_067` | One-shot latch so the ASE/PNG/anim are loaded only once. |
| `this[300]` (`0x12c`) | bool | `init_flag` | `InitObject` | Cleared at init. |
| `this[0x15f]` | handle | `material_handle` | `vfunc_04_067` | Material/texture handle passed to the shape's `0xf4` assign. |

## Vtable Methods (owned)

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| vtable 1 slot 7 | `00448a30` | `InitObject` | Registers `SteamPeriod`; sets shape scale `80.0` (`0x42a00000`); selects default anim. | non-trivial |
| vtable 1 slot 10 | `00448ba0` | `Update` | Inherited per-frame logic, clears `update_scratch`. | trivial |
| vtable 1 slot 16 | `00448ab0` | `OnCollisionEnter` | If `valve_active` and the collider is `C3DJIMMY`: applies impulse `(-10, -10, 5)` to Jimmy (his shape slot `0x1a0`), plays sound `0xbc`, stores him in `touching_player`. | non-trivial |
| vtable 1 slot 17 | `00448b20` | `OnCollisionExit` | If the leaving object is `C3DJIMMY`, clears `touching_player`. | non-trivial |
| vtable 1 slot 259 | `00448b50` | `Reset` | Inherited reset; then per `valve_active` calls shape slot `0x120`/`0x124` and sets scale `140.0` (`0x430c0000`). | non-trivial |
| vtable 4 slot 67 | `004489d0` | `EnsureAssetsLoaded` | One-shot: loads `valve.ase` (anim `HIDEFAULT`), `valve.png`, assigns material, sets default anim. | non-trivial |
| vtable 4 slot 73 | `00448bd0` | `vfunc_04_073` | Additional owned override (see decompiled body in dump). | raw block |

### Behavior

```c
C3DValve::OnCollisionEnter(other):           // vfunc_01_016 @ 00448ab0
    CGameObject::OnCollisionEnter(other)
    if valve_active and other->IsA("C3DJIMMY"):
        jimmy_shape = other - 0x30
        jimmy_shape->slot_0x1a0(-10.0, -10, 5.0)   // knock-back impulse
        play_sound(0xbc)                            // FUN_00458980(-1, 0xbc, 0)
        touching_player = other

C3DValve::EnsureAssetsLoaded():              // vfunc_04_067 @ 004489d0
    if not assets_loaded:
        assets_loaded = 1
        shape.register_anim("HIDEFAULT", "valve.ase")
        shape.load_texture("valve.png", 0)
        shape.assign_material(material_handle, 0)
        shape.set_default_anim(DAT_004ed040, 1)
```

The valve gates all interaction on `valve_active`; the steam cadence is driven by the
registered `SteamPeriod`. The impulse vector `(-10, -10, 5)` is the push Jimmy takes
when the steam/valve hits him.

## Constants And Wiring

| Item | Value | Evidence |
|---|---|---|
| `SteamPeriod` | `.gam` float @ `0x19c` | `InitObject` registrar |
| Initial scale | `80.0` | `InitObject` immediate `0x42a00000` |
| Reset scale | `140.0` | `Reset` immediate `0x430c0000` |
| Jimmy knock-back | impulse `(-10, -10, 5)` | `OnCollisionEnter` immediates |
| Hit sound | id `0xbc` | `FUN_00458980(-1, 0xbc, 0)` |
| Player class test | `C3DJIMMY` | `IsA` string compare |

## Assets

| Kind | Name | Notes |
|---|---|---|
| ASE model | `valve.ase` | anim tag `HIDEFAULT`; loaded lazily by `EnsureAssetsLoaded`. |
| PNG texture | `valve.png` | paired texture. |
| default anim | `DAT_004ed040` | set after load (flag 1). |

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java C3DValve` (`slots=370`, `owned_methods=7`); the
collision interaction, impulse/sound constants, and lazy asset load are read directly
from the decompiled bodies; `SteamPeriod` + assets resolved via PE strings. Not
runtime-validated.

Open questions:
- Decode `vfunc_04_073` (the remaining owned override) and the role of shape slots
  `0x120`/`0x124` in `Reset`.
- Confirm how `valve_active` and the steam cadence (`SteamPeriod`) drive the anim /
  steam-particle emission per frame.
- Name the `(-10, -10, 5)` axes precisely against the world basis.

## Notes

- Evidence: `DumpClass.java C3DValve /tmp/dumps2/decomp_C3DValve.md`.
- Hand-written from the decompiled bodies (supersedes the generated skeleton).
