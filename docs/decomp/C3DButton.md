# C3DButton

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DButton` |
| Base chain | `C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `00493b50, 00493b60, 00493fb0, 00493fec, 00494000` |
| Ctor(s) | factory/constructor installs the vftables and registers the class id (see `docs/_gam_classids.tsv`) |
| Dtor(s) | inherited deleting destructor (none owned) |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DButton` is a placeable **mechanisms moving parts** object (family `mechanisms_moving_parts`, wave 6). It walks the class vtable with 2 owned methods; its `.gam`-driven parameters and assets are registered in `InitObject` and listed below.

## Field Map (registered `.gam` properties)

Offsets are from the primary class pointer; types are the `.gam` serialization type ids (`1=string 2=flag4 3=float 4=raw4 6=int`).

| Offset | Type | Property | Source |
|---:|---|---|---|
| `0x17f` | float | `Red` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x180` | float | `Green` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x181` | float | `Blue` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x183` | int | `ButtonAvailable` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x184` | int | `NASound` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x185` | int | `AvailSound` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x186` | string | `ActivateButton` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x1ea` | int | `NewTaskState` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x1eb` | int | `Toggle` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x19f` | string | `Down.ase` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x1b8` | string | `Up.ase` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x1d1` | string | `UpDown.Png` | `InitObject` registrar (`vftable+0x3fc`) |

See `docs/gam_schema.md` for the per-FourCC value ranges/samples across all 35 levels (the field map, constants, and object wiring are data-driven from there).

## Vtable Methods (owned)

| Slot | Address | Role | Behavior |
|---|---|---|---|
| `vfunc_01_007` | `004119d0` | InitObject (property + asset registration) | registers 12 `.gam` properties (`Red`, `Green`, `Blue`, `ButtonAvailable`, `NASound`, `AvailSound`, `ActivateButton`, `NewTaskState`, `Toggle`, `Down.ase`, `Up.ase`, `UpDown.Png`); loads `this + 0x19f`, `this + 0x1b8`, `this + 0x1d1`, `UP` |
| `vfunc_01_257` | `00411ef0` | owned override | see decompiled body |

### Decompiled owned methods

**`vfunc_01_007` @ `004119d0`** — InitObject (property + asset registration)

```c
void __thiscall C3DButton::vfunc_01_007(C3DButton *this)

{
  C3DButton *pCVar1;
  CGameObject *this_00;
  
  (**(code **)(this->vftable + 0x3f8))(this,s_InitObject___004eca2c);
  C3DAnimated::vfunc_01_007((C3DAnimated *)this);
  (**(code **)(this->vftable + 0x3fc))(&PTR_DAT_004ed55c,this + 0x17f,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_Green_004ed554,this + 0x180,3,0);
  (**(code **)(this->vftable + 0x3fc))(&DAT_004ed54c,this + 0x181,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_ButtonAvailable_004eda2c,this + 0x183,6,0);
  (**(code **)(this->vftable + 0x3fc))(s_NASound_004eda24,this + 0x184,6,0);
  (**(code **)(this->vftable + 0x3fc))(s_AvailSound_004eda18,this + 0x185,6,0);
  (**(code **)(this->vftable + 0x3fc))(s_ActivateButton_004eda08,this + 0x186,1,0);
  (**(code **)(this->vftable + 0x3fc))(s_NewTaskState_004ed9f8,this + 0x1ea,6,0);
  (**(code **)(this->vftable + 0x3fc))(s_Toggle_004ed9f0,this + 0x1eb,6,0);
  (**(code **)(this->vftable + 0x3fc))(s_Down_ase_004ed9e4,this + 0x19f,1,0);
  (**(code **)(this->vftable + 0x3fc))(s_Up_ase_004ed9dc,this + 0x1b8,1,0);
  (**(code **)(this->vftable + 0x3fc))(s_UpDown_Png_004ed9d0,this + 0x1d1,1,0);
  pCVar1 = this + -0x30;
  (**(code **)(this[-0x30].vftable + 0x108))();
  (**(code **)(pCVar1->vftable + 0xd8))(s_HIDOWN_004ed9c8,this + 0x19f);
  (**(code **)(pCVar1->vftable + 0xd8))(&DAT_004ed9c0,this + 0x1b8);
  (**(code **)(pCVar1->vftable + 0xf0))(this + 0x1d1,0);
  (**(code **)(pCVar1->vftable + 0xf4))(this[0x12f].vftable,0);
  (**(code **)(this->vftable + 0x110))(0x43960000);
  (**(code **)(pCVar1->vftable + 0xe0))(&DAT_004ed9bc,1);
  CGameObject::vfunc_00_013(this_00);
  return;
}
```

**`vfunc_01_257` @ `00411ef0`** — owned override

```c
void __thiscall C3DButton::vfunc_01_257(C3DButton *this)

{
  CLocalGameObject::vfunc_00_257((CLocalGameObject *)this);
  (**(code **)(this->vftable + 0x110))(0x43960000);
  (**(code **)(this->vftable + 0x264))(0x43960000);
  return;
}
```

## Assets

| Kind | Name | Notes |
|---|---|---|
| ASE/anim | `this + 0x19f` | anim tag `HIDOWN` |
| ASE/anim | `this + 0x1b8` | anim tag `HIUP` |
| PNG texture | `this + 0x1d1` |  |
| default anim | `UP` | flag 1 |

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java C3DButton` (owned methods decompiled); `.gam` properties and assets resolved from the `InitObject` registrar calls with strings read directly from `Neutron.exe`. `.gam` value ranges cross-referenced via `docs/gam_schema.md`. Behavioral prose is derived from the decompiled bodies above; not runtime-validated.

Open questions:
- Confirm the gameplay semantics of the per-frame/owned override method(s) beyond the decompiled control flow.
- Pin the constructor address and class-id immediate (FourCC).

## Notes

- Generated by `tools/gen_placeable_specs.py` from the Ghidra dump + PE string resolution. Decompiled bodies are included verbatim as primary evidence.
