# CLoadLevel_gate_00457ec0 evidence

Recovered 2026-07-02 with `tools/ghidra/CreateFunctions.java` against
`~/ghidra-projects/JN_decomp` / `Neutron.exe`.

## Interpretation

`00457ec0` is the missing `CLoadLevel` contact/gate body. Its second argument is
the touching actor; the first branch calls the actor's `IsA` slot with
`"C3DJIMMY"` (`.rdata:004ecb20`) and returns for non-Jimmy touches.

The gate reads:

- `RequiredTask` at `this+0x5c0`; `"none"` skips the task gate.
- `FUN_0045fea0(RequiredTask)` returns the task state, or `-1` if the task is
  missing.
- For a found task state, return early when `state < RequiredLevel`
  (`this+0x610`) or when `ExactLevel != -1 && state != ExactLevel`
  (`this+0x618`).
- A missing task logs `ERROR: Task %s not found in in %s` and continues.

After the gate, `LevelName == "RETURN"` (`.rdata:004f2774`) takes a special
return path. Otherwise `LevelName != "none"` hides the portal via slot `0xd8`,
dispatches `LevelName` (`this+0x520`) and `StartPoint` (`this+0x570`) through
the Jimmy/player handoff slot, optionally plays `SoundIndex` (`this+0x614`),
and if `FadeType != -1` (`this+0x61c`) calls the fade helper with
`FadeType/FadeTime` (`this+0x620`) before locking/transitioning Jimmy.

`Radius` is not read in this body; it is the inherited collision/contact volume
that decides when this contact handler is called.

## Raw Ghidra Dump

## CLoadLevel_gate_00457ec0 @ 00457ec0

```c

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall CLoadLevel_gate_00457ec0(void *this,int *param_2)

{
  char *_Str1;
  char cVar1;
  int iVar2;
  undefined4 uVar3;
  CGameObject *this_00;
  uint uVar4;
  uint uVar5;
  int *piVar6;
  undefined2 extraout_var;
  undefined2 extraout_var_00;
  int unaff_EBX;
  undefined4 *puVar7;
  void **ppvVar8;
  int *piVar9;
  undefined4 *puVar10;
  void *apvStack_144 [20];
  undefined4 auStack_f4 [20];
  undefined4 auStack_a4 [20];
  undefined4 auStack_54 [21];

  apvStack_144[0] = this;
  CGameObject::vfunc_00_016(this);
  cVar1 = (**(code **)(*param_2 + 0x18))();
  if (cVar1 != '\0') {
    if (param_2 == (int *)0x0) {
      piVar6 = (int *)0x0;
    }
    else {
      piVar6 = param_2 + -0x30;
    }
    CGameObject::vfunc_00_013((CGameObject *)(piVar6 + 0x223));
    iVar2 = __strcmpi((char *)((int)this + 0x5c0),&DAT_004eca6c);
    if (iVar2 != 0) {
      iVar2 = FUN_0045fea0((char *)((int)this + 0x5c0));
      if (iVar2 == -1) {
        CGameObject::vfunc_00_013((CGameObject *)((int)this + 0x3a0));
      }
      else {
        if (iVar2 < *(int *)((int)this + 0x610)) {
          return;
        }
        if ((*(int *)((int)this + 0x618) != -1) && (*(int *)((int)this + 0x618) != iVar2)) {
          return;
        }
      }
    }
    _Str1 = (char *)((int)this + 0x520);
    iVar2 = __strcmpi(_Str1,s_RETURN_004f2774);
    if (iVar2 == 0) {
      CGameObject::vfunc_00_013(this_00);
      if (param_2 == (int *)0x0) {
        piVar6 = (int *)0x0;
      }
      else {
        piVar6 = param_2 + -0x30;
      }
      CGameObject::vfunc_00_013((CGameObject *)((int)piVar6 + 0x955));
      if (param_2 == (int *)0x0) {
        piVar6 = (int *)0x0;
      }
      else {
        piVar6 = param_2 + -0x30;
      }
      (**(code **)(*piVar6 + 0x178))();
      if (param_2 == (int *)0x0) {
        piVar6 = (int *)0x0;
      }
      else {
        piVar6 = param_2 + -0x30;
      }
      *(undefined1 *)(piVar6 + 0x255) = 1;
      if (param_2 == (int *)0x0) {
        piVar6 = (int *)0x0;
      }
      else {
        piVar6 = param_2 + -0x30;
      }
      *(undefined1 *)((int)piVar6 + 0x9a5) = 1;
      if (param_2 == (int *)0x0) {
        piVar6 = (int *)0x0;
      }
      else {
        piVar6 = param_2 + -0x30;
      }
      CGameObject::vfunc_00_013((CGameObject *)(piVar6 + 0x223));
      if (param_2 == (int *)0x0) {
        piVar6 = (int *)0x0;
      }
      else {
        piVar6 = param_2 + -0x30;
      }
      uVar4 = 0xffffffff;
      piVar6 = piVar6 + 0x223;
      do {
        piVar9 = piVar6;
        if (uVar4 == 0) break;
        uVar4 = uVar4 - 1;
        piVar9 = (int *)((int)piVar6 + 1);
        iVar2 = *piVar6;
        piVar6 = piVar9;
      } while ((char)iVar2 != '\0');
      uVar4 = ~uVar4;
      puVar7 = (undefined4 *)((int)piVar9 - uVar4);
      puVar10 = auStack_54;
      for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
        *puVar10 = *puVar7;
        puVar7 = puVar7 + 1;
        puVar10 = puVar10 + 1;
      }
      for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
        *(undefined1 *)puVar10 = *(undefined1 *)puVar7;
        puVar7 = (undefined4 *)((int)puVar7 + 1);
        puVar10 = (undefined4 *)((int)puVar10 + 1);
      }
      if (param_2 == (int *)0x0) {
        piVar6 = (int *)0x0;
      }
      else {
        piVar6 = param_2 + -0x30;
      }
      uVar4 = 0xffffffff;
      piVar6 = piVar6 + 0x23c;
      do {
        piVar9 = piVar6;
        if (uVar4 == 0) break;
        uVar4 = uVar4 - 1;
        piVar9 = (int *)((int)piVar6 + 1);
        iVar2 = *piVar6;
        piVar6 = piVar9;
      } while ((char)iVar2 != '\0');
      uVar4 = ~uVar4;
      puVar7 = (undefined4 *)((int)piVar9 - uVar4);
      puVar10 = auStack_f4;
      for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
        *puVar10 = *puVar7;
        puVar7 = puVar7 + 1;
        puVar10 = puVar10 + 1;
      }
      for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
        *(undefined1 *)puVar10 = *(undefined1 *)puVar7;
        puVar7 = (undefined4 *)((int)puVar7 + 1);
        puVar10 = (undefined4 *)((int)puVar10 + 1);
      }
      if (param_2 == (int *)0x0) {
        piVar6 = (int *)0x0;
      }
      else {
        piVar6 = param_2 + -0x30;
      }
      uVar4 = 0xffffffff;
      piVar6 = piVar6 + 0x223;
      do {
        piVar9 = piVar6;
        if (uVar4 == 0) break;
        uVar4 = uVar4 - 1;
        piVar9 = (int *)((int)piVar6 + 1);
        iVar2 = *piVar6;
        piVar6 = piVar9;
      } while ((char)iVar2 != '\0');
      uVar4 = ~uVar4;
      puVar7 = (undefined4 *)((int)piVar9 - uVar4);
      ppvVar8 = apvStack_144;
      for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
        *ppvVar8 = (void *)*puVar7;
        puVar7 = puVar7 + 1;
        ppvVar8 = ppvVar8 + 1;
      }
      for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
        *(undefined1 *)ppvVar8 = *(undefined1 *)puVar7;
        puVar7 = (undefined4 *)((int)puVar7 + 1);
        ppvVar8 = (void **)((int)ppvVar8 + 1);
      }
      if (param_2 == (int *)0x0) {
        piVar6 = (int *)0x0;
      }
      else {
        piVar6 = param_2 + -0x30;
      }
      uVar4 = 0xffffffff;
      piVar6 = piVar6 + 0x23c;
      do {
        piVar9 = piVar6;
        if (uVar4 == 0) break;
        uVar4 = uVar4 - 1;
        piVar9 = (int *)((int)piVar6 + 1);
        iVar2 = *piVar6;
        piVar6 = piVar9;
      } while ((char)iVar2 != '\0');
      uVar4 = ~uVar4;
      puVar7 = (undefined4 *)((int)piVar9 - uVar4);
      puVar10 = auStack_a4;
      for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
        *puVar10 = *puVar7;
        puVar7 = puVar7 + 1;
        puVar10 = puVar10 + 1;
      }
      for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
        *(undefined1 *)puVar10 = *(undefined1 *)puVar7;
        puVar7 = (undefined4 *)((int)puVar7 + 1);
        puVar10 = (undefined4 *)((int)puVar10 + 1);
      }
      if (param_2 == (int *)0x0) {
        piVar6 = (int *)0x0;
      }
      else {
        piVar6 = param_2 + -0x30;
      }
      (**(code **)(*piVar6 + 0x168))(auStack_54,auStack_f4,apvStack_144,auStack_a4);
      if (s_RECHARGE_004ed134._0_4_ != -1) {
        uVar3 = FUN_0047d390(0xffffffff,s_RECHARGE_004ed134._0_4_,0,
                             CONCAT22(extraout_var_00,(undefined2)DAT_004f6b38));
        FUN_0047dc80(uVar3);
      }
      (**(code **)(*param_2 + 0x2c4))(0,0,0);
      return;
    }
    iVar2 = __strcmpi(_Str1,&DAT_004eca6c);
    if (iVar2 != 0) {
      switch(DAT_004f0588) {
      case 0:
      case 1:
      case 4:
      case 7:
        if (param_2 == (int *)0x0) {
          piVar6 = (int *)0x0;
        }
        else {
          piVar6 = param_2 + -0x30;
        }
        (**(code **)(*piVar6 + 0x178))();
      }
      if (param_2 == (int *)0x0) {
        piVar6 = (int *)0x0;
      }
      else {
        piVar6 = param_2 + -0x30;
      }
      CGameObject::vfunc_00_013((CGameObject *)((int)piVar6 + 0x955));
      (**(code **)(*(int *)(unaff_EBX + -200) + 0xd8))();
      if (param_2 == (int *)0x0) {
        piVar6 = (int *)0x0;
      }
      else {
        piVar6 = param_2 + -0x30;
      }
      (**(code **)(*piVar6 + 0x168))(_Str1,unaff_EBX + 0x570,(int)piVar6 + 0x955,piVar6 + 0x1c9);
      if (param_2 == (int *)0x0) {
        piVar6 = (int *)0x0;
      }
      else {
        piVar6 = param_2 + -0x30;
      }
      CGameObject::vfunc_00_013((CGameObject *)((int)piVar6 + 0x955));
      if (s_RECHARGE_004ed134._0_4_ != -1) {
        uVar3 = FUN_0047d390(0xffffffff,s_RECHARGE_004ed134._0_4_,0,
                             CONCAT22(extraout_var,(undefined2)DAT_004f6b38));
        FUN_0047dc80(uVar3);
      }
      if (ram0x004ed13c != -1) {
        *(undefined1 *)(DAT_00509980 + 0x74c) = 0;
        FUN_00403c10(ram0x004ed13c,s_JIMEND_004ed140._0_4_);
        DAT_004f8430 = 1;
        if (param_2 == (int *)0x0) {
          param_2 = (int *)0x0;
        }
        else {
          param_2 = param_2 + -0x30;
        }
        (**(code **)(*param_2 + 0x11c))(1);
      }
    }
  }
  return;
}


```
