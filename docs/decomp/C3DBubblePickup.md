# C3DBubblePickup

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DBubblePickup` |
| Base chain | `CPickupType -> C3DTriggerType -> C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `00492a6c`, `00492a7c`, `00492ecc`, `00492ee0` |
| Ctor(s) | constructor/factory block `00410d90`; registers FourCC `3BUP` at `00410e47` |
| Dtor(s) | scalar deleting destructor at `00410ef0`; cleanup helper `00410f20`; adjusted destructor thunks at `00410ff0`, `00411000`, `00411010`, `00411020` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DBubblePickup` is a small `3BUP` `CPickupType` pickup that grants or enables the bubble-related progress flag when Jimmy touches it. The current `.gam` corpus has no serialized `3BUP` rows; the constructor hard-wires `sprites.omt` chunk id `26` (`bubshadw`) at size `50`, and the leaf behavior is a Jimmy-only touch handler that sets picture/inventory flag `(0, 0)` and calls story/progress helper `00406f90(10)`.

## Field Map

Offsets are byte offsets from the active `C3DBubblePickup` / `CPickupType` pointer unless marked outer. The active pointer is at outer `+0xc8`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x4b4` / outer `0x57c` | int | `SpriteSize` | ctor `00410d90`; `CPickupType`/`C3DSprite` | Constructor writes `50`; consumed by inherited sprite canvas loading. |
| inherited `0x4b8` / outer `0x580` | int | `SpriteIndex` | ctor `00410d90`; `CPickupType`/`C3DSprite` | Constructor writes `26`; resolves to `sprites.omt` chunk id `26`, named `bubshadw`. |
| inherited `0x4bc` / outer `0x584` | char buffer/string | `SpriteDatabase` | ctor `00410d90`; `CPickupType`/`C3DSprite` | Constructor copies `"sprites.omt"`; resolved by the inherited sprite canvas path. |
| outer `0x6d0` | subobject/tail | `class_streamer_tail` | constructor/destructor scaffolding | Tail object destroyed by the scalar deleting destructor. |

No BubblePickup-owned state fields were found beyond the inherited sprite identity fields. Its pickup state is stored externally through the picture/inventory and story/progress helpers.

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `00410d90` | `CtorBubblePickup3BUP` | Constructs `CPickupType`, installs BubblePickup vtables, registers runtime string `C3DBUBBLEPICKUP`, runs inherited pickup/sprite init, registers FourCC `3BUP`, seeds `SpriteDatabase="sprites.omt"`, `SpriteIndex=26`, `SpriteSize=50`, applies inherited setup constants, and runs inherited sprite physics setup. | non-trivial |
| 7 | `0045f170` | `CPickupType::InitObjectPickupType` | Inherited pickup property registration. No BubblePickup-owned `.gam` fields were found. | inherited |
| 11 | `00464210` | `C3DSprite::InitPhysicsSprite` | Inherited sprite physics/transform setup. | inherited |
| 16 | `00410f60` | `HandleBubblePickupTouch` | Runs base touch handling, accepts only `C3DJIMMY`, hides/disables the pickup, sets picture/inventory flag `(0, 0) = 1`, calls an inherited state slot with `0`, and triggers story/progress helper id `10`. | raw block |
| 241 | `00463090` | `C3DPolygon::UpdateSpriteOrPolygon` | Inherited update wrapper; BubblePickup does not own a per-frame integrator. | inherited |
| 257 | `00464780` | `C3DSprite::LoadSpriteCanvasWithBaseHook` | Inherited sprite canvas load. | inherited |
| 259 | `0045f1f0` | `CPickupType::LoadOrFinalizePickupType` | Inherited pickup post-load/finalization path. | inherited |
| vtable 2 slot 2 | `00410ef0` | scalar deleting destructor | Adjusts from the secondary pointer, runs cleanup helper `00410f20`, destroys the tail subobject at outer `0x6d0`, and frees the adjusted allocation when requested. | non-trivial |
| helper | `00410f20` | `CleanupBubblePickup` | Reinstalls BubblePickup vtables during destruction and tail-jumps to inherited `CPickupType` cleanup at `0045f130`. | non-trivial |

## Runtime Behavior

```c
C3DBubblePickup::CtorBubblePickup3BUP():
    CPickupType::Ctor()
    install_bubble_pickup_vtables()
    register_runtime_string("C3DBUBBLEPICKUP")
    trace_constructor("C3DBubblePickup()")
    CPickupType::InitObjectPickupType()
    register_fourcc("3BUP")

    SpriteDatabase = "sprites.omt"
    SpriteIndex = 26
    SpriteSize = 50

    inherited_slot_42(0)
    inherited_slot_40(0)
    inherited_slot_52(1)
    inherited_slot_34(1)
    inherited_scalar_slot_68(150.0)
    inherited_slot_46(0)
    C3DSprite::InitPhysicsSprite()
    inherited_finalize_or_sync_slot()
```

```c
C3DBubblePickup::HandleBubblePickupTouch(other):
    base_touch_hook(other)

    if !other.is_a("C3DJIMMY"):
        return

    outer_visibility_or_hide_slot(1)
    picture_inventory_set(group=0, id=0, value=1)
    inherited_enable_or_state_slot(0)
    story_progress_helper(10)
```

Unlike `C3DBaseballPickup`, this handler does not first query the existing picture/inventory flag and does not play a local sound/effect in the inspected block. It relies on the global helper call after setting flag `(0, 0)`.

## Constants And Wiring

### `.gam` Placeable Properties

`3BUP` is present in `docs/_gam_classids.tsv`, but it has no current row in `docs/gam_schema.md`. The `.gam` `ObjectTag` sample value `"BUBBLEPICKUP"` belongs to generic `C3DPickupItem` (`3PIC`) rows and is not a serialized `3BUP` instance.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| none | - | - | - | No current `.gam` instances or serialized BubblePickup-owned fields. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3BUP` | Concrete BubblePickup class id. | ctor `00410d90`; `push 0x33425550` at `00410e47` |
| `C3DBUBBLEPICKUP`, `C3DBubblePickup()` | Runtime class strings. | strings `.data:004ed89c` and `.data:004ed888`; constructor `00410d90` |
| `C3DJIMMY` | Required toucher class. | touch slot `00410f60`; string `.data:004ecb20` |
| `sprites.omt` | Primary sprite database. | constructor string `.data:004ed488` |
| sprite chunk id `26` | Pickup sprite. | constructor `SpriteIndex=26`; parsed `sprites.json` |
| `50` | `SpriteSize` default. | constructor outer `0x57c` |
| `150.0` | Inherited scalar through slot 68. | constructor immediate `0x43160000` |
| picture/inventory `(0, 0)` | Reward/progress flag written on touch. | touch slot calls `FUN_004038c0(0, 0, 1)` |
| story/progress helper id `10` | Follow-up side effect after touch reward. | touch slot calls `FUN_00406f90(0x0a)` |
| `3MEP` | MetalPickup class id, not BubblePickup. | constructor `0042e8e0` installs `C3DMetalPickup` vtables and string `C3DMETALPICKUP`; corrected in `docs/_gam_classids.tsv` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `sprites.omt` | constructor `00410d90` | Loaded by inherited sprite canvas path. |
| sprite chunk | id `26` / `bubshadw` | constructor; parsed `assets/parsed/sprites/sprites.json` | Parsed at JSON index `31`; image `assets/parsed/sprites/sprites_images/0031_128x128d32.png`. Do not confuse this with JSON index `26`, which is chunk id `17` named `TreeTop3s`. |
| picture/inventory flag | `(group=0, id=0)` | touch slot `00410f60` | Same helper family documented in `C3DPickupItem`; exact UI label still unresolved. |
| story/progress helper | id `10` | touch slot `00410f60` | Same helper family seen in friend/reward specs. Exact task/menu effect remains unresolved. |

## Confidence

Confidence: Medium

Validation: Static Ghidra class dump, `DumpFunctions.java` constructor/cleanup output, local `objdump` over raw touch slot and the stale `3MEP` constructor, string table checks, `.gam` schema cross-check, and parsed `sprites.omt` metadata only; not runtime-validated.

Open questions:
- Identify the exact gameplay/UI label for picture/inventory flag `(0, 0)`.
- Identify the concrete task/menu effect of `FUN_00406f90(10)`.
- Name inherited slots 34, 40, 42, 46, 52, and 68 with base object/OMedia semantics.
- Runtime-check how `3BUP` is spawned or enabled, since no current `.gam` row serializes it.

## Notes

- Evidence: `DumpClass.java C3DBubblePickup /tmp/decomp_C3DBubblePickup.md` (`slots=336`, `owned_methods=1`, `offsets=1`), `DumpFunctions.java /tmp/decomp_C3DBubblePickup_raw.md`, local objdump windows `00410d90..00411030` and `0042e8e0..0042ea40`, string scans, `C3DPickupItem` helper-family notes, and parsed sprite metadata.
- The class-id scanner row for `3MEP` previously named `C3DBubblePickup()` because that constructor reuses the same trace string at `004ed888`. Its runtime class string and vtables identify it as `C3DMetalPickup`; the TSV row has been corrected.
