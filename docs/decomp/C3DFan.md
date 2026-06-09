# C3DFan

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DFan` |
| FourCC | `3FAN` |
| Base chain | `C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `0049ac20, 0049ac30, 0049b080, 0049b0bc, 0049b0d0` |
| Ctor(s) | factory/constructor installs the vftables and registers the class id (see `docs/_gam_classids.tsv`) |
| Dtor(s) | scalar deleting destructor `vfunc_03_002` at `004184c0` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DFan` is a placeable **mechanisms moving parts** object (family `mechanisms_moving_parts`, wave 6). It walks the class vtable with 3 owned methods; its `.gam`-driven parameters and assets are registered in `InitObject` and listed below.

## Field Map (registered `.gam` properties)

Offsets are from the primary class pointer; types are the `.gam` serialization type ids (`1=string 2=flag4 3=float 4=raw4 6=int`).

| Offset | Type | Property | Source |
|---:|---|---|---|
| `0x17f` | float | `FanSpeed` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x180` | float | `FanRange` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x183` | int | `FanOn` | `InitObject` registrar (`vftable+0x3fc`) |

See `docs/gam_schema.md` for the per-FourCC value ranges/samples across all 35 levels (the field map, constants, and object wiring are data-driven from there).

## Vtable Methods (owned)

| Slot | Address | Role | Behavior |
|---|---|---|---|
| `vfunc_01_007` | `00418540` | InitObject (property + asset registration) | registers 3 `.gam` properties (`FanSpeed`, `FanRange`, `FanOn`); loads `fan.ase`, `fan.png`, `DEFAULT` |
| `vfunc_01_010` | `00418920` | post-init / per-frame logic | runs inherited per-frame logic then this class's update step — touches `FanSpeed`, `FanOn` |
| `vfunc_03_002` | `004184c0` | scalar deleting destructor | destroys the `OMediaClassStreamer` subobject and frees the allocation |

### Decompiled owned methods

**`vfunc_01_007` @ `00418540`** — InitObject (property + asset registration)

Interpreted: reads/writes registered properties `FanSpeed`, `FanRange`, `FanOn`.

```c
void __thiscall C3DFan::vfunc_01_007(C3DFan *this)

{
  C3DFan *pCVar1;
  CGameObject *this_00;
  
  (**(code **)(this->vftable + 0x3f8))(this,s_InitObject___004eca2c);
  C3DAnimated::vfunc_01_007((C3DAnimated *)this);
  (**(code **)(this->vftable + 0x3fc))(s_FanSpeed_004ee3fc,this + 0x17f,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_FanRange_004ee3f0,this + 0x180,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_FanOn_004ee3e8,this + 0x183,6,0);
  pCVar1 = this + -0x30;
  (**(code **)(this[-0x30].vftable + 0x108))();
  (**(code **)(pCVar1->vftable + 0xd8))(s_HIDEFAULT_004ed8e4,s_fan_ase_004ee3e0);
  (**(code **)(pCVar1->vftable + 0xf0))(s_fan_png_004ee3d8,0);
  (**(code **)(pCVar1->vftable + 0xf4))(this[0x12f].vftable,0);
  (**(code **)(pCVar1->vftable + 0xe0))(s_DEFAULT_004ee39c,1);
  CGameObject::vfunc_00_013(this_00);
  return;
}
```

**`vfunc_01_010` @ `00418920`** — post-init / per-frame logic

Interpreted: reads/writes registered properties `FanSpeed`, `FanOn`.

```c
void __thiscall C3DFan::vfunc_01_010(C3DFan *this)

{
  CLocalGameObject::vfunc_00_010((CLocalGameObject *)this);
  if (this[0x183].vftable != (undefined *)0x0) {
    this[0x182].vftable = this[0x17f].vftable;
    return;
  }
  this[0x182].vftable = (undefined *)0x0;
  return;
}
```

**`vfunc_03_002` @ `004184c0`** — scalar deleting destructor

```c
C3DFan * __thiscall C3DFan::vfunc_03_002(C3DFan *this)

{
  byte in_stack_00000004;
  
  FUN_004184f0();
  OMediaClassStreamer::~OMediaClassStreamer((OMediaClassStreamer *)(this + 0x1a9));
  if ((in_stack_00000004 & 1) != 0) {
    FUN_004789a0(this + -0xc);
  }
  return this + -0xc;
}
```

## Assets

| Kind | Name | Present in `assets/` | Notes |
|---|---|---|---|
| ASE/anim | `fan.ase` | ✓ `fan.ASE` | anim tag `HIDEFAULT` |
| PNG texture | `fan.png` | ✓ `fan.png` |  |
| default anim | `DEFAULT` | n/a | flag 1 |

## Validation

Registered properties cross-checked against the shipped `.gam` data for FourCC `3FAN` (`docs/gam_schema.md`):

| Property | Status | Detail |
|---|---|---|
| `FanSpeed` | confirmed in .gam | range/samples: 800 … 2.7e+03 |
| `FanRange` | confirmed in .gam | range/samples: 1e+03 … 3.5e+03 |
| `FanOn` | confirmed in .gam | range/samples: 1 … 1 |

3/3 registered properties are present in shipped `.gam` level data (the rest are recognised tuning/wiring the levels don't currently set). Any `TYPE MISMATCH` would flag an extraction error — none expected.

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java C3DFan` (owned methods decompiled); `.gam` properties and assets resolved from the `InitObject` registrar calls with strings read directly from `Neutron.exe`. `.gam` value ranges cross-referenced via `docs/gam_schema.md`. Behavioral prose is derived from the decompiled bodies above; not runtime-validated.

Open questions:
- Confirm the gameplay semantics of the per-frame/owned override method(s) beyond the decompiled control flow.
- Pin the constructor address and class-id immediate (FourCC).

## Notes

- Generated by `tools/gen_placeable_specs.py` from the Ghidra dump + PE string resolution. Decompiled bodies are included verbatim as primary evidence.
