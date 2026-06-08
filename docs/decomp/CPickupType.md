# CPickupType

## Identity

| Item | Value |
|---|---|
| RTTI name | `CPickupType` |
| Base chain | `C3DTriggerType -> C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004d1724`, `004d1734`, `004d1b84`, `004d1b98` |
| Ctor(s) | inherited/base construction only; no class-owned constructor body identified by current dump |
| Dtor(s) | adjusted scalar deleting destructor at `0045f100`, cleanup helper at `0045f130` |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

Offsets below are byte offsets from the primary `CPickupType` pointer. Ghidra currently prints the owned methods with 4-byte seed-struct units (`this + 0x17d`, etc.); those are converted here to byte offsets.

Do not confuse this class with `C3DPickupType`, which is a separate animated/AI pickup base under `C3DAI`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x520` | char buffer/string | `ToggleObject` | `C3DTriggerType` | Trigger object to toggle on pickup activation. |
| inherited `0x584` | int | `Toggle` | `C3DTriggerType` | Trigger toggle mode. |
| inherited `0x588` | char buffer/string | `NextTrigger` | `C3DTriggerType` | Follow-up trigger/camera tag. |
| inherited `0x5ec` | int | `FadeType` | `C3DTriggerType` | Trigger fade mode. |
| inherited `0x5f0` | float | `FadeTime` | `C3DTriggerType` | Trigger fade duration. |
| `0x5f4` | int | `PickupIndex` | property registration and state-table check | Index into global pickup state table `DAT_004f8438`; also serialized in pickup rows. |
| `0x5f8` | int | `PIC_NUMBER` | property registration at `0045f170` | Picture/inventory collection number; often `-1` for non-picture pickups. |
| `0x5fc` | int | `RequiredLevel` | property registration at `0045f170` | Minimum level/progress gate for the pickup. |
| `0x600` | int | `ExactLevel` | property registration at `0045f170` | Exact level/progress gate; `-1` means no exact gate in most rows. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `0045f170` | `InitObjectPickupType` | Runs `C3DTriggerType::InitObject`, then registers `PickupIndex`, `PIC_NUMBER`, `RequiredLevel`, and `ExactLevel`. | non-trivial |
| 259 | `0045f1f0` | `LoadPickupSpriteAndState` | Runs `C3DSprite::LoadSpriteCanvas`, queries a global pickup/index service through `DAT_00509948` slot `0x154`, stores the resulting pickup index/state id at `PickupIndex`, then checks `DAT_004f8438[PickupIndex]` to set visibility/collision-style state through inherited slots. | non-trivial |
| vtable 2 slot 2 | `0045f100` | `ScalarDeletingDestructor` | Runs cleanup helper `0045f130`, destroys the adjusted `OMediaClassStreamer` subobject, and frees the adjusted allocation when the delete flag is set. | non-trivial |

## Per-Frame Behavior

`CPickupType` does not own a normal per-frame integrator. Its owned logic runs during object/property initialization and sprite/state binding:

```c
CPickupType::InitObjectPickupType():
    C3DTriggerType::InitObjectTriggerType()
    RegisterProperty("PickupIndex", &PickupIndex, type=6)
    RegisterProperty("PIC_NUMBER", &PIC_NUMBER, type=6)
    RegisterProperty("RequiredLevel", &RequiredLevel, type=6)
    RegisterProperty("ExactLevel", &ExactLevel, type=6)

CPickupType::LoadPickupSpriteAndState():
    C3DSprite::LoadSpriteCanvas()
    PickupIndex = global_pickup_service->resolve_index_or_state()
    if DAT_004f8438[PickupIndex] == 0:
        show_or_enable_uncollected_pickup()
        set_inherited_state_flag(1)
    else:
        hide_or_disable_collected_pickup()
        set_inherited_state_flag(0)
```

The decompiler still has adjusted-base noise around the visibility/collision calls. The stable fact is the branch on `DAT_004f8438[PickupIndex]`; `C3DAnimated` also checks that same table through a resolved pickup object, which makes this field a global pickup-collection/state index rather than a cosmetic icon id.

## Constants And Wiring

`CPickupType` has no direct FourCC mapping in `docs/gam_schema.md`; its descendants carry the placeable rows. The primary data-bearing descendants in the current corpus are `C3DPickupItem` (`3PIC`), `C3DRedNeutron` (`3RED`), and `C3DAnimatedSprite` (`3ANI`). `C3DBubblePickup` is also a `CPickupType` descendant, but current schema rows with related FourCCs do not expose this four-field group in the same way.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `PickupIndex` | int (`6`) | `0x5f4` | `3PIC`: 383 values `203..3812`; `3RED`: 70 values `201..2902`; `3ANI`: 6 values `306..1109` | Used as index into `DAT_004f8438` pickup state table during sprite/state binding. |
| `PIC_NUMBER` | int (`6`) | `0x5f8` | `3PIC`: `-1..72`; `3RED`/`3ANI`: all `-1` | Picture/inventory collection number for picture-like pickups. |
| `RequiredLevel` | int (`6`) | `0x5fc` | `3PIC`: `-1..470`; `3RED`: `0..200`; `3ANI`: `0` | Progress gate registered here; consumer not in class-owned code. |
| `ExactLevel` | int (`6`) | `0x600` | `3PIC`: `-1..170`; `3RED`: `-1`; `3ANI`: `-1..30` | Exact progress gate registered here; consumer not in class-owned code. |

## Assets

The class itself does not name an asset, but its descendants use inherited `C3DSprite` marker/icon fields.

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `sprites.omt` | `3PIC`, `3ANI` rows | Main pickup icon/sprite database for pickup items and animated pickup sprites. |
| OMT database | `icons.omt` | `3RED` rows | Red Neutron pickup marker database. |
| sprite index | derived `SpriteIndex` | `.gam` rows | `3PIC` spans `8..189`; `3RED` uses `4`; `3ANI` spans `-1..177`. |
| pickup state table | `DAT_004f8438` | `0045f1f0`, also checked by `C3DAnimated` | Tracks collected/available pickup state by `PickupIndex`. |

## Confidence

Confidence: Medium

Validation: Static Ghidra + `.gam` schema cross-check only; not runtime-validated.

Open questions:
- Name the global pickup service at `DAT_00509948` slot `0x154` and confirm whether it returns a remapped index or validates serialized `PickupIndex`.
- Name the inherited slots used after the pickup-state check; current evidence suggests visibility/collision or render-enable toggles.
- Confirm the exact semantics of `RequiredLevel` and `ExactLevel` in the level/progress gate system.
- Apply real `CPickupType` structs in Ghidra so the decompiler stops printing field accesses as `this[0x17d]`.

## Notes

- Evidence: `DumpClass.java CPickupType /tmp/decomp_CPickupType.md` (`slots=336`, `owned_methods=3`, `offsets=5`).
- `.gam` evidence: `docs/gam_schema.md` rows for `3PIC`, `3RED`, and `3ANI` carry the registered pickup field group.
- `C3DAnimated::vfunc_01_241` checks `DAT_004f8438[piVar3[0x17d]]` after resolving `PickupLink`, supporting the `PickupIndex` state-table interpretation.
