# Menu manager target 4 evidence

Recovered 2026-07-02 with `tools/ghidra/CreateFunctions.java` and
`tools/ghidra/DumpRefs.java` against `~/ghidra-projects/JN_decomp` /
`Neutron.exe`.

## Interpretation

Target 4 traced the menu-manager strings around `.rdata:004ec5b0..004ec7c0`.
The useful xrefs resolved to `00402f60` (`LoadMyMenu`), `004038c0`
(`Activating Item`), `004060d0` (`displayMenu`), and the already-known route
helpers around `0040caa0`/`004603f0`. `CMenuElement::UpdateItemLogic`
(`0045e650`) is included as the item-side dispatch body.

Recovered menu-manager shape:

- `Menu_LoadCurrentMenu_00402f30` reads current menu index from `this+0x4d4`,
  fetches `DAT_004f8164[index]`, calls slot `0x4a0` with the menu table, then
  calls slot `0x4d8` with the menu index.
- `Menu_LoadMyMenu_00402f60` uses `DAT_004f8164[param]` as a menu table. It
  allocates an `OMediaCanvasElement` root if missing, attaches it to
  `DAT_00509a34`, then walks 29 item records (`0x28` bytes each after the
  root/menu header). For each record it lazily allocates an active canvas and
  a rollover canvas when their sprite/index fields are not `-1`, positions
  them through slot `0x450`, stores rounded screen coordinates back into the
  item record, and toggles visibility through slot `0x58` according to item
  state values `0`, `1`, and `4`.
- `Menu_ActivateItem_004038c0` bounds-checks `item < 29` and writes the item
  state field at `DAT_004f8164[menu] + 0x24 + item*0x28`.
- `Menu_DeactivateItem_00403910` writes that same state field to `0`.
- `Menu_UnloadMyMenu_00406080` hides the root canvas and both active/rollover
  canvases for all 29 records.
- `Menu_DisplayMenu_004060d0` shows the root canvas, then for each of 29 item
  records hides the active canvas and shows the rollover canvas when the state
  is neither `0` nor `4`. For the pickups menu table (`DAT_004f816c`) it also
  draws the item counter at active-item coordinates when the corresponding
  `DAT_004f7db8` counter is non-zero.
- `Menu_ItemCounterGet_004061b0`, `Menu_ItemCounterAdd_004061c0`, and
  `Menu_ItemCounterPulse_004061d0` manage the `DAT_004f7db8` paired counter
  table and a pulse timer at `DAT_004f8154..004f8160`.
- `Menu_NewGameRoute_0040caa0` is not the front-end route table itself. It is
  a mission/story script action dispatcher over `this+0x468` action strings
  such as `CARLOUT`, `TICKETBOOTH`, `REMOTE`, `RECHARGE`, and `RESTARTGAME`;
  several branches update `SCENE`, activate menu items, pulse counters, or
  finally call `FUN_00460e70("NewGame.tsk")`.
- `Menu_VRRouteTable_004603f0` loads a save/task stream, maps level FourCCs to
  `.gam` filenames, including `VR01..VR08`, writes the selected level name to
  `DAT_00509980+0x74d`, refreshes task/menu item state, and applies saved
  player start strings/coordinates when an active Jimmy object is present.
- `CMenuElement_UpdateItemLogic_0045e650` queries `this+0x16` through slots
  `0x38` then `0x14`; when that query returns `0`, it dispatches the adjusted
  `this+0xc` canvas subobject through `parent_or_input[0x23]+0xc`, shows the
  mouse cursor, and always finishes with `OMediaCanvasElement::update_logic`.

This opens the target 4 L1s for the original canvas menu graph and item
dispatch. It does not create a new linked certification: native `src/game/menu.c`
is the deliberate keyboard-list stand-in already certified only for the
`.rdata:004ec71c` routing table. It does not port `DAT_004f8164` canvas menu
tables, 29 item records, active/rollover canvas pairs, counter pulses, mouse
cursor dispatch, or the save/task refresh path.

## Raw Ghidra Dump

## Menu_LoadCurrentMenu_00402f30 @ 00402f30

```c

void __thiscall C2DInGameMenu::Menu_LoadCurrentMenu_00402f30(C2DInGameMenu *this)

{
  (**(code **)(this->vftable + 0x4a0))((&DAT_004f8164)[*(short *)&this[0x135].vftable]);
  (**(code **)(this->vftable + 0x4d8))((int)*(short *)&this[0x135].vftable);
  return;
}


```

## Menu_LoadMyMenu_00402f60 @ 00402f60

```c

void __thiscall Menu_LoadMyMenu_00402f60(void *this,int param_2)

{
  short sVar1;
  undefined2 uVar2;
  OMediaCanvasElement *pOVar3;
  int *piVar4;
  CGameObject *extraout_ECX;
  CGameObject *extraout_ECX_00;
  CGameObject *extraout_ECX_01;
  CGameObject *this_00;
  CGameObject *extraout_ECX_02;
  CGameObject *extraout_ECX_03;
  CGameObject *extraout_ECX_04;
  CGameObject *extraout_ECX_05;
  CGameObject *extraout_ECX_06;
  CGameObject *pCVar5;
  CGameObject *extraout_ECX_07;
  CGameObject *extraout_ECX_08;
  CGameObject *extraout_ECX_09;
  CGameObject *extraout_ECX_10;
  CGameObject *extraout_ECX_11;
  int iVar6;
  int iVar7;
  int *piVar8;
  char *pcVar9;
  int iVar10;
  void *pvStack_c;
  undefined1 *puStack_8;
  undefined4 local_4;

  local_4 = 0xffffffff;
  puStack_8 = &LAB_00487ee1;
  pvStack_c = ExceptionList;
  piVar8 = (int *)(&DAT_004f8164)[param_2];
  ExceptionList = &pvStack_c;
  CGameObject::vfunc_00_013(this);
  iVar7 = 0;
  pCVar5 = extraout_ECX;
  if (*piVar8 == 0) {
    pOVar3 = (OMediaCanvasElement *)FUN_00478990(0xd0);
    local_4 = 0;
    if (pOVar3 == (OMediaCanvasElement *)0x0) {
      piVar4 = (int *)0x0;
    }
    else {
      piVar4 = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
    }
    local_4 = 0xffffffff;
    *piVar8 = (int)piVar4;
    if (piVar4 != (int *)0x0) {
      (**(code **)(*piVar4 + 0x34))(DAT_00509a34);
    }
    if (DAT_00509a13 != '\0') {
      (**(code **)(*(int *)this + 0x450))(*piVar8,0,0);
      piVar8 = piVar8 + 3;
      pCVar5 = extraout_ECX_00;
      goto LAB_00403020;
    }
    (**(code **)(*(int *)this + 0x450))(*piVar8,0,0xbf800000,(int)(short)piVar8[1],0x3f800000,1);
    pCVar5 = extraout_ECX_01;
  }
  piVar8 = piVar8 + 3;
LAB_00403020:
  do {
    CGameObject::vfunc_00_013(pCVar5);
    pCVar5 = this_00;
    if ((*piVar8 == 0) && ((short)piVar8[8] != -1)) {
      iVar6 = (int)(short)piVar8[6];
      pcVar9 = s_Item___d_is_ActiveItem_IsActive__004ec5f0;
      iVar10 = iVar7;
      CGameObject::vfunc_00_013(this_00);
      pOVar3 = (OMediaCanvasElement *)FUN_00478990(0xd0,pcVar9,iVar10,iVar6);
      local_4 = 1;
      if (pOVar3 == (OMediaCanvasElement *)0x0) {
        piVar4 = (int *)0x0;
        pCVar5 = extraout_ECX_02;
      }
      else {
        piVar4 = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
        pCVar5 = extraout_ECX_03;
      }
      local_4 = 0xffffffff;
      *piVar8 = (int)piVar4;
      if (piVar4 != (int *)0x0) {
        (**(code **)(*piVar4 + 0x34))(DAT_00509a34);
        sVar1 = __ftol();
        __ftol((int)sVar1);
        CGameObject::vfunc_00_013((CGameObject *)(int)(short)piVar8[8]);
        iVar10 = *(int *)this;
        sVar1 = __ftol((int)(short)piVar8[8],0x3f800000,1);
        sVar1 = __ftol((float)(int)sVar1);
        (**(code **)(iVar10 + 0x450))(*piVar8,(float)(int)sVar1);
        uVar2 = __ftol();
        *(undefined2 *)((int)piVar8 + 0x1a) = uVar2;
        uVar2 = __ftol();
        *(undefined2 *)(piVar8 + 7) = uVar2;
        sVar1 = (short)piVar8[6];
        if (sVar1 == 0) {
LAB_00403158:
          (**(code **)(*(int *)*piVar8 + 0x58))(1);
          pCVar5 = extraout_ECX_06;
        }
        else if (sVar1 == 1) {
          pCVar5 = (CGameObject *)*piVar8;
          if (pCVar5[0x1c].vftable != (undefined *)0x0) {
            pCVar5[0x1c].vftable = (undefined *)0x1;
            (**(code **)(pCVar5->vftable + 0x58))(0);
            pCVar5 = extraout_ECX_05;
          }
        }
        else {
          pCVar5 = extraout_ECX_04;
          if (sVar1 == 4) goto LAB_00403158;
        }
      }
    }
    if ((piVar8[-1] == 0) && (*(short *)((int)piVar8 + 0x1e) != -1)) {
      pCVar5 = (CGameObject *)(int)(short)piVar8[6];
      pcVar9 = s_Item___d_is_Rollover_IsActive__d_004ec5b0;
      iVar10 = iVar7;
      CGameObject::vfunc_00_013(pCVar5);
      pOVar3 = (OMediaCanvasElement *)FUN_00478990(0xd0,pcVar9,iVar10,pCVar5);
      local_4 = 2;
      if (pOVar3 == (OMediaCanvasElement *)0x0) {
        piVar4 = (int *)0x0;
        pCVar5 = extraout_ECX_07;
      }
      else {
        piVar4 = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
        pCVar5 = extraout_ECX_08;
      }
      local_4 = 0xffffffff;
      piVar8[-1] = (int)piVar4;
      if (piVar4 != (int *)0x0) {
        (**(code **)(*piVar4 + 0x34))(DAT_00509a34);
        sVar1 = __ftol();
        __ftol((int)sVar1);
        CGameObject::vfunc_00_013((CGameObject *)(int)*(short *)((int)piVar8 + 0x1e));
        iVar10 = *(int *)this;
        sVar1 = __ftol((int)*(short *)((int)piVar8 + 0x1e),0x3f800000,1);
        sVar1 = __ftol((float)(int)sVar1);
        (**(code **)(iVar10 + 0x450))(piVar8[-1],(float)(int)sVar1);
        uVar2 = __ftol();
        *(undefined2 *)((int)piVar8 + 0x1a) = uVar2;
        uVar2 = __ftol();
        *(undefined2 *)(piVar8 + 7) = uVar2;
        sVar1 = (short)piVar8[6];
        if (sVar1 == 0) {
LAB_00403294:
          (**(code **)(*(int *)piVar8[-1] + 0x58))(1);
          pCVar5 = extraout_ECX_11;
        }
        else if (sVar1 == 1) {
          pCVar5 = (CGameObject *)piVar8[-1];
          if (pCVar5[0x1c].vftable != (undefined *)0x0) {
            pCVar5[0x1c].vftable = (undefined *)0x1;
            (**(code **)(pCVar5->vftable + 0x58))(0);
            pCVar5 = extraout_ECX_10;
          }
        }
        else {
          pCVar5 = extraout_ECX_09;
          if (sVar1 == 4) goto LAB_00403294;
        }
      }
    }
    iVar7 = iVar7 + 1;
    piVar8 = piVar8 + 10;
    if (0x1c < iVar7) {
      ExceptionList = pvStack_c;
      return;
    }
  } while( true );
}


```

## Menu_ItemRolloverState_00403890 @ 00403890

```c
(function creation failed)
```

## Menu_ActivateItem_004038c0 @ 004038c0

```c

void __thiscall
Menu_ActivateItem_004038c0(void *this,short param_2,short param_3,undefined2 param_4)

{
  if ((-1 < param_3) && (param_3 < 0x1d)) {
    CGameObject::vfunc_00_013(this);
    *(undefined2 *)((&DAT_004f8164)[param_2] + 0x24 + param_3 * 0x28) = param_4;
  }
  return;
}


```

## Menu_DeactivateItem_00403910 @ 00403910

```c

void __thiscall Menu_DeactivateItem_00403910(undefined4 param_1,short param_2,short param_3)

{
  if ((-1 < param_3) && (param_3 < 0x1d)) {
    *(undefined2 *)((&DAT_004f8164)[param_2] + 0x24 + param_3 * 0x28) = 0;
    CGameObject::vfunc_00_013((CGameObject *)(int)param_2);
  }
  return;
}


```

## Menu_UnloadMyMenu_00406080 @ 00406080

```c

void __thiscall C2DInGameMenu::Menu_UnloadMyMenu_00406080(C2DInGameMenu *this)

{
  int *piVar1;
  int iVar2;
  int *in_stack_00000004;

  if ((int *)*in_stack_00000004 != (int *)0x0) {
    (**(code **)(*(int *)*in_stack_00000004 + 0x58))(1);
  }
  piVar1 = in_stack_00000004 + 3;
  iVar2 = 0x1d;
  do {
    if ((int *)piVar1[-1] != (int *)0x0) {
      (**(code **)(*(int *)piVar1[-1] + 0x58))(1);
    }
    if ((int *)*piVar1 != (int *)0x0) {
      (**(code **)(*(int *)*piVar1 + 0x58))(1);
    }
    piVar1 = piVar1 + 10;
    iVar2 = iVar2 + -1;
  } while (iVar2 != 0);
  return;
}


```

## Menu_DisplayMenu_004060d0 @ 004060d0

```c

void __thiscall C2DInGameMenu::Menu_DisplayMenu_004060d0(C2DInGameMenu *this)

{
  undefined4 uVar1;
  CGameObject *extraout_ECX;
  CGameObject *extraout_ECX_00;
  CGameObject *extraout_ECX_01;
  CGameObject *extraout_ECX_02;
  CGameObject *pCVar2;
  int iVar3;
  int *piVar4;
  int *piVar5;
  int *in_stack_00000004;

  CGameObject::vfunc_00_013((CGameObject *)this);
  pCVar2 = (CGameObject *)*in_stack_00000004;
  if ((pCVar2 != (CGameObject *)0x0) && (pCVar2[0x1c].vftable != (undefined *)0x0)) {
    pCVar2[0x1c].vftable = (undefined *)0x1;
    (**(code **)(pCVar2->vftable + 0x58))(0);
    pCVar2 = extraout_ECX;
  }
  CGameObject::vfunc_00_013(pCVar2);
  piVar5 = &DAT_004f7db8;
  piVar4 = in_stack_00000004 + 9;
  iVar3 = 0x1d;
  pCVar2 = extraout_ECX_00;
  do {
    if ((in_stack_00000004 == DAT_004f816c) && (*piVar5 != 0)) {
      uVar1 = __ftol(&PTR_LAB_004d4544,&DAT_004ec794,*piVar5);
      uVar1 = __ftol(uVar1);
      FUN_00468660(uVar1);
      pCVar2 = extraout_ECX_01;
    }
    if (((short)*piVar4 != 0) && ((short)*piVar4 != 4)) {
      if ((int *)piVar4[-7] != (int *)0x0) {
        (**(code **)(*(int *)piVar4[-7] + 0x58))(1);
      }
      pCVar2 = (CGameObject *)piVar4[-6];
      if ((pCVar2 != (CGameObject *)0x0) && (pCVar2[0x1c].vftable != (undefined *)0x0)) {
        pCVar2[0x1c].vftable = (undefined *)0x1;
        (**(code **)(pCVar2->vftable + 0x58))(0);
        pCVar2 = extraout_ECX_02;
      }
    }
    piVar5 = piVar5 + 2;
    piVar4 = piVar4 + 10;
    iVar3 = iVar3 + -1;
  } while (iVar3 != 0);
  CGameObject::vfunc_00_013(pCVar2);
  return;
}


```

## Menu_ItemCounterGet_004061b0 @ 004061b0

```c

undefined4 __thiscall Menu_ItemCounterGet_004061b0(undefined4 param_1,int param_2)

{
  return (&DAT_004f7db8)[param_2 * 2];
}


```

## Menu_ItemCounterAdd_004061c0 @ 004061c0

```c

void __thiscall Menu_ItemCounterAdd_004061c0(undefined4 param_1,int param_2,int param_3)

{
  (&DAT_004f7db8)[param_2 * 2] = (&DAT_004f7db8)[param_2 * 2] + param_3;
  return;
}


```

## Menu_ItemCounterPulse_004061d0 @ 004061d0

```c

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall Menu_ItemCounterPulse_004061d0(undefined4 param_1,int param_2,undefined4 param_3)

{
  if (_DAT_004f815c <= 0.0) {
    DAT_004f8158 = 0xffffffff;
    DAT_004f8154 = param_2;
    _DAT_004f815c = 3.0;
    _DAT_004f8160 = param_3;
  }
  else if (DAT_004f8154 != -1) {
    DAT_004f8158 = param_2;
    (&DAT_004f7db8)[param_2 * 2] = (&DAT_004f7db8)[param_2 * 2] + 1;
    return;
  }
  (&DAT_004f7db8)[param_2 * 2] = (&DAT_004f7db8)[param_2 * 2] + 1;
  return;
}


```

## Menu_NewGameRoute_0040caa0 @ 0040caa0

```c

void __thiscall Menu_NewGameRoute_0040caa0(void *this)

{
  char *_Str1;
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  undefined4 uVar7;

  iVar1 = FUN_0045fea0(s_SCENE_004ed220);
  _Str1 = (char *)((int)this + 0x468);
  iVar2 = __strcmpi(_Str1,s_CARLOUT_004ed218);
  if ((iVar2 == 0) && (iVar1 == 0x46)) {
    FUN_0045f990(s_SCENE_004ed220,0x4b);
  }
  iVar2 = __strcmpi(_Str1,s_CALLFROMNICK_004ed208);
  if ((iVar2 == 0) && (iVar1 == 0x5a)) {
    FUN_0045f990(s_SCENE_004ed220,100);
  }
  iVar2 = __strcmpi(_Str1,s_TICKETBOOTH_004ed1fc);
  if ((iVar2 == 0) && (iVar1 == 0x140)) {
    FUN_0045f990(s_SCENE_004ed220,0x14a);
    Menu_ActivateItem_004038c0();
    Menu_ItemCounterPulse_004061d0();
  }
  iVar2 = __strcmpi(_Str1,s_teleportexplanation_004ed1e8);
  if ((iVar2 == 0) && ((iVar1 == 0x1e || (iVar1 == 0x1f)))) {
    FUN_0045f990(s_SCENE_004ed220,0x23);
  }
  iVar2 = __strcmpi(_Str1,s_RESCUECARL_004ed1dc);
  if ((iVar2 == 0) && (iVar1 == 0x1e0)) {
    FUN_0045f990(s_SCENE_004ed220,0x1ea);
  }
  iVar2 = __strcmpi(_Str1,s_SEECARL_004ed1d4);
  if ((iVar2 == 0) && (iVar1 == 0x208)) {
    FUN_0045f990(s_SCENE_004ed220,0x212);
  }
  iVar2 = __strcmpi(_Str1,s_EVADEYOKES_004ed1c8);
  if ((iVar2 == 0) && (iVar1 == 0x21c)) {
    FUN_0045f990(s_SCENE_004ed220,0x226);
  }
  iVar2 = __strcmpi(_Str1,s_INVISPART_004ed1bc);
  if (iVar2 == 0) {
    if (iVar1 == 0xa2) {
      uVar6 = 0xa8;
    }
    else {
      if (iVar1 != 0x1ea) goto LAB_0040cc2d;
      uVar6 = 500;
    }
    FUN_0045f990(s_SCENE_004ed220,uVar6);
  }
LAB_0040cc2d:
  iVar2 = __strcmpi(_Str1,s_ESCAPESHIP_004ed1b0);
  if ((iVar2 == 0) && (iVar1 == 0xac)) {
    FUN_0045f990(s_SCENE_004ed220,0xb2);
  }
  iVar2 = __strcmpi(_Str1,s_GOINGHOME_004ed1a4);
  if ((iVar2 == 0) && (iVar1 == 0xb2)) {
    FUN_0045f990(s_SCENE_004ed220,200);
  }
  iVar2 = __strcmpi(_Str1,s_LANDSHIP_004ed198);
  if ((iVar2 == 0) && (iVar1 == 0x1fe)) {
    FUN_0045f990(s_SCENE_004ed220,0x208);
  }
  iVar2 = __strcmpi(_Str1,s_REMOTE_004ed190);
  if (iVar2 == 0) {
    FUN_00406f90(3);
  }
  iVar2 = __strcmpi(_Str1,s_ABDUCTED_004ed184);
  if ((iVar2 == 0) && (iVar1 == 0x212)) {
    FUN_0045f990(s_SCENE_004ed220,0x21c);
  }
  iVar2 = __strcmpi(_Str1,s_SAVECARL_004ed178);
  if ((iVar2 == 0) && (iVar1 == 0x1fe)) {
    FUN_0045f990(s_SCENE_004ed220,0x208);
  }
  iVar2 = __strcmpi(_Str1,s_CARLDIS_004ed170);
  if ((iVar2 == 0) && (iVar1 == 0x17c)) {
    FUN_0045f990(s_SCENE_004ed220,0x186);
  }
  iVar2 = __strcmpi(_Str1,s_BEAMOFF_004ed168);
  if ((iVar2 == 0) && (iVar1 == 0x19a)) {
    uVar7 = 1;
    uVar5 = 0x18;
    uVar4 = 2;
    Menu_ActivateItem_004038c0();
    uVar3 = 0;
    uVar6 = 0x18;
    Menu_ItemCounterPulse_004061d0();
    FUN_0045f990(s_SCENE_004ed220,0x1a4,uVar6,uVar3,uVar4,uVar5,uVar7);
  }
  iVar2 = __strcmpi(_Str1,s_FOWLINV_004ed160);
  if ((iVar2 == 0) && (iVar1 == 0x1cc)) {
    FUN_0045f990(s_SCENE_004ed220,0x1d6);
  }
  iVar2 = __strcmpi(_Str1,s_PUTGODDARD_004ed154);
  if (iVar2 == 0) {
    if (iVar1 == 0x78) {
      iVar1 = FUN_00474070(&DAT_004ec7f8);
    }
    else {
      if (iVar1 != 0x91) goto LAB_0040ce1c;
      iVar1 = FUN_00474070(&DAT_004ec7f8);
    }
    if (iVar1 != 0) {
      (**(code **)(*(int *)(iVar1 + -0xc0) + 0x178))();
      (**(code **)(*(int *)(iVar1 + -0xc0) + 0x174))(4);
    }
  }
LAB_0040ce1c:
  iVar1 = __strcmpi(_Str1,s_GOGODDARD_004ed148);
  if ((iVar1 == 0) && (iVar1 = FUN_0045fea0(s_SCENE_004ed220), iVar1 == 0xa8)) {
    FUN_0045f990(s_SCENE_004ed220,0xaa);
    uVar4 = 1;
    uVar3 = 2;
    uVar6 = 0;
    Menu_ActivateItem_004038c0();
    FUN_00406f90(6,uVar6,uVar3,uVar4);
  }
  iVar1 = __strcmpi(_Str1,s_JIMEND_004ed140);
  if ((iVar1 == 0) && (iVar1 = FUN_00474070(&DAT_004ec7f8), iVar1 != 0)) {
    (**(code **)(*(int *)(iVar1 + -0xc0) + 0x178))();
  }
  iVar1 = __strcmpi(_Str1,s_RECHARGE_004ed134);
  if (iVar1 == 0) {
    if (25.0 <= DAT_004f83d4) {
      *(undefined1 *)((int)this + 0xc00) = 1;
    }
    else {
      DAT_004f83d4 = 25.0;
    }
  }
  iVar1 = __strcmpi(_Str1,s_KITEND1_004ed12c);
  if ((iVar1 == 0) && (iVar1 = FUN_0045fea0(s_KITTY1_004ed124), iVar1 == 0)) {
    FUN_0045f990(s_KITTY1_004ed124,10);
  }
  iVar1 = __strcmpi(_Str1,s_KITEND2_004ed11c);
  if ((iVar1 == 0) && (iVar1 = FUN_0045fea0(s_KITTY2_004ed114), iVar1 == 0)) {
    FUN_0045f990(s_KITTY2_004ed114,10);
  }
  iVar1 = __strcmpi(_Str1,s_KITEND3_004ed10c);
  if ((iVar1 == 0) && (iVar1 = FUN_0045fea0(s_KITTY3_004ed104), iVar1 == 0)) {
    FUN_0045f990(s_KITTY3_004ed104,10);
  }
  iVar1 = __strcmpi(_Str1,s_GETFUEL4_004ed0f8);
  if ((iVar1 == 0) && (iVar1 = FUN_0045fea0(s_SCENE_004ed220), iVar1 == 0x186)) {
    FUN_0045f990(s_SCENE_004ed220,400);
    FUN_00406f90(5);
  }
  iVar1 = __strcmpi(_Str1,s_BONUSSCREEN_004ed0ec);
  if (((iVar1 == 0) && (iVar1 = FUN_00474070(&DAT_004ec7f8), iVar1 != 0)) &&
     (*(int *)(iVar1 + 0x958) != 0)) {
    *(undefined1 *)(*(int *)(iVar1 + 0x958) + 0x4f0) = 1;
  }
  iVar1 = __strcmpi(_Str1,s_GIVEKEY_004ed0e4);
  if ((iVar1 == 0) && (iVar1 = FUN_0045fea0(s_SCENE_004ed220), iVar1 == 0xcd)) {
    FUN_0045f990(s_SCENE_004ed220,0xd2);
    Menu_ActivateItem_004038c0();
    Menu_ItemCounterPulse_004061d0();
  }
  iVar1 = __strcmpi(_Str1,s_GIVEAUTO_004ed0d8);
  if ((iVar1 == 0) && (iVar1 = FUN_0045fea0(s_SCENE_004ed220), iVar1 == 0x14a)) {
    FUN_0045f990(s_SCENE_004ed220,0x154);
    Menu_ActivateItem_004038c0();
    Menu_ItemCounterPulse_004061d0();
  }
  iVar1 = __strcmpi(_Str1,s_RESTARTGAME_004ed0cc);
  if (iVar1 == 0) {
    FUN_00460e70(s_NewGame_tsk_004ec71c);
  }
  return;
}


```

## Menu_VRRouteTable_004603f0 @ 004603f0

```c

undefined4 __thiscall Menu_VRRouteTable_004603f0(undefined4 param_1,char *param_2)

{
  undefined4 *puVar1;
  bool bVar2;
  char cVar3;
  int iVar4;
  int *piVar5;
  uint uVar6;
  uint uVar7;
  int iVar8;
  CGameObject *this;
  CGameObject *this_00;
  CGameObject *this_01;
  CGameObject *this_02;
  CGameObject *extraout_ECX;
  CGameObject *extraout_ECX_00;
  CGameObject *this_03;
  CGameObject *extraout_ECX_01;
  CGameObject *extraout_ECX_02;
  CGameObject *extraout_ECX_03;
  CGameObject *this_04;
  CGameObject *extraout_ECX_04;
  CGameObject *this_05;
  int *piVar9;
  code *pcVar10;
  long *plVar11;
  char *pcVar12;
  char *pcVar13;
  char *pcVar14;
  int *piVar15;
  undefined4 uStack_248;
  char *pcStack_244;
  int iStack_240;
  int local_220;
  long local_21c;
  undefined1 local_215 [5];
  undefined1 *local_210;
  OMediaFileStream local_20c [36];
  char local_1e8;
  float fStack_1d0;
  float fStack_1cc;
  float afStack_1c8 [2];
  OMediaFilePath local_1c0 [16];
  char local_1b0 [100];
  char acStack_14c [200];
  char local_84 [120];
  void *local_c;
  undefined1 *puStack_8;
  uint local_4;

  local_4 = 0xffffffff;
  puStack_8 = &LAB_0048bc88;
  local_c = ExceptionList;
  uVar6 = 0xffffffff;
  pcVar13 = (char *)&DAT_004f81a8;
  do {
    pcVar14 = pcVar13;
    if (uVar6 == 0) break;
    uVar6 = uVar6 - 1;
    pcVar14 = pcVar13 + 1;
    cVar3 = *pcVar13;
    pcVar13 = pcVar14;
  } while (cVar3 != '\0');
  uVar6 = ~uVar6;
  pcVar13 = pcVar14 + -uVar6;
  pcVar14 = local_84;
  for (uVar7 = uVar6 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
    *(undefined4 *)pcVar14 = *(undefined4 *)pcVar13;
    pcVar13 = pcVar13 + 4;
    pcVar14 = pcVar14 + 4;
  }
  for (uVar6 = uVar6 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
    *pcVar14 = *pcVar13;
    pcVar13 = pcVar13 + 1;
    pcVar14 = pcVar14 + 1;
  }
  uVar6 = 0xffffffff;
  do {
    pcVar13 = param_2;
    if (uVar6 == 0) break;
    uVar6 = uVar6 - 1;
    pcVar13 = param_2 + 1;
    cVar3 = *param_2;
    param_2 = pcVar13;
  } while (cVar3 != '\0');
  uVar6 = ~uVar6;
  iVar8 = -1;
  pcVar14 = local_84;
  do {
    pcVar12 = pcVar14;
    if (iVar8 == 0) break;
    iVar8 = iVar8 + -1;
    pcVar12 = pcVar14 + 1;
    cVar3 = *pcVar14;
    pcVar14 = pcVar12;
  } while (cVar3 != '\0');
  pcVar13 = pcVar13 + -uVar6;
  pcVar14 = pcVar12 + -1;
  for (uVar7 = uVar6 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
    *(undefined4 *)pcVar14 = *(undefined4 *)pcVar13;
    pcVar13 = pcVar13 + 4;
    pcVar14 = pcVar14 + 4;
  }
  local_210 = (undefined1 *)&uStack_248;
  for (uVar6 = uVar6 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
    *pcVar14 = *pcVar13;
    pcVar13 = pcVar13 + 1;
    pcVar14 = pcVar14 + 1;
  }
  local_220 = 0;
  ExceptionList = &local_c;
  FUN_0040acc0(local_84,local_215);
  OMediaFilePath::OMediaFilePath(local_1c0);
  local_4 = 0;
  OMediaFileStream::OMediaFileStream(local_20c);
  local_4 = CONCAT31(local_4._1_3_,1);
  CGameObject::vfunc_00_013(this);
  OMediaFileStream::setpath(local_20c,local_1c0);
  bVar2 = OMediaFileStream::fileexists(local_20c);
  if (!bVar2) {
    CGameObject::vfunc_00_013(this_00);
    OMediaFileStream::close(local_20c);
LAB_0046053b:
    local_4 = local_4 & 0xffffff00;
    OMediaFileStream::~OMediaFileStream(local_20c);
    local_4 = 0xffffffff;
    OMediaFilePath::~OMediaFilePath(local_1c0);
    ExceptionList = local_c;
    return 0;
  }
  CGameObject::vfunc_00_013(this_00);
  iStack_240 = 0x460525;
  OMediaFileStream::open(local_20c,1,false,false);
  if (local_1e8 == '\0') {
    CGameObject::vfunc_00_013(this_01);
    goto LAB_0046053b;
  }
  CGameObject::vfunc_00_013(this_01);
  pcVar10 = operator>>_exref;
  uVar6 = 0xffffffff;
  pcVar13 = (char *)&DAT_004f81a8;
  do {
    pcVar14 = pcVar13;
    if (uVar6 == 0) break;
    uVar6 = uVar6 - 1;
    pcVar14 = pcVar13 + 1;
    cVar3 = *pcVar13;
    pcVar13 = pcVar14;
  } while (cVar3 != '\0');
  uVar6 = ~uVar6;
  pcVar13 = pcVar14 + -uVar6;
  pcVar14 = local_1b0;
  for (uVar7 = uVar6 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
    *(undefined4 *)pcVar14 = *(undefined4 *)pcVar13;
    pcVar13 = pcVar13 + 4;
    pcVar14 = pcVar14 + 4;
  }
  local_21c = 0;
  for (uVar6 = uVar6 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
    *pcVar14 = *pcVar13;
    pcVar13 = pcVar13 + 1;
    pcVar14 = pcVar14 + 1;
  }
  OMediaStreamOperators::operator>>((OMediaStreamOperators *)local_20c,(long *)&local_210);
  OMediaStreamOperators::operator>>((OMediaStreamOperators *)local_20c,(float *)&DAT_004f83d4);
  OMediaStreamOperators::operator>>((OMediaStreamOperators *)local_20c,&DAT_004f83cc);
  OMediaStreamOperators::operator>>((OMediaStreamOperators *)local_20c,&DAT_004f83c0);
  OMediaStreamOperators::operator>>((OMediaStreamOperators *)local_20c,&DAT_004f8194);
  plVar11 = &DAT_004f8438;
  do {
    OMediaStreamOperators::operator>>((OMediaStreamOperators *)local_20c,plVar11);
    plVar11 = plVar11 + 1;
  } while ((int)plVar11 < 0x4fc5d8);
  plVar11 = &DAT_004f7db8;
  do {
    Menu_DeactivateItem_00403910();
    OMediaStreamOperators::operator>>((OMediaStreamOperators *)local_20c,plVar11);
    if (0 < *plVar11) {
      iStack_240 = 0x460626;
      Menu_ActivateItem_004038c0();
    }
    plVar11 = plVar11 + 2;
  } while ((int)plVar11 < 0x4f7ea0);
  if ((int)local_210 < 0x4c563142) {
    if (local_210 == (undefined1 *)0x4c563141) {
      pcVar13 = s_level1a_gam_004f4a18;
    }
    else {
      switch(local_210) {
      case (undefined1 *)0x4c455631:
        pcVar13 = s_level1_gam_004ef518;
        break;
      case (undefined1 *)0x4c455632:
        pcVar13 = s_level2_gam_004ef50c;
        break;
      case (undefined1 *)0x4c455633:
        pcVar13 = s_level3_gam_004ef4f4;
        break;
      case (undefined1 *)0x4c455634:
        pcVar13 = s_level4_gam_004ef500;
        break;
      case (undefined1 *)0x4c455635:
        pcVar13 = s_level5_gam_004ef4dc;
        break;
      case (undefined1 *)0x4c455636:
        pcVar13 = s_level6_gam_004ef4b8;
        break;
      case (undefined1 *)0x4c455637:
        pcVar13 = s_level7_gam_004f4a30;
        break;
      case (undefined1 *)0x4c455638:
        pcVar13 = s_level8_gam_004f4a24;
        break;
      default:
        goto switchD_00460658_default;
      }
    }
  }
  else if ((int)local_210 < 0x4c563242) {
    if (local_210 == (undefined1 *)0x4c563241) {
      pcVar13 = s_level2a_gam_004f49e8;
    }
    else {
      switch(local_210) {
      case (undefined1 *)0x4c563142:
        pcVar13 = s_level1b_gam_004ef524;
        break;
      case (undefined1 *)0x4c563143:
        pcVar13 = s_level1c_gam_004f4a0c;
        break;
      case (undefined1 *)0x4c563144:
        pcVar13 = s_level1d_gam_004f4a00;
        break;
      case (undefined1 *)0x4c563145:
        pcVar13 = s_level1e_gam_004f49f4;
        break;
      case (undefined1 *)0x4c563146:
        pcVar13 = s_level1f_gam_004ec7d8;
        break;
      default:
        goto switchD_00460658_default;
      }
    }
  }
  else if ((int)local_210 < 0x4c563542) {
    if (local_210 == (undefined1 *)0x4c563541) {
      pcVar13 = s_level5a_gam_004ef4c4;
    }
    else if ((int)local_210 < 0x4c563346) {
      if (local_210 == (undefined1 *)0x4c563345) {
        pcVar13 = s_level3e_gam_004f49a0;
      }
      else if ((int)local_210 < 0x4c563343) {
        if (local_210 == (undefined1 *)0x4c563342) {
          pcVar13 = s_level3b_gam_004f49c4;
        }
        else if (local_210 == (undefined1 *)0x4c563242) {
          pcVar13 = s_level2b_gam_004f49d0;
        }
        else {
          if (local_210 != (undefined1 *)0x4c563341) goto switchD_00460658_default;
          pcVar13 = s_level3a_gam_004f49dc;
        }
      }
      else if (local_210 == (undefined1 *)0x4c563343) {
        pcVar13 = s_level3c_gam_004f49ac;
      }
      else {
        if (local_210 != (undefined1 *)0x4c563344) goto switchD_00460658_default;
        pcVar13 = s_level3d_gam_004f49b8;
      }
    }
    else {
      switch(local_210) {
      case (undefined1 *)0x4c563441:
        pcVar13 = s_level4a_gam_004f4994;
        break;
      case (undefined1 *)0x4c563442:
        pcVar13 = s_level4b_gam_004f4988;
        break;
      case (undefined1 *)0x4c563443:
        pcVar13 = s_level4c_gam_004ef4e8;
        break;
      case (undefined1 *)0x4c563444:
        pcVar13 = s_level4d_gam_004f497c;
        break;
      default:
        goto switchD_00460658_default;
      }
    }
  }
  else if ((int)local_210 < 0x56523035) {
    if (local_210 == (undefined1 *)0x56523034) {
      pcVar13 = s_VR04_gam_004ec77c;
    }
    else if ((int)local_210 < 0x56523032) {
      if (local_210 == (undefined1 *)0x56523031) {
        pcVar13 = s_VR01_gam_004ec728;
      }
      else if (local_210 == (undefined1 *)0x4c563542) {
        pcVar13 = s_level5b_gam_004f4964;
      }
      else {
        if (local_210 != (undefined1 *)0x4c563641) goto switchD_00460658_default;
        pcVar13 = s_level6a_gam_004f4970;
      }
    }
    else if (local_210 == (undefined1 *)0x56523032) {
      pcVar13 = s_VR02_gam_004ec740;
    }
    else {
      if (local_210 != (undefined1 *)0x56523033) goto switchD_00460658_default;
      pcVar13 = s_VR03_gam_004ec734;
    }
  }
  else {
    switch(local_210) {
    case (undefined1 *)0x56523035:
      pcVar13 = s_VR05_gam_004ec770;
      break;
    case (undefined1 *)0x56523036:
      pcVar13 = s_VR06_gam_004ec758;
      break;
    case (undefined1 *)0x56523037:
      pcVar13 = s_VR07_gam_004ec764;
      break;
    case (undefined1 *)0x56523038:
      pcVar13 = s_VR08_gam_004ec74c;
      break;
    default:
      goto switchD_00460658_default;
    }
  }
  uVar6 = 0xffffffff;
  do {
    pcVar14 = pcVar13;
    if (uVar6 == 0) break;
    uVar6 = uVar6 - 1;
    pcVar14 = pcVar13 + 1;
    cVar3 = *pcVar13;
    pcVar13 = pcVar14;
  } while (cVar3 != '\0');
  uVar6 = ~uVar6;
  pcVar13 = pcVar14 + -uVar6;
  pcVar14 = (char *)(DAT_00509980 + 0x74d);
  for (uVar7 = uVar6 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
    *(undefined4 *)pcVar14 = *(undefined4 *)pcVar13;
    pcVar13 = pcVar13 + 4;
    pcVar14 = pcVar14 + 4;
  }
  for (uVar6 = uVar6 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
    *pcVar14 = *pcVar13;
    pcVar13 = pcVar13 + 1;
    pcVar14 = pcVar14 + 1;
  }
switchD_00460658_default:
  if ((DAT_005099e4 != (int *)0x0) && (cVar3 = (**(code **)(*DAT_005099e4 + 0x18))(), cVar3 != '\0')
     ) {
    if (DAT_005099e4 == (int *)0x0) {
      piVar9 = (int *)0x0;
    }
    else {
      piVar9 = DAT_005099e4 + -0x30;
    }
    OMediaStreamOperators::operator>>((OMediaStreamOperators *)local_20c,acStack_14c);
    uVar6 = 0xffffffff;
    pcVar13 = acStack_14c;
    do {
      pcVar14 = pcVar13;
      if (uVar6 == 0) break;
      uVar6 = uVar6 - 1;
      pcVar14 = pcVar13 + 1;
      cVar3 = *pcVar13;
      pcVar13 = pcVar14;
    } while (cVar3 != '\0');
    uVar6 = ~uVar6;
    piVar5 = (int *)(pcVar14 + -uVar6);
    piVar15 = piVar9 + 0x1c9;
    for (uVar7 = uVar6 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
      *piVar15 = *piVar5;
      piVar5 = piVar5 + 1;
      piVar15 = piVar15 + 1;
    }
    for (uVar6 = uVar6 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
      *(char *)piVar15 = (char)*piVar5;
      piVar5 = (int *)((int)piVar5 + 1);
      piVar15 = (int *)((int)piVar15 + 1);
    }
    uVar6 = 0xffffffff;
    pcVar13 = acStack_14c;
    do {
      pcVar14 = pcVar13;
      if (uVar6 == 0) break;
      uVar6 = uVar6 - 1;
      pcVar14 = pcVar13 + 1;
      cVar3 = *pcVar13;
      pcVar13 = pcVar14;
    } while (cVar3 != '\0');
    uVar6 = ~uVar6;
    piVar5 = (int *)(pcVar14 + -uVar6);
    piVar15 = piVar9 + 0x1dd;
    for (uVar7 = uVar6 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
      *piVar15 = *piVar5;
      piVar5 = piVar5 + 1;
      piVar15 = piVar15 + 1;
    }
    for (uVar6 = uVar6 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
      *(char *)piVar15 = (char)*piVar5;
      piVar5 = (int *)((int)piVar5 + 1);
      piVar15 = (int *)((int)piVar15 + 1);
    }
    OMediaStreamOperators::operator>>((OMediaStreamOperators *)local_20c,acStack_14c);
    OMediaStreamOperators::operator>>((OMediaStreamOperators *)local_20c,acStack_14c);
    uVar6 = 0xffffffff;
    pcVar13 = acStack_14c;
    do {
      pcVar14 = pcVar13;
      if (uVar6 == 0) break;
      uVar6 = uVar6 - 1;
      pcVar14 = pcVar13 + 1;
      cVar3 = *pcVar13;
      pcVar13 = pcVar14;
    } while (cVar3 != '\0');
    uVar6 = ~uVar6;
    piVar5 = (int *)(pcVar14 + -uVar6);
    piVar15 = piVar9 + 0x223;
    for (uVar7 = uVar6 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
      *piVar15 = *piVar5;
      piVar5 = piVar5 + 1;
      piVar15 = piVar15 + 1;
    }
    for (uVar6 = uVar6 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
      *(char *)piVar15 = (char)*piVar5;
      piVar5 = (int *)((int)piVar5 + 1);
      piVar15 = (int *)((int)piVar15 + 1);
    }
    OMediaStreamOperators::operator>>((OMediaStreamOperators *)local_20c,acStack_14c);
    uVar6 = 0xffffffff;
    pcVar13 = acStack_14c;
    do {
      pcVar14 = pcVar13;
      if (uVar6 == 0) break;
      uVar6 = uVar6 - 1;
      pcVar14 = pcVar13 + 1;
      cVar3 = *pcVar13;
      pcVar13 = pcVar14;
    } while (cVar3 != '\0');
    uVar6 = ~uVar6;
    piVar5 = (int *)(pcVar14 + -uVar6);
    piVar15 = piVar9 + 0x23c;
    for (uVar7 = uVar6 >> 2; uVar7 != 0; uVar7 = uVar7 - 1) {
      *piVar15 = *piVar5;
      piVar5 = piVar5 + 1;
      piVar15 = piVar15 + 1;
    }
    for (uVar6 = uVar6 & 3; uVar6 != 0; uVar6 = uVar6 - 1) {
      *(char *)piVar15 = (char)*piVar5;
      piVar5 = (int *)((int)piVar5 + 1);
      piVar15 = (int *)((int)piVar15 + 1);
    }
    afStack_1c8[1] = 1.0;
    OMediaStreamOperators::operator>>((OMediaStreamOperators *)local_20c,&fStack_1d0);
    OMediaStreamOperators::operator>>((OMediaStreamOperators *)local_20c,&fStack_1cc);
    OMediaStreamOperators::operator>>((OMediaStreamOperators *)local_20c,afStack_1c8);
    piVar9[0x26a] = (int)fStack_1d0;
    piVar9[0x26b] = (int)fStack_1cc;
    piVar9[0x26c] = (int)afStack_1c8[0];
    iStack_240 = DAT_00509980 + 0x74d;
    pcStack_244 = (char *)0x460bd7;
    (**(code **)(*piVar9 + 0x168))();
    pcVar10 = operator>>_exref;
  }
  iVar8 = 0;
  do {
    Menu_DeactivateItem_00403910();
    (*pcVar10)();
    iStack_240 = 0x460c02;
    Menu_ActivateItem_004038c0();
    iVar8 = iVar8 + 1;
  } while (iVar8 < 9);
  iVar8 = 0;
  do {
    Menu_DeactivateItem_00403910();
    (*pcVar10)();
    iStack_240 = 0x460c30;
    Menu_ActivateItem_004038c0();
    iVar8 = iVar8 + 1;
  } while (iVar8 < 10);
  for (puVar1 = (undefined4 *)*DAT_004fc5fc; puVar1 != DAT_004fc5fc; puVar1 = (undefined4 *)*puVar1)
  {
    local_220 = local_220 + 1;
  }
  (*pcVar10)();
  if (0 < local_220) {
    CGameObject::vfunc_00_013(this_02);
    iVar8 = 0;
    this_05 = extraout_ECX;
    if (0 < local_220) {
      do {
        OMediaStreamOperators::operator>>((OMediaStreamOperators *)local_20c,local_1b0);
        this_05 = extraout_ECX_00;
        if ((&stack0x00000000 == (undefined1 *)0x1b0) ||
           (OMediaStreamOperators::operator>>((OMediaStreamOperators *)local_20c,&local_21c),
           this_05 = this_03, local_21c < 0)) break;
        iStack_240 = 0x460cd5;
        CGameObject::vfunc_00_013(this_03);
        pcStack_244 = local_1b0;
        iStack_240 = local_21c;
        uStack_248 = 0x460ce7;
        FUN_0045f990();
        piVar9 = (int *)*DAT_0050999c;
        this_05 = extraout_ECX_01;
        if (piVar9 != DAT_0050999c) {
          do {
            piVar15 = (int *)piVar9[2];
            piVar5 = piVar15 + 0x10c;
            iVar4 = __strcmpi((char *)piVar5,&DAT_004eca6c);
            this_05 = extraout_ECX_02;
            if ((iVar4 != 0) &&
               (iVar4 = __strcmpi((char *)piVar5,local_1b0), this_05 = extraout_ECX_03, iVar4 == 0))
            {
              (**(code **)(*piVar15 + 0x424))();
              iStack_240 = 0x460d4b;
              CGameObject::vfunc_00_013(this_04);
              this_05 = extraout_ECX_04;
            }
            piVar9 = (int *)*piVar9;
          } while (piVar9 != DAT_0050999c);
        }
        iVar8 = iVar8 + 1;
      } while (iVar8 < local_220);
    }
    CGameObject::vfunc_00_013(this_05);
    piVar9 = (int *)*DAT_0050999c;
    piVar5 = DAT_0050999c;
    if (piVar9 != DAT_0050999c) {
      do {
        if ((int *)piVar9[2] != (int *)0x0) {
          (**(code **)(*(int *)piVar9[2] + 0x420))();
          piVar5 = DAT_0050999c;
        }
        piVar9 = (int *)*piVar9;
      } while (piVar9 != piVar5);
    }
  }
  OMediaFileStream::close(local_20c);
  local_4 = local_4 & 0xffffff00;
  OMediaFileStream::~OMediaFileStream(local_20c);
  local_4 = 0xffffffff;
  OMediaFilePath::~OMediaFilePath(local_1c0);
  ExceptionList = local_c;
  return 1;
}


```

## CMenuElement_UpdateItemLogic_0045e650 @ 0045e650

```c

void __thiscall CMenuElement::CMenuElement_UpdateItemLogic_0045e650(CMenuElement *this)

{
  int *piVar1;
  char cVar2;
  int *piVar3;
  CMenuElement *pCVar4;
  float in_stack_00000004;

  piVar1 = (int *)this[0x16].vftable;
  piVar3 = (int *)(**(code **)(*piVar1 + 0x38))();
  cVar2 = (**(code **)(*piVar3 + 0x14))();
  if (cVar2 == '\0') {
    if (this == (CMenuElement *)0x0) {
      pCVar4 = (CMenuElement *)0x0;
    }
    else {
      pCVar4 = this + 0xc;
    }
    (**(code **)(piVar1[0x23] + 0xc))(pCVar4);
    OMediaMouseCursor::show();
  }
  OMediaCanvasElement::update_logic((OMediaCanvasElement *)this,in_stack_00000004);
  return;
}


```
