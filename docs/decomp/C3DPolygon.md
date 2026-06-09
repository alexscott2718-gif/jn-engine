# C3DPolygon

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DPolygon` |
| FourCC | `3POL` |
| Base chain | `OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004d36dc, 004d36ec, 004d3b3c, 004d3b50` |
| Ctor(s) | factory/constructor installs the vftables and registers the class id (see `docs/_gam_classids.tsv`) |
| Dtor(s) | inherited deleting destructor (none owned) |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DPolygon` is a placeable **world props terrain** object (family `world_props_terrain`, wave 7). It walks the class vtable with 5 owned methods; its `.gam`-driven parameters and assets are registered in `InitObject` and listed below.

## Field Map (registered `.gam` properties)

No own `.gam` properties registered in `InitObject` (inherits its parent's property set, or is created at runtime rather than placed). See `docs/gam_schema.md` for any inherited properties.

## Vtable Methods (owned)

| Slot | Address | Role | Behavior |
|---|---|---|---|
| `vfunc_01_007` | `00462d90` | InitObject (property + asset registration) | inherited init + class registration |
| `vfunc_01_008` | `00462de0` | reset / reinit | see decompiled body |
| `vfunc_01_241` | `00463090` | reset / reinit | see decompiled body |
| `vfunc_03_048` | `00462d80` | owned override | see decompiled body |
| `vfunc_03_050` | `00462e20` | reset / reinit | see decompiled body |

### Decompiled owned methods

**`vfunc_01_007` @ `00462d90`** — InitObject (property + asset registration)

```c
void __thiscall C3DPolygon::vfunc_01_007(C3DPolygon *this)

{
  CGameObject *this_00;
  
  (**(code **)(this->vftable + 0x3f8))(this,s_InitObject___004eca2c);
  CLocalGameObject::vfunc_00_007((CLocalGameObject *)this);
  (**(code **)(this[-0x32].vftable + 0x2c))(DAT_00509a4c);
  (**(code **)(this[-0x32].vftable + 0x34))(DAT_00509a30);
  CGameObject::vfunc_00_013(this_00);
  return;
}
```

**`vfunc_01_008` @ `00462de0`** — reset / reinit

```c
void __thiscall C3DPolygon::vfunc_01_008(C3DPolygon *this)

{
  CGameObject *this_00;
  
  (**(code **)(this->vftable + 0x3f8))(this,s_UnInitObject___004edba4);
  (**(code **)(this[-0x32].vftable + 0xac))(0);
  CLocalGameObject::vfunc_00_008((CLocalGameObject *)this);
  CGameObject::vfunc_00_013(this_00);
  return;
}
```

**`vfunc_01_241` @ `00463090`** — reset / reinit

```c
void __thiscall C3DPolygon::vfunc_01_241(C3DPolygon *this)

{
  CGameObject::vfunc_00_013((CGameObject *)this);
  CLocalGameObject::vfunc_00_241((CLocalGameObject *)this);
  return;
}
```

**`vfunc_03_048` @ `00462d80`** — owned override

```c
void __thiscall C3DPolygon::vfunc_03_048(C3DPolygon *this)

{
  undefined *in_stack_00000004;
  
  this[0x1e].vftable = in_stack_00000004;
  this[0x133].vftable = in_stack_00000004;
  return;
}
```

**`vfunc_03_050` @ `00462e20`** — reset / reinit

```c
/* WARNING: Removing unreachable block (ram,0x00462ead) */

void __thiscall C3DPolygon::vfunc_03_050(C3DPolygon *this)

{
  char cVar1;
  undefined4 *puVar2;
  undefined4 uVar3;
  uint uVar4;
  CGameObject *this_00;
  undefined4 *puVar5;
  undefined1 in_stack_00000004;
  undefined4 *in_stack_00000008;
  uint in_stack_0000000c;
  undefined4 in_stack_00000010;
  float in_stack_00000014;
  char *pcStack_34;
  undefined4 *puStack_30;
  undefined8 local_2c;
  undefined1 *puStack_10;
  void *pvStack_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  puVar5 = in_stack_00000008;
  puStack_8 = &LAB_0048bf88;
  pvStack_c = ExceptionList;
  local_4 = 0;
  puStack_30 = (undefined4 *)&DAT_004c3260;
  if (in_stack_00000008 != (undefined4 *)0x0) {
    puStack_30 = in_stack_00000008;
  }
  local_2c = (double)in_stack_00000014;
  pcStack_34 = s_InitPolygon___s___f__004f4c78;
  ExceptionList = &pvStack_c;
  (**(code **)(this[0x32].vftable + 0x3f8))(this + 0x32);
  if (in_stack_0000000c == 0) goto LAB_00462f5d;
  puStack_10 = (undefined1 *)&pcStack_34;
  pcStack_34 = (char *)CONCAT31(pcStack_34._1_3_,in_stack_00000004);
  puStack_30 = (undefined4 *)0x0;
  local_2c = 0.0;
  if (&pcStack_34 == (char **)&stack0x00000004) {
    FUN_00460f70(in_stack_0000000c,0xffffffff);
    FUN_00460f70(0,0);
  }
  else {
    if (in_stack_0000000c != 0) {
      puVar2 = (undefined4 *)&DAT_004c3260;
      if (puVar5 != (undefined4 *)0x0) {
        puVar2 = puVar5;
      }
      if (*(byte *)((int)puVar2 + -1) < 0xfe) {
        FUN_00407550(1);
        puStack_30 = (undefined4 *)&DAT_004c3260;
        if (puVar5 != (undefined4 *)0x0) {
          puStack_30 = puVar5;
        }
        local_2c = (double)CONCAT44(in_stack_00000010,in_stack_0000000c);
        *(char *)((int)puStack_30 + -1) = *(char *)((int)puStack_30 + -1) + '\x01';
        goto LAB_00462f42;
      }
    }
    cVar1 = FUN_0044ecc0(in_stack_0000000c,1);
    if (cVar1 != '\0') {
      if (puVar5 == (undefined4 *)0x0) {
        puVar5 = (undefined4 *)&DAT_004c3260;
      }
      puVar2 = puStack_30;
      for (uVar4 = in_stack_0000000c >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
        *puVar2 = *puVar5;
        puVar5 = puVar5 + 1;
        puVar2 = puVar2 + 1;
      }
      for (uVar4 = in_stack_0000000c & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
        *(undefined1 *)puVar2 = *(undefined1 *)puVar5;
        puVar5 = (undefined4 *)((int)puVar5 + 1);
        puVar2 = (undefined4 *)((int)puVar2 + 1);
      }
      FUN_0044eca0(in_stack_0000000c);
      puVar5 = in_stack_00000008;
    }
  }
LAB_00462f42:
  uVar3 = FUN_00477630();
  local_2c = (double)CONCAT44(uVar3,0x462f57);
  (**(code **)(this->vftable + 0xac))();
LAB_00462f5d:
  local_2c._4_4_ = in_stack_00000014;
  local_2c._0_4_ = 0x462f6d;
  (**(code **)(this->vftable + 0xcc))();
  local_2c._0_4_ = 0x462f72;
  CGameObject::vfunc_00_013(this_00);
  if (puVar5 != (undefined4 *)0x0) {
    cVar1 = *(char *)((int)puVar5 + -1);
    if ((cVar1 == '\0') || (cVar1 == -1)) {
      local_2c = (double)CONCAT44(local_2c._4_4_,(undefined1 *)((int)puVar5 + -1));
      puStack_30 = (undefined4 *)0x462f8f;
      FUN_004789a0();
    }
    else {
      *(char *)((int)puVar5 + -1) = cVar1 + -1;
    }
  }
  ExceptionList = puStack_10;
  return;
}
```

## Assets

No direct ASE/PNG/anim references in `InitObject` (inherited visual path or runtime-assigned).

## Validation

FourCC `3POL` has no rows in the 35-level `.gam` corpus (`docs/gam_schema.md`) — this object type is not placed in any shipped level, so there is no `.gam` data to cross-check against.

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java C3DPolygon` (owned methods decompiled); `.gam` properties and assets resolved from the `InitObject` registrar calls with strings read directly from `Neutron.exe`. `.gam` value ranges cross-referenced via `docs/gam_schema.md`. Behavioral prose is derived from the decompiled bodies above; not runtime-validated.

Open questions:
- Confirm the gameplay semantics of the per-frame/owned override method(s) beyond the decompiled control flow.
- Pin the constructor address and class-id immediate (FourCC).

## Notes

- Generated by `tools/gen_placeable_specs.py` from the Ghidra dump + PE string resolution. Decompiled bodies are included verbatim as primary evidence.
