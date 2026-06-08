# C3DNeuCar

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DNeuCar` |
| Base chain | `C3DAICar -> C3DAI -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004aa05c`, `004aa06c`, `004aa4bc`, `004aa4f8`, `004aa50c` |
| Ctor(s) | constructor/factory block `004324c0`; registers FourCC `3NCA` at `00432582` |
| Dtor(s) | scalar deleting destructor at `004325c0`; cleanup helper at `004325f0`; adjusted destructor thunks at `00432700`, `00432710`, `00432720` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DNeuCar` is a concrete AI-car leaf for the `3NCA` class id. It has no current `.gam` instances in the 35-level corpus, so it appears to be code-spawned, unused by shipped `.gam`, or present for a route not covered by the current files. Its only gameplay-local behavior is initialization: it inherits `C3DAICar` contact/update/effect logic and binds the generic `car.ase` / `car.png` visual.

## Field Map

Offsets are byte offsets from the active `C3DAICar`/AI pointer unless marked `outer`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited active `0x604` / outer `0x6c4` | float | `contact_speed_or_tuning` | ctor `004324c0`; inherited `C3DAICar` | Constructor overwrites the inherited AI-car tuning field with `20.0`, instead of the `400.0` base default. |
| inherited active `0x608` / outer `0x6c8` | int | `current_state` | inherited `C3DAICar` | Inherited constructor seeds state `3`; no NeuCar-owned write found. |
| inherited active `0x87c` / outer `0x93c` | int | `AIState` | inherited `C3DAICar` | Inherited constructor seeds serialized/default state `3`; no NeuCar-owned `.gam` row exists. |
| inherited active `0x8ec` / outer `0x9ac` | handle | `ambient_effect_handle` | `C3DAICar` slots 259/272/273 | Base AI-car effect id `6`, inherited unchanged. |
| adjusted visual active `0x4bc` / outer `0x57c` | pointer | `car_material_or_shape_slot` | init slot `00432640` | Passed to the inherited texture/material assignment slot after `car.png` is loaded. This is an adjusted visual field. |

No NeuCar-owned runtime fields were found beyond the constructor's `20.0` inherited tuning override.

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| ctor | `004324c0` | `CtorNeuCar3NCA` | Constructs `C3DAICar`, installs NeuCar vtables, sets class strings `C3DNEUCAR`/`C3DNEUCAR()`, runs `InitObjectNeuCar`, registers FourCC `3NCA`, writes inherited AI tuning `20.0`, and runs inherited object setup hooks. | non-trivial |
| 7 | `00432640` | `InitObjectNeuCar` | Traces `"InitObject()"`, runs `C3DAI::InitObjectAI`, initializes the adjusted animated database/shape path, registers `HIDEFAULT -> car.ase`, loads `car.png`, assigns the texture/material, applies scalar `300.0`, selects `DEFAULT`, and finalizes. | non-trivial |
| 16 | `0040aad0` | `C3DAICar::HandleAICarContact` | Inherited AI-car contact response. | inherited |
| 17 | `0040ac10` | `C3DAICar::ClearAICarContact` | Inherited contact-exit behavior. | inherited |
| 241 | `0040aa50` | `C3DAICar::UpdateAICarContactTimer` | Inherited update/contact timer behavior. | inherited |
| 259 | `0040ac30` | `C3DAICar::PostLoadAICarEffect` | Inherited creation of ambient effect id `6`. | inherited |
| 265 | `0040e340` | `C3DAnimated::ApplyLevelGate` | Inherited animated level gate. | inherited |
| 272 | `0040ac60` | `C3DAICar::ReleaseAICarEffect` | Inherited release of ambient effect id `6`. | inherited |
| 273 | `0040ac80` | `C3DAICar::RestoreAICarEffect` | Inherited restore/recreate of ambient effect id `6`. | inherited |
| vtable 3 slot 2 | `004325c0` | scalar deleting destructor | Adjusts from the secondary pointer, runs cleanup helper `004325f0`, destroys the tail `OMediaClassStreamer` subobject at outer `0x9b4`, and frees the adjusted allocation when requested. | non-trivial |

## Runtime Behavior

```c
C3DNeuCar::CtorNeuCar3NCA():
    C3DAICar::Ctor()
    install_neucar_vtables()
    set_runtime_type("C3DNEUCAR")
    register_class_string("C3DNEUCAR()")
    InitObjectNeuCar()
    register_fourcc("3NCA")
    contact_speed_or_tuning = 20.0
```

```c
C3DNeuCar::InitObjectNeuCar():
    trace("InitObject()")
    C3DAI::InitObjectAI()
    init_anim3d_database_and_shape()
    register_anim("HIDEFAULT", "car.ase")
    create_texture_slot("car.png", 0)
    assign_texture_to_current_material(car_material_or_shape_slot, 0)
    apply_shape_scalar(300.0)
    set_anim("DEFAULT", true)
```

Player/contact response, horn behavior, contact timer, target resolution, and ambient AI-car effect handling are inherited from `C3DAICar`.

## Constants And Wiring

`C3DNeuCar` registers `3NCA`, but `3NCA` has no rows in `docs/gam_schema.md`. The class-id scan still names the registrar:

| FourCC | Registrar | Current schema use |
|---|---|---|
| `3NCA` | `C3DNeuCar` constructor `004324c0`, registrar site `00432582` | No `.gam` instances in the current corpus. |

| Name / Id | Use | Evidence |
|---|---|---|
| `C3DNEUCAR`, `C3DNEUCAR()` | Runtime class/object strings. | strings `.data:004eff74`, `.data:004eff68`; constructor path |
| `"InitObject()"` | Init trace string. | init slot `00432640`; string `.data:004eca2c` |
| `HIDEFAULT` | Animation/shape alias. | init slot `00432640`; string `.data:004ed8e4` |
| `car.ase`, `car.png` | Visual mesh and texture. | init slot `00432640`; strings `.data:004eff88`, `.data:004eff80` |
| `DEFAULT` | Selected animation/state. | init slot `00432640`; string `.data:004ee39c` |
| `20.0` | Inherited AI-car tuning override at active `0x604`. | ctor `004324c0`; immediate `0x41a00000` |
| `300.0` | Shape/visual scalar applied during init. | init slot `00432640`; immediate `0x43960000` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| ASE model | `car.ase` | init slot `00432640`; local file `assets/ase/car.ASE` | Local ASE metadata references source scene `car01.max` and bitmap `D:\Jimmy\car.png`. |
| PNG texture | `car.png` | init slot `00432640`; local file `assets/png/car.png` | 256x256 paletted PNG. |
| inherited sound/effect | id `6`; `"horn"` contact sound | `C3DAICar` base spec | NeuCar keeps the inherited AI-car ambient and contact effects. |

## Confidence

Confidence: Medium

Validation: Static Ghidra class dump, targeted decompilation, local disassembly of constructor/init/destructor ranges, class-id scan, string-table checks, and local asset metadata only; not runtime-validated.

Open questions:
- Confirm where, if anywhere, `3NCA` objects are spawned outside the serialized `.gam` files.
- Name the inherited visual scalar setter that receives `300.0`.
- Confirm whether the `20.0` inherited AI-car tuning value is speed, acceleration, or a pause/contact-related parameter.

## Notes

- Evidence: `DumpClass.java C3DNeuCar /tmp/decomp_C3DNeuCar.md` (`slots=391`, `owned_methods=1`, `offsets=0`), `DumpFunctions.java /tmp/decomp_C3DNeuCar_raw.md 004324c0 004325c0 00432640`, local objdump window `004324c0..00432760`, string extraction around `004eff68..004eff88`, and local assets `assets/ase/car.ASE` / `assets/png/car.png`.
- The similarly named `C3DNeuCar2` is a separate `3NC2` leaf with a different constructor/vtable set and should be documented separately.
