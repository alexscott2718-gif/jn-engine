# Walking-camera record write — UpdateWalkingCameraA/B (00438bc0 / 00439900)

Recovered 2026-07-02 with `tools/ghidra/WalkCamPass.java` / `WalkCamPass2.java`
against `~/ghidra-projects/JN_decomp` / `Neutron.exe` (prototype repair +
re-decompile + raw-listing x87 trace). This is the interpretation pass over the
raw target-3 dumps (`c3dplayer_movement_target3.md`), scoped to what the two
walking helpers write into the global camera/player-target record
`DAT_00509a50` (layout: `camera_record_layout.md`). The player-movement halves
(walk-speed accumulator, turn ramps, idle animations) stay under the
`C3DPlayer`/`free-roam-feel` linked-blocked row and are NOT re-interpreted
here.

## Identity and dispatch

Both helpers are C3DPlayer vtable-4 virtual methods (`docs/decomp/C3DPlayer.md`):

| slot | byte offset | address | name |
|---|---|---|---|
| 72 | `+0x120` | `0043a5d0` | `ProjectNoisyCameraTarget` |
| 73 | `+0x124` | `00438bc0` | `UpdateWalkingCameraA` |
| 74 | `+0x128` | `00439900` | `UpdateWalkingCameraB` |
| 91 | `+0x16c` | `0043b820` | `ProbePlayerRayBlend` |

Their only static references are the two vftable rows (`004a3d00/004a3d04`,
`004afc88/004afc8c`) — they are dispatched virtually per input mode
(`DAT_004f0588`), never called directly. A reads mouse deltas into the
`0x848`/`0x84c` pan accumulators and handles the SCRATCH/BUTTONS/PLAY idle
animations; B is the mouse-drive scheme (mouse X ramps `turn_or_yaw_rate`
`0x6d4` at 100 deg/s, mouse-Y speed beyond ±40 accelerates/brakes).

## The shared record-write shape

Both functions end with the identical pipeline (x87-traced from the raw
listings, tails at `00439625..004398ef` for A and `00439e33..0043a10e` for B):

```
snap = rec.pos                       # rec = DAT_00509a50; +0x44/48/4c
eye  = project(eye_offsets)          # a world-space camera position target
look = project(look_offsets)         # a world-space look point (player head)
k    = ProbePlayerRayBlend(look, &eye)   # camera collision: 1.0 free, 1.5 hit
rec.pos += (eye - snap) * axis_scales * k * dt     # per-axis smoothing
dir  = look - snap                   # NOTE: pre-update snapshot, not new pos
(ax, ay) = OMedia3DVector::angles(dir)             # 14-bit pitch/yaw
rec.angle_x += (ax - rec.angle_x)                  # full snap (mod 0x10000)
d = (ay - rec.angle_y) & 0x3fff; if d > 0x2000: d -= 0x4000
rec.angle_y += d                                   # full snap (mod 0x4000)
```

Both angle writes are **full snaps** — the camera orientation always points
from the pre-update camera position at the look point; all smoothing lives in
the position trail. (The earlier "smoothing at +0x50" wording in
`camera_record_layout.md` §Reference-sweep is superseded by this trace.)

A additionally persists the snapshot at `this+0x714/0x718/0x71c` and mirrors
the record into the debug globals `DAT_004f8418..0x442c` right before the
angle write; B keeps the snapshot on the stack only.

## Projections

The camera offsets are the `SetPlayerDefaultConstants` (`00437740`) seeds.
The local-offset axis convention is (x = side, y = up, z = forward), so the
defaults place the eye 350 behind / 200 above the player and the look point
at head height:

| field | default | role |
|---|---|---|
| `0x7c8/0x7cc/0x7d0` | `(0, 200, -350)` | eye local offset |
| `0x7d8/0x7dc/0x7e0` | `(0, 80, 0)` | look local offset |
| `0x84c` | `10.0` | A-only pitch pan (mouse-Y / analog in mode 6) |

- **A** projects both through the embedded 3D element's generic
  `transform_local` (element at `this+0xc0`, vtable `+0x384` =
  `transform_local_00472980`, three element angles in degrees × `45.511112`
  → 14-bit trig index; see `transform_local_00472980.md`), with the pan
  folded in: eye y = `0x7cc - 2*0x84c`, look y = `0x7dc + 1.9*0x84c`.
  While the auto-turn-around latch `0x828` is up, the eye target is frozen
  (reuses `this+0x804`) until `0x830 > 1.7`, then re-projects with
  z = `-0x7d0 * 3.7` (swings the eye to 1295 in FRONT for the 180° flip).
  The eye target vec4 persists at `this+0x804..0x810`.
- **B** projects the eye through `ProjectNoisyCameraTarget` (slot 72) and the
  look through the element's plain `transform_local`.

### ProjectNoisyCameraTarget (0043a5d0), repaired

`float* ProjectNoisyCameraTarget(this, float* out4, float x, float y, float z)`
is `transform_local` with two deviations (full x87 trace below):

- **pitch is hard-indexed at 0** (the sin/cos loads use table offset 0:
  `sin=0, cos=1`) — only yaw and roll of the element apply;
- **yaw = element angle_y + `this->0x6d4`** (the live turn rate) — the camera
  target leads into the turn. The summed yaw in degrees is folded before
  quantization with an original quirk: if negative, it becomes
  `360 - |int(yaw)|` (integer-degree truncation!); if ≥ 360, one 360 is
  subtracted; then index = `ftol(yaw * 45.511112) & 0x3fff`. Roll takes the
  plain `ftol(angle_z * 45.511112) & 0x3fff` path.

Final math (original space; identical shape to `CameraRecordLocalToWorld`
with A=pitch=0, B=yaw+lead, C=roll):

```
t1 = cos_roll*y - sin_roll*x
t2 = cos_roll*x + sin_roll*y
u  = z                          # cos(0)*z - t1*sin(0)
out.x = pos.x + t2*cos_yaw - u*sin_yaw
out.y = pos.y + t1
out.z = pos.z + u*cos_yaw + t2*sin_yaw
out.w = 1.0
```

## Camera collision — ProbePlayerRayBlend (0043b820), repaired

`float ProbePlayerRayBlend(this, vec4 src BY VALUE, float* eye4)`. Casts a
world ray between the caller's vectors (walking callers pass src = the look
point, eye4 = the eye target) via `FUN_0047c210` (gated on `DAT_004f69e5`;
two-stage test `0047c280`/`0047c4b0`; hit point in `DAT_005cfc60..6c` — ray
internals not traced further). On hit:

```
eye = src + 0.75 * (hit - src)      # pull the camera 75% of the way to the
return 1.5                          # obstruction and smooth 1.5x faster
```

On miss it returns the src vec's w — `transform_local` and
`ProjectNoisyCameraTarget` both set `w = 1.0`, so k = 1.0 free / 1.5 blocked.

## Per-axis smoothing scales

From the delta-scaling branches (A `0043973a..004397f8`, B
`00439f13..0043a06c`; `dt` at `[ESP+0x80]`/`[ESP+0x90]`, k at `[ESP+0x14]`):

| variant | condition | x | y | z | k applied |
|---|---|---|---|---|---|
| A | normal | `0x854` (1.6) | `0x858 + 0.2` (1.2) | `0x85c` (1.6) | yes |
| A | turn-around latch `0x828` | 1 | 1 | 1 | yes |
| A | `0x851` set | 0.2 | 0.2 | 0.2 | no |
| B | `motion_submode != 2` | 1.2 | **2.0** | 1.2 | yes |
| B | `motion_submode == 2` | 1.8 | height-gated | 1.8 | yes |
| B | `0x851` set | 0.1 | 0.1 | 0.1 | no |

B's submode-2 y gate (`00439f66..0043a025`): with ty = element world y
(slot `+0x310`) and cy = rec.y (pre-update): if `|ftol(ty - cy)| > 800`
→ y scale 1.5; else if `cy < ty + 20.0` → 1.5; else
`|ftol(dy - cy)| <= 550` → 0.2, else 0.8 — where `dy` is the y **delta**
(eye.y − snap.y), so the last gate compares a delta against an absolute
height (recovered as-is; looks like an original bug, kept faithfully).

## OMedia3DVector::angles — external import

The angle extraction calls through IAT `0x0048d108` → import `angles`
(`OMT2.dll`, the game's OMedia/OMT runtime). The authoritative body is the
LGPL Open Media Toolkit source (`~/omt-src/open-media-toolkit-master`,
`sources/OMTClasses/Math/OMedia3DVector.cpp` + `OMediaTrigo.h`, © Yves
Schmid / GarageCube):

```cpp
void OMedia3DVector::angles(omt_Angle &ax, omt_Angle &ay)
{
    OMedia3DVector v(x,y,z);
    ay = (-omd_ATan2(v.x,v.z)) & omc_MaxAngleMask;        // 0x3fff
    v.rotate(0,-ay,0);
    ax = ((-omd_ATan2(v.z,v.y)) + omd_Deg2Angle(90)) & omc_MaxAngleMask;
    if (ax > omc_MaxAngle>>1) ax = (short)-(omc_MaxAngle - ax);
}
```

with the table-based `atan2` (`OMediaCosSinTable::atan2`):

```cpp
omt_Angle atan2(float opp, float adj)
{
    if (opp==0.0 && adj==0.0) return 0;
    adj2 = adj*adj;  hyp = adj2 + opp*opp;
    t = long(adj2 * 0x4000 / hyp);          // acos_table_size = 0x4000
    angle = acos_tab[t];                    // acos_tab[l] = short(acos(l/0x2000 - 1)*8192/pi / 2)
    if (adj < 0)  return opp < 0 ? 8192 + angle : 8192 - angle;
    else          return opp < 0 ? 16384 - angle : angle;
}
```

(`omd_Deg2Angle(90/180/360) = 4096/8192/16384`; the float `t` index math and
the short-truncated `acos_tab` are transcribed exactly, table included, in the
native port and its oracle reference.)
`rotate(0,-ay,0)` is the OMT left-hand y rotation:
`x' = x*cos(-ay) - z*sin(-ay); z' = x*sin(-ay) + z*cos(-ay)`.

The record consumes these angles directly (`FrameStepAndRender` view build),
so the pair convention is: `ax` → record `angle_x` (+0x50, pitch), `ay` →
record `angle_y` (+0x52, yaw), matching the certified
`yaw = -angle_y, pitch = +angle_x` native bridge.

## Native port scope (src/game/camera_record.c)

`camera_record_walkcam_write` ports the **B normal path** (submode ≠ 2 — the
plain walking camera): ProjectNoisy eye with turn lead, plain-transform look,
probe blend hook, `(1.2, 2.0, 1.2)` scales, snapshot-anchored angle snap.
Native-space mapping is the project-wide Z mirror: native pos z = −original z,
original yaw_deg = degrees(native ry) − 180 (from the native player forward
`(sin ry, 0, cos ry)` vs the original transform forward `(−sin B, 0, cos B)`).
Documented deviations, mirroring the retarget row's style:

- the native world-ray query is not wired into the demo (probe hook defaults
  to NULL → k = 1.0); the blend/k mechanism itself IS ported and certified;
- A's variant (pan offsets, 0x854..0x85c scales, turn-around eye freeze),
  B's submode-2 scales, and the `0x851` modes are recovered above but not
  ported — they hang off player-movement state that native does not carry
  (see the `free-roam-feel` linked-blocked row);
- the original's `0x6d4` input ramp is player-movement state; the demo
  wrapper feeds the lead from the observed native yaw rate clamped to the
  recovered ±30 deg limit instead.

## Raw repaired dumps

Appended below: the prototype-repaired decompiles of both walking helpers and
the input/copy helpers, the raw listing tails of both record writes (the
authoritative x87 source for the tables above), the repaired
`ProjectNoisyCameraTarget` / `ProbePlayerRayBlend` decompiles with the
`ProjectNoisyCameraTarget` listing, and the IAT identification of the
`angles` import.


## UpdateWalkingCameraA_00438bc0 @ 00438bc0

```c

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void __thiscall UpdateWalkingCameraA_00438bc0(void *this,float dt)

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
  float local_64;
  float local_60;
  int local_5c;
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
  
  pfStack_80 = &local_60;
  local_64 = 0.0;
  local_60 = 0.0;
  FUN_0046a460(&local_64);
  if (DAT_004f83e0 != '\0') {
    return;
  }
  local_5c = (int)local_64._0_2_;
  *(float *)((int)this + 0x848) = (float)local_5c + *(float *)((int)this + 0x848);
  pfStack_80 = (float *)0x438c16;
  cVar5 = (**(code **)(*(int *)this + 0x15c))();
  if (cVar5 == '\0') {
    if (DAT_004f0588 != 6) {
      *(undefined4 *)((int)this + 0x84c) = 0x41200000;
    }
  }
  else if (DAT_004f0588 == 6) {
    if ((_DAT_00509aa4 < -0.5) || (DAT_00509860 != '\0')) {
      *(float *)((int)this + 0x84c) = *(float *)((int)this + 0x84c) - dt * 20.0;
    }
    else if ((0.5 < _DAT_00509aa4) || (DAT_0050985e != '\0')) {
      *(float *)((int)this + 0x84c) = dt * 20.0 + *(float *)((int)this + 0x84c);
    }
    else {
      local_5c = (int)local_60._0_2_;
      *(float *)((int)this + 0x84c) = (float)local_5c * dt + *(float *)((int)this + 0x84c);
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
    local_60 = (float)((int)((uVar11 >> 0x10) * 0x3e9) >> 0x10);
    fVar1 = (float)(int)local_60 * 0.001;
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
  if (((((local_64._0_2_ < -0x1e) || (DAT_00509860 != '\0')) || (_DAT_00509aa4 < -0.5)) &&
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
  local_60 = (float)(int)sStack_68;
  fStack_6c = (float)(int)local_60 *
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
      local_60 = *(float *)((int)this + 0x6c4) * -0.5;
      if (fVar1 < local_60) {
        *(float *)((int)this + 0x6c8) = local_60;
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
  if ((*(char *)((int)this + 0x850) == '\0') && (1 < SUB42(local_64,0))) {
    local_60 = (float)((int)SUB42(local_64,0) << 4);
    fVar1 = (float)(int)local_60 + *(float *)((int)this + 0x6c8);
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
      local_64 = *(float *)(iVar8 + 4) - unaff_EBX * 0.5;
      *(float *)((int)this + 0x710) = local_64;
      (**(code **)(*(int *)((int)this + 0xc0) + 0x36c))(0,local_64,0);
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
  local_64 = unaff_EBP - *(float *)((int)this + 0x718);
  local_60 = unaff_EBX - *(float *)((int)this + 0x71c);
  if (*(char *)((int)this + 0x851) == '\0') {
    if (*(char *)((int)this + 0x828) == '\0') {
      fVar2 = fVar2 * *(float *)((int)this + 0x854);
      fVar16 = local_64 * fStack_24 * (*(float *)((int)this + 0x858) + 0.2) * fVar1;
      fVar3 = local_60 * *(float *)((int)this + 0x85c);
    }
    else {
      fVar16 = local_64 * fVar1 * fStack_24;
      fVar3 = local_60;
    }
    fVar2 = fVar2 * fVar1 * fStack_24;
    fStack_24 = fVar3 * fVar1 * fStack_24;
  }
  else {
    fVar2 = fVar2 * fStack_24 * 0.2;
    fVar16 = local_64 * fStack_24 * 0.2;
    fStack_24 = local_60 * fStack_24 * 0.2;
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

void __thiscall UpdateWalkingCameraB_00439900(void *this,float dt)

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
  float local_70;
  float local_6c [2];
  float fStack_64;
  float fStack_60;
  float fStack_5c;
  float fStack_58;
  float fStack_54;
  undefined1 auStack_50 [16];
  float fStack_40;
  
  pfStack_90 = local_6c;
  pfStack_94 = &local_70;
  local_70 = 0.0;
  local_6c[0] = 0.0;
  uStack_98 = 0x439922;
  FUN_0046a460();
  fVar2 = (float)(int)local_70._0_2_;
  if (fVar2 == 0.0) {
    if ((DAT_0050985d != '\0') || (_DAT_00509aa0 < -0.5)) {
      *(undefined1 *)((int)this + 0x6e6) = 1;
      fVar2 = dt * 100.0 + fVar2;
    }
    else if ((DAT_0050985f != '\0') || (0.5 < _DAT_00509aa0)) {
      *(undefined1 *)((int)this + 0x6e6) = 1;
      fVar2 = fVar2 - dt * 100.0;
    }
    else if (*(char *)((int)this + 0x6e6) != '\0') {
      if (0.0 <= *(float *)((int)this + 0x6d4)) {
        if ((0.0 < *(float *)((int)this + 0x6d4)) &&
           (fVar6 = *(float *)((int)this + 0x6d4) - dt * 100.0,
           *(float *)((int)this + 0x6d4) = fVar6, fVar6 < 0.0)) {
          *(undefined4 *)((int)this + 0x6d4) = 0;
        }
      }
      else {
        fVar6 = dt * 100.0 + *(float *)((int)this + 0x6d4);
        *(float *)((int)this + 0x6d4) = fVar6;
        if (0.0 < fVar6) {
          *(undefined4 *)((int)this + 0x6d4) = 0;
        }
      }
    }
  }
  else {
    fVar2 = fVar2 * dt;
    *(undefined1 *)((int)this + 0x6e6) = 0;
  }
  fVar6 = (float)(int)local_6c[0]._0_2_;
  *(float *)((int)this + 0x6d4) = fVar2 + *(float *)((int)this + 0x6d4);
  *(undefined1 *)((int)this + 0x6e4) = 0;
  *(undefined1 *)((int)this + 0x6e5) = 0;
  fVar2 = (1.0 / dt) * (float)(int)fVar6;
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
  pfStack_ac = &local_70;
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
  uStack_c0 = &local_70;
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
  fStack_60 = local_70 - (float)pfStack_90;
  fStack_5c = local_6c[0] - unaff_EDI;
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

## FUN_0046a460 @ 0046a460

```c

void FUN_0046a460(short *param_1,short *param_2)

{
  short sVar1;
  short local_8 [2];
  short local_4 [2];
  
  FUN_00476d70(local_8,local_4);
  sVar1 = FUN_00476dd0();
  *param_1 = sVar1 - local_8[0];
  sVar1 = FUN_00476df0();
  *param_2 = sVar1 - local_4[0];
  FUN_00476db0();
  return;
}


```

## FUN_00409f60 @ 00409f60

```c

void FUN_00409f60(undefined4 *param_1)

{
  undefined4 *in_ECX;
  
  *in_ECX = *param_1;
  in_ECX[1] = param_1[1];
  in_ECX[2] = param_1[2];
  in_ECX[3] = param_1[3];
  return;
}


```

## LISTING UpdateWalkingCameraA record-write tail (00439625..004398ef)

```asm
00439625  FLD float ptr [EBP + 0x84c]
0043962b  MOV EDX,dword ptr [EBP + 0x7d0]
00439631  LEA ECX,[EBP + 0xc0]
00439637  FADD ST0,ST0
00439639  MOV EAX,dword ptr [ECX]
0043963b  PUSH EDX
0043963c  MOV EDX,dword ptr [EBP + 0x7c8]
00439642  PUSH ECX
00439643  FSUBR float ptr [EBP + 0x7cc]
00439649  FSTP float ptr [ESP]
0043964c  PUSH EDX
0043964d  LEA EDX,[ESP + 0x48]
00439651  PUSH EDX
00439652  CALL dword ptr [EAX + 0x384]
00439658  MOV ESI,EAX
0043965a  LEA EDI,[EBP + 0x804]
00439660  MOV EAX,dword ptr [ESI]
00439662  MOV dword ptr [ESP + 0x2c],EAX
00439666  MOV ECX,dword ptr [ESI + 0x4]
00439669  MOV dword ptr [ESP + 0x30],ECX
0043966d  MOV EDX,dword ptr [ESI + 0x8]
00439670  MOV dword ptr [ESP + 0x34],EDX
00439674  MOV ESI,dword ptr [ESI + 0xc]
00439677  MOV dword ptr [EDI],EAX
00439679  MOV dword ptr [ESP + 0x38],ESI
0043967d  MOV dword ptr [EDI + 0x4],ECX
00439680  MOV dword ptr [EDI + 0x8],EDX
00439683  MOV dword ptr [EDI + 0xc],ESI
00439686  JMP 0x004396a9
00439688  LEA EAX,[EBP + 0x804]
0043968e  MOV ECX,dword ptr [EAX]
00439690  MOV dword ptr [ESP + 0x2c],ECX
00439694  MOV EDX,dword ptr [EAX + 0x4]
00439697  MOV dword ptr [ESP + 0x30],EDX
0043969b  MOV ECX,dword ptr [EAX + 0x8]
0043969e  MOV dword ptr [ESP + 0x34],ECX
004396a2  MOV EDX,dword ptr [EAX + 0xc]
004396a5  MOV dword ptr [ESP + 0x38],EDX
004396a9  FLD float ptr [EBP + 0x84c]
004396af  FMUL double ptr [0x004afce8]
004396b5  MOV EDX,dword ptr [EBP + 0x7e0]
004396bb  LEA ECX,[EBP + 0xc0]
004396c1  PUSH EDX
004396c2  MOV EDX,dword ptr [EBP + 0x7d8]
004396c8  FADD float ptr [EBP + 0x7dc]
004396ce  MOV EAX,dword ptr [ECX]
004396d0  PUSH ECX
004396d1  FSTP float ptr [ESP]
004396d4  PUSH EDX
004396d5  LEA EDX,[ESP + 0x48]
004396d9  PUSH EDX
004396da  CALL dword ptr [EAX + 0x384]
004396e0  MOV ECX,dword ptr [EAX]
004396e2  MOV ESI,dword ptr [EBP]
004396e5  MOV dword ptr [ESP + 0x4c],ECX
004396e9  MOV EDX,dword ptr [EAX + 0x4]
004396ec  MOV dword ptr [ESP + 0x50],EDX
004396f0  MOV ECX,dword ptr [EAX + 0x8]
004396f3  MOV dword ptr [ESP + 0x54],ECX
004396f7  MOV EDX,dword ptr [EAX + 0xc]
004396fa  LEA EAX,[ESP + 0x2c]
004396fe  MOV dword ptr [ESP + 0x58],EDX
00439702  PUSH EAX
00439703  LEA EDX,[ESP + 0x50]
00439707  SUB ESP,0x10
0043970a  MOV ECX,ESP
0043970c  PUSH EDX
0043970d  CALL 0x00409f60
00439712  MOV ECX,EBP
00439714  CALL dword ptr [ESI + 0x16c]
0043971a  FSTP float ptr [ESP + 0x14]
0043971e  FLD float ptr [ESP + 0x2c]
00439722  FSUB float ptr [EBP + 0x714]
00439728  FLD float ptr [ESP + 0x30]
0043972c  FSUB float ptr [EBP + 0x718]
00439732  MOV AL,byte ptr [EBP + 0x851]
00439738  TEST AL,AL
0043973a  FSTP float ptr [ESP + 0x40]
0043973e  FLD float ptr [ESP + 0x34]
00439742  FSUB float ptr [EBP + 0x71c]
00439748  FSTP float ptr [ESP + 0x44]
0043974c  JZ 0x00439783
0043974e  FMUL float ptr [ESP + 0x80]
00439755  FMUL double ptr [0x004a2ad0]
0043975b  FLD float ptr [ESP + 0x40]
0043975f  FMUL float ptr [ESP + 0x80]
00439766  FMUL double ptr [0x004a2ad0]
0043976c  FSTP float ptr [ESP + 0x20]
00439770  FLD float ptr [ESP + 0x44]
00439774  FMUL float ptr [ESP + 0x80]
0043977b  FMUL double ptr [0x004a2ad0]
00439781  JMP 0x004397f8
00439783  MOV AL,byte ptr [EBP + 0x828]
00439789  TEST AL,AL
0043978b  JZ 0x004397b1
0043978d  FMUL float ptr [ESP + 0x14]
00439791  FMUL float ptr [ESP + 0x80]
00439798  FLD float ptr [ESP + 0x40]
0043979c  FMUL float ptr [ESP + 0x14]
004397a0  FMUL float ptr [ESP + 0x80]
004397a7  FSTP float ptr [ESP + 0x20]
004397ab  FLD float ptr [ESP + 0x44]
004397af  JMP 0x004397ed
004397b1  FMUL float ptr [EBP + 0x854]
004397b7  FMUL float ptr [ESP + 0x14]
004397bb  FMUL float ptr [ESP + 0x80]
004397c2  FLD float ptr [EBP + 0x858]
004397c8  FADD double ptr [0x004a2ad0]
004397ce  FLD float ptr [ESP + 0x40]
004397d2  FMUL float ptr [ESP + 0x80]
004397d9  FMULP
004397db  FMUL float ptr [ESP + 0x14]
004397df  FSTP float ptr [ESP + 0x20]
004397e3  FLD float ptr [ESP + 0x44]
004397e7  FMUL float ptr [EBP + 0x85c]
004397ed  FMUL float ptr [ESP + 0x14]
004397f1  FMUL float ptr [ESP + 0x80]
004397f8  MOV EAX,[0x00509a50]
004397fd  LEA ECX,[ESP + 0x24]
00439801  FSTP float ptr [ESP + 0x80]
00439808  ADD EAX,0x44
0043980b  LEA EDX,[ESP + 0x28]
0043980f  PUSH ECX
00439810  PUSH EDX
00439811  FADD float ptr [EAX]
00439813  LEA ECX,[ESP + 0x74]
00439817  FSTP float ptr [EAX]
00439819  FLD float ptr [ESP + 0x28]
0043981d  FADD float ptr [EAX + 0x4]
00439820  FSTP float ptr [EAX + 0x4]
00439823  FLD float ptr [ESP + 0x88]
0043982a  FADD float ptr [EAX + 0x8]
0043982d  FSTP float ptr [EAX + 0x8]
00439830  FLD float ptr [ESP + 0x54]
00439834  FSUB float ptr [EBP + 0x714]
0043983a  MOV dword ptr [ESP + 0x80],0x0
00439845  FSTP float ptr [ESP + 0x74]
00439849  FLD float ptr [ESP + 0x58]
0043984d  FSUB float ptr [EBP + 0x718]
00439853  FSTP float ptr [ESP + 0x78]
00439857  FLD float ptr [ESP + 0x5c]
0043985b  FSUB float ptr [EBP + 0x71c]
00439861  FSTP float ptr [ESP + 0x7c]
00439865  CALL dword ptr [0x0048d108]
0043986b  MOV ECX,dword ptr [0x00509a50]
00439871  MOV EAX,dword ptr [ESP + 0x24]
00439875  MOV DI,word ptr [ECX + 0x50]
00439879  LEA EDX,[ECX + 0x50]
0043987c  MOV SI,word ptr [ECX + 0x52]
00439880  MOVSX EBX,DI
00439883  MOV dword ptr [ESP + 0x80],EBX
0043988a  SUB EAX,ESI
0043988c  FILD dword ptr [ESP + 0x80]
00439893  MOVSX ESI,SI
00439896  FSTP float ptr [0x004f8428]
0043989c  MOV dword ptr [ESP + 0x80],ESI
004398a3  AND EAX,0x3fff
004398a8  FILD dword ptr [ESP + 0x80]
004398af  CMP AX,0x2000
004398b3  FSTP float ptr [0x004f842c]
004398b9  FLD float ptr [ECX + 0x44]
004398bc  FSTP float ptr [0x004f8418]
004398c2  FLD float ptr [ECX + 0x48]
004398c5  FSTP float ptr [0x004f841c]
004398cb  FLD float ptr [ECX + 0x4c]
004398ce  FSTP float ptr [0x004f8420]
004398d4  JLE 0x004398db
004398d6  SUB EAX,0x4000
004398db  MOV ECX,dword ptr [ESP + 0x28]
004398df  SUB ECX,EDI
004398e1  ADD word ptr [EDX],CX
004398e4  ADD word ptr [EDX + 0x2],AX
004398e8  POP EDI
004398e9  POP ESI
004398ea  POP EBP
004398eb  POP EBX
004398ec  ADD ESP,0x6c
004398ef  RET 0x4
```

## LISTING UpdateWalkingCameraB record-write tail (00439e33..0043a10e)

```asm
00439e33  MOV EAX,[0x00509a50]
00439e38  MOV ECX,dword ptr [ESI + 0x7cc]
00439e3e  MOV EDX,dword ptr [ESI]
00439e40  FLD float ptr [EAX + 0x44]
00439e43  FSTP float ptr [ESP + 0x3c]
00439e47  FLD float ptr [EAX + 0x48]
00439e4a  FSTP float ptr [ESP + 0x40]
00439e4e  FLD float ptr [EAX + 0x4c]
00439e51  MOV EAX,dword ptr [ESI + 0x7d0]
00439e57  PUSH EAX
00439e58  MOV EAX,dword ptr [ESI + 0x7c8]
00439e5e  PUSH ECX
00439e5f  LEA ECX,[ESP + 0x34]
00439e63  PUSH EAX
00439e64  PUSH ECX
00439e65  FSTP float ptr [ESP + 0x54]
00439e69  MOV ECX,ESI
00439e6b  CALL dword ptr [EDX + 0x120]
00439e71  MOV EDX,dword ptr [EAX]
00439e73  MOV dword ptr [ESP + 0x4c],EDX
00439e77  MOV ECX,dword ptr [EAX + 0x4]
00439e7a  MOV dword ptr [ESP + 0x50],ECX
00439e7e  MOV EDX,dword ptr [EAX + 0x8]
00439e81  MOV ECX,dword ptr [ESI + 0x7dc]
00439e87  MOV dword ptr [ESP + 0x54],EDX
00439e8b  MOV EAX,dword ptr [EAX + 0xc]
00439e8e  MOV EDX,dword ptr [EDI]
00439e90  MOV dword ptr [ESP + 0x58],EAX
00439e94  MOV EAX,dword ptr [ESI + 0x7e0]
00439e9a  PUSH EAX
00439e9b  MOV EAX,dword ptr [ESI + 0x7d8]
00439ea1  PUSH ECX
00439ea2  LEA ECX,[ESP + 0x34]
00439ea6  PUSH EAX
00439ea7  PUSH ECX
00439ea8  MOV ECX,EDI
00439eaa  CALL dword ptr [EDX + 0x384]
00439eb0  MOV EDX,dword ptr [EAX]
00439eb2  MOV EBP,dword ptr [ESI]
00439eb4  MOV dword ptr [ESP + 0x5c],EDX
00439eb8  MOV ECX,dword ptr [EAX + 0x4]
00439ebb  MOV dword ptr [ESP + 0x60],ECX
00439ebf  MOV EDX,dword ptr [EAX + 0x8]
00439ec2  LEA ECX,[ESP + 0x4c]
00439ec6  MOV dword ptr [ESP + 0x64],EDX
00439eca  MOV EAX,dword ptr [EAX + 0xc]
00439ecd  PUSH ECX
00439ece  SUB ESP,0x10
00439ed1  LEA EDX,[ESP + 0x70]
00439ed5  MOV ECX,ESP
00439ed7  MOV dword ptr [ESP + 0x7c],EAX
00439edb  PUSH EDX
00439edc  CALL 0x00409f60
00439ee1  MOV ECX,ESI
00439ee3  CALL dword ptr [EBP + 0x16c]
00439ee9  FSTP float ptr [ESP + 0x14]
00439eed  FLD float ptr [ESP + 0x4c]
00439ef1  FSUB float ptr [ESP + 0x3c]
00439ef5  FLD float ptr [ESP + 0x50]
00439ef9  FSUB float ptr [ESP + 0x40]
00439efd  CMP byte ptr [ESI + 0x851],BL
00439f03  FSTP float ptr [ESP + 0x30]
00439f07  FLD float ptr [ESP + 0x54]
00439f0b  FSUB float ptr [ESP + 0x44]
00439f0f  FSTP float ptr [ESP + 0x34]
00439f13  JZ 0x00439f4d
00439f15  FMUL float ptr [ESP + 0x90]
00439f1c  FMUL double ptr [0x0049cfb8]
00439f22  FSTP float ptr [ESP + 0x18]
00439f26  FLD float ptr [ESP + 0x30]
00439f2a  FMUL float ptr [ESP + 0x90]
00439f31  FMUL double ptr [0x0049cfb8]
00439f37  FLD float ptr [ESP + 0x34]
00439f3b  FMUL float ptr [ESP + 0x90]
00439f42  FMUL double ptr [0x0049cfb8]
00439f48  JMP 0x0043a06c
00439f4d  FMUL float ptr [ESP + 0x90]
00439f54  CMP word ptr [ESI + 0x7c4],0x2
00439f5c  FMUL float ptr [ESP + 0x14]
00439f60  JNZ 0x0043a03c
00439f66  MOV EAX,[0x00509a50]
00439f6b  MOV EDX,dword ptr [EDI]
00439f6d  FMUL double ptr [0x00494cb8]
00439f73  MOV ECX,dword ptr [EAX + 0x48]
00439f76  LEA EAX,[ESP + 0x7c]
00439f7a  MOV dword ptr [ESP + 0x24],ECX
00439f7e  PUSH EAX
00439f7f  MOV ECX,EDI
00439f81  FSTP float ptr [ESP + 0x1c]
00439f85  CALL dword ptr [EDX + 0x310]
00439f8b  FLD float ptr [EAX + 0x4]
00439f8e  FSUB float ptr [ESP + 0x24]
00439f92  CALL 0x0047f940
00439f97  CDQ
00439f98  XOR EAX,EDX
00439f9a  SUB EAX,EDX
00439f9c  CMP EAX,0x320
00439fa1  JG 0x0043a010
00439fa3  MOV ECX,dword ptr [0x00509a50]
00439fa9  MOV EAX,dword ptr [EDI]
00439fab  MOV EDX,dword ptr [ECX + 0x48]
00439fae  LEA ECX,[ESP + 0x7c]
00439fb2  PUSH ECX
00439fb3  MOV ECX,EDI
00439fb5  MOV dword ptr [ESP + 0x28],EDX
00439fb9  CALL dword ptr [EAX + 0x310]
00439fbf  FLD float ptr [EAX + 0x4]
00439fc2  FADD float ptr [0x00495324]
00439fc8  FCOMP float ptr [ESP + 0x24]
00439fcc  FNSTSW AX
00439fce  TEST AH,0x41
00439fd1  JZ 0x0043a010
00439fd3  MOV EDX,dword ptr [0x00509a50]
00439fd9  FLD float ptr [EDX + 0x48]
00439fdc  FSUBR float ptr [ESP + 0x30]
00439fe0  CALL 0x0047f940
00439fe5  FLD float ptr [ESP + 0x30]
00439fe9  FMUL float ptr [ESP + 0x90]
00439ff0  CDQ
00439ff1  FMUL float ptr [ESP + 0x14]
00439ff5  XOR EAX,EDX
00439ff7  SUB EAX,EDX
00439ff9  CMP EAX,0x226
00439ffe  JLE 0x0043a008
0043a000  FMUL double ptr [0x004a6850]
0043a006  JMP 0x0043a025
0043a008  FMUL double ptr [0x004a2ad0]
0043a00e  JMP 0x0043a025
0043a010  FLD float ptr [ESP + 0x30]
0043a014  FMUL float ptr [ESP + 0x90]
0043a01b  FMUL float ptr [ESP + 0x14]
0043a01f  FMUL double ptr [0x0048e598]
0043a025  FLD float ptr [ESP + 0x34]
0043a029  FMUL float ptr [ESP + 0x90]
0043a030  FMUL float ptr [ESP + 0x14]
0043a034  FMUL double ptr [0x00494cb8]
0043a03a  JMP 0x0043a06c
0043a03c  FMUL double ptr [0x004a0ce0]
0043a042  FSTP float ptr [ESP + 0x18]
0043a046  FLD float ptr [ESP + 0x30]
0043a04a  FMUL float ptr [ESP + 0x14]
0043a04e  FMUL float ptr [ESP + 0x90]
0043a055  FADD ST0,ST0
0043a057  FLD float ptr [ESP + 0x34]
0043a05b  FMUL float ptr [ESP + 0x90]
0043a062  FMUL float ptr [ESP + 0x14]
0043a066  FMUL double ptr [0x004a0ce0]
0043a06c  MOV EAX,[0x00509a50]
0043a071  LEA ECX,[ESP + 0x12]
0043a075  FSTP float ptr [ESP + 0x14]
0043a079  FLD float ptr [ESP + 0x18]
0043a07d  FADD float ptr [EAX + 0x44]
0043a080  ADD EAX,0x44
0043a083  LEA EDX,[ESP + 0x28]
0043a087  PUSH ECX
0043a088  PUSH EDX
0043a089  LEA ECX,[ESP + 0x74]
0043a08d  FSTP float ptr [EAX]
0043a08f  FADD float ptr [EAX + 0x4]
0043a092  FSTP float ptr [EAX + 0x4]
0043a095  FLD float ptr [ESP + 0x1c]
0043a099  FADD float ptr [EAX + 0x8]
0043a09c  FSTP float ptr [EAX + 0x8]
0043a09f  FLD float ptr [ESP + 0x64]
0043a0a3  FSUB float ptr [ESP + 0x44]
0043a0a7  MOV dword ptr [ESP + 0x80],0x0
0043a0b2  FSTP float ptr [ESP + 0x74]
0043a0b6  FLD float ptr [ESP + 0x68]
0043a0ba  FSUB float ptr [ESP + 0x48]
0043a0be  FSTP float ptr [ESP + 0x78]
0043a0c2  FLD float ptr [ESP + 0x6c]
0043a0c6  FSUB float ptr [ESP + 0x4c]
0043a0ca  FSTP float ptr [ESP + 0x7c]
0043a0ce  CALL dword ptr [0x0048d108]
0043a0d4  MOV EDX,dword ptr [0x00509a50]
0043a0da  MOV AX,word ptr [ESP + 0x12]
0043a0df  SUB AX,word ptr [EDX + 0x52]
0043a0e3  MOV SI,word ptr [EDX + 0x50]
0043a0e7  LEA ECX,[EDX + 0x50]
0043a0ea  AND EAX,0x3fff
0043a0ef  CMP AX,0x2000
0043a0f3  JLE 0x0043a0fa
0043a0f5  SUB EAX,0x4000
0043a0fa  MOV EDX,dword ptr [ESP + 0x28]
0043a0fe  POP EDI
0043a0ff  SUB EDX,ESI
0043a101  POP ESI
0043a102  ADD word ptr [ECX],DX
0043a105  ADD word ptr [ECX + 0x2],AX
0043a109  POP EBP
0043a10a  POP EBX
0043a10b  ADD ESP,0x7c
0043a10e  RET 0x4
```

## REFS TO 00438bc0
- 004a3d00 (DATA) in (no function)
- 004afc88 (DATA) in (no function)

## REFS TO 00439900
- 004a3d04 (DATA) in (no function)
- 004afc8c (DATA) in (no function)

repaired float * ProjectNoisyCameraTarget_0043a5d0(void * this, float * out4, float x, float y, float z)
repaired float ProbePlayerRayBlend_0043b820(void * this, float src_x, float src_y, float src_z, float src_w, float * eye4)

## ProjectNoisyCameraTarget_0043a5d0 @ 0043a5d0

```c

float * __thiscall ProjectNoisyCameraTarget_0043a5d0(void *this,float *out4,float x,float y,float z)

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
  undefined1 local_20 [4];
  undefined4 uStack_1c;
  undefined1 auStack_14 [16];
  float *pfStack_4;
  
  iVar10 = (**(code **)(*(int *)((int)this + 0xc0) + 0x328))(local_20);
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
  fVar9 = fVar6 * x - fVar5 * (float)out4;
  pfVar13 = (float *)(**(code **)(*(int *)((int)this + 0xc0) + 0x310))(auStack_14);
  fVar7 = pfVar13[1];
  fVar8 = pfVar13[2];
  *pfStack_4 = *pfVar13 + fVar9;
  pfStack_4[1] = ((fVar5 * x + fVar6 * (float)out4) * fVar4 - (fVar2 * y - fVar9 * fVar1) * fVar3) +
                 fVar7;
  pfStack_4[2] = fVar1 * y + fVar9 * fVar2 + fVar8;
  pfStack_4[3] = 1.0;
  return pfStack_4;
}


```

## ProbePlayerRayBlend_0043b820 @ 0043b820

```c

float __thiscall
ProbePlayerRayBlend_0043b820(void *this,float src_x,float src_y,float src_z,float src_w,float *eye4)

{
  char cVar1;
  float unaff_retaddr;
  undefined4 local_14;
  undefined1 local_10 [12];
  float fStack_4;
  
  local_14 = 0x3f800000;
  (**(code **)(*(int *)((int)this + 0xc0) + 0x310))();
  (**(code **)(*(int *)((int)this + 0xc0) + 0x310))(&local_14);
  FUN_00409f60(src_z);
  FUN_00409f60(&fStack_4);
  cVar1 = FUN_0047c210();
  if (cVar1 != '\0') {
    *(undefined4 *)src_z = DAT_005cfc60;
    *(undefined4 *)((int)src_z + 4) = DAT_005cfc64;
    *(undefined4 *)((int)src_z + 8) = DAT_005cfc68;
    *(undefined4 *)((int)src_z + 0xc) = DAT_005cfc6c;
    *(float *)src_z = fStack_4 - (fStack_4 - *(float *)src_z) * 0.75;
    *(float *)((int)src_z + 4) = unaff_retaddr - (unaff_retaddr - *(float *)((int)src_z + 4)) * 0.75
    ;
    *(float *)((int)src_z + 8) = src_x - (src_x - *(float *)((int)src_z + 8)) * 0.75;
    return 1.5;
  }
  return (float)local_10;
}


```

## FUN_0047c210 @ 0047c210

```c

undefined4 FUN_0047c210(void)

{
  char cVar1;
  undefined4 uVar2;
  undefined4 in_stack_00000024;
  
  if (DAT_004f69e5 == '\0') {
    return 0;
  }
  FUN_00409f60(&stack0x00000014);
  FUN_00409f60(&stack0x00000004);
  cVar1 = FUN_0047c280();
  if (cVar1 != '\0') {
    return 1;
  }
  FUN_00409f60(&stack0x00000014);
  FUN_00409f60(&stack0x00000004);
  uVar2 = FUN_0047c4b0();
  return uVar2;
}


```

## LISTING ProjectNoisyCameraTarget @ 0043a5d0

```asm
0043a5d0  SUB ESP,0x34
0043a5d3  PUSH ESI
0043a5d4  MOV ESI,ECX
0043a5d6  PUSH EDI
0043a5d7  LEA ECX,[ESP + 0x1c]
0043a5db  MOV EAX,dword ptr [ESI + 0xc0]
0043a5e1  LEA EDI,[ESI + 0xc0]
0043a5e7  PUSH ECX
0043a5e8  MOV ECX,EDI
0043a5ea  CALL dword ptr [EAX + 0x328]
0043a5f0  FLD float ptr [EAX + 0x4]
0043a5f3  FADD float ptr [ESI + 0x6d4]
0043a5f9  MOV EDX,dword ptr [EAX + 0x8]
0043a5fc  MOV dword ptr [ESP + 0x24],EDX
0043a600  FCOM float ptr [0x0048d914]
0043a606  FNSTSW AX
0043a608  TEST AH,0x1
0043a60b  JZ 0x0043a628
0043a60d  CALL 0x0047f940
0043a612  CDQ
0043a613  XOR EAX,EDX
0043a615  MOV ECX,0x168
0043a61a  SUB EAX,EDX
0043a61c  SUB ECX,EAX
0043a61e  MOV dword ptr [ESP + 0x8],ECX
0043a622  FILD dword ptr [ESP + 0x8]
0043a626  JMP 0x0043a63b
0043a628  FCOM float ptr [0x0048e5c4]
0043a62e  FNSTSW AX
0043a630  TEST AH,0x41
0043a633  JNZ 0x0043a63b
0043a635  FSUB float ptr [0x0048e5c4]
0043a63b  FLD float ptr [ESP + 0x24]
0043a63f  FMUL float ptr [0x0048e5e8]
0043a645  CALL 0x0047f940
0043a64a  FMUL float ptr [0x0048e5e8]
0043a650  MOV ESI,EAX
0043a652  CALL 0x0047f940
0043a657  MOV ECX,dword ptr [0x0048d10c]
0043a65d  AND EAX,0x3fff
0043a662  SHL EAX,0x2
0043a665  FLD float ptr [ECX]
0043a667  FLD float ptr [ECX + 0x10004]
0043a66d  FLD float ptr [EAX + ECX*0x1]
0043a670  FLD float ptr [EAX + ECX*0x1 + 0x10004]
0043a677  AND ESI,0x3fff
0043a67d  MOV EDX,dword ptr [EDI]
0043a67f  SHL ESI,0x2
0043a682  LEA EAX,[ESP + 0x2c]
0043a686  FLD float ptr [ESI + ECX*0x1]
0043a689  FLD float ptr [ESI + ECX*0x1 + 0x10004]
0043a690  FLD ST0
0043a692  FMUL float ptr [ESP + 0x48]
0043a696  FLD ST2
0043a698  FMUL float ptr [ESP + 0x44]
0043a69c  PUSH EAX
0043a69d  MOV ECX,EDI
0043a69f  FSUBP
0043a6a1  FSTP float ptr [ESP + 0xc]
0043a6a5  FMUL float ptr [ESP + 0x48]
0043a6a9  FXCH
0043a6ab  FMUL float ptr [ESP + 0x4c]
0043a6af  FADDP
0043a6b1  FSTP float ptr [ESP + 0x48]
0043a6b5  FLD ST2
0043a6b7  FMUL float ptr [ESP + 0x50]
0043a6bb  FLD float ptr [ESP + 0xc]
0043a6bf  FMUL ST5
0043a6c1  FSUBP
0043a6c3  FLD float ptr [ESP + 0x48]
0043a6c7  FMUL ST2
0043a6c9  FLD ST1
0043a6cb  FMUL ST4
0043a6cd  FSUBP
0043a6cf  FSTP float ptr [ESP + 0x10]
0043a6d3  FMUL ST1
0043a6d5  FLD float ptr [ESP + 0x48]
0043a6d9  FMUL ST3
0043a6db  FADDP
0043a6dd  FSTP float ptr [ESP + 0x18]
0043a6e1  FSTP ST0
0043a6e3  FSTP ST0
0043a6e5  FLD float ptr [ESP + 0xc]
0043a6e9  FMUL ST1
0043a6eb  FXCH ST2
0043a6ed  FMUL float ptr [ESP + 0x50]
0043a6f1  FADDP ST2,ST0
0043a6f3  FXCH
0043a6f5  FSTP float ptr [ESP + 0x14]
0043a6f9  FSTP ST0
0043a6fb  CALL dword ptr [EDX + 0x310]
0043a701  FLD float ptr [EAX]
0043a703  FADD float ptr [ESP + 0xc]
0043a707  MOV ECX,dword ptr [EAX + 0x4]
0043a70a  MOV EDX,dword ptr [EAX + 0x8]
0043a70d  FLD float ptr [ESP + 0x10]
0043a711  MOV dword ptr [ESP + 0x20],ECX
0043a715  MOV EAX,dword ptr [ESP + 0x40]
0043a719  FADD float ptr [ESP + 0x20]
0043a71d  FLD float ptr [ESP + 0x14]
0043a721  MOV dword ptr [ESP + 0x24],EDX
0043a725  FADD float ptr [ESP + 0x24]
0043a729  FSTP float ptr [ESP + 0x14]
0043a72d  FXCH
0043a72f  FSTP float ptr [EAX]
0043a731  MOV ECX,dword ptr [ESP + 0x14]
0043a735  POP EDI
0043a736  FSTP float ptr [EAX + 0x4]
0043a739  MOV dword ptr [EAX + 0x8],ECX
0043a73c  MOV dword ptr [EAX + 0xc],0x3f800000
0043a743  POP ESI
0043a744  ADD ESP,0x34
0043a747  RET 0x10
```

## IAT 0x0048d108
data: addr 000e946c
symbol: PTR_angles_0048d108
ref -> EXTERNAL:0000000b angles
