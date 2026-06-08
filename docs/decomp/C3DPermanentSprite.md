# C3DPermanentSprite

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DPermanentSprite` |
| Base chain | `C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004ada08`, `004ada18`, `004ade68`, `004ade7c` |
| Ctor(s) | inherited/base construction only; no class-owned constructor body identified by current dump |
| Dtor(s) | inherited/adjusted base destructors; no class-owned destructor body decompiled |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

`C3DPermanentSprite` introduces no confirmed primary-pointer fields. It reuses the inherited `C3DSprite` sprite database/index/size fields and changes only the database lookup/fallback path.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x4b4` | int | `SpriteSize` | `C3DSprite`; consumed at `00435430`/`004354c0` | Scale/size passed as float to the canvas initialization path. |
| inherited `0x4b8` | int | `SpriteIndex` | `C3DSprite`; consumed at `00435430`/`004354c0` | Canvas index loaded from `SpriteDatabase`; fallback path uses index `3`. |
| inherited `0x4bc` | char buffer/string | `SpriteDatabase` | `C3DSprite`; consumed at `00435430`/`004354c0` | Permanent-sprite OMT/database filename looked up with `FUN_0046acc0`. |
| adjusted OMedia | pointer | `current_canvas` | `00435430`, `004354c0` | If non-null after named-database load, canvas dirty flags at offsets `0x60` and `0x64` are set and the update callback is called. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 257 | `00435430` | `LoadPermanentSpriteCanvasWithBaseHook` | Runs `CLocalGameObject` slot 257 first, resolves inherited `SpriteDatabase` through `FUN_0046acc0`, initializes the canvas with inherited `SpriteIndex`/`SpriteSize`, marks the current canvas dirty, and falls back to `permanenticons.omt` index `3` if the named database is missing. | non-trivial |
| 259 | `004354c0` | `LoadPermanentSpriteCanvas` | Same permanent-sprite canvas loading logic as slot 257 but without the `CLocalGameObject` base hook. | non-trivial |

## Per-Frame Behavior

`C3DPermanentSprite` has no owned per-frame update integrator. It is an asset-binding specialization over `C3DSprite`:

```c
C3DPermanentSprite::LoadPermanentSpriteCanvas():
    db = lookup_permanent_sprite_database(SpriteDatabase)
    if db:
        init_canvas(db, SpriteIndex, float(SpriteSize))
        mark_current_canvas_dirty()
    else:
        fallback = lookup_permanent_sprite_database("permanenticons.omt")
        if fallback:
            init_canvas(fallback, 3, float(SpriteSize))
```

## Constants And Wiring

`C3DPermanentSprite` has no direct FourCC mapping in `docs/gam_schema.md`. It is a shared base for leaves including `C3DBaseball`, `C3DBubble`, `C3DShadow`, and `C3DTargetCursor`; those leaves provide their own `.gam` rows or runtime setup.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `SpriteSize` | int (`6`) | inherited `0x4b4` | leaf-defined | Converted to float for permanent-sprite canvas initialization. |
| `SpriteDatabase` | str (`1`) | inherited `0x4bc` | leaf-defined; `3TAR` rows use `"sprites.omt"` on the `C3DShadow` branch | Resolved by `FUN_0046acc0`; missing database falls back to `permanenticons.omt`. |
| `SpriteIndex` | int (`6`) | inherited `0x4b8` | leaf-defined; `3TAR` rows span `98..176` on the `C3DShadow` branch | Canvas index for the named database. |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | inherited `SpriteDatabase` | leaf `.gam` rows or leaf setup | Primary permanent-sprite database. |
| OMT database | `permanenticons.omt` | fallback string at `004f02c0` | Used when inherited `SpriteDatabase` lookup fails. |
| fallback canvas index | `3` | `00435430`, `004354c0` | Same fallback index pattern as other sprite bases, but using the permanent icon database. |

## Confidence

Confidence: Medium

Validation: Static Ghidra + local `.gam`/hierarchy cross-check only; not runtime-validated.

Open questions:
- Name `FUN_0046acc0` and determine how it differs from `C3DSprite`'s `FUN_0046a910` database lookup.
- Confirm the complete leaf data for `C3DBaseball`, `C3DBubble`, `C3DShadow`, and `C3DTargetCursor`.
- Apply full `OMediaCanvasElement` structs so adjusted `current_canvas` offsets become primary-pointer offsets.

## Notes

- Evidence: `DumpClass.java C3DPermanentSprite /tmp/decomp_C3DPermanentSprite.md` (`slots=335`, `owned_methods=2`, `offsets=1`).
- `docs/decomp/_hierarchy.md` lists `C3DPermanentSprite` as the parent of `C3DBaseball`, `C3DBubble`, `C3DShadow`, and `C3DTargetCursor`.
- This class is not the pickup trigger base; `CPickupType` owns pickup state, while `C3DPermanentSprite` only changes sprite database resolution.
