# C3DCube

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DCube` |
| FourCC | `3CUB` |
| Base chain | `C3DObject -> OMedia3DMorphAnim -> OMedia3DShapeElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> OMediaAnim -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004d2670, 004d2680, 004d2ad0, 004d2b0c, 004d2b20` |
| Ctor(s) | factory/constructor installs the vftables and registers the class id (see `docs/_gam_classids.tsv`) |
| Dtor(s) | inherited deleting destructor (none owned) |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DCube` is a placeable **world props terrain** object (family `world_props_terrain`, wave 7). It walks the class vtable with 4 owned methods; its `.gam`-driven parameters and assets are registered in `InitObject` and listed below.

## Field Map (registered `.gam` properties)

No own `.gam` properties registered in `InitObject` (inherits its parent's property set, or is created at runtime rather than placed). See `docs/gam_schema.md` for any inherited properties.

## Vtable Methods (owned)

| Slot | Address | Role | Behavior |
|---|---|---|---|
| `vfunc_01_007` | `004614e0` | InitObject (property + asset registration) | inherited init + class registration |
| `vfunc_01_008` | `00461580` | reset / reinit | see decompiled body |
| `vfunc_04_055` | `004617f0` | reset / reinit | see decompiled body |
| `vfunc_04_056` | `00461600` | reset / reinit | see decompiled body |

### Decompiled owned methods

**`vfunc_01_007` @ `004614e0`** — InitObject (property + asset registration)

```c
void __thiscall C3DCube::vfunc_01_007(C3DCube *this)

{
  OMedia3DShape *this_00;
  undefined4 uVar1;
  CGameObject *this_01;
  void *unaff_ESI;
  void *pvStack_c;
  undefined1 *puStack_8;
  undefined4 uStack_4;
  
  uStack_4 = 0xffffffff;
  puStack_8 = &LAB_0048bd4b;
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
  (**(code **)(this[-0x30].vftable + 0xc4))(0x33435542);
  CGameObject::vfunc_00_013(this_01);
  ExceptionList = unaff_ESI;
  return;
}
```

**`vfunc_01_008` @ `00461580`** — reset / reinit

```c
void __thiscall C3DCube::vfunc_01_008(C3DCube *this)

{
  undefined *puVar1;
  int iVar2;
  CGameObject *this_00;
  
  (**(code **)(this->vftable + 0x3f8))(this,s_UnInitObject___004edba4);
  iVar2 = (**(code **)(this[-0x30].vftable + 0xb0))();
  (**(code **)(this[-0x30].vftable + 0xac))(0);
  if (iVar2 != 0) {
    (*(code *)**(undefined4 **)(iVar2 + 0x24))(1);
  }
  puVar1 = this[0x12e].vftable;
  if (puVar1 != (undefined *)0x0) {
    (*(code *)**(undefined4 **)(puVar1 + *(int *)(*(int *)(puVar1 + 4) + 4) + 4))(1);
    this[0x12e].vftable = (undefined *)0x0;
  }
  C3DObject::vfunc_01_008((C3DObject *)this);
  CGameObject::vfunc_00_013(this_00);
  return;
}
```

**`vfunc_04_055` @ `004617f0`** — reset / reinit

```c
void __thiscall C3DCube::vfunc_04_055(C3DCube *this)

{
  undefined *puVar1;
  CGameObject *this_00;
  undefined4 unaff_retaddr;
  undefined4 in_stack_00000004;
  undefined4 in_stack_00000008;
  OMedia3DMaterial *pOStack0000000c;
  void *in_stack_00000014;
  void *pvStack_c;
  undefined1 *puStack_8;
  undefined4 uStack_4;
  
  uStack_4 = 0xffffffff;
  puStack_8 = &LAB_0048bd8b;
  pvStack_c = ExceptionList;
  ExceptionList = &pvStack_c;
  (**(code **)(this[0x30].vftable + 0x3f8))
            (this + 0x30,s_InitCube_argb__f__004f4ae4,(double)(float)in_stack_00000014);
  (**(code **)(*(int *)this[0x23].vftable + 0xa8))();
  pOStack0000000c = (OMedia3DMaterial *)FUN_00478990(0x8c);
  pvStack_c = (void *)0x0;
  if (pOStack0000000c == (OMedia3DMaterial *)0x0) {
    puVar1 = (undefined *)0x0;
  }
  else {
    puVar1 = (undefined *)OMedia3DMaterial::OMedia3DMaterial(pOStack0000000c);
  }
  this[0x15e].vftable = puVar1;
  pvStack_c = (void *)0xffffffff;
  *(undefined4 *)(puVar1 + 0x6c) = uStack_4;
  *(undefined4 *)(puVar1 + 0x70) = unaff_retaddr;
  *(undefined4 *)(puVar1 + 0x74) = in_stack_00000004;
  *(undefined4 *)(puVar1 + 0x78) = in_stack_00000008;
  *(undefined4 *)(this[0x15e].vftable + 0x2c) = 2;
  (**(code **)(*(int *)this[0x23].vftable + 0x70))(this[0x15e].vftable);
  (**(code **)(this->vftable + 200))();
  CGameObject::vfunc_00_013(this_00);
  ExceptionList = in_stack_00000014;
  return;
}
```

**`vfunc_04_056` @ `00461600`** — reset / reinit

```c
/* WARNING: Removing unreachable block (ram,0x004616ed) */

void __thiscall C3DCube::vfunc_04_056(C3DCube *this)

{
  char *pcVar1;
  char cVar2;
  undefined *puVar3;
  C3DCube *pCVar4;
  uint uVar5;
  CGameObject *this_00;
  C3DCube *unaff_EBP;
  C3DCube *pCVar6;
  C3DCube *unaff_retaddr;
  uint in_stack_00000004;
  C3DCube *in_stack_00000008;
  OMedia3DMaterial *pOStack0000000c;
  float in_stack_00000014;
  undefined4 uStack_38;
  C3DCube *pCStack_34;
  char *pcStack_30;
  C3DCube *pCStack_2c;
  double local_28;
  void *pvStack_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  puStack_8 = &LAB_0048bd73;
  pvStack_c = ExceptionList;
  local_4 = 0;
  pCStack_2c = (C3DCube *)&DAT_004c3260;
  if (in_stack_00000008 != (C3DCube *)0x0) {
    pCStack_2c = in_stack_00000008;
  }
  pCStack_34 = this + 0x30;
  local_28 = (double)in_stack_00000014;
  pcStack_30 = s_InitCube___s___f__004f4ad0;
  uStack_38 = 0x461658;
  ExceptionList = &pvStack_c;
  (**(code **)(this[0x30].vftable + 0x3f8))();
  local_28 = (double)(ulonglong)(uint)in_stack_00000014;
  pCStack_2c = (C3DCube *)0x46166f;
  (**(code **)(*(int *)this[0x23].vftable + 0xa8))();
  if (in_stack_00000004 == 0) {
    pCStack_2c = (C3DCube *)0x8c;
    pcStack_30 = (char *)0x461681;
    pOStack0000000c = (OMedia3DMaterial *)FUN_00478990();
    pvStack_c._0_1_ = 1;
    if (pOStack0000000c == (OMedia3DMaterial *)0x0) {
      puVar3 = (undefined *)0x0;
    }
    else {
      pCStack_2c = (C3DCube *)0x1;
      pcStack_30 = (char *)0x46169b;
      puVar3 = (undefined *)OMedia3DMaterial::OMedia3DMaterial(pOStack0000000c);
    }
    this[0x15e].vftable = puVar3;
    *(undefined4 *)(puVar3 + 0x28) = 1;
    pvStack_c = (void *)((uint)pvStack_c._1_3_ << 8);
    *(undefined4 *)(this[0x15e].vftable + 0x2c) = 2;
    pCStack_2c = (C3DCube *)this[0x15e].vftable;
    goto LAB_00461798;
  }
  pOStack0000000c = (OMedia3DMaterial *)&uStack_38;
  uStack_38 = CONCAT31(uStack_38._1_3_,(undefined1)local_4);
  pCStack_34 = (C3DCube *)0x0;
  pcStack_30 = (char *)0x0;
  pCStack_2c = (C3DCube *)0x0;
  if (&uStack_38 == &local_4) {
    pOStack0000000c = (OMedia3DMaterial *)&uStack_38;
    FUN_00460f70(in_stack_00000004,0xffffffff);
    FUN_00460f70(0,0);
  }
  else {
    if (in_stack_00000004 != 0) {
      pCVar4 = unaff_retaddr;
      if (unaff_retaddr == (C3DCube *)0x0) {
        pCVar4 = (C3DCube *)&DAT_004c3260;
      }
      if (*(byte *)((int)&pCVar4[-1].vftable + 3) < 0xfe) {
        pOStack0000000c = (OMedia3DMaterial *)&uStack_38;
        FUN_00407550(1);
        pCStack_34 = unaff_retaddr;
        if (unaff_retaddr == (C3DCube *)0x0) {
          pCStack_34 = (C3DCube *)&DAT_004c3260;
        }
        pcStack_30 = (char *)in_stack_00000004;
        pCStack_2c = in_stack_00000008;
        pcVar1 = (char *)((int)&pCStack_34[-1].vftable + 3);
        *pcVar1 = *pcVar1 + '\x01';
        goto LAB_0046178a;
      }
    }
    cVar2 = FUN_0044ecc0(in_stack_00000004,1);
    if (cVar2 != '\0') {
      pCVar4 = unaff_retaddr;
      if (unaff_retaddr == (C3DCube *)0x0) {
        pCVar4 = (C3DCube *)&DAT_004c3260;
      }
      pCVar6 = pCStack_34;
      for (uVar5 = in_stack_00000004 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
        pCVar6->vftable = pCVar4->vftable;
        pCVar4 = pCVar4 + 1;
        pCVar6 = pCVar6 + 1;
      }
      for (uVar5 = in_stack_00000004 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
        *(undefined1 *)&pCVar6->vftable = *(undefined1 *)&pCVar4->vftable;
        pCVar4 = (C3DCube *)((int)&pCVar4->vftable + 1);
        pCVar6 = (C3DCube *)((int)&pCVar6->vftable + 1);
      }
      FUN_0044eca0(in_stack_00000004);
      this = unaff_EBP;
    }
  }
LAB_0046178a:
  pCStack_2c = (C3DCube *)FUN_004779c0();
  in_stack_00000008 = unaff_retaddr;
LAB_00461798:
  pcStack_30 = (char *)0x4617a4;
  (**(code **)(*(int *)this[0x23].vftable + 0x70))();
  pcStack_30 = (char *)0x4617ae;
  (**(code **)(this->vftable + 200))();
  pcStack_30 = (char *)0x4617b3;
  CGameObject::vfunc_00_013(this_00);
  if (in_stack_00000008 != (C3DCube *)0x0) {
    cVar2 = *(char *)((int)&in_stack_00000008[-1].vftable + 3);
    if ((cVar2 == '\0') || (cVar2 == -1)) {
      pcStack_30 = (char *)((int)&in_stack_00000008[-1].vftable + 3);
      pCStack_34 = (C3DCube *)0x4617d0;
      FUN_004789a0();
    }
    else {
      *(char *)((int)&in_stack_00000008[-1].vftable + 3) = cVar2 + -1;
    }
  }
  ExceptionList = unaff_EBP;
  return;
}
```

## Assets

No direct ASE/PNG/anim references in `InitObject` (inherited visual path or runtime-assigned).

## Validation

No registered `.gam` properties to cross-check (inherited property set or runtime-created object).

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java C3DCube` (owned methods decompiled); `.gam` properties and assets resolved from the `InitObject` registrar calls with strings read directly from `Neutron.exe`. `.gam` value ranges cross-referenced via `docs/gam_schema.md`. Behavioral prose is derived from the decompiled bodies above; not runtime-validated.

Open questions:
- Confirm the gameplay semantics of the per-frame/owned override method(s) beyond the decompiled control flow.
- Pin the constructor address and class-id immediate (FourCC).

## Notes

- Generated by `tools/gen_placeable_specs.py` from the Ghidra dump + PE string resolution. Decompiled bodies are included verbatim as primary evidence.
