# C3DPlayer movement target 3 evidence

Recovered 2026-07-02 with `tools/ghidra/CreateFunctions.java` against
`~/ghidra-projects/JN_decomp` / `Neutron.exe`.

## Interpretation

Target 3 function-defined and dumped the player movement/action helper set
listed in `docs/ghidra_recovery_plan.md`: `00437890`, `00437c40`,
`00437f90`, `00438bc0`, `00439900`, `0043a120`, `0043a420`, `0043a5d0`,
`0043a790`, `0043a7f0`, `0043aff0`, `0043b5a0`, and `0043b820`.

Signature caveat: several bodies still need final stack-argument repair.
`UpdateGroundMoveA_00437c40` and `UpdateJumpFallMove_00437f90` show the
per-frame `dt` value as `unaff_retaddr`/other recovered temporaries because
the helper created function boundaries but did not assign final prototypes.
The dump is still L1-useful: it exposes control flow, offsets, constants,
strings, and called slots that were previously only raw `objdump` prose.

Recovered movement shape:

- `ResetPlayerRuntime_00437890` clears `current_speed`, `lean_angle`, and
  `previous_lean_angle`, then seeds `initial_or_ground_y` from the transform
  slot result.
- `UpdateGroundMoveA_00437c40` applies `turn_or_yaw_rate` through slot
  `0x334`, uses the engine trig table to rotate the `walk_speed`/bias vector,
  writes inherited velocity through slot `0x2c4`, clears scratch animation
  state when no active player exists, and has a grounded adjustment path
  through slots `0x218`/`0x338`.
- `UpdateJumpFallMove_00437f90` seeds probe offsets `0x7ec=-40.0` and
  `0x7f0=10.0`, applies the same turn/velocity projection, branches on
  `jump_or_motion_phase` (`0x7e8`), handles linked-object burn-time/scale
  through `0x7f8`/`0x884`, and switches `JUMP`/`FALL`/phase states.
- `UpdateWalkingCameraA_00438bc0` is the largest active walking helper:
  it handles special-lock input side states, random idle action animations
  (`SCRATCH`, `BUTTONS`, `PLAY`) via the `0x9bc`/`0x9b8` scratch fields,
  accelerates/decelerates `walk_speed` using `0x6bc`/`0x6c0`/`0x6c4`,
  snapshots camera origin `0x714..0x71c`, and updates `DAT_00509a50`
  position/angles.
- `UpdateWalkingCameraB_00439900` is the explicit accel/decel ramp: turn input
  changes `turn_or_yaw_rate` by `dt*100`, forward input accelerates
  `walk_speed` toward `walk_speed_cap`, the no-input high-speed tail subtracts
  `walk_decel_step*0.5` once above `cap*0.90909094`, reverse/brake paths
  subtract `walk_decel_step` or `*0.5`, and yaw clamps are `+/-50` in submode
  `2` and `+/-30` otherwise.
- `UpdateSittingOrSmoothCamera_0043a120` reuses the same turn-input ramp and
  smooths `DAT_00509a50` toward an offset target with `1.2` position scaling.
- `UpdateRotateToTargetDelta_0043a420` compares current and target transform
  fields, wraps angle deltas to `[-180,180]`, and applies slot `0x334` with
  multipliers `4.5`, `3.7`, and `4.5`.
- `ProjectNoisyCameraTarget_0043a5d0` projects a local offset through the
  engine trig table and current transform, writing a vec4 with `w=1.0`.
- `TouchOrCollisionFallback_0043a790` ignores/handles `C3DGODDARD` specially;
  non-Goddard touches delegate to inherited touch, and non-active players hit
  by Goddard zero velocity.
- `GroundAheadPredicate_0043a7f0` only runs in `motion_submode==0`; it probes
  from a transformed point down by `2000.0` and returns true when the random/
  hit threshold exceeds `0x46`.
- `SetPlayerAnimationState_0043aff0` is now body-backed: it rejects duplicate
  or locked transitions, handles `STOP`, `WALK`, `EDGE`, `JUMP`, `SWING`,
  `BACK`, `FALL`, `LEFT`, `RIGHT`, `HIT`, `FENCE`, and `LADDER`, sets
  `DAT_004f83e0` for fence/ladder, stores restore transforms at `0x838..0x844`,
  uses durations `4.0` (`FENCE`) and `6.0` (`LADDER`), sets grace `5.0`, and
  plays sound ids `0x92`/`0x8f`.
- `PlayerLoadLevel_0043b5a0` copies level-transition strings into the player
  buffers, sets global game flag `DAT_00509980+0x1fe`, snapshots current
  position to `0x9a8..0x9b4` if needed, calls global game slot `0x100`, and
  stops/reset animation through slot `0x148`.
- `ProbePlayerRayBlend_0043b820` raycasts between the caller vector and player
  position, blends hit coordinates 75 percent back toward the source, and
  returns `1.5` on hit.

This opens the target 3 L1 evidence. It does not create a linkable native row:
the native player remains the approved tank-turn design, and replacing it with
this recovered state machine is explicitly a product/native-port decision.

2026-07-02 follow-up: the walking-camera halves are now fully interpreted
— prototype-repaired re-dump, record-write x87 trace,
ProjectNoisyCameraTarget / ProbePlayerRayBlend decode, and the OMedia
angles() import — in `walking_camera_record_write.md`; the B-variant
record write is ported and certified (`C3DPlayer`/`walking-camera-record`).
The A/B summaries above keep the movement-half reading; the camera-write
details there are superseded.

## Raw Ghidra Dump

## ResetPlayerRuntime_00437890 @ 00437890

```c

void __thiscall ResetPlayerRuntime_00437890(void *this)

{
  int iVar1;
  undefined1 auStack_10 [16];

  CLocalGameObject::vfunc_00_010(this);
  *(undefined4 *)((int)this + 0x608) = 0;
  *(undefined4 *)((int)this + 0x614) = 0;
  *(undefined4 *)((int)this + 0x618) = 0;
  iVar1 = (**(code **)(*(int *)this + 0x328))(auStack_10);
  *(undefined4 *)((int)this + 0x61c) = *(undefined4 *)(iVar1 + 4);
  return;
}


```

## UpdateGroundMoveA_00437c40 @ 00437c40

```c

void __thiscall UpdateGroundMoveA_00437c40(void *this)

{
  int *piVar1;
  float fVar2;
  char cVar3;
  float *pfVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  undefined4 *puVar8;
  float10 fVar9;
  float fVar10;
  float unaff_retaddr;
  undefined1 *puStack_bc;
  undefined1 *puStack_b8;
  float *pfStack_b4;
  float *pfStack_b0;
  float fStack_ac;
  undefined4 uStack_a8;
  float fStack_a4;
  float fStack_a0;
  float fStack_9c;
  float fStack_98;
  float *pfStack_94;
  undefined4 uStack_90;
  float fStack_8c;
  undefined4 uStack_88;
  undefined1 *puStack_84;
  float fStack_74;
  float fStack_70;
  float fStack_6c;
  undefined4 uStack_68;
  float fStack_64;
  float fStack_60;
  float fStack_5c;
  undefined4 uStack_58;
  undefined1 auStack_54 [4];
  float afStack_50 [2];
  undefined1 auStack_48 [4];
  undefined1 auStack_44 [4];
  undefined1 auStack_40 [52];
  float fStack_c;

  fStack_60 = 0.0;
  puStack_84 = auStack_40;
  uStack_58 = 0;
  piVar1 = (int *)((int)this + 0xc0);
  uStack_88 = 0x437c70;
  pfVar4 = (float *)(**(code **)(*(int *)((int)this + 0xc0) + 700))();
  afStack_50[0] = pfVar4[1];
  if (DAT_004f83e0 == '\0') {
    if ((SQRT(*pfVar4 * *pfVar4 + pfVar4[2] * pfVar4[2] + afStack_50[0] * afStack_50[0]) != 0.0) ||
       (*(float *)((int)this + 0x870) != 0.0)) {
      uStack_88 = 0;
      *(float *)((int)this + 0x6d8) = *(float *)((int)this + 0x6d4);
      fStack_8c = *(float *)((int)this + 0x6d4) * unaff_retaddr;
      uStack_90 = 0;
      pfStack_94 = (float *)0x437cee;
      (**(code **)(*piVar1 + 0x334))();
      pfStack_94 = (float *)0x437cf8;
      fVar9 = (float10)(**(code **)(*piVar1 + 0x248))();
      pfStack_94 = afStack_50;
      fStack_70 = (float)(((float10)*(float *)((int)this + 0x6d8) * (float10)0.022222223 *
                           ((float10)1.0 - fVar9) * (float10)*(float *)((int)this + 0x6c8) +
                          (float10)*(float *)((int)this + 0x870)) * (float10)fStack_c);
      fStack_98 = 6.19789e-39;
      (**(code **)(*piVar1 + 0x328))();
      uStack_88 = 0x437d40;
      uVar5 = __ftol();
      iVar6 = (uVar5 & 0x3fff) * 4;
      fStack_74 = *(float *)(global_exref + iVar6);
      fVar2 = *(float *)(global_exref + 0x10004) * fStack_64;
      fVar10 = -(-(*(float *)global_exref * fStack_64) * *(float *)global_exref);
      fStack_64 = fVar2 * *(float *)(global_exref + iVar6 + 0x10004) - fVar10 * fStack_74;
      fStack_5c = fVar2 * fStack_74 + fVar10 * *(float *)(global_exref + iVar6 + 0x10004);
    }
    uStack_88 = *(undefined4 *)((int)this + 0x6c8);
    fStack_8c = 0.0;
    pfStack_94 = (float *)auStack_44;
    uStack_90 = 0;
    fStack_98 = 6.198103e-39;
    pfVar4 = (float *)(**(code **)(*piVar1 + 0x38c))();
    fStack_98 = fStack_6c + pfVar4[2];
    fStack_9c = fStack_60;
    fStack_a0 = fStack_74 + *pfVar4;
    fStack_a4 = 6.198165e-39;
    (**(code **)(*piVar1 + 0x2c4))();
    if (DAT_005099e4 == 0) {
      *(undefined1 *)((int)this + 0x9bc) = 0;
      *(undefined4 *)((int)this + 0x9b8) = 0;
    }
    fStack_a4 = 6.198215e-39;
    cVar3 = (**(code **)(*piVar1 + 0x218))();
    if (cVar3 != '\0') {
      fStack_a4 = 6.19824e-39;
      iVar6 = (**(code **)(*piVar1 + 0x338))();
      if (iVar6 != 0) {
        fStack_a4 = 6.198265e-39;
        iVar6 = (**(code **)(*piVar1 + 0x338))();
        fStack_a4 = 1.0;
        uStack_a8 = 0;
        fStack_74 = *(float *)(iVar6 + 0x14);
        pfStack_b0 = &fStack_60;
        fStack_ac = 0.0;
        pfStack_b4 = (float *)0x437e75;
        iVar6 = (**(code **)(*piVar1 + 900))();
        uStack_68 = *(undefined4 *)(iVar6 + 8);
        pfStack_b4 = &fStack_60;
        puStack_b8 = (undefined1 *)0x437e8b;
        iVar6 = (**(code **)(*piVar1 + 0x310))();
        fStack_a4 = fStack_6c - *(float *)(iVar6 + 8);
        puStack_b8 = auStack_54;
        puStack_bc = (undefined1 *)0x437ea5;
        (**(code **)(*piVar1 + 0x310))();
        puStack_bc = auStack_48;
        (**(code **)(*piVar1 + 0x310))();
        fVar10 = OMedia3DVector::quick_magnitude((OMedia3DVector *)&fStack_9c);
        fVar10 = 1.0 / fVar10;
        fStack_9c = fVar10 * fStack_9c;
        fStack_98 = fVar10 * fStack_98;
        pfStack_94 = (float *)(fVar10 * (float)pfStack_94);
        fStack_8c = (float)pfStack_94 - fStack_ac;
        OMedia3DVector::angles
                  ((OMedia3DVector *)&fStack_8c,(short *)&pfStack_b0,(short *)&pfStack_b4);
        iVar6 = *piVar1;
        iVar7 = (**(code **)(iVar6 + 700))();
        iVar7 = (**(code **)(*piVar1 + 700))(&fStack_60,*(undefined4 *)(iVar7 + 8));
        puVar8 = (undefined4 *)
                 (**(code **)(*piVar1 + 700))(&fStack_74,*(float *)(iVar7 + 4) - 250.0);
        (**(code **)(iVar6 + 0x2c4))(*puVar8);
        FUN_00476ff0(&fStack_a4,&puStack_b8,&puStack_bc,&stack0xffffff40);
        return;
      }
    }
  }
  return;
}


```

## UpdateJumpFallMove_00437f90 @ 00437f90

```c

void __thiscall UpdateJumpFallMove_00437f90(void *this)

{
  int *piVar1;
  float fVar2;
  char cVar3;
  short sVar4;
  float *pfVar5;
  uint uVar6;
  int iVar7;
  int iVar8;
  undefined4 *puVar9;
  CGameObject *this_00;
  CGameObject *this_01;
  float unaff_EBP;
  float unaff_retaddr;
  char *pcVar10;
  double dVar11;
  undefined4 uVar12;
  float *pfStack_88;
  float *pfStack_84;
  undefined4 *puStack_80;
  float fStack_7c;
  undefined4 uStack_78;
  float fStack_74;
  float *pfStack_70;
  undefined4 uStack_6c;
  float fStack_68;
  undefined4 uStack_64;
  undefined1 *puStack_60;
  float fStack_4c;
  float fStack_48;
  float fStack_44;
  undefined4 uStack_40;
  float fStack_3c;
  undefined4 uStack_38;
  float afStack_30 [3];
  undefined1 auStack_24 [4];
  undefined1 auStack_20 [32];

  puStack_60 = auStack_20;
  piVar1 = (int *)((int)this + 0xc0);
  uStack_40 = 0;
  uStack_38 = 0;
  *(undefined4 *)((int)this + 0x7ec) = 0xc2200000;
  *(undefined4 *)((int)this + 0x7f0) = 0x41200000;
  uStack_64 = 0x437fd6;
  pfVar5 = (float *)(**(code **)(*(int *)((int)this + 0xc0) + 700))();
  afStack_30[0] = pfVar5[1];
  if (SQRT(*pfVar5 * *pfVar5 + pfVar5[2] * pfVar5[2] + afStack_30[0] * afStack_30[0]) != 0.0) {
    uStack_64 = 0;
    *(float *)((int)this + 0x6d8) = *(float *)((int)this + 0x6d4);
    fStack_68 = *(float *)((int)this + 0x6d4) * unaff_retaddr;
    uStack_6c = 0;
    pfStack_70 = (float *)0x438031;
    (**(code **)(*piVar1 + 0x334))();
    pfStack_70 = (float *)0x43803b;
    (**(code **)(*piVar1 + 0x248))();
    pfStack_70 = afStack_30;
    fStack_74 = 6.199042e-39;
    (**(code **)(*piVar1 + 0x328))();
    uStack_64 = 0x438076;
    uVar6 = __ftol();
    iVar7 = (uVar6 & 0x3fff) * 4;
    fStack_48 = *(float *)(global_exref + iVar7);
    fStack_4c = *(float *)(global_exref + 0x10004) * fStack_44;
    fVar2 = -(-(*(float *)global_exref * fStack_44) * *(float *)global_exref);
    fStack_44 = fStack_4c * *(float *)(global_exref + iVar7 + 0x10004) - fVar2 * fStack_48;
    fStack_3c = fStack_4c * fStack_48 + fVar2 * *(float *)(global_exref + iVar7 + 0x10004);
  }
  uStack_64 = *(undefined4 *)((int)this + 0x6c8);
  fStack_68 = 0.0;
  pfStack_70 = (float *)auStack_24;
  uStack_6c = 0;
  fStack_74 = 6.199252e-39;
  pfVar5 = (float *)(**(code **)(*piVar1 + 0x38c))();
  fStack_74 = fStack_4c + pfVar5[2];
  uStack_78 = uStack_40;
  fStack_7c = unaff_EBP + *pfVar5;
  puStack_80 = (undefined4 *)0x43812a;
  (**(code **)(*piVar1 + 0x2c4))();
  puStack_80 = &uStack_40;
  pfStack_84 = (float *)0x438139;
  (**(code **)(*piVar1 + 0x310))();
  pfStack_84 = &fStack_44;
  pfStack_88 = (float *)0x438148;
  (**(code **)(*piVar1 + 700))();
  if (*(short *)((int)this + 0x7e8) < 3) {
    iVar7 = *piVar1;
    pfStack_88 = &fStack_48;
    iVar8 = (**(code **)(iVar7 + 0x270))();
    puVar9 = (undefined4 *)
             (**(code **)(*piVar1 + 0x270))
                       (&stack0xffffffa4,DAT_005cfc04 * 1.33 - DAT_005cfc04,
                        *(undefined4 *)(iVar8 + 8));
    uVar12 = *puVar9;
  }
  else {
    if ((*(int *)((int)this + 0x7f8) != 0) &&
       (*(float *)(*(int *)((int)this + 0x7f8) + 0x6c0) <= 0.0)) {
      pfStack_88 = (float *)s_MEBTEMP_BurnTime_>_0_004f0608;
      CGameObject::vfunc_00_013(this_00);
      if ((*(short *)((int)this + 0x7e8) == 4) || (*(short *)((int)this + 0x7e8) == 3)) {
        pfStack_88 = (float *)0x1;
        *(undefined2 *)((int)this + 0x7e8) = 0;
        *(undefined2 *)((int)this + 0x7c4) = 1;
        (**(code **)(*(int *)this + 0xe0))(&DAT_004ef3e8);
        *(undefined4 *)(*(int *)((int)this + 0x7f8) + 0x6c0) = 0;
        (**(code **)(*(int *)this + 0x140))();
        return;
      }
    }
    iVar7 = *piVar1;
    if (500.0 <= *(float *)((int)this + 0x86c)) {
      pfStack_88 = &fStack_48;
      iVar8 = (**(code **)(iVar7 + 0x270))();
      puVar9 = (undefined4 *)
               (**(code **)(*piVar1 + 0x270))
                         (&stack0xffffffa4,DAT_005cfc04 * 0.1 - DAT_005cfc04,
                          *(undefined4 *)(iVar8 + 8));
      uVar12 = *puVar9;
    }
    else {
      pfStack_88 = &fStack_48;
      iVar8 = (**(code **)(iVar7 + 0x270))();
      puVar9 = (undefined4 *)
               (**(code **)(*piVar1 + 0x270))
                         (&stack0xffffffa4,-DAT_005cfc04 - DAT_005cfc04 * 0.1,
                          *(undefined4 *)(iVar8 + 8));
      (**(code **)(iVar7 + 0x278))(*puVar9);
      iVar7 = (**(code **)(*piVar1 + 700))(&stack0xffffffa4);
      if (-1.0 <= *(float *)(iVar7 + 4)) goto LAB_00438313;
      iVar7 = *piVar1;
      iVar8 = (**(code **)(iVar7 + 0x270))();
      puVar9 = (undefined4 *)
               (**(code **)(*piVar1 + 0x270))
                         (&fStack_74,-DAT_005cfc04 - DAT_005cfc04 * 0.3,*(undefined4 *)(iVar8 + 8));
      uVar12 = *puVar9;
    }
  }
  (**(code **)(iVar7 + 0x278))(uVar12);
LAB_00438313:
  cVar3 = (**(code **)(*(int *)this + 0x158))();
  if (cVar3 == '\0') {
    switch(*(undefined2 *)((int)this + 0x7e8)) {
    case 1:
      *(undefined2 *)((int)this + 0x7e8) = 2;
      return;
    case 3:
      *(undefined2 *)((int)this + 0x7c4) = 1;
      (**(code **)(*(int *)this + 0xe0))(&DAT_004ef3e8,1);
      if (*(int *)((int)this + 0x7f8) != 0) {
        *(undefined4 *)(*(int *)((int)this + 0x7f8) + 0x6c0) = 0;
      }
      (**(code **)(*(int *)this + 0x140))();
      *(undefined2 *)((int)this + 0x7e8) = 4;
      return;
    case 5:
      cVar3 = (**(code **)(*piVar1 + 0x218))();
      if (cVar3 == '\0') {
        return;
      }
    case 4:
      *(undefined2 *)((int)this + 0x7e8) = 0;
    }
  }
  else {
    if (*(int *)((int)this + 0x7f8) != 0) {
      pfStack_84 = (float *)((*(float *)(*(int *)((int)this + 0x7f8) + 0x6c0) /
                             *(float *)((int)this + 0x884)) * 100.0);
      dVar11 = (double)(float)pfStack_84;
      pcVar10 = s_MEBTEMP_percent__f_004f05f4;
      CGameObject::vfunc_00_013(this_01);
      FUN_004037f0(pfStack_84,pcVar10,dVar11);
    }
    switch(*(undefined2 *)((int)this + 0x7e8)) {
    case 0:
      cVar3 = (**(code **)(*piVar1 + 0x218))();
      if (cVar3 == '\0') {
        (**(code **)(*(int *)this + 0x150))();
        (**(code **)(*(int *)this + 0x140))();
        return;
      }
      if (*(int *)((int)this + 0x7f8) != 0) {
        *(undefined4 *)(*(int *)((int)this + 0x7f8) + 0x6c0) = 0;
      }
      (**(code **)(*(int *)this + 0x150))();
      (**(code **)(*(int *)this + 0x148))();
      (**(code **)(*piVar1 + 0x28c))(0,0x44098000,0);
      *(undefined2 *)((int)this + 0x7e8) = 1;
      *(undefined4 *)((int)this + 0x874) = 0;
      return;
    case 1:
      fVar2 = fStack_3c + *(float *)((int)this + 0x874);
      *(float *)((int)this + 0x874) = fVar2;
      if (0.4 < fVar2) {
        iVar7 = *(int *)(DAT_00509948 + 0x490);
        if (((((iVar7 == 0x4c455636) || (iVar7 == 0x4c563442)) ||
             ((iVar7 == 0x4c563641 ||
              (((iVar7 == 0x4c455633 || (iVar7 == 0x4c563341)) || (iVar7 == 0x4c563342)))))) ||
            (((iVar7 == 0x4c563343 || (iVar7 == 0x4c563344)) ||
             ((DAT_004f0588 == 2 ||
              (((DAT_004f83d4 <= 0.0 || (DAT_004f0588 == 6)) ||
               ((DAT_004f0588 == 7 ||
                ((sVar4 = FUN_00403950(0,3), sVar4 == 0 || (sVar4 = FUN_00403950(0,3), sVar4 == 4)))
                ))))))))) && (*(int *)(DAT_00509948 + 0x490) != 0x56523034)) {
          *(undefined2 *)((int)this + 0x7e8) = 3;
          return;
        }
        if (*(int *)((int)this + 0x7f8) != 0) {
          *(undefined4 *)(*(int *)((int)this + 0x7f8) + 0x6c0) = *(undefined4 *)((int)this + 0x884);
          (**(code **)(*piVar1 + 0x264))();
        }
        (**(code **)(*(int *)this + 0x150))();
        (**(code **)(*(int *)this + 0x138))(2);
        iVar7 = *piVar1;
        iVar8 = (**(code **)(iVar7 + 700))(&fStack_68);
        puVar9 = (undefined4 *)
                 (**(code **)(*piVar1 + 700))(&fStack_7c,0x41200000,*(undefined4 *)(iVar8 + 8));
        (**(code **)(iVar7 + 0x2c4))(*puVar9);
        *(undefined2 *)((int)this + 0x7e8) = 3;
        return;
      }
      break;
    case 2:
      iVar7 = *(int *)(DAT_00509948 + 0x490);
      if (((((((iVar7 == 0x4c455636) || (iVar7 == 0x4c563442)) || (iVar7 == 0x4c563641)) ||
            ((iVar7 == 0x4c455633 || (iVar7 == 0x4c563341)))) || (iVar7 == 0x4c563342)) ||
          ((((iVar7 == 0x4c563343 || (iVar7 == 0x4c563344)) ||
            ((DAT_004f0588 == 2 ||
             (((DAT_004f83d4 <= 0.0 || (DAT_004f0588 == 6)) || (DAT_004f0588 == 7)))))) ||
           ((sVar4 = FUN_00403950(0,3), sVar4 == 0 || (sVar4 = FUN_00403950(0,3), sVar4 == 4))))))
         && (*(int *)(DAT_00509948 + 0x490) != 0x56523034)) {
        *(undefined2 *)((int)this + 0x7e8) = 3;
        return;
      }
      if (*(int *)((int)this + 0x7f8) != 0) {
        *(undefined4 *)(*(int *)((int)this + 0x7f8) + 0x6c0) = *(undefined4 *)((int)this + 0x884);
        (**(code **)(*piVar1 + 0x264))();
      }
      (**(code **)(*(int *)this + 0x150))();
      (**(code **)(*(int *)this + 0x138))(2);
      iVar7 = *piVar1;
      iVar8 = (**(code **)(iVar7 + 700))(&pfStack_88);
      puVar9 = (undefined4 *)
               (**(code **)(*piVar1 + 700))(&stack0xffffffa4,0x41200000,*(undefined4 *)(iVar8 + 8));
      (**(code **)(iVar7 + 0x2c4))(*puVar9);
      *(undefined2 *)((int)this + 0x7e8) = 3;
      return;
    case 3:
      cVar3 = (**(code **)(*piVar1 + 0x218))();
      if (cVar3 != '\0') {
        *(undefined2 *)((int)this + 0x7c4) = 1;
        (**(code **)(*(int *)this + 0xe0))(&DAT_004ef3e8,1);
        (**(code **)(*piVar1 + 0x264))(0x42700000);
        if (*(int *)((int)this + 0x7f8) != 0) {
          *(undefined4 *)(*(int *)((int)this + 0x7f8) + 0x6c0) = 0;
        }
        (**(code **)(*(int *)this + 0x140))();
        *(undefined2 *)((int)this + 0x7e8) = 4;
        return;
      }
      break;
    case 5:
      cVar3 = (**(code **)(*piVar1 + 0x218))();
      if (cVar3 != '\0') {
        *(undefined2 *)((int)this + 0x7e8) = 0;
        return;
      }
    }
  }
  return;
}


```

## UpdateWalkingCameraA_00438bc0 @ 00438bc0

```c

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall UpdateWalkingCameraA_00438bc0(void *this,float param_2)

{
  char *_Str2;
  float fVar1;
  float fVar2;
  float fVar3;
  bool bVar4;
  char cVar5;
  ushort uVar6;
  float *pfVar7;
  int iVar8;
  int iVar9;
  undefined4 *puVar10;
  uint uVar11;
  uint uVar12;
  float unaff_EBX;
  float unaff_EBP;
  float unaff_ESI;
  char *pcVar13;
  short unaff_DI;
  char *pcVar14;
  float10 fVar15;
  float unaff_retaddr;
  float fVar16;
  float *pfStack_80;
  float fStack_6c;
  short sStack_68;
  float fStack_64;
  float fStack_60;
  int iStack_5c;
  float fStack_58;
  float fStack_54;
  float fStack_50;
  float fStack_4c;
  float fStack_48;
  undefined4 uStack_44;
  float fStack_40;
  undefined4 uStack_3c;
  float fStack_38;
  float fStack_34;
  float fStack_30;
  undefined4 uStack_2c;
  float fStack_24;
  float fStack_20;
  float fStack_1c;
  float fStack_18;

  pfStack_80 = &fStack_60;
  fStack_64 = 0.0;
  fStack_60 = 0.0;
  FUN_0046a460(&fStack_64);
  if (DAT_004f83e0 != '\0') {
    return;
  }
  iStack_5c = (int)fStack_64._0_2_;
  *(float *)((int)this + 0x848) = (float)iStack_5c + *(float *)((int)this + 0x848);
  pfStack_80 = (float *)0x438c16;
  cVar5 = (**(code **)(*(int *)this + 0x15c))();
  if (cVar5 == '\0') {
    if (DAT_004f0588 != 6) {
      *(undefined4 *)((int)this + 0x84c) = 0x41200000;
    }
  }
  else if (DAT_004f0588 == 6) {
    if ((_DAT_00509aa4 < -0.5) || (DAT_00509860 != '\0')) {
      *(float *)((int)this + 0x84c) = *(float *)((int)this + 0x84c) - param_2 * 20.0;
    }
    else if ((0.5 < _DAT_00509aa4) || (DAT_0050985e != '\0')) {
      *(float *)((int)this + 0x84c) = param_2 * 20.0 + *(float *)((int)this + 0x84c);
    }
    else {
      iStack_5c = (int)fStack_60._0_2_;
      *(float *)((int)this + 0x84c) = (float)iStack_5c * param_2 + *(float *)((int)this + 0x84c);
    }
  }
  pfStack_80 = &fStack_40;
  *(undefined1 *)((int)this + 0x6e4) = 0;
  (**(code **)(*(int *)((int)this + 0xc0) + 0x310))();
  fStack_48 = 1.0;
  fStack_18 = 0.0;
  cVar5 = (**(code **)(*(int *)((int)this + 0xc0) + 0x218))();
  if (cVar5 == '\0') {
    (**(code **)(*(int *)this + 0x150))(&DAT_004ef3e8);
  }
  fVar1 = unaff_retaddr + *(float *)((int)this + 0x9b8);
  _Str2 = (char *)((int)this + 0x9bc);
  *(float *)((int)this + 0x9b8) = fVar1;
  if (fVar1 <= 10.0) {
    if (*_Str2 != '\0') {
      iVar8 = __strcmpi(s_SCRATCH_004f0660,_Str2);
      if ((iVar8 == 0) && (2.5 < *(float *)((int)this + 0x9b8))) {
        *_Str2 = '\0';
        *(undefined4 *)((int)this + 0x9b8) = 0;
      }
      iVar8 = __strcmpi(s_BUTTONS_004f0650,_Str2);
      if ((iVar8 == 0) && (3.0 < *(float *)((int)this + 0x9b8))) {
        *_Str2 = '\0';
        *(undefined4 *)((int)this + 0x9b8) = 0;
      }
      iVar8 = __strcmpi(&DAT_004f0658,_Str2);
      if ((iVar8 == 0) && (4.8 < *(float *)((int)this + 0x9b8))) {
        *_Str2 = '\0';
        goto LAB_00438ef1;
      }
    }
  }
  else if (*_Str2 == '\0') {
    uVar11 = *DAT_00509a40 * 0x19660d + 0x7fff;
    *DAT_00509a40 = uVar11;
    fStack_60 = (float)((int)((uVar11 >> 0x10) * 0x3e9) >> 0x10);
    fVar1 = (float)(int)fStack_60 * 0.001;
    if (0.2 <= fVar1) {
      if (0.4 <= fVar1) {
        if (0.7 <= fVar1) {
          uVar11 = 0xffffffff;
          pcVar13 = s_BUTTONS_004f0650;
          do {
            pcVar14 = pcVar13;
            if (uVar11 == 0) break;
            uVar11 = uVar11 - 1;
            pcVar14 = pcVar13 + 1;
            cVar5 = *pcVar13;
            pcVar13 = pcVar14;
          } while (cVar5 != '\0');
          uVar11 = ~uVar11;
          pcVar13 = pcVar14 + -uVar11;
          pcVar14 = _Str2;
          for (uVar12 = uVar11 >> 2; uVar12 != 0; uVar12 = uVar12 - 1) {
            *(undefined4 *)pcVar14 = *(undefined4 *)pcVar13;
            pcVar13 = pcVar13 + 4;
            pcVar14 = pcVar14 + 4;
          }
          for (uVar11 = uVar11 & 3; uVar11 != 0; uVar11 = uVar11 - 1) {
            *pcVar14 = *pcVar13;
            pcVar13 = pcVar13 + 1;
            pcVar14 = pcVar14 + 1;
          }
          FUN_00458980(0xffffffff,100,0);
        }
        else {
          uVar11 = 0xffffffff;
          pcVar13 = &DAT_004f0658;
          do {
            pcVar14 = pcVar13;
            if (uVar11 == 0) break;
            uVar11 = uVar11 - 1;
            pcVar14 = pcVar13 + 1;
            cVar5 = *pcVar13;
            pcVar13 = pcVar14;
          } while (cVar5 != '\0');
          uVar11 = ~uVar11;
          pcVar13 = pcVar14 + -uVar11;
          pcVar14 = _Str2;
          for (uVar12 = uVar11 >> 2; uVar12 != 0; uVar12 = uVar12 - 1) {
            *(undefined4 *)pcVar14 = *(undefined4 *)pcVar13;
            pcVar13 = pcVar13 + 4;
            pcVar14 = pcVar14 + 4;
          }
          for (uVar11 = uVar11 & 3; uVar11 != 0; uVar11 = uVar11 - 1) {
            *pcVar14 = *pcVar13;
            pcVar13 = pcVar13 + 1;
            pcVar14 = pcVar14 + 1;
          }
        }
      }
      else {
        uVar11 = 0xffffffff;
        pcVar13 = s_SCRATCH_004f0660;
        do {
          pcVar14 = pcVar13;
          if (uVar11 == 0) break;
          uVar11 = uVar11 - 1;
          pcVar14 = pcVar13 + 1;
          cVar5 = *pcVar13;
          pcVar13 = pcVar14;
        } while (cVar5 != '\0');
        uVar11 = ~uVar11;
        pcVar13 = pcVar14 + -uVar11;
        pcVar14 = _Str2;
        for (uVar12 = uVar11 >> 2; uVar12 != 0; uVar12 = uVar12 - 1) {
          *(undefined4 *)pcVar14 = *(undefined4 *)pcVar13;
          pcVar13 = pcVar13 + 4;
          pcVar14 = pcVar14 + 4;
        }
        for (uVar11 = uVar11 & 3; uVar11 != 0; uVar11 = uVar11 - 1) {
          *pcVar14 = *pcVar13;
          pcVar13 = pcVar13 + 1;
          pcVar14 = pcVar14 + 1;
        }
        FUN_00458980(0xffffffff,0x76,0);
      }
    }
    if (*_Str2 != '\0') {
      (**(code **)(*(int *)this + 0x150))(_Str2);
    }
LAB_00438ef1:
    *(undefined4 *)((int)this + 0x9b8) = 0;
  }
  *(undefined4 *)((int)this + 0x714) = *(undefined4 *)(DAT_00509a50 + 0x44);
  *(undefined4 *)((int)this + 0x718) = *(undefined4 *)(DAT_00509a50 + 0x48);
  *(undefined4 *)((int)this + 0x71c) = *(undefined4 *)(DAT_00509a50 + 0x4c);
  pfVar7 = (float *)FUN_00476f10(&uStack_44,0,0,0x42c80000);
  fStack_24 = *pfVar7;
  fStack_20 = pfVar7[1];
  fStack_1c = pfVar7[2];
  fStack_18 = pfVar7[3];
  OMedia3DVector::angles
            ((OMedia3DVector *)&fStack_24,(short *)&stack0xffffff92,(short *)&stack0xffffff90);
  *(undefined2 *)((int)this + 0x82a) = 0;
  *(undefined1 *)((int)this + 0x850) = 0;
  if (((((fStack_64._0_2_ < -0x1e) || (DAT_00509860 != '\0')) || (_DAT_00509aa4 < -0.5)) &&
      (((DAT_004f0588 != 3 && (DAT_004f0588 != -1)) ||
       (cVar5 = (**(code **)(*(int *)this + 0x15c))(), cVar5 == '\0')))) &&
     (*(char *)((int)this + 0x864) == '\0')) {
    *(undefined4 *)((int)this + 0x9b8) = 0;
    *_Str2 = '\0';
    *(undefined1 *)((int)this + 0x850) = 1;
    if (*(char *)((int)this + 0x828) != '\0') {
      sStack_68 = 0;
    }
  }
  fStack_60 = (float)(int)sStack_68;
  fStack_6c = (float)(int)fStack_60 *
              (1.05 - *(float *)((int)this + 0x6c8) / *(float *)((int)this + 0x6c4));
  if (1.0 < fStack_6c) {
    *(undefined2 *)((int)this + 0x82a) = 0xffff;
    *(undefined4 *)((int)this + 0x878) = 0x3dcccccd;
    *(undefined4 *)((int)this + 0x9b8) = 0;
    *_Str2 = '\0';
  }
  if (fStack_6c < -1.0) {
    *(undefined2 *)((int)this + 0x82a) = 1;
    *(undefined4 *)((int)this + 0x87c) = 0x3dcccccd;
    *(undefined4 *)((int)this + 0x9b8) = 0;
    *_Str2 = '\0';
  }
  bVar4 = fStack_6c < -1.0 || 1.0 < fStack_6c;
  if (fStack_6c <= 25.0) {
    if (fStack_6c < -25.0) {
      bVar4 = true;
      fStack_6c = -25.0;
      *(undefined2 *)((int)this + 0x82a) = 1;
      *(undefined4 *)((int)this + 0x87c) = 0x3f000000;
    }
  }
  else {
    bVar4 = true;
    fStack_6c = 25.0;
    *(undefined2 *)((int)this + 0x82a) = 0xffff;
    *(undefined4 *)((int)this + 0x878) = 0x3f000000;
  }
  if (*(short *)((int)this + 0x82a) == 0) {
    if (0.0 < *(float *)((int)this + 0x87c)) {
      *(float *)((int)this + 0x87c) = *(float *)((int)this + 0x87c) - unaff_retaddr;
    }
    if (*(float *)((int)this + 0x87c) <= 0.0) {
      *(undefined4 *)((int)this + 0x87c) = 0;
    }
    else {
      *(undefined2 *)((int)this + 0x82a) = 1;
    }
    if (0.0 < *(float *)((int)this + 0x878)) {
      *(float *)((int)this + 0x878) = *(float *)((int)this + 0x878) - unaff_retaddr;
    }
    if (*(float *)((int)this + 0x878) <= 0.0) {
      *(undefined4 *)((int)this + 0x878) = 0;
    }
    else {
      *(undefined2 *)((int)this + 0x82a) = 0xffff;
    }
  }
  *(undefined4 *)((int)this + 0x870) = 0;
  if ((DAT_0050985d != '\0') || (_DAT_00509aa0 < -0.5)) {
    fStack_6c = fStack_6c + 20.0;
    *(undefined4 *)((int)this + 0x870) = 0xc47a0000;
    *(undefined2 *)((int)this + 0x82a) = 0xffff;
    bVar4 = true;
    *(undefined1 *)((int)this + 0x6e6) = 1;
    *(undefined4 *)((int)this + 0x9b8) = 0;
    *_Str2 = '\0';
    if ((*(float *)((int)this + 0x6c8) < *(float *)((int)this + 0x6c4)) &&
       (fVar1 = *(float *)((int)this + 0x6bc) + *(float *)((int)this + 0x6c8),
       *(float *)((int)this + 0x6c8) = fVar1, *(float *)((int)this + 0x6c4) < fVar1)) {
      *(undefined4 *)((int)this + 0x6c8) = *(undefined4 *)((int)this + 0x6c4);
    }
  }
  else if ((DAT_0050985f != '\0') || (0.5 < _DAT_00509aa0)) {
    fStack_6c = fStack_6c - 20.0;
    *(undefined4 *)((int)this + 0x870) = 0x447a0000;
    bVar4 = true;
    *(undefined2 *)((int)this + 0x82a) = 1;
    *(undefined1 *)((int)this + 0x6e6) = 1;
    *(undefined4 *)((int)this + 0x9b8) = 0;
    *_Str2 = '\0';
    if ((*(float *)((int)this + 0x6c8) < *(float *)((int)this + 0x6c4)) &&
       (fVar1 = *(float *)((int)this + 0x6bc) + *(float *)((int)this + 0x6c8),
       *(float *)((int)this + 0x6c8) = fVar1, *(float *)((int)this + 0x6c4) < fVar1)) {
      *(undefined4 *)((int)this + 0x6c8) = *(undefined4 *)((int)this + 0x6c4);
    }
  }
  if ((DAT_0050985e != '\0') || (0.5 < _DAT_00509aa4)) {
    *(undefined1 *)((int)this + 0x6e4) = 1;
    bVar4 = true;
    *(undefined4 *)((int)this + 0x9b8) = 0;
    *_Str2 = '\0';
    if (fStack_6c != 0.0) {
      fStack_6c = fStack_6c * 0.5;
    }
    if ((*(float *)((int)this + 0x6c8) < *(float *)((int)this + 0x6c4)) &&
       (fVar1 = *(float *)((int)this + 0x6bc) + *(float *)((int)this + 0x6c8),
       *(float *)((int)this + 0x6c8) = fVar1, *(float *)((int)this + 0x6c4) < fVar1)) {
      *(undefined4 *)((int)this + 0x6c8) = *(undefined4 *)((int)this + 0x6c4);
    }
  }
  if (*(char *)((int)this + 0x850) == '\0') {
    *(undefined4 *)((int)this + 0x830) = 0;
  }
  else {
    *(undefined1 *)((int)this + 0x6e4) = 1;
    *(undefined4 *)((int)this + 0x9b8) = 0;
    *_Str2 = '\0';
    if (*(char *)((int)this + 0x828) == '\0') {
      fVar1 = *(float *)((int)this + 0x6c8) - *(float *)((int)this + 0x6c0) * 1.5;
      *(float *)((int)this + 0x6c8) = fVar1;
      fStack_60 = *(float *)((int)this + 0x6c4) * -0.5;
      if (fVar1 < fStack_60) {
        *(float *)((int)this + 0x6c8) = fStack_60;
      }
      if (1.5 < *(float *)((int)this + 0x830)) {
        *(undefined4 *)((int)this + 0x6c8) = *(undefined4 *)((int)this + 0x6c4);
        *(undefined1 *)((int)this + 0x828) = 1;
        iVar8 = (**(code **)(*(int *)((int)this + 0xc0) + 0x328))(&uStack_44);
        fStack_6c = 0.0;
        bVar4 = true;
        *(float *)((int)this + 0x710) = *(float *)(iVar8 + 4) + 180.0;
        goto LAB_004393fb;
      }
    }
    else {
      *(undefined4 *)((int)this + 0x6c8) = *(undefined4 *)((int)this + 0x6c4);
    }
    bVar4 = true;
  }
LAB_004393fb:
  if ((*(char *)((int)this + 0x828) != '\0') && (*(char *)((int)this + 0x850) == '\0')) {
    *(undefined1 *)((int)this + 0x828) = 0;
  }
  if (*(char *)((int)this + 0x6e4) == '\0') {
    if ((0.0 < *(float *)((int)this + 0x6c8)) &&
       (fVar1 = *(float *)((int)this + 0x6c8) - *(float *)((int)this + 0x6c0),
       *(float *)((int)this + 0x6c8) = fVar1, fVar1 < 0.0)) {
      *(undefined4 *)((int)this + 0x6c8) = 0;
    }
    if ((*(float *)((int)this + 0x6c8) < 0.0) &&
       (fVar1 = *(float *)((int)this + 0x6c0) + *(float *)((int)this + 0x6c8),
       *(float *)((int)this + 0x6c8) = fVar1, 0.0 <= fVar1)) {
      *(undefined4 *)((int)this + 0x6c8) = 0;
    }
  }
  if ((DAT_004f0588 == 6) && ((DAT_00509834 != '\0' || (DAT_00509ac4 != '\0')))) {
    *(undefined4 *)((int)this + 0x6c8) = 0;
  }
  if (*(float *)((int)this + 0x6c8) < 0.0) {
    (**(code **)(*(int *)this + 0x150))(&DAT_004f0648);
  }
  if ((*(char *)((int)this + 0x850) == '\0') && (1 < SUB42(fStack_64,0))) {
    fStack_60 = (float)((int)SUB42(fStack_64,0) << 4);
    fVar1 = (float)(int)fStack_60 + *(float *)((int)this + 0x6c8);
    *(float *)((int)this + 0x6c8) = fVar1;
    if (*(float *)((int)this + 0x6c4) < fVar1) {
      *(undefined4 *)((int)this + 0x6c8) = *(undefined4 *)((int)this + 0x6c4);
    }
LAB_00439526:
    if (*(char *)((int)this + 0x828) == '\0') {
      iVar8 = *(int *)((int)this + 0xc0);
      iVar9 = (**(code **)(iVar8 + 0x328))(&uStack_44,0);
      (**(code **)(iVar8 + 0x36c))(0,unaff_EBX + *(float *)(iVar9 + 4));
    }
    else if (fStack_6c == 0.0) {
      (**(code **)(*(int *)((int)this + 0xc0) + 0x36c))(0,*(undefined4 *)((int)this + 0x710),0);
    }
    else {
      iVar8 = (**(code **)(*(int *)((int)this + 0xc0) + 0x328))(&uStack_44);
      fStack_64 = *(float *)(iVar8 + 4) - unaff_EBX * 0.5;
      *(float *)((int)this + 0x710) = fStack_64;
      (**(code **)(*(int *)((int)this + 0xc0) + 0x36c))(0,fStack_64,0);
    }
  }
  else if (bVar4) goto LAB_00439526;
  if (*(char *)((int)this + 0x828) == '\0') {
LAB_00439625:
    pfVar7 = (float *)(**(code **)(*(int *)((int)this + 0xc0) + 900))
                                (&uStack_44,*(undefined4 *)((int)this + 0x7c8),
                                 *(float *)((int)this + 0x7cc) -
                                 (*(float *)((int)this + 0x84c) + *(float *)((int)this + 0x84c)),
                                 *(undefined4 *)((int)this + 2000));
    fStack_54 = *pfVar7;
    fStack_50 = pfVar7[1];
    fStack_4c = pfVar7[2];
    fStack_48 = pfVar7[3];
    *(float *)((int)this + 0x804) = fStack_54;
    *(float *)((int)this + 0x808) = fStack_50;
    *(float *)((int)this + 0x80c) = fStack_4c;
    *(float *)((int)this + 0x810) = fStack_48;
  }
  else {
    if (*(float *)((int)this + 0x830) <= 1.7) {
      if (*(char *)((int)this + 0x828) == '\0') goto LAB_00439625;
      pfVar7 = (float *)((int)this + 0x804);
    }
    else {
      pfVar7 = (float *)(**(code **)(*(int *)((int)this + 0xc0) + 900))
                                  (&uStack_44,*(undefined4 *)((int)this + 0x7c8),
                                   *(undefined4 *)((int)this + 0x7cc),
                                   -*(float *)((int)this + 2000) * 3.7);
    }
    fStack_54 = *pfVar7;
    fStack_50 = pfVar7[1];
    fStack_4c = pfVar7[2];
    fStack_48 = pfVar7[3];
  }
  puVar10 = (undefined4 *)
            (**(code **)(*(int *)((int)this + 0xc0) + 900))
                      (&uStack_44,*(undefined4 *)((int)this + 0x7d8),
                       *(float *)((int)this + 0x84c) * 1.9 + *(float *)((int)this + 0x7dc),
                       *(undefined4 *)((int)this + 0x7e0));
  uStack_44 = *puVar10;
  iVar8 = *(int *)this;
  fStack_40 = (float)puVar10[1];
  uStack_3c = puVar10[2];
  fStack_38 = (float)puVar10[3];
  FUN_00409f60(&uStack_44);
  fVar15 = (float10)(**(code **)(iVar8 + 0x16c))();
  iVar8 = DAT_00509a50;
  fVar1 = (float)fVar15;
  fVar2 = unaff_ESI - *(float *)((int)this + 0x714);
  fStack_64 = unaff_EBP - *(float *)((int)this + 0x718);
  fStack_60 = unaff_EBX - *(float *)((int)this + 0x71c);
  if (*(char *)((int)this + 0x851) == '\0') {
    if (*(char *)((int)this + 0x828) == '\0') {
      fVar2 = fVar2 * *(float *)((int)this + 0x854);
      fVar16 = fStack_64 * fStack_24 * (*(float *)((int)this + 0x858) + 0.2) * fVar1;
      fVar3 = fStack_60 * *(float *)((int)this + 0x85c);
    }
    else {
      fVar16 = fStack_64 * fVar1 * fStack_24;
      fVar3 = fStack_60;
    }
    fVar2 = fVar2 * fVar1 * fStack_24;
    fStack_24 = fVar3 * fVar1 * fStack_24;
  }
  else {
    fVar2 = fVar2 * fStack_24 * 0.2;
    fVar16 = fStack_64 * fStack_24 * 0.2;
    fStack_24 = fStack_60 * fStack_24 * 0.2;
  }
  *(float *)(DAT_00509a50 + 0x44) = fVar2 + *(float *)(DAT_00509a50 + 0x44);
  *(float *)(iVar8 + 0x48) = fVar16 + *(float *)(iVar8 + 0x48);
  *(float *)(iVar8 + 0x4c) = fStack_24 + *(float *)(iVar8 + 0x4c);
  fStack_38 = fStack_58 - *(float *)((int)this + 0x714);
  uStack_2c = 0;
  fStack_34 = fStack_54 - *(float *)((int)this + 0x718);
  fStack_30 = fStack_50 - *(float *)((int)this + 0x71c);
  OMedia3DVector::angles
            ((OMedia3DVector *)&fStack_38,(short *)&stack0xffffff84,(short *)&pfStack_80);
  iVar8 = DAT_00509a50;
  _DAT_004f8428 = (float)(int)*(short *)(DAT_00509a50 + 0x50);
  uVar6 = (short)pfStack_80 - *(short *)(DAT_00509a50 + 0x52) & 0x3fff;
  _DAT_004f842c = (float)(int)*(short *)(DAT_00509a50 + 0x52);
  _DAT_004f8418 = *(undefined4 *)(DAT_00509a50 + 0x44);
  _DAT_004f841c = *(undefined4 *)(DAT_00509a50 + 0x48);
  _DAT_004f8420 = *(undefined4 *)(DAT_00509a50 + 0x4c);
  if (0x2000 < uVar6) {
    uVar6 = uVar6 + 0xc000;
  }
  *(short *)(DAT_00509a50 + 0x50) =
       *(short *)(DAT_00509a50 + 0x50) + (unaff_DI - *(short *)(DAT_00509a50 + 0x50));
  *(short *)(iVar8 + 0x52) = *(short *)(iVar8 + 0x52) + uVar6;
  return;
}


```

## UpdateWalkingCameraB_00439900 @ 00439900

```c

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall UpdateWalkingCameraB_00439900(void *this,float param_2)

{
  int *piVar1;
  float fVar2;
  float fVar3;
  char cVar4;
  ushort uVar5;
  float fVar6;
  int iVar7;
  float *pfVar8;
  uint uVar9;
  int iVar10;
  float unaff_EBX;
  float unaff_EBP;
  float unaff_EDI;
  float10 fVar11;
  undefined4 uStack_c0;
  undefined1 *puStack_bc;
  float fStack_b8;
  undefined4 uStack_b4;
  undefined4 uStack_b0;
  float *pfStack_ac;
  undefined4 uStack_a8;
  undefined4 uStack_a4;
  float fStack_a0;
  float fStack_9c;
  undefined4 uStack_98;
  float *pfStack_94;
  float *pfStack_90;
  float fStack_7c;
  float fStack_70;
  float afStack_6c [2];
  float fStack_64;
  float fStack_60;
  float fStack_5c;
  float fStack_58;
  float fStack_54;
  undefined1 auStack_50 [16];
  float fStack_40;

  pfStack_90 = afStack_6c;
  pfStack_94 = &fStack_70;
  fStack_70 = 0.0;
  afStack_6c[0] = 0.0;
  uStack_98 = 0x439922;
  FUN_0046a460();
  fVar2 = (float)(int)fStack_70._0_2_;
  if (fVar2 == 0.0) {
    if ((DAT_0050985d != '\0') || (_DAT_00509aa0 < -0.5)) {
      *(undefined1 *)((int)this + 0x6e6) = 1;
      fVar2 = param_2 * 100.0 + fVar2;
    }
    else if ((DAT_0050985f != '\0') || (0.5 < _DAT_00509aa0)) {
      *(undefined1 *)((int)this + 0x6e6) = 1;
      fVar2 = fVar2 - param_2 * 100.0;
    }
    else if (*(char *)((int)this + 0x6e6) != '\0') {
      if (0.0 <= *(float *)((int)this + 0x6d4)) {
        if ((0.0 < *(float *)((int)this + 0x6d4)) &&
           (fVar6 = *(float *)((int)this + 0x6d4) - param_2 * 100.0,
           *(float *)((int)this + 0x6d4) = fVar6, fVar6 < 0.0)) {
          *(undefined4 *)((int)this + 0x6d4) = 0;
        }
      }
      else {
        fVar6 = param_2 * 100.0 + *(float *)((int)this + 0x6d4);
        *(float *)((int)this + 0x6d4) = fVar6;
        if (0.0 < fVar6) {
          *(undefined4 *)((int)this + 0x6d4) = 0;
        }
      }
    }
  }
  else {
    fVar2 = fVar2 * param_2;
    *(undefined1 *)((int)this + 0x6e6) = 0;
  }
  fVar6 = (float)(int)afStack_6c[0]._0_2_;
  *(float *)((int)this + 0x6d4) = fVar2 + *(float *)((int)this + 0x6d4);
  *(undefined1 *)((int)this + 0x6e4) = 0;
  *(undefined1 *)((int)this + 0x6e5) = 0;
  fVar2 = (1.0 / param_2) * (float)(int)fVar6;
  if (((DAT_0050985e != '\0') || (0.5 < _DAT_00509aa4)) || (40.0 < fVar2)) {
LAB_00439ae6:
    *(undefined1 *)((int)this + 0x6e4) = 1;
    if (*(float *)((int)this + 0x6c8) < *(float *)((int)this + 0x6c4)) {
      if ((DAT_0050985e != '\0') || (0.5 < _DAT_00509aa4)) {
LAB_00439b38:
        fVar3 = *(float *)((int)this + 0x6bc);
      }
      else {
        fVar3 = fVar2;
        if (*(short *)((int)this + 0x7c4) == 2) {
          pfStack_90 = (float *)0x439b2e;
          cVar4 = (**(code **)(*(int *)this + 0x15c))();
          if (cVar4 != '\0') goto LAB_00439b38;
        }
      }
      *(float *)((int)this + 0x6c8) = fVar3 + *(float *)((int)this + 0x6c8);
      if (*(float *)((int)this + 0x6c4) < *(float *)((int)this + 0x6c8)) {
        *(undefined4 *)((int)this + 0x6c8) = *(undefined4 *)((int)this + 0x6c4);
      }
    }
  }
  else if (*(short *)((int)this + 0x7c4) == 2) {
    pfStack_90 = (float *)0x439ad7;
    cVar4 = (**(code **)(*(int *)this + 0x15c))();
    if (cVar4 != '\0') goto LAB_00439ae6;
  }
  if ((*(char *)((int)this + 0x6e4) == '\0') &&
     (*(float *)((int)this + 0x6c4) * 0.90909094 < *(float *)((int)this + 0x6c8))) {
    *(float *)((int)this + 0x6c8) =
         *(float *)((int)this + 0x6c8) - *(float *)((int)this + 0x6c0) * 0.5;
  }
  piVar1 = (int *)((int)this + 0xc0);
  pfStack_90 = (float *)0x439bb6;
  cVar4 = (**(code **)(*(int *)((int)this + 0xc0) + 0x218))();
  if (cVar4 != '\0') {
    if (*(float *)((int)this + 0x6c8) <= 0.0) {
      *(undefined4 *)((int)this + 0x6c8) = 0;
    }
    else {
      *(float *)((int)this + 0x6c8) =
           *(float *)((int)this + 0x6c8) - *(float *)((int)this + 0x6c0) * 0.5;
    }
  }
  if (((DAT_00509860 != '\0') || (_DAT_00509aa4 < -0.5)) || (fVar2 < -40.0)) {
    *(undefined1 *)((int)this + 0x6e5) = 1;
    if (*(float *)((int)this + 0x6c8) <= 0.0) {
      if ((*(float *)((int)this + 0x6c8) < 0.0) &&
         (fVar2 = *(float *)((int)this + 0x6c0) + *(float *)((int)this + 0x6c8),
         *(float *)((int)this + 0x6c8) = fVar2, 0.0 < fVar2)) goto LAB_00439cac;
    }
    else {
      pfStack_90 = (float *)0x439c41;
      cVar4 = (**(code **)(*piVar1 + 0x218))();
      if (cVar4 == '\0') {
        fVar2 = *(float *)((int)this + 0x6c8) - *(float *)((int)this + 0x6c0) * 0.5;
      }
      else {
        fVar2 = *(float *)((int)this + 0x6c8) - *(float *)((int)this + 0x6c0);
      }
      *(float *)((int)this + 0x6c8) = fVar2;
      if (fVar2 < 0.0) {
LAB_00439cac:
        *(undefined4 *)((int)this + 0x6c8) = 0;
      }
    }
  }
  if (*(short *)((int)this + 0x7c4) == 2) {
    if (-50.0 <= *(float *)((int)this + 0x6d4)) {
      if (50.0 < *(float *)((int)this + 0x6d4)) {
        *(undefined4 *)((int)this + 0x6d4) = 0x42480000;
      }
    }
    else {
      *(undefined4 *)((int)this + 0x6d4) = 0xc2480000;
    }
    if (*(char *)((int)this + 0x6e4) == '\0') {
      pfStack_90 = (float *)(*(float *)((int)this + 0x6d4) * -0.25);
      iVar10 = *piVar1;
      if (*(char *)((int)this + 0x6e5) == '\0') {
        pfStack_94 = (float *)auStack_50;
        uStack_98 = 0x439d78;
        iVar7 = (**(code **)(iVar10 + 0x328))();
        uStack_98 = *(undefined4 *)(iVar7 + 4);
LAB_00439e2a:
        fStack_9c = 0.0;
      }
      else {
        pfStack_94 = (float *)auStack_50;
        uStack_98 = 0x439d5b;
        iVar7 = (**(code **)(iVar10 + 0x328))();
        uStack_98 = *(undefined4 *)(iVar7 + 4);
        fStack_9c = *(float *)((int)this + 0x7f0);
      }
    }
    else {
      pfStack_90 = (float *)(*(float *)((int)this + 0x6d4) * -0.25);
      iVar10 = *piVar1;
      pfStack_94 = (float *)auStack_50;
      uStack_98 = 0x439d22;
      iVar7 = (**(code **)(iVar10 + 0x328))();
      uStack_98 = *(undefined4 *)(iVar7 + 4);
      fStack_9c = *(float *)((int)this + 0x7ec);
    }
  }
  else {
    if (-30.0 <= *(float *)((int)this + 0x6d4)) {
      if (30.0 < *(float *)((int)this + 0x6d4)) {
        *(undefined4 *)((int)this + 0x6d4) = 0x41f00000;
      }
    }
    else {
      *(undefined4 *)((int)this + 0x6d4) = 0xc1f00000;
    }
    if (*(char *)((int)this + 0x6e4) == '\0') {
      iVar10 = *piVar1;
      pfStack_90 = (float *)-*(float *)((int)this + 0x6d4);
      if (*(char *)((int)this + 0x6e5) == '\0') {
        pfStack_94 = (float *)auStack_50;
        uStack_98 = 0x439e26;
        iVar7 = (**(code **)(iVar10 + 0x328))();
        uStack_98 = *(undefined4 *)(iVar7 + 4);
        goto LAB_00439e2a;
      }
      pfStack_94 = (float *)auStack_50;
      uStack_98 = 0x439e0c;
      iVar7 = (**(code **)(iVar10 + 0x328))();
      uStack_98 = *(undefined4 *)(iVar7 + 4);
      fStack_9c = *(float *)((int)this + 0x7f0);
    }
    else {
      iVar10 = *piVar1;
      pfStack_90 = (float *)-*(float *)((int)this + 0x6d4);
      pfStack_94 = (float *)auStack_50;
      uStack_98 = 0x439dda;
      iVar7 = (**(code **)(iVar10 + 0x328))();
      uStack_98 = *(undefined4 *)(iVar7 + 4);
      fStack_9c = *(float *)((int)this + 0x7ec);
    }
  }
  fStack_a0 = 6.20973e-39;
  (**(code **)(iVar10 + 0x36c))();
  uStack_a4 = *(undefined4 *)((int)this + 0x7cc);
  fStack_60 = *(float *)(DAT_00509a50 + 0x44);
  fStack_5c = *(float *)(DAT_00509a50 + 0x48);
  fStack_58 = *(float *)(DAT_00509a50 + 0x4c);
  fStack_a0 = *(float *)((int)this + 2000);
  uStack_a8 = *(undefined4 *)((int)this + 0x7c8);
  pfStack_ac = &fStack_70;
  uStack_b0 = 0x439e71;
  pfVar8 = (float *)(**(code **)(*(int *)this + 0x120))();
  fStack_60 = *pfVar8;
  fStack_5c = pfVar8[1];
  fStack_58 = pfVar8[2];
  uStack_b4 = *(undefined4 *)((int)this + 0x7dc);
  fStack_54 = pfVar8[3];
  uStack_b0 = *(undefined4 *)((int)this + 0x7e0);
  fStack_b8 = *(float *)((int)this + 0x7d8);
  puStack_bc = &stack0xffffff80;
  uStack_c0 = (float *)0x439eb0;
  pfVar8 = (float *)(**(code **)(*piVar1 + 900))();
  fStack_60 = *pfVar8;
  iVar10 = *(int *)this;
  fStack_5c = pfVar8[1];
  fStack_58 = pfVar8[2];
  uStack_c0 = &fStack_70;
  fStack_54 = pfVar8[3];
  FUN_00409f60(&fStack_60);
  fVar11 = (float10)(**(code **)(iVar10 + 0x16c))();
  puStack_bc = (undefined1 *)(float)fVar11;
  fStack_a0 = unaff_EBX - (float)pfStack_90;
  fStack_9c = fStack_7c - unaff_EDI;
  if (*(char *)((int)this + 0x851) != '\0') {
    fStack_b8 = (unaff_EBP - (float)pfStack_94) * fStack_40 * 0.1;
    fVar2 = fStack_a0 * fStack_40 * 0.1;
    puStack_bc = (undefined1 *)(fStack_9c * fStack_40 * 0.1);
    goto LAB_0043a06c;
  }
  fStack_b8 = (unaff_EBP - (float)pfStack_94) * fStack_40 * (float)puStack_bc;
  if (*(short *)((int)this + 0x7c4) != 2) {
    fStack_b8 = fStack_b8 * 1.2;
    fVar2 = fStack_a0 * (float)puStack_bc * fStack_40;
    fVar2 = fVar2 + fVar2;
    puStack_bc = (undefined1 *)(fStack_9c * fStack_40 * (float)puStack_bc * 1.2);
    goto LAB_0043a06c;
  }
  fStack_b8 = fStack_b8 * 1.8;
  pfStack_ac = *(float **)(DAT_00509a50 + 0x48);
  (**(code **)(*piVar1 + 0x310))(&fStack_54);
  uVar9 = __ftol();
  if ((int)((uVar9 ^ (int)uVar9 >> 0x1f) - ((int)uVar9 >> 0x1f)) < 0x321) {
    pfStack_ac = *(float **)(DAT_00509a50 + 0x48);
    iVar10 = (**(code **)(*piVar1 + 0x310))(&fStack_54);
    if ((float)pfStack_ac < *(float *)(iVar10 + 4) + 20.0) goto LAB_0043a010;
    uVar9 = __ftol();
    fVar2 = fStack_a0 * fStack_40 * (float)puStack_bc;
    if ((int)((uVar9 ^ (int)uVar9 >> 0x1f) - ((int)uVar9 >> 0x1f)) < 0x227) {
      fVar2 = fVar2 * 0.2;
    }
    else {
      fVar2 = fVar2 * 0.8;
    }
  }
  else {
LAB_0043a010:
    fVar2 = fStack_a0 * fStack_40 * (float)puStack_bc * 1.5;
  }
  puStack_bc = (undefined1 *)(fStack_9c * fStack_40 * (float)puStack_bc * 1.8);
LAB_0043a06c:
  iVar10 = DAT_00509a50;
  *(float *)(DAT_00509a50 + 0x44) = fStack_b8 + *(float *)(DAT_00509a50 + 0x44);
  *(float *)(iVar10 + 0x48) = fVar2 + *(float *)(iVar10 + 0x48);
  *(float *)(iVar10 + 0x4c) = (float)puStack_bc + *(float *)(iVar10 + 0x4c);
  fStack_64 = fVar6 - (float)pfStack_94;
  fStack_58 = 0.0;
  fStack_60 = fStack_70 - (float)pfStack_90;
  fStack_5c = afStack_6c[0] - unaff_EDI;
  OMedia3DVector::angles
            ((OMedia3DVector *)&fStack_64,(short *)&uStack_a8,(short *)((int)&uStack_c0 + 2));
  iVar10 = DAT_00509a50;
  uVar5 = uStack_c0._2_2_ - *(short *)(DAT_00509a50 + 0x52) & 0x3fff;
  if (0x2000 < uVar5) {
    uVar5 = uVar5 + 0xc000;
  }
  *(short *)(DAT_00509a50 + 0x50) =
       *(short *)(DAT_00509a50 + 0x50) + ((short)uStack_a8 - *(short *)(DAT_00509a50 + 0x50));
  *(short *)(iVar10 + 0x52) = *(short *)(iVar10 + 0x52) + uVar5;
  return;
}


```

## UpdateSittingOrSmoothCamera_0043a120 @ 0043a120

```c

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall UpdateSittingOrSmoothCamera_0043a120(void *this,float param_2)

{
  int iVar1;
  float fVar2;
  float fVar3;
  ushort uVar4;
  float *pfVar5;
  float unaff_ESI;
  float unaff_EDI;
  float10 fVar6;
  short sStack_96;
  float *pfStack_94;
  undefined1 *puStack_90;
  undefined4 uStack_8c;
  undefined4 uStack_88;
  float fStack_84;
  undefined1 *puStack_80;
  float fStack_7c;
  float *pfStack_78;
  float *pfStack_74;
  float fStack_64;
  float fStack_60;
  float fStack_5c;
  int iStack_58;
  float fStack_54;
  float fStack_50;
  float fStack_4c;
  float fStack_48;
  float fStack_44;
  float fStack_40;
  float fStack_3c;
  float fStack_30;
  undefined1 auStack_20 [16];
  undefined1 auStack_10 [16];

  pfStack_74 = &fStack_5c;
  pfStack_78 = &fStack_60;
  fStack_60 = 0.0;
  fStack_5c = 0.0;
  fStack_7c = 6.210825e-39;
  FUN_0046a460();
  iStack_58 = (int)fStack_60._0_2_;
  fVar2 = (float)iStack_58;
  if (fVar2 == 0.0) {
    if ((DAT_0050985d != '\0') || (_DAT_00509aa0 < -0.5)) {
      *(undefined1 *)((int)this + 0x6e6) = 1;
      fVar2 = param_2 * 100.0 + fVar2;
    }
    else if ((DAT_0050985f != '\0') || (0.5 < _DAT_00509aa0)) {
      *(undefined1 *)((int)this + 0x6e6) = 1;
      fVar2 = fVar2 - param_2 * 100.0;
    }
    else if (*(char *)((int)this + 0x6e6) != '\0') {
      if (0.0 <= *(float *)((int)this + 0x6d4)) {
        if ((0.0 < *(float *)((int)this + 0x6d4)) &&
           (fVar3 = *(float *)((int)this + 0x6d4) - param_2 * 100.0,
           *(float *)((int)this + 0x6d4) = fVar3, fVar3 < 0.0)) {
          *(undefined4 *)((int)this + 0x6d4) = 0;
        }
      }
      else {
        fVar3 = param_2 * 100.0 + *(float *)((int)this + 0x6d4);
        *(float *)((int)this + 0x6d4) = fVar3;
        if (0.0 < fVar3) {
          *(undefined4 *)((int)this + 0x6d4) = 0;
        }
      }
    }
  }
  else {
    fVar2 = fVar2 * param_2;
    *(undefined1 *)((int)this + 0x6e6) = 0;
  }
  pfStack_74 = *(float **)((int)this + 2000);
  pfStack_78 = *(float **)((int)this + 0x7cc);
  fStack_7c = *(float *)((int)this + 0x7c8);
  puStack_80 = auStack_10;
  *(float *)((int)this + 0x6d4) = fVar2 + *(float *)((int)this + 0x6d4);
  fStack_50 = *(float *)(DAT_00509a50 + 0x44);
  fStack_4c = *(float *)(DAT_00509a50 + 0x48);
  fStack_48 = *(float *)(DAT_00509a50 + 0x4c);
  fStack_84 = 6.211337e-39;
  pfVar5 = (float *)(**(code **)(*(int *)this + 0x120))();
  fStack_50 = *pfVar5;
  fStack_4c = pfVar5[1];
  fStack_48 = pfVar5[2];
  fStack_44 = pfVar5[3];
  fStack_84 = *(float *)((int)this + 0x7e0);
  uStack_88 = *(undefined4 *)((int)this + 0x7dc);
  uStack_8c = *(undefined4 *)((int)this + 0x7d8);
  puStack_90 = auStack_20;
  pfStack_94 = (float *)0x43a2f5;
  pfVar5 = (float *)(**(code **)(*(int *)((int)this + 0xc0) + 900))();
  fStack_50 = *pfVar5;
  iVar1 = *(int *)this;
  fStack_4c = pfVar5[1];
  fStack_48 = pfVar5[2];
  fStack_44 = pfVar5[3];
  pfStack_94 = &fStack_60;
  FUN_00409f60(&fStack_50);
  fVar6 = (float10)(**(code **)(iVar1 + 0x16c))();
  iVar1 = DAT_00509a50;
  fStack_40 = unaff_EDI - (float)puStack_80;
  fStack_3c = unaff_ESI - fStack_7c;
  *(float *)(DAT_00509a50 + 0x44) =
       (float)(fVar6 * ((float10)(float)pfStack_74 - (float10)fStack_84) * (float10)fStack_30 *
               (float10)1.2 + (float10)*(float *)(DAT_00509a50 + 0x44));
  *(float *)(iVar1 + 0x48) =
       (float)(fVar6 * (float10)fStack_40 * (float10)fStack_30 * (float10)1.2 +
              (float10)*(float *)(iVar1 + 0x48));
  *(float *)(iVar1 + 0x4c) =
       (float)(fVar6 * (float10)fStack_3c * (float10)fStack_30 * (float10)1.2 +
              (float10)*(float *)(iVar1 + 0x4c));
  fStack_54 = fStack_64 - fStack_84;
  fStack_48 = 0.0;
  fStack_50 = fStack_60 - (float)puStack_80;
  fStack_4c = fStack_5c - fStack_7c;
  OMedia3DVector::angles((OMedia3DVector *)&fStack_54,(short *)&uStack_88,&sStack_96);
  iVar1 = DAT_00509a50;
  uVar4 = sStack_96 - *(short *)(DAT_00509a50 + 0x52) & 0x3fff;
  if (0x2000 < uVar4) {
    uVar4 = uVar4 + 0xc000;
  }
  *(short *)(DAT_00509a50 + 0x50) =
       *(short *)(DAT_00509a50 + 0x50) + ((short)uStack_88 - *(short *)(DAT_00509a50 + 0x50));
  *(short *)(iVar1 + 0x52) = *(short *)(iVar1 + 0x52) + uVar4;
  return;
}


```

## UpdateRotateToTargetDelta_0043a420 @ 0043a420

```c

void __thiscall UpdateRotateToTargetDelta_0043a420(void *this,float param_2)

{
  float fVar1;
  char cVar2;
  float fStack_18;
  float fStack_14;

  cVar2 = FUN_00475ca0();
  if (((cVar2 != '\0') && (*(char *)((int)this + 5) != '\0')) && (*(char *)((int)this + 10) != '\0')
     ) {
    *(undefined1 *)((int)this + 10) = 0;
    fStack_14 = 0.0;
    fStack_18 = 0.0;
    if (*(float *)((int)this + 0x274) != *(float *)((int)this + 0x224)) {
      fStack_14 = *(float *)((int)this + 0x274) - *(float *)((int)this + 0x224);
      if (fStack_14 <= 180.0) {
        if (fStack_14 < -180.0) {
          fStack_14 = fStack_14 + 360.0;
        }
        *(undefined1 *)((int)this + 10) = 1;
      }
      else {
        fStack_14 = fStack_14 - 360.0;
        *(undefined1 *)((int)this + 10) = 1;
      }
    }
    if (*(float *)((int)this + 0x278) != *(float *)((int)this + 0x228)) {
      fStack_18 = *(float *)((int)this + 0x278) - *(float *)((int)this + 0x228);
      if (fStack_18 <= 180.0) {
        if (fStack_18 < -180.0) {
          fStack_18 = fStack_18 + 360.0;
        }
      }
      else {
        fStack_18 = fStack_18 - 360.0;
      }
      *(undefined1 *)((int)this + 10) = 1;
    }
    fVar1 = 0.0;
    if (*(float *)((int)this + 0x27c) != *(float *)((int)this + 0x22c)) {
      fVar1 = *(float *)((int)this + 0x27c) - *(float *)((int)this + 0x22c);
      if (fVar1 <= 180.0) {
        if (fVar1 < -180.0) {
          fVar1 = fVar1 + 360.0;
        }
      }
      else {
        fVar1 = fVar1 - 360.0;
      }
      *(undefined1 *)((int)this + 10) = 1;
    }
    (**(code **)(*(int *)this + 0x334))
              (fStack_14 * param_2 * 4.5,fStack_18 * param_2 * 3.7,fVar1 * param_2 * 4.5);
  }
  return;
}


```

## ProjectNoisyCameraTarget_0043a5d0 @ 0043a5d0

```c

void __thiscall
ProjectNoisyCameraTarget_0043a5d0(void *this,float param_2,float param_3,float param_4)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  int iVar10;
  uint uVar11;
  uint uVar12;
  float *pfVar13;
  undefined1 auStack_20 [4];
  undefined4 uStack_1c;
  undefined1 auStack_14 [16];
  float *pfStack_4;

  iVar10 = (**(code **)(*(int *)((int)this + 0xc0) + 0x328))(auStack_20);
  uStack_1c = *(undefined4 *)(iVar10 + 8);
  if (*(float *)(iVar10 + 4) + *(float *)((int)this + 0x6d4) < 0.0) {
    __ftol();
  }
  uVar11 = __ftol();
  uVar12 = __ftol();
  iVar10 = (uVar12 & 0x3fff) * 4;
  fVar1 = *(float *)global_exref;
  fVar2 = *(float *)(global_exref + 0x10004);
  fVar3 = *(float *)(global_exref + iVar10);
  fVar4 = *(float *)(global_exref + iVar10 + 0x10004);
  iVar10 = (uVar11 & 0x3fff) * 4;
  fVar5 = *(float *)(global_exref + iVar10);
  fVar6 = *(float *)(global_exref + iVar10 + 0x10004);
  fVar9 = fVar6 * param_3 - fVar5 * param_2;
  pfVar13 = (float *)(**(code **)(*(int *)((int)this + 0xc0) + 0x310))(auStack_14);
  fVar7 = pfVar13[1];
  fVar8 = pfVar13[2];
  *pfStack_4 = *pfVar13 + fVar9;
  pfStack_4[1] = ((fVar5 * param_3 + fVar6 * param_2) * fVar4 -
                 (fVar2 * param_4 - fVar9 * fVar1) * fVar3) + fVar7;
  pfStack_4[2] = fVar1 * param_4 + fVar9 * fVar2 + fVar8;
  pfStack_4[3] = 1.0;
  return;
}


```

## TouchOrCollisionFallback_0043a790 @ 0043a790

```c

void __thiscall TouchOrCollisionFallback_0043a790(void *this,int *param_2)

{
  char cVar1;

  cVar1 = (**(code **)(*param_2 + 0x18))(s_C3DGODDARD_004ecc44);
  if (cVar1 == '\0') {
    CGameObject::vfunc_00_261(this);
    return;
  }
  if (DAT_005099e4 != (-(uint)(this != (void *)0xc0) & (uint)this)) {
    (**(code **)(*(int *)this + 0x2c4))(0,0,0);
  }
  return;
}


```

## GroundAheadPredicate_0043a7f0 @ 0043a7f0

```c

uint __thiscall GroundAheadPredicate_0043a7f0(void *this)

{
  uint in_EAX;
  undefined4 *puVar1;
  uint uVar2;
  undefined4 uStack_40;
  float fStack_3c;
  undefined4 uStack_38;
  undefined4 uStack_34;
  undefined4 uStack_30;
  float fStack_2c;
  undefined4 uStack_28;
  undefined4 uStack_24;
  undefined1 auStack_10 [16];

  if (*(short *)((int)this + 0x7c4) == 0) {
    puVar1 = (undefined4 *)
             (**(code **)(*(int *)((int)this + 0xc0) + 900))(auStack_10,0,0,0x428c0000);
    uStack_40 = *puVar1;
    fStack_2c = (float)puVar1[1];
    uStack_38 = puVar1[2];
    uStack_34 = puVar1[3];
    fStack_3c = fStack_2c - 2000.0;
    uStack_30 = uStack_40;
    uStack_28 = uStack_38;
    uStack_24 = uStack_34;
    FUN_00409f60(&uStack_40);
    FUN_00409f60(&uStack_30);
    in_EAX = FUN_0047c6e0();
    if ((char)in_EAX != '\0') {
      fStack_3c = (float)DAT_005cfc64;
      uStack_40 = DAT_005cfc60;
      uStack_38 = DAT_005cfc68;
      uStack_34 = DAT_005cfc6c;
      uVar2 = __ftol();
      in_EAX = (uVar2 ^ (int)uVar2 >> 0x1f) - ((int)uVar2 >> 0x1f);
      if (0x46 < (int)in_EAX) {
        return CONCAT31((int3)(in_EAX >> 8),1);
      }
    }
  }
  return in_EAX & 0xffffff00;
}


```

## SetPlayerAnimationState_0043aff0 @ 0043aff0

```c

void __thiscall SetPlayerAnimationState_0043aff0(void *this,char *param_2)

{
  char *pcVar1;
  int iVar2;
  undefined4 *puVar3;
  undefined **_Str2;
  char *pcVar4;
  undefined1 auStack_10 [16];

  pcVar1 = (char *)(**(code **)(*(int *)this + 0x100))();
  iVar2 = __strcmpi(param_2,pcVar1);
  if (iVar2 != 0) {
    _Str2 = &PTR_DAT_004ef2e4;
    pcVar1 = (char *)(**(code **)(*(int *)this + 0x100))();
    iVar2 = __strcmpi(pcVar1,(char *)_Str2);
    if (iVar2 != 0) {
      pcVar4 = s_SPLAT_004ef6e0;
      pcVar1 = (char *)(**(code **)(*(int *)this + 0x100))();
      iVar2 = __strcmpi(pcVar1,pcVar4);
      if (iVar2 != 0) {
        pcVar4 = s_FENCE_004f0670;
        pcVar1 = (char *)(**(code **)(*(int *)this + 0x100))();
        iVar2 = __strcmpi(pcVar1,pcVar4);
        if (iVar2 != 0) {
          pcVar4 = s_LADDER_004f0668;
          pcVar1 = (char *)(**(code **)(*(int *)this + 0x100))();
          iVar2 = __strcmpi(pcVar1,pcVar4);
          if (iVar2 != 0) {
            pcVar4 = s_SWING_004ef6d8;
            pcVar1 = (char *)(**(code **)(*(int *)this + 0x100))();
            iVar2 = __strcmpi(pcVar1,pcVar4);
            if (iVar2 != 0) {
              if ((*(char *)((int)this + 0x9bc) != '\0') &&
                 (iVar2 = __strcmpi(param_2,(char *)((int)this + 0x9bc)), iVar2 == 0)) {
                pcVar4 = &DAT_004ed040;
                pcVar1 = (char *)(**(code **)(*(int *)this + 0x100))();
                iVar2 = __strcmpi(pcVar1,pcVar4);
                if (iVar2 != 0) {
                  return;
                }
                (**(code **)(*(int *)this + 0xe0))((char *)((int)this + 0x9bc),1);
                return;
              }
              iVar2 = __strcmpi(param_2,&DAT_004eca64);
              if (iVar2 == 0) {
                (**(code **)(*(int *)this + 0x144))();
                (**(code **)(*(int *)this + 0xe0))(&DAT_004eca64,1);
                (**(code **)(*(int *)((int)this + 0xc0) + 0x278))(0,0,0);
                return;
              }
              pcVar4 = &DAT_004f0684;
              pcVar1 = (char *)(**(code **)(*(int *)this + 0x100))();
              iVar2 = __strcmpi(pcVar1,pcVar4);
              if ((iVar2 != 0) && (iVar2 = __strcmpi(param_2,&DAT_004f0684), iVar2 == 0)) {
                (**(code **)(*(int *)this + 0x148))();
                (**(code **)(*(int *)this + 0xe0))(&DAT_004f0684,1);
                return;
              }
              pcVar4 = &DAT_004f05ec;
              pcVar1 = (char *)(**(code **)(*(int *)this + 0x100))();
              iVar2 = __strcmpi(pcVar1,pcVar4);
              if ((iVar2 != 0) && (iVar2 = __strcmpi(param_2,&DAT_004f05ec), iVar2 == 0)) {
                (**(code **)(*(int *)this + 0xe0))(&DAT_004f05ec,0);
                return;
              }
              iVar2 = __strcmpi(param_2,s_SWING_004ef6d8);
              if (iVar2 == 0) {
                (**(code **)(*(int *)this + 0x148))();
                (**(code **)(*(int *)this + 0xe0))(s_SWING_004ef6d8,1);
                return;
              }
              iVar2 = __strcmpi(param_2,&DAT_004f0648);
              if (iVar2 == 0) {
                pcVar4 = &DAT_004f0648;
                pcVar1 = (char *)(**(code **)(*(int *)this + 0x100))();
                iVar2 = __strcmpi(pcVar1,pcVar4);
                if (iVar2 != 0) {
                  (**(code **)(*(int *)this + 0xe0))(&DAT_004f0648,1);
                  return;
                }
              }
              else if (0.0 <= *(float *)((int)this + 0x6c8)) {
                iVar2 = __strcmpi(param_2,&DAT_004ed040);
                if (iVar2 == 0) {
                  pcVar4 = s_SPLAT_004ef6e0;
                  pcVar1 = (char *)(**(code **)(*(int *)this + 0x100))();
                  iVar2 = __strcmpi(pcVar1,pcVar4);
                  if (iVar2 != 0) {
                    (**(code **)(*(int *)this + 0x148))();
                    (**(code **)(*(int *)this + 0xe0))(&DAT_004ed040,1);
                    (**(code **)(*(int *)((int)this + 0xc0) + 0x278))(0,0,0);
                    return;
                  }
                }
                iVar2 = __strcmpi(param_2,&DAT_004ef3e8);
                if (iVar2 == 0) {
                  (**(code **)(*(int *)this + 0xe0))(&DAT_004ef3e8,1);
                  (**(code **)(*(int *)this + 0x148))();
                  return;
                }
                iVar2 = __strcmpi(param_2,&DAT_004f05e4);
                if (iVar2 == 0) {
                  (**(code **)(*(int *)this + 0x148))();
                  (**(code **)(*(int *)this + 0xe0))(&DAT_004f05e4,1);
                  (**(code **)(*(int *)((int)this + 0xc0) + 0x278))(0,0,0);
                  return;
                }
                iVar2 = __strcmpi(param_2,s_RIGHT_004f05dc);
                if (iVar2 == 0) {
                  (**(code **)(*(int *)this + 0x148))();
                  (**(code **)(*(int *)this + 0xe0))(s_RIGHT_004f05dc,1);
                  (**(code **)(*(int *)((int)this + 0xc0) + 0x278))(0,0,0);
                  return;
                }
                iVar2 = __strcmpi(param_2,(char *)&PTR_DAT_004ef2e4);
                if (iVar2 == 0) {
                  (**(code **)(*(int *)this + 0xe0))(&PTR_DAT_004ef2e4,0);
                  return;
                }
                iVar2 = __strcmpi(param_2,s_FENCE_004f0670);
                if (iVar2 == 0) {
                  puVar3 = (undefined4 *)
                           (**(code **)(*(int *)((int)this + 0xc0) + 900))
                                     (auStack_10,0,0x43a00000,0x43160000);
                  *(undefined4 *)((int)this + 0x838) = *puVar3;
                  *(undefined4 *)((int)this + 0x83c) = puVar3[1];
                  *(undefined4 *)((int)this + 0x840) = puVar3[2];
                  *(undefined4 *)((int)this + 0x844) = puVar3[3];
                  (**(code **)(*(int *)((int)this + 0xc0) + 0x2c4))(0,0,0);
                  (**(code **)(*(int *)((int)this + 0xc0) + 0xa8))(0);
                  (**(code **)(*(int *)this + 0x148))();
                  DAT_004f83e0 = 1;
                  *(undefined4 *)((int)this + 0x888) = 0x40800000;
                  *(undefined4 *)((int)this + 0x868) = 0x40a00000;
                  *(undefined4 *)((int)this + 0x834) = 0;
                  (**(code **)(*(int *)this + 0xe0))(s_FENCE_004f0670,0);
                  FUN_00458980(0xffffffff,0x92,0);
                  return;
                }
                iVar2 = __strcmpi(param_2,s_LADDER_004f0668);
                if (iVar2 == 0) {
                  puVar3 = (undefined4 *)
                           (**(code **)(*(int *)((int)this + 0xc0) + 900))
                                     (auStack_10,0,0x44480000,0x43160000);
                  *(undefined4 *)((int)this + 0x838) = *puVar3;
                  *(undefined4 *)((int)this + 0x83c) = puVar3[1];
                  *(undefined4 *)((int)this + 0x840) = puVar3[2];
                  *(undefined4 *)((int)this + 0x844) = puVar3[3];
                  (**(code **)(*(int *)((int)this + 0xc0) + 0x2c4))(0,0,0);
                  (**(code **)(*(int *)((int)this + 0xc0) + 0xa8))(0);
                  (**(code **)(*(int *)this + 0x148))();
                  DAT_004f83e0 = 1;
                  *(undefined4 *)((int)this + 0x888) = 0x40c00000;
                  *(undefined4 *)((int)this + 0x868) = 0x40a00000;
                  *(undefined4 *)((int)this + 0x834) = 0;
                  (**(code **)(*(int *)this + 0xe0))(s_LADDER_004f0668,0);
                  FUN_00458980(0xffffffff,0x8f,0);
                  return;
                }
                (**(code **)(*(int *)this + 0xe0))(param_2,1);
              }
            }
          }
        }
      }
    }
  }
  return;
}


```

## PlayerLoadLevel_0043b5a0 @ 0043b5a0

```c

void __thiscall
PlayerLoadLevel_0043b5a0(void *this,char *param_2,CGameObject *param_3,char *param_4,char *param_5)

{
  char cVar1;
  undefined **ppuVar2;
  undefined4 *puVar3;
  CGameObject *this_00;
  uint uVar4;
  uint uVar5;
  CGameObject *pCVar6;
  CGameObject *this_01;
  char *pcVar7;
  char *pcVar8;
  char *pcVar9;
  int iVar10;
  char *pcVar11;
  char *pcVar12;
  char *pcVar13;
  char *pcVar14;
  char *pcVar15;
  CGameObject *pCVar16;
  char *pcVar17;
  char *pcVar18;
  undefined1 auStack_150 [16];
  char acStack_140 [80];
  char acStack_f0 [80];
  char acStack_a0 [80];
  char acStack_50 [80];

  pcVar14 = s_Calling_PlayerLoadLevel__s__s__s_004f06ec;
  pcVar15 = param_2;
  pCVar16 = param_3;
  pcVar17 = param_4;
  pcVar18 = param_5;
  CGameObject::vfunc_00_013(this);
  pcVar11 = s_MENTEMP_Calling_PlayerLoadLevel___004f06bc;
  pcVar8 = param_2;
  pCVar6 = param_3;
  pcVar12 = param_4;
  pcVar13 = param_5;
  CGameObject::vfunc_00_013(param_3);
  iVar10 = (int)this + 0x8f0;
  *(undefined1 *)((int)this + 0x7fc) = 0;
  pcVar9 = s_1__s__s_004f06b0;
  *(undefined1 *)(DAT_00509980 + 0x1fe) = 1;
  pcVar7 = param_5;
  CGameObject::vfunc_00_013(this_00);
  FUN_0047d850(pcVar9,pcVar7,iVar10,pcVar11,pcVar8,pCVar6,pcVar12,pcVar13,pcVar14,pcVar15,pCVar16,
               pcVar17,pcVar18);
  uVar4 = 0xffffffff;
  do {
    pcVar7 = param_2;
    if (uVar4 == 0) break;
    uVar4 = uVar4 - 1;
    pcVar7 = param_2 + 1;
    cVar1 = *param_2;
    param_2 = pcVar7;
  } while (cVar1 != '\0');
  uVar4 = ~uVar4;
  pcVar7 = pcVar7 + -uVar4;
  pcVar8 = acStack_140;
  for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
    *(undefined4 *)pcVar8 = *(undefined4 *)pcVar7;
    pcVar7 = pcVar7 + 4;
    pcVar8 = pcVar8 + 4;
  }
  for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *pcVar8 = *pcVar7;
    pcVar7 = pcVar7 + 1;
    pcVar8 = pcVar8 + 1;
  }
  uVar4 = 0xffffffff;
  do {
    pCVar6 = param_3;
    if (uVar4 == 0) break;
    uVar4 = uVar4 - 1;
    pCVar6 = (CGameObject *)((int)&param_3->vftable + 1);
    ppuVar2 = &param_3->vftable;
    param_3 = pCVar6;
  } while (*(char *)ppuVar2 != '\0');
  uVar4 = ~uVar4;
  pcVar7 = (char *)((int)pCVar6 - uVar4);
  pcVar8 = acStack_f0;
  for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
    *(undefined4 *)pcVar8 = *(undefined4 *)pcVar7;
    pcVar7 = pcVar7 + 4;
    pcVar8 = pcVar8 + 4;
  }
  for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *pcVar8 = *pcVar7;
    pcVar7 = pcVar7 + 1;
    pcVar8 = pcVar8 + 1;
  }
  uVar4 = 0xffffffff;
  do {
    pcVar7 = param_4;
    if (uVar4 == 0) break;
    uVar4 = uVar4 - 1;
    pcVar7 = param_4 + 1;
    cVar1 = *param_4;
    param_4 = pcVar7;
  } while (cVar1 != '\0');
  uVar4 = ~uVar4;
  pcVar7 = pcVar7 + -uVar4;
  pcVar8 = acStack_a0;
  for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
    *(undefined4 *)pcVar8 = *(undefined4 *)pcVar7;
    pcVar7 = pcVar7 + 4;
    pcVar8 = pcVar8 + 4;
  }
  for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *pcVar8 = *pcVar7;
    pcVar7 = pcVar7 + 1;
    pcVar8 = pcVar8 + 1;
  }
  uVar4 = 0xffffffff;
  do {
    pcVar7 = param_5;
    if (uVar4 == 0) break;
    uVar4 = uVar4 - 1;
    pcVar7 = param_5 + 1;
    cVar1 = *param_5;
    param_5 = pcVar7;
  } while (cVar1 != '\0');
  uVar4 = ~uVar4;
  pcVar7 = pcVar7 + -uVar4;
  pcVar8 = acStack_50;
  for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
    *(undefined4 *)pcVar8 = *(undefined4 *)pcVar7;
    pcVar7 = pcVar7 + 4;
    pcVar8 = pcVar8 + 4;
  }
  for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *pcVar8 = *pcVar7;
    pcVar7 = pcVar7 + 1;
    pcVar8 = pcVar8 + 1;
  }
  uVar4 = 0xffffffff;
  pcVar7 = acStack_f0;
  do {
    pcVar8 = pcVar7;
    if (uVar4 == 0) break;
    uVar4 = uVar4 - 1;
    pcVar8 = pcVar7 + 1;
    cVar1 = *pcVar7;
    pcVar7 = pcVar8;
  } while (cVar1 != '\0');
  uVar4 = ~uVar4;
  pcVar7 = pcVar8 + -uVar4;
  pcVar8 = (char *)((int)this + 0x724);
  for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
    *(undefined4 *)pcVar8 = *(undefined4 *)pcVar7;
    pcVar7 = pcVar7 + 4;
    pcVar8 = pcVar8 + 4;
  }
  for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *pcVar8 = *pcVar7;
    pcVar7 = pcVar7 + 1;
    pcVar8 = pcVar8 + 1;
  }
  uVar4 = 0xffffffff;
  pcVar7 = acStack_f0;
  do {
    pcVar8 = pcVar7;
    if (uVar4 == 0) break;
    uVar4 = uVar4 - 1;
    pcVar8 = pcVar7 + 1;
    cVar1 = *pcVar7;
    pcVar7 = pcVar8;
  } while (cVar1 != '\0');
  uVar4 = ~uVar4;
  pcVar7 = pcVar8 + -uVar4;
  pcVar8 = (char *)((int)this + 0x774);
  for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
    *(undefined4 *)pcVar8 = *(undefined4 *)pcVar7;
    pcVar7 = pcVar7 + 4;
    pcVar8 = pcVar8 + 4;
  }
  for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *pcVar8 = *pcVar7;
    pcVar7 = pcVar7 + 1;
    pcVar8 = pcVar8 + 1;
  }
  uVar4 = 0xffffffff;
  pcVar7 = acStack_a0;
  do {
    pcVar8 = pcVar7;
    if (uVar4 == 0) break;
    uVar4 = uVar4 - 1;
    pcVar8 = pcVar7 + 1;
    cVar1 = *pcVar7;
    pcVar7 = pcVar8;
  } while (cVar1 != '\0');
  uVar4 = ~uVar4;
  pcVar7 = pcVar8 + -uVar4;
  pcVar8 = (char *)((int)this + 0x88c);
  for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
    *(undefined4 *)pcVar8 = *(undefined4 *)pcVar7;
    pcVar7 = pcVar7 + 4;
    pcVar8 = pcVar8 + 4;
  }
  for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *pcVar8 = *pcVar7;
    pcVar7 = pcVar7 + 1;
    pcVar8 = pcVar8 + 1;
  }
  uVar4 = 0xffffffff;
  pcVar7 = acStack_50;
  do {
    pcVar8 = pcVar7;
    if (uVar4 == 0) break;
    uVar4 = uVar4 - 1;
    pcVar8 = pcVar7 + 1;
    cVar1 = *pcVar7;
    pcVar7 = pcVar8;
  } while (cVar1 != '\0');
  uVar4 = ~uVar4;
  pcVar7 = pcVar8 + -uVar4;
  pcVar8 = (char *)((int)this + 0x8f0);
  for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
    *(undefined4 *)pcVar8 = *(undefined4 *)pcVar7;
    pcVar7 = pcVar7 + 4;
    pcVar8 = pcVar8 + 4;
  }
  for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *pcVar8 = *pcVar7;
    pcVar7 = pcVar7 + 1;
    pcVar8 = pcVar8 + 1;
  }
  CGameObject::vfunc_00_013((CGameObject *)0x0);
  uVar4 = 0xffffffff;
  pcVar7 = acStack_140;
  do {
    pcVar8 = pcVar7;
    if (uVar4 == 0) break;
    uVar4 = uVar4 - 1;
    pcVar8 = pcVar7 + 1;
    cVar1 = *pcVar7;
    pcVar7 = pcVar8;
  } while (cVar1 != '\0');
  uVar4 = ~uVar4;
  pcVar7 = pcVar8 + -uVar4;
  pcVar8 = (char *)((int)this + 0x955);
  for (uVar5 = uVar4 >> 2; uVar5 != 0; uVar5 = uVar5 - 1) {
    *(undefined4 *)pcVar8 = *(undefined4 *)pcVar7;
    pcVar7 = pcVar7 + 4;
    pcVar8 = pcVar8 + 4;
  }
  for (uVar4 = uVar4 & 3; uVar4 != 0; uVar4 = uVar4 - 1) {
    *pcVar8 = *pcVar7;
    pcVar7 = pcVar7 + 1;
    pcVar8 = pcVar8 + 1;
  }
  pCVar6 = (CGameObject *)0x0;
  if (*(char *)((int)this + 0x9a5) == '\0') {
    puVar3 = (undefined4 *)(**(code **)(*(int *)((int)this + 0xc0) + 0x310))(auStack_150);
    pCVar6 = (CGameObject *)((int)this + 0x9a8);
    *(undefined4 *)((int)this + 0x9a8) = *puVar3;
    *(undefined4 *)((int)this + 0x9ac) = puVar3[1];
    *(undefined4 *)((int)this + 0x9b0) = puVar3[2];
    *(undefined4 *)((int)this + 0x9b4) = puVar3[3];
  }
  CGameObject::vfunc_00_013(pCVar6);
  (**(code **)(*DAT_00509980 + 0x100))(acStack_140);
  (**(code **)(*(int *)this + 0x148))();
  CGameObject::vfunc_00_013(this_01);
  return;
}


```

## ProbePlayerRayBlend_0043b820 @ 0043b820

```c

float10 __thiscall
ProbePlayerRayBlend_0043b820(void *this,float param_2,undefined4 param_3,float *param_4)

{
  char cVar1;
  float unaff_retaddr;
  undefined4 uStack_14;
  undefined1 auStack_10 [12];
  float fStack_4;

  uStack_14 = 0x3f800000;
  (**(code **)(*(int *)((int)this + 0xc0) + 0x310))();
  (**(code **)(*(int *)((int)this + 0xc0) + 0x310))(&uStack_14);
  FUN_00409f60(param_4);
  FUN_00409f60(&fStack_4);
  cVar1 = FUN_0047c210();
  if (cVar1 != '\0') {
    *param_4 = DAT_005cfc60;
    param_4[1] = DAT_005cfc64;
    param_4[2] = DAT_005cfc68;
    param_4[3] = DAT_005cfc6c;
    *param_4 = fStack_4 - (fStack_4 - *param_4) * 0.75;
    param_4[1] = unaff_retaddr - (unaff_retaddr - param_4[1]) * 0.75;
    param_4[2] = param_2 - (param_2 - param_4[2]) * 0.75;
    return (float10)1.5;
  }
  return (float10)(float)auStack_10;
}


```
