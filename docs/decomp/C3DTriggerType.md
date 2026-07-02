# C3DTriggerType

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DTriggerType` |
| Base chain | `C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004bd270`, `004bd280`, `004bd6d0`, `004bd6e4` |
| Ctor(s) | inherited/base construction only; no class-owned constructor body identified by current dump |
| Dtor(s) | inherited/adjusted base destructors; no class-owned destructor body decompiled |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

Offsets below are byte offsets from the primary `C3DTriggerType` pointer. Ghidra currently prints the owned methods with 4-byte seed-struct units (`this + 0x148`, `this + 0x161`, etc.); those are converted here to byte offsets.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x4b4` | int | `SpriteSize` | `C3DSprite` | Sprite/icon size for the editor-visible trigger marker. |
| inherited `0x4b8` | int | `SpriteIndex` | `C3DSprite` | Canvas index for the trigger marker. |
| inherited `0x4bc` | char buffer/string | `SpriteDatabase` | `C3DSprite` | OMT database for the trigger marker. |
| `0x520` | char buffer/string | `ToggleObject` | property registration at `00447c60` | Object tag/name to toggle when this trigger fires. |
| `0x584` | int | `Toggle` | property registration at `00447c60` | Toggle mode/state, commonly `-1`, `0`, or `1` in `.gam` rows. |
| `0x588` | char buffer/string | `NextTrigger` | property registration and runtime use at `00447a70` | Object tag/name of the next trigger or camera target to resolve after activation. |
| `0x5ec` | int | `FadeType` | property registration at `00447c60` | Fade mode; mostly `-1` in current trigger-derived rows. |
| `0x5f0` | float | `FadeTime` | property registration at `00447c60` | Fade duration; commonly `1.0` in current trigger-derived rows. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `00447c60` | `InitObjectTriggerType` | Runs `C3DSprite::InitObject`, then registers `Toggle`, `ToggleObject`, `NextTrigger`, `FadeType`, and `FadeTime`. | non-trivial |
| 242 | `00447a70` | `RunTriggerTypeNextTarget` | Runs the `CGameObject` slot-22 base hook, then — only while the global trigger-focus byte `DAT_0050985a` is set and the current game object's active-trigger pointer (`DAT_00509980 + 0xb4`) is this object — sets `DAT_004f8182`, resolves non-`"none"` `NextTrigger` via `FindObjectByTag_00474070`, and repoints the global camera/player-target record `DAT_00509a50` (`+0x44..0x4c`) to the target's world position plus the fixed camera-local offset `(20, -20, -100)` rotated through the record's three 14-bit angle shorts (`+0x50/0x52/0x54`). Full L1 in `docs/decomp/evidence/triggertype_trigger_target5.md`. | non-trivial |

## Per-Frame Behavior

`C3DTriggerType` does not own a normal per-frame integrator. Its owned runtime method is an activation/dispatch hook for the trigger system:

```c
C3DTriggerType::RunTriggerTypeNextTarget(param):          // 00447a70, ret 0x4
    CGameObject::vfunc_00_022(param)                      // base slot-22 hook
    if (DAT_0050985a == 0) return                         // trigger-focus gate byte
    if (DAT_00509980->active_trigger_0xb4 != this) return // current game object's active trigger
    DAT_004f8182 = 1                                      // one-frame action/transition flag
    if (stricmp(NextTrigger, "none" @004eca6c) == 0) return
    target = FindObjectByTag_00474070(NextTrigger)        // ObjectTag at +0x3a0, ring list DAT_0050999c
    if (!target) return

    rec = DAT_00509a50                                    // camera/player-target record
    A = idx(rec->s16_0x50); B = idx(rec->s16_0x52); C = idx(rec->s16_0x54)
    // idx(s) = ftol(s * 0.02197265625f * 45.511112f) & 0x3fff — the angle shorts
    // are 14-bit units (16384 == 360°); the constant pair (360/16384 @0048e5b4,
    // 16384/360 @0048e5e8) is numerically the identity, so they index the
    // global_exref trig table (sin +i*4, cos +i*4+0x10004) directly.

    X1  =  20.0*cos[C] + 20.0*sin[C]     //  (X1,Y1)  = Rz(C) · (20, -20)
    Y1  =  20.0*sin[C] - 20.0*cos[C]     //  20.0 @00495324, -20.0 @004bd7c4
    T   = -100.0*cos[A] - X1*sin[A]      //  (OUTy,T) = R(A)  · (X1, -100)
    OUTy =   X1*cos[A] - 100.0*sin[A]    //  -100.0 @0049fac4, 100.0 @0048d96c
    OUTx =  Y1*cos[B] - T*sin[B]         //  (OUTx,OUTz) = R(B) · (Y1, T)
    OUTz =  Y1*sin[B] + T*cos[B]

    pos = target->vfunc_0x310()          // world position
    rec->pos_0x44 = OUTx + pos.x
    rec->pos_0x48 = OUTy + pos.y
    rec->pos_0x4c = OUTz + pos.z
```

Recovered to full L1 (target 5): the fixed camera-local offset `(20, -20, -100)`
is rotated through the record's current three 14-bit angles and added to the
resolved target's world position — the record keeps its orientation and swings
its position to a fixed offset from the `NextTrigger` object. The earlier
"still raw decompiler output" reading (a single `fVar1` component added to all
three stores) was an x87-stack decompiler artifact across the three `__ftol`
calls; the disassembly in
`docs/decomp/evidence/triggertype_trigger_target5.md` is authoritative. The
record is the same `DAT_00509a50` camera/player-target record already seen in
`C3DObject`, `C3DPlayer`, `C3DCar`, and `C3DFlyingObject`.

## Constants And Wiring

`C3DTriggerType` has no direct FourCC mapping in `docs/gam_schema.md`; it is the shared base for trigger-like leaves including `C3DAITrigger`, `C3DCutSceneCamera`, `C3DMultiCutSceneCamera`, `C3DMusicTrigger`, and `CPickupType` descendants. The base registers the five common trigger fields below. `C3DTrigger`/`3TRI` is a separate `C3DSprite` descendant and should be documented independently.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `Toggle` | int (`6`) | `0x584` | trigger-derived rows: `3PIC` 376, `3AIT` 174, `3CAM` 136, `3MCA` 113, `3RED` 70, `3MUS` 8, `3ANI` 6; values `-1..1` | Registered here; specific toggle behavior is likely in the broader trigger dispatcher or derived classes. |
| `ToggleObject` | str (`1`) | `0x520` | examples: `"applepie"`, `"LITE1"`, `"beam"`, `"NONE"`, `"none"` | Registered here; likely names the object acted on by `Toggle`. |
| `NextTrigger` | str (`1`) | `0x588` | examples: `"2space1"`, `"AI2"`, `"DEFAULT"`, `"GODDARDDIS"`, `"none"` | Resolved by `FindObjectByTag_00474070` (case-insensitive `ObjectTag` match at `+0x3a0` over the `DAT_0050999c` ring list) in slot 242 when not `"none"`. |
| `FadeType` | int (`6`) | `0x5ec` | mostly `-1`; `LOAD` rows outside this base also use `-1..2` | Registered here; no class-owned consumer beyond property exposure. |
| `FadeTime` | float (`3`) | `0x5f0` | commonly `1.0` in trigger-derived rows | Registered here; no class-owned consumer beyond property exposure. |

## Assets

The class does not name a new asset directly. Trigger markers reuse inherited `C3DSprite` canvas fields, normally icon databases provided by derived `.gam` rows.

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `icons.omt`, `permanenticons.omt`, or `sprites.omt` | derived `.gam` rows | Inherited `SpriteDatabase` values for visible trigger markers. |
| canvas index | derived `SpriteIndex` | derived `.gam` rows | Examples include `3AIT` index `9`, `3CAM` index `10`, `3MCA` index `7`, and `3PIC` pickup icons. |
| target object | `NextTrigger` object tag | `.gam` wiring; `FindObjectByTag_00474070` | Resolved at runtime to drive the global camera/player-target record. |

## Confidence

Confidence: Medium

Validation: Static Ghidra + `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Resolved (target 5): `DAT_0050985a` is the trigger-focus gate byte (no static
  writer xref; also read at `00404cb3`), `DAT_00509980 + 0xb4` is the current
  game object's active-trigger pointer, `DAT_00509a50` is the global
  camera/player-target record, and `FUN_00474070` is now
  `FindObjectByTag_00474070`. See
  `docs/decomp/evidence/triggertype_trigger_target5.md`.
- Confirm whether `Toggle`, `ToggleObject`, `FadeType`, and `FadeTime` are consumed by a shared dispatcher outside this class or by each derived trigger.
- Replace 4-byte seed-struct offset conversion with real `C3DTriggerType` and string-buffer structs in Ghidra.

## Notes

- Evidence: `DumpClass.java C3DTriggerType /tmp/decomp_C3DTriggerType.md` (`slots=336`, `owned_methods=2`, `offsets=5`).
- `.gam` evidence: `docs/gam_schema.md` rows for `3AIT`, `3CAM`, `3MCA`, `3MUS`, `3ANI`, `3RED`, and `3PIC` show the shared trigger property group populated by descendants.
- Slot 242 compares `NextTrigger` against `DAT_004eca6c`, confirmed as the literal string `"none"` (`.data` at `4eca6c`).
- Target 5 evidence (full disassembly, constants, globals): `docs/decomp/evidence/triggertype_trigger_target5.md`.

## Record-camera demo port (2026-07-02)

The global camera/player-target record mechanism this class writes into is now
ported natively as a gated demo module: `src/game/camera_record.c/h`. The
module keeps the original representation (native-space position + the three
14-bit angle shorts) and ports, from the recovered L1s:

- the slot-242 retarget write (`camera_record_retarget`): fixed camera-local
  offset `(20,-20,-100)` rotated through the record's current angles, added to
  the target world position (native Z mirror on output);
- the generic record transforms `CameraRecordLocalToWorld_00476e10` /
  `..Dir_00476f10`;
- the `CGameType::InitGame` seed and the record → view bridge derived from the
  recovered `FrameStepAndRender` view build (`yaw = -angle_y`,
  `pitch = angle_x`); see `docs/decomp/evidence/camera_record_layout.md`.

Wiring: the V key (or `JN_CAMREC=follow|hold`) cycles OFF → FOLLOW → HOLD;
`behavior_ai_trigger.c` invokes the retarget on NextTrigger dispatch while the
demo mode is on. Two original gates are *not* ported: the trigger-focus byte
`DAT_0050985a` (no recovered writer) and the active-trigger pointer
(`DAT_00509980+0xb4`); the demo gates on its mode instead, and the FOLLOW
mode is explicit scaffolding pending an `UpdateWalkingCameraA/B` port.

Certified 2026-07-02: the retarget aspect row is now **linked** in
docs/linkage_certificates.csv (oracle tools/linkage_oracles/C3DTriggerType.py,
516 synthetic cases, mutation tested via the native Z mirror).
