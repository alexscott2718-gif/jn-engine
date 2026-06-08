# C3DBaseball

## Identity

| Item | Value |
|---|---|
| RTTI name | `C3DBaseball` |
| Base chain | `C3DPermanentSprite -> C3DSprite -> OMediaCanvasElement -> OMediaElement -> OMediaWorldPosition -> OMediaWorldAngle -> OMediaElementContainer -> OMediaDBObject -> OMediaClassStreamer -> OMediaListener -> OMediaMessagePort -> CLocalGameObject -> CGameObject` |
| Vftable(s) | `0049140c`, `0049141c`, `0049186c`, `00491880` |
| Ctor(s) | constructor/factory block `0040fc50`; registers FourCC `3BAS` at `0040fd0d` |
| Dtor(s) | scalar deleting destructor at `0040fe70`; cleanup helper `0040fea0`; adjusted destructor thunks at `00410050`, `00410060`, `00410070`, `00410080` |
| Ledger row | `docs/decomp_ledger.csv` |

`C3DBaseball` is the runtime `3BAS` retained-sprite baseball projectile used by other gameplay objects such as `C3DBalloon`. It is not placed by the current `.gam` corpus; the constructor hard-wires `retainedsprites.omt`, preloads the four `ball0000..ball0003` frames, then relies on inherited sprite and transform slots for movement/update behavior.

## Field Map

Offsets are byte offsets from the active `C3DBaseball` / `C3DPermanentSprite` pointer unless marked outer. The active pointer is at outer `+0xc8`.

| Offset | Type | Name | Source | Meaning |
|---:|---|---|---|---|
| inherited `0x4b4` / outer `0x57c` | int | `SpriteSize` | ctor `0040fc50`; `C3DPermanentSprite` | Constructor writes `25`; passed as the size/scalar for permanent-sprite canvas setup. |
| inherited `0x4b8` / outer `0x580` | int | `SpriteIndex` | ctor `0040fc50`; `C3DPermanentSprite` | Constructor writes `2`; base permanent-sprite load uses this as the primary retained-sprite chunk id. |
| inherited `0x4bc` / outer `0x584` | char buffer/string | `SpriteDatabase` | ctor `0040fc50`; `C3DPermanentSprite` | Constructor copies `"retainedsprites.omt"`; resolved through `FUN_0046acc0`. |
| active `0x52c` / outer `0x5f4` | pointer/int | `baseball_runtime_link` | ctor clears | Cleared before frame preload. No Baseball-owned consumer found yet. |
| active `0x530..0x53c` / outer `0x5f8..0x604` | pointer[4] | `baseball_frame_canvases` | ctor `0040fc50` | Canvas pointers for retained-sprite chunk ids `2`, `3`, `4`, and `5`; each canvas is marked dirty immediately after load. |
| active `0x540` / outer `0x608` | float | `baseball_scale_or_weight` | ctor `0040fc50` | Constructor default `1.0`; exact inherited consumer unresolved. |
| active `0x544` / outer `0x60c` | int/float | `baseball_timer_or_state` | ctor `0040fc50` | Constructor clears to zero; no Baseball-owned branch reads it in confirmed methods. |
| active `0x548` / outer `0x610` | uint16 | `baseball_frame_state` | ctor `0040fc50` | Constructor clears to zero; likely retained-sprite frame/state bookkeeping. |
| outer `0x618` | subobject/tail | `class_streamer_tail` | constructor/destructor scaffolding | Tail object constructed before base init and destroyed by the scalar deleting destructor. |

## Vtable Methods

`DumpClass` only decompiled slot 259 as class-owned, but direct vtable and objdump evidence confirms Baseball-owned raw targets at slots 220, 221, 223, and 241.

| Slot | Address | Name | Behavior | Status |
|---:|---|---|---|---|
| factory | `0040fc50` | `CtorBaseball3BAS` | Constructs `C3DPermanentSprite`, installs Baseball vtables, registers runtime string `C3DBASEBALL`, runs inherited sprite init, registers FourCC `3BAS`, seeds retained-sprite database/index/size defaults, preloads frame chunks `2..5`, clears small runtime state, and applies inherited movement/collision setup constants. | non-trivial |
| 7 | `00464130` | `C3DSprite::InitObjectSprite` | Inherited sprite property registration for `SpriteSize`, `SpriteDatabase`, and `SpriteIndex`. | inherited |
| 11 | `00464210` | `C3DSprite::InitPhysicsSprite` | Inherited sprite physics/transform setup; called again by Baseball post-load slot 259 through vtable offset `0x2c`. | inherited |
| 220 | `0040ff50` | `ForwardBaseballVector` | Wraps an incoming vector/argument block through helper `00409f60`, then forwards it to inherited transform slot `00472690`. | raw block |
| 221 | `0040fee0` | `ForwardBaseballCurrentVector` | Reads the current transform vector through vtable offset `0x2bc`, copies it into a temporary vector block, wraps caller arguments, and forwards through `CLocalGameObject` helper `00458890`. | raw block |
| 223 | `0040ff80` | `ApplyBaseballScaledVector` | Reads the current transform vector, runs inherited slot `00472970`, builds `(x, y * -0.65, z, 0.0)`, and passes it to vtable offset `0x288`. | raw block |
| 241 | `0040ff40` | `UpdateBaseballSprite` | Tail wrapper around inherited sprite/update helper `00463090(dt)`. | raw block |
| 257 | `00435430` | `C3DPermanentSprite::LoadPermanentSpriteCanvasWithBaseHook` | Inherited permanent-sprite canvas load with base hook. | inherited |
| 259 | `00410000` | `PostLoadBaseball` | Runs `C3DPermanentSprite::LoadPermanentSpriteCanvas`, then inherited sprite physics slot 11, then applies scalar `15.0` through inherited vtable offset `0x110`. | non-trivial |
| vtable 2 slot 2 | `0040fe70` | scalar deleting destructor | Adjusts from the secondary pointer, runs cleanup helper `0040fea0`, destroys the tail subobject at outer `0x618`, and frees the adjusted allocation when requested. | non-trivial |
| helper | `0040fea0` | `CleanupBaseball` | Reinstalls Baseball vtables during destruction and tail-jumps to inherited `C3DPermanentSprite` cleanup helper at `004353f0`. | non-trivial |

## Runtime Behavior

```c
C3DBaseball::CtorBaseball3BAS():
    C3DPermanentSprite::Ctor()
    baseball_runtime_link = null
    install_baseball_vtables()
    register_runtime_string("C3DBASEBALL")
    trace_constructor("C3DBaseball()")
    C3DSprite::InitObjectSprite()
    register_fourcc("3BAS")

    SpriteDatabase = "retainedsprites.omt"
    SpriteIndex = 2
    SpriteSize = 25

    db = lookup_permanent_sprite_database("retainedsprites.omt")
    for chunk_id in [2, 3, 4, 5]:
        canvas = load_canvas(db, chunk_id)
        mark_canvas_dirty(canvas)
        baseball_frame_canvases[chunk_id - 2] = canvas

    baseball_scale_or_weight = 1.0
    baseball_timer_or_state = 0
    baseball_frame_state = 0

    inherited_enable_or_visibility_slot(1)
    C3DSprite::InitSpriteProperties(25.0)
    inherited_scalar_slot_60(3.2)
    inherited_scalar_slot_68(15.0)
    inherited_transform_slot_108(0)
    inherited_bool_slot_36(0)
    inherited_transform_slot_52(0)
    inherited_scalar_slot_62(0.46)
    inherited_finalize_or_sync_slot()
```

```c
C3DBaseball::PostLoadBaseball():
    C3DPermanentSprite::LoadPermanentSpriteCanvas()
    C3DSprite::InitPhysicsSprite()
    inherited_scalar_slot_68(15.0)
```

```c
C3DBaseball::ApplyBaseballScaledVector():
    current = get_current_vector()
    inherited_finalize_or_sync_slot()
    set_vector_0x288({current.x, current.y * -0.65, current.z, 0.0})
```

The vector helper slots are still named behaviorally. They are stable enough to port as wrappers around inherited transform slots, but the exact `CGameObject`/`OMediaWorldPosition` method names need full base struct/method naming.

## Constants And Wiring

### `.gam` Placeable Properties

`3BAS` has a class-id row in `docs/_gam_classids.tsv`, but no current row in `docs/gam_schema.md`. The original levels appear to spawn baseball projectiles from gameplay code rather than serializing them as placeables.

| Property | Type | Offset | Range / Samples | Consuming Logic |
|---|---|---:|---|---|
| none | - | - | - | No current `.gam` instances or serialized Baseball-owned fields. |

### Runtime Constants

| Name / Id | Use | Evidence |
|---|---|---|
| `3BAS` | Concrete Baseball class id. | ctor `0040fc50`; `push 0x33424153` at `0040fd0d` |
| `C3DBASEBALL`, `C3DBaseball()` | Runtime class strings. | strings `.data:004ecc50` and `.data:004ed6a8`; constructor `0040fc50` |
| `retainedsprites.omt` | Primary retained-sprite database. | string `.data:004ed694`; constructor copy and lookup |
| chunk ids `2`, `3`, `4`, `5` | Preloaded baseball animation/frame canvases. | constructor calls `FUN_00477890(db, id)` four times |
| `25` / `25.0` | `SpriteSize` default and sprite-property setup scalar. | constructor outer `0x57c`; immediate `0x41c80000` |
| `3.2` | Inherited scalar applied through slot 60. | constructor immediate `0x404ccccd` |
| `15.0` | Inherited scalar applied during construction and post-load. | constructor and slot 259 immediate `0x41700000` |
| `0.46` | Inherited scalar applied through slot 62. | constructor immediate `0x3eeb851f` |
| `-0.65` | Y-scale used by slot 223 vector helper. | float at `.rdata:0049195c`; objdump `0040ff80` |

## Assets

| Kind | Name / Id | Source | Notes |
|---|---|---|---|
| OMT database | `retainedsprites.omt` | constructor `0040fc50`; parsed metadata `assets/parsed/RetainedSprites/RetainedSprites.json` | The constructor resolves this database with the permanent-sprite lookup helper. |
| retained sprite chunk | id `2` / `ball0000` | constructor; parsed `RetainedSprites.json` | Parsed at JSON index `6`; image `assets/parsed/RetainedSprites/RetainedSprites_images/0006_32x32d32.png`. |
| retained sprite chunk | id `3` / `ball0001` | constructor; parsed `RetainedSprites.json` | Parsed at JSON index `5`; image `assets/parsed/RetainedSprites/RetainedSprites_images/0005_32x32d32.png`. |
| retained sprite chunk | id `4` / `ball0002` | constructor; parsed `RetainedSprites.json` | Parsed at JSON index `4`; image `assets/parsed/RetainedSprites/RetainedSprites_images/0004_32x32d32.png`. |
| retained sprite chunk | id `5` / `ball0003` | constructor; parsed `RetainedSprites.json` | Parsed at JSON index `3`; image `assets/parsed/RetainedSprites/RetainedSprites_images/0003_32x32d32.png`. |

## Confidence

Confidence: Medium

Validation: Static Ghidra class dump, `DumpFunctions.java` constructor/slot 259 output, local `objdump` over raw helper slots and destructor window, string table checks, `.gam` schema cross-check, and parsed `RetainedSprites.omt` metadata only; not runtime-validated.

Open questions:
- Name inherited transform slots `0x288`, `0x2bc`, and the helpers at `00458890`, `00472690`, `00472970`, and `00463090`.
- Confirm whether fields at active `0x540..0x548` are animation-frame state, projectile lifetime state, or both.
- Runtime-check the projectile source and lifetime path from Jimmy/player inventory code.
- Verify the gameplay meaning of the `-0.65` Y-scale helper in an original-game throw.

## Notes

- Evidence: `DumpClass.java C3DBaseball /tmp/decomp_C3DBaseball.md` (`slots=335`, `owned_methods=1`, `offsets=0`), `DumpFunctions.java /tmp/decomp_C3DBaseball_raw.md` for the constructor and slot 259, local objdump windows `0040fc50..00410090` for destructor/helper slots, string scans, and parsed retained-sprite metadata.
- `docs/_gam_classids.tsv` already maps `3BAS -> C3DBaseball()`; no `tools/gam_schema.py` regeneration was needed because the current `.gam` corpus has no `3BAS` instances.
