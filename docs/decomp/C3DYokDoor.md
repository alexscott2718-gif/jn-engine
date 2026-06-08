# C3DYokDoor

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DYokDoor` |
| Base chain | `C3DAnimated -> C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004bf5e4, 004bf5f4, 004bfa44, 004bfa80, 004bfa94` |
| Ctor(s) | factory/constructor installs the vftables and registers the class id (see `docs/_gam_classids.tsv`) |
| Dtor(s) | inherited deleting destructor (none owned) |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DYokDoor` is a placeable **mechanisms moving parts** object (family `mechanisms_moving_parts`, wave 6). It walks the class vtable with 4 owned methods; its `.gam`-driven parameters and assets are registered in `InitObject` and listed below.

## Field Map (registered `.gam` properties)

Offsets are from the primary class pointer; types are the `.gam` serialization type ids (`1=string 2=flag4 3=float 4=raw4 6=int`).

| Offset | Type | Property | Source |
|---:|---|---|---|
| `0x17f` | int | `ItemClosed` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x181` | string | `Next` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x1b5` | float | `DoorSpeed` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x1b6` | float | `OpenTime` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x1b9` | float | `OpenAmount` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x19a` | string | `OmtDatabase` | `InitObject` registrar (`vftable+0x3fc`) |
| `0x1b3` | int | `OmtIndex` | `InitObject` registrar (`vftable+0x3fc`) |

See `docs/gam_schema.md` for the per-FourCC value ranges/samples across all 35 levels (the field map, constants, and object wiring are data-driven from there).

## Vtable Methods (owned)

| Slot | Address | Role | Behavior |
|---|---|---|---|
| `vfunc_01_007` | `00449f70` | InitObject (property + asset registration) | registers 7 `.gam` properties (`ItemClosed`, `Next`, `DoorSpeed`, `OpenTime`, `OpenAmount`, `OmtDatabase`, `OmtIndex`) |
| `vfunc_01_010` | `00449f10` | post-init / per-frame logic | runs inherited per-frame logic then this class's update step — touches `ItemClosed`, `OpenAmount` |
| `vfunc_01_259` | `0044a060` | owned override | see decompiled body — touches `OmtDatabase`, `OmtIndex` |
| `vfunc_01_266` | `0044a2a0` | owned override | see decompiled body — touches `Next`, `DoorSpeed` |

### Decompiled owned methods

**`vfunc_01_007` @ `00449f70`** — InitObject (property + asset registration)

```c
void __thiscall C3DYokDoor::vfunc_01_007(C3DYokDoor *this)

{
  CGameObject *this_00;
  
  (**(code **)(this->vftable + 0x3f8))(this,s_InitObject___004eca2c);
  C3DAnimated::vfunc_01_007((C3DAnimated *)this);
  (**(code **)(this->vftable + 0x3fc))(s_ItemClosed_004ee2e0,this + 0x17f,6,0);
  (**(code **)(this->vftable + 0x3fc))(&DAT_004edd18,this + 0x181,1,0);
  (**(code **)(this->vftable + 0x3fc))(s_DoorSpeed_004ee2d4,this + 0x1b5,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_OpenTime_004ee2c8,this + 0x1b6,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_OpenAmount_004ee2bc,this + 0x1b9,3,0);
  (**(code **)(this->vftable + 0x3fc))(s_OmtDatabase_004eccd0,this + 0x19a,1,0);
  (**(code **)(this->vftable + 0x3fc))(s_OmtIndex_004eccc4,this + 0x1b3,6,0);
  *(undefined1 *)&this[0x15d].vftable = 0;
  (**(code **)(this->vftable + 0x2c))();
  CGameObject::vfunc_00_013(this_00);
  return;
}
```

**`vfunc_01_010` @ `00449f10`** — post-init / per-frame logic

```c
void __thiscall C3DYokDoor::vfunc_01_010(C3DYokDoor *this)

{
  undefined4 *puVar1;
  undefined1 local_10 [16];
  
  CLocalGameObject::vfunc_00_010((CLocalGameObject *)this);
  this[0x180].vftable = this[0x17f].vftable;
  if (this[0x17f].vftable == (undefined *)0x0) {
    puVar1 = (undefined4 *)(**(code **)(this->vftable + 0x164))(local_10);
    (**(code **)(this->vftable + 0x318))
              (*puVar1,(float)puVar1[1] + (float)this[0x1b9].vftable,puVar1[2]);
  }
  return;
}
```

**`vfunc_01_259` @ `0044a060`** — owned override

```c
void __thiscall C3DYokDoor::vfunc_01_259(C3DYokDoor *this)

{
  undefined *puVar1;
  undefined *puVar2;
  undefined4 uVar3;
  
  C3DAnimated::vfunc_01_259((C3DAnimated *)this);
  puVar2 = (undefined *)FUN_0046a910(this + 0x19a);
  this[0x12a].vftable = puVar2;
  puVar1 = this[-0x30].vftable;
  uVar3 = FUN_00477ba0(puVar2,this[0x1b3].vftable);
  (**(code **)(puVar1 + 0xac))(uVar3);
  uVar3 = (**(code **)(this[-0x30].vftable + 0xb0))();
  FUN_00477550(uVar3);
  (**(code **)(this[-0x30].vftable + 0xcc))();
  return;
}
```

**`vfunc_01_266` @ `0044a2a0`** — owned override

```c
void __thiscall C3DYokDoor::vfunc_01_266(C3DYokDoor *this)

{
  char cVar1;
  undefined *puVar2;
  int iVar3;
  int *piVar4;
  undefined *in_stack_00000004;
  
  if ((float)this[0x1b4].vftable <= 0.0) {
    if (this[0x180].vftable == in_stack_00000004) {
      return;
    }
    this[0x1b4].vftable = this[0x1b5].vftable;
    if (this[0x1ba].vftable == (undefined *)0xffffffff) {
      puVar2 = (undefined *)
               FUN_004589c0(-(uint)(this != (C3DYokDoor *)0xc0) & (uint)this,0xffffffff,0x3b,1);
      this[0x1ba].vftable = puVar2;
    }
  }
  else {
    if (this[0x180].vftable == in_stack_00000004) {
      return;
    }
    this[0x1b4].vftable = (undefined *)((float)this[0x1b5].vftable - (float)this[0x1b4].vftable);
  }
  iVar3 = __strcmpi((char *)(this + 0x181),&DAT_004eca6c);
  if ((iVar3 != 0) && (piVar4 = (int *)FUN_00474070(this + 0x181), piVar4 != (int *)0x0)) {
    cVar1 = (**(code **)(*piVar4 + 0x18))(s_C3DAITRIGGER_004ecd4c);
    if (cVar1 == '\0') {
      cVar1 = (**(code **)(*piVar4 + 0x18))(s_C3DCUTSCENECAMERA_004edefc);
      if (cVar1 != '\0') {
        (**(code **)(piVar4[-0x32] + 0xd8))();
      }
    }
    else {
      FUN_0040c300(-(uint)(this != (C3DYokDoor *)0xc0) & (uint)this);
    }
  }
  if (in_stack_00000004 == (undefined *)0x0) {
    this[0x180].vftable = (undefined *)0x0;
    return;
  }
  if (in_stack_00000004 == (undefined *)0x1) {
    this[0x180].vftable = (undefined *)0x1;
    return;
  }
  this[0x180].vftable = (undefined *)(uint)(this[0x180].vftable == (undefined *)0x0);
  return;
}
```

## Assets

No direct ASE/PNG/anim references in `InitObject` (inherited visual path or runtime-assigned).

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java C3DYokDoor` (owned methods decompiled); `.gam` properties and assets resolved from the `InitObject` registrar calls with strings read directly from `Neutron.exe`. `.gam` value ranges cross-referenced via `docs/gam_schema.md`. Behavioral prose is derived from the decompiled bodies above; not runtime-validated.

Open questions:
- Confirm the gameplay semantics of the per-frame/owned override method(s) beyond the decompiled control flow.
- Pin the constructor address and class-id immediate (FourCC).

## Notes

- Generated by `tools/gen_placeable_specs.py` from the Ghidra dump + PE string resolution. Decompiled bodies are included verbatim as primary evidence.
