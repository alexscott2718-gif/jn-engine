# Camera/player-target record (`DAT_00509a50`) — consolidated layout

Recovered 2026-07-02 with `tools/ghidra/CreateFunctions.java`, `DumpFunctions.java`,
and `DumpRefs.java` against `~/ghidra-projects/JN_decomp` / `Neutron.exe`, plus raw
`objdump` disassembly of the `FrameStepAndRender` matrix block (x87/artifact check).
This consolidates the record references previously scattered across
`C3DTriggerType.md`, `CGameType.md`, `C3DObject.md`, `C3DPlayer.md`, `C3DCar.md`,
`C3DVehicle.md`, `C3DFlyingObject.md`, `C3DJimmy.md`, `CViewPort.md`, and `CEditor.md`.

## Identity

`DAT_00509a50` is not a bare struct: it is a heap-allocated, vtable'd OMedia
camera/viewport element of size **0x120**, created once by
`InitViewPort_00476490` (trace string `InitViewPort()` @ `004f6750`):

- allocation: `FUN_00478990(0x120)`; constructor `FUN_0047e310(ctx)` where
  `ctx = DAT_00509a48 + 0x14` when the window/context global `DAT_00509a48` is
  set, else `0`.
- post-ctor wiring: virtual slot `0x2c` (init), slot `0x34(DAT_00509a30)`
  (attach to world/supervisor), slot `0xb0(DAT_00509a48)` (bind context), then
  slot `0xac` three times — once with a zeroed local, then twice with
  `rec+0xe4` after setting `rec[0x3f]=1` (`+0xfc`) and `rec[0x40]=1` (`+0x100`)
  — the viewport-rect apply path.
- `DAT_00509a38` (the render/view element consumed by
  `CViewPort::FrameStepAndRender`) is created separately by `FUN_004765f0`
  (single WRITE xref at `00476651`).

The record is the single global bridge between gameplay camera logic and the
renderer: gameplay writes position/angles into it; the render loop converts the
angles into the view rotation matrix each frame.

## Field map (byte offsets from the record pointer)

| Offset | Type | Name | Evidence | Meaning |
|---:|---|---|---|---|
| `+0x00` | ptr | vftable | `InitViewPort_00476490` virtual calls | OMedia element vtable. |
| `+0x44/0x48/0x4c` | float×3 | `pos` | seeded `(0, 10000, 0)` by `CGameType::InitGame` (`00474a10`); written by `UpdateWalkingCameraA/B`, `RunTriggerTypeNextTarget` (`00447a70`), editor save/restore (`0046bcb0`) | Camera world position. |
| `+0x50/0x52/0x54` | s16×3 | `angle_x/angle_y/angle_z` | `FrameStepAndRender` (`0047e4f0`), `CameraRecordLocalToWorld_00476e10`, walking cameras | Three 14-bit angles (`16384 == 360°`), direct indexes into the `global_exref` trig table (`sin` at `+i*4`, `cos` at `+i*4+0x10004`). Order applied: X (`+0x50`), Y (`+0x52`), Z (`+0x54`). |
| `+0xd4/0xd8/0xdc/0xe0` | int×4 | `view_rect` | `FUN_00476dd0` = `(rect.right-rect.left)/2`, `FUN_00476df0` = `(rect.bottom-rect.top)/2` | Viewport rectangle (left, top, right, bottom); the two helpers return the half-extents (screen center). |
| `+0xe4..` | block | `view_rect_apply` | `InitViewPort` passes `rec+0xe4` to slot `0xac` | Sub-block handed to the viewport-rect apply virtual. |
| `+0xfc`, `+0x100` | int×2 | `init_flags` | `InitViewPort` sets both to `1` before the two `slot 0xac` calls | Enable flags for the rect-apply path. |
| `+0x114` | u16 | `mode_flags` | `CEditor` init (`0046bcb0`) sets `= 5`; editor uninit (`0046bdc0`) does `&= 0xfffa` | Camera mode bitfield: editor free-fly sets bits 0+2; gameplay leaves them clear. |

## The generic record transform — `CameraRecordLocalToWorld_00476e10`

`FUN_00476e10(float out[4], float x, float y, float z)` is the general form of
the rotate-local-offset-then-add-position math that slot 242
(`RunTriggerTypeNextTarget`) inlines with the constant offset `(20,-20,-100)`:

```c
sx = sin[rec->angle_x & 0x3fff]; cx = cos[rec->angle_x & 0x3fff];
sy = sin[rec->angle_y & 0x3fff]; cy = cos[rec->angle_y & 0x3fff];
sz = sin[rec->angle_z & 0x3fff]; cz = cos[rec->angle_z & 0x3fff];
xz = cz*z - sz*y;          // NOTE: decompiled params (param_2,param_3,param_4) = (y, z, w-ish);
yz = sz*z + cz*y;          //       see raw dump below for exact binding
t  = cx*w - xz*sx;
out.x = rec->pos.x + (yz*cy - t*sy);
out.y = rec->pos.y + sx*w + xz*cx;
out.z = rec->pos.z + yz*sy + t*cy;
out.w = 1.0;
```

`FUN_00476f10` is the direction-only variant: identical rotation chain, no
position add, `out.w = 0`.

This is the same three-axis structure as the recovered
`transform_local_00472980` (cutscene placement) and the slot-242 retarget —
one rotation convention shared across the whole original camera system.

## The per-frame view build — `CViewPort::FrameStepAndRender` (`0047e4f0`)

Now function-defined and decompiled (previously summarized as
`rebuild_view_matrix_from_camera_globals()`). Matrix objects are 68 bytes:
`{ float m[16]; u32 tag }` with observed tags `1` identity, `4` translation,
`5` rotation, `6` composed/final. Builders (all fill identity first, then):

- `BuildRotXMatrix_0047e740(angle)`: `m[5]=c, m[6]=-s, m[9]=s, m[10]=c`, tag 5.
- `BuildRotYMatrix_0047e7c0(angle)`: `m[0]=c, m[2]=s, m[8]=-s, m[10]=c`, tag 5.
- `BuildRotZMatrix_0047e830(angle)`: `m[0]=c, m[1]=-s, m[4]=s, m[5]=c`, tag 5.
- `BuildTranslationMatrix_0047e700()`: identity, tag 1 (caller then writes
  `m[12..14]` and tag 4).
- `Matrix4Multiply_0047e8b0(this=A, B, out)`: row-vector convention,
  `out = B · A` (`out[i][j] = Σk B[i][k]·A[k][j]`), clears `out` tag to 0.

Per frame, when `DAT_00509a38 != 0` (verified against raw disassembly
`47e58c..47e6a6` — the zero translation is real, not an x87 artifact):

```c
A = rec->angle_x;  B = rec->angle_y;  C = rec->angle_z;   // +0x50/+0x52/+0x54
M_rx  = RotX(-A);  M_ry = RotY(-B);  M_rz = RotZ(-C);
M4    = mul(this=M_rz, B=M_rx)      // = RotX(-A) · RotZ(-C)   (row-vector)
M5    = mul(this=M4,   B=M_ry)      // = RotY(-B) · RotX(-A) · RotZ(-C), tag 5
M0    = Translation(0, 0, 0)        // tag 4 — translation deliberately zero
Mout  = mul(this=M5,   B=M0)        // tag forced to 6
memcpy(DAT_00509a38 + 0x7c, &Mout, 17*4);                 // matrix + tag
```

**The camera position does not enter this matrix.** The view rotation is pure
`RotY(-angle_y) · RotX(-angle_x) · RotZ(-angle_z)`; the record's `pos` reaches
the renderer separately through the view element's own world-position path
(the OMedia element at `DAT_00509a38` carries position independently — the
editor and distance/audio helpers `FUN_0047d640`/`FUN_0047d9d0` read
`rec->pos` directly for their own purposes).

## Reference sweep (DumpRefs, 2026-07-02)

~150 code references to `DAT_00509a50`. Functional groups:

- **Writers of `pos`:** `CGameType::InitGame` seed; `UpdateWalkingCameraA/B`
  (`00438bc0`/`00439900`) per-frame follow; `UpdateSittingOrSmoothCamera`
  (`0043a120`); `RunTriggerTypeNextTarget` (`00447a70`) retarget; `CEditor`
  save/restore pair.
- **Writers of `angles`:** `UpdateWalkingCameraA` (`+0x50` smoothing at
  `00438f0c..`, `+0x52` wrap-relative turn at `004397f8..`), walking camera B
  variants, editor free-fly.
- **Render consumer:** `FrameStepAndRender` (angles → view matrix, above).
- **Transform helpers:** `CameraRecordLocalToWorld_00476e10`,
  `CameraRecordLocalToWorldDir_00476f10`, half-extent getters
  `FUN_00476dd0`/`FUN_00476df0`.
- **Distance/audio/culling readers:** `FUN_0047d640`, `FUN_0047d9d0`,
  `FUN_0047d510`, `FUN_0046b160` (editor input/pick path; also reads
  `view_rect`).
- **Mass readers:** the `CLevel*Game` controller family (paired reads at
  `0044f190..004579ac`, one pair per controller class) and many placeable
  update bodies (billboarding/facing reads documented per-class).

Raw decompiled dumps follow.

## Raw dump: FrameStepAndRender

```c

void __thiscall FrameStepAndRender(void *this,OMediaRendererInterface *param_2)

{
  undefined2 uVar1;
  undefined2 uVar2;
  char cVar3;
  CGameObject *this_00;
  int iVar4;
  undefined4 unaff_ESI;
  undefined4 *puVar5;
  undefined4 unaff_EDI;
  undefined4 *puVar6;
  float10 fVar7;
  undefined1 auStack_1dc [48];
  undefined4 uStack_1ac;
  undefined4 uStack_1a8;
  undefined4 uStack_1a4;
  undefined4 uStack_19c;
  undefined1 auStack_198 [64];
  undefined4 uStack_158;
  undefined4 auStack_154 [16];
  undefined4 uStack_114;
  undefined1 auStack_110 [64];
  undefined4 uStack_d0;
  undefined1 auStack_cc [64];
  undefined4 uStack_8c;
  undefined4 uStack_48;
  undefined1 auStack_44 [64];
  undefined4 uStack_4;
  
  fVar7 = (float10)(**(code **)(*(int *)this + 0xd4))();
  DAT_00696030 = (float)fVar7;
  if ((fVar7 <= (float10)0.0) || (1.0 <= DAT_00696030)) {
    CGameObject::vfunc_00_013(this_00);
  }
  else {
    cVar3 = FUN_00475ca0();
    if (((cVar3 != '\0') && (DAT_00509980 != 0)) && (*(char *)(DAT_00509980 + 0x8c) == '\0')) {
      FUN_00479bc0(DAT_00696030);
      FUN_00462a20(DAT_00696030);
      FUN_004693a0(DAT_00696030);
    }
    FUN_0047d8f0(DAT_00696030);
    FUN_00473740();
    FUN_00473770();
    uStack_114 = 0;
    if (DAT_00509a38 != 0) {
      uVar1 = *(undefined2 *)(DAT_00509a50 + 0x54);
      uVar2 = *(undefined2 *)(DAT_00509a50 + 0x52);
      uStack_158 = 0;
      uStack_19c = 0;
      uStack_d0 = 0;
      uStack_8c = 0;
      uStack_48 = 0;
      uStack_4 = 0;
      FUN_0047e740(-*(short *)(DAT_00509a50 + 0x50));
      FUN_0047e7c0(-CONCAT22((short)((uint)unaff_EDI >> 0x10),uVar2));
      FUN_0047e830(-CONCAT22((short)((uint)unaff_ESI >> 0x10),uVar1));
      FUN_0047e8b0(auStack_110,auStack_44);
      FUN_0047e8b0(auStack_cc,auStack_198);
      uStack_158 = 5;
      FUN_0047e700();
      uStack_1ac = 0;
      uStack_1a8 = 0;
      uStack_1a4 = 0;
      uStack_19c = 4;
      FUN_0047e8b0(auStack_1dc,auStack_154);
      uStack_114 = 6;
      puVar5 = auStack_154;
      puVar6 = (undefined4 *)(DAT_00509a38 + 0x7c);
      for (iVar4 = 0x11; iVar4 != 0; iVar4 = iVar4 + -1) {
        *puVar6 = *puVar5;
        puVar5 = puVar5 + 1;
        puVar6 = puVar6 + 1;
      }
    }
    FUN_00477db0(DAT_00696030);
  }
  OMediaViewPort::render_viewport(this,param_2);
  FUN_00468600();
  DAT_00696028 = DAT_00696028 + 1;
  return;
}


```

## Raw dump: matrix helpers


```c

void __thiscall BuildTranslationMatrix(void *this)

{
  *(undefined4 *)this = 0x3f800000;
  *(undefined4 *)((int)this + 4) = 0;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 0xc) = 0;
  *(undefined4 *)((int)this + 0x10) = 0;
  *(undefined4 *)((int)this + 0x14) = 0x3f800000;
  *(undefined4 *)((int)this + 0x18) = 0;
  *(undefined4 *)((int)this + 0x1c) = 0;
  *(undefined4 *)((int)this + 0x20) = 0;
  *(undefined4 *)((int)this + 0x24) = 0;
  *(undefined4 *)((int)this + 0x28) = 0x3f800000;
  *(undefined4 *)((int)this + 0x2c) = 0;
  *(undefined4 *)((int)this + 0x30) = 0;
  *(undefined4 *)((int)this + 0x34) = 0;
  *(undefined4 *)((int)this + 0x38) = 0;
  *(undefined4 *)((int)this + 0x3c) = 0x3f800000;
  *(undefined4 *)((int)this + 0x40) = 1;
  return;
}


```

## BuildRotXMatrix @ 0047e740

```c

void __thiscall BuildRotXMatrix(void *this,uint param_2)

{
  undefined4 uVar1;
  float fVar2;
  
  *(undefined4 *)((int)this + 4) = 0;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 0xc) = 0;
  *(undefined4 *)((int)this + 0x10) = 0;
  *(undefined4 *)((int)this + 0x18) = 0;
  *(undefined4 *)((int)this + 0x1c) = 0;
  *(undefined4 *)((int)this + 0x20) = 0;
  *(undefined4 *)((int)this + 0x24) = 0;
  *(undefined4 *)((int)this + 0x2c) = 0;
  *(undefined4 *)((int)this + 0x30) = 0;
  *(undefined4 *)((int)this + 0x34) = 0;
  *(undefined4 *)((int)this + 0x38) = 0;
  *(undefined4 *)this = 0x3f800000;
  *(undefined4 *)((int)this + 0x14) = 0x3f800000;
  *(undefined4 *)((int)this + 0x28) = 0x3f800000;
  *(undefined4 *)((int)this + 0x3c) = 0x3f800000;
  *(undefined4 *)((int)this + 0x40) = 1;
  uVar1 = *(undefined4 *)(global_exref + (param_2 & 0x3fff) * 4 + 0x10004);
  fVar2 = *(float *)(global_exref + (param_2 & 0x3fff) * 4);
  *(undefined4 *)((int)this + 0x40) = 5;
  *(undefined4 *)((int)this + 0x14) = uVar1;
  *(float *)((int)this + 0x18) = -fVar2;
  *(float *)((int)this + 0x24) = fVar2;
  *(undefined4 *)((int)this + 0x28) = uVar1;
  return;
}


```

## BuildRotYMatrix @ 0047e7c0

```c

void __thiscall BuildRotYMatrix(void *this,uint param_2)

{
  undefined4 uVar1;
  float fVar2;
  
  *(undefined4 *)((int)this + 4) = 0;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 0xc) = 0;
  *(undefined4 *)((int)this + 0x10) = 0;
  *(undefined4 *)((int)this + 0x18) = 0;
  *(undefined4 *)((int)this + 0x1c) = 0;
  *(undefined4 *)((int)this + 0x20) = 0;
  *(undefined4 *)((int)this + 0x24) = 0;
  *(undefined4 *)((int)this + 0x2c) = 0;
  *(undefined4 *)((int)this + 0x30) = 0;
  *(undefined4 *)((int)this + 0x34) = 0;
  *(undefined4 *)((int)this + 0x38) = 0;
  *(undefined4 *)this = 0x3f800000;
  *(undefined4 *)((int)this + 0x14) = 0x3f800000;
  *(undefined4 *)((int)this + 0x28) = 0x3f800000;
  *(undefined4 *)((int)this + 0x3c) = 0x3f800000;
  *(undefined4 *)((int)this + 0x40) = 1;
  uVar1 = *(undefined4 *)(global_exref + (param_2 & 0x3fff) * 4 + 0x10004);
  fVar2 = *(float *)(global_exref + (param_2 & 0x3fff) * 4);
  *(undefined4 *)((int)this + 0x40) = 5;
  *(undefined4 *)this = uVar1;
  *(float *)((int)this + 8) = fVar2;
  *(float *)((int)this + 0x20) = -fVar2;
  *(undefined4 *)((int)this + 0x28) = uVar1;
  return;
}


```

## BuildRotZMatrix @ 0047e830

```c

void __thiscall BuildRotZMatrix(void *this,uint param_2)

{
  undefined4 uVar1;
  float fVar2;
  
  *(undefined4 *)((int)this + 4) = 0;
  *(undefined4 *)((int)this + 8) = 0;
  *(undefined4 *)((int)this + 0xc) = 0;
  *(undefined4 *)((int)this + 0x10) = 0;
  *(undefined4 *)((int)this + 0x18) = 0;
  *(undefined4 *)((int)this + 0x1c) = 0;
  *(undefined4 *)((int)this + 0x20) = 0;
  *(undefined4 *)((int)this + 0x24) = 0;
  *(undefined4 *)((int)this + 0x2c) = 0;
  *(undefined4 *)((int)this + 0x30) = 0;
  *(undefined4 *)((int)this + 0x34) = 0;
  *(undefined4 *)((int)this + 0x38) = 0;
  *(undefined4 *)this = 0x3f800000;
  *(undefined4 *)((int)this + 0x14) = 0x3f800000;
  *(undefined4 *)((int)this + 0x28) = 0x3f800000;
  *(undefined4 *)((int)this + 0x3c) = 0x3f800000;
  *(undefined4 *)((int)this + 0x40) = 1;
  uVar1 = *(undefined4 *)(global_exref + (param_2 & 0x3fff) * 4 + 0x10004);
  fVar2 = *(float *)(global_exref + (param_2 & 0x3fff) * 4);
  *(undefined4 *)((int)this + 0x40) = 5;
  *(undefined4 *)this = uVar1;
  *(float *)((int)this + 4) = -fVar2;
  *(float *)((int)this + 0x10) = fVar2;
  *(undefined4 *)((int)this + 0x14) = uVar1;
  return;
}


```

## Matrix4Multiply @ 0047e8b0

```c

void __thiscall Matrix4Multiply(void *this,float *param_2,int param_3)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  int iVar5;
  float *pfVar6;
  float *pfVar7;
  
  pfVar6 = (float *)((int)this + 0x20);
  iVar5 = 4;
  pfVar7 = (float *)(param_3 + 0x30);
  do {
    fVar1 = pfVar6[-8];
    fVar2 = pfVar6[-4];
    fVar3 = *pfVar6;
    fVar4 = pfVar6[4];
    pfVar6 = pfVar6 + 1;
    iVar5 = iVar5 + -1;
    pfVar7[-0xc] = fVar3 * param_2[2] + fVar4 * param_2[3] + fVar1 * *param_2 + fVar2 * param_2[1];
    pfVar7[-8] = fVar3 * param_2[6] + fVar4 * param_2[7] + fVar2 * param_2[5] + fVar1 * param_2[4];
    *(float *)((param_3 - (int)this) + -4 + (int)pfVar6) =
         fVar1 * param_2[8] + fVar3 * param_2[10] + fVar4 * param_2[0xb] + fVar2 * param_2[9];
    *pfVar7 = fVar2 * param_2[0xd] +
              fVar1 * param_2[0xc] + fVar3 * param_2[0xe] + fVar4 * param_2[0xf];
    pfVar7 = pfVar7 + 1;
  } while (iVar5 != 0);
  *(undefined4 *)(param_3 + 0x40) = 0;
  return;
}


```

## Raw dump: InitViewPort, view-global creator, record helpers

## FUN_00476490 @ 00476490

```c

void FUN_00476490(void)

{
  int *piVar1;
  int iVar2;
  CGameObject *in_ECX;
  CGameObject *extraout_ECX;
  CGameObject *extraout_ECX_00;
  CGameObject *extraout_ECX_01;
  CGameObject *extraout_ECX_02;
  CGameObject *extraout_ECX_03;
  CGameObject *this;
  char *pcStack_28;
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_0048c90b;
  local_c = ExceptionList;
  pcStack_28 = s_InitViewPort___004f6750;
  ExceptionList = &local_c;
  CGameObject::vfunc_00_013(in_ECX);
  this = extraout_ECX;
  if (DAT_00509a50 == (int *)0x0) {
    pcStack_28 = (char *)0x120;
    iVar2 = FUN_00478990();
    local_4 = 0;
    if (iVar2 == 0) {
      DAT_00509a50 = (int *)0x0;
      this = extraout_ECX_00;
    }
    else if (DAT_00509a48 == 0) {
      pcStack_28 = (char *)0x1;
      DAT_00509a50 = (int *)FUN_0047e310(0);
      this = extraout_ECX_02;
    }
    else {
      pcStack_28 = (char *)0x1;
      DAT_00509a50 = (int *)FUN_0047e310(DAT_00509a48 + 0x14);
      this = extraout_ECX_01;
    }
    local_4 = 0xffffffff;
    if (DAT_00509a50 != (int *)0x0) {
      pcStack_28 = DAT_00509a4c;
      (**(code **)(*DAT_00509a50 + 0x2c))();
      (**(code **)(*DAT_00509a50 + 0x34))(DAT_00509a30);
      (**(code **)(*DAT_00509a50 + 0xb0))(DAT_00509a48);
      pcStack_28 = (char *)0x0;
      (**(code **)(*DAT_00509a50 + 0xac))(&pcStack_28);
      piVar1 = DAT_00509a50;
      DAT_00509a50[0x3f] = 1;
      (**(code **)(*piVar1 + 0xac))(piVar1 + 0x39);
      piVar1 = DAT_00509a50;
      DAT_00509a50[0x40] = 1;
      (**(code **)(*piVar1 + 0xac))(piVar1 + 0x39);
      this = extraout_ECX_03;
    }
  }
  pcStack_28 = (char *)0x4765b5;
  CGameObject::vfunc_00_013(this);
  ExceptionList = local_c;
  return;
}


```

## FUN_004765f0 @ 004765f0

```c

void FUN_004765f0(void)

{
  OMediaWorld *this;
  CGameObject *in_ECX;
  CGameObject *extraout_ECX;
  CGameObject *extraout_ECX_00;
  CGameObject *extraout_ECX_01;
  CGameObject *this_00;
  char *pcVar1;
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_0048c92b;
  local_c = ExceptionList;
  pcVar1 = s_InitWorld___004f6774;
  ExceptionList = &local_c;
  CGameObject::vfunc_00_013(in_ECX);
  this = (OMediaWorld *)FUN_00478990(0xa4,pcVar1);
  local_4 = 0;
  if (this == (OMediaWorld *)0x0) {
    DAT_00509a4c = 0;
    this_00 = extraout_ECX;
  }
  else {
    DAT_00509a4c = OMediaWorld::OMediaWorld(this);
    this_00 = extraout_ECX_00;
  }
  local_4 = 0xffffffff;
  if (DAT_00509a4c != 0) {
    DAT_00509a38 = FUN_00476750(this);
    DAT_00509a28 = FUN_00476750();
    DAT_00509a30 = FUN_00476750();
    DAT_00509a3c = DAT_00509a30;
    DAT_00509a34 = FUN_00476750();
    DAT_00509a2c = FUN_00476750();
    FUN_004767c0();
    this_00 = extraout_ECX_01;
  }
  CGameObject::vfunc_00_013(this_00);
  ExceptionList = local_c;
  return;
}


```

## FUN_0046b160 @ 0046b160

```c

void FUN_0046b160(int param_1)

{
  CGameObject *this;
  int iVar1;
  char cVar2;
  short sVar3;
  OMediaCanvas *this_00;
  OMediaClassStreamer *pOVar4;
  OMediaCanvasFont *this_01;
  OMediaStringField *pOVar5;
  int *piVar6;
  int iVar7;
  OMediaElement *in_ECX;
  int iVar8;
  uint uVar9;
  uint uVar10;
  short unaff_BX;
  OMediaElement *pOVar11;
  int iVar12;
  char *pcVar13;
  char *pcVar14;
  OMediaElement *pOVar15;
  char *pcStack_f4;
  int iStack_c4;
  OMediaFilePath aOStack_b8 [8];
  uint local_b0;
  OMediaFilePath local_a8 [16];
  OMediaFileStream local_98 [40];
  char acStack_70 [4];
  char acStack_6c [80];
  void *pvStack_1c;
  undefined1 uStack_14;
  void *pvStack_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  local_4 = 0xffffffff;
  puStack_8 = &LAB_0048c51f;
  pvStack_c = ExceptionList;
  local_b0 = 0;
  ExceptionList = &pvStack_c;
  if (param_1 != 0) {
    ExceptionList = &pvStack_c;
    *(undefined **)(in_ECX + 4) = &DAT_004d5fd0;
    OMediaClassStreamer::OMediaClassStreamer((OMediaClassStreamer *)(in_ECX + 0x814));
    local_4 = 0;
  }
  local_b0 = (uint)(param_1 != 0);
  OMediaElement::OMediaElement(in_ECX);
  *(undefined4 *)(in_ECX + 0x9c) = 0x3f800000;
  *(undefined4 *)(in_ECX + 0xac) = 0x3f800000;
  *(undefined4 *)(in_ECX + 0xb0) = 0;
  *(undefined4 *)(in_ECX + 0xb4) = 0;
  *(undefined4 *)(in_ECX + 0xb8) = 0;
  *(undefined4 *)(in_ECX + 0xbc) = 0;
  *(undefined4 *)(in_ECX + 0xc4) = 0;
  *(undefined ***)in_ECX = &CEditor::vftable_2;
  *(undefined ***)(in_ECX + 0x30) = &CEditor::vftable_1;
  local_4 = 1;
  *(undefined ***)(in_ECX + *(int *)(*(int *)(in_ECX + 4) + 4) + 4) = &CEditor::vftable;
  this = (CGameObject *)(*(int *)(*(int *)(in_ECX + 4) + 4) + -0x810);
  *(CGameObject **)(in_ECX + *(int *)(*(int *)(in_ECX + 4) + 4)) = this;
  CGameObject::vfunc_00_013(this);
  DAT_00509980 = (CEditor *)in_ECX;
  in_ECX[0x7f9] = (OMediaElement)0x0;
  CEditor::vfunc_02_043((CEditor *)in_ECX);
  in_ECX[0x8c] = (OMediaElement)0x0;
  *(undefined4 *)(in_ECX + 0x78) = 0x45444954;
  pOVar11 = in_ECX + 200;
  for (iVar8 = 0xd; iVar8 != 0; iVar8 = iVar8 + -1) {
    *(undefined4 *)pOVar11 = 0;
    pOVar11 = pOVar11 + 4;
  }
  FUN_0040acc0();
  OMediaFilePath::OMediaFilePath(local_a8);
  local_4._0_1_ = 2;
  OMediaFileStream::OMediaFileStream(local_98);
  local_4._0_1_ = 3;
  OMediaFileStream::setpath(local_98,local_a8);
  OMediaFileStream::open(local_98,1,false,false);
  this_00 = (OMediaCanvas *)FUN_00478990();
  local_4._0_1_ = 4;
  if (this_00 == (OMediaCanvas *)0x0) {
    iVar8 = 0;
  }
  else {
    iVar8 = OMediaCanvas::OMediaCanvas(this_00);
  }
  local_4._0_1_ = 3;
  if (iVar8 == 0) {
    pOVar4 = (OMediaClassStreamer *)0x0;
  }
  else {
    pOVar4 = (OMediaClassStreamer *)(*(int *)(*(int *)(iVar8 + 4) + 4) + 4 + iVar8);
  }
  OMediaStreamOperators::operator>>((OMediaStreamOperators *)local_98,pOVar4);
  DAT_00509978 = iVar8;
  CGameObject::vfunc_00_013(*(CGameObject **)(DAT_00509a50 + 0xd8));
  this_01 = (OMediaCanvasFont *)FUN_00478990();
  local_4._0_1_ = 5;
  if (this_01 == (OMediaCanvasFont *)0x0) {
    DAT_0050997c = (int *)0x0;
  }
  else {
    DAT_0050997c = (int *)OMediaCanvasFont::OMediaCanvasFont(this_01);
  }
  iVar8 = 10;
  local_4 = CONCAT31(local_4._1_3_,3);
  (**(code **)(*DAT_0050997c + 0x14))();
  *(undefined1 *)(DAT_0050997c + 0x84) = 1;
  (**(code **)(*DAT_0050997c + 0x20))();
  iStack_c4 = 100;
  DAT_0050997c[0x86] = 3;
  DAT_0050997c[0x87] = 1;
  pOVar11 = in_ECX + 0x28c;
  do {
    uVar9 = 0xffffffff;
    pcVar13 = s_Empty_now_004f5798;
    do {
      pcVar14 = pcVar13;
      if (uVar9 == 0) break;
      uVar9 = uVar9 - 1;
      pcVar14 = pcVar13 + 1;
      cVar2 = *pcVar13;
      pcVar13 = pcVar14;
    } while (cVar2 != '\0');
    uVar9 = ~uVar9;
    pcVar13 = pcVar14 + -uVar9;
    pcVar14 = acStack_6c;
    for (uVar10 = uVar9 >> 2; uVar10 != 0; uVar10 = uVar10 - 1) {
      *(undefined4 *)pcVar14 = *(undefined4 *)pcVar13;
      pcVar13 = pcVar13 + 4;
      pcVar14 = pcVar14 + 4;
    }
    for (uVar9 = uVar9 & 3; uVar9 != 0; uVar9 = uVar9 - 1) {
      *pcVar14 = *pcVar13;
      pcVar13 = pcVar13 + 1;
      pcVar14 = pcVar14 + 1;
    }
    pOVar5 = (OMediaStringField *)FUN_00478990();
    uStack_14 = 6;
    if (pOVar5 == (OMediaStringField *)0x0) {
      piVar6 = (int *)0x0;
    }
    else {
      piVar6 = (int *)OMediaStringField::OMediaStringField(pOVar5);
    }
    *(int **)pOVar11 = piVar6;
    uStack_14 = 3;
    (**(code **)(*piVar6 + 0x2c))();
    piVar6 = *(int **)pOVar11;
    *(undefined2 *)(piVar6 + 0x5e) = 0x20;
    *(undefined2 *)((int)piVar6 + 0x17a) = 0xe;
    (**(code **)(*piVar6 + 0xc0))();
    piVar6 = *(int **)pOVar11;
    piVar6[0x78] = (int)DAT_0050997c;
    (**(code **)(*piVar6 + 0xc0))();
    iVar7 = *(int *)pOVar11;
    *(float *)(iVar7 + 0x48) = (float)iVar8;
    *(OMediaStringField **)(iVar7 + 0x44) = pOVar5;
    uVar9 = 0xffffffff;
    pcVar13 = acStack_70;
    do {
      if (uVar9 == 0) break;
      uVar9 = uVar9 - 1;
      cVar2 = *pcVar13;
      pcVar13 = pcVar13 + 1;
    } while (cVar2 != '\0');
    FUN_004074b0(acStack_70,~uVar9 - 1);
    (**(code **)(**(int **)pOVar11 + 200))();
    (**(code **)(**(int **)pOVar11 + 0x58))(1);
    iVar7 = *(int *)pOVar11;
    pOVar11 = pOVar11 + 4;
    *(undefined4 *)(iVar7 + 0x1ec) = 3;
    iStack_c4 = iStack_c4 + -1;
  } while (iStack_c4 != 0);
  sVar3 = 0;
  do {
    iVar12 = (int)sVar3;
    iVar7 = __strcmpi(s_Select_Object_004f5410 + iVar12 * 0x20,(char *)&DAT_004f81a8);
    if (iVar7 == 0) break;
    uVar9 = 0xffffffff;
    pcVar13 = s_Select_Object_004f5410 + iVar12 * 0x20;
    do {
      pcVar14 = pcVar13;
      if (uVar9 == 0) break;
      uVar9 = uVar9 - 1;
      pcVar14 = pcVar13 + 1;
      cVar2 = *pcVar13;
      pcVar13 = pcVar14;
    } while (cVar2 != '\0');
    uVar9 = ~uVar9;
    pcVar13 = pcVar14 + -uVar9;
    pcVar14 = acStack_6c;
    for (uVar10 = uVar9 >> 2; uVar10 != 0; uVar10 = uVar10 - 1) {
      *(undefined4 *)pcVar14 = *(undefined4 *)pcVar13;
      pcVar13 = pcVar13 + 4;
      pcVar14 = pcVar14 + 4;
    }
    for (uVar9 = uVar9 & 3; uVar9 != 0; uVar9 = uVar9 - 1) {
      *pcVar14 = *pcVar13;
      pcVar13 = pcVar13 + 1;
      pcVar14 = pcVar14 + 1;
    }
    pOVar5 = (OMediaStringField *)FUN_00478990();
    uStack_14 = 7;
    if (pOVar5 == (OMediaStringField *)0x0) {
      piVar6 = (int *)0x0;
    }
    else {
      piVar6 = (int *)OMediaStringField::OMediaStringField(pOVar5);
    }
    *(int **)(in_ECX + iVar12 * 4 + 0xfc) = piVar6;
    uStack_14 = 3;
    (**(code **)(*piVar6 + 0x2c))();
    piVar6 = *(int **)(in_ECX + iVar12 * 4 + 0xfc);
    *(undefined2 *)(piVar6 + 0x5e) = 0x20;
    *(undefined2 *)((int)piVar6 + 0x17a) = 0xe;
    (**(code **)(*piVar6 + 0xc0))();
    piVar6 = *(int **)(in_ECX + iVar12 * 4 + 0xfc);
    piVar6[0x78] = (int)DAT_0050997c;
    (**(code **)(*piVar6 + 0xc0))();
    iVar7 = *(int *)(in_ECX + iVar12 * 4 + 0xfc);
    uVar9 = 0xffffffff;
    *(float *)(iVar7 + 0x44) = (float)(int)pOVar5;
    *(float *)(iVar7 + 0x48) = (float)iVar8;
    pcStack_f4 = (char *)0x0;
    pcVar13 = acStack_70;
    do {
      if (uVar9 == 0) break;
      uVar9 = uVar9 - 1;
      cVar2 = *pcVar13;
      pcVar13 = pcVar13 + 1;
    } while (cVar2 != '\0');
    uVar9 = ~uVar9 - 1;
    cVar2 = FUN_0044ecc0(uVar9,1);
    if (cVar2 != '\0') {
      pcVar13 = acStack_70;
      for (uVar10 = uVar9 >> 2; uVar10 != 0; uVar10 = uVar10 - 1) {
        *(undefined4 *)pcStack_f4 = *(undefined4 *)pcVar13;
        pcVar13 = pcVar13 + 4;
        pcStack_f4 = pcStack_f4 + 4;
      }
      for (uVar10 = uVar9 & 3; uVar10 != 0; uVar10 = uVar10 - 1) {
        *pcStack_f4 = *pcVar13;
        pcVar13 = pcVar13 + 1;
        pcStack_f4 = pcStack_f4 + 1;
      }
      FUN_0044eca0(uVar9);
    }
    iVar7 = (int)unaff_BX;
    (**(code **)(**(int **)(in_ECX + iVar7 * 4 + 0xfc) + 200))();
    (**(code **)(**(int **)(in_ECX + iVar7 * 4 + 0xfc) + 0x58))(1);
    *(undefined4 *)(*(int *)(in_ECX + iVar7 * 4 + 0xfc) + 0x1ec) = 3;
    sVar3 = sVar3 + 1;
  } while (sVar3 < 100);
  pOVar11 = in_ECX + 0x424;
  iVar7 = 100;
  iVar12 = (*(int *)(DAT_00509a50 + 0xe0) - *(int *)(DAT_00509a50 + 0xd8)) / 2 + -0x30;
  do {
    if (iVar12 < -((*(int *)(DAT_00509a50 + 0xe0) - *(int *)(DAT_00509a50 + 0xd8)) / 2)) {
      iVar12 = (*(int *)(DAT_00509a50 + 0xe0) - *(int *)(DAT_00509a50 + 0xd8)) / 2 + -0x30;
    }
    pOVar5 = (OMediaStringField *)FUN_00478990();
    uStack_14 = 8;
    if (pOVar5 == (OMediaStringField *)0x0) {
      piVar6 = (int *)0x0;
    }
    else {
      piVar6 = (int *)OMediaStringField::OMediaStringField(pOVar5);
    }
    *(int **)pOVar11 = piVar6;
    uStack_14 = 3;
    (**(code **)(*piVar6 + 0x2c))();
    piVar6 = *(int **)pOVar11;
    *(undefined2 *)(piVar6 + 0x5e) = 0x20;
    *(undefined2 *)((int)piVar6 + 0x17a) = 0xe;
    (**(code **)(*piVar6 + 0xc0))();
    piVar6 = *(int **)pOVar11;
    piVar6[0x78] = (int)DAT_0050997c;
    (**(code **)(*piVar6 + 0xc0))();
    iVar1 = *(int *)pOVar11;
    *(float *)(iVar1 + 0x44) = (float)(int)pOVar5;
    *(float *)(iVar1 + 0x48) = (float)iVar8;
    (**(code **)(**(int **)pOVar11 + 0x58))();
    iVar12 = iVar12 + -0xc;
    pOVar11 = pOVar11 + 4;
    iVar7 = iVar7 + -1;
  } while (iVar7 != 0);
  pOVar11 = in_ECX + 0x5b4;
  iVar7 = 100;
  iVar12 = (*(int *)(DAT_00509a50 + 0xe0) - *(int *)(DAT_00509a50 + 0xd8)) / 2 + -0x30;
  do {
    if (iVar12 < -((*(int *)(DAT_00509a50 + 0xe0) - *(int *)(DAT_00509a50 + 0xd8)) / 2)) {
      iVar12 = (*(int *)(DAT_00509a50 + 0xe0) - *(int *)(DAT_00509a50 + 0xd8)) / 2 + -0x30;
    }
    pOVar5 = (OMediaStringField *)FUN_00478990();
    uStack_14 = 9;
    if (pOVar5 == (OMediaStringField *)0x0) {
      piVar6 = (int *)0x0;
    }
    else {
      piVar6 = (int *)OMediaStringField::OMediaStringField(pOVar5);
    }
    *(int **)pOVar11 = piVar6;
    uStack_14 = 3;
    (**(code **)(*piVar6 + 0x2c))();
    piVar6 = *(int **)pOVar11;
    *(undefined2 *)(piVar6 + 0x5e) = 0x20;
    *(undefined2 *)((int)piVar6 + 0x17a) = 0xe;
    (**(code **)(*piVar6 + 0xc0))();
    piVar6 = *(int **)pOVar11;
    piVar6[0x78] = (int)DAT_0050997c;
    (**(code **)(*piVar6 + 0xc0))();
    iVar1 = *(int *)pOVar11;
    *(float *)(iVar1 + 0x44) = (float)(int)pOVar5;
    *(float *)(iVar1 + 0x48) = (float)iVar8;
    *(undefined4 *)(*(int *)pOVar11 + 0x1ec) = 1;
    (**(code **)(**(int **)pOVar11 + 0x58))();
    iVar12 = iVar12 + -0xc;
    pOVar11 = pOVar11 + 4;
    iVar7 = iVar7 + -1;
  } while (iVar7 != 0);
  *(undefined4 *)(in_ECX + 0x41c) = 0;
  *(undefined4 *)(in_ECX + 0x420) = 0;
  CEditor::vfunc_02_057((CEditor *)in_ECX);
  uVar9 = 0xffffffff;
  *(undefined4 *)(in_ECX + 0x744) = 0;
  *(undefined4 *)(in_ECX + 0x748) = 0x42c80000;
  in_ECX[0x74c] = (OMediaElement)0x0;
  pcVar13 = (char *)&DAT_004f81a8;
  do {
    pcVar14 = pcVar13;
    if (uVar9 == 0) break;
    uVar9 = uVar9 - 1;
    pcVar14 = pcVar13 + 1;
    cVar2 = *pcVar13;
    pcVar13 = pcVar14;
  } while (cVar2 != '\0');
  uVar9 = ~uVar9;
  pOVar11 = (OMediaElement *)(pcVar14 + -uVar9);
  pOVar15 = in_ECX + 0x74d;
  for (uVar10 = uVar9 >> 2; uVar10 != 0; uVar10 = uVar10 - 1) {
    *(undefined4 *)pOVar15 = *(undefined4 *)pOVar11;
    pOVar11 = pOVar11 + 4;
    pOVar15 = pOVar15 + 4;
  }
  for (uVar9 = uVar9 & 3; uVar9 != 0; uVar9 = uVar9 - 1) {
    *pOVar15 = *pOVar11;
    pOVar11 = pOVar11 + 1;
    pOVar15 = pOVar15 + 1;
  }
  uVar9 = 0xffffffff;
  pcVar13 = (char *)&DAT_004f81a8;
  do {
    pcVar14 = pcVar13;
    if (uVar9 == 0) break;
    uVar9 = uVar9 - 1;
    pcVar14 = pcVar13 + 1;
    cVar2 = *pcVar13;
    pcVar13 = pcVar14;
  } while (cVar2 != '\0');
  uVar9 = ~uVar9;
  pOVar11 = (OMediaElement *)(pcVar14 + -uVar9);
  pOVar15 = in_ECX + 0x7a4;
  for (uVar10 = uVar9 >> 2; uVar10 != 0; uVar10 = uVar10 - 1) {
    *(undefined4 *)pOVar15 = *(undefined4 *)pOVar11;
    pOVar11 = pOVar11 + 4;
    pOVar15 = pOVar15 + 4;
  }
  for (uVar9 = uVar9 & 3; uVar9 != 0; uVar9 = uVar9 - 1) {
    *pOVar15 = *pOVar11;
    pOVar11 = pOVar11 + 1;
    pOVar15 = pOVar15 + 1;
  }
  *(undefined4 *)(in_ECX + 0x7a0) = 0xffffffff;
  *(undefined2 *)(in_ECX + 0x7fa) = 0xffff;
  in_ECX[0x7f8] = (OMediaElement)0x0;
  *(undefined4 *)(in_ECX + 0x7fc) = 0;
  in_ECX[0x800] = (OMediaElement)0x0;
  *(undefined4 *)(in_ECX + 0x804) = 0;
  *(undefined4 *)(in_ECX + 0x808) = 0;
  in_ECX[0x80c] = (OMediaElement)0x0;
  in_ECX[0x80d] = (OMediaElement)0x0;
  CGameObject::vfunc_00_013((CGameObject *)0x0);
  uStack_14 = 2;
  OMediaFileStream::~OMediaFileStream((OMediaFileStream *)local_a8);
  uStack_14 = 1;
  OMediaFilePath::~OMediaFilePath(aOStack_b8);
  ExceptionList = pvStack_1c;
  return;
}


```

## FUN_004767c0 @ 004767c0

```c

void FUN_004767c0(void)

{
  int iVar1;
  CGameObject *in_ECX;
  CGameObject *pCVar2;
  
  CGameObject::vfunc_00_013(in_ECX);
  pCVar2 = (CGameObject *)0x0;
  if (DAT_00509a38 != 0) {
    *(undefined4 *)(DAT_00509a38 + 4) = 0x287;
    iVar1 = DAT_00509a38;
    *(undefined4 *)(DAT_00509a38 + 0x10c) = 0;
    *(undefined4 *)(iVar1 + 0x110) = 0;
    *(undefined4 *)(iVar1 + 0x114) = 0;
    *(undefined4 *)(iVar1 + 0x118) = 0x3f800000;
    *(undefined4 *)(DAT_00509a38 + 0xc0) = 0x49742400;
  }
  if (DAT_00509a28 != 0) {
    *(undefined4 *)(DAT_00509a28 + 4) = 0x184;
    *(undefined4 *)(DAT_00509a28 + 0x30) = 1;
    *(undefined4 *)(DAT_00509a28 + 0xc4) = 0;
  }
  if (DAT_00509a3c != (CGameObject *)0x0) {
    DAT_00509a3c[1].vftable = (undefined *)0x9c;
    if (DAT_00509a13 == '\0') {
      DAT_00509a3c[2].vftable = (undefined *)((uint)DAT_00509a3c[2].vftable | 0x20);
    }
    pCVar2 = DAT_00509a3c;
    DAT_00509a3c[0x43].vftable = (undefined *)0x0;
    pCVar2[0x44].vftable = (undefined *)0x0;
    pCVar2[0x45].vftable = (undefined *)0x0;
    pCVar2[0x46].vftable = (undefined *)0x3f800000;
    pCVar2 = DAT_00509a3c;
    DAT_00509a3c[0x30].vftable = DAT_004f6564;
  }
  if (DAT_00509a34 != 0) {
    *(undefined4 *)(DAT_00509a34 + 4) = 0x184;
    *(undefined4 *)(DAT_00509a34 + 0x30) = 1;
    *(undefined4 *)(DAT_00509a34 + 0xc4) = 0;
  }
  if (DAT_00509a2c != (CGameObject *)0x0) {
    DAT_00509a2c[1].vftable = (undefined *)0x184;
    pCVar2 = DAT_00509a2c;
    DAT_00509a2c[0xc].vftable = (undefined *)0x1;
    DAT_00509a2c[0x31].vftable = (undefined *)0x0;
  }
  CGameObject::vfunc_00_013(pCVar2);
  return;
}


```

## FUN_00476dd0 @ 00476dd0

```c

int FUN_00476dd0(void)

{
  return (*(int *)(DAT_00509a50 + 0xdc) - *(int *)(DAT_00509a50 + 0xd4)) / 2;
}


```

## FUN_00476df0 @ 00476df0

```c

int FUN_00476df0(void)

{
  return (*(int *)(DAT_00509a50 + 0xe0) - *(int *)(DAT_00509a50 + 0xd8)) / 2;
}


```

## FUN_00476e10 @ 00476e10

```c

void FUN_00476e10(float *param_1,float param_2,float param_3,float param_4)

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
  float fVar10;
  int iVar11;
  int iVar12;
  
  iVar12 = (*(ushort *)(DAT_00509a50 + 0x50) & 0x3fff) * 4;
  fVar1 = *(float *)(global_exref + iVar12);
  fVar2 = *(float *)(global_exref + iVar12 + 0x10004);
  iVar11 = (*(ushort *)(DAT_00509a50 + 0x52) & 0x3fff) * 4;
  iVar12 = (*(ushort *)(DAT_00509a50 + 0x54) & 0x3fff) * 4;
  fVar3 = *(float *)(global_exref + iVar11);
  fVar4 = *(float *)(global_exref + iVar11 + 0x10004);
  fVar10 = *(float *)(global_exref + iVar12 + 0x10004) * param_3 -
           *(float *)(global_exref + iVar12) * param_2;
  fVar9 = *(float *)(global_exref + iVar12) * param_3 +
          *(float *)(global_exref + iVar12 + 0x10004) * param_2;
  fVar8 = fVar2 * param_4 - fVar10 * fVar1;
  fVar5 = *(float *)(DAT_00509a50 + 0x44);
  fVar6 = *(float *)(DAT_00509a50 + 0x48);
  fVar7 = *(float *)(DAT_00509a50 + 0x4c);
  param_1[3] = 1.0;
  *param_1 = fVar5 + (fVar9 * fVar4 - fVar8 * fVar3);
  param_1[2] = fVar7 + fVar9 * fVar3 + fVar8 * fVar4;
  param_1[1] = fVar1 * param_4 + fVar10 * fVar2 + fVar6;
  return;
}


```

## FUN_00476f10 @ 00476f10

```c

void FUN_00476f10(float *param_1,float param_2,float param_3,float param_4)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  int iVar8;
  int iVar9;
  
  iVar9 = (*(ushort *)(DAT_00509a50 + 0x50) & 0x3fff) * 4;
  iVar8 = (*(ushort *)(DAT_00509a50 + 0x52) & 0x3fff) * 4;
  fVar1 = *(float *)(global_exref + iVar9);
  fVar2 = *(float *)(global_exref + iVar9 + 0x10004);
  iVar9 = (*(ushort *)(DAT_00509a50 + 0x54) & 0x3fff) * 4;
  fVar3 = *(float *)(global_exref + iVar8);
  fVar4 = *(float *)(global_exref + iVar8 + 0x10004);
  fVar5 = *(float *)(global_exref + iVar9);
  fVar6 = *(float *)(global_exref + iVar9 + 0x10004);
  fVar7 = fVar6 * param_3 - fVar5 * param_2;
  param_1[3] = 0.0;
  fVar5 = fVar5 * param_3 + fVar6 * param_2;
  fVar6 = fVar2 * param_4 - fVar7 * fVar1;
  *param_1 = fVar5 * fVar4 - fVar6 * fVar3;
  param_1[1] = fVar1 * param_4 + fVar7 * fVar2;
  param_1[2] = fVar5 * fVar3 + fVar6 * fVar4;
  return;
}


```

## FUN_0047d640 @ 0047d640

```c

int FUN_0047d640(int *param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  float fVar1;
  int iVar2;
  int iVar3;
  float *pfVar4;
  float unaff_ESI;
  float fVar5;
  float fStack_24;
  float fStack_20;
  float fStack_1c;
  undefined4 uStack_18;
  undefined1 local_10 [16];
  
  iVar3 = FUN_0047d390(param_2,param_3,param_4);
  iVar2 = DAT_00509a50;
  if (iVar3 != -1) {
    (&DAT_00695d9c)[iVar3] = param_1;
    fVar5 = *(float *)(iVar2 + 0x44);
    fVar1 = *(float *)(iVar2 + 0x48);
    pfVar4 = (float *)(**(code **)(*param_1 + 0x310))(local_10);
    fStack_24 = unaff_ESI - *pfVar4;
    fStack_20 = fVar5 - pfVar4[1];
    fStack_1c = fVar1 - pfVar4[2];
    uStack_18 = 0;
    fVar5 = OMedia3DVector::quick_magnitude((OMedia3DVector *)&fStack_24);
    iVar2 = DAT_0069600c;
    (&DAT_00695f84)[iVar3] = fVar5;
    if (*(int *)(*(int *)(iVar2 + 0x40) + iVar3 * 4) != 0) {
      (&DAT_00695e3c)[iVar3] = 0x2b11;
      (&DAT_00695e1c)[iVar3] = 1;
    }
    FUN_0047d9d0(iVar3);
  }
  return iVar3;
}


```

## FUN_0047d9d0 @ 0047d9d0

```c

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_0047d9d0(int param_1)

{
  int *piVar1;
  float *pfVar2;
  undefined4 uVar3;
  int iVar4;
  int iVar5;
  float fVar6;
  float fStack_34;
  float fStack_30;
  float fStack_2c;
  undefined4 uStack_28;
  float local_20;
  float fStack_1c;
  float fStack_14;
  float fStack_10;
  float fStack_c;
  float fStack_8;
  
  if ((DAT_0069600c != 0) && (DAT_00696010 == '\0')) {
    piVar1 = *(int **)(*(int *)(DAT_0069600c + 0x40) + param_1 * 4);
    if ((int *)(&DAT_00695d9c)[param_1] != (int *)0x0) {
      pfVar2 = (float *)(**(code **)(*(int *)(&DAT_00695d9c)[param_1] + 0x310))(&local_20);
      fStack_10 = pfVar2[1];
      fStack_c = pfVar2[2];
      local_20 = *(float *)(DAT_00509a50 + 0x48);
      fStack_1c = *(float *)(DAT_00509a50 + 0x4c);
      fStack_34 = *(float *)(DAT_00509a50 + 0x44) - *pfVar2;
      uStack_28 = 0;
      fStack_30 = local_20 - fStack_10;
      fStack_2c = fStack_1c - fStack_c;
      fVar6 = OMedia3DVector::quick_magnitude((OMedia3DVector *)&fStack_34);
      if (fVar6 != 0.0) {
        fVar6 = OMedia3DVector::quick_magnitude((OMedia3DVector *)&fStack_34);
        fVar6 = 1.0 / fVar6;
        iVar4 = *piVar1;
        fStack_34 = fStack_34 * fVar6;
        fStack_30 = fStack_30 * fVar6;
        fStack_2c = fStack_2c * fVar6;
        iVar5 = (*(ushort *)(DAT_00509a50 + 0x52) & 0x3fff) * 4;
        fVar6 = *(float *)(global_exref + 0x10004);
        fStack_14 = fVar6 * *(float *)(global_exref + iVar5 + 0x10004) -
                    -(*(float *)global_exref * -*(float *)global_exref) *
                    *(float *)(global_exref + iVar5);
        uVar3 = __ftol();
        (**(code **)(iVar4 + 0x20))(uVar3);
        iVar4 = *piVar1;
        __ftol();
        uVar3 = __ftol();
        (**(code **)(iVar4 + 0x18))(uVar3);
        if ((&DAT_00695e1c)[param_1] != '\0') {
          pfVar2 = (float *)(&DAT_00695f84 + param_1);
          iVar4 = __ftol();
          if (*pfVar2 < fStack_8) {
            (**(code **)(*piVar1 + 0x1c))((&DAT_00695e3c)[param_1] - iVar4);
            *pfVar2 = (float)(int)fVar6;
            return;
          }
          (**(code **)(*piVar1 + 0x1c))((&DAT_00695e3c)[param_1] + iVar4);
          *pfVar2 = (float)(int)fVar6;
        }
      }
    }
  }
  return;
}


```

## FUN_0047d510 @ 0047d510

```c

int FUN_0047d510(int *param_1,undefined4 param_2,undefined4 param_3,int param_4,undefined4 param_5,
                undefined4 param_6,undefined1 *param_7)

{
  char cVar1;
  float fVar2;
  float fVar3;
  int iVar4;
  int iVar5;
  float *pfVar6;
  float fVar7;
  undefined1 auStack_54 [12];
  undefined4 uStack_48;
  undefined1 *puStack_44;
  float fStack_2c;
  float fStack_28;
  float fStack_24;
  undefined4 uStack_20;
  undefined1 local_1c [16];
  void *local_c;
  undefined1 *puStack_8;
  undefined4 local_4;
  
  puStack_8 = &LAB_0048ca78;
  local_c = ExceptionList;
  puStack_44 = param_7;
  local_4 = 0;
  param_7 = auStack_54;
  ExceptionList = &local_c;
  FUN_00458fc0(&param_3);
  iVar5 = FUN_0047d420(param_2);
  iVar4 = DAT_00509a50;
  if (iVar5 != -1) {
    (&DAT_00695d9c)[iVar5] = param_1;
    fVar7 = *(float *)(iVar4 + 0x44);
    fVar2 = *(float *)(iVar4 + 0x48);
    fVar3 = *(float *)(iVar4 + 0x4c);
    puStack_44 = local_1c;
    uStack_48 = 0x47d593;
    pfVar6 = (float *)(**(code **)(*param_1 + 0x310))();
    fStack_2c = fVar7 - *pfVar6;
    fStack_28 = fVar2 - pfVar6[1];
    fStack_24 = fVar3 - pfVar6[2];
    uStack_20 = 0;
    puStack_44 = (undefined1 *)0x47d5c5;
    fVar7 = OMedia3DVector::quick_magnitude((OMedia3DVector *)&fStack_2c);
    iVar4 = DAT_0069600c;
    (&DAT_00695f84)[iVar5] = fVar7;
    if (*(int *)(*(int *)(iVar4 + 0x40) + iVar5 * 4) != 0) {
      (&DAT_00695e3c)[iVar5] = 0x2b11;
      (&DAT_00695e1c)[iVar5] = 1;
    }
    uStack_48 = 0x47d5f3;
    puStack_44 = (undefined1 *)iVar5;
    FUN_0047d9d0();
  }
  if (param_4 != 0) {
    cVar1 = *(char *)(param_4 + -1);
    if ((cVar1 != '\0') && (cVar1 != -1)) {
      *(char *)(param_4 + -1) = cVar1 + -1;
      ExceptionList = local_c;
      return iVar5;
    }
    puStack_44 = (undefined1 *)(param_4 + -1);
    uStack_48 = 0x47d627;
    FUN_004789a0();
  }
  ExceptionList = local_c;
  return iVar5;
}


```

## vfunc_02_045 @ 0046bcb0

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

## vfunc_02_046 @ 0046bdc0

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

## vfunc_02_055 @ 0046cf30

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

