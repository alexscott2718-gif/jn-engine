# C3DJimmy target 6 evidence

Recovered 2026-07-02 (linked branch, Ghidra recovery plan target 6) with
`tools/ghidra/CreateFunctions.java` and `DumpFunctions.java` against
`~/ghidra-projects/JN_decomp` / `Neutron.exe`.

Target 6 repaired the `C3DJimmy` function boundaries that were previously
described as raw blocks in `docs/decomp/C3DJimmy.md`. The first Ghidra pass had
already saved:

- `00424600` `UpdateJimmyFrame_00424600`
- `00426030` `UpdateJimmyActiveController_00426030`
- `00423610` `JimmySetupOrReset_00423610`
- `00425db0` `InitJimmyRuntimeHandles_00425db0`
- `00425ef0` `JimmyEnterActionMenuLock_00425ef0`
- `0042af00` `SaveLevelActionSnapshot_0042af00`
- `0042af50` `SaveLevelActionSnapshotAndTrigger_0042af50`

This pass added the remaining helper boundaries:

`00428d50`, `00427ff0`, `00429d00`, `0042ab00`, `00425170`,
`004252e0`, `00427370`, `00425490`, `004255c0`, `00426e40`,
`00427340`, `00426a70`, `004269d0`, `00424e80`, `0042aa20`,
`004289a0`, `0042ac10`, `00428870`, `00425b20`, `00429c10`,
`00424d10`, `00429970`, `00429910`, `0042a720`, and `00401430`.

Signature caveats: `CreateFunctions.java` forces a generic `__thiscall`
prototype onto each created function. Several target 6 bodies still show the
per-frame `dt` argument as `unaff_retaddr`; this is the same prototype artifact
seen in target 3, not evidence that Jimmy uses a return address value.
Likewise, some helper arguments remain unnamed until final class structs and
slot signatures are applied.

## Interpretation

### `0xa18` controller identity

The only write to Jimmy field `0xa18` is in the `C3DJimmy` factory block
`FUN_00422160`: it allocates `0x51c` bytes, calls ctor `00401430`, stores the
result at `[this+0xa18]`, then registers the allocation with string
`0x4ef05c`. A PE string read resolves `0x4ef05c` to `C2DInGameMenu`.

The ctor body confirms the identity. `00401430` installs
`C2DInGameMenu::vftable`, initializes the in-game menu/screen data
(`omt/screens.omt`), sets controller fields including `+0x4b4/+0x4b8`, calls
`C2DInGameMenu` slots `276`, `293`, `299`, `305`, and `306`, and ends by
setting `DAT_004ec494 = 1`. Jimmy's old "gadget controller" pointer is
therefore the in-game HUD/menu overlay object, used as a gadget/menu command
endpoint.

Observed Jimmy-side protocol:

- `+0x4b8` queues menu/gadget commands. Target 6 observes command ids
  `0x13`, `0x92..0x9b`, and argument forms such as `(cmd, 1, 200, 0x12)` and
  `(cmd, 1, 0, -1)`.
- `+0x4a8` polls overlay/menu state; `UpdateJimmyFrame` waits for state `2`
  before calling `+0x460` while the global action lock is active.
- `+0x4b4` toggles race/countdown display state; `+0x468` receives the special
  countdown value; `+0x4e8` selects or clears highlighted command ids.
- Jimmy also writes controller fields directly: byte `+0x4c0 = 1`, dword
  `+0x4c4 = 0`, and reads short `+0x4c8` / `+0x4ec` as selection/display
  fields.

### Companions and setup

`JimmySetupOrReset_00423610` is the level-entry/respawn configurator. It walks
the global object ring at `DAT_0050999c` to resolve the inherited `StartPoint`
tag, hands off the matching transform to Jimmy and to the global camera/target
record, and registers the `MusicDatabase` name through
`FUN_0046a910`/`FUN_0047d710` into handle `0x978`.

The same body proves both companion identities:

- Field `0x95c` is a code-spawned `C3DGoddard`: factory `FUN_0041c810` (the
  `3GOD` factory), then setup/visibility slots. The older
  `level_timer_toggle_object` name was wrong.
- Field `0x970` is a code-spawned, initially hidden `C3DJeep`: factory
  `FUN_004211a0` (the `3JEE` factory), positioned through object slots and
  hidden/disabled immediately after creation.

Setup resets `DAT_004eefc8` and `DAT_004eefd0` to `-1.0`, handles the
`VR01..VR08` range by setting `0x938`, `DAT_004f8188 = 20.0`, and controller
slot `0x4ec`, and then runs the large per-level menu/task route switch. On the
special level set `LV4B`, `LV5A`, `LV5B`, `LEV6`, `LEV7`, and `LV6A`, it
seeds `DAT_004f83d4` to at least `20.0`.

### `UpdateJimmyFrame_00424600`

The main frame update has four recovered domains:

- Inactive-player cleanup: if `DAT_005099e4 == 0`, Jimmy restores/hides runtime
  action objects, calls the action peer/controller cleanup slots, releases
  handles `0x974` and `0x97c`, and restores `0x950` when present.
- Action-lock poll: when `0x958` exists and `DAT_004f8181` is set, Jimmy polls
  slot `0x4a8`; any state other than `2` returns early, while state `2` calls
  slot `0x460` and returns.
- Special-level death/timer path: on `LV4B`, `LV5A`, `LV5B`, `LEV6`, `LEV7`,
  and `LV6A`, when `DAT_004f83d4 <= 0`, Jimmy freezes the game, hides Jimmy and
  Goddard through slot `0x11c(true)`, seeds `0x1dac = 10.8s`, and plays sound
  `0xe5`. When `0x1dac` expires, the body re-enables Jimmy/Goddard, leaves
  action state, loads `RestartLevel.tsk`, and notifies the action peer through
  slot `0x488`.
- Timers/HUD: `DAT_004eefc8 >= 0` accumulates `dt`, toggles controller slot
  `0x4b4(1)`, and draws `TIME` digits at x positions `0x10f`, `0x133`, and
  `0x157`. `_DAT_004eefd0` is a second countdown: `LV4B` uses its negative
  grace to load `RestartLevel.tsk` after `-8.0s`; otherwise it counts down and
  plays warning sound `0x5b` on expiry.

The tail calls `C3DPlayer::vfunc_01_241` and inherited helper slots, then
arbitrates Goddard mode: if `0x95c` has short `+0x9a0 == 4` and float
`+0x6bc > 9.0`, Jimmy clears the float and calls companion slot `0x17c(2)` or
`0x17c(0)` depending on short `+0x9a2`.

### `UpdateJimmyActiveController_00426030`

The active-controller update is gated by `FUN_00407060`, `FUN_00475ca0`,
`DAT_005099e4`, `DAT_004f8430`, and byte `0x1d2d`, then calls
`C3DPlayer::vfunc_01_243` and aborts if slot `0x188(dt)` reports blocked.

Fall/fly/landing behavior:

- With active action mode, `vy < -900` and `FUN_00403950(0,3)==1` enters the
  fly path: set `FLY`/`HIFLY`, start looping sound id `1` into `0x97c`, latch
  `0x1da4`, seed `0x1da8 = 5.0`, set word `0x728 = 5`, and switch mode word
  `0x704`.
- `vy < -1100` latches `0x1d98`.
- Landing while `0x1d98` is set clears the latch, plays sound `0xc0`, and
  calls slot `0x1a0(-10.0, -10, 5.0)`.

Mode word `0x704 == 2` applies `FUN_0042a920(dt * -0.5)`, leaves action state
when `DAT_004f83d4 <= 0`, and smooths camera/attached-object state toward the
global camera record `DAT_00509a50`.

The `3PIC` halo path calls `FUN_00458e40('3PIC')`; within 850 units, object
`0x1d94` gets alpha `1.0 - distance * 0.0011764705` (`1/850`) and is moved to
the pickup position with a `-100` Y offset. Otherwise the halo is restored or
hidden.

The input tail distinguishes no-input cleanup from active gadget input. The
no-input path releases handles, restores temporary objects, and stops `HISHOOT`
within a 10-second idle window (`0x608`). The input path dispatches by
`DAT_004f0588`: mode `0` returns; mode `2` plays beep `0x75` and latches
`0x1d65`; mode `4` returns; mode `6` is aim/shoot, clamping `0x78c` to
`[0,45]`, transforming local vector `(0, aim+80.0, 45.0)` through slot
`0x384`, converting with `OMedia3DVector::angles`, saving `0x1d68`, and using
`SHOOT`; other accepted modes start looping sound id `0` into `0x974`.

### Menu lock and helper cluster

`JimmyEnterActionMenuLock_00425ef0` is vtable-4 slot 126. It only enters when
`DAT_004ec494 && !DAT_004f8181`, pauses the game via global game slot
`0x168(1)`, optionally shows the cursor, sets `DAT_004f8181 = 1`, byte
`0x55f = 1`, clears `0x1e34`, and drives the `C2DInGameMenu` controller through
slots `0x4c4`, `0x488`, `0x45c`, `0x4ac(param)`, `0x4b4(0)`, `0x460`, and
`0x4ec`, plus direct writes `controller+0x4c0 = 1` and
`controller+0x4c4 = 0`.

`JimmyExitActionMenuLock_00425b20` reverses that state: clears
`DAT_004f8181`, unpauses through global slot `0x168(0)`, hides the cursor when
appropriate, restores collision/input slots, clears controller command state,
sets `DAT_004f8434`, and calls controller slot `0x4e8(-1)`.

The helper cluster now has body-backed roles:

- Slots 78/80 enter and exit a looping gadget sound/menu state.
- Slots 93/94 are the large select/deactivate dispatchers for AMI, VR routes,
  rocket/scooter/gadget modes, and `DAT_004f0588`.
- Slot 98 updates swing/shrink action state; slots 105/106/110/112 handle
  action probes, attached-object distances, `RUNSHRUNK`, and mode-7
  trajectory state.
- Slot 107 is the in-game menu overlay/timer updater, including controller
  state polls, highlighted-command updates, and `Menu_ActivateItem_004038c0`.
- Slot 108 records/replays transform breadcrumbs; slot 109 pulses
  splat/impact/special material and impulse effects.
- Slot 113 rejects unavailable actions with the `"not available"` cue; slot
  114 drives a mode-2 charge/progress meter; slot 116 is the phone-booth
  story-level route table; slot 119 applies shrink target effects with
  `LIBBYPLANT`/`C3DDINO` special cases; slots 122/123 snapshot the current
  level/transform, with slot 123 queueing controller command `0x13`.

## Certification disposition

Target 6 creates a new explicit certificate row:
`C3DJimmy,gadget-mode-dispatch,inventory / gadgets,linked-blocked`.

The row is blocked on L2, not L1. Native `behavior_player.c` is the approved
tank-turn player implementation with a small tool-use path and vehicle ride
suppression. Native `game_flow.c` has a simplified lives/restart bridge. There
is no native `C2DInGameMenu` gadget controller, no controller slot protocol,
no code-spawned Goddard/Jeep companion setup, and no port of the original
oxygen/race/secondary countdown globals. A green oracle around the current
native inventory/tool path would compare a different native design, so no
oracle was written.

## Raw Ghidra Dump - core target 6 bodies

## UpdateJimmyFrame_00424600 @ 00424600

```c

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall UpdateJimmyFrame_00424600(void *this,float param_2)

{
  int iVar1;
  float fVar2;
  char cVar3;
  short sVar4;
  undefined4 uVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  undefined4 *puVar8;
  int *piVar9;
  int iVar10;
  int *piVar11;
  float unaff_retaddr;
  undefined1 auStack_14 [20];

  _DAT_004f8408 = _DAT_004f8408 + param_2;
  iVar10 = 0;
  if (DAT_005099e4 == 0) {
    if (*(int *)((int)this + 0x1d94) == 0) {
      piVar9 = (int *)0x0;
    }
    else {
      piVar9 = (int *)(*(int *)((int)this + 0x1d94) + -0xc0);
    }
    (**(code **)(*piVar9 + 0x58))(1);
    (**(code **)(**(int **)((int)this + 0x958) + 0x4c4))();
    *(undefined4 *)((int)this + 0x1d74) = 0;
    if (*(int *)((int)this + 0x974) != -1) {
      FUN_0047d7a0(*(int *)((int)this + 0x974),0);
      *(undefined4 *)((int)this + 0x974) = 0xffffffff;
      if (*(int **)((int)this + 0x950) != (int *)0x0) {
        (**(code **)(**(int **)((int)this + 0x950) + 0x58))(1);
      }
    }
    if (*(int *)((int)this + 0x97c) != -1) {
      FUN_0047d7a0(*(int *)((int)this + 0x97c),0);
      *(undefined4 *)((int)this + 0x97c) = 0xffffffff;
    }
  }
  cVar3 = (**(code **)(*(int *)this + 0x218))();
  if (cVar3 != '\0') {
    if (DAT_004f0588 == 0) {
      (**(code **)(*(int *)this + 0x264))(0x43200000);
      (**(code **)(*(int *)((int)this + -0xc0) + 0xe0))(&DAT_004ed040,1);
    }
    else {
      (**(code **)(*(int *)this + 0x264))(0x42700000);
    }
  }
  if (*(char *)((int)this + 0x1d90) != '\0') {
    uVar5 = FUN_0045fea0(s_SCENE_004ed220);
    FUN_00468660(0x14,0xf0,&PTR_LAB_004d4544,&DAT_004ec794,uVar5);
  }
  if ((*(int **)((int)this + 0x958) != (int *)0x0) && (DAT_004f8181 != '\0')) {
    sVar4 = (**(code **)(**(int **)((int)this + 0x958) + 0x4a8))();
    if (sVar4 != 2) {
      return;
    }
    (**(code **)(**(int **)((int)this + 0x958) + 0x460))();
    return;
  }
  piVar9 = (int *)((int)this + -0xc0);
  (**(code **)(*(int *)((int)this + -0xc0) + 0x208))(param_2);
  if ((0.0 < *(float *)((int)this + 0x1dac)) &&
     (fVar2 = *(float *)((int)this + 0x1dac) - unaff_retaddr, *(float *)((int)this + 0x1dac) = fVar2
     , fVar2 <= 0.0)) {
    *(undefined4 *)((int)this + 0x1dac) = 0;
    (**(code **)(*piVar9 + 0x11c))(0);
    if (*(int **)((int)this + 0x95c) != (int *)0x0) {
      (**(code **)(**(int **)((int)this + 0x95c) + 0x11c))(0);
    }
    (**(code **)(*piVar9 + 0x178))();
    FUN_00468660(0x3c,0x39,&LAB_00424947,s__6_0d_004ec714,DAT_004f83c0);
    FUN_00460e70(s_RestartLevel_tsk_004ec7e4);
    if (*(int **)((int)this + 0x958) == (int *)0x0) {
      return;
    }
    (**(code **)(**(int **)((int)this + 0x958) + 0x488))();
    return;
  }
  if (DAT_004f83d4 <= 0.0) {
    iVar1 = DAT_00509948[0x124];
    if (iVar1 < 0x4c563543) {
      if ((iVar1 < 0x4c563541) &&
         ((iVar1 < 0x4c455636 || ((0x4c455637 < iVar1 && (iVar1 != 0x4c563442))))))
      goto LAB_004248de;
    }
    else if (iVar1 != 0x4c563641) goto LAB_004248de;
    if (*(float *)((int)this + 0x1dac) == 0.0) {
      FUN_0047d850();
      FUN_00403c10(4,0x40c00000);
      (**(code **)(*DAT_00509948 + 0x168))(1);
      (**(code **)(*piVar9 + 0x11c))(1);
      if (*(int **)((int)this + 0x95c) != (int *)0x0) {
        (**(code **)(**(int **)((int)this + 0x95c) + 0x11c))(1);
      }
      *(undefined4 *)((int)this + 0x1dac) = 0x412ccccd;
      FUN_00458980(0xffffffff,0xe5,0);
    }
  }
LAB_004248de:
  *(float *)((int)this + 0x1d9c) = unaff_retaddr + *(float *)((int)this + 0x1d9c);
  if (DAT_00509a12 != '\0') {
    piVar11 = (int *)*DAT_0050999c;
    if (piVar11 != DAT_0050999c) {
      do {
        cVar3 = (**(code **)(*(int *)piVar11[2] + 0x220))();
        if (cVar3 != '\0') {
          iVar10 = iVar10 + 1;
        }
        piVar11 = (int *)*piVar11;
      } while (piVar11 != DAT_0050999c);
    }
    FUN_00468660(0x123,100,0x54494d45,0x4ef3e0,iVar10);
    param_2 = unaff_retaddr;
  }
  if (((*(char *)((int)this + 0x1da4) != '\0') && (0.0 < *(float *)((int)this + 0x1da8))) &&
     (fVar2 = *(float *)((int)this + 0x1da8) - unaff_retaddr, *(float *)((int)this + 0x1da8) = fVar2
     , fVar2 <= 0.0)) {
    *(undefined4 *)((int)this + 0x1da8) = 0;
  }
  if (0.0 <= DAT_004eefc8) {
    DAT_004eefc8 = DAT_004eefc8 + unaff_retaddr;
    if (*(int **)((int)this + 0x958) != (int *)0x0) {
      (**(code **)(**(int **)((int)this + 0x958) + 0x4b4))(1);
    }
    uVar5 = __ftol();
    uVar6 = __ftol();
    __ftol();
    uVar7 = __ftol();
    FUN_00468660(0x10f,0xf,0x54494d45,&DAT_004ef3d8,uVar5);
    FUN_00468660(0x133,0xf,0x54494d45,&DAT_004ef3d8,uVar6);
    FUN_00468660(0x157,0xf,0x54494d45,&DAT_004ef3d8,uVar7);
    param_2 = unaff_retaddr;
    if (0.0 < _DAT_004eefcc) {
      uVar5 = __ftol();
      FUN_00468660(0x217,0x1e,0x54494d45,&PTR_DAT_004ef3d4,uVar5);
    }
  }
  if (_DAT_004eefd0 < 0.0) {
    if ((DAT_00509948[0x124] == 0x4c563442) &&
       (_DAT_004eefd0 = _DAT_004eefd0 - unaff_retaddr, _DAT_004eefd0 < -8.0)) {
      FUN_00460e70(s_RestartLevel_tsk_004ec7e4);
    }
  }
  else {
    if (((DAT_005099e4 != 0) && (*(char *)((int)this + 0x1d91) == '\0')) &&
       (_DAT_004eefd0 = _DAT_004eefd0 - unaff_retaddr, _DAT_004eefd0 < 0.0)) {
      FUN_00458980(0xffffffff,0x5b,0);
    }
    uVar5 = __ftol();
    uVar6 = __ftol();
    __ftol();
    uVar7 = __ftol();
    FUN_00468660(0x10f,0xf,0x54494d45,&DAT_004ef3d8,uVar5);
    FUN_00468660(0x133,0xf,0x54494d45,&DAT_004ef3d8,uVar6);
    FUN_00468660(0x157,0xf,0x54494d45,&DAT_004ef3d8,uVar7);
    param_2 = unaff_retaddr;
  }
  iVar10 = (**(code **)(*(int *)this + 700))(auStack_14);
  if (0.0 < *(float *)(iVar10 + 4)) {
    (**(code **)(*piVar9 + 0x1a4))(param_2);
  }
  (**(code **)(*piVar9 + 0x1a8))(param_2);
  cVar3 = (**(code **)(*(int *)this + 0x218))();
  if ((cVar3 == '\0') && (*(char *)((int)this + 0x1d41) != '\0')) {
    (**(code **)(*piVar9 + 0x194))();
  }
  C3DPlayer::vfunc_01_241(this);
  (**(code **)(*piVar9 + 0x1c8))(param_2);
  cVar3 = (**(code **)(*(int *)this + 0x218))();
  if (cVar3 != '\0') {
    puVar8 = (undefined4 *)(**(code **)(*(int *)this + 0x310))(&stack0xffffffe0);
    *(undefined4 *)((int)this + 0x1d4c) = *puVar8;
    *(undefined4 *)((int)this + 0x1d50) = puVar8[1];
    *(undefined4 *)((int)this + 0x1d54) = puVar8[2];
    *(undefined4 *)((int)this + 0x1d58) = puVar8[3];
  }
  (**(code **)(*piVar9 + 0x1b4))(param_2);
  (**(code **)(*piVar9 + 0x1b0))(param_2);
  iVar10 = *(int *)((int)this + 0x95c);
  if (((iVar10 != 0) && (*(short *)(iVar10 + 0x9a0) == 4)) && (9.0 < *(float *)(iVar10 + 0x6bc))) {
    *(undefined4 *)(iVar10 + 0x6bc) = 0;
    piVar9 = *(int **)((int)this + 0x95c);
    if (*(short *)((int)piVar9 + 0x9a2) == 1) {
      (**(code **)(*piVar9 + 0x17c))(2);
      return;
    }
    if (*(short *)((int)piVar9 + 0x9a2) == 3) {
      (**(code **)(*piVar9 + 0x17c))(0);
    }
  }
  return;
}


```

## UpdateJimmyActiveController_00426030 @ 00426030

```c

void __thiscall UpdateJimmyActiveController_00426030(void *this,float param_2)

{
  int *piVar1;
  float fVar2;
  char cVar3;
  short sVar4;
  int iVar5;
  undefined4 uVar6;
  int iVar7;
  float *pfVar8;
  int *piVar9;
  float *pfVar10;
  undefined4 *puVar11;
  char *pcVar12;
  C3DPlayer *extraout_ECX;
  int *piVar13;
  C3DPlayer *extraout_ECX_00;
  C3DPlayer *extraout_ECX_01;
  C3DPlayer *extraout_ECX_02;
  C3DPlayer *this_00;
  CGameObject *this_01;
  float unaff_EBP;
  float fVar14;
  float unaff_retaddr;
  char *pcVar15;
  float fStack_48;
  float fStack_44;
  float fStack_40;
  float fStack_3c;
  float afStack_38 [3];
  float fStack_2c;
  float fStack_28;
  undefined1 auStack_24 [4];
  undefined1 auStack_20 [4];
  float fStack_1c;
  undefined1 auStack_18 [12];
  undefined4 uStack_c;
  undefined4 uStack_4;

  cVar3 = FUN_00407060();
  if (cVar3 != '\0') {
    return;
  }
  cVar3 = FUN_00475ca0();
  if (cVar3 == '\0') {
    return;
  }
  if (DAT_005099e4 == 0) {
    return;
  }
  if (DAT_004f8430 != '\0') {
    return;
  }
  if (*(char *)((int)this + 0x1d2d) != '\0') {
    return;
  }
  C3DPlayer::vfunc_01_243(this);
  piVar1 = (int *)((int)this + -0xc0);
  cVar3 = (**(code **)(*(int *)((int)this + -0xc0) + 0x188))(param_2);
  if (cVar3 != '\0') {
    return;
  }
  cVar3 = (**(code **)(*(int *)this + 0x218))();
  if (cVar3 == '\0') {
    if (((*(char *)((int)this + 0x7c0) != '\0') && (DAT_004f8210 == '\0')) && (DAT_004f0588 != 0)) {
      iVar5 = (**(code **)(*(int *)this + 700))(auStack_24);
      if (-900.0 <= *(float *)(iVar5 + 4)) {
        if ((*(char *)((int)this + 0x1da4) != '\0') && (*(float *)((int)this + 0x1da8) == 0.0)) {
          *(undefined1 *)((int)this + 0x1da4) = 0;
          (**(code **)(*piVar1 + 0x140))();
          *(undefined2 *)((int)this + 0x728) = 0;
          (**(code **)(*piVar1 + 0xe0))(&DAT_004ef3e8,1);
          if (*(int *)((int)this + 0x738) != 0) {
            *(undefined4 *)(*(int *)((int)this + 0x738) + 0x6c0) = 0;
          }
        }
      }
      else {
        sVar4 = FUN_00403950(0,3);
        if (sVar4 == 1) {
          if (*(int *)((int)this + 0x738) != 0) {
            *(undefined4 *)(*(int *)((int)this + 0x738) + 0x6c0) = 0x40400000;
          }
          (**(code **)(*piVar1 + 0x150))(&PTR_DAT_004eca60);
          if (*(int *)((int)this + 0x97c) == -1) {
            uVar6 = FUN_00458980(0xffffffff,1,1);
            *(undefined4 *)((int)this + 0x97c) = uVar6;
          }
          *(undefined1 *)((int)this + 0x1da4) = 1;
          *(undefined4 *)((int)this + 0x1da8) = 0x40a00000;
          (**(code **)(*piVar1 + 0xe0))(&PTR_DAT_004eca60,1);
          *(undefined2 *)((int)this + 0x728) = 5;
          (**(code **)(*(int *)this + 0x2cc))(0,0x42c80000,0);
          *(undefined2 *)((int)this + 0x704) = 1;
        }
        else {
          iVar5 = (**(code **)(*(int *)this + 700))(auStack_24);
          if (*(float *)(iVar5 + 4) < -1100.0) {
            *(undefined1 *)((int)this + 0x1d98) = 1;
          }
        }
      }
    }
  }
  else if (*(char *)((int)this + 0x1d98) != '\0') {
    *(undefined1 *)((int)this + 0x1d98) = 0;
    FUN_00458980(0xffffffff,0xc0,0);
    (**(code **)(*piVar1 + 0x1a0))(0xc1200000,0xfffffff6,0x40a00000);
  }
  if (*(short *)((int)this + 0x704) == 2) {
    FUN_0042a920(unaff_retaddr * -0.5);
    if (DAT_004f83d4 <= 0.0) {
      (**(code **)(*piVar1 + 0x178))();
    }
    afStack_38[0] = 0.0;
    (**(code **)(*(int *)this + 0x310))(auStack_24);
    iVar5 = (**(code **)(*(int *)this + 0x310))(&fStack_28);
    fStack_48 = (*(float *)(iVar5 + 4) + 100.0) - unaff_EBP;
    fVar2 = *(float *)(DAT_00509a50 + 0x4c);
    iVar5 = (**(code **)(*(int *)this + 0x310))(&fStack_2c);
    fStack_3c = *(float *)(iVar5 + 8) - fStack_48;
    fVar14 = OMedia3DVector::quick_magnitude((OMedia3DVector *)&fStack_44);
    fVar14 = 1.0 / fVar14;
    fStack_44 = fVar14 * fStack_44;
    fStack_40 = fVar14 * fStack_40;
    fStack_3c = fVar14 * fStack_3c;
    if (*(int *)((int)this + 0x954) != 0) {
      iVar5 = *(int *)(*(int *)((int)this + 0x954) + 200);
      iVar7 = (**(code **)(*(int *)this + 0x310))(auStack_24);
      iVar7 = (**(code **)(*(int *)this + 0x310))
                        (afStack_38,*(float *)(iVar7 + 8) - fStack_40 * 80.0);
      pfVar8 = (float *)(**(code **)(*(int *)this + 0x310))
                                  (&fStack_1c,(*(float *)(iVar7 + 4) + 50.0) - fStack_48 * 80.0);
      (**(code **)(iVar5 + 0x318))(*pfVar8 - fVar2 * 80.0);
      param_2 = unaff_retaddr;
    }
  }
  (**(code **)(*piVar1 + 0x1bc))(param_2);
  piVar9 = (int *)FUN_00458e40(0x33504943);
  if (piVar9 == (int *)0x0) {
    if (*(int *)((int)this + 0x1d94) == 0) {
      piVar9 = (int *)0x0;
    }
    else {
      piVar9 = (int *)(*(int *)((int)this + 0x1d94) + -0xc0);
    }
    iVar5 = *piVar9;
    uVar6 = 1;
LAB_00426563:
    (**(code **)(iVar5 + 0x58))(uVar6);
    this_00 = extraout_ECX_02;
  }
  else {
    pfVar8 = (float *)(**(code **)(*piVar9 + 0x310))(auStack_18);
    pfVar10 = (float *)(**(code **)(*(int *)this + 0x310))(&fStack_2c);
    fVar2 = SQRT((pfVar8[1] - pfVar10[1]) * (pfVar8[1] - pfVar10[1]) +
                 (pfVar8[2] - pfVar10[2]) * (pfVar8[2] - pfVar10[2]) +
                 (*pfVar8 - *pfVar10) * (*pfVar8 - *pfVar10));
    if (850.0 <= fVar2) {
      if (*(int *)((int)this + 0x1d94) == 0) {
        (**(code **)(iRam00000000 + 0x58))(1);
        this_00 = extraout_ECX_01;
      }
      else {
        (**(code **)(*(int *)(*(int *)((int)this + 0x1d94) + -0xc0) + 0x58))(1);
        this_00 = extraout_ECX_00;
      }
    }
    else {
      this_00 = extraout_ECX;
      if (piVar9[0x1a8] != 0) {
        if (*(int *)((int)this + 0x1d94) == 0) {
          piVar13 = (int *)0x0;
        }
        else {
          piVar13 = (int *)(*(int *)((int)this + 0x1d94) + -0xc0);
        }
        (**(code **)(*piVar13 + 0x114))(1.0 - fVar2 * 0.0011764705);
        puVar11 = (undefined4 *)(**(code **)(*piVar9 + 0x310))(&fStack_1c);
        (**(code **)(**(int **)((int)this + 0x1d94) + 0x314))
                  (*puVar11,puVar11[1],puVar11[2],puVar11[3]);
        (**(code **)(**(int **)((int)this + 0x1d94) + 0x334))(0,fStack_1c * -100.0,0);
        if (*(int *)((int)this + 0x1d94) == 0) {
          piVar9 = (int *)0x0;
        }
        else {
          piVar9 = (int *)(*(int *)((int)this + 0x1d94) + -0xc0);
        }
        this_00 = (C3DPlayer *)0x0;
        if (piVar9[0x1c] != 0) {
          iVar5 = *piVar9;
          piVar9[0x1c] = 1;
          uVar6 = 0;
          goto LAB_00426563;
        }
      }
    }
  }
  cVar3 = C3DPlayer::vfunc_04_087(this_00);
  if ((cVar3 == '\0') && (cVar3 = FUN_0046a3e0(0xd), cVar3 == '\0')) {
    (**(code **)(**(int **)(*(int *)((int)this + 0x958) + 0x4e4) + 0x58))(1);
    *(undefined1 *)((int)this + 0x1d65) = 0;
    if (*(char *)((int)this + 0x1d66) != '\0') {
      FUN_00458980(0xffffffff,10,0);
      (**(code **)(*piVar1 + 0x148))();
      (**(code **)(*piVar1 + 0x180))(0,0x42a00000,0x42480000,*(undefined4 *)((int)this + 0x1d68));
      *(undefined4 *)((int)this + 0xa48) = 0;
      *(undefined1 *)((int)this + 0x1d66) = 0;
      FUN_0042a920(0xbf800000);
    }
    if (*(int *)((int)this + 0x974) != -1) {
      FUN_00458a00(*(int *)((int)this + 0x974),0);
      *(undefined4 *)((int)this + 0x974) = 0xffffffff;
    }
    if (*(int *)((int)this + 0x988) != -1) {
      FUN_00458a00(*(int *)((int)this + 0x988),0);
      *(undefined4 *)((int)this + 0x988) = 0xffffffff;
    }
    if (*(float *)((int)this + 0x608) < 10.0) {
      pcVar15 = s_HISHOOT_004ef28c;
      pcVar12 = (char *)(**(code **)(*piVar1 + 0xe8))();
      iVar5 = __strcmpi(pcVar12,pcVar15);
      if (iVar5 == 0) {
        (**(code **)(*piVar1 + 0x148))();
      }
    }
    if (*(int **)((int)this + 0x950) == (int *)0x0) {
      return;
    }
    (**(code **)(**(int **)((int)this + 0x950) + 0x58))(1);
    return;
  }
  if (*(char *)((int)this + 0x1d65) != '\0') {
    return;
  }
  if (DAT_004f8182 != '\0') {
    return;
  }
  if (DAT_004f0588 == 0) {
    return;
  }
  *(undefined4 *)((int)this + 0x608) = 0;
  cVar3 = (**(code **)(*piVar1 + 0x1c0))(uStack_4);
  if (cVar3 != '\0') {
    return;
  }
  cVar3 = (**(code **)(*piVar1 + 0x1c4))(uStack_4);
  if (cVar3 != '\0') {
    return;
  }
  if (DAT_004f0588 == 4) {
    return;
  }
  cVar3 = (**(code **)(*(int *)this + 0x218))();
  if (cVar3 == '\0') {
    if ((DAT_004f0588 == 2) || (sVar4 = FUN_00403950(0,3), sVar4 != 1)) {
LAB_00426996:
      FUN_00458980(0xffffffff,0x75,0);
      *(undefined1 *)((int)this + 0x1d65) = 1;
      goto LAB_004269aa;
    }
    pcVar12 = &DAT_004ef3e8;
    if (*(int *)((int)this + 0x974) != -1) {
      pcVar15 = (char *)(**(code **)(*piVar1 + 0x100))();
      iVar5 = __strcmpi(pcVar15,pcVar12);
      if (iVar5 == 0) {
        if (*(int *)((int)this + 0x974) != -1) {
          FUN_00458a00(*(int *)((int)this + 0x974),0);
          *(undefined4 *)((int)this + 0x974) = 0xffffffff;
        }
        (**(code **)(**(int **)((int)this + 0x950) + 0x58))(1);
        return;
      }
      goto LAB_004269aa;
    }
    pcVar15 = (char *)(**(code **)(*piVar1 + 0x100))();
    iVar5 = __strcmpi(pcVar15,pcVar12);
    if (iVar5 == 0) {
      return;
    }
  }
  else {
    CGameObject::vfunc_00_013(this_01);
    (**(code **)(*piVar1 + 0xe0))(s_SHOOT_004ef448,1);
    (**(code **)(*piVar1 + 0x148))();
    (**(code **)(**(int **)(*(int *)((int)this + 0x958) + 0x4e4) + 0x58))(1);
    if (DAT_004f0588 == 6) {
      uVar6 = __ftol();
      uVar6 = __ftol(0x54494d45,s____2d_004ef440,uVar6);
      FUN_00468660(0x11e,uVar6);
      piVar9 = *(int **)(*(int *)((int)this + 0x958) + 0x4e4);
      if (piVar9[0x1c] != 0) {
        piVar9[0x1c] = 1;
        (**(code **)(*piVar9 + 0x58))(0);
      }
      if (*(char *)((int)this + 0x1d66) == '\0') {
        *(undefined1 *)((int)this + 0x1d66) = 1;
        (**(code **)(*piVar1 + 0x148))();
      }
      else {
        if (45.0 < *(float *)((int)this + 0x78c)) {
          *(undefined4 *)((int)this + 0x78c) = 0x42340000;
        }
        if (*(float *)((int)this + 0x78c) < 0.0) {
          *(undefined4 *)((int)this + 0x78c) = 0;
        }
        iVar5 = (**(code **)(*(int *)this + 0x310))(auStack_20);
        fVar2 = *(float *)(iVar5 + 4);
        fVar14 = *(float *)(iVar5 + 8);
        pfVar8 = (float *)(**(code **)(*(int *)this + 900))
                                    (auStack_24,0,*(float *)((int)this + 0x78c) + 80.0,0x42340000);
        fStack_40 = *pfVar8 - (fVar2 + 80.0);
        fStack_2c = pfVar8[1];
        fStack_28 = pfVar8[2];
        fStack_3c = fStack_2c - fVar14;
        afStack_38[1] = 0.0;
        afStack_38[0] = fStack_28 - fStack_48;
        OMedia3DVector::angles
                  ((OMedia3DVector *)&fStack_40,(short *)&stack0xffffffac,(short *)&stack0xffffffaa)
        ;
        *(undefined4 *)((int)this + 0x1d68) = *(undefined4 *)((int)this + 0x78c);
      }
      goto LAB_004269aa;
    }
    if ((DAT_004f0588 == 2) || (sVar4 = FUN_00403950(0,3), sVar4 != 1)) goto LAB_00426996;
    if (*(int *)((int)this + 0x974) != -1) goto LAB_004269aa;
  }
  uVar6 = FUN_00458980(0xffffffff,0,1);
  *(undefined4 *)((int)this + 0x974) = uVar6;
  (**(code **)(*piVar1 + 0x148))();
LAB_004269aa:
  if (DAT_004f0588 != 4) {
    (**(code **)(*piVar1 + 0x1b8))(uStack_c);
  }
  return;
}


```

## JimmySetupOrReset_00423610 @ 00423610

```c

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall JimmySetupOrReset_00423610(void *this)

{
  undefined4 uVar1;
  int iVar2;
  char cVar3;
  int iVar4;
  undefined4 *puVar5;
  int iVar6;
  undefined4 uVar7;
  uint uVar8;
  uint uVar9;
  CGameObject *extraout_ECX;
  CGameObject *extraout_ECX_00;
  CGameObject *extraout_ECX_01;
  CGameObject *extraout_ECX_02;
  CGameObject *extraout_ECX_03;
  CGameObject *extraout_ECX_04;
  CGameObject *extraout_ECX_05;
  CGameObject *extraout_ECX_06;
  CGameObject *pCVar10;
  char *pcVar11;
  char *pcVar12;
  char *pcVar13;
  int *piVar14;
  int *piVar15;
  undefined4 *puVar16;
  undefined4 uStackY_16c;
  undefined4 uStackY_168;
  char **ppcStackY_164;
  char *pcStackY_160;
  undefined1 *puStackY_15c;
  undefined4 uStackY_158;
  undefined1 *puStackY_154;
  undefined4 uStackY_150;
  undefined1 *puStackY_14c;
  undefined4 uStackY_148;
  undefined1 *puStackY_144;
  undefined4 uStackY_140;
  undefined1 *puStackY_13c;
  undefined4 uStackY_138;
  undefined1 *puStackY_134;
  char *pcStackY_130;
  undefined1 *puStackY_12c;
  int *piVar17;
  undefined4 uStack_e8;
  undefined4 uStack_e4;
  undefined1 auStack_d8 [28];
  undefined1 auStack_bc [8];
  undefined1 auStack_b4 [36];
  undefined1 auStack_90 [40];
  undefined1 auStack_68 [12];
  undefined1 auStack_5c [24];
  undefined1 auStack_44 [16];
  void *pvStack_34;
  undefined4 uStack_28;
  void *pvStack_c;
  undefined1 *puStack_8;
  undefined4 uStack_4;

  uStack_4 = 0xffffffff;
  puStack_8 = &LAB_0048910c;
  pvStack_c = ExceptionList;
  ExceptionList = &pvStack_c;
  CGameObject::vfunc_00_013(this);
  *(undefined1 *)((int)this + 0x1d91) = 0;
  DAT_004f8434 = 0;
  DAT_00696008 = FUN_0046a910();
  uVar8 = 0xffffffff;
  pcVar12 = &DAT_004eca6c;
  do {
    pcVar11 = pcVar12;
    if (uVar8 == 0) break;
    uVar8 = uVar8 - 1;
    pcVar11 = pcVar12 + 1;
    cVar3 = *pcVar12;
    pcVar12 = pcVar11;
  } while (cVar3 != '\0');
  uVar8 = ~uVar8;
  pcVar12 = pcVar11 + -uVar8;
  pcVar11 = (char *)((int)this + 0x98c);
  for (uVar9 = uVar8 >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
    *(undefined4 *)pcVar11 = *(undefined4 *)pcVar12;
    pcVar12 = pcVar12 + 4;
    pcVar11 = pcVar11 + 4;
  }
  for (uVar8 = uVar8 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
    *pcVar11 = *pcVar12;
    pcVar12 = pcVar12 + 1;
    pcVar11 = pcVar11 + 1;
  }
  (**(code **)(*(int *)((int)this + -0xc0) + 0xac))();
  (**(code **)(*(int *)this + 0x110))();
  (**(code **)(*(int *)this + 0x100))();
  *(undefined1 *)((int)this + 0x938) = 0;
  _DAT_004f6b3c = 0x3d8f5c29;
  DAT_004f8430 = 0;
  *(undefined1 *)((int)this + 0x49f) = 0;
  *(undefined1 *)((int)this + 0x1d43) = 0;
  *(undefined1 *)((int)this + 0x1d2d) = 0;
  *(undefined1 *)((int)this + 0x1d2c) = 0;
  (**(code **)(*DAT_00509948 + 0x168))();
  (**(code **)(*(int *)((int)this + -0xc0) + 0x11c))();
  DAT_004f6b34 = DAT_00695f80;
  pCVar10 = (CGameObject *)0x0;
  if (*(int **)((int)this + 0x95c) != (int *)0x0) {
    (**(code **)(**(int **)((int)this + 0x95c) + 0x11c))();
    pCVar10 = extraout_ECX;
  }
  if (DAT_00696008 != 0) {
    DAT_004f8434 = 1;
  }
  if (DAT_004f0588 == 5) {
    (**(code **)(*(int *)((int)this + -0xc0) + 0x178))();
    pCVar10 = extraout_ECX_00;
  }
  if (*(int *)((int)this + 0x974) != -1) {
    FUN_00458a00();
    *(undefined4 *)((int)this + 0x974) = 0xffffffff;
    pCVar10 = extraout_ECX_01;
  }
  if (*(int *)((int)this + 0x988) != -1) {
    FUN_00458a00();
    *(undefined4 *)((int)this + 0x988) = 0xffffffff;
    pCVar10 = extraout_ECX_02;
  }
  CGameObject::vfunc_00_013(pCVar10);
  *(undefined2 *)((int)this + 0x728) = 0;
  *(undefined4 *)((int)this + 0x970) = 0;
  piVar17 = *(int **)((int)this + 0x958);
  if ((piVar17 != (int *)0x0) && (*(char *)((int)piVar17 + 0x4be) != '\0')) {
    (**(code **)(*piVar17 + 0x474))();
  }
  C3DAnimated::vfunc_01_259(this);
  pcVar12 = (char *)((int)this + 0x6b4);
  iVar4 = __strcmpi(pcVar12,&DAT_004eca6c);
  if (iVar4 != 0) {
    uVar8 = 0xffffffff;
    pcVar11 = pcVar12;
    do {
      pcVar13 = pcVar11;
      if (uVar8 == 0) break;
      uVar8 = uVar8 - 1;
      pcVar13 = pcVar11 + 1;
      cVar3 = *pcVar11;
      pcVar11 = pcVar13;
    } while (cVar3 != '\0');
    uVar8 = ~uVar8;
    pcVar11 = pcVar13 + -uVar8;
    pcVar13 = (char *)((int)this + 0x664);
    for (uVar9 = uVar8 >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
      *(undefined4 *)pcVar13 = *(undefined4 *)pcVar11;
      pcVar11 = pcVar11 + 4;
      pcVar13 = pcVar13 + 4;
    }
    for (uVar8 = uVar8 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
      *pcVar13 = *pcVar11;
      pcVar11 = pcVar11 + 1;
      pcVar13 = pcVar13 + 1;
    }
  }
  uVar8 = 0xffffffff;
  pcVar11 = &DAT_004eca6c;
  do {
    pcVar13 = pcVar11;
    if (uVar8 == 0) break;
    uVar8 = uVar8 - 1;
    pcVar13 = pcVar11 + 1;
    cVar3 = *pcVar11;
    pcVar11 = pcVar13;
  } while (cVar3 != '\0');
  uVar8 = ~uVar8;
  pcVar11 = pcVar13 + -uVar8;
  for (uVar9 = uVar8 >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
    *(undefined4 *)pcVar12 = *(undefined4 *)pcVar11;
    pcVar11 = pcVar11 + 4;
    pcVar12 = pcVar12 + 4;
  }
  for (uVar8 = uVar8 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
    *pcVar12 = *pcVar11;
    pcVar11 = pcVar11 + 1;
    pcVar12 = pcVar12 + 1;
  }
  iVar4 = __strcmpi((char *)((int)this + 0x664),(char *)&DAT_004f81a8);
  if ((iVar4 != 0) && (piVar17 = (int *)*DAT_0050999c, piVar17 != DAT_0050999c)) {
    do {
      piVar14 = (int *)piVar17[2];
      if ((piVar14 != (int *)0x0) &&
         ((cVar3 = (**(code **)(*piVar14 + 0x18))(), cVar3 != '\0' &&
          (iVar4 = __strcmpi((char *)((int)this + 0x664),(char *)(piVar14 + 0xe8)), iVar4 == 0)))) {
        uVar8 = 0xffffffff;
        pcVar12 = &DAT_004eca6c;
        do {
          pcVar11 = pcVar12;
          if (uVar8 == 0) break;
          uVar8 = uVar8 - 1;
          pcVar11 = pcVar12 + 1;
          cVar3 = *pcVar12;
          pcVar12 = pcVar11;
        } while (cVar3 != '\0');
        uVar8 = ~uVar8;
        pcVar12 = pcVar11 + -uVar8;
        pcVar11 = (char *)((int)this + 0x98c);
        for (uVar9 = uVar8 >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
          *(undefined4 *)pcVar11 = *(undefined4 *)pcVar12;
          pcVar12 = pcVar12 + 4;
          pcVar11 = pcVar11 + 4;
        }
        for (uVar8 = uVar8 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
          *pcVar11 = *pcVar12;
          pcVar12 = pcVar12 + 1;
          pcVar11 = pcVar11 + 1;
        }
        (**(code **)(*(int *)this + 0x2c))();
        if (*(int **)((int)this + 0x738) != (int *)0x0) {
          (**(code **)(**(int **)((int)this + 0x738) + 0x2c))();
          (**(code **)(**(int **)((int)this + 0x738) + 0x34))();
          (**(code **)(*(int *)(*(int *)((int)this + 0x738) + 0xc0) + 0x2c))();
        }
        puVar5 = (undefined4 *)(**(code **)(*piVar14 + 0x164))();
        *(undefined4 *)((int)this + 0x1d4c) = *puVar5;
        *(undefined4 *)((int)this + 0x1d50) = puVar5[1];
        *(undefined4 *)((int)this + 0x1d54) = puVar5[2];
        *(undefined4 *)((int)this + 0x1d58) = puVar5[3];
        (**(code **)(*piVar14 + 0x164))();
        puStackY_12c = (undefined1 *)0x42395a;
        (**(code **)(*(int *)this + 0x314))();
        iVar4 = *(int *)this;
        puStackY_12c = auStack_68;
        pcStackY_130 = (char *)0x42396f;
        iVar6 = (**(code **)(*piVar14 + 0x164))();
        pcStackY_130 = *(char **)(iVar6 + 8);
        puStackY_134 = auStack_5c;
        uStackY_138 = 0x423985;
        iVar6 = (**(code **)(*piVar14 + 0x164))();
        uStackY_138 = *(undefined4 *)(iVar6 + 4);
        puStackY_13c = &stack0xffffff10;
        uStackY_140 = 0x423998;
        puVar5 = (undefined4 *)(**(code **)(*piVar14 + 0x164))();
        uStackY_140 = *puVar5;
        puStackY_144 = (undefined1 *)0x4239a3;
        (**(code **)(iVar4 + 0x16c))();
        iVar4 = *(int *)this;
        puStackY_144 = auStack_90;
        uStackY_148 = 0x4239b8;
        iVar6 = (**(code **)(*piVar14 + 0x170))();
        uStackY_148 = *(undefined4 *)(iVar6 + 8);
        puStackY_14c = auStack_b4;
        uStackY_150 = 0x4239ce;
        iVar6 = (**(code **)(*piVar14 + 0x170))();
        uStackY_150 = *(undefined4 *)(iVar6 + 4);
        puStackY_154 = &stack0xffffff08;
        uStackY_158 = 0x4239e1;
        puVar5 = (undefined4 *)(**(code **)(*piVar14 + 0x170))();
        uStackY_158 = *puVar5;
        puStackY_15c = (undefined1 *)0x4239ec;
        (**(code **)(iVar4 + 0x174))();
        if (*(int *)((int)this + 0x95c) != 0) {
          puStackY_15c = &stack0xffffff08;
          pcStackY_160 = (char *)0x423a05;
          puVar5 = (undefined4 *)(**(code **)(*piVar14 + 0x164))();
          uStackY_16c = *puVar5;
          puStackY_14c = (undefined1 *)&uStackY_16c;
          uStackY_168 = puVar5[1];
          ppcStackY_164 = (char **)puVar5[2];
          pcStackY_160 = (char *)puVar5[3];
          (**(code **)(*(int *)(*(int *)((int)this + 0x95c) + 0xc0) + 0x168))();
        }
        puStackY_15c = auStack_d8;
        pcStackY_160 = (char *)0x423a4e;
        iVar4 = (**(code **)(*piVar14 + 0x164))();
        puStackY_134 = *(undefined1 **)(iVar4 + 8);
        pcStackY_160 = auStack_bc;
        ppcStackY_164 = (char **)0x423a67;
        iVar4 = (**(code **)(*piVar14 + 0x164))();
        puStackY_134 = *(undefined1 **)(iVar4 + 4);
        ppcStackY_164 = &pcStackY_130;
        uStackY_168 = 0x423a7d;
        puVar5 = (undefined4 *)(**(code **)(*piVar14 + 0x164))();
        *(undefined4 *)((int)this + -0x7c) = *puVar5;
        *(undefined4 *)((int)this + -0x78) = uStack_e4;
        *(undefined4 *)((int)this + -0x74) = uStack_e8;
        if (*(char *)((int)this + 0x8e5) != '\0') {
          *(undefined1 *)((int)this + 0x8e5) = 0;
          (**(code **)(*(int *)this + 0x16c))();
          *(undefined4 *)((int)this + -0x7c) = *(undefined4 *)((int)this + 0x8e8);
          *(undefined4 *)((int)this + -0x78) = *(undefined4 *)((int)this + 0x8ec);
          *(undefined4 *)((int)this + -0x74) = *(undefined4 *)((int)this + 0x8f0);
          puStackY_12c = *(undefined1 **)((int)this + 0x8e8);
          pcStackY_130 = (char *)0x423b24;
          (**(code **)(*(int *)(*(int *)((int)this + 0x95c) + 0xc0) + 0x168))();
        }
        if (DAT_00509948 != (int *)0x0) {
          (**(code **)(*DAT_00509948 + 0x118))();
        }
        iVar2 = DAT_00509a50;
        iVar4 = piVar14[0x14a];
        iVar6 = piVar14[0x149];
        *(int *)(DAT_00509a50 + 0x44) = piVar14[0x148];
        *(int *)(iVar2 + 0x48) = iVar6;
        *(int *)(iVar2 + 0x4c) = iVar4;
        FUN_0047d7a0();
        FUN_0046aef0();
        FUN_0046aef0();
        *(undefined4 *)((int)this + 0x978) = 0xffffffff;
        puStackY_12c = (undefined1 *)0x423ba4;
        iVar4 = __strcmpi((char *)(piVar14 + 0x150),&DAT_004eca6c);
        if (iVar4 != 0) {
          uVar8 = 0xffffffff;
          piVar14 = piVar14 + 0x150;
          do {
            piVar15 = piVar14;
            if (uVar8 == 0) break;
            uVar8 = uVar8 - 1;
            piVar15 = (int *)((int)piVar14 + 1);
            iVar4 = *piVar14;
            piVar14 = piVar15;
          } while ((char)iVar4 != '\0');
          uVar8 = ~uVar8;
          puVar5 = (undefined4 *)((int)piVar15 - uVar8);
          puVar16 = (undefined4 *)((int)this + 0x98c);
          for (uVar9 = uVar8 >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
            *puVar16 = *puVar5;
            puVar5 = puVar5 + 1;
            puVar16 = puVar16 + 1;
          }
          for (uVar8 = uVar8 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
            *(undefined1 *)puVar16 = *(undefined1 *)puVar5;
            puVar5 = (undefined4 *)((int)puVar5 + 1);
            puVar16 = (undefined4 *)((int)puVar16 + 1);
          }
          DAT_00696004 = FUN_0046a910();
          if ((DAT_00696004 != 0) && (*(int *)((int)this + 0x978) == -1)) {
            uVar7 = FUN_0047d710();
            *(undefined4 *)((int)this + 0x978) = uVar7;
          }
        }
      }
      piVar17 = (int *)*piVar17;
    } while (piVar17 != DAT_0050999c);
  }
  puVar5 = (undefined4 *)(**(code **)(*(int *)this + 900))();
  uVar7 = puVar5[1];
  uVar1 = puVar5[2];
  *(undefined4 *)(DAT_00509a50 + 0x44) = *puVar5;
  *(undefined4 *)(DAT_00509a50 + 0x48) = uVar7;
  *(undefined4 *)(DAT_00509a50 + 0x4c) = uVar1;
  if (*(int *)((int)this + 0x95c) == 0) {
    iVar4 = FUN_00478990();
    uStack_28 = 0;
    if (iVar4 == 0) {
      iVar4 = 0;
      pCVar10 = extraout_ECX_03;
    }
    else {
      iVar4 = FUN_0041c810();
      pCVar10 = extraout_ECX_04;
    }
    uStack_28 = 0xffffffff;
    *(int *)((int)this + 0x95c) = iVar4;
    if (iVar4 == 0) {
      puStackY_12c = (undefined1 *)0x0;
    }
    else {
      puStackY_12c = (undefined1 *)(iVar4 + 0xc0);
    }
    pcStackY_130 = (char *)0x423ccf;
    CGameObject::vfunc_00_013(pCVar10);
  }
  else {
    puStackY_12c = (undefined1 *)0x0;
    pcStackY_130 = (char *)0x423ce8;
    (**(code **)(*(int *)(*(int *)((int)this + 0x95c) + 0xc0) + 0x174))();
  }
  if (*(int *)((int)this + 0x970) == 0) {
    iVar4 = FUN_00478990();
    uStack_28 = 1;
    if (iVar4 == 0) {
      iVar4 = 0;
      pCVar10 = extraout_ECX_05;
    }
    else {
      iVar4 = FUN_004211a0();
      pCVar10 = extraout_ECX_06;
    }
    uStack_28 = 0xffffffff;
    *(int *)((int)this + 0x970) = iVar4;
    if (iVar4 != 0) {
      puStackY_12c = (undefined1 *)(iVar4 + 0xc0);
      pcStackY_130 = (char *)0x423d4d;
      CGameObject::vfunc_00_013(pCVar10);
      puStackY_12c = (undefined1 *)0x3f800000;
      pcStackY_130 = (char *)0x40400000;
      puStackY_134 = (undefined1 *)0x40400000;
      uStackY_138 = 0xc1f00000;
      puStackY_13c = (undefined1 *)0x42920000;
      uStackY_140 = 0x423d7b;
      (**(code **)(**(int **)((int)this + 0x970) + 0x168))();
    }
  }
  if (*(int **)((int)this + 0x970) != (int *)0x0) {
    (**(code **)(**(int **)((int)this + 0x970) + 0xd8))();
    (**(code **)(*(int *)(*(int *)((int)this + 0x970) + 0xc0) + 0xd0))();
    puStackY_12c = (undefined1 *)0x423dc1;
    (**(code **)(*(int *)(*(int *)((int)this + 0x970) + 0xc0) + 0xb8))();
  }
  DAT_004eefc8 = 0xbf800000;
  _DAT_004eefd0 = 0xbf800000;
  if (*(int **)((int)this + 0x958) != (int *)0x0) {
    (**(code **)(**(int **)((int)this + 0x958) + 0x4b4))();
  }
  puStackY_12c = (undefined1 *)0x0;
  pcStackY_130 = "j";
  FUN_00403870();
  pcStackY_130 = (char *)0x0;
  puStackY_134 = (undefined1 *)0x3;
  uStackY_138 = 0;
  puStackY_13c = (undefined1 *)0x423dff;
  FUN_00403870();
  puStackY_13c = (undefined1 *)0x0;
  uStackY_140 = 0;
  puStackY_144 = (undefined1 *)0x0;
  uStackY_148 = 0x423e0a;
  FUN_00403870();
  uStackY_148 = 0;
  puStackY_14c = (undefined1 *)0x5;
  uStackY_150 = 0;
  puStackY_154 = (undefined1 *)0x423e15;
  FUN_00403870();
  puStackY_154 = (undefined1 *)0x0;
  uStackY_158 = 1;
  puStackY_15c = (undefined1 *)0x0;
  pcStackY_160 = "j";
  FUN_00403870();
  pcStackY_160 = (char *)0x0;
  ppcStackY_164 = (char **)0x6;
  uStackY_168 = 0;
  uStackY_16c = 0x423e2b;
  FUN_00403870();
  puStackY_12c = (undefined1 *)0x0;
  pcStackY_130 = "j";
  FUN_00403870();
  pcStackY_130 = (char *)0x0;
  puStackY_134 = (undefined1 *)0x4;
  uStackY_138 = 0;
  puStackY_13c = (undefined1 *)0x423e44;
  FUN_00403870();
  if ((0x56523030 < DAT_00509948[0x124]) && (DAT_00509948[0x124] < 0x56523039)) {
    puStackY_12c = (undefined1 *)0x0;
    *(undefined1 *)((int)this + 0x938) = 1;
    pcStackY_130 = "j\x01j\x03j";
    FUN_00403870();
    pcStackY_130 = (char *)0x1;
    puStackY_134 = (undefined1 *)0x3;
    uStackY_138 = 0;
    puStackY_13c = (undefined1 *)0x423e81;
    FUN_00403870();
    puStackY_13c = (undefined1 *)0x1;
    uStackY_140 = 0;
    puStackY_144 = (undefined1 *)0x0;
    uStackY_148 = 0x423e8c;
    FUN_00403870();
    uStackY_148 = 1;
    puStackY_14c = (undefined1 *)0x5;
    uStackY_150 = 0;
    puStackY_154 = (undefined1 *)0x423e97;
    FUN_00403870();
    puStackY_154 = (undefined1 *)0x1;
    uStackY_158 = 1;
    puStackY_15c = (undefined1 *)0x0;
    pcStackY_160 = "j\x01j\x06j";
    FUN_00403870();
    pcStackY_160 = (char *)0x1;
    ppcStackY_164 = (char **)0x6;
    uStackY_168 = 0;
    uStackY_16c = 0x423ead;
    FUN_00403870();
    puStackY_12c = (undefined1 *)0x0;
    pcStackY_130 = "j\x01j\x04j";
    FUN_00403870();
    pcStackY_130 = (char *)0x1;
    puStackY_134 = (undefined1 *)0x4;
    uStackY_138 = 0;
    puStackY_13c = (undefined1 *)0x423ec6;
    FUN_00403870();
    _DAT_004f8188 = 0x41a00000;
    (**(code **)(**(int **)((int)this + 0x958) + 0x4ec))();
  }
  iVar4 = DAT_00509948[0x124];
  if (iVar4 < 0x4c563542) {
    if (iVar4 == 0x4c563541) {
      puStackY_12c = (undefined1 *)0x0;
      pcStackY_130 = "j\x01j";
      FUN_00403870();
      pcStackY_130 = (char *)0x1;
      puStackY_134 = (undefined1 *)0x0;
      uStackY_138 = 0;
      puStackY_13c = (undefined1 *)0x424136;
      FUN_00403870();
      puStackY_13c = (undefined1 *)0x1;
      uStackY_140 = 5;
      puStackY_144 = (undefined1 *)0x0;
      uStackY_148 = 0x424141;
      FUN_00403870();
      uStackY_148 = 1;
      puStackY_14c = (undefined1 *)0x6;
      uStackY_150 = 0;
      puStackY_154 = (undefined1 *)0x42414c;
      FUN_00403870();
      puStackY_154 = (undefined1 *)0x1;
      uStackY_158 = 4;
      puStackY_15c = (undefined1 *)0x0;
      pcStackY_160 = (char *)0x424157;
      FUN_00403870();
      (**(code **)(*(int *)((int)this + -0xc0) + 0x178))();
      FUN_0045fee0();
      _DAT_004f6b3c = 0x3ca3d70a;
      iVar4 = *(int *)((int)this + -0xc0);
LAB_00424406:
      (**(code **)(iVar4 + 0x174))();
    }
    else if (iVar4 < 0x4c563242) {
      if (iVar4 == 0x4c563241) {
LAB_00424102:
        puStackY_12c = (undefined1 *)0x0;
        pcStackY_130 = "j\x01j\aj";
        FUN_00403870();
        pcStackY_130 = (char *)0x1;
        puStackY_134 = (undefined1 *)0x7;
        uStackY_138 = 0;
        puStackY_13c = (undefined1 *)0x424118;
        FUN_00403870();
      }
      else if (iVar4 < 0x4c455638) {
        if (iVar4 == 0x4c455637) {
          (**(code **)(*(int *)((int)this + -0xc0) + 0x178))();
          (**(code **)(*(int *)((int)this + -0xc0) + 0x174))();
          puStackY_12c = (undefined1 *)0x423f5d;
          FUN_00406f50();
          puStackY_12c = (undefined1 *)0x1;
          pcStackY_130 = (char *)0x7;
          puStackY_134 = (undefined1 *)0x0;
          uStackY_138 = 0x423f68;
          FUN_00403870();
          uStackY_138 = 1;
          puStackY_13c = (undefined1 *)0x3;
          uStackY_140 = 0;
          puStackY_144 = (undefined1 *)0x423f73;
          FUN_00403870();
          puStackY_144 = (undefined1 *)0x1;
          uStackY_148 = 5;
          puStackY_14c = (undefined1 *)0x0;
          uStackY_150 = 0x423f7e;
          FUN_00403870();
          uStackY_150 = 1;
          puStackY_154 = (undefined1 *)0x4;
          uStackY_158 = 0;
          puStackY_15c = (undefined1 *)0x423f89;
          FUN_00403870();
          puStackY_15c = (undefined1 *)0x1;
          pcStackY_160 = (char *)0x6;
          ppcStackY_164 = (char **)0x0;
          uStackY_168 = 0x423f94;
          FUN_00403870();
          puStackY_12c = (undefined1 *)0x0;
          pcStackY_130 = (char *)0x423fa2;
          FUN_00403870();
          pcStackY_130 = s_RestartLevel_tsk_004ec7e4;
          puStackY_134 = (undefined1 *)0x423fac;
          FUN_0045fee0();
        }
        else {
          if (iVar4 == 0x4c455633) goto LAB_0042402f;
          if (iVar4 == 0x4c455636) goto LAB_004241c6;
        }
      }
      else if (iVar4 == 0x4c455638) {
        (**(code **)(*(int *)((int)this + -0xc0) + 0x178))();
        (**(code **)(*(int *)((int)this + -0xc0) + 0x174))();
      }
      else if ((0x4c563140 < iVar4) && (iVar4 < 0x4c563144)) goto LAB_00424102;
    }
    else if (iVar4 < 0x4c563345) {
      if (iVar4 != 0x4c563344) {
        if (iVar4 < 0x4c563341) goto switchD_00424373_default;
        if (0x4c563342 < iVar4) {
          if (iVar4 != 0x4c563343) goto switchD_00424373_default;
          puStackY_12c = (undefined1 *)0x0;
          pcStackY_130 = (char *)0x42402c;
          FUN_00403870();
        }
      }
LAB_0042402f:
      puStackY_12c = (undefined1 *)0x0;
      pcStackY_130 = "j\x01j\x03j";
      FUN_00403870();
      pcStackY_130 = (char *)0x1;
      puStackY_134 = (undefined1 *)0x3;
      uStackY_138 = 0;
      puStackY_13c = (undefined1 *)0x424045;
      FUN_00403870();
      puStackY_13c = (undefined1 *)0x1;
      uStackY_140 = 0;
      puStackY_144 = (undefined1 *)0x0;
      uStackY_148 = 0x424050;
      FUN_00403870();
      uStackY_148 = 1;
      puStackY_14c = (undefined1 *)0x1;
      uStackY_150 = 0;
      puStackY_154 = (undefined1 *)0x42405b;
      FUN_00403870();
      puStackY_154 = (undefined1 *)0x1;
      uStackY_158 = 5;
      puStackY_15c = (undefined1 *)0x0;
      pcStackY_160 = (char *)0x424066;
      FUN_00403870();
    }
    else {
      if (iVar4 == 0x4c563441) goto LAB_00424102;
      if (iVar4 == 0x4c563442) {
        (**(code **)(*(int *)((int)this + -0xc0) + 0x178))();
        puStackY_12c = (undefined1 *)0x0;
        pcStackY_130 = "j\x01j\x04j";
        FUN_00403870();
        pcStackY_130 = (char *)0x1;
        puStackY_134 = (undefined1 *)0x4;
        uStackY_138 = 0;
        puStackY_13c = (undefined1 *)0x4240aa;
        FUN_00403870();
        puStackY_13c = (undefined1 *)0x1;
        uStackY_140 = 1;
        puStackY_144 = (undefined1 *)0x0;
        uStackY_148 = 0x4240b5;
        FUN_00403870();
        uStackY_148 = 1;
        puStackY_14c = (undefined1 *)0x2;
        uStackY_150 = 0;
        puStackY_154 = (undefined1 *)0x4240c0;
        FUN_00403870();
        (**(code **)(*(int *)((int)this + -0xc0) + 0x174))();
        FUN_0042aeb0();
        if (*(int **)((int)this + 0x958) != (int *)0x0) {
          (**(code **)(**(int **)((int)this + 0x958) + 0x4b4))();
        }
        FUN_0045fee0();
      }
    }
  }
  else if (iVar4 < 0x56523035) {
    if (iVar4 == 0x56523034) {
      (**(code **)(*(int *)((int)this + -0xc0) + 0x178))();
      puStackY_12c = (undefined1 *)0x0;
      pcStackY_130 = (char *)0x424351;
      FUN_00403870();
      (**(code **)(*(int *)((int)this + -0xc0) + 0x174))();
    }
    else if (iVar4 < 0x56523032) {
      if (iVar4 == 0x56523031) {
        (**(code **)(*(int *)((int)this + -0xc0) + 0x178))();
        puStackY_12c = (undefined1 *)0x0;
        pcStackY_130 = (char *)0x4242cc;
        FUN_00403870();
      }
      else if (iVar4 == 0x4c563542) {
        puStackY_12c = (undefined1 *)0x0;
        pcStackY_130 = "j\x01j";
        FUN_00403870();
        pcStackY_130 = (char *)0x1;
        puStackY_134 = (undefined1 *)0x0;
        uStackY_138 = 0;
        puStackY_13c = (undefined1 *)0x42425c;
        FUN_00403870();
        puStackY_13c = (undefined1 *)0x1;
        uStackY_140 = 5;
        puStackY_144 = (undefined1 *)0x0;
        uStackY_148 = 0x424267;
        FUN_00403870();
        uStackY_148 = 1;
        puStackY_14c = (undefined1 *)0x6;
        uStackY_150 = 0;
        puStackY_154 = (undefined1 *)0x424272;
        FUN_00403870();
        puStackY_154 = (undefined1 *)0x1;
        uStackY_158 = 4;
        puStackY_15c = (undefined1 *)0x0;
        pcStackY_160 = (char *)0x42427d;
        FUN_00403870();
        pcStackY_160 = s_RestartLevel_tsk_004ec7e4;
        ppcStackY_164 = (char **)0x424287;
        FUN_0045fee0();
        puStackY_12c = (undefined1 *)0x0;
        pcStackY_130 = (char *)0x424295;
        Menu_ActivateItem_004038c0();
        (**(code **)(*(int *)((int)this + -0xc0) + 0x178))();
      }
      else if (iVar4 == 0x4c563641) {
LAB_004241c6:
        (**(code **)(*(int *)((int)this + -0xc0) + 0x178))();
        (**(code **)(*(int *)((int)this + -0xc0) + 0x174))();
        puStackY_12c = (undefined1 *)0x4241ef;
        FUN_00406f50();
        puStackY_12c = (undefined1 *)0x1;
        pcStackY_130 = (char *)0x7;
        puStackY_134 = (undefined1 *)0x0;
        uStackY_138 = 0x4241fa;
        FUN_00403870();
        uStackY_138 = 1;
        puStackY_13c = (undefined1 *)0x3;
        uStackY_140 = 0;
        puStackY_144 = (undefined1 *)0x424205;
        FUN_00403870();
        puStackY_144 = (undefined1 *)0x1;
        uStackY_148 = 5;
        puStackY_14c = (undefined1 *)0x0;
        uStackY_150 = 0x424210;
        FUN_00403870();
        uStackY_150 = 1;
        puStackY_154 = (undefined1 *)0x4;
        uStackY_158 = 0;
        puStackY_15c = (undefined1 *)0x42421b;
        FUN_00403870();
        puStackY_15c = (undefined1 *)0x1;
        pcStackY_160 = (char *)0x6;
        ppcStackY_164 = (char **)0x0;
        uStackY_168 = 0x424226;
        FUN_00403870();
        puStackY_12c = (undefined1 *)0x0;
        pcStackY_130 = (char *)0x424234;
        FUN_00403870();
        pcStackY_130 = s_RestartLevel_tsk_004ec7e4;
        puStackY_134 = (undefined1 *)0x42423e;
        FUN_0045fee0();
      }
    }
    else if (iVar4 == 0x56523032) {
      (**(code **)(*(int *)((int)this + -0xc0) + 0x178))();
      puStackY_12c = (undefined1 *)0x0;
      pcStackY_130 = (char *)0x42432a;
      FUN_00403870();
    }
    else if (iVar4 == 0x56523033) {
      (**(code **)(*(int *)((int)this + -0xc0) + 0x178))();
      puStackY_12c = (undefined1 *)0x0;
      pcStackY_130 = (char *)0x424301;
      FUN_00403870();
      iVar4 = *(int *)((int)this + -0xc0);
      goto LAB_00424406;
    }
  }
  else {
    switch(iVar4) {
    case 0x56523035:
      puStackY_12c = (undefined1 *)0x0;
      pcStackY_130 = (char *)0x424385;
      FUN_00403870();
      (**(code **)(*(int *)((int)this + -0xc0) + 0x178))();
      break;
    case 0x56523036:
      (**(code **)(*(int *)((int)this + -0xc0) + 0x178))();
      puStackY_12c = (undefined1 *)0x0;
      pcStackY_130 = (char *)0x4243b9;
      FUN_00403870();
      break;
    case 0x56523037:
      (**(code **)(*(int *)((int)this + -0xc0) + 0x178))();
      puStackY_12c = (undefined1 *)0x0;
      pcStackY_130 = (char *)0x4243db;
      FUN_00403870();
      break;
    case 0x56523038:
      (**(code **)(*(int *)((int)this + -0xc0) + 0x178))();
      puStackY_12c = (undefined1 *)0x0;
      pcStackY_130 = (char *)0x4243ff;
      FUN_00403870();
      iVar4 = *(int *)((int)this + -0xc0);
      goto LAB_00424406;
    }
  }
switchD_00424373_default:
  iVar4 = DAT_00509948[0x124];
  if (iVar4 < 0x4c563543) {
    if ((iVar4 < 0x4c563541) &&
       ((iVar4 < 0x4c455636 || ((0x4c455637 < iVar4 && (iVar4 != 0x4c563442)))))) goto LAB_00424463;
  }
  else if (iVar4 != 0x4c563641) goto LAB_00424463;
  if (DAT_004f83d4 < 20.0) {
    DAT_004f83d4 = 20.0;
  }
LAB_00424463:
  (**(code **)(*DAT_00509948 + 0x118))();
  (**(code **)(*(int *)this + 0x410))();
  if (*(int *)((int)this + 0x95c) != 0) {
    puStackY_12c = (undefined1 *)0x0;
    puStackY_134 = auStack_44;
    pcStackY_130 = (char *)0x0;
    uStackY_138 = 0x4244a9;
    puVar5 = (undefined4 *)(**(code **)(*(int *)this + 900))();
    puStackY_144 = (undefined1 *)*puVar5;
    uStackY_140 = puVar5[1];
    puStackY_13c = (undefined1 *)puVar5[2];
    uStackY_138 = puVar5[3];
    uStackY_148 = 0x4244f0;
    (**(code **)(*(int *)(*(int *)((int)this + 0x95c) + 0xc0) + 0x168))();
  }
  piVar17 = *(int **)((int)this + 0x9f0);
  if (piVar17 != (int *)0x0) {
    if (piVar17[0x1d0] == 0) {
      puStackY_12c = (undefined1 *)0x424522;
      (**(code **)(*piVar17 + 0x58))();
    }
    else if (piVar17[0x1c] != 0) {
      piVar17[0x1c] = 1;
      puStackY_12c = (undefined1 *)0x424519;
      (**(code **)(*piVar17 + 0x58))();
    }
  }
  if (*(int **)((int)this + 0x958) != (int *)0x0) {
    puStackY_12c = (undefined1 *)0x0;
    pcStackY_130 = (char *)0x0;
    puStackY_134 = (undefined1 *)0x0;
    uStackY_138 = 0x42453f;
    (**(code **)(**(int **)((int)this + 0x958) + 0x4b8))();
  }
  ExceptionList = pvStack_34;
  return;
}


```

## InitJimmyRuntimeHandles_00425db0 @ 00425db0

```c

void __thiscall InitJimmyRuntimeHandles_00425db0(void *this)

{
  int iVar1;
  undefined4 uVar2;
  undefined4 uStack_28;
  int iStack_24;
  undefined4 uStack_20;
  undefined1 uStack_5;
  undefined1 *puStack_4;

  C3DAnimated::vfunc_01_273(this);
  uStack_20 = 0x425dce;
  iVar1 = __strcmpi((char *)((int)this + 0x98c),&DAT_004eca6c);
  if (iVar1 != 0) {
    DAT_00696004 = FUN_0046a910();
    if (DAT_00696004 != 0) {
      uStack_20 = 0xffffffff;
      uStack_28 = 0x425df5;
      iStack_24 = DAT_00696004;
      uVar2 = FUN_0047d710();
      *(undefined4 *)((int)this + 0x978) = uVar2;
    }
  }
  if (*(int *)((int)this + 0x1d48) != -1) {
    uStack_20 = 0xffffffff;
    iStack_24 = 0x425e10;
    uVar2 = FUN_00458980();
    *(undefined4 *)((int)this + 0x1d48) = uVar2;
  }
  if (*(int *)((int)this + 0x988) != -1) {
    puStack_4 = (undefined1 *)&uStack_28;
    FUN_0040acc0(s_not_available_004ef410,&uStack_5);
    uVar2 = FUN_00458b40(0xffffffff);
    *(undefined4 *)((int)this + 0x988) = uVar2;
  }
  if (*(int *)((int)this + 0x1d44) != -1) {
    uStack_20 = 0xffffffff;
    iStack_24 = 0x425e5c;
    uVar2 = FUN_00458980();
    *(undefined4 *)((int)this + 0x1d44) = uVar2;
  }
  if (*(int *)((int)this + 0x980) != -1) {
    uStack_20 = 0xffffffff;
    iStack_24 = 0x425e77;
    uVar2 = FUN_00458980();
    *(undefined4 *)((int)this + 0x980) = uVar2;
  }
  if (*(int *)((int)this + 0x97c) != -1) {
    uStack_20 = 0xffffffff;
    iStack_24 = 0x425e92;
    uVar2 = FUN_00458980();
    *(undefined4 *)((int)this + 0x97c) = uVar2;
  }
  if (*(int *)((int)this + 0x984) != -1) {
    uStack_20 = 0xffffffff;
    iStack_24 = 0x425eb8;
    uVar2 = FUN_00458980();
    *(undefined4 *)((int)this + 0x984) = uVar2;
  }
  iVar1 = *(int *)((int)this + 0x9f0);
  if ((iVar1 != 0) && (*(int *)(iVar1 + 0xf88) != -1)) {
    *(undefined4 *)(iVar1 + 0xf88) = 0xffffffff;
    (**(code **)(**(int **)((int)this + 0x9f0) + 0x128))();
  }
  return;
}


```

## JimmyEnterActionMenuLock_00425ef0 @ 00425ef0

```c

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall JimmyEnterActionMenuLock_00425ef0(void *this)

{
  CGameObject *this_00;
  int unaff_retaddr;

  if ((DAT_004ec494 != '\0') && (DAT_004f8181 == '\0')) {
    (**(code **)(*(int *)((int)this + 0xc0) + 0x448))(1);
    CGameObject::vfunc_00_013(this_00);
    (**(code **)(**(int **)((int)this + 0xa18) + 0x4c4))();
    (**(code **)(*(int *)this + 0x104))();
    *(undefined4 *)((int)this + 0x1e34) = 0;
    (**(code **)(**(int **)((int)this + 0xa18) + 0x488))();
    (**(code **)(**(int **)((int)this + 0xa18) + 0x45c))();
    (**(code **)(**(int **)((int)this + 0xa18) + 0x4ac))(unaff_retaddr);
    (**(code **)(*DAT_00509948 + 0x168))(1);
    (**(code **)(*(int *)this + 0x148))();
    DAT_004f8181 = '\x01';
    (**(code **)(**(int **)((int)this + 0xa18) + 0x4b4))(0);
    *(undefined1 *)((int)this + 0x55f) = 1;
    if (unaff_retaddr != 2) {
      ShowCursor(1);
    }
    *(undefined1 *)(*(int *)((int)this + 0xa18) + 0x4c0) = 1;
    *(undefined4 *)(*(int *)((int)this + 0xa18) + 0x4c4) = 0;
    (**(code **)(*(int *)((int)this + 0xc0) + 0x214))(0);
    (**(code **)(**(int **)((int)this + 0xa18) + 0x460))();
    FUN_0047d850();
    DAT_004f8434 = 0;
    _DAT_004f8188 = 0x41a00000;
    (**(code **)(**(int **)((int)this + 0xa18) + 0x4ec))();
  }
  return;
}


```

## SaveLevelActionSnapshot_0042af00 @ 0042af00

```c

void __thiscall SaveLevelActionSnapshot_0042af00(void *this)

{
  undefined4 *puVar1;
  undefined1 auStack_10 [16];

  *(undefined4 *)((int)this + 0x1e48) = *(undefined4 *)(DAT_00509948 + 0x490);
  puVar1 = (undefined4 *)(**(code **)(*(int *)((int)this + 0xc0) + 0x310))(auStack_10);
  *(undefined4 *)((int)this + 0x1e38) = *puVar1;
  *(undefined4 *)((int)this + 0x1e3c) = puVar1[1];
  *(undefined4 *)((int)this + 0x1e40) = puVar1[2];
  *(undefined4 *)((int)this + 0x1e44) = puVar1[3];
  return;
}


```

## SaveLevelActionSnapshotAndTrigger_0042af50 @ 0042af50

```c

void __thiscall SaveLevelActionSnapshotAndTrigger_0042af50(void *this)

{
  undefined4 *puVar1;
  undefined1 auStack_10 [16];

  *(undefined4 *)((int)this + 0x1e48) = *(undefined4 *)(DAT_00509948 + 0x490);
  puVar1 = (undefined4 *)(**(code **)(*(int *)((int)this + 0xc0) + 0x310))(auStack_10);
  *(undefined4 *)((int)this + 0x1e38) = *puVar1;
  *(undefined4 *)((int)this + 0x1e3c) = puVar1[1];
  *(undefined4 *)((int)this + 0x1e40) = puVar1[2];
  *(undefined4 *)((int)this + 0x1e44) = puVar1[3];
  (**(code **)(**(int **)((int)this + 0xa18) + 0x4b8))(0x13,1,200,0x12);
  return;
}


```

## JimmyActionHelper93_00428d50 @ 00428d50

```c

void __thiscall JimmyActionHelper93_00428d50(void *this,short param_2)

{
  char *pcVar1;
  int *piVar2;
  char cVar3;
  short sVar4;
  undefined4 uVar5;
  int iVar6;
  undefined4 *puVar7;
  CGameObject *extraout_ECX;
  CGameObject *extraout_ECX_00;
  CGameObject *this_00;
  CGameObject *extraout_ECX_01;
  CGameObject *extraout_ECX_02;
  CGameObject *this_01;
  CGameObject *extraout_ECX_03;
  undefined4 extraout_ECX_04;
  CGameObject *this_02;
  CGameObject *extraout_ECX_05;
  CGameObject *this_03;
  CGameObject *extraout_ECX_06;
  CGameObject *extraout_ECX_07;
  CGameObject *this_04;
  uint uVar8;
  uint uVar9;
  CGameObject *this_05;
  CGameObject *extraout_ECX_08;
  CGameObject *extraout_ECX_09;
  CGameObject *this_06;
  CGameObject *extraout_ECX_10;
  CGameObject *this_07;
  CGameObject *extraout_ECX_11;
  CGameObject *this_08;
  CGameObject *extraout_ECX_12;
  CGameObject *extraout_ECX_13;
  CGameObject *this_09;
  CGameObject *extraout_ECX_14;
  CGameObject *extraout_ECX_15;
  CGameObject *this_10;
  CGameObject *extraout_ECX_16;
  int iVar10;
  char *pcVar11;
  char *pcVar12;
  char *pcStack_130;
  char *pcStack_12c;
  char *pcStack_128;
  char acStack_108 [12];
  undefined1 auStack_fc [20];
  undefined1 auStack_e8 [8];
  undefined1 auStack_e0 [4];
  undefined1 auStack_dc [12];
  undefined1 auStack_d0 [28];
  undefined1 auStack_b4 [16];
  char acStack_a4 [12];
  undefined1 auStack_98 [24];
  undefined1 auStack_80 [8];
  undefined1 auStack_78 [24];
  char acStack_60 [96];

  iVar10 = (int)param_2;
  pcStack_12c = s_CAll_in_AMI__d_004ef6c8;
  pcStack_130 = (char *)0x428d72;
  pcStack_128 = (char *)iVar10;
  CGameObject::vfunc_00_013(this);
  if (*(char *)((int)this + 0x1e59) == '\0') {
    this_03 = *(CGameObject **)((int)this + 0xa18);
    if ((this_03 != (CGameObject *)0x0) && (param_2 < 9)) {
      pcStack_12c = (char *)0x428d9e;
      pcStack_128 = (char *)iVar10;
      (**(code **)(this_03->vftable + 0x4b0))();
      this_03 = extraout_ECX;
    }
    if ((((DAT_005099e4 != 0) && (*(int *)((int)this + 0xa18) != 0)) && (DAT_004f8181 == '\0')) &&
       ((param_2 != 3 && (param_2 != -1)))) {
      *(undefined4 *)((int)this + 0x1e34) = 0x41200000;
    }
    switch(iVar10) {
    case 0:
      if (*(int *)((int)this + 0xa14) != 0) {
        pcStack_128 = (char *)0x9;
        pcStack_12c = (char *)0x0;
        pcStack_130 = (char *)0x428f75;
        sVar4 = FUN_00403950();
        if (sVar4 == 2) {
          pcStack_128 = (char *)0x1;
          pcStack_12c = (char *)0x9;
          pcStack_130 = (char *)0x0;
          Menu_ActivateItem_004038c0();
          pcStack_128 = (char *)0xffffffff;
          pcStack_12c = (char *)0x0;
          pcStack_130 = (char *)0x1;
          (**(code **)(**(int **)((int)this + 0xa18) + 0x4b8))(0x9b);
          CGameObject::vfunc_00_013(this_02);
          (**(code **)(*(int *)this + 0x168))
                    (s_vr01_gam_004ef650,s_PHONEBOOTH_004ec788,(int)this + 0x955,(int)this + 0x724);
          this_03 = extraout_ECX_05;
        }
        else {
          if ((DAT_004f83d4 <= 0.0) && (*(char *)((int)this + 0x9f8) == '\0')) {
            DAT_004f0588 = 0xffff;
            return;
          }
          iVar10 = *(int *)((int)this + 0xa14);
          this_03 = (CGameObject *)
                    CONCAT31((int3)((uint)extraout_ECX_04 >> 8),*(char *)(iVar10 + 0x614));
          if (*(char *)(iVar10 + 0x614) == '\0') {
            if (*(char *)(iVar10 + 0x616) != '\0') {
              return;
            }
            if (*(char *)(iVar10 + 0x615) != '\0') {
              return;
            }
            pcStack_128 = (char *)0x43200000;
            pcStack_12c = (char *)0x429056;
            (**(code **)(*(int *)((int)this + 0xc0) + 0x110))();
            pcStack_12c = (char *)0x3ed47ae1;
            pcStack_130 = (char *)0x429065;
            (**(code **)(*(int *)((int)this + 0xc0) + 0x100))();
            DAT_004f0588 = 0;
            *(undefined2 *)((int)this + 0x7c4) = 2;
            *(undefined4 *)((int)this + 0x6c4) = 0x44a8c000;
            pcStack_130 = (char *)0x42908a;
            FUN_00410ca0();
            if (*(int *)((int)this + 0xa40) == -1) {
              pcStack_130 = (char *)0x1;
              uVar5 = FUN_00458980(0xffffffff,2);
              *(undefined4 *)((int)this + 0xa40) = uVar5;
            }
            pcStack_130 = (char *)0x4290b2;
            (**(code **)(*(int *)this + 0x148))();
            this_03 = extraout_ECX_06;
          }
        }
      }
      break;
    case 1:
      pcStack_128 = s_Activating_Rocket_004ef5c4;
      pcStack_12c = (char *)0x429457;
      CGameObject::vfunc_00_013(this_03);
      if (*(int *)((int)this + 0xab0) != 0) {
        pcStack_128 = s_ACT_2_Rocket_004ef5b4;
        pcStack_12c = (char *)0x429477;
        CGameObject::vfunc_00_013(this_07);
        pcStack_128 = (char *)0x32;
        pcStack_12c = (char *)0x96;
        pcStack_130 = (char *)0x42948c;
        FUN_0043f870();
        piVar2 = *(int **)((int)this + 0xab0);
        if (piVar2[0x1c] != 0) {
          pcStack_128 = (char *)0x0;
          piVar2[0x1c] = 1;
          pcStack_12c = (char *)0x4294a3;
          (**(code **)(*piVar2 + 0x58))();
        }
        pcStack_128 = (char *)0x1;
        pcStack_12c = (char *)0x4294b8;
        (**(code **)(*(int *)(*(int *)((int)this + 0xab0) + 0xc0) + 0xd0))();
        pcStack_12c = (char *)0x1;
        pcStack_130 = (char *)0x4294cd;
        (**(code **)(*(int *)(*(int *)((int)this + 0xab0) + 0xc0) + 0x214))();
        piVar2 = (int *)((int)this + 0xc0);
        pcStack_130 = acStack_108;
        iVar10 = *(int *)(*(int *)((int)this + 0xab0) + 0xc0);
        iVar6 = (**(code **)(*(int *)((int)this + 0xc0) + 0x310))();
        iVar6 = (**(code **)(*piVar2 + 0x310))(auStack_fc,*(undefined4 *)(iVar6 + 8));
        puVar7 = (undefined4 *)(**(code **)(*piVar2 + 0x310))(auStack_e0,*(undefined4 *)(iVar6 + 4))
        ;
        (**(code **)(iVar10 + 0x318))(*puVar7);
        iVar10 = *(int *)(*(int *)((int)this + 0xab0) + 0xc0);
        iVar6 = (**(code **)(*piVar2 + 0x328))(auStack_d0);
        iVar6 = (**(code **)(*piVar2 + 0x328))(auStack_b4,*(undefined4 *)(iVar6 + 8));
        puVar7 = (undefined4 *)(**(code **)(*piVar2 + 0x328))(auStack_98,*(undefined4 *)(iVar6 + 4))
        ;
        (**(code **)(iVar10 + 0x330))(*puVar7);
        if (*(int *)((int)this + 0xab0) == 0) {
          iVar10 = 0;
        }
        else {
          iVar10 = *(int *)((int)this + 0xab0) + 0xc0;
        }
        (**(code **)(*DAT_00509948 + 0x118))(iVar10);
        *(void **)(*(int *)((int)this + 0xab0) + 0x740) = this;
        (**(code **)(*piVar2 + 0x1e4))(1);
        if (*(int *)((int)this + 0xa1c) != 0) {
          (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0x1e4))(1);
          (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0x214))(0);
          *(undefined1 *)(*(int *)((int)this + 0xa1c) + 0x9a4) = 1;
          (**(code **)(**(int **)((int)this + 0xa1c) + 0xe0))(&PTR_DAT_004eca5c,1);
          (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0x224))(0);
          if (*(int *)((int)this + 0xa30) != 0) {
            (**(code **)(*(int *)(*(int *)((int)this + 0xa30) + 0xc0) + 0x1e4))(1);
            (**(code **)(*(int *)(*(int *)((int)this + 0xa30) + 0xc0) + 0x224))(0);
          }
        }
        (**(code **)(*(int *)this + 0xe0))(s_DRIVE_004ef344,1);
        (**(code **)(*(int *)this + 0x148))();
        DAT_004f0588 = 1;
      }
      pcStack_128 = (char *)0x9;
      pcStack_12c = (char *)0x0;
      pcStack_130 = (char *)0x42968f;
      sVar4 = FUN_00403950();
      this_03 = extraout_ECX_11;
      if (sVar4 == 2) {
        pcStack_128 = (char *)0x1;
        pcStack_12c = (char *)0x9;
        pcStack_130 = (char *)0x0;
        Menu_ActivateItem_004038c0();
        pcStack_128 = (char *)0xffffffff;
        pcStack_12c = (char *)0x0;
        pcStack_130 = (char *)0x1;
        (**(code **)(**(int **)((int)this + 0xa18) + 0x4b8))(0x9b);
        CGameObject::vfunc_00_013(this_08);
        (**(code **)(*(int *)this + 0x168))
                  (s_vr02_gam_004ef58c,s_PHONEBOOTH_004ec788,(int)this + 0x955,(int)this + 0x724);
        this_03 = extraout_ECX_12;
      }
      break;
    case 2:
      if ((0.0 < DAT_004f83d4) || (*(char *)((int)this + 0x9f8) != '\0')) {
        if (*(int **)((int)this + 0xa1c) != (int *)0x0) {
          pcStack_128 = (char *)0x1;
          pcStack_12c = (char *)0x428e8a;
          (**(code **)(**(int **)((int)this + 0xa1c) + 0x58))();
          pcStack_12c = (char *)0x0;
          pcStack_130 = (char *)0x428e9f;
          (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0xd0))();
          pcStack_130 = (char *)0x0;
          (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0x214))();
        }
        DAT_004f0588 = 2;
        *(undefined4 *)((int)this + 0x1e1c) = 0;
        *(undefined1 *)((int)this + 0x55d) = 1;
        *(undefined4 *)((int)this + 0x1e20) = 0;
        if (*(int **)((int)this + 0xa18) != (int *)0x0) {
          pcStack_128 = (char *)0x428ee2;
          (**(code **)(**(int **)((int)this + 0xa18) + 0x48c))();
          pcStack_128 = (char *)0x428ef0;
          (**(code **)(**(int **)((int)this + 0xa18) + 0x484))();
        }
      }
      else {
        DAT_004f0588 = 0xffff;
      }
      pcStack_128 = (char *)0x9;
      pcStack_12c = (char *)0x0;
      pcStack_130 = (char *)0x428ef8;
      sVar4 = FUN_00403950();
      this_03 = extraout_ECX_02;
      if (sVar4 == 2) {
        pcStack_128 = (char *)0x1;
        pcStack_12c = (char *)0x9;
        pcStack_130 = (char *)0x0;
        Menu_ActivateItem_004038c0();
        pcStack_128 = (char *)0xffffffff;
        pcStack_12c = (char *)0x0;
        pcStack_130 = (char *)0x1;
        (**(code **)(**(int **)((int)this + 0xa18) + 0x4b8))(0x9b);
        CGameObject::vfunc_00_013(this_01);
        (**(code **)(*(int *)this + 0x168))
                  (s_vr03_gam_004ef678,s_PHONEBOOTH_004ec788,(int)this + 0x955,(int)this + 0x724);
        this_03 = extraout_ECX_03;
      }
      break;
    case 4:
      if (*(int *)((int)this + 0xa1c) != 0) {
        pcStack_128 = (char *)0x1;
        pcStack_12c = (char *)0x4290d2;
        (**(code **)(**(int **)(*(int *)((int)this + 0xa1c) + 0x994) + 0x58))();
        pcStack_12c = (char *)0x4290ea;
        cVar3 = (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0xcc))();
        if (cVar3 != '\0') {
          DAT_004f0588 = 4;
          iVar10 = *(int *)(*(int *)((int)this + 0xa30) + 0xc0);
          piVar2 = (int *)(*(int *)((int)this + 0xa1c) + 0xc0);
          pcStack_128 = auStack_80;
          pcStack_12c = (char *)0x42914a;
          iVar6 = (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0x310))();
          pcStack_12c = *(char **)(iVar6 + 8);
          pcStack_130 = acStack_a4;
          iVar6 = (**(code **)(*piVar2 + 0x310))();
          puVar7 = (undefined4 *)
                   (**(code **)(*piVar2 + 0x310))(auStack_e8,*(undefined4 *)(iVar6 + 4));
          (**(code **)(iVar10 + 0x16c))(*puVar7);
          pcStack_12c = (char *)(*(int *)((int)this + 0xa1c) + 0xc0);
          iVar10 = *(int *)(*(int *)((int)this + 0xa30) + 0xc0);
          piVar2 = (int *)(*(int *)((int)this + 0xa1c) + 0xc0);
          iVar6 = (**(code **)(*piVar2 + 0x328))(auStack_78);
          iVar6 = (**(code **)(*piVar2 + 0x328))(auStack_dc,*(undefined4 *)(iVar6 + 8));
          puVar7 = (undefined4 *)
                   (**(code **)(*piVar2 + 0x328))(&pcStack_130,*(undefined4 *)(iVar6 + 4));
          (**(code **)(iVar10 + 0x174))(*puVar7);
          (**(code **)(**(int **)((int)this + 0xa30) + 0x174))();
          (**(code **)(*(int *)(*(int *)((int)this + 0xa30) + 0xc0) + 0xd0))(1);
          (**(code **)(*(int *)(*(int *)((int)this + 0xa30) + 0xc0) + 0x214))(1);
          (**(code **)(**(int **)((int)this + 0xa1c) + 0x58))(1);
          (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0xd0))(0);
          (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0x214))(0);
          DAT_004f0588 = 4;
        }
      }
      pcStack_128 = (char *)0x9;
      pcStack_12c = (char *)0x0;
      pcStack_130 = (char *)0x42929f;
      sVar4 = FUN_00403950();
      this_03 = extraout_ECX_07;
      if (sVar4 == 2) {
        pcStack_128 = (char *)0x1;
        pcStack_12c = (char *)0x9;
        pcStack_130 = (char *)0x0;
        Menu_ActivateItem_004038c0();
        pcStack_128 = (char *)0xffffffff;
        pcStack_12c = (char *)0x0;
        pcStack_130 = (char *)0x1;
        (**(code **)(**(int **)((int)this + 0xa18) + 0x4b8))(0x9b);
        pcVar1 = (char *)((int)this + 0x724);
        CGameObject::vfunc_00_013(this_04);
        uVar8 = 0xffffffff;
        pcVar11 = (char *)((int)this + 0x955);
        do {
          pcVar12 = pcVar11;
          if (uVar8 == 0) break;
          uVar8 = uVar8 - 1;
          pcVar12 = pcVar11 + 1;
          cVar3 = *pcVar11;
          pcVar11 = pcVar12;
        } while (cVar3 != '\0');
        uVar8 = ~uVar8;
        pcVar11 = pcVar12 + -uVar8;
        pcVar12 = acStack_60;
        for (uVar9 = uVar8 >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
          *(undefined4 *)pcVar12 = *(undefined4 *)pcVar11;
          pcVar11 = pcVar11 + 4;
          pcVar12 = pcVar12 + 4;
        }
        for (uVar8 = uVar8 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
          *pcVar12 = *pcVar11;
          pcVar11 = pcVar11 + 1;
          pcVar12 = pcVar12 + 1;
        }
        uVar8 = 0xffffffff;
        pcVar11 = pcVar1;
        do {
          pcVar12 = pcVar11;
          if (uVar8 == 0) break;
          uVar8 = uVar8 - 1;
          pcVar12 = pcVar11 + 1;
          cVar3 = *pcVar11;
          pcVar11 = pcVar12;
        } while (cVar3 != '\0');
        uVar8 = ~uVar8;
        iVar10 = *(int *)this;
        pcVar11 = pcVar12 + -uVar8;
        pcVar12 = acStack_60;
        for (uVar9 = uVar8 >> 2; uVar9 != 0; uVar9 = uVar9 - 1) {
          *(undefined4 *)pcVar12 = *(undefined4 *)pcVar11;
          pcVar11 = pcVar11 + 4;
          pcVar12 = pcVar12 + 4;
        }
        for (uVar8 = uVar8 & 3; uVar8 != 0; uVar8 = uVar8 - 1) {
          *pcVar12 = *pcVar11;
          pcVar11 = pcVar11 + 1;
          pcVar12 = pcVar12 + 1;
        }
        (**(code **)(iVar10 + 0x168))
                  (s_vr05_gam_004ef628,s_PHONEBOOTH_004ec788,(int)this + 0x955,pcVar1);
        pcStack_128 = (char *)((int)this + 0x8f0);
        pcStack_12c = (char *)((int)this + 0x88c);
        pcStack_130 = pcVar1;
        CGameObject::vfunc_00_013(this_05);
        this_03 = extraout_ECX_08;
      }
      break;
    case 5:
      DAT_004f0588 = 5;
      if (*(int *)((int)this + 0xa1c) != 0) {
        pcStack_128 = (char *)0x42939a;
        (**(code **)(*(int *)((int)this + 0xc0) + 0x410))();
        if (*(int *)((int)this + 0xa1c) == 0) {
          pcStack_128 = (char *)0x0;
        }
        else {
          pcStack_128 = (char *)(*(int *)((int)this + 0xa1c) + 0xc0);
        }
        pcStack_12c = (char *)0x4293bc;
        (**(code **)(*DAT_00509948 + 0x118))();
        pcStack_12c = (char *)0x1;
        pcStack_130 = (char *)0x4293cc;
        (**(code **)(**(int **)((int)this + 0xa1c) + 0x188))();
        pcStack_130 = (char *)0x1;
        (**(code **)(**(int **)((int)this + 0xa18) + 0x4bc))();
      }
      pcStack_128 = (char *)0x9;
      pcStack_12c = (char *)0x0;
      pcStack_130 = (char *)0x4293e4;
      sVar4 = FUN_00403950();
      this_03 = extraout_ECX_09;
      if (sVar4 == 2) {
        pcStack_128 = (char *)0x1;
        pcStack_12c = (char *)0x9;
        pcStack_130 = (char *)0x0;
        Menu_ActivateItem_004038c0();
        pcStack_128 = (char *)0xffffffff;
        pcStack_12c = (char *)0x0;
        pcStack_130 = (char *)0x1;
        (**(code **)(**(int **)((int)this + 0xa18) + 0x4b8))(0x9b);
        CGameObject::vfunc_00_013(this_06);
        (**(code **)(*(int *)this + 0x168))
                  (s_vr06_gam_004ef5d8,s_PHONEBOOTH_004ec788,(int)this + 0x955,(int)this + 0x724);
        this_03 = extraout_ECX_10;
      }
      break;
    case 6:
      DAT_004f0588 = 6;
      *(float *)((int)this + 0x7cc) = *(float *)((int)this + 0x7cc) + 30.0;
      piVar2 = *(int **)(*(int *)((int)this + 0xa18) + 0x4e4);
      if (piVar2[0x1c] != 0) {
        pcStack_128 = (char *)0x0;
        piVar2[0x1c] = 1;
        pcStack_12c = (char *)0x429731;
        (**(code **)(*piVar2 + 0x58))();
      }
      if (*(int **)((int)this + 0xa1c) != (int *)0x0) {
        pcStack_128 = (char *)0x1;
        pcStack_12c = (char *)0x429742;
        (**(code **)(**(int **)((int)this + 0xa1c) + 0x58))();
        pcStack_12c = (char *)0x0;
        pcStack_130 = (char *)0x429757;
        (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0xd0))();
        pcStack_130 = (char *)0x0;
        (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0x214))();
      }
      pcStack_128 = (char *)0x9;
      pcStack_12c = (char *)0x0;
      pcStack_130 = (char *)0x429774;
      sVar4 = FUN_00403950();
      this_03 = extraout_ECX_13;
      if (sVar4 == 2) {
        pcStack_128 = (char *)0x1;
        pcStack_12c = (char *)0x9;
        pcStack_130 = (char *)0x0;
        Menu_ActivateItem_004038c0();
        pcStack_128 = (char *)0xffffffff;
        pcStack_12c = (char *)0x0;
        pcStack_130 = (char *)0x1;
        (**(code **)(**(int **)((int)this + 0xa18) + 0x4b8))(0x9b);
        CGameObject::vfunc_00_013(this_09);
        (**(code **)(*(int *)this + 0x168))
                  (s_vr07_gam_004ef564,s_PHONEBOOTH_004ec788,(int)this + 0x955,(int)this + 0x724);
        this_03 = extraout_ECX_14;
      }
      break;
    case 7:
      if (*(int **)((int)this + 0xa1c) != (int *)0x0) {
        pcStack_128 = (char *)0x1;
        pcStack_12c = (char *)0x4297ee;
        (**(code **)(**(int **)((int)this + 0xa1c) + 0x58))();
        pcStack_12c = (char *)0x0;
        pcStack_130 = (char *)0x429803;
        (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0xd0))();
        pcStack_130 = (char *)0x0;
        (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0x214))();
      }
      pcStack_128 = (char *)0x9;
      *(undefined4 *)((int)this + 0x7c8) = 0;
      *(undefined4 *)((int)this + 0x7cc) = 0x42480000;
      *(undefined4 *)((int)this + 2000) = 0xc3af0000;
      *(undefined4 *)((int)this + 0x7d8) = 0;
      *(undefined4 *)((int)this + 0x7dc) = 0x43160000;
      *(undefined4 *)((int)this + 0x7e0) = 0x42a00000;
      pcStack_12c = (char *)0x0;
      DAT_004f0588 = 7;
      pcStack_130 = (char *)0x42985d;
      sVar4 = FUN_00403950();
      this_03 = extraout_ECX_15;
      if (sVar4 == 2) {
        pcStack_128 = (char *)0x1;
        pcStack_12c = (char *)0x9;
        pcStack_130 = (char *)0x0;
        Menu_ActivateItem_004038c0();
        pcStack_128 = (char *)0xffffffff;
        pcStack_12c = (char *)0x0;
        pcStack_130 = (char *)0x1;
        (**(code **)(**(int **)((int)this + 0xa18) + 0x4b8))(0x9b);
        CGameObject::vfunc_00_013(this_10);
        (**(code **)(*(int *)this + 0x168))
                  (s_vr08_gam_004ef53c,s_PHONEBOOTH_004ec788,(int)this + 0x955,(int)this + 0x724);
        this_03 = extraout_ECX_16;
      }
      break;
    case 8:
      *(undefined1 *)((int)this + 0x1e24) = 1;
      *(undefined1 *)((int)this + 0x1e25) = 1;
      break;
    case -1:
    case 3:
      pcStack_128 = (char *)0x9;
      pcStack_12c = (char *)0x0;
      pcStack_130 = (char *)0x428de8;
      sVar4 = FUN_00403950();
      this_03 = extraout_ECX_00;
      if (sVar4 == 2) {
        pcStack_128 = (char *)0x1;
        pcStack_12c = (char *)0x9;
        pcStack_130 = (char *)0x0;
        Menu_ActivateItem_004038c0();
        pcStack_128 = (char *)0xffffffff;
        pcStack_12c = (char *)0x0;
        pcStack_130 = (char *)0x1;
        (**(code **)(**(int **)((int)this + 0xa18) + 0x4b8))(0x9b);
        CGameObject::vfunc_00_013(this_00);
        (**(code **)(*(int *)this + 0x168))
                  (s_vr04_gam_004ef6a0,s_PHONEBOOTH_004ec788,(int)this + 0x955,(int)this + 0x724);
        this_03 = extraout_ECX_01;
      }
    }
    pcStack_128 = s_Exiting_AMI_004ef530;
    pcStack_12c = (char *)0x4298d7;
    CGameObject::vfunc_00_013(this_03);
  }
  return;
}


```

## JimmyActionHelper94_00427ff0 @ 00427ff0

```c

void __thiscall JimmyActionHelper94_00427ff0(void *this)

{
  int *piVar1;
  char cVar2;
  int iVar3;
  undefined4 *puVar4;
  int iVar5;
  int *piVar6;
  CGameObject *extraout_ECX;
  CGameObject *this_00;
  char *pcStack_a8;
  undefined1 auStack_90 [20];
  undefined1 auStack_7c [20];
  undefined1 auStack_68 [8];
  undefined1 auStack_60 [28];
  undefined1 auStack_44 [36];
  char acStack_20 [32];

  this_00 = (CGameObject *)0x0;
  if (*(int **)((int)this + 0xa18) != (int *)0x0) {
    pcStack_a8 = (char *)0x3;
    (**(code **)(**(int **)((int)this + 0xa18) + 0x4b0))();
    (**(code **)(**(int **)((int)this + 0xa18) + 0x4c4))();
    this_00 = extraout_ECX;
  }
  switch(DAT_004f0588) {
  case 0:
    if (*(int *)((int)this + 0xa14) != 0) {
      *(undefined4 *)((int)this + 0xa20) = 0x3f000000;
      *(undefined2 *)((int)this + 0x7c4) = 0;
      *(undefined4 *)((int)this + 0x6c4) = 0x44228000;
      pcStack_a8 = (char *)0x428065;
      FUN_00410d00();
      *(undefined1 *)(*(int *)((int)this + 0xa14) + 0x615) = 0;
      if (*(int *)((int)this + 0xa40) != -1) {
        pcStack_a8 = (char *)0x0;
        FUN_00458a00(*(int *)((int)this + 0xa40));
        *(undefined4 *)((int)this + 0xa40) = 0xffffffff;
      }
      pcStack_a8 = (char *)0x42700000;
      (**(code **)(*(int *)((int)this + 0xc0) + 0x110))();
      (**(code **)(*(int *)((int)this + 0xc0) + 0x100))(0x3fa1eb85);
      DAT_004f0588 = 0xffff;
      return;
    }
    break;
  case 1:
    pcStack_a8 = s_Deactivating_Rocket_004ef494;
    CGameObject::vfunc_00_013(this_00);
    pcStack_a8 = (char *)0x1;
    (**(code **)(**(int **)((int)this + 0xab0) + 0x58))();
    (**(code **)(*(int *)(*(int *)((int)this + 0xab0) + 0xc0) + 0xd0))(0);
    FUN_0043f870(0x46,0x1e);
    (**(code **)(*(int *)(*(int *)((int)this + 0xab0) + 0xc0) + 0x214))(0);
    (**(code **)(*(int *)(*(int *)((int)this + 0xab0) + 0xc0) + 0x1e4))(1);
    (**(code **)(**(int **)((int)this + 0xab0) + 0x128))(0);
    iVar5 = *(int *)(*(int *)((int)this + 0xab0) + 0xf7c);
    if (iVar5 == 0) {
      piVar6 = (int *)0x0;
    }
    else {
      piVar6 = (int *)(iVar5 + -0xc0);
    }
    (**(code **)(*piVar6 + 0x58))(1);
    if (this == (void *)0x0) {
      iVar5 = 0;
    }
    else {
      iVar5 = (int)this + 0xc0;
    }
    (**(code **)(*DAT_00509948 + 0x118))(iVar5);
    piVar6 = (int *)((int)this + 0xc0);
    (**(code **)(*(int *)((int)this + 0xc0) + 0x1e4))(0);
    if (*(int *)((int)this + 0xa1c) != 0) {
      (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0x1e4))(0);
      *(undefined1 *)(*(int *)((int)this + 0xa1c) + 0x9a4) = 0;
      (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0x224))(1);
      (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0x214))(1);
      if ((DAT_00509a13 == '\0') &&
         (piVar1 = *(int **)(*(int *)((int)this + 0xa1c) + 0x994), piVar1[0x1c] != 0)) {
        piVar1[0x1c] = 1;
        (**(code **)(*piVar1 + 0x58))(0);
      }
      if (*(int *)((int)this + 0xa30) != 0) {
        (**(code **)(*(int *)(*(int *)((int)this + 0xa30) + 0xc0) + 0x1e4))(0);
      }
    }
    if ((*(int *)((int)this + 0xab0) != 0) && (*(int *)(*(int *)((int)this + 0xab0) + 0x740) != 0))
    {
      (**(code **)(*DAT_00509948 + 0x118))(-(uint)(this != (void *)0x0) & (uint)piVar6);
      (**(code **)(*(int *)this + 0xe0))(&DAT_004eca64,1);
      *(undefined4 *)(*(int *)((int)this + 0xab0) + 0x740) = 0;
      *(undefined1 *)(*(int *)((int)this + 0xa1c) + 0x9a4) = 0;
    }
    (**(code **)(*piVar6 + 0x278))(0,0,0);
    (**(code **)(*piVar6 + 0x2c4))(0,0,0);
    (**(code **)(*(int *)this + 0xe0))(&DAT_004ef3e8,1);
    *(undefined2 *)((int)this + 0x7c4) = 1;
    if (*(int **)((int)this + 0xa18) != (int *)0x0) {
      (**(code **)(**(int **)((int)this + 0xa18) + 0x480))();
      DAT_004f0588 = 0xffff;
      return;
    }
    break;
  case 2:
    pcStack_a8 = (char *)0x7;
    *(undefined4 *)((int)this + 0x1e1c) = 0x40e00000;
    (**(code **)(*(int *)this + 0xfc))(0,6);
    *(undefined1 *)((int)this + 0x55d) = 1;
    *(undefined4 *)((int)this + 0x1e20) = 0;
    if (*(int **)((int)this + 0xa18) != (int *)0x0) {
      (**(code **)(**(int **)((int)this + 0xa18) + 0x488))();
    }
    piVar6 = *(int **)((int)this + 0xa1c);
    if ((piVar6 != (int *)0x0) && ((char)piVar6[0x27a] == '\0')) {
      if (piVar6[0x1c] != 0) {
        piVar6[0x1c] = 1;
        (**(code **)(*piVar6 + 0x58))(0);
      }
      (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0xd0))(1);
      (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0x214))(1);
    }
    (**(code **)(*(int *)this + 0x114))(0);
    DAT_004f0588 = 0xffff;
    return;
  case 4:
    if (*(int *)((int)this + 0xa1c) != 0) {
      if ((DAT_00509a13 == '\0') &&
         (piVar6 = *(int **)(*(int *)((int)this + 0xa1c) + 0x994), piVar6[0x1c] != 0)) {
        pcStack_a8 = (char *)0x0;
        piVar6[0x1c] = 1;
        (**(code **)(*piVar6 + 0x58))();
      }
      pcStack_a8 = (char *)0x4281f9;
      cVar2 = (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0xcc))();
      if (cVar2 == '\0') {
        if (*(int **)((int)this + 0xa18) != (int *)0x0) {
          pcStack_a8 = (char *)0x0;
          (**(code **)(**(int **)((int)this + 0xa18) + 0x4b4))();
        }
        *(undefined4 *)((int)this + 0xa20) = 0x3f000000;
        piVar6 = (int *)(*(int *)((int)this + 0xa30) + 0xc0);
        iVar5 = *(int *)(*(int *)((int)this + 0xa1c) + 0xc0);
        pcStack_a8 = acStack_20;
        iVar3 = (**(code **)(*(int *)(*(int *)((int)this + 0xa30) + 0xc0) + 0x310))();
        iVar3 = (**(code **)(*piVar6 + 0x310))(auStack_44,*(undefined4 *)(iVar3 + 8));
        puVar4 = (undefined4 *)(**(code **)(*piVar6 + 0x310))(auStack_68,*(undefined4 *)(iVar3 + 4))
        ;
        (**(code **)(iVar5 + 0x16c))(*puVar4);
        iVar5 = *(int *)(*(int *)((int)this + 0xa1c) + 0xc0);
        piVar6 = (int *)(*(int *)((int)this + 0xa30) + 0xc0);
        iVar3 = (**(code **)(*piVar6 + 0x328))(&pcStack_a8);
        iVar3 = (**(code **)(*piVar6 + 0x328))(&stack0xffffff64,*(undefined4 *)(iVar3 + 8));
        puVar4 = (undefined4 *)(**(code **)(*piVar6 + 0x328))(auStack_90,*(undefined4 *)(iVar3 + 4))
        ;
        (**(code **)(iVar5 + 0x174))(*puVar4);
        piVar6 = *(int **)((int)this + 0xa1c);
        if (piVar6[0x1c] != 0) {
          piVar6[0x1c] = 1;
          (**(code **)(*piVar6 + 0x58))(0);
        }
        (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0xd0))(1);
        (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0x214))(1);
        FUN_0042ae90();
        if (*(int *)((int)this + 0xa30) == 0) {
          iVar5 = 0;
        }
        else {
          iVar5 = *(int *)((int)this + 0xa30) + 0xc0;
        }
        piVar6 = (int *)((int)this + 0xc0);
        (**(code **)(*(int *)((int)this + 0xc0) + 0x3b0))(iVar5);
        (**(code **)(**(int **)((int)this + 0xa30) + 0xd8))();
        (**(code **)(*(int *)(*(int *)((int)this + 0xa30) + 0xc0) + 0xd0))(0);
        (**(code **)(*(int *)(*(int *)((int)this + 0xa30) + 0xc0) + 0x214))(0);
        (**(code **)(**(int **)((int)this + 0xa30) + 0x164))();
        iVar5 = *(int *)(*(int *)((int)this + 0xa30) + 0x690);
        if (iVar5 != -1) {
          FUN_00458a00(iVar5,0);
          *(undefined4 *)(*(int *)((int)this + 0xa30) + 0x690) = 0xffffffff;
        }
        if (*(int *)(*(int *)((int)this + 0xa30) + 0x69c) != 0) {
          *(undefined1 *)((int)this + 0x1e25) = 0;
          DAT_004f8182 = 0;
          (**(code **)(*DAT_00509948 + 0x118))(-(uint)(this != (void *)0x0) & (uint)piVar6);
          iVar5 = *piVar6;
          iVar3 = (**(code **)(iVar5 + 0x310))(&stack0xffffff68);
          iVar3 = (**(code **)(*piVar6 + 0x310))(auStack_7c,*(undefined4 *)(iVar3 + 8));
          puVar4 = (undefined4 *)
                   (**(code **)(*piVar6 + 0x310))(auStack_60,*(float *)(iVar3 + 4) + 40.0);
          (**(code **)(iVar5 + 0x318))(*puVar4);
          (**(code **)(*piVar6 + 0x278))(0,0,0);
          (**(code **)(*piVar6 + 0x2c4))(0,0,0);
          (**(code **)(*(int *)this + 0xe0))(&DAT_004eca64,1);
          if ((DAT_00509a13 == '\0') && (piVar6 = *(int **)((int)this + 0xa2c), piVar6[0x1c] != 0))
          {
            piVar6[0x1c] = 1;
            (**(code **)(*piVar6 + 0x58))(0);
          }
          *(undefined4 *)(*(int *)((int)this + 0xa30) + 0x69c) = 0;
          DAT_004f0588 = 0xffff;
          return;
        }
      }
    }
    break;
  case 5:
    if (*(int *)((int)this + 0xa1c) != 0) {
      pcStack_a8 = (char *)0x0;
      (**(code **)(**(int **)((int)this + 0xa18) + 0x4bc))();
      (**(code **)(**(int **)((int)this + 0xa1c) + 0x188))(0);
      DAT_004f0588 = 0xffff;
      return;
    }
    break;
  case 6:
    piVar6 = *(int **)((int)this + 0xa1c);
    *(undefined4 *)((int)this + 0x84c) = 0;
    if (piVar6 != (int *)0x0) {
      if (piVar6[0x1c] != 0) {
        pcStack_a8 = (char *)0x0;
        piVar6[0x1c] = 1;
        (**(code **)(*piVar6 + 0x58))();
      }
      pcStack_a8 = (char *)0x1;
      (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0xd0))();
      (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0x214))(1);
      piVar6 = *(int **)(*(int *)((int)this + 0xa1c) + 0x994);
      if ((piVar6 != (int *)0x0) && (piVar6[0x1c] != 0)) {
        pcStack_a8 = (char *)0x0;
        piVar6[0x1c] = 1;
        (**(code **)(*piVar6 + 0x58))();
      }
    }
    pcStack_a8 = (char *)0x428797;
    (**(code **)(*(int *)this + 0x170))();
    iVar5 = **(int **)(*(int *)((int)this + 0xa18) + 0x4e4);
    goto LAB_0042882c;
  case 7:
    piVar6 = *(int **)((int)this + 0xa1c);
    if ((piVar6 != (int *)0x0) && ((char)piVar6[0x27a] == '\0')) {
      if (piVar6[0x1c] != 0) {
        pcStack_a8 = (char *)0x0;
        piVar6[0x1c] = 1;
        (**(code **)(*piVar6 + 0x58))();
      }
      pcStack_a8 = (char *)0x1;
      (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0xd0))();
      (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0x214))(1);
    }
    pcStack_a8 = (char *)0x428806;
    (**(code **)(*(int *)this + 0x204))();
    pcStack_a8 = (char *)0x428810;
    (**(code **)(*(int *)this + 0x170))();
    DAT_004f8210 = 0;
    *(undefined1 *)((int)this + 0x864) = 0;
    if (*(int **)((int)this + 0xb14) == (int *)0x0) {
      DAT_004f0588 = 0xffff;
      return;
    }
    iVar5 = **(int **)((int)this + 0xb14);
LAB_0042882c:
    pcStack_a8 = (char *)0x1;
    (**(code **)(iVar5 + 0x58))();
  }
  DAT_004f0588 = 0xffff;
  return;
}


```

## JimmyActionHelper98_00429d00 @ 00429d00

```c

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __thiscall JimmyActionHelper98_00429d00(void *this)

{
  int *piVar1;
  float fVar2;
  float fVar3;
  int *piVar4;
  float fVar5;
  char cVar6;
  float *pfVar7;
  undefined4 uVar8;
  int iVar9;
  int iVar10;
  undefined4 *puVar11;
  C3DPlayer *this_00;
  C3DPlayer *extraout_ECX;
  C3DPlayer *this_01;
  float fVar12;
  float fVar13;
  float unaff_retaddr;
  float fVar14;
  float fVar15;
  float fStack_80;
  float fStack_7c;
  float fStack_78;
  float fStack_74;
  float fStack_70;
  float fStack_6c;
  float fStack_68;
  float fStack_64;
  float fStack_60;
  float fStack_5c;
  float fStack_58;
  float fStack_54;
  float fStack_50;
  undefined4 uStack_4c;
  float fStack_48;
  float fStack_44;
  float fStack_40;
  undefined1 auStack_3c [20];
  undefined1 auStack_28 [16];
  undefined1 auStack_18 [4];
  float fStack_14;

  fStack_5c = 1.0;
  piVar1 = (int *)((int)this + 0xc0);
  uStack_4c = 1.0;
  pfVar7 = (float *)(**(code **)(*(int *)((int)this + 0xc0) + 0x310))();
  fStack_6c = *pfVar7;
  fStack_64 = pfVar7[2];
  fStack_68 = pfVar7[1] + 116.0;
  fStack_60 = pfVar7[3];
  cVar6 = C3DPlayer::vfunc_04_087((C3DPlayer *)pfVar7[1]);
  if (cVar6 == '\0') {
    if (DAT_004f8210 != '\0') {
      iVar10 = *(int *)((int)this + 0x1e04);
      goto joined_r0x00429da4;
    }
  }
  else {
    iVar10 = *(int *)((int)this + 0x1e04);
    if (DAT_004f8210 == '\0') {
joined_r0x00429da4:
      if (iVar10 != -1) {
        FUN_00458a00();
        *(undefined4 *)((int)this + 0x1e04) = 0xffffffff;
      }
    }
    else if (iVar10 == -1) {
      uVar8 = FUN_00458980(0xffffffff);
      *(undefined4 *)((int)this + 0x1e04) = uVar8;
    }
  }
  if (DAT_004f0588 != 7) {
    return 0;
  }
  if (_DAT_004f82c8 < DAT_004f82b8) {
    DAT_004f82b8 = DAT_004f82b8 - unaff_retaddr * 950.0;
  }
  cVar6 = FUN_0042a730();
  if (cVar6 != '\0') {
    if (DAT_004f8210 == '\0') goto LAB_0042a37b;
    cVar6 = C3DPlayer::vfunc_04_087(this_00);
    if (cVar6 == '\0') {
      (**(code **)(*(int *)this + 0x204))();
      return 0;
    }
  }
  if (DAT_004f8210 != '\0') {
    iVar9 = DAT_004f82cc * 0x10;
    iVar10 = *piVar1;
    fStack_5c = *(float *)(&DAT_004f8214 + iVar9);
    fStack_58 = *(float *)(&DAT_004f8218 + iVar9);
    fStack_54 = *(float *)(&DAT_004f821c + iVar9);
    fStack_50 = (float)(&DAT_004f8220)[DAT_004f82cc * 4];
    iVar9 = (**(code **)(iVar10 + 0x328))();
    (**(code **)(iVar10 + 0x330))(0,*(undefined4 *)(iVar9 + 4));
    (**(code **)(*(int *)this + 0x150))(s_SWING_004ef6d8);
    iVar10 = DAT_004f82cc * 0x10;
    fVar15 = *(float *)(&DAT_004f8218 + iVar10) - fStack_7c;
    fVar2 = *(float *)(&DAT_004f821c + iVar10) - fStack_78;
    fVar14 = _DAT_004f82bc * fStack_14 * 30.0 + (*(float *)(&DAT_004f8214 + iVar10) - fStack_80);
    fVar3 = _DAT_004f82c0 * fStack_14;
    fVar12 = OMedia3DVector::quick_magnitude((OMedia3DVector *)&stack0xffffff60);
    fVar12 = 1.0 / fVar12;
    iVar10 = DAT_004f82cc * 0x10;
    fVar13 = DAT_004f82b8 * fVar12 * fVar14;
    fVar5 = DAT_004f82b8 * fVar12 * (fVar3 * 30.0 + fVar2);
    fVar3 = *(float *)(&DAT_004f821c + iVar10);
    fVar2 = *(float *)(&DAT_004f8214 + iVar10);
    fVar14 = (float)(&DAT_004f8220)[DAT_004f82cc * 4];
    fVar15 = *(float *)(&DAT_004f8218 + iVar10) - (DAT_004f82b8 * fVar12 * fVar15 + 116.0);
    pfVar7 = (float *)(**(code **)(*piVar1 + 0x310))(auStack_28);
    fStack_5c = *pfVar7;
    fStack_58 = pfVar7[1];
    uStack_4c = 0.0 - fStack_5c;
    fStack_54 = pfVar7[2];
    fStack_50 = pfVar7[3];
    fStack_48 = (fVar2 - fVar13) - fStack_58;
    fStack_44 = fVar15 - fStack_54;
    fStack_40 = 0.0;
    FUN_00409f60(&uStack_4c);
    FUN_0047b4b0(-(uint)(this != (void *)0x0) & (uint)piVar1,&fStack_5c);
    (**(code **)(*piVar1 + 0x314))(fStack_5c,fStack_58,fStack_54,fStack_50);
    iVar10 = *piVar1;
    iVar9 = (**(code **)(iVar10 + 700))(auStack_3c);
    puVar11 = (undefined4 *)(**(code **)(*piVar1 + 700))(&fStack_50,0,*(undefined4 *)(iVar9 + 8));
    (**(code **)(iVar10 + 0x2c4))(*puVar11);
    _DAT_004f82c4 = _DAT_004f82c4 + unaff_retaddr;
    _DAT_004f82bc = _DAT_004f82bc - fVar15 * unaff_retaddr * 0.17;
    _DAT_004f82c0 = _DAT_004f82c0 - fVar14 * unaff_retaddr * 0.17;
    if (0.2 <= _DAT_004f82c4) {
      if (_DAT_004f82bc <= 50.0) {
        if (_DAT_004f82bc < -50.0) {
          _DAT_004f82bc = -50.0;
        }
      }
      else {
        _DAT_004f82bc = 50.0;
      }
      if (_DAT_004f82c0 <= 50.0) {
        if (_DAT_004f82c0 < -50.0) {
          _DAT_004f82c0 = -50.0;
        }
      }
      else {
        _DAT_004f82c0 = 50.0;
      }
      if (350.0 <= DAT_004f82b8) {
        _DAT_004f82bc = _DAT_004f82bc * 0.98;
        fVar2 = 0.98;
      }
      else {
        _DAT_004f82bc = _DAT_004f82bc * 0.95;
        fVar2 = 0.95;
      }
      _DAT_004f82c0 = _DAT_004f82c0 * fVar2;
      _DAT_004f82c4 = _DAT_004f82c4 - 0.2;
    }
    if (*(int *)((int)this + 0xb0c) != 0) {
      OMedia3DVector::angles
                ((OMedia3DVector *)&stack0xffffff74,(short *)((int)&uStack_4c + 2),
                 (short *)&uStack_4c);
      fStack_48 = (float)(int)uStack_4c._2_2_;
      (**(code **)(*(int *)(*(int *)((int)this + 0xb0c) + 0xc0) + 0x330))
                ((float)(int)fStack_48 * 0.021972656);
      (**(code **)(*(int *)(*(int *)((int)this + 0xb0c) + 0xc0) + 0x314))
                (fVar3 - fVar5,fVar14 + 116.0,fStack_80,fStack_7c);
      pfVar7 = (float *)(**(code **)(*piVar1 + 0x310))(&fStack_40);
      fStack_6c = *pfVar7;
      fStack_68 = pfVar7[1];
      fStack_64 = pfVar7[2];
      fStack_58 = fStack_68 - 2500.0;
      fStack_60 = pfVar7[3];
      fStack_5c = fStack_6c;
      fStack_54 = fStack_64;
      fStack_50 = fStack_60;
      if (DAT_00509a13 == '\0') {
        FUN_00409f60(&fStack_5c);
        FUN_00409f60(&fStack_6c);
        cVar6 = FUN_0047c6e0();
        if (cVar6 == '\0') {
          if (*(int **)((int)this + 0xa2c) != (int *)0x0) {
            (**(code **)(**(int **)((int)this + 0xa2c) + 0x58))();
          }
        }
        else {
          fStack_74 = DAT_005cfc68;
          fStack_7c = DAT_005cfc60;
          fStack_78 = DAT_005cfc64;
          fStack_70 = DAT_005cfc6c;
          if (*(int *)((int)this + 0xa2c) != 0) {
            iVar10 = *(int *)(*(int *)((int)this + 0xa2c) + 200);
            (**(code **)(*piVar1 + 0x310))();
            puVar11 = (undefined4 *)(**(code **)(*piVar1 + 0x310))(auStack_18,fStack_7c + 30.0);
            (**(code **)(iVar10 + 0x318))(*puVar11);
            piVar4 = *(int **)((int)this + 0xa2c);
            if (piVar4[0x1c] != 0) {
              piVar4[0x1c] = 1;
              (**(code **)(*piVar4 + 0x58))();
            }
          }
        }
      }
    }
  }
LAB_0042a37b:
  pfVar7 = (float *)(**(code **)(*piVar1 + 0x310))();
  fStack_70 = *pfVar7;
  fStack_6c = pfVar7[1];
  fStack_68 = pfVar7[2];
  fStack_64 = pfVar7[3];
  pfVar7 = (float *)(**(code **)(*piVar1 + 900))(auStack_28,0,0x45160000);
  fStack_70 = *pfVar7;
  fStack_6c = pfVar7[1];
  fStack_68 = pfVar7[2];
  fStack_64 = pfVar7[3];
  FUN_00409f60(&fStack_70);
  FUN_00409f60(&fStack_80);
  cVar6 = FUN_0047c210();
  fVar14 = DAT_005cfc68;
  fVar3 = DAT_005cfc64;
  fVar2 = DAT_005cfc60;
  if (cVar6 == '\0') {
    if (*(int **)((int)this + 0xb14) != (int *)0x0) {
      (**(code **)(**(int **)((int)this + 0xb14) + 0x58))(1);
    }
  }
  else {
    fStack_40 = DAT_005cfc68 - fStack_78;
    if (SQRT((DAT_005cfc64 - fStack_7c) * (DAT_005cfc64 - fStack_7c) +
             fStack_40 * fStack_40 + (DAT_005cfc60 - fStack_80) * (DAT_005cfc60 - fStack_80)) <
        450.0) {
      if (*(int **)((int)this + 0xb14) != (int *)0x0) {
        (**(code **)(**(int **)((int)this + 0xb14) + 0x58))(1);
      }
      return 1;
    }
    if ((DAT_00695d54 != 0) &&
       ((*(int *)(DAT_00695d54 + 4) == 0 ||
        ((iVar10 = *(int *)(*(int *)(DAT_00695d54 + 4) + 0xc), iVar10 != 0 &&
         ((((iVar10 = *(int *)(iVar10 + 8), 0x4f < iVar10 && (iVar10 < 0x5a)) ||
           ((0xd1 < iVar10 && (iVar10 < 0xdc)))) ||
          ((((0x3b < iVar10 && (iVar10 < 0x46)) || (iVar10 == -1)) || (iVar10 == 0xaa)))))))))) {
      this_01 = (C3DPlayer *)0x0;
      if (*(int **)((int)this + 0xb14) != (int *)0x0) {
        (**(code **)(**(int **)((int)this + 0xb14) + 0x58))(1);
        this_01 = extraout_ECX;
      }
      cVar6 = C3DPlayer::vfunc_04_087(this_01);
      if (((cVar6 != '\0') || (cVar6 = FUN_0046a3e0(0xd), cVar6 != '\0')) && (250.0 < DAT_004f82b8))
      {
        DAT_004f82b8 = DAT_004f82b8 - fStack_14 * 250.0;
      }
      return 1;
    }
    if (*(int *)((int)this + 0xb14) != 0) {
      fVar15 = fStack_70 - fStack_80;
      fVar12 = fStack_6c - fStack_7c;
      fVar5 = fStack_68 - fStack_78;
      fVar13 = OMedia3DVector::quick_magnitude((OMedia3DVector *)&stack0xffffff60);
      fVar13 = 1.0 / fVar13;
      (**(code **)(*(int *)(*(int *)((int)this + 0xb14) + 200) + 0x318))
                (fVar2 - fVar13 * fVar15 * 100.0,fVar3 - fVar13 * fVar12 * 100.0,
                 fVar14 - fVar13 * fVar5 * 100.0);
      piVar1 = *(int **)((int)this + 0xb14);
      if (piVar1[0x1c] != 0) {
        piVar1[0x1c] = 1;
        (**(code **)(*piVar1 + 0x58))(0);
        return 0;
      }
    }
  }
  return 0;
}


```

## JimmyActionHelper102_0042ab00 @ 0042ab00

```c

void __thiscall JimmyActionHelper102_0042ab00(void)

{
  FUN_00458980(0xffffffff,0x27,0);
  if (DAT_004f0588 != 0) {
    FUN_00458980(0xffffffff,0xbe,0);
  }
  return;
}


```

## JimmyActionHelper105_00425170 @ 00425170

```c

void __thiscall JimmyActionHelper105_00425170(void *this)

{
  int *piVar1;
  char cVar2;
  undefined4 *puVar3;
  int iVar4;
  int iVar5;
  undefined1 *puVar6;
  undefined4 uStack_44;
  float fStack_40;
  undefined4 uStack_3c;
  undefined4 uStack_38;
  undefined4 uStack_34;
  float fStack_30;
  undefined4 uStack_2c;
  undefined4 uStack_28;
  undefined4 uStack_24;
  undefined1 auStack_20 [32];

  puVar6 = auStack_20;
  uStack_24 = 0x3f800000;
  piVar1 = (int *)((int)this + 0xc0);
  uStack_34 = 0x3f800000;
  puVar3 = (undefined4 *)(**(code **)(*(int *)((int)this + 0xc0) + 0x310))(puVar6);
  uStack_44 = *puVar3;
  fStack_30 = (float)puVar3[1];
  uStack_3c = puVar3[2];
  fStack_40 = fStack_30 + 150.0;
  uStack_38 = puVar3[3];
  uStack_34 = uStack_44;
  uStack_2c = uStack_3c;
  uStack_28 = uStack_38;
  FUN_00409f60(&uStack_44);
  FUN_00409f60(&uStack_34);
  cVar2 = FUN_0047c210();
  if ((cVar2 != '\0') &&
     (iVar4 = (**(code **)(*piVar1 + 700))(&uStack_24), 0.0 < *(float *)(iVar4 + 4))) {
    (**(code **)(*piVar1 + 700))(&uStack_28);
    iVar4 = *piVar1;
    iVar5 = (**(code **)(iVar4 + 700))(&uStack_2c);
    puVar3 = (undefined4 *)
             (**(code **)(*piVar1 + 700))(auStack_20,puVar6,*(undefined4 *)(iVar5 + 8));
    (**(code **)(iVar4 + 0x2c4))(*puVar3);
    if (DAT_004f0588 != 0) {
      if (5.0 < *(float *)((int)this + 0x1e5c)) {
        FUN_00458980(0xffffffff,0x2f,0);
      }
      *(undefined4 *)((int)this + 0x1e5c) = 0;
    }
    (**(code **)(*(int *)this + 0x1e0))();
  }
  return;
}


```

## JimmyActionHelper106_004252e0 @ 004252e0

```c

void __thiscall JimmyActionHelper106_004252e0(void *this)

{
  int *piVar1;
  int iVar2;
  char cVar3;
  undefined4 *puVar4;
  int iVar5;
  float *pfVar6;
  float *pfVar7;
  float unaff_ESI;
  undefined4 uStack_54;
  float fStack_50;
  undefined4 uStack_4c;
  undefined4 uStack_48;
  undefined4 uStack_44;
  float fStack_40;
  undefined4 uStack_3c;
  undefined4 uStack_38;
  float fStack_34;
  undefined4 uStack_30;
  undefined4 uStack_2c;
  undefined4 uStack_28;
  undefined1 auStack_24 [12];
  undefined1 auStack_18 [24];

  fStack_34 = 1.0;
  uStack_44 = 0x3f800000;
  if (DAT_004f8210 == '\0') {
    piVar1 = (int *)((int)this + 0xc0);
    puVar4 = (undefined4 *)(**(code **)(*(int *)((int)this + 0xc0) + 0x310))(&uStack_30);
    uStack_54 = *puVar4;
    fStack_40 = (float)puVar4[1];
    uStack_4c = puVar4[2];
    fStack_50 = fStack_40 - 2500.0;
    uStack_48 = puVar4[3];
    uStack_44 = uStack_54;
    uStack_3c = uStack_4c;
    uStack_38 = uStack_48;
    FUN_00409f60(&uStack_54);
    FUN_00409f60(&uStack_44);
    cVar3 = FUN_0047c6e0();
    if (cVar3 != '\0') {
      fStack_34 = DAT_005cfc60;
      uStack_30 = DAT_005cfc64;
      uStack_28 = DAT_005cfc6c;
      uStack_2c = DAT_005cfc68;
      if (*(int *)((int)this + 0xa2c) != 0) {
        iVar2 = *(int *)(*(int *)((int)this + 0xa2c) + 200);
        iVar5 = (**(code **)(*piVar1 + 0x310))(auStack_24);
        puVar4 = (undefined4 *)
                 (**(code **)(*piVar1 + 0x310))
                           (auStack_18,fStack_34 + unaff_ESI,*(undefined4 *)(iVar5 + 8));
        (**(code **)(iVar2 + 0x318))(*puVar4);
        pfVar6 = (float *)(**(code **)(*(int *)(*(int *)((int)this + 0xa2c) + 200) + 0x310))
                                    (&uStack_28);
        pfVar7 = (float *)(**(code **)(*piVar1 + 0x310))(&uStack_3c);
        *(float *)((int)this + 0x86c) =
             SQRT((*pfVar6 - *pfVar7) * (*pfVar6 - *pfVar7) +
                  (pfVar6[1] - pfVar7[1]) * (pfVar6[1] - pfVar7[1]) +
                  (pfVar6[2] - pfVar7[2]) * (pfVar6[2] - pfVar7[2]));
      }
    }
  }
  return;
}


```

## JimmyActionHelper107_00427370 @ 00427370

```c

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __thiscall JimmyActionHelper107_00427370(void *this,undefined4 param_2)

{
  char cVar1;
  short sVar2;
  short sVar3;
  undefined2 uVar4;
  undefined2 extraout_var;
  undefined2 extraout_var_00;
  C3DPlayer *this_00;
  C3DPlayer *extraout_ECX;
  C3DPlayer *extraout_ECX_00;
  C3DPlayer *extraout_ECX_01;
  C3DPlayer *extraout_ECX_02;
  C3DPlayer *extraout_ECX_03;
  C3DPlayer *extraout_ECX_04;
  C3DPlayer *extraout_ECX_05;
  C3DPlayer *this_01;
  undefined2 extraout_var_01;
  undefined2 extraout_var_02;
  undefined2 extraout_var_03;
  int iVar5;
  int *piVar6;
  float unaff_retaddr;
  undefined4 uVar7;
  undefined4 uVar8;
  void *pvStack_4;

  if (*(int **)((int)this + 0xa18) == (int *)0x0) {
    return 0;
  }
  pvStack_4 = this;
  if (DAT_004f8181 == '\0') {
    if (DAT_004f8180 != '\0') {
      return 0;
    }
    if (DAT_004ec494 != '\x01') {
      return 0;
    }
    cVar1 = FUN_00475ca0();
    if (cVar1 == '\0') {
      return 0;
    }
    if (DAT_005099e4 == 0) {
      return 0;
    }
    if (DAT_004f8430 != '\0') {
      return 0;
    }
    (**(code **)(**(int **)((int)this + 0xa18) + 0x468))(DAT_004f83d4);
    sVar2 = __ftol();
    FUN_00468660(0x28,0x43,&LAB_00424947,&DAT_004ec794,(int)sVar2);
    uVar4 = extraout_var;
    if (0.0 < _DAT_004f83d0) {
      _DAT_004f83d0 = _DAT_004f83d0 - unaff_retaddr;
      FUN_00468660(300,0x1e,&LAB_00424947,&DAT_004ec794,(int)DAT_004f83cc);
      uVar4 = extraout_var_00;
    }
    if (_DAT_004f83c8 == 0.0) {
      if (_DAT_004f8188 < 7.0) {
        return 0;
      }
      FUN_00468660(0x20d,CONCAT22(uVar4,*(short *)(*(int *)((int)this + 0xa18) + 0x4ec) + 0x1c2),
                   &PTR_LAB_004d4544,s__6_0d_004ec714,DAT_004f83c0);
      return 0;
    }
    if (_DAT_004f8188 < 7.0) {
      return 0;
    }
    if (*(int *)((int)this + 0xa18) == 0) {
      return 0;
    }
    FUN_00468660(0x1a9,CONCAT22(uVar4,*(short *)(*(int *)((int)this + 0xa18) + 0x4ec) + 0x1b3),
                 &LAB_00424947,s__6_0d_004ec714,DAT_004f83c0);
    return 0;
  }
  sVar2 = (**(code **)(**(int **)((int)this + 0xa18) + 0x4a8))();
  if (sVar2 == 2) {
    cVar1 = FUN_0042a730();
    if ((((cVar1 == '\0') && (cVar1 = C3DPlayer::vfunc_04_087(this_00), cVar1 == '\0')) &&
        (cVar1 = FUN_0046a3e0(0xd), cVar1 == '\0')) && (DAT_00509849 == '\0')) {
      *(undefined1 *)((int)this + 0x1e25) = 0;
      return 1;
    }
    if (*(char *)((int)this + 0x1e25) == '\0') {
      if (*(int *)((int)this + 0x1e4c) != -1) {
        (**(code **)(*(int *)((int)this + 0xc0) + 0x214))(1);
        *(undefined1 *)((int)this + 0x1e25) = 1;
        DAT_004f8182 = 1;
        (**(code **)(*(int *)this + 0x1fc))();
        (**(code **)(*(int *)this + 0x1f8))(3);
        *(undefined4 *)((int)this + 0x1e4c) = 3;
        return 1;
      }
      (**(code **)(*(int *)this + 0x1fc))();
      *(undefined1 *)((int)this + 0x1e25) = 1;
    }
    return 1;
  }
  sVar2 = (**(code **)(**(int **)((int)this + 0xa18) + 0x4a8))();
  if (sVar2 == 4) {
    Menu_ActivateItem_004038c0();
    piVar6 = &DAT_004f82e4;
    iVar5 = 0x5f;
    do {
      if (*piVar6 != -1) {
        FUN_00468660(0x19f,iVar5,&PTR_LAB_004d4544,&PTR_DAT_004ef490,*piVar6);
        FUN_00468660(0x145,iVar5,&PTR_LAB_004d4544,&DAT_004ef488,piVar6[4]);
      }
      piVar6 = piVar6 + 5;
      iVar5 = iVar5 + 0x1e;
    } while ((int)piVar6 < 0x4f8399);
  }
  sVar2 = (**(code **)(**(int **)((int)this + 0xa18) + 0x4a8))();
  if (sVar2 == 5) {
    if (DAT_004f6b34 < 10) {
      uVar7 = 0xa0;
    }
    else {
      uVar7 = 0x91;
    }
    FUN_00468660(uVar7,200,&LAB_00424947,&DAT_004ec794,DAT_004f6b34);
    if (DAT_004f6b38 < 10) {
      uVar7 = 0x1ad;
    }
    else {
      uVar7 = 0x19e;
    }
    FUN_00468660(uVar7,200,&LAB_00424947,&DAT_004ec794,DAT_004f6b38);
  }
  FUN_00476d70(&pvStack_4,&param_2);
  uVar7 = (**(code **)(**(int **)((int)this + 0xa18) + 0x4a4))(pvStack_4,param_2);
  sVar2 = (short)uVar7;
  if (sVar2 == -1) {
    sVar2 = (**(code **)(**(int **)((int)this + 0xa18) + 0x4a8))();
    if (sVar2 == 3) {
      Menu_ActivateItem_004038c0();
      Menu_ActivateItem_004038c0();
      Menu_ActivateItem_004038c0();
      Menu_ActivateItem_004038c0();
      Menu_ActivateItem_004038c0();
      Menu_ActivateItem_004038c0();
      Menu_ActivateItem_004038c0();
    }
    else {
      sVar2 = (**(code **)(**(int **)((int)this + 0xa18) + 0x4a8))();
      if (sVar2 != 1) {
        return 1;
      }
    }
    (**(code **)(**(int **)((int)this + 0xa18) + 0x4e8))(0xffffffff);
    return 1;
  }
  sVar3 = (**(code **)(**(int **)((int)this + 0xa18) + 0x4a8))();
  if (sVar3 != 3) {
    sVar3 = (**(code **)(**(int **)((int)this + 0xa18) + 0x4a8))();
    this_01 = extraout_ECX_04;
    if ((sVar3 == 1) && (sVar2 == 9)) {
      uVar8 = 0xb5;
      iVar5 = **(int **)((int)this + 0xa18);
      goto LAB_00427705;
    }
    goto switchD_004275ea_caseD_b;
  }
  Menu_ActivateItem_004038c0();
  Menu_ActivateItem_004038c0();
  Menu_ActivateItem_004038c0();
  Menu_ActivateItem_004038c0();
  Menu_ActivateItem_004038c0();
  Menu_ActivateItem_004038c0();
  Menu_ActivateItem_004038c0();
  this_01 = extraout_ECX;
  switch(sVar2) {
  case 1:
    Menu_ActivateItem_004038c0();
    (**(code **)(**(int **)((int)this + 0xa18) + 0x4e8))(0x95);
    this_01 = extraout_ECX_00;
    break;
  case 2:
    Menu_ActivateItem_004038c0();
    iVar5 = **(int **)((int)this + 0xa18);
    uVar8 = 0xd;
    goto LAB_00427705;
  case 3:
    Menu_ActivateItem_004038c0();
    (**(code **)(**(int **)((int)this + 0xa18) + 0x4e8))(0x85);
    this_01 = extraout_ECX_01;
    break;
  case 4:
    Menu_ActivateItem_004038c0();
    iVar5 = **(int **)((int)this + 0xa18);
    uVar8 = 0x88;
    goto LAB_00427705;
  default:
    uVar8 = 0xffffffff;
    iVar5 = **(int **)((int)this + 0xa18);
    goto LAB_00427705;
  case 6:
    Menu_ActivateItem_004038c0();
    (**(code **)(**(int **)((int)this + 0xa18) + 0x4e8))(0xcc);
    this_01 = extraout_ECX_02;
    break;
  case 0xb:
    break;
  case 0xc:
    Menu_ActivateItem_004038c0();
    iVar5 = **(int **)((int)this + 0xa18);
    uVar8 = 0x75;
LAB_00427705:
    (**(code **)(iVar5 + 0x4e8))(uVar8);
    this_01 = extraout_ECX_05;
    break;
  case 0xd:
    Menu_ActivateItem_004038c0();
    (**(code **)(**(int **)((int)this + 0xa18) + 0x4e8))(200);
    this_01 = extraout_ECX_03;
  }
switchD_004275ea_caseD_b:
  cVar1 = C3DPlayer::vfunc_04_087(this_01);
  if (((cVar1 == '\0') && (cVar1 = FUN_0042a730(), cVar1 == '\0')) &&
     ((cVar1 = FUN_0046a3e0(0xd), cVar1 == '\0' && (DAT_00509849 == '\0')))) {
    *(undefined1 *)((int)this + 0x1e25) = 0;
    return 1;
  }
  cVar1 = (**(code **)(**(int **)((int)this + 0xa18) + 0x478))(uVar7);
  if (cVar1 == '\0') {
    return 1;
  }
  sVar2 = (**(code **)(**(int **)((int)this + 0xa18) + 0x4a8))();
  if ((sVar2 == 0) && (*(short *)(*(int *)((int)this + 0xa18) + 0x4c8) == 9)) {
    iVar5 = FUN_0045fea0(s_SCENE_004ed220);
    if (499 < iVar5) {
      (**(code **)(**(int **)((int)this + 0xa18) + 0x4e8))(0xb7);
      return 1;
    }
    if (*(char *)((int)this + 0x1e25) != '\0') {
      return 1;
    }
    *(undefined1 *)((int)this + 0x1e25) = 1;
    if ((0x56523030 < *(int *)(DAT_00509948 + 0x490)) &&
       (*(int *)(DAT_00509948 + 0x490) < 0x56523039)) {
      return 1;
    }
    sVar2 = FUN_00403950(0,9);
    if (sVar2 != 2) {
      Menu_ActivateItem_004038c0();
      return 1;
    }
    Menu_ActivateItem_004038c0();
    return 1;
  }
  uVar4 = (**(code **)(**(int **)((int)this + 0xa18) + 0x4a8))();
  switch(uVar4) {
  case 0:
    if (*(char *)((int)this + 0x1e25) == '\0') {
      if (*(short *)(*(int *)((int)this + 0xa18) + 0x4c8) != 10) {
        (**(code **)(*(int *)this + 0x1fc))();
        (**(code **)(*(int *)this + 0x178))();
        (**(code **)(*(int *)this + 0x174))
                  (CONCAT22(extraout_var_03,*(undefined2 *)(*(int *)((int)this + 0xa18) + 0x4c8)));
        Menu_ActivateItem_004038c0();
        (**(code **)(*(int *)((int)this + 0xc0) + 0x214))(1);
        *(undefined1 *)((int)this + 0x1e25) = 1;
        return 1;
      }
      if (*(int *)((int)this + 0x1e4c) == -1) {
        (**(code **)(*(int *)((int)this + 0xc0) + 0x214))(1);
        (**(code **)(*(int *)this + 0x1fc))();
      }
      else {
        (**(code **)(*(int *)((int)this + 0xc0) + 0x214))(1);
        (**(code **)(*(int *)this + 0x1fc))();
        (**(code **)(*(int *)this + 0x1f8))(3);
        *(undefined4 *)((int)this + 0x1e4c) = 3;
      }
      *(undefined1 *)((int)this + 0x1e25) = 1;
      Menu_ActivateItem_004038c0();
      return 1;
    }
    break;
  case 1:
    sVar2 = *(short *)(*(int *)((int)this + 0xa18) + 0x4c8);
    if ((sVar2 < 10) || (0xc < sVar2)) {
      (**(code **)(*(int *)this + 0x1fc))();
      (**(code **)(*(int *)this + 0x1d0))
                (CONCAT22(extraout_var_01,*(undefined2 *)(*(int *)((int)this + 0xa18) + 0x4c8)));
      goto LAB_00427998;
    }
    if (*(char *)((int)this + 0x1e25) == '\0') {
      (**(code **)(*(int *)this + 0x1d0))
                (CONCAT22((short)((uint)*(int *)((int)this + 0xa18) >> 0x10),sVar2));
      *(undefined1 *)((int)this + 0x1e25) = 1;
      return 1;
    }
    break;
  case 2:
    if (*(char *)((int)this + 0x1e25) == '\0') {
      (**(code **)(*(int *)((int)this + 0xc0) + 0x214))(1);
      *(undefined1 *)((int)this + 0x1e25) = 1;
      DAT_004f8182 = 1;
      (**(code **)(*(int *)this + 0x1fc))();
      (**(code **)(*(int *)this + 0x1f8))(3);
      *(undefined4 *)((int)this + 0x1e4c) = 3;
      return 1;
    }
    break;
  case 3:
    if (*(char *)((int)this + 0x1e25) == '\0') {
      (**(code **)(*(int *)this + 0x1fc))();
      (**(code **)(*(int *)this + 500))
                (CONCAT22(extraout_var_02,*(undefined2 *)(*(int *)((int)this + 0xa18) + 0x4c8)));
      goto LAB_00427998;
    }
    break;
  case 4:
    if (*(char *)((int)this + 0x1e25) != '\0') {
      return 1;
    }
    piVar6 = *(int **)((int)this + 0xa18);
    iVar5 = (short)piVar6[0x132] + -1;
    switch(iVar5) {
    case 0:
      (**(code **)(*(int *)((int)this + 0xc0) + 0x214))(1);
      *(undefined1 *)((int)this + 0x1e25) = 1;
      DAT_004f8182 = 1;
      (**(code **)(*(int *)this + 0x1fc))();
      if (*(int *)((int)this + 0x1e4c) != -1) {
        (**(code **)(*(int *)this + 0x1f8))(3);
      }
      goto LAB_00427998;
    case 1:
      (**(code **)(*(int *)this + 0x178))();
      (**(code **)(*(int *)((int)this + 0xc0) + 0x214))(1);
      *(undefined1 *)((int)this + 0x1e25) = 1;
      DAT_004f8182 = 1;
      (**(code **)(*(int *)this + 0x1fc))();
      (**(code **)(*(int *)this + 0x178))();
      FUN_00406f90(0xb);
      return 1;
    case 2:
      (**(code **)(*(int *)this + 0x178))();
      (**(code **)(*(int *)((int)this + 0xc0) + 0x214))(1);
      *(undefined1 *)((int)this + 0x1e25) = 1;
      DAT_004f8182 = 1;
      (**(code **)(*(int *)this + 0x1fc))();
      if (*(int *)((_DAT_004eefd4 * 5 + -0x19) * 4 + 0x4f82d0) != -1) {
        (**(code **)(*(int *)this + 0x178))();
        FUN_00406f90(9);
      }
      return 1;
    case 3:
      if ((0x56523030 < *(int *)(DAT_00509948 + 0x490)) &&
         (*(int *)(DAT_00509948 + 0x490) < 0x56523039)) {
        (**(code **)(*piVar6 + 0x4e8))(8);
        goto LAB_00427998;
      }
      (**(code **)(*(int *)this + 0x1fc))();
      if (*(int *)((_DAT_004eefd4 * 5 + -0x19) * 4 + 0x4f82d0) != -1) {
        FUN_00406f90(8);
        DAT_004f8182 = 1;
        goto LAB_00427998;
      }
      FUN_0042b0b0();
      (**(code **)(**(int **)((int)this + 0xa18) + 0x4e8))(0x87);
      break;
    case 4:
      (**(code **)(*(int *)this + 0x1fc))();
      if (*(int *)((_DAT_004eefd4 * 5 + -0x19) * 4 + 0x4f82d0) != -1) {
        FUN_00406f90(0xd);
        (**(code **)(**(int **)((int)this + 0xa18) + 0x4e8))(0xa5);
        DAT_004f8182 = 1;
        goto LAB_00427998;
      }
      break;
    default:
      Menu_ActivateItem_004038c0(piVar6,4,CONCAT22((short)((uint)iVar5 >> 0x10),DAT_004eefd4),1);
      _DAT_004eefd4 = (int)*(short *)(*(int *)((int)this + 0xa18) + 0x4c8);
      Menu_ActivateItem_004038c0(*(int *)((int)this + 0xa18),4,_DAT_004eefd4,2);
      *(undefined1 *)((int)this + 0x1e25) = 1;
      return 1;
    }
    DAT_004f8182 = 1;
LAB_00427998:
    (**(code **)(*(int *)((int)this + 0xc0) + 0x214))(1);
    *(undefined1 *)((int)this + 0x1e25) = 1;
    return 1;
  case 5:
    if (*(char *)((int)this + 0x1e25) == '\0') {
      *(undefined1 *)((int)this + 0x1e25) = 1;
      switch(*(undefined2 *)(*(int *)((int)this + 0xa18) + 0x4c8)) {
      case 1:
        DAT_004f6b34 = DAT_004f6b34 + 1;
        if (9 < DAT_004f6b34) {
          DAT_004f6b34 = 10;
        }
        DAT_00695f80 = DAT_004f6b34;
        return 1;
      case 2:
        DAT_004f6b34 = DAT_004f6b34 + -1;
        if (DAT_004f6b34 < 1) {
          DAT_004f6b34 = 0;
        }
        DAT_00695f80 = DAT_004f6b34;
        return 1;
      case 3:
        goto switchD_00427ce3_caseD_3;
      case 4:
        DAT_004f6b38 = DAT_004f6b38 + 1;
        if (9 < DAT_004f6b38) {
          DAT_004f6b38 = 10;
          return 1;
        }
        break;
      case 5:
        DAT_004f6b38 = DAT_004f6b38 + -1;
        if (DAT_004f6b38 < 1) {
          DAT_004f6b38 = 0;
          return 1;
        }
      }
    }
    break;
  case 6:
    if (*(char *)((int)this + 0x1e25) == '\0') {
switchD_00427ce3_caseD_3:
      (**(code **)(*(int *)((int)this + 0xc0) + 0x214))(1);
      *(undefined1 *)((int)this + 0x1e25) = 1;
      DAT_004f8182 = 1;
      (**(code **)(*(int *)this + 0x1fc))();
      (**(code **)(*(int *)this + 0x1f8))(3);
      *(undefined4 *)((int)this + 0x1e4c) = 3;
      return 1;
    }
  }
  return 1;
}


```

## JimmyActionHelper108_00425490 @ 00425490

```c

void __thiscall JimmyActionHelper108_00425490(void *this,float param_2)

{
  undefined4 *puVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined1 auStack_10 [16];

  param_2 = param_2 + *(float *)((int)this + 0x1ddc);
  *(float *)((int)this + 0x1ddc) = param_2;
  if (*(char *)((int)this + 0x851) == '\0') {
    if (0.3 < param_2) {
      iVar2 = *(int *)((int)this + 0x1de0) + 1;
      *(int *)((int)this + 0x1de0) = iVar2;
      if (299 < iVar2) {
        *(undefined4 *)((int)this + 0x1de0) = 0;
      }
      puVar3 = (undefined4 *)(**(code **)(*(int *)((int)this + 0xc0) + 0x310))(auStack_10);
      puVar1 = (undefined4 *)(*(int *)((int)this + 0x1de0) * 0x10 + 0xb1c + (int)this);
      *puVar1 = *puVar3;
      puVar1[1] = puVar3[1];
      puVar1[2] = puVar3[2];
      puVar1[3] = puVar3[3];
      *(float *)((int)this + 0x1ddc) = *(float *)((int)this + 0x1ddc) - 0.3;
      return;
    }
  }
  else if (0.05 < param_2) {
    iVar2 = *(int *)((int)this + 0x1de0) + -1;
    *(int *)((int)this + 0x1de0) = iVar2;
    *(float *)((int)this + 0x1ddc) = param_2 - 0.05;
    if (0 < iVar2) {
      iVar2 = iVar2 * 0x10;
      (**(code **)(*(int *)((int)this + 0xc0) + 0x314))
                (*(undefined4 *)(iVar2 + 0xb1c + (int)this),
                 *(undefined4 *)((int)this + iVar2 + 0xb20),
                 *(undefined4 *)((int)this + iVar2 + 0xb24),
                 *(undefined4 *)((int)this + iVar2 + 0xb28));
      return;
    }
    *(undefined1 *)((int)this + 0x851) = 0;
    return;
  }
  return;
}


```

## JimmyActionHelper109_004255c0 @ 004255c0

```c

void __thiscall JimmyActionHelper109_004255c0(void *this,float param_2)

{
  int *piVar1;
  int iVar2;
  char cVar3;
  float10 fVar4;

  if (*(char *)((int)this + 0x1ded) == '\0') {
    if (0.0 < *(float *)((int)this + 0x1e60)) {
      *(float *)((int)this + 0x1e60) = *(float *)((int)this + 0x1e60) - param_2;
    }
    if (*(char *)((int)this + 0x1e03) != '\0') {
      fVar4 = (float10)param_2 + (float10)*(float *)((int)this + 0x1de8);
      *(float *)((int)this + 0x1de8) = (float)fVar4;
      if (fVar4 <= (float10)*(float *)((int)this + 0x1e2c)) {
        fVar4 = (float10)fsin(fVar4 * (float10)20.0);
        fVar4 = (fVar4 + (float10)1.0) * (float10)0.5;
      }
      else {
        fVar4 = (float10)1.0;
        *(undefined1 *)((int)this + 0x1e03) = 0;
      }
      iVar2 = *(int *)((int)this + 0x5c0);
      *(undefined4 *)(iVar2 + 0x4c) = 0x3f800000;
      *(float *)(iVar2 + 0x50) = (float)fVar4;
      *(float *)(iVar2 + 0x54) = (float)fVar4;
      *(undefined4 *)(iVar2 + 0x58) = 0x3f800000;
    }
    if (*(char *)((int)this + 0x1dec) != '\0') {
      fVar4 = (float10)param_2 + (float10)*(float *)((int)this + 0x1de8);
      *(float *)((int)this + 0x1de8) = (float)fVar4;
      if (fVar4 <= (float10)4.0) {
        fVar4 = (float10)fsin(fVar4 * (float10)20.0);
        fVar4 = (fVar4 + (float10)1.0) * (float10)0.5;
      }
      else {
        fVar4 = (float10)1.0;
        *(undefined1 *)((int)this + 0x1dec) = 0;
      }
      iVar2 = *(int *)((int)this + 0x5c0);
      *(float *)(iVar2 + 0x4c) = (float)fVar4;
      *(undefined4 *)(iVar2 + 0x50) = 0x3f800000;
      *(float *)(iVar2 + 0x54) = (float)fVar4;
      *(undefined4 *)(iVar2 + 0x58) = 0x3f800000;
    }
  }
  else {
    piVar1 = (int *)((int)this + 0xc0);
    *(float *)((int)this + 0x1de8) = param_2 + *(float *)((int)this + 0x1de8);
    (**(code **)(*(int *)((int)this + 0xc0) + 0x410))();
    fVar4 = (float10)fsin((float10)*(float *)((int)this + 0x1de8) * (float10)60.0);
    (**(code **)(*(int *)this + 0xf4))
              (*(undefined4 *)((int)this + 0x57c),
               (fVar4 + (float10)1.0) * (float10)0.5 <= (float10)0.5);
    if (1.0 < *(float *)((int)this + 0x1de8)) {
      (**(code **)(*(int *)this + 0xf4))(*(undefined4 *)((int)this + 0x57c),0);
      *(undefined4 *)((int)this + 0x1de8) = 0;
      *(undefined4 *)((int)this + 0x1e60) = 0x3f666666;
      *(undefined1 *)((int)this + 0x1ded) = 0;
      *(undefined4 *)((int)this + 0x1df4) = 0x42c80000;
      *(float *)((int)this + 0x1df0) = *(float *)((int)this + 0x1df0) * 7000.0;
      *(float *)((int)this + 0x1df8) = *(float *)((int)this + 0x1df8) * 7000.0;
      (**(code **)(*piVar1 + 0x2c0))
                (*(undefined4 *)((int)this + 0x1df0),*(undefined4 *)((int)this + 0x1df4),
                 *(undefined4 *)((int)this + 0x1df8),*(undefined4 *)((int)this + 0x1dfc));
      if (DAT_005099e4 == (-(uint)(this != (void *)0x0) & (uint)piVar1)) {
        cVar3 = (**(code **)(*piVar1 + 0x218))();
        if (cVar3 != '\0') {
          (**(code **)(*(int *)this + 0xe0))(&DAT_004ed040,1);
          return;
        }
        (**(code **)(*(int *)this + 0xe0))(&DAT_004ef3e8,1);
        return;
      }
    }
  }
  return;
}


```

## JimmyActionHelper110_00426e40 @ 00426e40

```c

void __thiscall JimmyActionHelper110_00426e40(void *this)

{
  int *piVar1;
  char cVar2;
  short sVar3;
  int iVar4;
  undefined4 *puVar5;
  float *pfVar6;
  float *pfVar7;
  int iVar8;
  undefined4 uVar9;
  uint uVar10;
  uint uVar11;
  int *piVar12;
  char *pcVar13;
  char *pcVar14;
  int *piVar15;
  short sStack_e6;
  int *piStack_e4;
  float fStack_e0;
  float fStack_dc;
  float fStack_d8;
  float fStack_d4;
  float fStack_d0;
  undefined4 uStack_cc;
  float fStack_c8;
  undefined4 uStack_c4;
  undefined4 uStack_c0;
  undefined4 uStack_bc;
  undefined4 uStack_b8;
  undefined4 uStack_b4;
  undefined4 uStack_b0;
  undefined4 uStack_ac;
  int *piStack_a8;
  short asStack_a2 [81];

  piStack_a8 = this;
  sVar3 = FUN_00403950();
  if ((sVar3 == 1) && (DAT_004f0588 == -1)) {
    piVar1 = *(int **)((int)this + 0xa10);
    if (piVar1 != (int *)0x0) {
      if (piVar1[0x1c] != 0) {
        piVar1[0x1c] = 1;
        (**(code **)(*piVar1 + 0x58))();
      }
      piVar1 = (int *)((int)this + 0xc0);
      iVar8 = *(int *)(*(int *)((int)this + 0xa10) + 0xc0);
      (**(code **)(*(int *)((int)this + 0xc0) + 0x310))();
      (**(code **)(*piVar1 + 0x310))();
      (**(code **)(*piVar1 + 0x310))();
      (**(code **)(iVar8 + 0x318))();
      iVar8 = *(int *)(*(int *)((int)this + 0xa10) + 0xc0);
      (**(code **)(*piVar1 + 0x328))();
      iVar4 = (**(code **)(*piVar1 + 0x328))();
      puVar5 = (undefined4 *)
               (**(code **)(*piVar1 + 0x328))(&stack0xffffff08,*(undefined4 *)(iVar4 + 4));
      (**(code **)(iVar8 + 0x330))(*puVar5);
    }
    FUN_0042a920();
    piStack_e4 = (int *)*DAT_0050999c;
    if (piStack_e4 != DAT_0050999c) {
      do {
        piVar1 = (int *)piStack_e4[2];
        if ((piVar1 != (int *)0x0) && (*(char *)((int)piVar1 + 0x13e) != '\0')) {
          cVar2 = (**(code **)(*piVar1 + 0x18))();
          if ((cVar2 != '\0') && ((char)piVar1[0x226] != '\0')) {
            pfVar6 = (float *)(**(code **)(*piVar1 + 0x310))();
            piVar12 = (int *)((int)this + 0xc0);
            pfVar7 = (float *)(**(code **)(*(int *)((int)this + 0xc0) + 0x310))();
            fStack_e0 = *pfVar6 - *pfVar7;
            fStack_dc = pfVar6[1] - pfVar7[1];
            fStack_d8 = pfVar6[2] - pfVar7[2];
            fStack_d4 = 0.0;
            iVar8 = (**(code **)(*piVar1 + 0x310))();
            iVar4 = (**(code **)(*piVar12 + 0x310))();
            if (*(float *)(iVar4 + 4) - 150.0 < *(float *)(iVar8 + 4)) {
              iVar8 = (**(code **)(*piVar1 + 0x310))();
              iVar4 = (**(code **)(*piVar12 + 0x310))();
              if ((*(float *)(iVar8 + 4) < *(float *)(iVar4 + 4) + 60.0) &&
                 (SQRT(fStack_d8 * fStack_d8 + fStack_d4 * fStack_d4 + fStack_d0 * fStack_d0) <
                  1500.0)) {
                OMedia3DVector::angles((OMedia3DVector *)&fStack_d8,asStack_a2,&sStack_e6);
                iVar8 = (**(code **)(*piVar12 + 0x328))();
                fStack_dc = (float)(int)sStack_e6;
                if (*(float *)(iVar8 + 4) - 5.0 < (float)(int)fStack_dc * 0.021972656) {
                  iVar8 = (**(code **)(*piVar12 + 0x328))();
                  fStack_dc = (float)(int)sStack_e6;
                  if (((float)(int)fStack_dc * 0.021972656 < *(float *)(iVar8 + 4) + 5.0) &&
                     ((char)piVar1[0x22c] == '\0')) {
                    uStack_bc = 0x3f800000;
                    uStack_ac = 0x3f800000;
                    puVar5 = (undefined4 *)(**(code **)(*piVar12 + 0x310))();
                    uStack_cc = *puVar5;
                    uStack_c4 = puVar5[2];
                    fStack_c8 = (float)puVar5[1] + 150.0;
                    uStack_c0 = puVar5[3];
                    puVar5 = (undefined4 *)(**(code **)(*piVar12 + 900))();
                    uStack_b8 = *puVar5;
                    uStack_b4 = puVar5[1];
                    uStack_b0 = puVar5[2];
                    uStack_ac = puVar5[3];
                    FUN_00409f60();
                    FUN_00409f60(&fStack_c8);
                    cVar2 = FUN_0047c210();
                    if (cVar2 == '\0') {
                      (**(code **)(*(int *)this + 0x1dc))();
                      cVar2 = (**(code **)(*piVar1 + 0x18))();
                      if (cVar2 != '\0') {
                        uVar10 = 0xffffffff;
                        pcVar13 = s_RUNSHRUNK_004ef47c;
                        do {
                          pcVar14 = pcVar13;
                          if (uVar10 == 0) break;
                          uVar10 = uVar10 - 1;
                          pcVar14 = pcVar13 + 1;
                          cVar2 = *pcVar13;
                          pcVar13 = pcVar14;
                        } while (cVar2 != '\0');
                        uVar10 = ~uVar10;
                        piVar12 = (int *)(pcVar14 + -uVar10);
                        piVar15 = piVar1 + 0x1d8;
                        for (uVar11 = uVar10 >> 2; uVar11 != 0; uVar11 = uVar11 - 1) {
                          *piVar15 = *piVar12;
                          piVar12 = piVar12 + 1;
                          piVar15 = piVar15 + 1;
                        }
                        for (uVar10 = uVar10 & 3; uVar10 != 0; uVar10 = uVar10 - 1) {
                          *(char *)piVar15 = (char)*piVar12;
                          piVar12 = (int *)((int)piVar12 + 1);
                          piVar15 = (int *)((int)piVar15 + 1);
                        }
                        (**(code **)(piVar1[-0x30] + 0xe0))();
                        this = piStack_a8;
                      }
                      FUN_0040acc0();
                      FUN_00458a40();
                      if (*(int *)((int)this + 0xa1c) != 0) {
                        cVar2 = (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0x218))
                                          ();
                        if (cVar2 != '\0') {
                          (**(code **)(**(int **)((int)this + 0xa1c) + 0x15c))();
                          cVar2 = FUN_0047d890();
                          if (cVar2 == '\0') {
                            FUN_0040acc0();
                            uVar9 = FUN_00458b40();
                            *(undefined4 *)(*(int *)((int)this + 0xa1c) + 0x99c) = uVar9;
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
        piStack_e4 = (int *)*piStack_e4;
      } while (piStack_e4 != DAT_0050999c);
    }
  }
  return;
}


```

## JimmyActionHelper111_00427340 @ 00427340

```c

void __thiscall JimmyActionHelper111_00427340(void *this,float param_2)

{
  if (*(int *)((int)this + 0xa1c) != 0) {
    (**(code **)(*(int *)this + 0x1cc))(param_2 * -0.05);
  }
  return;
}


```

## JimmyActionHelper112_00426a70 @ 00426a70

```c

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 __thiscall JimmyActionHelper112_00426a70(void *this,float param_2)

{
  int *piVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  char cVar4;
  undefined4 uVar5;
  undefined4 *puVar6;
  int iVar7;
  int iVar8;
  float unaff_ESI;
  float unaff_EDI;
  undefined4 *puVar9;
  undefined1 *puStack_54;
  undefined4 auStack_34 [2];
  float fStack_2c;
  undefined1 auStack_28 [4];
  undefined4 uStack_24;
  undefined1 auStack_20 [32];

  auStack_34[0] = 0x3f800000;
  uStack_24 = 0x3f800000;
  if (DAT_004f0588 != 7) {
    return 0;
  }
  if (DAT_004f8210 == '\0') {
    if (*(int *)((int)this + 0xa3c) != -1) {
      puStack_54 = (undefined1 *)0x0;
      FUN_00458a00(*(int *)((int)this + 0xa3c));
      *(undefined4 *)((int)this + 0xa3c) = 0xffffffff;
    }
    piVar1 = (int *)((int)this + 0xc0);
    puStack_54 = auStack_20;
    (**(code **)(*(int *)((int)this + 0xc0) + 0x310))();
    puVar9 = &uStack_24;
    (**(code **)(*piVar1 + 900))(puVar9,0,0x45160000,0x45160000);
    FUN_00409f60(&stack0xffffffbc);
    FUN_00409f60(&puStack_54);
    cVar4 = FUN_0047c210();
    if (cVar4 != '\0') {
      if (*(int **)((int)this + 0xa1c) != (int *)0x0) {
        (**(code **)(**(int **)((int)this + 0xa1c) + 0x58))(1);
        (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0xd0))(0);
        (**(code **)(*(int *)(*(int *)((int)this + 0xa1c) + 0xc0) + 0x214))(0);
      }
      uVar3 = DAT_005cfc6c;
      uVar2 = DAT_005cfc68;
      uVar5 = DAT_005cfc64;
      iVar7 = (int)DAT_004f82cc;
      iVar8 = iVar7 * 0x10;
      *(undefined4 *)(&DAT_004f8214 + iVar8) = DAT_005cfc60;
      *(undefined4 *)(&DAT_004f8218 + iVar8) = uVar5;
      *(undefined4 *)(&DAT_004f821c + iVar8) = uVar2;
      (&DAT_004f8220)[iVar7 * 4] = uVar3;
      iVar7 = DAT_004f82cc * 0x10;
      fStack_2c = *(float *)(&DAT_004f821c + iVar7) - unaff_ESI;
      DAT_004f82b8 = SQRT((*(float *)(&DAT_004f8214 + iVar7) - (float)puStack_54) *
                          (*(float *)(&DAT_004f8214 + iVar7) - (float)puStack_54) +
                          (*(float *)(&DAT_004f8218 + iVar7) - unaff_EDI) *
                          (*(float *)(&DAT_004f8218 + iVar7) - unaff_EDI) + fStack_2c * fStack_2c) -
                     100.0;
      if (_DAT_004f82b4 == 0.0) {
        if (*(int *)((int)this + 0xb0c) != 0) {
          (**(code **)(**(int **)(*(int *)((int)this + 0xb0c) + 0x57c) + 0x34))
                    (0x3f800000,0x3f800000,DAT_004f82b8 * 0.0016666667);
        }
        _DAT_004f82b4 = DAT_004f82b8;
      }
      DAT_004f8210 = '\x01';
      *(undefined1 *)((int)this + 0x864) = 1;
      *(undefined2 *)((int)this + 0x7e8) = 0;
      if (*(int *)((int)this + 0x7f8) != 0) {
        *(undefined4 *)(*(int *)((int)this + 0x7f8) + 0x6c0) = 0;
      }
      FUN_00458980(0xffffffff,0x1f,0);
      if (*(int *)((int)this + 0x1e04) != -1) {
        FUN_00458a00(*(int *)((int)this + 0x1e04),0);
        *(undefined4 *)((int)this + 0x1e04) = 0xffffffff;
      }
      if (*(int *)((int)this + 0x1e04) == -1) {
        uVar5 = FUN_00458980(0xffffffff,0x22,1);
        *(undefined4 *)((int)this + 0x1e04) = uVar5;
      }
      _DAT_004f82bc = 0;
      _DAT_004f82c0 = 0;
      iVar7 = *piVar1;
      iVar8 = (**(code **)(iVar7 + 700))(auStack_34);
      puVar6 = (undefined4 *)
               (**(code **)(*piVar1 + 700))(auStack_28,0x42c80000,*(undefined4 *)(iVar8 + 8));
      (**(code **)(iVar7 + 0x2c4))(*puVar6);
      _DAT_004f82c4 = 0;
      _DAT_004f82c8 = (*(float *)(&DAT_004f8218 + DAT_004f82cc * 0x10) - (float)puVar9) - 200.0;
      piVar1 = *(int **)((int)this + 0xb0c);
      *(undefined4 *)((int)this + 0x7cc) = 0x43c80000;
      *(undefined4 *)((int)this + 2000) = 0xc4228000;
      *(undefined4 *)((int)this + 0x854) = 0x3f4ccccd;
      *(undefined4 *)((int)this + 0x858) = 0x3f800000;
      *(undefined4 *)((int)this + 0x85c) = 0x3f4ccccd;
      if ((piVar1 != (int *)0x0) && (piVar1[0x1c] != 0)) {
        piVar1[0x1c] = 1;
        (**(code **)(*piVar1 + 0x58))(0);
        return 1;
      }
    }
  }
  else if (DAT_004f82b8 <= 250.0) {
    if (*(int *)((int)this + 0x1e04) != -1) {
      puStack_54 = (undefined1 *)0x0;
      FUN_00458a00(*(int *)((int)this + 0x1e04));
      *(undefined4 *)((int)this + 0x1e04) = 0xffffffff;
    }
  }
  else {
    DAT_004f82b8 = DAT_004f82b8 - param_2 * 250.0;
    if (*(int *)((int)this + 0x1e04) == -1) {
      puStack_54 = (undefined1 *)0x1;
      uVar5 = FUN_00458980(0xffffffff,0x22);
      *(undefined4 *)((int)this + 0x1e04) = uVar5;
      return 1;
    }
  }
  return 1;
}


```

## JimmyActionHelper113_004269d0 @ 004269d0

```c

undefined4 __thiscall JimmyActionHelper113_004269d0(void *this)

{
  undefined4 uVar1;
  undefined1 auStack_20 [8];
  undefined4 uStack_18;
  int iStack_14;
  undefined4 uStack_10;
  undefined1 uStack_5;
  undefined1 *puStack_4;

  if ((DAT_004f83d4 <= 0.0) && (*(char *)((int)this + 0x9f8) == '\0')) {
    if (*(int *)((int)this + 0xa34) != -1) {
      uStack_10 = 0;
      uStack_18 = 0x426a06;
      iStack_14 = *(int *)((int)this + 0xa34);
      FUN_00458a00();
      *(undefined4 *)((int)this + 0xa34) = 0xffffffff;
    }
    if (*(int *)((int)this + 0xa48) == -1) {
      uStack_10 = 1;
      puStack_4 = auStack_20;
      FUN_0040acc0(s_not_available_004ef410,&uStack_5);
      uVar1 = FUN_00458b40(0xffffffff);
      *(undefined4 *)((int)this + 0xa48) = uVar1;
    }
    if (*(int **)((int)this + 0xa10) != (int *)0x0) {
      uStack_10 = 1;
      iStack_14 = 0x426a57;
      (**(code **)(**(int **)((int)this + 0xa10) + 0x58))();
    }
    return 1;
  }
  return 0;
}


```

## JimmyActionHelper114_00424e80 @ 00424e80

```c

void __thiscall JimmyActionHelper114_00424e80(void *this,float param_2)

{
  float fVar1;
  char cVar2;
  uint uVar3;
  C3DPlayer *this_00;
  bool bVar4;

  if (DAT_005099e4 == 0) {
    (**(code **)(**(int **)((int)this + 0xa18) + 0x488))();
    return;
  }
  if (DAT_004f0588 == 2) {
    (**(code **)(**(int **)((int)this + 0xa18) + 0x484))();
    cVar2 = C3DPlayer::vfunc_04_087(this_00);
    if ((cVar2 != '\0') && (cVar2 = FUN_0042a730(), cVar2 == '\0')) {
      if (DAT_004f8182 != '\0') {
        return;
      }
      if ((*(float *)((int)this + 0x1e1c) == 0.0) && (*(int *)((int)this + 0x1e20) == 0x40e00000)) {
        if ((DAT_004f83d4 <= 2.0) && (*(char *)((int)this + 0x9f8) == '\0')) {
          return;
        }
        FUN_0042a920(0xc0000000);
        *(undefined4 *)((int)this + 0x1e1c) = 0x40e00000;
        (**(code **)(*(int *)this + 0xfc))(0,6,1);
        (**(code **)(*(int *)this + 0x114))(0x3f333333);
        *(undefined1 *)((int)this + 0x55d) = 0;
        *(undefined4 *)((int)this + 0x1e20) = 0;
      }
    }
    if ((*(float *)((int)this + 0x1e1c) == 0.0) &&
       ((0.0 < DAT_004f83d4 || (*(char *)((int)this + 0x9f8) != '\0')))) {
      fVar1 = param_2 + *(float *)((int)this + 0x1e20);
      *(float *)((int)this + 0x1e20) = fVar1;
      if (*(int *)((int)this + 0xa18) != 0) {
        FUN_004037f0(fVar1 * 0.14285715 * 100.0);
      }
      if (7.0 <= *(float *)((int)this + 0x1e20)) {
        if (*(int **)((int)this + 0xa18) != (int *)0x0) {
          (**(code **)(**(int **)((int)this + 0xa18) + 0x490))();
        }
        *(undefined4 *)((int)this + 0x1e20) = 0x40e00000;
      }
    }
    if (0.0 < *(float *)((int)this + 0x1e1c)) {
      param_2 = *(float *)((int)this + 0x1e1c) - param_2;
      *(float *)((int)this + 0x1e1c) = param_2;
      param_2 = param_2 * 0.14285715 * 100.0;
      if (param_2 <= 0.0) {
        param_2 = 0.0;
      }
      if (*(int *)((int)this + 0xa18) != 0) {
        FUN_004037f0(param_2);
      }
      if (0.0 < *(float *)((int)this + 0x1e1c)) {
        if (*(float *)((int)this + 0x1e1c) <= 3.0) {
          uVar3 = __ftol();
          uVar3 = uVar3 & 0x80000001;
          bVar4 = uVar3 == 0;
          if ((int)uVar3 < 0) {
            bVar4 = (uVar3 - 1 | 0xfffffffe) == 0xffffffff;
          }
          if (!bVar4) {
            (**(code **)(*(int *)this + 0xfc))(0,6,7);
            return;
          }
          (**(code **)(*(int *)this + 0xfc))(0,6,1);
        }
      }
      else {
        *(undefined4 *)((int)this + 0x1e1c) = 0;
        (**(code **)(*(int *)this + 0xfc))(0,6,7);
        (**(code **)(*(int *)this + 0x114))(0);
        *(undefined1 *)((int)this + 0x55d) = 1;
        if (*(int **)((int)this + 0xa18) != (int *)0x0) {
          (**(code **)(**(int **)((int)this + 0xa18) + 0x48c))();
          return;
        }
      }
    }
  }
  return;
}


```

## JimmyActionHelper115_0042aa20 @ 0042aa20

```c

void __thiscall JimmyActionHelper115_0042aa20(void *this,float param_2)

{
  param_2 = param_2 + *(float *)((int)this + 0xa28);
  *(float *)((int)this + 0xa28) = param_2;
  if (100.0 < param_2) {
    *(undefined4 *)((int)this + 0xa28) = 0x42c80000;
  }
  if (*(float *)((int)this + 0xa28) < 0.0) {
    *(undefined4 *)((int)this + 0xa28) = 0;
  }
  return;
}


```

## JimmyActionHelper116_004289a0 @ 004289a0

```c

void __thiscall JimmyActionHelper116_004289a0(void *this,undefined2 param_2)

{
  int iVar1;
  undefined4 uVar2;

  switch(param_2) {
  case 0:
    if (*(int *)(DAT_00509948 + 0x490) == 0x4c563142) {
      return;
    }
    if (*(int **)((int)this + 0xa18) != (int *)0x0) {
      (**(code **)(**(int **)((int)this + 0xa18) + 0x4b8))(0x94,1,0,0xffffffff);
    }
    (**(code **)(*(int *)this + 0x168))
              (s_level1b_gam_004ef524,s_PHONEBOOTH_004ec788,(int)this + 0x955,(int)this + 0x724);
    break;
  case 1:
    if (*(int *)(DAT_00509948 + 0x490) == 0x4c455631) {
      return;
    }
    if (*(int **)((int)this + 0xa18) != (int *)0x0) {
      (**(code **)(**(int **)((int)this + 0xa18) + 0x4b8))(0x95,1,0,0xffffffff);
    }
    (**(code **)(*(int *)this + 0x168))
              (s_level1_gam_004ef518,s_PHONEBOOTH_004ec788,(int)this + 0x955,(int)this + 0x724);
    break;
  case 2:
    if (*(int *)(DAT_00509948 + 0x490) == 0x4c455632) {
      return;
    }
    if (*(int **)((int)this + 0xa18) != (int *)0x0) {
      (**(code **)(**(int **)((int)this + 0xa18) + 0x4b8))(0x99,1,0,0xffffffff);
    }
    (**(code **)(*(int *)this + 0x168))
              (s_level2_gam_004ef50c,s_PHONEBOOTH_004ec788,(int)this + 0x955,(int)this + 0x724);
    break;
  case 3:
    if (*(int *)(DAT_00509948 + 0x490) == 0x4c455634) {
      return;
    }
    if (*(int **)((int)this + 0xa18) != (int *)0x0) {
      (**(code **)(**(int **)((int)this + 0xa18) + 0x4b8))(0x93,1,0,0xffffffff);
    }
    (**(code **)(*(int *)this + 0x168))
              (s_level4_gam_004ef500,s_PHONEBOOTH_004ec788,(int)this + 0x955,(int)this + 0x724);
    break;
  case 4:
    if (*(int *)(DAT_00509948 + 0x490) == 0x4c455633) {
      return;
    }
    if (*(int **)((int)this + 0xa18) != (int *)0x0) {
      (**(code **)(**(int **)((int)this + 0xa18) + 0x4b8))(0x97,1,0,0xffffffff);
    }
    (**(code **)(*(int *)this + 0x168))
              (s_level3_gam_004ef4f4,s_PHONEBOOTH_004ec788,(int)this + 0x955,(int)this + 0x724);
    break;
  case 5:
    if (*(int *)(DAT_00509948 + 0x490) == 0x4c563443) {
      return;
    }
    if (*(int **)((int)this + 0xa18) != (int *)0x0) {
      (**(code **)(**(int **)((int)this + 0xa18) + 0x4b8))(0x96,1,0,0xffffffff);
    }
    (**(code **)(*(int *)this + 0x168))
              (s_level4c_gam_004ef4e8,s_PHONEBOOTH_004ec788,(int)this + 0x955,(int)this + 0x724);
    break;
  case 6:
    if (*(int *)(DAT_00509948 + 0x490) == 0x4c455635) {
      return;
    }
    if (*(int **)((int)this + 0xa18) != (int *)0x0) {
      (**(code **)(**(int **)((int)this + 0xa18) + 0x4b8))(0x9a,1,0,0xffffffff);
    }
    (**(code **)(*(int *)this + 0x168))
              (s_level5_gam_004ef4dc,s_PHONEBOOTH_004ec788,(int)this + 0x955,(int)this + 0x724);
    break;
  case 7:
    if (*(int *)(DAT_00509948 + 0x490) == 0x4c563541) {
      return;
    }
    if (*(int **)((int)this + 0xa18) != (int *)0x0) {
      (**(code **)(**(int **)((int)this + 0xa18) + 0x4b8))(0x98,1,0,0xffffffff);
    }
    (**(code **)(*(int *)this + 0x168))
              (s_level5a_gam_004ef4c4,s_phonebooth_004ef4d0,(int)this + 0x955,(int)this + 0x724);
    break;
  case 8:
    if (*(int **)((int)this + 0xa18) != (int *)0x0) {
      (**(code **)(**(int **)((int)this + 0xa18) + 0x4b8))(0x92,1,0,0xffffffff);
    }
    (**(code **)(*(int *)this + 0x168))
              (s_level6_gam_004ef4b8,s_PHONEBOOTH_004ec788,(int)this + 0x955,(int)this + 0x724);
    break;
  case 10:
  case 0xc:
    (**(code **)(**(int **)((int)this + 0xa18) + 0x4e8))(0xffffffff);
    uVar2 = 0x91;
    iVar1 = **(int **)((int)this + 0xa18);
    goto LAB_00428cfd;
  case 0xb:
    (**(code **)(**(int **)((int)this + 0xa18) + 0x4e8))(0xffffffff);
    uVar2 = 0x90;
    iVar1 = **(int **)((int)this + 0xa18);
LAB_00428cfd:
    (**(code **)(iVar1 + 0x4e8))(uVar2);
  }
  if (*(int **)((int)this + 0xa10) != (int *)0x0) {
    (**(code **)(**(int **)((int)this + 0xa10) + 0x58))(1);
  }
  return;
}


```

## JimmyActionHelper119_0042ac10 @ 0042ac10

```c

void __thiscall JimmyActionHelper119_0042ac10(void *this,int param_2)

{
  int iVar1;
  int *piVar2;

  CGameObject::vfunc_00_013(this);
  if (param_2 == 0) {
    piVar2 = (int *)0x0;
  }
  else {
    piVar2 = (int *)(param_2 + -0xc0);
  }
  (**(code **)(*piVar2 + 0xe0))(s_SHRINK_004ef71c,0);
  if (param_2 == 0) {
    piVar2 = (int *)0x0;
  }
  else {
    piVar2 = (int *)(param_2 + -0xc0);
  }
  (**(code **)(*piVar2 + 0x124))(5);
  if (param_2 == 0) {
    iVar1 = 0;
  }
  else {
    iVar1 = param_2 + -0xc0;
  }
  *(undefined4 *)(iVar1 + 0x974) = 0x41f00000;
  if (param_2 == 0) {
    iVar1 = 0;
  }
  else {
    iVar1 = param_2 + -0xc0;
  }
  *(undefined1 *)(iVar1 + 0x970) = 1;
  iVar1 = __strcmpi((char *)(param_2 + 0x3a0),s_LIBBYPLANT_004ef2bc);
  if (iVar1 == 0) {
    iVar1 = FindObjectByTag_00474070();
    if ((iVar1 != 0) && (piVar2 = (int *)(iVar1 + -0xc0), piVar2 != (int *)0x0)) {
      FUN_0042adc0(100);
      (**(code **)(*piVar2 + 0x124))(0);
      *(undefined4 *)(iVar1 + 0x604) = 0x43200000;
      (**(code **)(*piVar2 + 0x120))(iVar1 + 0x760,&DAT_004eca54);
      (**(code **)(*piVar2 + 0x120))(iVar1 + 0x7d8,&DAT_004eca54);
      iVar1 = FUN_0045fea0(s_SCENE_004ed220);
      if (iVar1 == 0xfa) {
        FUN_0045f990(s_SCENE_004ed220,0xff);
        return;
      }
    }
  }
  else {
    iVar1 = __strcmpi((char *)(param_2 + 0x3a0),s_C3DDINO_004ee208);
    if (iVar1 == 0) {
      iVar1 = FUN_0045fea0(&DAT_004ed8f8);
      if (iVar1 < 10) {
        FUN_0045f990(&DAT_004ed8f8,10);
        FUN_0042adc0(300);
        return;
      }
    }
    else {
      FUN_0042adc0(10);
      if (param_2 != 0) {
        *(undefined4 *)(param_2 + 0x88c) = 0x3f800000;
        return;
      }
      uRam0000094c = 0x3f800000;
    }
  }
  return;
}


```

## JimmyActionHelper125_00428870 @ 00428870

```c

void __thiscall JimmyActionHelper125_00428870(void *this,undefined2 param_2)

{
  char cVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined4 uVar4;

  switch(param_2) {
  case 1:
    FUN_00406f90(7);
    DAT_004f8182 = 1;
    return;
  default:
    goto switchD_0042888d_caseD_2;
  case 3:
    (**(code **)(*(int *)this + 0x1f8))(4);
    iVar2 = 1;
    puVar3 = &DAT_004f82e4;
    do {
      FUN_0047f967(&stack0xffffffac,s_JimmyGame_d_tsk_004ef4a8,iVar2);
      cVar1 = FUN_00460210(&stack0xffffffac);
      uVar4 = DAT_004fc690;
      if (cVar1 == '\0') {
        *puVar3 = 0xffffffff;
      }
      else {
        *puVar3 = DAT_004fc68c;
        puVar3[4] = uVar4;
      }
      puVar3 = puVar3 + 5;
      iVar2 = iVar2 + 1;
    } while ((int)puVar3 < 0x4f8399);
    *(undefined1 *)((int)this + 0x1e25) = 1;
    return;
  case 4:
    (**(code **)(*(int *)this + 0x1f8))(5);
    *(undefined1 *)((int)this + 0x1e25) = 1;
    return;
  case 6:
    iVar2 = *(int *)this;
    uVar4 = 6;
    break;
  case 0xc:
    iVar2 = *(int *)this;
    uVar4 = 0;
    break;
  case 0xd:
    (**(code **)(*(int *)this + 0x1f8))(2);
    *(undefined1 *)((int)this + 0x1e25) = 1;
    return;
  }
  (**(code **)(iVar2 + 0x1f8))(uVar4);
  *(undefined1 *)((int)this + 0x1e25) = 1;
switchD_0042888d_caseD_2:
  return;
}


```

## JimmyActionHelper127_00425b20 @ 00425b20

```c

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall JimmyActionHelper127_00425b20(void *this)

{
  if (DAT_004f8181 != '\0') {
    CGameObject::vfunc_00_013(this);
    DAT_004f8181 = '\0';
    if ((0.0 < _DAT_004eefd0) || (0.0 < DAT_004eefc8)) {
      (**(code **)(**(int **)((int)this + 0xa18) + 0x4b4))(1);
    }
    if ((DAT_004f0588 == 2) || (2 < *(short *)((int)this + 0x7e8))) {
      (**(code **)(**(int **)((int)this + 0xa18) + 0x484))();
    }
    (**(code **)(*DAT_00509948 + 0x168))(0);
    *(undefined1 *)((int)this + 0x55f) = 0;
    (**(code **)(**(int **)((int)this + 0xa18) + 0x45c))();
    if (*(short *)(*(int *)((int)this + 0xa18) + 0x4d4) != 2) {
      ShowCursor(0);
    }
    *(undefined4 *)(*(int *)((int)this + 0xa18) + 0x4c4) = 0;
    *(undefined1 *)(*(int *)((int)this + 0xa18) + 0x4c0) = 1;
    DAT_004f8434 = 1;
    (**(code **)(*(int *)((int)this + 0xc0) + 0x214))(1);
    DAT_004f8182 = 0;
    (**(code **)(*(int *)((int)this + 0xc0) + 0x448))(0);
    (**(code **)(**(int **)((int)this + 0xa18) + 0x4e8))(0xffffffff);
  }
  return;
}


```

## JimmyActionHelper129_00429c10 @ 00429c10

```c

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall JimmyActionHelper129_00429c10(void *this)

{
  if (DAT_004f8210 != '\0') {
    DAT_004f8210 = '\0';
    *(undefined1 *)((int)this + 0x864) = 0;
    *(undefined4 *)((int)this + 0x854) = 0x3fcccccd;
    *(undefined4 *)((int)this + 0x858) = 0x3f800000;
    *(undefined4 *)((int)this + 0x85c) = 0x3fcccccd;
    *(undefined4 *)((int)this + 0x7c8) = 0;
    *(undefined4 *)((int)this + 0x7cc) = 0x42480000;
    *(undefined4 *)((int)this + 2000) = 0xc3af0000;
    *(undefined4 *)((int)this + 0x7d8) = 0;
    *(undefined4 *)((int)this + 0x7dc) = 0x43020000;
    *(undefined4 *)((int)this + 0x7e0) = 0x42c80000;
    _DAT_004f82bc = _DAT_004f82bc * 0.96;
    _DAT_004f82c0 = _DAT_004f82c0 * 0.96;
    (**(code **)(*(int *)((int)this + 0xc0) + 0x2c4))(0,0,0);
    (**(code **)(*(int *)this + 0xe0))(&DAT_004ef3e8,1);
    if (*(int *)((int)this + 0x1e04) != -1) {
      FUN_00458a00(*(int *)((int)this + 0x1e04),0);
      *(undefined4 *)((int)this + 0x1e04) = 0xffffffff;
    }
    if (*(int **)((int)this + 0xb0c) != (int *)0x0) {
      (**(code **)(**(int **)((int)this + 0xb0c) + 0x58))(1);
    }
  }
  return;
}


```

## JimmyActionHelper130_00424d10 @ 00424d10

```c

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall JimmyActionHelper130_00424d10(undefined4 param_1,float param_2)

{
  undefined4 uVar1;

  if (0.0 < _DAT_004f83c8) {
    _DAT_004f83c8 = _DAT_004f83c8 - param_2;
    if (_DAT_004f83c8 <= 0.0) {
      _DAT_004f83c8 = 0.0;
    }
    if ((DAT_005099e4 != 0) && (DAT_004f8198 == '\0')) {
      if (0.2 < _DAT_004f83c8) {
        FUN_00468660(300,0xd2,&LAB_00424947,&DAT_004ec794,DAT_004f83c4);
        return;
      }
      uVar1 = __ftol(&LAB_00424947,&DAT_004ec794,DAT_004f83c4);
      uVar1 = __ftol(uVar1);
      FUN_00468660(uVar1);
      return;
    }
    if (0.2 < _DAT_004f83c8) {
      FUN_00468660(0x1c2,0xd2,&LAB_00424947,&DAT_004ec794,DAT_004f83c4);
      return;
    }
    uVar1 = __ftol(&LAB_00424947,&DAT_004ec794,DAT_004f83c4);
    uVar1 = __ftol(uVar1);
    FUN_00468660(uVar1);
  }
  return;
}


```

## JimmyActionHelper78_00429970 @ 00429970

```c

void __thiscall JimmyActionHelper78_00429970(void *this)

{
  undefined4 uVar1;

  CGameObject::vfunc_00_022(this);
  (**(code **)(**(int **)((int)this + 0xa18) + 0x484))();
  if (*(int *)((int)this + 0xa3c) == -1) {
    uVar1 = FUN_00458980(0xffffffff,1,1);
    *(undefined4 *)((int)this + 0xa3c) = uVar1;
  }
  return;
}


```

## JimmyActionHelper80_00429910 @ 00429910

```c

void __thiscall JimmyActionHelper80_00429910(void *this)

{
  CGameObject::vfunc_00_013(this);
  *(undefined1 *)((int)this + 0x1e64) = 0;
  *(undefined4 *)((int)this + 0x1e68) = 0;
  if (DAT_004f0588 != 2) {
    (**(code **)(**(int **)((int)this + 0xa18) + 0x488))();
  }
  if (*(int *)((int)this + 0xa3c) != -1) {
    FUN_00458a00(*(int *)((int)this + 0xa3c),0);
    *(undefined4 *)((int)this + 0xa3c) = 0xffffffff;
  }
  return;
}


```

## JimmyActionHelper85_0042a720 @ 0042a720

```c

void __thiscall JimmyActionHelper85_0042a720(void *this)

{
  CGameObject::vfunc_00_013(*(CGameObject **)((int)this + 0xa14));
  return;
}


```

## JimmyGadgetControllerCtor_00401430 @ 00401430

```c

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void * __thiscall JimmyGadgetControllerCtor_00401430(void *this)

{
  char cVar1;
  undefined4 uVar2;
  OMediaCanvasElement *pOVar3;
  int *piVar4;
  CGameObject *this_00;
  CGameObject *this_01;
  uint uVar5;
  int iVar6;
  CGameObject *this_02;
  CGameObject *this_03;
  CGameObject *this_04;
  float unaff_ESI;
  undefined2 uVar7;
  int **ppiVar8;
  undefined4 *puVar9;
  char *pcVar10;
  char *pcVar11;
  void *pvStack_c;
  undefined1 *puStack_8;
  int local_4;

  local_4 = 0xffffffff;
  puStack_8 = &LAB_00487e6b;
  pvStack_c = ExceptionList;
  ExceptionList = &pvStack_c;
  FUN_004586f0();
  local_4 = 0;
  *(undefined ***)this = &C2DInGameMenu::vftable;
  CGameObject::vfunc_00_005(this);
  CGameObject::vfunc_00_013(this_00);
  CGameObject::vfunc_00_254(this_01);
  CLocalGameObject::vfunc_00_007(this);
  FUN_004783c0();
  CGameObject::vfunc_00_064(this);
  *(undefined4 *)((int)this + 0x404) = 0x324d454e;
  *(undefined1 *)((int)this + 0x4be) = 0;
  _DAT_004f8030 = 0;
  _DAT_004f8034 = 0;
  _DAT_004f8038 = 0;
  uVar5 = 0xffffffff;
  _DAT_004f803c = 0;
  *(undefined4 *)((int)this + 0x4b4) = 0;
  *(undefined4 *)((int)this + 0x4b8) = 0;
  *(undefined1 *)((int)this + 0x4bf) = 0;
  *(undefined1 *)((int)this + 0x4e8) = 0;
  *(undefined4 *)((int)this + 0x50c) = 0;
  *(undefined4 *)((int)this + 0x510) = 0xffffffff;
  pcVar10 = s_omt_screens_omt_004ec54c;
  do {
    if (uVar5 == 0) break;
    uVar5 = uVar5 - 1;
    cVar1 = *pcVar10;
    pcVar10 = pcVar10 + 1;
  } while (cVar1 != '\0');
  FUN_004074b0(s_omt_screens_omt_004ec54c,~uVar5 - 1);
  uVar2 = FUN_00476170();
  *(undefined4 *)((int)this + 0x4b8) = uVar2;
  uVar2 = FUN_00476000(uVar2);
  *(undefined4 *)((int)this + 0x4b4) = uVar2;
  C2DInGameMenu::vfunc_00_299(this);
  C2DInGameMenu::vfunc_00_293(this);
  *(undefined2 *)((int)this + 0x4bc) = 3;
  *(undefined2 *)((int)this + 0x4c8) = 3;
  DAT_004f8040 = (int *)0x0;
  iVar6 = (-(uint)(DAT_00509a13 != '\0') & 6) + 1;
  pOVar3 = (OMediaCanvasElement *)FUN_00478990();
  local_4._0_1_ = 1;
  if (pOVar3 == (OMediaCanvasElement *)0x0) {
    DAT_004f8040 = (int *)0x0;
  }
  else {
    DAT_004f8040 = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
  }
  local_4 = (uint)local_4._1_3_ << 8;
  if (DAT_004f8040 != (int *)0x0) {
    (**(code **)(*DAT_004f8040 + 0x34))();
  }
  piVar4 = DAT_004f8040;
  DAT_004f8040[0x2d] = 6;
  piVar4[0x2e] = 7;
  (**(code **)(*DAT_004f8040 + 0xac))();
  C2DInGameMenu::vfunc_00_276(this);
  pOVar3 = (OMediaCanvasElement *)FUN_00478990();
  puStack_8._0_1_ = 2;
  if (pOVar3 == (OMediaCanvasElement *)0x0) {
    DAT_004f7ea4 = (int *)0x0;
  }
  else {
    DAT_004f7ea4 = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
  }
  puStack_8 = (undefined1 *)((uint)puStack_8._1_3_ << 8);
  if (DAT_004f7ea4 != (int *)0x0) {
    (**(code **)(*DAT_004f7ea4 + 0x34))();
  }
  piVar4 = DAT_004f7ea4;
  DAT_004f7ea4[0x2d] = 6;
  piVar4[0x2e] = iVar6;
  (**(code **)(*DAT_004f7ea4 + 0xac))();
  C2DInGameMenu::vfunc_00_276(this);
  pOVar3 = (OMediaCanvasElement *)FUN_00478990();
  pvStack_c._0_1_ = 3;
  if (pOVar3 == (OMediaCanvasElement *)0x0) {
    DAT_004f80fc = (int *)0x0;
  }
  else {
    DAT_004f80fc = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
  }
  pvStack_c = (void *)((uint)pvStack_c._1_3_ << 8);
  if (DAT_004f80fc != (int *)0x0) {
    (**(code **)(*DAT_004f80fc + 0x34))();
  }
  piVar4 = DAT_004f80fc;
  DAT_004f80fc[0x2d] = 6;
  piVar4[0x2e] = iVar6;
  (**(code **)(*DAT_004f80fc + 0xac))();
  C2DInGameMenu::vfunc_00_276(this);
  pOVar3 = (OMediaCanvasElement *)FUN_00478990();
  if (pOVar3 == (OMediaCanvasElement *)0x0) {
    DAT_004f7cb0 = (int *)0x0;
  }
  else {
    DAT_004f7cb0 = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
  }
  if (DAT_004f7cb0 != (int *)0x0) {
    (**(code **)(*DAT_004f7cb0 + 0x34))();
  }
  piVar4 = DAT_004f7cb0;
  DAT_004f7cb0[0x2d] = 6;
  piVar4[0x2e] = iVar6;
  (**(code **)(*DAT_004f7cb0 + 0xac))();
  C2DInGameMenu::vfunc_00_276(this);
  (**(code **)(*DAT_004f7cb0 + 0x58))(1);
  pOVar3 = (OMediaCanvasElement *)FUN_00478990(0xd0);
  if (pOVar3 == (OMediaCanvasElement *)0x0) {
    DAT_004f8028 = (int *)0x0;
  }
  else {
    DAT_004f8028 = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
  }
  if (DAT_004f8028 != (int *)0x0) {
    (**(code **)(*DAT_004f8028 + 0x34))(DAT_00509a34);
  }
  *(undefined4 *)((int)this + 0x4d0) = 0xffffffff;
  piVar4 = DAT_004f8028;
  DAT_004f8028[0x2d] = 6;
  piVar4[0x2e] = iVar6;
  (**(code **)(*DAT_004f8028 + 0xac))(0);
  C2DInGameMenu::vfunc_00_276(this);
  piVar4 = DAT_004f8028;
  DAT_004f8028[0x2b] = (int)(unaff_ESI * 30.0);
  piVar4[0x2c] = 0;
  (**(code **)(*DAT_004f8028 + 0x58))(1);
  _DAT_004f8188 = 0x41a00000;
  ppiVar8 = &DAT_004f804c;
  for (iVar6 = 0x2c; iVar6 != 0; iVar6 = iVar6 + -1) {
    *ppiVar8 = (int *)0x0;
    ppiVar8 = ppiVar8 + 1;
  }
  _DAT_004f7f04 = *(undefined4 *)((int)this + 0x4b4);
  _DAT_004f7ef4 = 0x6d;
  _DAT_004f7ef6 = 0;
  _DAT_004f7efc = 6;
  uVar7 = 0x19a;
  _DAT_004f7f00 = 7;
  _DAT_004f7f1c = *(undefined4 *)((int)this + 0x4b4);
  _DAT_004f7f0c = 0x73;
  _DAT_004f7f0e = 0;
  _DAT_004f7f14 = 6;
  _DAT_004f7f18 = 7;
  _DAT_004f7f34 = *(undefined4 *)((int)this + 0x4b4);
  _DAT_004f7f24 = 0x6f;
  _DAT_004f7f26 = 0;
  _DAT_004f7f2c = 6;
  _DAT_004f7f30 = 7;
  _DAT_004f7f4c = *(undefined4 *)((int)this + 0x4b4);
  _DAT_004f7f3c = 0x71;
  _DAT_004f7f3e = 0;
  _DAT_004f7f44 = 6;
  _DAT_004f7f48 = 7;
  _DAT_004f7f64 = *(undefined4 *)((int)this + 0x4b4);
  _DAT_004f7f54 = 0x74;
  _DAT_004f7f56 = 0;
  _DAT_004f7f5c = 6;
  _DAT_004f7f60 = 7;
  _DAT_004f7f94 = *(undefined4 *)((int)this + 0x4b4);
  _DAT_004f7f84 = 0x76;
  _DAT_004f7f86 = 0;
  _DAT_004f7f8c = 6;
  _DAT_004f7f90 = 7;
  _DAT_004f7fac = *(undefined4 *)((int)this + 0x4b4);
  _DAT_004f7f9c = 0x6e;
  _DAT_004f7f9e = 0;
  _DAT_004f7fa4 = 6;
  _DAT_004f7fa8 = 7;
  _DAT_004f7f7c = *(undefined4 *)((int)this + 0x4b4);
  _DAT_004f7f6c = 0x72;
  _DAT_004f7f6e = 0;
  _DAT_004f7f74 = 6;
  _DAT_004f7f78 = 7;
  _DAT_004f7ef8 = uVar7;
  _DAT_004f7f10 = uVar7;
  _DAT_004f7f28 = uVar7;
  _DAT_004f7f40 = uVar7;
  _DAT_004f7f58 = uVar7;
  _DAT_004f7f70 = uVar7;
  _DAT_004f7f88 = uVar7;
  _DAT_004f7fa0 = uVar7;
  pOVar3 = (OMediaCanvasElement *)FUN_00478990(0xd0);
  if (pOVar3 == (OMediaCanvasElement *)0x0) {
    DAT_004f7cd4 = (int *)0x0;
  }
  else {
    DAT_004f7cd4 = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
  }
  (**(code **)(*DAT_004f7cd4 + 0x34))(DAT_00509a34);
  C2DInGameMenu::vfunc_00_276(this);
  (**(code **)(*DAT_004f7cd4 + 0x58))(1);
  pOVar3 = (OMediaCanvasElement *)FUN_00478990(0xd0);
  if (pOVar3 == (OMediaCanvasElement *)0x0) {
    DAT_004f7cd8 = (int *)0x0;
  }
  else {
    DAT_004f7cd8 = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
  }
  (**(code **)(*DAT_004f7cd8 + 0x34))(DAT_00509a34);
  C2DInGameMenu::vfunc_00_276(this);
  (**(code **)(*DAT_004f7cd8 + 0x58))(1);
  pOVar3 = (OMediaCanvasElement *)FUN_00478990(0xd0);
  if (pOVar3 == (OMediaCanvasElement *)0x0) {
    DAT_004f7cdc = (int *)0x0;
  }
  else {
    DAT_004f7cdc = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
  }
  (**(code **)(*DAT_004f7cdc + 0x34))(DAT_00509a34);
  C2DInGameMenu::vfunc_00_276(this);
  (**(code **)(*DAT_004f7cdc + 0x58))(1);
  pOVar3 = (OMediaCanvasElement *)FUN_00478990(0xd0);
  if (pOVar3 == (OMediaCanvasElement *)0x0) {
    DAT_004f7ce0 = (int *)0x0;
  }
  else {
    DAT_004f7ce0 = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
  }
  (**(code **)(*DAT_004f7ce0 + 0x34))(DAT_00509a34);
  C2DInGameMenu::vfunc_00_276(this);
  (**(code **)(*DAT_004f7ce0 + 0x58))(1);
  pOVar3 = (OMediaCanvasElement *)FUN_00478990(0xd0);
  if (pOVar3 == (OMediaCanvasElement *)0x0) {
    DAT_004f7ce4 = (int *)0x0;
  }
  else {
    DAT_004f7ce4 = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
  }
  (**(code **)(*DAT_004f7ce4 + 0x34))(DAT_00509a34);
  C2DInGameMenu::vfunc_00_276(this);
  (**(code **)(*DAT_004f7ce4 + 0x58))(1);
  pOVar3 = (OMediaCanvasElement *)FUN_00478990(0xd0);
  if (pOVar3 == (OMediaCanvasElement *)0x0) {
    DAT_004f7cec = (int *)0x0;
  }
  else {
    DAT_004f7cec = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
  }
  uVar2 = DAT_00509a34;
  (**(code **)(*DAT_004f7cec + 0x34))();
  C2DInGameMenu::vfunc_00_276(this);
  (**(code **)(*DAT_004f7cec + 0x58))(1);
  pOVar3 = (OMediaCanvasElement *)FUN_00478990(0xd0);
  if (pOVar3 == (OMediaCanvasElement *)0x0) {
    DAT_004f7cf0 = (int *)0x0;
  }
  else {
    DAT_004f7cf0 = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
  }
  (**(code **)(*DAT_004f7cf0 + 0x34))(DAT_00509a34);
  C2DInGameMenu::vfunc_00_276(this);
  (**(code **)(*DAT_004f7cf0 + 0x58))(1);
  pOVar3 = (OMediaCanvasElement *)FUN_00478990(0xd0);
  if (pOVar3 == (OMediaCanvasElement *)0x0) {
    DAT_004f7ce8 = (int *)0x0;
  }
  else {
    DAT_004f7ce8 = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
  }
  (**(code **)(*DAT_004f7ce8 + 0x34))(DAT_00509a34);
  C2DInGameMenu::vfunc_00_276(this);
  (**(code **)(*DAT_004f7ce8 + 0x58))(1);
  piVar4 = DAT_004f7cd4;
  if (DAT_00509a13 == '\0') {
    DAT_004f7cd4[0x2d] = 6;
    piVar4[0x2e] = 1;
    piVar4 = DAT_004f7cd8;
    DAT_004f7cd8[0x2d] = 6;
    piVar4[0x2e] = 1;
    piVar4 = DAT_004f7cdc;
    DAT_004f7cdc[0x2d] = 6;
    piVar4[0x2e] = 1;
    piVar4 = DAT_004f7ce0;
    DAT_004f7ce0[0x2d] = 6;
    piVar4[0x2e] = 1;
    piVar4 = DAT_004f7ce4;
    DAT_004f7ce4[0x2d] = 6;
    piVar4[0x2e] = 1;
    piVar4 = DAT_004f7cec;
    DAT_004f7cec[0x2d] = 6;
    piVar4[0x2e] = 1;
    piVar4 = DAT_004f7cf0;
    DAT_004f7cf0[0x2d] = 6;
    piVar4[0x2e] = 1;
    piVar4 = DAT_004f7ce8;
    DAT_004f7ce8[0x2d] = 6;
    piVar4[0x2e] = 1;
  }
  else {
    DAT_004f7cd4[0x2d] = 6;
    piVar4[0x2e] = 7;
    piVar4 = DAT_004f7cd8;
    DAT_004f7cd8[0x2d] = 6;
    piVar4[0x2e] = 7;
    piVar4 = DAT_004f7cdc;
    DAT_004f7cdc[0x2d] = 6;
    piVar4[0x2e] = 7;
    piVar4 = DAT_004f7ce0;
    DAT_004f7ce0[0x2d] = 6;
    piVar4[0x2e] = 7;
    piVar4 = DAT_004f7ce4;
    DAT_004f7ce4[0x2d] = 6;
    piVar4[0x2e] = 7;
    piVar4 = DAT_004f7cec;
    DAT_004f7cec[0x2d] = 6;
    piVar4[0x2e] = 7;
    piVar4 = DAT_004f7cf0;
    DAT_004f7cf0[0x2d] = 6;
    piVar4[0x2e] = 7;
    piVar4 = DAT_004f7ce8;
    DAT_004f7ce8[0x2d] = 6;
    piVar4[0x2e] = 7;
  }
  _DAT_004f7c54 = *(undefined4 *)((int)this + 0x4b4);
  _DAT_004f7c44 = 0x75;
  _DAT_004f7c46 = 0x189;
  _DAT_004f7c4c = 6;
  _DAT_004f7c50 = 7;
  _DAT_004f7cac = *(undefined4 *)((int)this + 0x4b4);
  _DAT_004f7c9c = 0;
  _DAT_004f7c9e = 0;
  _DAT_004f7ca0 = 0xffff;
  _DAT_004f7ca4 = 6;
  _DAT_004f7ca8 = 7;
  _DAT_004f7c48 = uVar7;
  *(undefined1 *)((int)this + 0x4f0) = 0;
  if (DAT_004f7c98 != (int *)0x0) {
    (**(code **)(*DAT_004f7c98 + 0x58))(1);
    if (DAT_004f7c98[0x23] != 0) {
      (*(code *)**(undefined4 **)(DAT_004f7c98[0x23] + 0x24))(1);
    }
    if (DAT_004f7c98 != (int *)0x0) {
      (**(code **)(DAT_004f7c98[0xc] + 8))(1);
    }
    DAT_004f7c98 = (int *)0x0;
  }
  *(undefined4 *)((int)this + 0x4f4) = 0;
  *(undefined4 *)((int)this + 0x4f8) = 0x40000000;
  *(undefined4 *)((int)this + 0x4fc) = 0;
  *(undefined4 *)((int)this + 0x500) = 0;
  *(undefined4 *)((int)this + 0x504) = 0;
  *(undefined4 *)((int)this + 0x4d8) = 3;
  *(undefined4 *)((int)this + 0x4ec) = 0xffffffbb;
  C2DInGameMenu::vfunc_00_305(this);
  pOVar3 = (OMediaCanvasElement *)FUN_00478990(0xd0);
  if (pOVar3 == (OMediaCanvasElement *)0x0) {
    DAT_004f7c90 = (int *)0x0;
  }
  else {
    DAT_004f7c90 = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
  }
  (**(code **)(*DAT_004f7c90 + 0x34))(DAT_00509a34);
  C2DInGameMenu::vfunc_00_276(this);
  piVar4 = DAT_004f7c90;
  DAT_004f7c90[0x2d] = 6;
  piVar4[0x2e] = 1;
  (**(code **)(*DAT_004f7c90 + 0x58))(1);
  pcVar11 = s_Before_IntSetScreen_004ec538;
  DAT_004f8180 = 0;
  CGameObject::vfunc_00_013(this_02);
  _DAT_004f814c = *(undefined4 *)((int)this + 0x4b4);
  pcVar10 = s_After_IntSetScreen_004ec524;
  _DAT_004f813c = 0x6b;
  _DAT_004f813e = 0;
  _DAT_004f8140 = 0xffff;
  _DAT_004f8144 = 6;
  _DAT_004f8148 = 1;
  CGameObject::vfunc_00_013(this_03);
  pOVar3 = (OMediaCanvasElement *)FUN_00478990(0xd0,pcVar10,pcVar11);
  if (pOVar3 == (OMediaCanvasElement *)0x0) {
    piVar4 = (int *)0x0;
  }
  else {
    piVar4 = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
  }
  *(int **)((int)this + 0x4e4) = piVar4;
  (**(code **)(*piVar4 + 0x34))(DAT_00509a34);
  iVar6 = *(int *)((int)this + 0x4e4);
  *(undefined4 *)(iVar6 + 0xb4) = 6;
  *(undefined4 *)(iVar6 + 0xb8) = uVar2;
  C2DInGameMenu::vfunc_00_276(this);
  (**(code **)(**(int **)((int)this + 0x4e4) + 0x58))(1);
  pOVar3 = (OMediaCanvasElement *)FUN_00478990(0xd0);
  if (pOVar3 == (OMediaCanvasElement *)0x0) {
    DAT_004f7c94 = (int *)0x0;
  }
  else {
    DAT_004f7c94 = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
  }
  (**(code **)(*DAT_004f7c94 + 0x34))(DAT_00509a34);
  piVar4 = DAT_004f7c94;
  DAT_004f7c94[0x2d] = 6;
  piVar4[0x2e] = 7;
  C2DInGameMenu::vfunc_00_276(this);
  (**(code **)(*DAT_004f7c94 + 0x58))(1);
  pOVar3 = (OMediaCanvasElement *)FUN_00478990(0xd0);
  if (pOVar3 == (OMediaCanvasElement *)0x0) {
    DAT_004f7cfc = (int *)0x0;
  }
  else {
    DAT_004f7cfc = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
  }
  (**(code **)(*DAT_004f7cfc + 0x34))(DAT_00509a34);
  piVar4 = DAT_004f7cfc;
  DAT_004f7cfc[0x2d] = 6;
  piVar4[0x2e] = 7;
  C2DInGameMenu::vfunc_00_276(this);
  (**(code **)(*DAT_004f7cfc + 0x58))(1);
  pOVar3 = (OMediaCanvasElement *)FUN_00478990(0xd0);
  if (pOVar3 == (OMediaCanvasElement *)0x0) {
    DAT_004f7ea0 = (int *)0x0;
  }
  else {
    DAT_004f7ea0 = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
  }
  (**(code **)(*DAT_004f7ea0 + 0x34))(DAT_00509a34);
  piVar4 = DAT_004f7ea0;
  DAT_004f7ea0[0x2d] = 6;
  piVar4[0x2e] = 7;
  C2DInGameMenu::vfunc_00_276(this);
  (**(code **)(*DAT_004f7ea0 + 0x58))(1);
  pOVar3 = (OMediaCanvasElement *)FUN_00478990(0xd0);
  if (pOVar3 == (OMediaCanvasElement *)0x0) {
    DAT_004f7cd0 = (int *)0x0;
  }
  else {
    DAT_004f7cd0 = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
  }
  (**(code **)(*DAT_004f7cd0 + 0x34))(DAT_00509a34);
  piVar4 = DAT_004f7cd0;
  DAT_004f7cd0[0x2d] = 6;
  piVar4[0x2e] = 7;
  C2DInGameMenu::vfunc_00_276(this);
  (**(code **)(*DAT_004f7cd0 + 0x58))(1);
  pOVar3 = (OMediaCanvasElement *)FUN_00478990(0xd0);
  if (pOVar3 == (OMediaCanvasElement *)0x0) {
    DAT_004f8024 = (int *)0x0;
  }
  else {
    DAT_004f8024 = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
  }
  (**(code **)(*DAT_004f8024 + 0x34))(DAT_00509a34);
  piVar4 = DAT_004f8024;
  DAT_004f8024[0x2d] = 6;
  piVar4[0x2e] = 7;
  C2DInGameMenu::vfunc_00_276(this);
  (**(code **)(*DAT_004f8024 + 0x58))();
  _DAT_004f8134 = *(undefined4 *)((int)this + 0x4b4);
  _DAT_004f8124 = 0xf4;
  _DAT_004f8126 = 0x19a;
  _DAT_004f8128 = 0x14;
  _DAT_004f812c = 6;
  _DAT_004f8130 = 7;
  pOVar3 = (OMediaCanvasElement *)FUN_00478990(0xd0);
  if (pOVar3 == (OMediaCanvasElement *)0x0) {
    DAT_004f7eec = (int *)0x0;
  }
  else {
    DAT_004f7eec = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
  }
  (**(code **)(*DAT_004f7eec + 0x34))(DAT_00509a34);
  piVar4 = DAT_004f7eec;
  DAT_004f7eec[0x2d] = 6;
  piVar4[0x2e] = 7;
  C2DInGameMenu::vfunc_00_276(this);
  (**(code **)(*DAT_004f7eec + 0x58))(1);
  pOVar3 = (OMediaCanvasElement *)FUN_00478990(0xd0);
  if (pOVar3 == (OMediaCanvasElement *)0x0) {
    DAT_004f802c = (int *)0x0;
  }
  else {
    DAT_004f802c = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
  }
  (**(code **)(*DAT_004f802c + 0x34))(DAT_00509a34);
  piVar4 = DAT_004f802c;
  DAT_004f802c[0x2d] = 6;
  piVar4[0x2e] = 7;
  C2DInGameMenu::vfunc_00_276(this);
  (**(code **)(*DAT_004f802c + 0x58))();
  pOVar3 = (OMediaCanvasElement *)FUN_00478990(0xd0);
  if (pOVar3 == (OMediaCanvasElement *)0x0) {
    DAT_004f8118 = (int *)0x0;
  }
  else {
    DAT_004f8118 = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
  }
  (**(code **)(*DAT_004f8118 + 0x34))(DAT_00509a34);
  piVar4 = DAT_004f8118;
  DAT_004f8118[0x2d] = 6;
  piVar4[0x2e] = 7;
  C2DInGameMenu::vfunc_00_276(this);
  (**(code **)(*DAT_004f8118 + 0x58))(1);
  pOVar3 = (OMediaCanvasElement *)FUN_00478990(0xd0);
  if (pOVar3 == (OMediaCanvasElement *)0x0) {
    DAT_004f7db0 = (int *)0x0;
  }
  else {
    DAT_004f7db0 = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
  }
  (**(code **)(*DAT_004f7db0 + 0x34))(DAT_00509a34);
  piVar4 = DAT_004f7db0;
  DAT_004f7db0[0x2d] = 6;
  piVar4[0x2e] = 7;
  C2DInGameMenu::vfunc_00_276(this);
  (**(code **)(*DAT_004f7db0 + 0x58))(1);
  DAT_004f8198 = 0;
  *(undefined4 *)((int)this + 0x4dc) = 0;
  pOVar3 = (OMediaCanvasElement *)FUN_00478990(0xd0);
  if (pOVar3 == (OMediaCanvasElement *)0x0) {
    DAT_004f8020 = (int *)0x0;
  }
  else {
    DAT_004f8020 = (int *)OMediaCanvasElement::OMediaCanvasElement(pOVar3);
  }
  (**(code **)(*DAT_004f8020 + 0x34))(DAT_00509a34);
  C2DInGameMenu::vfunc_00_276(this);
  piVar4 = DAT_004f8020;
  DAT_004f8020[0x2d] = 6;
  piVar4[0x2e] = 7;
  (**(code **)(*DAT_004f8020 + 0x58))(1);
  _DAT_004f8114 = *(undefined4 *)((int)this + 0x4b4);
  _DAT_004f8108 = 0;
  _DAT_004f8104 = 0xf7;
  _DAT_004f8106 = 0;
  _DAT_004f810c = 6;
  _DAT_004f8110 = 7;
  _DAT_004f7ccc = *(undefined4 *)((int)this + 0x4b4);
  _DAT_004f7cbc = 0x14f;
  _DAT_004f7cbe = 0;
  _DAT_004f7cc0 = 200;
  _DAT_004f7cc4 = 6;
  _DAT_004f7cc8 = 7;
  C2DInGameMenu::vfunc_00_306(this);
  (**(code **)(*DAT_004f804c + 0x58))(1);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  C2DInGameMenu::vfunc_00_306(this);
  puVar9 = &DAT_004f7d00;
  for (iVar6 = 0x2c; iVar6 != 0; iVar6 = iVar6 + -1) {
    *puVar9 = 0;
    puVar9 = puVar9 + 1;
  }
  CGameObject::vfunc_00_108(this);
  CGameObject::vfunc_00_036(this);
  CGameObject::vfunc_00_052(this);
  *(undefined4 *)((int)this + 0x4c4) = 0;
  *(undefined1 *)((int)this + 0x4c0) = 0;
  DAT_004f8181 = 0;
  *(undefined4 *)((int)this + 0x4cc) = 0;
  *(undefined4 *)((int)this + 0x4e0) = 0;
  DAT_004ec494 = 1;
  _DAT_004ec490 = 0x3f800000;
  *(undefined4 *)((int)this + 0x508) = 0;
  *(undefined4 *)((int)this + 0x514) = 0;
  *(undefined4 *)((int)this + 0x518) = 0;
  CGameObject::vfunc_00_013(this_04);
  ExceptionList = (void *)0x1;
  return this;
}


```
