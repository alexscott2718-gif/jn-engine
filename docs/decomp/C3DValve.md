# C3DValve

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DValve` |
| Base chain | `C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004bde10, 004bde20, 004be270, 004be2ac, 004be2c0` |
| Ctor(s) | factory/constructor installs the vftables and registers the class id (see `docs/_gam_classids.tsv`) |
| Dtor(s) | inherited deleting destructor (none owned) |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DValve` is a placeable **mechanisms moving parts** object (family `mechanisms_moving_parts`, wave 6). It walks the class vtable with 7 owned methods; its `.gam`-driven parameters and assets are registered in `InitObject` and listed below.

## Field Map (registered `.gam` properties)

Offsets are from the primary class pointer; types are the `.gam` serialization type ids (`1=string 2=flag4 3=float 4=raw4 6=int`).

| Offset | Type | Property | Source |
|---:|---|---|---|
| `0x19c` | float | `SteamPeriod` | `InitObject` registrar (`vftable+0x3fc`) |

See `docs/gam_schema.md` for the per-FourCC value ranges/samples across all 35 levels (the field map, constants, and object wiring are data-driven from there).

## Vtable Methods (owned)

| Slot | Address | Role | Behavior |
|---|---|---|---|
| `vfunc_01_007` | `00448a30` | InitObject (property + asset registration) | registers 1 `.gam` property (`SteamPeriod`); loads `DEFAULT` |
| `vfunc_01_010` | `00448ba0` | post-init / per-frame logic | runs inherited per-frame logic then this class's update step |
| `vfunc_01_016` | `00448ab0` | owned override | see decompiled body |
| `vfunc_01_017` | `00448b20` | owned override | see decompiled body |
| `vfunc_01_259` | `00448b50` | owned override | see decompiled body |
| `vfunc_04_067` | `004489d0` | owned override | see decompiled body |
| `vfunc_04_073` | `00448bd0` | owned override | see decompiled body |

### Decompiled owned methods

**`vfunc_01_007` @ `00448a30`** — InitObject (property + asset registration)

```c
void __thiscall C3DValve::vfunc_01_007(C3DValve *this)

{
  CGameObject *this_00;
  
  (**(code **)(this->vftable + 0x3f8))(this,s_InitObject___004eca2c);
  C3DAnimated::vfunc_01_007((C3DAnimated *)this);
  (**(code **)(this->vftable + 0x3fc))(s_SteamPeriod_004ee8b0,this + 0x19c,3,0);
  (**(code **)(this[-0x30].vftable + 0x108))();
  *(undefined1 *)&this[300].vftable = 0;
  (**(code **)(this->vftable + 0x110))(0x42a00000);
  (**(code **)(this[-0x30].vftable + 0xe0))(s_DEFAULT_004ee39c,1);
  CGameObject::vfunc_00_013(this_00);
  return;
}
```

**`vfunc_01_010` @ `00448ba0`** — post-init / per-frame logic

```c
void __thiscall C3DValve::vfunc_01_010(C3DValve *this)

{
  CLocalGameObject::vfunc_00_010((CLocalGameObject *)this);
  this[0x19a].vftable = (undefined *)0x0;
  return;
}
```

**`vfunc_01_016` @ `00448ab0`** — owned override

```c
void __thiscall C3DValve::vfunc_01_016(C3DValve *this)

{
  char cVar1;
  int *piVar2;
  int *in_stack_00000004;
  
  CGameObject::vfunc_00_016((CGameObject *)this);
  if (this[0x19b].vftable != (undefined *)0x0) {
    cVar1 = (**(code **)(*in_stack_00000004 + 0x18))(s_C3DJIMMY_004ecb20);
    if (cVar1 != '\0') {
      if (in_stack_00000004 == (int *)0x0) {
        piVar2 = (int *)0x0;
      }
      else {
        piVar2 = in_stack_00000004 + -0x30;
      }
      (**(code **)(*piVar2 + 0x1a0))(0xc1200000,0xfffffff6,0x40a00000);
      FUN_00458980(0xffffffff,0xbc,0);
      this[0x19f].vftable = (undefined *)in_stack_00000004;
    }
  }
  return;
}
```

**`vfunc_01_017` @ `00448b20`** — owned override

```c
void __thiscall C3DValve::vfunc_01_017(C3DValve *this)

{
  char cVar1;
  int *in_stack_00000004;
  
  CGameObject::vfunc_00_017((CGameObject *)this);
  cVar1 = (**(code **)(*in_stack_00000004 + 0x18))(s_C3DJIMMY_004ecb20);
  if (cVar1 != '\0') {
    this[0x19f].vftable = (undefined *)0x0;
  }
  return;
}
```

**`vfunc_01_259` @ `00448b50`** — owned override

```c
void __thiscall C3DValve::vfunc_01_259(C3DValve *this)

{
  C3DAnimated::vfunc_01_259((C3DAnimated *)this);
  if (this[0x19b].vftable != (undefined *)0x0) {
    (**(code **)(this[-0x30].vftable + 0x120))();
    (**(code **)(this->vftable + 0x110))(0x430c0000);
    return;
  }
  (**(code **)(this[-0x30].vftable + 0x124))();
  (**(code **)(this->vftable + 0x110))(0x430c0000);
  return;
}
```

**`vfunc_04_067` @ `004489d0`** — owned override

```c
void __thiscall C3DValve::vfunc_04_067(C3DValve *this)

{
  undefined *puVar1;
  
  if (*(char *)((int)&this[0x18d].vftable + 1) == '\0') {
    puVar1 = this->vftable;
    *(undefined1 *)((int)&this[0x18d].vftable + 1) = 1;
    (**(code **)(puVar1 + 0xd8))(s_HIDEFAULT_004ed8e4,s_valve_ase_004f16a4);
    (**(code **)(this->vftable + 0xf0))(s_valve_png_004f1698,0);
    (**(code **)(this->vftable + 0xf4))(this[0x15f].vftable,0);
    (**(code **)(this->vftable + 0xe0))(&DAT_004ed040,1);
  }
  return;
}
```

**`vfunc_04_073` @ `00448bd0`** — owned override

```c
void __thiscall C3DValve::vfunc_04_073(C3DValve *this)

{
  if (this[0x1af].vftable != (undefined *)0xffffffff) {
    FUN_00458a00(this[0x1af].vftable,0);
    this[0x1af].vftable = (undefined *)0xffffffff;
  }
  return;
}
```

## Assets

| Kind | Name | Notes |
|---|---|---|
| default anim | `DEFAULT` | flag 1 |

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java C3DValve` (owned methods decompiled); `.gam` properties and assets resolved from the `InitObject` registrar calls with strings read directly from `Neutron.exe`. `.gam` value ranges cross-referenced via `docs/gam_schema.md`. Behavioral prose is derived from the decompiled bodies above; not runtime-validated.

Open questions:
- Confirm the gameplay semantics of the per-frame/owned override method(s) beyond the decompiled control flow.
- Pin the constructor address and class-id immediate (FourCC).

## Notes

- Generated by `tools/gen_placeable_specs.py` from the Ghidra dump + PE string resolution. Decompiled bodies are included verbatim as primary evidence.
