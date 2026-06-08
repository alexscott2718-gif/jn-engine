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
| 242 | `00447a70` | `RunTriggerTypeNextTarget` | Runs the `CGameObject` slot-22 no-op/base hook. If global trigger state is active and this object is the active trigger pointer, sets a global trigger flag, resolves non-empty `NextTrigger` with `FUN_00474070`, and writes a derived target-relative position into global record `DAT_00509a50` offsets `0x44..0x4c`. | non-trivial |

## Per-Frame Behavior

`C3DTriggerType` does not own a normal per-frame integrator. Its owned runtime method is an activation/dispatch hook for the trigger system:

```c
C3DTriggerType::RunTriggerTypeNextTarget():
    CGameObject::Slot22BaseHook()
    if global_trigger_enabled and global_active_trigger == this:
        DAT_004f8182 = 1
        if stricmp(NextTrigger, "none") != 0:
            target = lookup_object_by_tag(NextTrigger)
            if target:
                yaw = DAT_00509a50->angle_z_or_heading
                offset = build_forward_offset_from_trig_table(yaw)
                target_pos = target->GetWorldPosition()
                DAT_00509a50->pos_x = target_pos.x + offset.x
                DAT_00509a50->pos_y = target_pos.y + offset.y
                DAT_00509a50->pos_z = target_pos.z + offset.z
```

The trigonometric math is still raw decompiler output. It uses the global 14-bit angle table (`global_exref`) and a fixed-scale offset with visible constants `20.0` and `-100.0`; the resulting position record is the same `DAT_00509a50` camera/player-target record already seen in `C3DObject` and `C3DFlyingObject`.

## Constants And Wiring

`C3DTriggerType` has no direct FourCC mapping in `docs/gam_schema.md`; it is the shared base for trigger-like leaves including `C3DAITrigger`, `C3DCutSceneCamera`, `C3DMultiCutSceneCamera`, `C3DMusicTrigger`, and `CPickupType` descendants. The base registers the five common trigger fields below. `C3DTrigger`/`3TRI` is a separate `C3DSprite` descendant and should be documented independently.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `Toggle` | int (`6`) | `0x584` | trigger-derived rows: `3PIC` 376, `3AIT` 174, `3CAM` 136, `3MCA` 113, `3RED` 70, `3MUS` 8, `3ANI` 6; values `-1..1` | Registered here; specific toggle behavior is likely in the broader trigger dispatcher or derived classes. |
| `ToggleObject` | str (`1`) | `0x520` | examples: `"applepie"`, `"LITE1"`, `"beam"`, `"NONE"`, `"none"` | Registered here; likely names the object acted on by `Toggle`. |
| `NextTrigger` | str (`1`) | `0x588` | examples: `"2space1"`, `"AI2"`, `"DEFAULT"`, `"GODDARDDIS"`, `"none"` | Resolved by `FUN_00474070` in slot 242 when non-empty/non-`none`. |
| `FadeType` | int (`6`) | `0x5ec` | mostly `-1`; `LOAD` rows outside this base also use `-1..2` | Registered here; no class-owned consumer beyond property exposure. |
| `FadeTime` | float (`3`) | `0x5f0` | commonly `1.0` in trigger-derived rows | Registered here; no class-owned consumer beyond property exposure. |

## Assets

The class does not name a new asset directly. Trigger markers reuse inherited `C3DSprite` canvas fields, normally icon databases provided by derived `.gam` rows.

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `icons.omt`, `permanenticons.omt`, or `sprites.omt` | derived `.gam` rows | Inherited `SpriteDatabase` values for visible trigger markers. |
| canvas index | derived `SpriteIndex` | derived `.gam` rows | Examples include `3AIT` index `9`, `3CAM` index `10`, `3MCA` index `7`, and `3PIC` pickup icons. |
| target object | `NextTrigger` object tag | `.gam` wiring; `FUN_00474070` | Resolved at runtime to drive the global target/camera record. |

## Confidence

Confidence: Medium

Validation: Static Ghidra + `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Name `DAT_0050985a`, `DAT_00509980 + 0xb4`, `DAT_004f8182`, and `DAT_00509a50` once the trigger/camera dispatcher is mapped.
- Confirm whether `Toggle`, `ToggleObject`, `FadeType`, and `FadeTime` are consumed by a shared dispatcher outside this class or by each derived trigger.
- Label `FUN_00474070`; current evidence says it resolves an object by tag/name.
- Replace 4-byte seed-struct offset conversion with real `C3DTriggerType` and string-buffer structs in Ghidra.

## Notes

- Evidence: `DumpClass.java C3DTriggerType /tmp/decomp_C3DTriggerType.md` (`slots=336`, `owned_methods=2`, `offsets=5`).
- `.gam` evidence: `docs/gam_schema.md` rows for `3AIT`, `3CAM`, `3MCA`, `3MUS`, `3ANI`, `3RED`, and `3PIC` show the shared trigger property group populated by descendants.
- Slot 242 compares `NextTrigger` against `DAT_004eca6c`; by schema samples and existing docs this is treated as the empty/`none` sentinel until the string is named directly in Ghidra.
