# CEditor

## Identity

| Item | Value |
|---|---|
| RTTI name | `CEditor` |
| Base chain | `OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort` |
| Vftable(s) | `004d5e7c, 004d5e8c, 004d5ea0` |
| Ctor(s) | factory/constructor installs the vftables and registers the class id (see `docs/_gam_classids.tsv`) |
| Dtor(s) | scalar deleting destructor `vfunc_01_002` at `0046ba70` |
| Ledger row | `docs/decomp_ledger.csv` |

`CEditor` is a placeable **level game controllers** object (family `level_game_controllers`, wave 10). It walks the class vtable with 12 owned methods; its `.gam`-driven parameters and assets are registered in `InitObject` and listed below.

## Field Map (registered `.gam` properties)

No own `.gam` properties registered in `InitObject` (inherits its parent's property set, or is created at runtime rather than placed). See `docs/gam_schema.md` for any inherited properties.

## Vtable Methods (owned)

| Slot | Address | Role | Behavior |
|---|---|---|---|
| `vfunc_01_002` | `0046ba70` | scalar deleting destructor | destroys the `OMediaClassStreamer` subobject and frees the allocation |
| `vfunc_02_043` | `0046bc40` | reset / reinit | see decompiled body |
| `vfunc_02_045` | `0046bcb0` | reset / reinit | see decompiled body |
| `vfunc_02_046` | `0046bdc0` | reset / reinit | see decompiled body |
| `vfunc_02_049` | `0046bec0` | reset / reinit | see decompiled body |
| `vfunc_02_051` | `0046d270` | owned override | see decompiled body |
| `vfunc_02_055` | `0046cf30` | owned override | see decompiled body |
| `vfunc_02_057` | `0046d4f0` | owned override | see decompiled body |
| `vfunc_02_059` | `0046daa0` | owned override | see decompiled body |
| `vfunc_02_062` | `0046dd50` | reset / reinit | see decompiled body |
| `vfunc_02_063` | `0046e170` | reset / reinit | see decompiled body |
| `vfunc_02_066` | `0046e7b0` | owned override | see decompiled body |

### Decompiled owned methods

**`vfunc_01_002` @ `0046ba70`** — scalar deleting destructor

```c
CEditor * __thiscall CEditor::vfunc_01_002(CEditor *this)

{
  byte in_stack_00000004;
  
  FUN_0046baa0();
  OMediaClassStreamer::~OMediaClassStreamer((OMediaClassStreamer *)(this + 0x1f9));
  if ((in_stack_00000004 & 1) != 0) {
    FUN_004789a0(this + -0xc);
  }
  return this + -0xc;
}
```

**`vfunc_02_043` @ `0046bc40`** — reset / reinit

```c
void __thiscall CEditor::vfunc_02_043(CEditor *this)

{
  CGameObject *this_00;
  
  CGameObject::vfunc_00_013((CGameObject *)this);
  (**(code **)(this->vftable + 0x2c))(DAT_00509a4c);
  this[0x24].vftable = (undefined *)0x0;
  this[0x25].vftable = (undefined *)0x0;
  this[0x26].vftable = (undefined *)0x0;
  this[0x28].vftable = (undefined *)0x0;
  this[0x29].vftable = (undefined *)0x0;
  this[0x2a].vftable = (undefined *)0x0;
  CGameObject::vfunc_00_013(this_00);
  return;
}
```

**`vfunc_02_045` @ `0046bcb0`** — reset / reinit

```c
void __thiscall CEditor::vfunc_02_045(CEditor *this)

{
  int *piVar1;
  int iVar2;
  CGameObject *this_00;
  CEditor *pCVar3;
  char *_Str1;
  
  CGameObject::vfunc_00_013((CGameObject *)this);
  FUN_004766e0();
  (**(code **)(this->vftable + 0x128))();
  this[0x24].vftable = *(undefined **)(DAT_00509a50 + 0x44);
  this[0x25].vftable = *(undefined **)(DAT_00509a50 + 0x48);
  this[0x26].vftable = *(undefined **)(DAT_00509a50 + 0x4c);
  *(undefined2 *)(DAT_00509a50 + 0x114) = 5;
  if (DAT_00509a38 != 0) {
    *(uint *)(DAT_00509a38 + 4) = *(uint *)(DAT_00509a38 + 4) & 0xffffff7f;
  }
  if (DAT_00509a3c != 0) {
    *(uint *)(DAT_00509a3c + 4) = *(uint *)(DAT_00509a3c + 4) & 0xffffff7f;
  }
  _Str1 = s_Select_Object_004f5410;
  pCVar3 = this + 0x3f;
  do {
    iVar2 = __strcmpi(_Str1,(char *)&DAT_004f81a8);
    if (iVar2 == 0) break;
    piVar1 = (int *)pCVar3->vftable;
    if (piVar1[0x1c] != 0) {
      piVar1[0x1c] = 1;
      (**(code **)(*piVar1 + 0x58))(0);
    }
    _Str1 = _Str1 + 0x20;
    pCVar3 = pCVar3 + 1;
  } while ((int)_Str1 < 0x4f6090);
  iVar2 = DAT_00509a50;
  *(undefined **)(DAT_00509a50 + 0x44) = this[0x24].vftable;
  *(undefined **)(iVar2 + 0x48) = this[0x25].vftable;
  *(undefined **)(iVar2 + 0x4c) = this[0x26].vftable;
  (**(code **)(this->vftable + 0xe4))(s_Select_Object_004f5788);
  this[0x1d1].vftable = (undefined *)0x0;
  FUN_0046a400(0);
  CGameObject::vfunc_00_013(this_00);
  return;
}
```

**`vfunc_02_046` @ `0046bdc0`** — reset / reinit

```c
void __thiscall CEditor::vfunc_02_046(CEditor *this)

{
  int iVar1;
  CGameObject *this_00;
  CEditor *pCVar2;
  char *pcVar3;
  
  pcVar3 = s_UnInitEditor___004f5810;
  CGameObject::vfunc_00_013((CGameObject *)this);
  FUN_0046a400(1,pcVar3);
  (**(code **)(this->vftable + 0x128))();
  pcVar3 = s_Select_Object_004f5410;
  pCVar2 = this + 0x3f;
  *(ushort *)(DAT_00509a50 + 0x114) = *(ushort *)(DAT_00509a50 + 0x114) & 0xfffa;
  do {
    iVar1 = __strcmpi(pcVar3,(char *)&DAT_004f81a8);
    if (iVar1 == 0) break;
    (**(code **)(*(int *)pCVar2->vftable + 0x58))(1);
    pcVar3 = pcVar3 + 0x20;
    pCVar2 = pCVar2 + 1;
  } while ((int)pcVar3 < 0x4f6090);
  (**(code **)(this->vftable + 0xe8))(1);
  ShowCursor(0);
  (**(code **)(this->vftable + 0xe0))(0);
  FUN_004766e0();
  this[0x2c].vftable = (undefined *)0x0;
  this[0x2d].vftable = (undefined *)0x0;
  this[0x2e].vftable = (undefined *)0x0;
  this[0x2f].vftable = (undefined *)0x0;
  this[0x31].vftable = (undefined *)0x0;
  CGameObject::vfunc_00_013(this_00);
  return;
}
```

**`vfunc_02_049` @ `0046bec0`** — reset / reinit

```c
void __thiscall CEditor::vfunc_02_049(CEditor *this)

{
  undefined *puVar1;
  undefined *puVar2;
  short sVar3;
  CGameObject *this_00;
  CGameObject *this_01;
  CGameObject *extraout_ECX;
  CGameObject *this_02;
  CGameObject *this_03;
  CGameObject *this_04;
  CGameObject *this_05;
  float in_stack_00000004;
  
  sVar3 = *(short *)((int)&this[0x1fe].vftable + 2);
  if ((-1 < sVar3) &&
     (sVar3 = sVar3 + -1, *(short *)((int)&this[0x1fe].vftable + 2) = sVar3, sVar3 < 0)) {
    *(undefined1 *)&this[0x1d3].vftable = 1;
  }
  puVar2 = (undefined *)(in_stack_00000004 + (float)this[0x201].vftable);
  puVar1 = this[0x202].vftable;
  this[0x201].vftable = puVar2;
  if (2 < (int)puVar1) goto LAB_0046c052;
  if (DAT_00509853 == '\0') {
    *(undefined1 *)((int)&this[0x203].vftable + 1) = 0;
LAB_0046bf35:
    if ((float)puVar2 <= 3.0) goto LAB_0046c052;
  }
  else {
    if (*(char *)((int)&this[0x203].vftable + 1) != '\0') goto LAB_0046bf35;
    *(undefined1 *)((int)&this[0x203].vftable + 1) = 1;
  }
  if (puVar1 == (undefined *)0x0) {
    CGameObject::vfunc_00_013((CGameObject *)0x0);
    (**(code **)(this->vftable + 300))(0);
    CGameObject::vfunc_00_013(this_05);
    this[0x202].vftable = (undefined *)0x1;
    this[0x201].vftable = (undefined *)0x0;
  }
  else if (puVar1 == (undefined *)0x1) {
    CGameObject::vfunc_00_013((CGameObject *)0x1);
    (**(code **)(this->vftable + 300))(1);
    CGameObject::vfunc_00_013(this_03);
    this[0x202].vftable = (undefined *)0x2;
    this[0x201].vftable = (undefined *)0x0;
    (**(code **)(this->vftable + 0x114))(s_level1d_gam_004f4a00);
    CGameObject::vfunc_00_013(this_04);
  }
  else if (puVar1 == (undefined *)0x2) {
    this[0x202].vftable = (undefined *)0x3;
    CGameObject::vfunc_00_013((CGameObject *)0x2);
    if (DAT_00509988 != (int *)0x0) {
      CGameObject::vfunc_00_013(this_00);
      (**(code **)(*DAT_00509984 + 0x58))(1);
      CGameObject::vfunc_00_013(this_01);
      this_02 = (CGameObject *)0x0;
      if (DAT_00509988 != (int *)0x0) {
        (**(code **)(*DAT_00509988 + 8))(1);
        this_02 = extraout_ECX;
      }
      CGameObject::vfunc_00_013(this_02);
      DAT_00509988 = (int *)0x0;
    }
  }
LAB_0046c052:
  if (*(char *)&this[0x1d3].vftable != '\0') {
    (**(code **)(this->vftable + 0x104))();
  }
  return;
}
```

**`vfunc_02_051` @ `0046d270`** — owned override

```c
undefined * __thiscall CEditor::vfunc_02_051(CEditor *this)

{
  CEditor *pCVar1;
  CEditor *pCVar2;
  undefined *puVar3;
  OMedia3DPolygon *this_00;
  undefined *extraout_EAX;
  undefined4 uVar4;
  int iVar5;
  undefined *in_stack_00000004;
  
  pCVar1 = this + 0x2d;
  if ((this[0x2d].vftable != (undefined *)0x0) && (this[0x2c].vftable != (undefined *)0x0)) {
    (**(code **)(this->vftable + 0xec))();
  }
  switch(this[0x1d1].vftable) {
  case (undefined *)0x0:
  case (undefined *)0x2:
  case (undefined *)0x3:
    puVar3 = this->vftable;
    uVar4 = __ftol();
    uVar4 = __ftol(uVar4);
    (**(code **)(puVar3 + 0xd8))(uVar4);
    if (pCVar1->vftable != (undefined *)0x0) {
      puVar3 = this->vftable;
      uVar4 = __ftol();
      uVar4 = __ftol(uVar4);
      puVar3 = (undefined *)(**(code **)(puVar3 + 0xdc))(uVar4);
      if ((char)puVar3 != '\0') {
        return puVar3;
      }
    }
    break;
  case (undefined *)0x1:
  case (undefined *)0x5:
    puVar3 = this->vftable;
    uVar4 = __ftol();
    uVar4 = __ftol(uVar4);
    (**(code **)(puVar3 + 0xd8))(uVar4);
    puVar3 = this->vftable;
    uVar4 = __ftol();
    uVar4 = __ftol(uVar4);
    (**(code **)(puVar3 + 0xd4))(uVar4);
    break;
  case (undefined *)0x4:
  case (undefined *)0x6:
    puVar3 = this->vftable;
    uVar4 = __ftol();
    uVar4 = __ftol(uVar4);
    (**(code **)(puVar3 + 0xd8))(uVar4);
    puVar3 = this->vftable;
    uVar4 = __ftol();
    uVar4 = __ftol(uVar4);
    puVar3 = (undefined *)(**(code **)(puVar3 + 0xd4))(uVar4);
    return puVar3;
  case (undefined *)0x7:
    puVar3 = this->vftable;
    uVar4 = __ftol();
    uVar4 = __ftol(uVar4);
    puVar3 = (undefined *)(**(code **)(puVar3 + 0xd8))(uVar4);
    return puVar3;
  }
  puVar3 = (undefined *)0x0;
  if (*(int *)(in_stack_00000004 + 0x24) != 0) {
    if (this[0x1d1].vftable == (undefined *)0x0) {
      this[0x31].vftable = in_stack_00000004;
      (**(code **)(this->vftable + 0xe0))(0);
      puVar3 = *(undefined **)(in_stack_00000004 + 0x2c);
      pCVar2 = this + 0x2c;
      pCVar1->vftable = (undefined *)0x0;
      pCVar2->vftable = puVar3;
      FUN_004592f0(*(undefined4 *)(puVar3 + 0x78),pCVar1,pCVar2);
      puVar3 = (undefined *)0x0;
      if (pCVar1->vftable != (undefined *)0x0) {
        this[0x30].vftable = *(undefined **)(in_stack_00000004 + 0x30);
        iVar5 = *(int *)(pCVar1->vftable + 0x404);
        if (iVar5 == 0x33544552) {
          pCVar2->vftable = (undefined *)0x0;
          this[0x2d].vftable = (undefined *)0x0;
          this[0x2e].vftable = (undefined *)0x0;
          this[0x2f].vftable = (undefined *)0x0;
          this[0x31].vftable = (undefined *)0x0;
          return puVar3;
        }
        if (iVar5 == 0x33534b59) {
          pCVar2->vftable = (undefined *)0x0;
          this[0x2d].vftable = (undefined *)0x0;
          this[0x2e].vftable = (undefined *)0x0;
          this[0x2f].vftable = (undefined *)0x0;
          this[0x31].vftable = (undefined *)0x0;
          return puVar3;
        }
        puVar3 = (undefined *)(**(code **)(this->vftable + 0xe0))(1);
      }
    }
    else {
      puVar3 = this[0x1d1].vftable + -2;
      if (puVar3 == (undefined *)0x0) {
        puVar3 = (undefined *)0x0;
        if (*(int *)(in_stack_00000004 + 0x3c) != 0) {
          iVar5 = *(int *)(in_stack_00000004 + 0x40) - *(int *)(in_stack_00000004 + 0x3c);
          puVar3 = (undefined *)(iVar5 * -0x6db6db6d);
          if ((iVar5 / 0x1c + (iVar5 >> 0x1f) != iVar5 >> 0x1f) &&
             (puVar3 = *(undefined **)(*(int *)(in_stack_00000004 + 0x3c) + 8),
             puVar3 != (undefined *)0xffffffff)) {
            this_00 = (OMedia3DPolygon *)
                      (*(int *)(*(int *)(in_stack_00000004 + 0x30) + 0x88) + (int)puVar3 * 0x30);
            this[0x2f].vftable = this_00;
            this[0x2e].vftable = *(undefined **)(this_00 + 4);
            OMedia3DPolygon::set_material(this_00,(OMedia3DMaterial *)0x0);
            this[0x30].vftable = *(undefined **)(in_stack_00000004 + 0x30);
            return extraout_EAX;
          }
        }
      }
    }
  }
  return puVar3;
}
```

**`vfunc_02_055` @ `0046cf30`** — owned override

```c
undefined2 __thiscall CEditor::vfunc_02_055(CEditor *this)

{
  undefined *puVar1;
  int *piVar2;
  char cVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  int iVar7;
  undefined4 *puVar8;
  CEditor *pCVar9;
  char *pcVar10;
  char *pcVar11;
  undefined4 *puVar12;
  int in_stack_00000004;
  int in_stack_00000008;
  char *local_7c;
  char local_70 [100];
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_0048c558;
  local_c = ExceptionList;
  iVar7 = 0;
  pCVar9 = this + 0x16d;
  while ((((puVar1 = pCVar9->vftable,
           (float)in_stack_00000004 <=
           (float)((*(int *)(DAT_00509a50 + 0xdc) - *(int *)(DAT_00509a50 + 0xd4)) / 2) +
           *(float *)(puVar1 + 0x44) ||
           ((float)((*(int *)(DAT_00509a50 + 0xdc) - *(int *)(DAT_00509a50 + 0xd4)) / 2) +
            *(float *)(puVar1 + 0xac) + *(float *)(puVar1 + 0x44) <= (float)in_stack_00000004)) ||
          ((float)in_stack_00000008 <=
           ((float)((*(int *)(DAT_00509a50 + 0xe0) - *(int *)(DAT_00509a50 + 0xd8)) / 2) -
           *(float *)(puVar1 + 0x48)) - *(float *)(puVar1 + 0xb0))) ||
         ((float)((*(int *)(DAT_00509a50 + 0xe0) - *(int *)(DAT_00509a50 + 0xd8)) / 2) -
          *(float *)(puVar1 + 0x48) <= (float)in_stack_00000008))) {
    iVar7 = iVar7 + 1;
    pCVar9 = pCVar9 + 1;
    if (99 < iVar7) {
      return 0;
    }
  }
  ExceptionList = &local_c;
  this[0x108].vftable = this[iVar7 + 0x16d].vftable;
  this[0x107].vftable = this[iVar7 + 0x109].vftable;
  FUN_00407550();
  local_4 = 0;
  FUN_0044e990();
  pcVar10 = local_7c;
  if (local_7c == (char *)0x0) {
    pcVar10 = &DAT_004c3260;
  }
  uVar5 = 0xffffffff;
  do {
    pcVar11 = pcVar10;
    if (uVar5 == 0) break;
    uVar5 = uVar5 - 1;
    pcVar11 = pcVar10 + 1;
    cVar3 = *pcVar10;
    pcVar10 = pcVar11;
  } while (cVar3 != '\0');
  uVar5 = ~uVar5;
  pcVar10 = pcVar11 + -uVar5;
  pcVar11 = local_70;
  for (uVar6 = uVar5 >> 2; uVar6 != 0; uVar6 = uVar6 - 1) {
    *(undefined4 *)pcVar11 = *(undefined4 *)pcVar10;
    pcVar10 = pcVar10 + 4;
    pcVar11 = pcVar11 + 4;
  }
  for (uVar5 = uVar5 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
    *pcVar11 = *pcVar10;
    pcVar10 = pcVar10 + 1;
    pcVar11 = pcVar11 + 1;
  }
  iVar4 = __strcmpi(local_70,(char *)&DAT_004f5a5c);
  if (iVar4 == 0) {
    uVar5 = 0xffffffff;
    pcVar10 = s_FALSE_004f5a54;
    do {
      if (uVar5 == 0) break;
      uVar5 = uVar5 - 1;
      cVar3 = *pcVar10;
      pcVar10 = pcVar10 + 1;
    } while (cVar3 != '\0');
    uVar5 = ~uVar5 - 1;
    cVar3 = FUN_0044ecc0(uVar5,1);
    if (cVar3 != '\0') {
      pcVar10 = s_FALSE_004f5a54;
      pcVar11 = (char *)0x0;
      for (uVar6 = uVar5 >> 2; uVar6 != 0; uVar6 = uVar6 - 1) {
        *(undefined4 *)pcVar11 = *(undefined4 *)pcVar10;
        pcVar10 = pcVar10 + 4;
        pcVar11 = pcVar11 + 4;
      }
      for (uVar6 = uVar5 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
        *pcVar11 = *pcVar10;
        pcVar10 = pcVar10 + 1;
        pcVar11 = pcVar11 + 1;
      }
LAB_0046d161:
      FUN_0044eca0(uVar5);
    }
  }
  else {
    iVar4 = __strcmpi(local_70,s_FALSE_004f5a54);
    if (iVar4 != 0) {
      (**(code **)(*(int *)(this[iVar7 + 0x16d].vftable + 0x164) + 0xc))();
      piVar2 = (int *)this[iVar7 + 0x16d].vftable;
      piVar2[0x87] = 0;
      piVar2[0x88] = piVar2[0x76];
      (**(code **)(*piVar2 + 0xc0))();
      goto LAB_0046d232;
    }
    uVar5 = 0xffffffff;
    pcVar10 = (char *)&DAT_004f5a5c;
    do {
      if (uVar5 == 0) break;
      uVar5 = uVar5 - 1;
      cVar3 = *pcVar10;
      pcVar10 = pcVar10 + 1;
    } while (cVar3 != '\0');
    uVar5 = ~uVar5 - 1;
    cVar3 = FUN_0044ecc0(uVar5,1);
    if (cVar3 != '\0') {
      puVar8 = &DAT_004f5a5c;
      puVar12 = (undefined4 *)0x0;
      for (uVar6 = uVar5 >> 2; uVar6 != 0; uVar6 = uVar6 - 1) {
        *puVar12 = *puVar8;
        puVar8 = puVar8 + 1;
        puVar12 = puVar12 + 1;
      }
      for (uVar6 = uVar5 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
        *(undefined1 *)puVar12 = *(undefined1 *)puVar8;
        puVar8 = (undefined4 *)((int)puVar8 + 1);
        puVar12 = (undefined4 *)((int)puVar12 + 1);
      }
      goto LAB_0046d161;
    }
  }
  (**(code **)(*(int *)this[0x108].vftable + 200))();
  (**(code **)(this->vftable + 0xec))();
LAB_0046d232:
  if (local_7c != (char *)0x0) {
    cVar3 = local_7c[-1];
    if ((cVar3 != '\0') && (cVar3 != -1)) {
      local_7c[-1] = cVar3 + -1;
      ExceptionList = local_c;
      return 1;
    }
    FUN_004789a0();
  }
  ExceptionList = local_c;
  return 1;
}
```

**`vfunc_02_057` @ `0046d4f0`** — owned override

```c
void __thiscall CEditor::vfunc_02_057(CEditor *this)

{
  undefined *puVar1;
  int iVar2;
  int iVar3;
  short sVar4;
  char *in_stack_00000004;
  
  sVar4 = 0;
  while( true ) {
    iVar2 = __strcmpi(s_Select_Object_004f5410 + sVar4 * 0x20,(char *)&DAT_004f81a8);
    if (iVar2 == 0) {
      return;
    }
    iVar2 = __strcmpi(in_stack_00000004,s_Select_Object_004f5410 + sVar4 * 0x20);
    if (iVar2 == 0) break;
    sVar4 = sVar4 + 1;
    if (99 < sVar4) {
      return;
    }
  }
  sVar4 = 0;
  do {
    iVar3 = (int)sVar4;
    iVar2 = __strcmpi(s_Select_Object_004f5410 + iVar3 * 0x20,(char *)&DAT_004f81a8);
    if (iVar2 == 0) {
      return;
    }
    iVar2 = __strcmpi(in_stack_00000004,s_Select_Object_004f5410 + iVar3 * 0x20);
    if (iVar2 == 0) {
      puVar1 = this[iVar3 + 0x3f].vftable;
      *(undefined4 *)(puVar1 + 0x90) = 0x3f800000;
      *(undefined4 *)(puVar1 + 0x94) = 0x3f800000;
      *(undefined4 *)(puVar1 + 0x98) = 0;
      *(undefined4 *)(puVar1 + 0x9c) = 0x3f800000;
    }
    else {
      puVar1 = this[iVar3 + 0x3f].vftable;
      *(undefined4 *)(puVar1 + 0x90) = 0x3f800000;
      *(undefined4 *)(puVar1 + 0x94) = 0;
      *(undefined4 *)(puVar1 + 0x98) = 0x3f800000;
      *(undefined4 *)(puVar1 + 0x9c) = 0x3e99999a;
    }
    sVar4 = sVar4 + 1;
  } while (sVar4 < 100);
  return;
}
```

**`vfunc_02_059` @ `0046daa0`** — owned override

```c
void __thiscall CEditor::vfunc_02_059(CEditor *this)

{
  char cVar1;
  undefined *puVar2;
  int iVar3;
  float fVar4;
  uint uVar5;
  uint uVar6;
  int *piVar7;
  float *pfVar8;
  char *pcVar9;
  char *pcVar10;
  float *pfVar11;
  float10 fVar12;
  char *local_cc;
  char *local_b8;
  undefined4 local_b4;
  char local_ac [80];
  char local_5c [80];
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_0048c586;
  local_c = ExceptionList;
  if ((this[0x2d].vftable != (undefined *)0x0) && (this[0x2c].vftable != (undefined *)0x0)) {
    ExceptionList = &local_c;
    FUN_00407550(0);
    local_4 = 0;
    FUN_00407550(0);
    local_4 = CONCAT31(local_4._1_3_,1);
    if ((this[0x107].vftable != (undefined *)0x0) && (this[0x108].vftable != (undefined *)0x0)) {
      FUN_0044e990(this[0x107].vftable + 0x1d0,0,0xffffffff);
      pcVar9 = local_b8;
      if (local_b8 == (char *)0x0) {
        pcVar9 = &DAT_004c3260;
      }
      uVar5 = 0xffffffff;
      do {
        pcVar10 = pcVar9;
        if (uVar5 == 0) break;
        uVar5 = uVar5 - 1;
        pcVar10 = pcVar9 + 1;
        cVar1 = *pcVar9;
        pcVar9 = pcVar10;
      } while (cVar1 != '\0');
      uVar5 = ~uVar5;
      puVar2 = this[0x108].vftable;
      pcVar9 = pcVar10 + -uVar5;
      pcVar10 = local_5c;
      for (uVar6 = uVar5 >> 2; uVar6 != 0; uVar6 = uVar6 - 1) {
        *(undefined4 *)pcVar10 = *(undefined4 *)pcVar9;
        pcVar9 = pcVar9 + 4;
        pcVar10 = pcVar10 + 4;
      }
      for (uVar5 = uVar5 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
        *pcVar10 = *pcVar9;
        pcVar9 = pcVar9 + 1;
        pcVar10 = pcVar10 + 1;
      }
      FUN_0044e990(puVar2 + 0x1d0,0,0xffffffff);
      pcVar9 = local_cc;
      if (local_cc == (char *)0x0) {
        pcVar9 = &DAT_004c3260;
      }
      uVar5 = 0xffffffff;
      do {
        pcVar10 = pcVar9;
        if (uVar5 == 0) break;
        uVar5 = uVar5 - 1;
        pcVar10 = pcVar9 + 1;
        cVar1 = *pcVar9;
        pcVar9 = pcVar10;
      } while (cVar1 != '\0');
      uVar5 = ~uVar5;
      pcVar9 = pcVar10 + -uVar5;
      pcVar10 = local_ac;
      for (uVar6 = uVar5 >> 2; uVar6 != 0; uVar6 = uVar6 - 1) {
        *(undefined4 *)pcVar10 = *(undefined4 *)pcVar9;
        pcVar9 = pcVar9 + 4;
        pcVar10 = pcVar10 + 4;
      }
      for (uVar5 = uVar5 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
        *pcVar10 = *pcVar9;
        pcVar9 = pcVar9 + 1;
        pcVar10 = pcVar10 + 1;
      }
      FUN_004801e5(local_ac);
      piVar7 = (int *)**(int **)(this[0x2d].vftable + 0x26c);
      if (piVar7 != *(int **)(this[0x2d].vftable + 0x26c)) {
        do {
          uVar5 = 0xffffffff;
          pcVar9 = (char *)(piVar7[2] + 8);
          do {
            if (uVar5 == 0) break;
            uVar5 = uVar5 - 1;
            cVar1 = *pcVar9;
            pcVar9 = pcVar9 + 1;
          } while (cVar1 != '\0');
          iVar3 = FUN_004638a0(0,local_b4,(char *)(piVar7[2] + 8),~uVar5 - 1);
          if (iVar3 != 0) goto switchD_0046dc1e_caseD_5;
          pfVar11 = *(float **)(piVar7[2] + 0x48);
          switch(*(undefined4 *)(piVar7[2] + 4)) {
          case 1:
            uVar5 = 0xffffffff;
            pcVar9 = local_ac;
            do {
              pcVar10 = pcVar9;
              if (uVar5 == 0) break;
              uVar5 = uVar5 - 1;
              pcVar10 = pcVar9 + 1;
              cVar1 = *pcVar9;
              pcVar9 = pcVar10;
            } while (cVar1 != '\0');
            uVar5 = ~uVar5;
            pfVar8 = (float *)(pcVar10 + -uVar5);
            for (uVar6 = uVar5 >> 2; uVar6 != 0; uVar6 = uVar6 - 1) {
              *pfVar11 = *pfVar8;
              pfVar8 = pfVar8 + 1;
              pfVar11 = pfVar11 + 1;
            }
            for (uVar5 = uVar5 & 3; uVar5 != 0; uVar5 = uVar5 - 1) {
              *(undefined1 *)pfVar11 = *(undefined1 *)pfVar8;
              pfVar8 = (float *)((int)pfVar8 + 1);
              pfVar11 = (float *)((int)pfVar11 + 1);
            }
            break;
          case 2:
            iVar3 = __strcmpi(local_ac,(char *)&DAT_004f5a5c);
            fVar4 = (float)(uint)(iVar3 == 0);
            goto LAB_0046dc86;
          case 3:
            fVar12 = (float10)FUN_004801e5(local_ac);
            *pfVar11 = (float)fVar12;
            break;
          case 4:
            goto LAB_0046dc7e;
          case 6:
LAB_0046dc7e:
            fVar4 = (float)FUN_0048015a(local_ac);
LAB_0046dc86:
            *pfVar11 = fVar4;
          }
switchD_0046dc1e_caseD_5:
          piVar7 = (int *)*piVar7;
        } while (piVar7 != (int *)*(int *)(this[0x2d].vftable + 0x26c));
      }
    }
    (**(code **)(*(int *)this[0x2d].vftable + 0x404))();
    if (local_cc != (char *)0x0) {
      cVar1 = local_cc[-1];
      if ((cVar1 == '\0') || (cVar1 == -1)) {
        FUN_004789a0(local_cc + -1);
      }
      else {
        local_cc[-1] = cVar1 + -1;
      }
    }
    if (local_b8 != (char *)0x0) {
      cVar1 = local_b8[-1];
      if ((cVar1 != '\0') && (cVar1 != -1)) {
        local_b8[-1] = cVar1 + -1;
        ExceptionList = local_c;
        return;
      }
      FUN_004789a0(local_b8 + -1);
    }
  }
  ExceptionList = local_c;
  return;
}
```

**`vfunc_02_062` @ `0046dd50`** — reset / reinit

```c
void __thiscall CEditor::vfunc_02_062(CEditor *this)

{
  char cVar1;
  OMediaClassStreamer *pOVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  undefined4 *puVar6;
  char *pcVar7;
  char *pcVar8;
  char *pcVar9;
  char *in_stack_00000004;
  undefined4 uStack_108;
  char *pcStack_104;
  int iStack_100;
  CGameObject *local_e0;
  undefined1 local_d9;
  int *local_d8;
  undefined1 *local_d4;
  OMediaFilePath local_d0 [16];
  OMediaFileStream local_c0 [60];
  char local_84 [120];
  void *local_c;
  undefined1 *puStack_8;
  uint local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_0048c5a8;
  local_c = ExceptionList;
  uVar3 = 0xffffffff;
  pcVar7 = &DAT_004f5aa0;
  do {
    pcVar9 = pcVar7;
    if (uVar3 == 0) break;
    uVar3 = uVar3 - 1;
    pcVar9 = pcVar7 + 1;
    cVar1 = *pcVar7;
    pcVar7 = pcVar9;
  } while (cVar1 != '\0');
  uVar3 = ~uVar3;
  pcVar7 = pcVar9 + -uVar3;
  pcVar9 = local_84;
  for (uVar4 = uVar3 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
    *(undefined4 *)pcVar9 = *(undefined4 *)pcVar7;
    pcVar7 = pcVar7 + 4;
    pcVar9 = pcVar9 + 4;
  }
  for (uVar3 = uVar3 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
    *pcVar9 = *pcVar7;
    pcVar7 = pcVar7 + 1;
    pcVar9 = pcVar9 + 1;
  }
  uVar3 = 0xffffffff;
  do {
    pcVar7 = in_stack_00000004;
    if (uVar3 == 0) break;
    uVar3 = uVar3 - 1;
    pcVar7 = in_stack_00000004 + 1;
    cVar1 = *in_stack_00000004;
    in_stack_00000004 = pcVar7;
  } while (cVar1 != '\0');
  uVar3 = ~uVar3;
  iVar5 = -1;
  pcVar9 = local_84;
  do {
    pcVar8 = pcVar9;
    if (iVar5 == 0) break;
    iVar5 = iVar5 + -1;
    pcVar8 = pcVar9 + 1;
    cVar1 = *pcVar9;
    pcVar9 = pcVar8;
  } while (cVar1 != '\0');
  pcVar7 = pcVar7 + -uVar3;
  pcVar9 = pcVar8 + -1;
  for (uVar4 = uVar3 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
    *(undefined4 *)pcVar9 = *(undefined4 *)pcVar7;
    pcVar7 = pcVar7 + 4;
    pcVar9 = pcVar9 + 4;
  }
  local_d4 = (undefined1 *)&uStack_108;
  for (uVar3 = uVar3 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
    *pcVar9 = *pcVar7;
    pcVar7 = pcVar7 + 1;
    pcVar9 = pcVar9 + 1;
  }
  ExceptionList = &local_c;
  FUN_0040acc0(local_84,&local_d9);
  OMediaFilePath::OMediaFilePath(local_d0);
  local_4 = 0;
  OMediaFileStream::OMediaFileStream(local_c0);
  local_4 = CONCAT31(local_4._1_3_,1);
  iStack_100 = 0x46de1b;
  FUN_0047d390();
  OMediaFileStream::setpath(local_c0,local_d0);
  iStack_100 = 0x46de3c;
  OMediaFileStream::open(local_c0,0,true,true);
  local_e0 = (CGameObject *)0x0;
  for (puVar6 = (undefined4 *)*DAT_0050999c; puVar6 != DAT_0050999c; puVar6 = (undefined4 *)*puVar6)
  {
    if (*(char *)(puVar6[2] + 0xb) == '\x01') {
      local_e0 = (CGameObject *)((int)&local_e0->vftable + 1);
    }
  }
  local_d4 = *(undefined1 **)(DAT_00509948 + 0x78);
  OMediaStreamOperators::operator<<((OMediaStreamOperators *)local_c0,(long *)&local_d4);
  OMediaStreamOperators::operator<<((OMediaStreamOperators *)local_c0,(long *)&local_e0);
  iStack_100 = 0x46de98;
  CGameObject::vfunc_00_013(local_e0);
  iVar5 = 1;
  local_d8 = (int *)*DAT_0050999c;
  puVar6 = DAT_0050999c;
  if (local_d8 != DAT_0050999c) {
    do {
      pOVar2 = (OMediaClassStreamer *)local_d8[2];
      if ((pOVar2 != (OMediaClassStreamer *)0x0) && (pOVar2[0xb] == (OMediaClassStreamer)0x1)) {
        pcStack_104 = s__3d___20s__s__d_004f5a6c;
        uStack_108 = 0x46dee9;
        iStack_100 = iVar5;
        CGameObject::vfunc_00_013((CGameObject *)(pOVar2 + 0x3a0));
        OMediaStreamOperators::operator<<
                  ((OMediaStreamOperators *)local_c0,(long *)(pOVar2 + 0x404));
        OMediaStreamOperators::operator<<
                  ((OMediaStreamOperators *)local_c0,(long *)(pOVar2 + 0x428));
        OMediaStreamOperators::operator<<((OMediaStreamOperators *)local_c0,pOVar2);
        iVar5 = iVar5 + 1;
        puVar6 = DAT_0050999c;
      }
      local_d8 = (int *)*local_d8;
    } while (local_d8 != puVar6);
  }
  OMediaFileStream::close(local_c0);
  local_4 = local_4 & 0xffffff00;
  OMediaFileStream::~OMediaFileStream(local_c0);
  local_4 = 0xffffffff;
  OMediaFilePath::~OMediaFilePath(local_d0);
  ExceptionList = local_c;
  return;
}
```

**`vfunc_02_063` @ `0046e170`** — reset / reinit

```c
void __thiscall CEditor::vfunc_02_063(CEditor *this)

{
  bool bVar1;
  char cVar2;
  OMediaClassStreamer *pOVar3;
  uint uVar4;
  uint uVar5;
  int iVar6;
  CGameObject *this_00;
  CGameObject *this_01;
  CGameObject *this_02;
  CGameObject *this_03;
  CGameObject *this_04;
  CGameObject *this_05;
  CGameObject *this_06;
  CGameObject *extraout_ECX;
  CGameObject *extraout_ECX_00;
  CGameObject *this_07;
  CGameObject *this_08;
  CGameObject *this_09;
  char *pcVar7;
  CEditor *pCVar8;
  char *pcVar9;
  char *pcVar10;
  char *in_stack_00000004;
  CGameObject aCStack_108 [2];
  undefined4 uStack_100;
  long lStack_e0;
  CGameObject *local_dc;
  CGameObject CStack_d8;
  undefined1 uStack_d4;
  undefined1 local_d1;
  OMediaFilePath local_d0 [16];
  OMediaFileStream local_c0 [36];
  char local_9c;
  char local_84 [120];
  void *pvStack_c;
  undefined1 *puStack_8;
  uint local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_0048c5d8;
  pvStack_c = ExceptionList;
  uVar4 = 0xffffffff;
  pcVar7 = in_stack_00000004;
  do {
    pcVar10 = pcVar7;
    if (uVar4 == 0) break;
    uVar4 = uVar4 - 1;
    pcVar10 = pcVar7 + 1;
    cVar2 = *pcVar7;
    pcVar7 = pcVar10;
  } while (cVar2 != '\0');
  uVar4 = ~uVar4;
  pcVar7 = pcVar10 + -uVar4;
  pCVar8 = this + 0x1e9;
  ExceptionList = &pvStack_c;
  for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
    pCVar8->vftable = *(undefined **)pcVar7;
    pcVar7 = pcVar7 + 4;
    pCVar8 = pCVar8 + 1;
  }
  for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *(char *)&pCVar8->vftable = *pcVar7;
    pcVar7 = pcVar7 + 1;
    pCVar8 = (CEditor *)((int)&pCVar8->vftable + 1);
  }
  uVar4 = 0xffffffff;
  pcVar7 = &DAT_004f5aa0;
  do {
    pcVar10 = pcVar7;
    if (uVar4 == 0) break;
    uVar4 = uVar4 - 1;
    pcVar10 = pcVar7 + 1;
    cVar2 = *pcVar7;
    pcVar7 = pcVar10;
  } while (cVar2 != '\0');
  uVar4 = ~uVar4;
  local_dc = aCStack_108;
  pcVar7 = pcVar10 + -uVar4;
  pcVar10 = local_84;
  for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
    *(undefined4 *)pcVar10 = *(undefined4 *)pcVar7;
    pcVar7 = pcVar7 + 4;
    pcVar10 = pcVar10 + 4;
  }
  for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *pcVar10 = *pcVar7;
    pcVar7 = pcVar7 + 1;
    pcVar10 = pcVar10 + 1;
  }
  uVar4 = 0xffffffff;
  do {
    pcVar7 = in_stack_00000004;
    if (uVar4 == 0) break;
    uVar4 = uVar4 - 1;
    pcVar7 = in_stack_00000004 + 1;
    cVar2 = *in_stack_00000004;
    in_stack_00000004 = pcVar7;
  } while (cVar2 != '\0');
  uVar4 = ~uVar4;
  iVar6 = -1;
  pcVar10 = local_84;
  do {
    pcVar9 = pcVar10;
    if (iVar6 == 0) break;
    iVar6 = iVar6 + -1;
    pcVar9 = pcVar10 + 1;
    cVar2 = *pcVar10;
    pcVar10 = pcVar9;
  } while (cVar2 != '\0');
  pcVar7 = pcVar7 + -uVar4;
  pcVar10 = pcVar9 + -1;
  for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
    *(undefined4 *)pcVar10 = *(undefined4 *)pcVar7;
    pcVar7 = pcVar7 + 4;
    pcVar10 = pcVar10 + 4;
  }
  for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *pcVar10 = *pcVar7;
    pcVar7 = pcVar7 + 1;
    pcVar10 = pcVar10 + 1;
  }
  FUN_0040acc0(local_84,&local_d1);
  OMediaFilePath::OMediaFilePath(local_d0);
  local_4 = 0;
  OMediaFileStream::OMediaFileStream(local_c0);
  local_4 = CONCAT31(local_4._1_3_,1);
  local_dc = (CGameObject *)0x0;
  CGameObject::vfunc_00_013(this_00);
  FUN_004766e0();
  FUN_00479000();
  OMediaFileStream::setpath(local_c0,local_d0);
  bVar1 = OMediaFileStream::fileexists(local_c0);
  if (bVar1) {
    uStack_100 = 0x46e2fe;
    OMediaFileStream::open(local_c0,1,false,false);
    if (local_9c == '\0') {
      CGameObject::vfunc_00_013(this_03);
    }
    else {
      OMediaStreamOperators::operator>>((OMediaStreamOperators *)local_c0,(long *)&stack0x00000008);
      iVar6 = FUN_00459cb0();
      if ((iVar6 == 0) || (iVar6 + -0x8c == 0)) {
        CGameObject::vfunc_00_013(this_04);
      }
      else {
        DAT_00509948 = iVar6 + -0x8c;
        OMediaStreamOperators::operator>>((OMediaStreamOperators *)local_c0,(long *)&local_dc);
        if (0 < (int)local_dc) {
          OMediaStreamOperators::operator>>((OMediaStreamOperators *)local_c0,&lStack_e0);
          cVar2 = (**(code **)(this->vftable + 0x11c))();
          if (cVar2 == '\0') {
            pOVar3 = (OMediaClassStreamer *)FUN_00459cb0();
          }
          else {
            CGameObject::vfunc_00_013(this_05);
            pOVar3 = (OMediaClassStreamer *)(**(code **)(this->vftable + 0x110))();
          }
          if (pOVar3 != (OMediaClassStreamer *)0x0) {
            while( true ) {
              local_dc = (CGameObject *)((int)&local_dc[-1].vftable + 3);
              CGameObject::vfunc_00_013(local_dc);
              *(long *)(pOVar3 + 0x404) = lStack_e0;
              OMediaStreamOperators::operator>>((OMediaStreamOperators *)local_c0,pOVar3);
              CGameObject::vfunc_00_013(local_dc);
              if ((int)local_dc < 1) break;
              OMediaStreamOperators::operator>>((OMediaStreamOperators *)local_c0,&lStack_e0);
              uStack_d4 = 0;
              CStack_d8.vftable._0_1_ = (undefined1)((uint)lStack_e0 >> 0x18);
              CStack_d8.vftable._1_1_ = (undefined1)((uint)lStack_e0 >> 0x10);
              CStack_d8.vftable._2_1_ = (undefined1)((uint)lStack_e0 >> 8);
              CStack_d8.vftable._3_1_ = (undefined1)lStack_e0;
              cVar2 = (**(code **)(this->vftable + 0x11c))();
              if (cVar2 != '\0') {
                pOVar3 = (OMediaClassStreamer *)(**(code **)(this->vftable + 0x110))();
                if (pOVar3 != (OMediaClassStreamer *)0x0) {
                  uStack_100 = 0x46e490;
                  CGameObject::vfunc_00_013(this_06);
                  this_07 = extraout_ECX;
                  goto LAB_0046e4a4;
                }
                CGameObject::vfunc_00_013(this_06);
LAB_0046e4d1:
                CGameObject::vfunc_00_013(&CStack_d8);
                break;
              }
              pOVar3 = (OMediaClassStreamer *)FUN_00459cb0();
              this_07 = extraout_ECX_00;
LAB_0046e4a4:
              if (pOVar3 == (OMediaClassStreamer *)0x0) goto LAB_0046e4d1;
              CGameObject::vfunc_00_013(this_07);
            }
          }
        }
        OMediaFileStream::close(local_c0);
        CGameObject::vfunc_00_013(this_08);
        (**(code **)(this->vftable + 0x128))();
        CGameObject::vfunc_00_013(this_09);
        (**(code **)(*DAT_00509984 + 0x58))();
      }
    }
  }
  else {
    CGameObject::vfunc_00_013(this_01);
    uStack_100 = 0x46e2ab;
    iVar6 = FUN_00459cb0();
    if (iVar6 == 0) {
      DAT_00509948 = 0;
      CGameObject::vfunc_00_013(this_02);
    }
    else {
      DAT_00509948 = iVar6 + -0x8c;
      if (DAT_00509948 == 0) {
        CGameObject::vfunc_00_013(this_02);
      }
    }
  }
  local_4 = local_4 & 0xffffff00;
  OMediaFileStream::~OMediaFileStream(local_c0);
  local_4 = 0xffffffff;
  OMediaFilePath::~OMediaFilePath(local_d0);
  ExceptionList = pvStack_c;
  return;
}
```

**`vfunc_02_066` @ `0046e7b0`** — owned override

```c
void __thiscall CEditor::vfunc_02_066(CEditor *this)

{
  char cVar1;
  bool bVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  char *pcVar6;
  char *pcVar7;
  char *pcVar8;
  char *in_stack_00000004;
  undefined1 auStack_110 [4];
  undefined4 uStack_10c;
  char *pcStack_108;
  char *pcStack_104;
  char *pcStack_100;
  undefined1 local_e6;
  undefined1 local_e5;
  undefined1 *local_e4;
  OMediaFilePath local_e0 [16];
  OMediaFilePath local_d0 [16];
  OMediaFileStream local_c0 [60];
  char local_84 [120];
  void *local_c;
  undefined1 *puStack_8;
  uint local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_0048c614;
  local_c = ExceptionList;
  uVar3 = 0xffffffff;
  pcVar6 = &DAT_004f5aa0;
  do {
    pcVar8 = pcVar6;
    if (uVar3 == 0) break;
    uVar3 = uVar3 - 1;
    pcVar8 = pcVar6 + 1;
    cVar1 = *pcVar6;
    pcVar6 = pcVar8;
  } while (cVar1 != '\0');
  uVar3 = ~uVar3;
  pcVar6 = pcVar8 + -uVar3;
  pcVar8 = local_84;
  for (uVar4 = uVar3 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
    *(undefined4 *)pcVar8 = *(undefined4 *)pcVar6;
    pcVar6 = pcVar6 + 4;
    pcVar8 = pcVar8 + 4;
  }
  for (uVar3 = uVar3 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
    *pcVar8 = *pcVar6;
    pcVar6 = pcVar6 + 1;
    pcVar8 = pcVar8 + 1;
  }
  uVar3 = 0xffffffff;
  pcVar6 = in_stack_00000004;
  do {
    pcVar8 = pcVar6;
    if (uVar3 == 0) break;
    uVar3 = uVar3 - 1;
    pcVar8 = pcVar6 + 1;
    cVar1 = *pcVar6;
    pcVar6 = pcVar8;
  } while (cVar1 != '\0');
  uVar3 = ~uVar3;
  iVar5 = -1;
  pcVar6 = local_84;
  do {
    pcVar7 = pcVar6;
    if (iVar5 == 0) break;
    iVar5 = iVar5 + -1;
    pcVar7 = pcVar6 + 1;
    cVar1 = *pcVar6;
    pcVar6 = pcVar7;
  } while (cVar1 != '\0');
  pcVar6 = pcVar8 + -uVar3;
  pcVar8 = pcVar7 + -1;
  for (uVar4 = uVar3 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
    *(undefined4 *)pcVar8 = *(undefined4 *)pcVar6;
    pcVar6 = pcVar6 + 4;
    pcVar8 = pcVar8 + 4;
  }
  pcStack_100 = (char *)0x6e756c6c;
  local_e4 = auStack_110;
  for (uVar3 = uVar3 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
    *pcVar8 = *pcVar6;
    pcVar6 = pcVar6 + 1;
    pcVar8 = pcVar8 + 1;
  }
  ExceptionList = &local_c;
  FUN_0040acc0(local_84,&local_e5);
  OMediaFilePath::OMediaFilePath(local_d0);
  local_4 = 0;
  OMediaFileStream::OMediaFileStream(local_c0);
  local_4 = CONCAT31(local_4._1_3_,1);
  pcStack_100 = (char *)0x46e884;
  OMediaFileStream::setpath(local_c0,local_d0);
  bVar2 = OMediaFileStream::fileexists(local_c0);
  if (bVar2) {
    do {
      pcStack_108 = local_84;
      pcStack_104 = s_gam__s_bak___3d_004f5de4;
      uStack_10c = 0x46e8a4;
      FUN_0047f967();
      pcStack_100 = (char *)0x6e756c6c;
      local_e4 = auStack_110;
      FUN_0040acc0(local_84,&local_e6);
      OMediaFilePath::OMediaFilePath(local_e0);
      local_4._0_1_ = 2;
      pcStack_100 = (char *)0x46e8e2;
      OMediaFileStream::setpath(local_c0,local_e0);
      local_4 = CONCAT31(local_4._1_3_,1);
      OMediaFilePath::~OMediaFilePath(local_e0);
      bVar2 = OMediaFileStream::fileexists(local_c0);
    } while (bVar2);
  }
  uVar3 = 0xffffffff;
  do {
    pcVar6 = in_stack_00000004;
    if (uVar3 == 0) break;
    uVar3 = uVar3 - 1;
    pcVar6 = in_stack_00000004 + 1;
    cVar1 = *in_stack_00000004;
    in_stack_00000004 = pcVar6;
  } while (cVar1 != '\0');
  uVar3 = ~uVar3;
  pcVar6 = pcVar6 + -uVar3;
  pcVar8 = local_84;
  for (uVar4 = uVar3 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
    *(undefined4 *)pcVar8 = *(undefined4 *)pcVar6;
    pcVar6 = pcVar6 + 4;
    pcVar8 = pcVar8 + 4;
  }
  for (uVar3 = uVar3 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
    *pcVar8 = *pcVar6;
    pcVar6 = pcVar6 + 1;
    pcVar8 = pcVar8 + 1;
  }
  pcStack_100 = local_84;
  pcStack_104 = s__s_bak___3d_004f5dd8;
  uStack_10c = 0x46e936;
  FUN_0047f967();
  local_4 = local_4 & 0xffffff00;
  OMediaFileStream::~OMediaFileStream(local_c0);
  local_4 = 0xffffffff;
  OMediaFilePath::~OMediaFilePath(local_d0);
  ExceptionList = local_c;
  return;
}
```

## Assets

No direct ASE/PNG/anim references in `InitObject` (inherited visual path or runtime-assigned).

## Confidence

Confidence: Medium

Validation: Ghidra `DumpClass.java CEditor` (owned methods decompiled); `.gam` properties and assets resolved from the `InitObject` registrar calls with strings read directly from `Neutron.exe`. `.gam` value ranges cross-referenced via `docs/gam_schema.md`. Behavioral prose is derived from the decompiled bodies above; not runtime-validated.

Open questions:
- Confirm the gameplay semantics of the per-frame/owned override method(s) beyond the decompiled control flow.
- Pin the constructor address and class-id immediate (FourCC).

## Notes

- Generated by `tools/gen_placeable_specs.py` from the Ghidra dump + PE string resolution. Decompiled bodies are included verbatim as primary evidence.
