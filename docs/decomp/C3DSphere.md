# C3DSphere

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DSphere` |
| FourCC | `3SPH` |
| Base chain | `C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004d41cc, 004d41dc, 004d462c, 004d4668, 004d467c` |
| Ctor(s) | factory/constructor installs the vftables and registers the class id (see `docs/_gam_classids.tsv`) |
| Dtor(s) | inherited deleting destructor (none owned) |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DSphere` is a placeable **world props terrain** object (family `world_props_terrain`, wave 7). It walks the class vtable with 2 owned methods; its `.gam`-driven parameters and assets are registered in `InitObject` and listed below.

## Field Map (registered `.gam` properties)

No own `.gam` properties registered in `InitObject` (inherits its parent's property set, or is created at runtime rather than placed). See `docs/gam_schema.md` for any inherited properties.

## Vtable Methods (owned)

| Slot | Address | Role | Behavior |
|---|---|---|---|
| `vfunc_01_007` | `00463b30` | InitObject (property + asset registration) | inherited init + class registration |
| `vfunc_04_055` | `00463bd0` | reset / reinit | see decompiled body |

### Decompiled owned methods

**`vfunc_01_007` @ `00463b30`** — InitObject (property + asset registration)

```c
void __thiscall C3DSphere::vfunc_01_007(C3DSphere *this)

{
  OMedia3DShape *this_00;
  undefined4 uVar1;
  CGameObject *this_01;
  void *unaff_ESI;
  void *pvStack_c;
  undefined1 *puStack_8;
  undefined4 uStack_4;
  
  uStack_4 = 0xffffffff;
  puStack_8 = &LAB_0048c0ab;
  pvStack_c = ExceptionList;
  ExceptionList = &pvStack_c;
  (**(code **)(this->vftable + 0x3f8))(this,s_InitObject___004eca2c);
  C3DObject::vfunc_01_007((C3DObject *)this);
  this_00 = (OMedia3DShape *)FUN_00478990(0xc0);
  uStack_4 = 0;
  if (this_00 == (OMedia3DShape *)0x0) {
    uVar1 = 0;
  }
  else {
    uVar1 = OMedia3DShape::OMedia3DShape(this_00);
  }
  uStack_4 = 0xffffffff;
  (**(code **)(this[-0x30].vftable + 0xac))(uVar1);
  (**(code **)(this[-0x30].vftable + 0xc4))(0x33535048);
  CGameObject::vfunc_00_013(this_01);
  ExceptionList = unaff_ESI;
  return;
}
```

**`vfunc_04_055` @ `00463bd0`** — reset / reinit

```c
/* WARNING: Removing unreachable block (ram,0x00463cc0) */

void __thiscall C3DSphere::vfunc_04_055(C3DSphere *this)

{
  char cVar1;
  OMedia3DMaterial *this_00;
  undefined *puVar2;
  undefined4 *puVar3;
  uint uVar4;
  CGameObject *this_01;
  C3DSphere *unaff_ESI;
  undefined4 *puVar5;
  uint unaff_retaddr;
  undefined4 in_stack_00000004;
  undefined4 *in_stack_00000008;
  float in_stack_00000014;
  undefined1 auStack_3c [4];
  undefined4 *puStack_38;
  C3DSphere *pCStack_34;
  char *pcStack_30;
  undefined4 *puStack_2c;
  double local_28;
  void *pvStack_c;
  undefined1 *puStack_8;
  undefined4 *local_4;
  
  puStack_8 = &LAB_0048c0d3;
  pvStack_c = ExceptionList;
  local_4 = (undefined4 *)0x0;
  puStack_2c = (undefined4 *)&DAT_004c3260;
  if (in_stack_00000008 != (undefined4 *)0x0) {
    puStack_2c = in_stack_00000008;
  }
  pCStack_34 = this + 0x30;
  local_28 = (double)in_stack_00000014;
  pcStack_30 = s_InitSphere___s___f__004f4d68;
  puStack_38 = (undefined4 *)0x463c28;
  ExceptionList = &pvStack_c;
  (**(code **)(this[0x30].vftable + 0x3f8))();
  local_28 = 2.54639494955358e-313;
  pcStack_30 = (char *)0x463c42;
  (**(code **)(*(int *)this[0x23].vftable + 0xac))();
  if (unaff_retaddr == 0) {
    pcStack_30 = (char *)0x8c;
    pCStack_34 = (C3DSphere *)0x463c54;
    this_00 = (OMedia3DMaterial *)FUN_00478990();
    if (this_00 == (OMedia3DMaterial *)0x0) {
      puVar2 = (undefined *)0x0;
    }
    else {
      pcStack_30 = (char *)0x1;
      pCStack_34 = (C3DSphere *)0x463c6e;
      puVar2 = (undefined *)OMedia3DMaterial::OMedia3DMaterial(this_00);
    }
    this[0x15e].vftable = puVar2;
    *(undefined4 *)(puVar2 + 0x28) = 1;
    *(undefined4 *)(this[0x15e].vftable + 0x2c) = 2;
    pcStack_30 = this[0x15e].vftable;
    goto LAB_00463d6b;
  }
  auStack_3c[0] = puStack_8._0_1_;
  puStack_38 = (undefined4 *)0x0;
  pCStack_34 = (C3DSphere *)0x0;
  pcStack_30 = (char *)0x0;
  if ((undefined1 **)auStack_3c == &puStack_8) {
    FUN_00460f70(unaff_retaddr,0xffffffff);
    FUN_00460f70(0,0);
  }
  else {
    if (unaff_retaddr != 0) {
      puVar3 = local_4;
      if (local_4 == (undefined4 *)0x0) {
        puVar3 = (undefined4 *)&DAT_004c3260;
      }
      if (*(byte *)((int)puVar3 + -1) < 0xfe) {
        FUN_00407550(1);
        puStack_38 = local_4;
        if (local_4 == (undefined4 *)0x0) {
          puStack_38 = (undefined4 *)&DAT_004c3260;
        }
        pcStack_30 = (char *)in_stack_00000004;
        *(char *)((int)puStack_38 + -1) = *(char *)((int)puStack_38 + -1) + '\x01';
        pCStack_34 = (C3DSphere *)unaff_retaddr;
        goto LAB_00463d5d;
      }
    }
    cVar1 = FUN_0044ecc0(unaff_retaddr,1);
    if (cVar1 != '\0') {
      puVar3 = local_4;
      if (local_4 == (undefined4 *)0x0) {
        puVar3 = (undefined4 *)&DAT_004c3260;
      }
      puVar5 = puStack_38;
      for (uVar4 = unaff_retaddr >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
        *puVar5 = *puVar3;
        puVar3 = puVar3 + 1;
        puVar5 = puVar5 + 1;
      }
      for (uVar4 = unaff_retaddr & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
        *(undefined1 *)puVar5 = *(undefined1 *)puVar3;
        puVar3 = (undefined4 *)((int)puVar3 + 1);
        puVar5 = (undefined4 *)((int)puVar5 + 1);
      }
      FUN_0044eca0(unaff_retaddr);
      this = unaff_ESI;
    }
  }
LAB_00463d5d:
  pcStack_30 = (char *)FUN_004779c0();
  in_stack_00000008 = local_4;
LAB_00463d6b:
  pCStack_34 = (C3DSphere *)0x463d77;
  (**(code **)(*(int *)this[0x23].vftable + 0x70))();
  pCStack_34 = (C3DSphere *)0x463d81;
  (**(code **)(this->vftable + 200))();
  pCStack_34 = (C3DSphere *)0x463d86;
  CGameObject::vfunc_00_013(this_01);
  if (in_stack_00000008 != (undefined4 *)0x0) {
    cVar1 = *(char *)((int)in_stack_00000008 + -1);
    if ((cVar1 == '\0') || (cVar1 == -1)) {
      pCStack_34 = (C3DSphere *)((int)in_stack_00000008 + -1);
      puStack_38 = (undefined4 *)0x463da3;
      FUN_004789a0();
    }
    else {
      *(char *)((int)in_stack_00000008 + -1) = cVar1 + -1;
    }
  }
  ExceptionList = unaff_ESI;
  return;
}
```

## Assets

No direct ASE/PNG/anim references in `InitObject` (inherited visual path or runtime-assigned).

## Validation

No registered `.gam` properties to cross-check (inherited property set or runtime-created object).

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java C3DSphere` (owned methods decompiled); `.gam` properties and assets resolved from the `InitObject` registrar calls with strings read directly from `Neutron.exe`. `.gam` value ranges cross-referenced via `docs/gam_schema.md`. Behavioral prose is derived from the decompiled bodies above; not runtime-validated.

Open questions:
- Confirm the gameplay semantics of the per-frame/owned override method(s) beyond the decompiled control flow.
- Pin the constructor address and class-id immediate (FourCC).

## Notes

- Generated by `tools/gen_placeable_specs.py` from the Ghidra dump + PE string resolution. Decompiled bodies are included verbatim as primary evidence.
