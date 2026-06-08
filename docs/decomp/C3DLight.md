# C3DLight

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DLight` |
| Base chain | `OMediaLight -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004d2c10, 004d2c20, 004d3070, 004d3084` |
| Ctor(s) | factory/constructor installs the vftables and registers the class id (see `docs/_gam_classids.tsv`) |
| Dtor(s) | inherited deleting destructor (none owned) |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DLight` is a placeable **world props terrain** object (family `world_props_terrain`, wave 7). It walks the class vtable with 2 owned methods; its `.gam`-driven parameters and assets are registered in `InitObject` and listed below.

## Field Map (registered `.gam` properties)

Offsets are from the primary class pointer; types are the `.gam` serialization type ids (`1=string 2=flag4 3=float 4=raw4 6=int`).

| Offset | Type | Property | Source |
|---:|---|---|---|
| `0x12f` | raw4 | `LightType` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x12d` | float | `LightRange` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x12e` | float | `CAttenuation` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x130` | float | `DifRed` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x131` | float | `DifGreen` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x132` | float | `DifBlue` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x133` | float | `SpecRed` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x134` | float | `SpecGreen` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x135` | float | `SpecBlue` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x136` | float | `AmbRed` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x137` | float | `AmbGreen` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x138` | float | `AmbBlue` | `InitObject` registrar (`vftable+0x3fc`) |

See `docs/gam_schema.md` for the per-FourCC value ranges/samples across all 35 levels (the field map, constants, and object wiring are data-driven from there).

## Vtable Methods (owned)

| Slot | Address | Role | Behavior |
|---|---|---|---|
| `vfunc_01_007` | `00461bb0` | InitObject (property + asset registration) | registers 12 `.gam` properties (`LightType`, `LightRange`, `CAttenuation`, `DifRed`, `DifGreen`, `DifBlue`, `SpecRed`, `SpecGreen`, `SpecBlue`, `AmbRed`, `AmbGreen`, `AmbBlue`) |
| `vfunc_03_043` | `00461ba0` | owned override | see decompiled body |

### Decompiled owned methods

**`vfunc_01_007` @ `00461bb0`** — InitObject (property + asset registration)

```c
void __thiscall C3DLight::vfunc_01_007(C3DLight *this)

{
  CGameObject *this_00;
  
  (**(code **)(this->vftable + 0x3f8))(this,s_InitObject___004eca2c);
  CLocalGameObject::vfunc_00_007((CLocalGameObject *)this);
  (**(code **)(this[-0x41].vftable + 0xac))(0x334c4947);
  this[0x101].vftable = this[-0x23].vftable;
  (**(code **)(this[-0x41].vftable + 0x2c))(DAT_00509a4c);
  (**(code **)(this[-0x41].vftable + 0x34))(DAT_00509a30);
  (**(code **)(this->vftable + 0x3fc))(s_LightType_004f4bc8,this + 0x12f,4,0);
  (**(code **)(this->vftable + 0x3fc))(s_LightRange_004f4bbc,this + 0x12d,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_CAttenuation_004f4bac,this + 0x12e,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_DifRed_004f4ba4,this + 0x130,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_DifGreen_004f4b98,this + 0x131,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_DifBlue_004f4b90,this + 0x132,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_SpecRed_004f4b88,this + 0x133,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_SpecGreen_004f4b7c,this + 0x134,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_SpecBlue_004f4b70,this + 0x135,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_AmbRed_004f4b68,this + 0x136,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_AmbGreen_004f4b5c,this + 0x137,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_AmbBlue_004f4b54,this + 0x138,3,0);
  (**(code **)(this->vftable + 0x404))();
  CGameObject::vfunc_00_013(this_00);
  return;
}
```

**`vfunc_03_043` @ `00461ba0`** — owned override

```c
void __thiscall C3DLight::vfunc_03_043(C3DLight *this)

{
  undefined *in_stack_00000004;
  
  this[0x1e].vftable = in_stack_00000004;
  this[0x142].vftable = in_stack_00000004;
  return;
}
```

## Assets

No direct ASE/PNG/anim references in `InitObject` (inherited visual path or runtime-assigned).

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java C3DLight` (owned methods decompiled); `.gam` properties and assets resolved from the `InitObject` registrar calls with strings read directly from `Neutron.exe`. `.gam` value ranges cross-referenced via `docs/gam_schema.md`. Behavioral prose is derived from the decompiled bodies above; not runtime-validated.

Open questions:
- Confirm the gameplay semantics of the per-frame/owned override method(s) beyond the decompiled control flow.
- Pin the constructor address and class-id immediate (FourCC).

## Notes

- Generated by `tools/gen_placeable_specs.py` from the Ghidra dump + PE string resolution. Decompiled bodies are included verbatim as primary evidence.
