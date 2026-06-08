# C3DNeuCar2

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DNeuCar2` |
| Base chain | `C3DAICar -> C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004aa694`, `004aa6a4`, `004aaaf4`, `004aab30`, `004aab44` |
| Ctor(s) | constructor/factory block `00432730`; registers FourCC `3NC2` at `004327f2` |
| Dtor(s) | scalar deleting destructor at `00432830`; cleanup helper at `00432860`; adjusted destructor thunks at `00432970`, `00432980`, `00432990` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DNeuCar2` is the paired `3NC2` AI-car leaf next to `C3DNeuCar`. It has no current `.gam` instances in the 35-level corpus. It inherits `C3DAICar` contact/update/effect behavior, uses the same `car.ase` mesh, but binds `cargreen.png` instead of `car.png`.

## Field Map

Offsets are byte offsets from the active `C3DAICar`/AI pointer unless marked `outer`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited active `0x604` / outer `0x6c4` | float | `contact_speed_or_tuning` | ctor `00432730`; inherited `C3DAICar` | Constructor overwrites the inherited AI-car tuning field with `20.0`, matching `C3DNeuCar`. |
| inherited active `0x608` / outer `0x6c8` | int | `current_state` | inherited `C3DAICar` | Inherited constructor seeds state `3`; no NeuCar2-owned write found. |
| inherited active `0x87c` / outer `0x93c` | int | `AIState` | inherited `C3DAICar` | Inherited constructor seeds serialized/default state `3`; no NeuCar2-owned `.gam` row exists. |
| inherited active `0x8ec` / outer `0x9ac` | handle | `ambient_effect_handle` | `C3DAICar` slots 259/272/273 | Base AI-car effect id `6`, inherited unchanged. |
| adjusted visual active `0x4bc` / outer `0x57c` | pointer | `car_material_or_shape_slot` | init slot `004328b0` | Passed to the inherited texture/material assignment slot after `cargreen.png` is loaded. This is an adjusted visual field. |

No NeuCar2-owned runtime fields were found beyond the constructor's inherited tuning override.

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| ctor | `00432730` | `CtorNeuCar2_3NC2` | Constructs `C3DAICar`, installs NeuCar2 vtables, sets class strings `C3DNEUCAR2`/`C3DNEUCAR2()`, runs `InitObjectNeuCar2`, registers FourCC `3NC2`, writes inherited AI tuning `20.0`, and runs inherited object setup hooks. | non-trivial |
| 7 | `004328b0` | `InitObjectNeuCar2` | Traces `"InitObject()"`, runs `C3DAI::InitObjectAI`, initializes the adjusted animated database/shape path, registers `HIDEFAULT -> car.ase`, loads `cargreen.png`, assigns the texture/material, applies scalar `300.0`, selects `DEFAULT`, and finalizes. | non-trivial |
| 16 | `0040aad0` | `C3DAICar::HandleAICarContact` | Inherited AI-car contact response. | inherited |
| 17 | `0040ac10` | `C3DAICar::ClearAICarContact` | Inherited contact-exit behavior. | inherited |
| 241 | `0040aa50` | `C3DAICar::UpdateAICarContactTimer` | Inherited update/contact timer behavior. | inherited |
| 259 | `0040ac30` | `C3DAICar::PostLoadAICarEffect` | Inherited creation of ambient effect id `6`. | inherited |
| 265 | `0040e340` | `C3DAnimated::ApplyLevelGate` | Inherited animated level gate. | inherited |
| 272 | `0040ac60` | `C3DAICar::ReleaseAICarEffect` | Inherited release of ambient effect id `6`. | inherited |
| 273 | `0040ac80` | `C3DAICar::RestoreAICarEffect` | Inherited restore/recreate of ambient effect id `6`. | inherited |
| vtable 3 slot 2 | `00432830` | scalar deleting destructor | Adjusts from the secondary pointer, runs cleanup helper `00432860`, destroys the tail `OMediaClassStreamer` subobject at outer `0x9b4`, and frees the adjusted allocation when requested. | non-trivial |

## Runtime Behavior

```c
C3DNeuCar2::CtorNeuCar2_3NC2():
    C3DAICar::Ctor()
    install_neucar2_vtables()
    set_runtime_type("C3DNEUCAR2")
    register_class_string("C3DNEUCAR2()")
    InitObjectNeuCar2()
    register_fourcc("3NC2")
    contact_speed_or_tuning = 20.0
```

```c
C3DNeuCar2::InitObjectNeuCar2():
    trace("InitObject()")
    C3DAI::InitObjectAI()
    init_anim3d_database_and_shape()
    register_anim("HIDEFAULT", "car.ase")
    create_texture_slot("cargreen.png", 0)
    assign_texture_to_current_material(car_material_or_shape_slot, 0)
    apply_shape_scalar(300.0)
    set_anim("DEFAULT", true)
```

Player/contact response, horn behavior, contact timer, target resolution, and ambient AI-car effect handling are inherited from `C3DAICar`.

## Constants And Wiring

`C3DNeuCar2` registers `3NC2`, but `3NC2` has no rows in `docs/gam_schema.md`. The class-id scan still names the registrar:

| FourCC | Registrar | Current schema use |
|---|---|---|
| `3NC2` | `C3DNeuCar2` constructor `00432730`, registrar site `004327f2` | No `.gam` instances in the current corpus. |

| Name / Id | Use | Evidence |
|---|---|---|
| `C3DNEUCAR2`, `C3DNEUCAR2()` | Runtime class/object strings. | strings `.data:004effbc`, `.data:004effac`; constructor path |
| `"InitObject()"` | Init trace string. | init slot `004328b0`; string `.data:004eca2c` |
| `HIDEFAULT` | Animation/shape alias. | init slot `004328b0`; string `.data:004ed8e4` |
| `car.ase`, `cargreen.png` | Visual mesh and texture. | init slot `004328b0`; strings `.data:004eff88`, `.data:004effc8` |
| `DEFAULT` | Selected animation/state. | init slot `004328b0`; string `.data:004ee39c` |
| `20.0` | Inherited AI-car tuning override at active `0x604`. | ctor `00432730`; immediate `0x41a00000` |
| `300.0` | Shape/visual scalar applied during init. | init slot `004328b0`; immediate `0x43960000` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| ASE model | `car.ase` | init slot `004328b0`; local file `assets/ase/car.ASE` | Same mesh as `C3DNeuCar`. |
| PNG texture | `cargreen.png` | init slot `004328b0`; local file `assets/png/cargreen.png` | 256x256 paletted PNG; this is the only confirmed visual difference from `C3DNeuCar`. |
| inherited sound/effect | id `6`; `"horn"` contact sound | `C3DAICar` base spec | NeuCar2 keeps the inherited AI-car ambient and contact effects. |

## Confidence

Confidence: Medium

Validation: Static Ghidra class dump, local disassembly of constructor/init/destructor ranges, class-id scan, string-table checks, and local asset metadata only; not runtime-validated.

Open questions:
- Confirm where, if anywhere, `3NC2` objects are spawned outside the serialized `.gam` files.
- Name the inherited visual scalar setter that receives `300.0`.
- Confirm whether the `20.0` inherited AI-car tuning value is speed, acceleration, or a pause/contact-related parameter.

## Notes

- Evidence: `DumpClass.java C3DNeuCar2 /tmp/decomp_C3DNeuCar2.md` (`slots=391`, `owned_methods=1`, `offsets=0`), local objdump window `00432730..004329d0`, string extraction around `004eff88..004effc8`, and local assets `assets/ase/car.ASE` / `assets/png/cargreen.png`.
- `C3DNeuCar2` is behaviorally parallel to `C3DNeuCar`; the executable-level differences recovered here are FourCC/name/vtables and `cargreen.png`.
