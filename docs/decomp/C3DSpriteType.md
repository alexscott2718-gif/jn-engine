# C3DSpriteType

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DSpriteType` |
| Base chain | `C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004b6b10`, `004b6b20`, `004b6f70`, `004b6f84` |
| Ctor(s) | inherited/base construction only; no class-owned constructor body identified by current dump |
| Dtor(s) | adjusted scalar deleting destructor at `00441a30`, cleanup helper at `00441a60` |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

`C3DSpriteType` is a thin typed-sprite base. It introduces no confirmed primary-pointer fields beyond the inherited `C3DSprite` sprite database/index/size state. The negative indexes in the decompiler are adjusted-base accesses into the `OMediaCanvasElement`/allocation layout, not standalone negative fields on the primary object.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x4b4` | int | `SpriteSize` | `C3DSprite`; consumed at `00441ac0` | Scale/size passed as float to the canvas initialization path. |
| inherited `0x4b8` | int | `SpriteIndex` | `C3DSprite`; consumed at `00441ac0` | Canvas index loaded from `SpriteDatabase`; fallback path uses index `3`. |
| inherited `0x4bc` | char buffer/string | `SpriteDatabase` | `C3DSprite`; consumed at `00441ac0` | OMT/database filename looked up with `FUN_0046a910`. |
| adjusted OMedia | uint16 | `sprite_type_mode` | write in `00441ac0` | Low word on an adjusted canvas/base subobject set to `3` after canvas load or fallback attempt. Exact primary offset needs full OMedia layout. |
| adjusted OMedia | pointer | `current_canvas` | `00441ac0` | If non-null after named-database load, dirty flags at canvas offsets `0x60` and `0x64` are set before an update callback. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 259 | `00441ac0` | `LoadSpriteTypeCanvas` | Resolves inherited `SpriteDatabase`; if present, initializes inherited sprite canvas with inherited `SpriteIndex`/`SpriteSize`, marks the current canvas dirty, and calls its update callback. If missing, falls back to `icons.omt` index `3`. Always writes adjusted `sprite_type_mode = 3` before returning. | non-trivial |
| vtable 2 slot 2 | `00441a30` | `ScalarDeletingDestructor` | Runs class cleanup helper `00441a60`, destroys the embedded/adjusted `OMediaClassStreamer` subobject, and frees the adjusted allocation when the delete flag is set. | non-trivial |

## Per-Frame Behavior

`C3DSpriteType` has no owned update integrator. It specializes the inherited sprite canvas binding so derived typed sprites enter the same canvas mode after either successful named-database load or fallback.

```c
C3DSpriteType::LoadSpriteTypeCanvas():
    db = lookup_omt_database(SpriteDatabase)
    if db:
        init_canvas(db, SpriteIndex, float(SpriteSize))
        if current_canvas:
            current_canvas->dirty_60 = 1
            current_canvas->dirty_64 = 1
            current_canvas->update()
    else:
        fallback = lookup_omt_database("icons.omt")
        if fallback:
            init_canvas(fallback, 3, float(SpriteSize))
    adjusted_sprite_type_mode = 3
```

## Constants And Wiring

`C3DSpriteType` has no direct FourCC mapping in `docs/gam_schema.md`; it is an abstract/shared base for many leaf sprite classes such as `C3DArrow`, `C3DBush`, `C3DCheckPoint`, `C3DNeutron`, `C3DPasscard`, `C3DRocketFuel`, and `C3DSoundEffect`. Leaf classes provide the actual `.gam` FourCC rows and any class-specific constants.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `SpriteSize` | int (`6`) | inherited `0x4b4` | populated by derived/leaf initialization or inherited `.gam` registration | Converted to float for canvas initialization. |
| `SpriteDatabase` | str (`1`) | inherited `0x4bc` | populated by derived/leaf initialization or inherited `.gam` registration | Resolved by `FUN_0046a910`; missing database falls back to `icons.omt`. |
| `SpriteIndex` | int (`6`) | inherited `0x4b8` | populated by derived/leaf initialization or inherited `.gam` registration | Canvas index for the named database. |
| `sprite_type_mode` | uint16 | adjusted OMedia | constant `3` | Written after load/fallback attempt; likely selects the canvas/sprite draw mode used by typed sprite leaves. |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | inherited `SpriteDatabase` | `00441ac0` | Primary sprite database for the leaf class. |
| OMT database | `icons.omt` | fallback string at `004ecddc` | Used if inherited `SpriteDatabase` lookup fails. |
| fallback canvas index | `3` | `00441ac0` | Same fallback index used by `C3DSprite::LoadSpriteCanvas`. |

## Confidence

Confidence: Medium

Validation: Static Ghidra only; not runtime-validated.

Open questions:
- Apply a complete `OMediaCanvasElement` struct so the adjusted `sprite_type_mode` and `current_canvas` offsets can be converted to primary-object offsets.
- Name `FUN_0046a910`, the inherited canvas init slot at vtable offset `0xc8`, and the current-canvas update callback.
- Confirm whether `sprite_type_mode = 3` is a sprite draw mode, canvas mode, or visibility/render-state selector in OMedia.

## Notes

- Evidence: `DumpClass.java C3DSpriteType /tmp/decomp_C3DSpriteType.md` (`slots=335`, `owned_methods=2`, `offsets=2`).
- The decompiler renders inherited sprite fields as `this[0x12d]`, `this[0x12e]`, and `this + 0x12f` because the active `this` pointer is adjusted. Their behavior matches the inherited `C3DSprite` `SpriteSize`, `SpriteIndex`, and `SpriteDatabase` fields.
- `docs/decomp/_hierarchy.md` lists `C3DSpriteType` as a shared parent for 24 leaf classes.
