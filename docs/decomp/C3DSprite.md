# C3DSprite

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DSprite` |
| Base chain | `OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `004d4768`, `004d4778`, `004d4bc8`, `004d4bdc` |
| Ctor(s) | constructor/body at `00464070` from disassembly |
| Dtor(s) | adjusted scalar deleting destructor at `00464040` |
| Ledger row | `docs/decomp_ledger.csv` |

## Field Map

Offsets below are byte offsets from the primary `C3DSprite` pointer. The class uses an adjusted `OMediaCanvasElement` subobject for canvas setters; negative indexes in the dump are inherited/adjusted-base accesses, not new negative fields.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| `0x4b4` | int | `SpriteSize` | `.gam` registration at `00464130`; canvas load at `00464780`/`00464810` | Scale/size argument passed as float to the canvas initialization path. |
| `0x4b8` | int | `SpriteIndex` | `.gam` registration; canvas load | Canvas index loaded from `SpriteDatabase`; fallback path uses index `3`. |
| `0x4bc` | char buffer/string | `SpriteDatabase` | `.gam` registration; `FUN_0046a910` lookup | OMT/database filename looked up before loading the canvas. |
| adjusted OMedia | pointer | `current_canvas` | `00464780`, `00464810` | OMedia canvas pointer marked dirty at offsets `0x60` and `0x64` after load. Exact absolute offset needs full OMediaCanvasElement struct. |
| inherited | float | `PositionX/Y/Z` | `00464210`, raw `004646c0` | Initial physics pushes inherited position to OMedia; raw slot 247 pulls OMedia position back. |
| inherited | angle state | `Rotation*` bridge | `00464210`, raw `00464700` | Initial physics converts inherited 14-bit angles to degrees; raw slot 248 pulls OMedia angles back to wrapped 14-bit fields. |

## Vtable Methods

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| 7 | `00464130` | `InitObjectSprite` | Traces `"InitObject()"`, runs `CLocalGameObject::InitObject`, attaches global OMedia objects, then registers `SpriteSize`, `SpriteDatabase`, and `SpriteIndex`. | non-trivial |
| 8 | `004641d0` | `UnInitObjectSprite` | Traces `"UnInitObject()"`, runs `CLocalGameObject::UnInitObject`, then detaches/clears the adjusted OMedia canvas pointer. | non-trivial |
| 11 | `00464210` | `InitPhysicsSprite` | Runs `CLocalGameObject::InitPhysics`, pushes inherited position through slot `0x16c`, pushes inherited angle state through slot `0x174`, then invalidates/updates the OMedia element. | non-trivial |
| 247 | `004646c0` | `PullWorldPositionToSprite` | Raw vtable target not defined as a Ghidra function. Disassembly reads OMedia world position through slot `0x310` and copies X/Y/Z back into inherited position fields. | non-trivial |
| 248 | `00464700` | `PullWorldAnglesToSprite` | Raw vtable target not defined as a Ghidra function. Disassembly calls the base angle helper and stores OMedia orientation components as wrapped 14-bit angle fields. | non-trivial |
| 257 | `00464780` | `LoadSpriteCanvasWithBaseHook` | Runs `CLocalGameObject` base hook, resolves `SpriteDatabase` via `FUN_0046a910`, calls adjusted canvas init with database/index/size, marks the current canvas dirty, and falls back to `icons.omt` index `3` if the named database is missing. | non-trivial |
| 259 | `00464810` | `LoadSpriteCanvas` | Same canvas-loading logic as slot 257, but without the `CLocalGameObject` base hook. | non-trivial |
| vtable 3 slot 49 | `00464490` | `InitCanvasByIdOrName` | Helper over OMedia canvas setters; resolves an id/name through `FUN_00477780`, calls canvas setter slots `0xac` and `0xd0`, and manages refcounted string storage. | TODO |
| vtable 3 slot 51 | `004642a0` | `InitSpriteCanvas` | Logs `InitSprite`, resolves a canvas through `FUN_00477630`, calls adjusted canvas setter slots, then initializes sprite properties. | non-trivial |
| vtable 3 slot 52 | `004645f0` | `InitSpriteProperties` | Sets OMedia canvas render/property fields from size: dimensions, render modes `6/7`, texture/filtering modes `3/3`, and optional software-renderer color constants when `DAT_00509a13` is set. | non-trivial |

## Per-Frame Behavior

`C3DSprite` itself has no class-owned update integrator. Runtime behavior is mostly initialization and asset binding:

```c
C3DSprite::InitObjectSprite():
    CLocalGameObject::InitObject()
    attach_global_omedia_objects(DAT_00509a4c, DAT_00509a30)
    RegisterProperty("SpriteSize", &SpriteSize, type=6)
    RegisterProperty("SpriteDatabase", &SpriteDatabase, type=1)
    RegisterProperty("SpriteIndex", &SpriteIndex, type=6)

C3DSprite::LoadSpriteCanvas():
    db = lookup_omt_database(SpriteDatabase)
    if db:
        init_canvas(db, SpriteIndex, float(SpriteSize))
        mark_current_canvas_dirty()
    else:
        fallback = lookup_omt_database("icons.omt")
        if fallback:
            init_canvas(fallback, 3, float(SpriteSize))
```

Physics setup mirrors `C3DObject`'s transform bridge, but through `OMediaCanvasElement` instead of `OMedia3DShapeElement`.

## Constants And Wiring

`C3DSprite` is mapped by class-id scan to `3SPR` (`FUN_00463f10`). The generated schema also points `3NEU` at `FUN_004329a0`, but `C3DNeutron` vtable evidence shows that function constructs the concrete `C3DNeutron` placeable; `C3DSprite` remains the inherited consumer of the serialized `SpriteSize`, `SpriteDatabase`, and `SpriteIndex` fields. `3SPR` instances only carry common object/local fields and rely on defaults or later class setup for canvas binding.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| `SpriteSize` | int (`6`) | `0x4b4` | `3NEU`: 294 values, `100..1300`; common `100` | Converted to float and passed to canvas init. |
| `SpriteDatabase` | str (`1`) | `0x4bc` | `3NEU`: 294 values, all `"icons.omt"` | Resolved by `FUN_0046a910`; missing database falls back to `"icons.omt"`. |
| `SpriteIndex` | int (`6`) | `0x4b8` | `3NEU`: 294 values, all `4` | Canvas index for the named database. |
| `SpriteSize` / `SpriteDatabase` / `SpriteIndex` | - | same | `3SPR`: 0 serialized values | Registered by class, absent from current `.gam` rows. |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `icons.omt` | `.gam` `3NEU` rows; fallback string at `004ecddc` | Primary/fallback sprite database. |
| canvas index | `4` | `.gam` `3NEU` rows | Neutron sprite index for `3NEU`. |
| fallback canvas index | `3` | `00464780`, `00464810` | Used when `SpriteDatabase` lookup fails and `icons.omt` exists. |

## Confidence

Confidence: Medium

Validation: Static Ghidra + local disassembly only; not runtime-validated.

Open questions:
- Create/label raw Ghidra functions `004646c0`, `00464700`, `00464040`, `00464120`, and `00464430`.
- Name `FUN_0046a910`, `FUN_00477630`, `FUN_00477780`, and the adjusted canvas setter slots.
- Apply full `OMediaCanvasElement` and `CGameObject` structs so inherited transform/canvas offsets are exact.
- ~~Determine default `SpriteSize`/database/index values used by `3SPR` instances.~~
  **Database and index: answered below. `SpriteSize` is still open** -- it comes
  from the constructor at `00464070`, which is not recovered.

## What a bare `3SPR` binds (2026-08-21)

This page already contains the answer to two thirds of its own open question,
and to the same two thirds of `GTR-20260717-3spr-defaults` ("What default canvas
binding and sprite size/database/index values do bare `3SPR` instances use at
runtime?").

All **15** shipped `3SPR` rows are bare: not one authors `SpriteDatabase`,
`SpriteIndex` or `SpriteSize` (Level1 x1, level1b x1, level1c x4, level2a x9;
every one tagged `C3DSPRITE`). So `LoadSpriteCanvas` (`00464810`, and `00464780`
with the base hook) necessarily takes its **else** arm -- the `SpriteDatabase`
lookup through `FUN_0046a910` cannot succeed on a value no level sets -- and
binds:

> `icons.omt`, canvas index **3**

which `src/game/sprite_chunk_map_generated.h` names as the canvas the artist
called **`question`** -- the editor's question-mark placeholder. Its neighbours
in that database are `triger`, `dispatch`, `load`, `sprite`, `start`, `camera`,
`AITRIG`: this is the authoring toolkit's icon set, and index 3 is the one that
means "nothing bound here".

That reads as a complete answer rather than a coincidence: a bare `3SPR` is an
object a designer placed and never finished, and the engine drew a question mark
over it. Note that `3NEU` -- the concrete class that inherits these same three
fields -- authors `icons.omt` index **4** (`sprite`) on all 294 of its rows, so
the neighbouring index is in real use by real data.

**Still open, and it does need the executable:** the `SpriteSize` the fallback
passes as its third argument. It is whatever the constructor at `00464070` left
in `0x4b4`, and that body is not recovered. `3NEU`'s authored sizes run 100..1300
with 100 the mode, which bounds a guess but does not make one.

### Why native still draws nothing for these

`entity_visual.c` resolves `3SPR` to an invisible row and `entities.c` binds it
to `vt_resolver_inert`. Making the fallback fire would put 15 question-mark
icons into four shipped levels -- and one of them is in `Level1`, so it would
move the `level1` golden. Faithful, probably; but "the original drew editor
placeholders in the retail game" is a claim worth an owner's confirmation before
a golden moves for it, and the size the icon would be drawn at is exactly the
part still unrecovered. Recorded here rather than acted on.

## Notes

- Evidence: `DumpClass.java C3DSprite /tmp/decomp_C3DSprite.md` (`slots=335`, `owned_methods=8`, `offsets=4`).
- Extra check: `DumpFunctions.java /tmp/decomp_C3DSprite_extra.md 004646c0 00464700 00464040 00464120 00464430` reports no functions in Ghidra, but `objdump` over `/home/scotty/xp-jnbg-original/Neutron.exe` shows normal bodies for the transform helpers/destructor thunks.
- `C3DNeutron.md` corrects the concrete ownership of `3NEU`; keep the inherited `3NEU` sprite-property ranges here because the base class still registers and consumes those fields.
- String evidence includes `C3DSprite()`, `~C3DSprite()`, `SpriteSize`, `SpriteDatabase`, `SpriteIndex`, and `LoadCanvas*`.
