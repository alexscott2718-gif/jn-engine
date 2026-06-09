# C3DSparkWire

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DSparkWire` |
| FourCC | `3SPA` |
| Base chain | `C3DTesla -> C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004b5ef4, 004b5f04, 004b6354, 004b6390, 004b63a4` |
| Ctor(s) | factory/constructor installs the vftables and registers the class id (see `docs/_gam_classids.tsv`) |
| Dtor(s) | inherited deleting destructor (none owned) |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DSparkWire` is a placeable **effects triggers nav cameras sound** object (family `effects_triggers_nav_cameras_sound`, wave 8). It walks the class vtable with 2 owned methods; its `.gam`-driven parameters and assets are registered in `InitObject` and listed below.

## Field Map (registered `.gam` properties)

Offsets are from the primary class pointer; types are the `.gam` serialization type ids (`1=string 2=flag4 3=float 4=raw4 6=int`).

| Offset | Type | Property | Source |
|---:|---|---|---|
| `0x181` | int | `ItemActive` | `InitObject` registrar (`vftable+0x3fc`) |

See `docs/gam_schema.md` for the per-FourCC value ranges/samples across all 35 levels (the field map, constants, and object wiring are data-driven from there).

## Vtable Methods (owned)

| Slot | Address | Role | Behavior |
|---|---|---|---|
| `vfunc_01_007` | `00441440` | InitObject (property + asset registration) | registers 1 `.gam` property (`ItemActive`); loads `powerline.ase`, `powerlinestop.ase`, `powerline01.png`, `powerline02.png`, `powerline01.png`, `powerline02.png`, `WALK` |
| `vfunc_01_010` | `00441400` | owned override | see decompiled body |

### Decompiled owned methods

**`vfunc_01_007` @ `00441440`** — InitObject (property + asset registration)

Interpreted: reads/writes registered property `ItemActive`.

```c
void __thiscall C3DSparkWire::vfunc_01_007(C3DSparkWire *this)

{
  C3DSparkWire *pCVar1;
  undefined2 extraout_var;
  CGameObject *this_00;
  
  (**(code **)(this->vftable + 0x3f8))(this,s_InitObject___004eca2c);
  C3DAnimated::vfunc_01_007((C3DAnimated *)this);
  (**(code **)(this->vftable + 0x3fc))(s_ItemActive_004ef974,this + 0x181,6,0);
  pCVar1 = this + -0x30;
  (**(code **)(this[-0x30].vftable + 0x108))();
  *(undefined1 *)&this[300].vftable = 0;
  (**(code **)(pCVar1->vftable + 0xd8))(s_HIWALK_004ed808,s_powerline_ase_004f0d38);
  (**(code **)(pCVar1->vftable + 0xd8))(s_HISTOP_004ec9f4,s_powerlinestop_ase_004f0d24);
  (**(code **)(pCVar1->vftable + 0xf0))(s_powerline01_png_004f0d14,0);
  (**(code **)(pCVar1->vftable + 0xf0))(s_powerline02_png_004f0d04,1);
  (**(code **)(pCVar1->vftable + 0xf0))(s_powerline01_png_004f0d14,2);
  (**(code **)(pCVar1->vftable + 0xf0))(s_powerline02_png_004f0d04,3);
  (**(code **)(pCVar1->vftable + 0xf4))
            (this[0x12f].vftable,CONCAT22(extraout_var,*(undefined2 *)&this[0x17f].vftable));
  (**(code **)(this->vftable + 0x110))(0x42700000);
  (**(code **)(pCVar1->vftable + 0xe0))(&DAT_004eca54,1);
  CGameObject::vfunc_00_013(this_00);
  return;
}
```

**`vfunc_01_010` @ `00441400`** — owned override

Interpreted: fires an exit/deactivate action (slot `0x58`).

```c
void __thiscall C3DSparkWire::vfunc_01_010(C3DSparkWire *this)

{
  C3DTesla::vfunc_01_010((C3DTesla *)this);
  if (this[0x182].vftable == (undefined *)0x0) {
    (**(code **)(this[-0x30].vftable + 0x58))(1);
  }
  else if (this[-0x14].vftable != (undefined *)0x0) {
    this[-0x14].vftable = (undefined *)0x1;
    (**(code **)(this[-0x30].vftable + 0x58))(0);
    return;
  }
  return;
}
```

## Assets

| Kind | Name | Present in `assets/` | Notes |
|---|---|---|---|
| ASE/anim | `powerline.ase` | ✓ `powerline.ASE` | anim tag `HIWALK` |
| ASE/anim | `powerlinestop.ase` | ✓ `powerlinestop.ASE` | anim tag `HISTOP` |
| PNG texture | `powerline01.png` | ✓ `powerline01.png` |  |
| PNG texture | `powerline02.png` | ✓ `powerline02.png` |  |
| PNG texture | `powerline01.png` | ✓ `powerline01.png` |  |
| PNG texture | `powerline02.png` | ✓ `powerline02.png` |  |
| default anim | `WALK` | n/a | flag 1 |

## Validation

Registered properties cross-checked against the shipped `.gam` data for FourCC `3SPA` (`docs/gam_schema.md`):

| Property | Status | Detail |
|---|---|---|
| `ItemActive` | confirmed in .gam | range/samples: 1 … 1 |

1/1 registered properties are present in shipped `.gam` level data (the rest are recognised tuning/wiring the levels don't currently set). Any `TYPE MISMATCH` would flag an extraction error — none expected.

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java C3DSparkWire` (owned methods decompiled); `.gam` properties and assets resolved from the `InitObject` registrar calls with strings read directly from `Neutron.exe`. `.gam` value ranges cross-referenced via `docs/gam_schema.md`. Behavioral prose is derived from the decompiled bodies above; not runtime-validated.

Open questions:
- Confirm the gameplay semantics of the per-frame/owned override method(s) beyond the decompiled control flow.
- Pin the constructor address and class-id immediate (FourCC).

## Notes

- Generated by `tools/gen_placeable_specs.py` from the Ghidra dump + PE string resolution. Decompiled bodies are included verbatim as primary evidence.
